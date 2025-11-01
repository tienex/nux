/*
** ==========================================================================
**
** JIT Backend - sljit-based Implementation
**
** Architecture-independent shader JIT compiler using sljit
**
** --------------------------------------------------------------------------
**
** Vincent 3D Rendering Library, Programmable Pipeline Edition
**
** Copyright (C) 2003-2007 Hans-Martin Will.
**
** @CDDL_HEADER_START@
**
** The contents of this file are subject to the terms of the
** Common Development and Distribution License, Version 1.0 only
** (the "License").  You may not use this file except in compliance
** with the License.
**
** You can obtain a copy of the license at
** http://www.vincent3d.com/software/ogles2/license/license.html
** See the License for the specific language governing permissions
** and limitations under the License.
**
** When distributing Covered Code, include this CDDL_HEADER in each
** file and include the License file named LICENSE.TXT in the root folder
** of your distribution.
** If applicable, add the following below this CDDL_HEADER, with the
** fields enclosed by brackets "[]" replaced with your own identifying
** information: Portions Copyright [yyyy] [name of copyright owner]
**
** @CDDL_HEADER_END@
**
** ==========================================================================
*/

#include <GLES/gl.h>
#include "config.h"
#include "platform/platform.h"
#include "gl/state.h"
#include "frontend/compiler.h"
#include "frontend/linker.h"
#include "frontend/il.h"
#include "frontend/memory.h"
#include "backend/jit.h"
#include "backend/jit_internal.h"
#include "backend/interpreter.h"

/* Include sljit for JIT compilation */
#define SLJIT_CONFIG_AUTO 1
#define SLJIT_VERBOSE 0
#include "../../../sljit/sljit_src/sljitLir.h"

#include <math.h>
#include <string.h>
#include <stddef.h>  /* For offsetof */

/* No helper functions needed - we call libm directly (sqrtf, floorf, etc.) */

/*
** --------------------------------------------------------------------------
** Constants
** --------------------------------------------------------------------------
*/

/* Register allocation for shader execution
 * These map to physical registers via sljit
 */
#define REG_CONTEXT     SLJIT_S0   /* Shader context pointer */
#define REG_TEMP1       SLJIT_R0   /* Temporary register 1 */
#define REG_TEMP2       SLJIT_R1   /* Temporary register 2 */
#define REG_TEMP3       SLJIT_R2   /* Temporary register 3 */

/*
** --------------------------------------------------------------------------
** Internal structures
** --------------------------------------------------------------------------
*/

typedef struct JitContext {
	struct sljit_compiler *compiler;
	Linker *linker;
	Memory *memory;
	ShaderProgram *program;

	/* Register allocation tracking */
	GLint nextTempReg;		/* Next available temp register */

	/* Shader type */
	GLboolean isFragmentShader;		/* TRUE for fragment, FALSE for vertex */
} JitContext;

/*
** --------------------------------------------------------------------------
** Internal functions
** --------------------------------------------------------------------------
*/

/**
 * Initialize a JIT context for shader compilation
 */
static GLboolean InitJitContext(JitContext *ctx, Linker *linker, GLboolean isFragmentShader) {
	ctx->compiler = sljit_create_compiler(NULL, NULL);
	if (!ctx->compiler) {
		return GL_FALSE;
	}

	ctx->linker = linker;
	ctx->memory = linker->resultMemory;
	ctx->program = NULL;
	ctx->nextTempReg = SLJIT_R2;  /* Start after REG_TEMP1 and REG_TEMP2 */
	ctx->isFragmentShader = isFragmentShader;

	return GL_TRUE;
}

/**
 * Cleanup JIT context
 */
static void CleanupJitContext(JitContext *ctx) {
	if (ctx->compiler) {
		sljit_free_compiler(ctx->compiler);
		ctx->compiler = NULL;
	}
}

/**
 * Generate prologue for shader function
 * Sets up stack frame and saves registers
 */
static GLboolean GeneratePrologue(JitContext *ctx) {
	struct sljit_compiler *C = ctx->compiler;

	/* Function entry */
	sljit_emit_enter(C, 0, SLJIT_ARGS1(W, P), 3, 1, 0, 0, 0);

	/* REG_CONTEXT = first argument (context pointer) */
	/* Already set by sljit_emit_enter with ARGS1(W, P) */

	return GL_TRUE;
}

/**
 * Generate epilogue for shader function
 * Returns success status and restores registers
 */
static GLboolean GenerateEpilogue(JitContext *ctx) {
	struct sljit_compiler *C = ctx->compiler;

	/* Return GL_TRUE (success) */
	sljit_emit_return(C, SLJIT_MOV, SLJIT_IMM, GL_TRUE);

	return GL_TRUE;
}

/**
 * Get context field base offset and variable offset
 * Returns the offset to the pointer field in the context structure,
 * and sets *pVarOffset to the offset within that array
 */
static GLsizei GetContextFieldOffset(const SrcRef *ref, GLsizei *pVarOffset, GLboolean isFragmentShader) {
	ProgVarBase *var = ref->base;
	GLsizei contextFieldOffset = 0;
	GLsizei varOffset = 0;

	if (!var) {
		*pVarOffset = 0;
		return 0;
	}

	/* Calculate offset to the pointer field in the context structure */
	switch (var->segment) {
		case ProgVarSegParam:
			/* Uniforms/parameters - context->uniform pointer */
			if (isFragmentShader) {
				contextFieldOffset = offsetof(FragContext, uniform);
			} else {
				contextFieldOffset = offsetof(VertexContext, uniform);
			}
			varOffset = var->location * sizeof(Vec4f);
			break;

		case ProgVarSegLocal:
			/* Temporaries - context->temp pointer */
			if (isFragmentShader) {
				contextFieldOffset = offsetof(FragContext, temp);
			} else {
				contextFieldOffset = offsetof(VertexContext, temp);
			}
			varOffset = var->location * sizeof(Vec4f);
			break;

		case ProgVarSegAttrib:
			/* Vertex attributes - only valid for vertex shaders */
			if (!isFragmentShader) {
				contextFieldOffset = offsetof(VertexContext, attrib);
				varOffset = var->location * sizeof(Vec4f);
			}
			break;

		case ProgVarSegVarying:
			/* Varyings - different type in vertex vs fragment */
			if (isFragmentShader) {
				contextFieldOffset = offsetof(FragContext, varying);
				varOffset = var->location * sizeof(GLfloat);
			} else {
				contextFieldOffset = offsetof(VertexContext, varying);
				varOffset = var->location * sizeof(GLfloat);
			}
			break;

		default:
			break;
	}

	*pVarOffset = varOffset;
	return contextFieldOffset;
}

/**
 * Load a shader variable component into a register
 * Handles swizzling and offset calculations
 */
static GLboolean LoadComponent(JitContext *ctx, sljit_s32 dstReg, const SrcReg *src, GLuint component, GLboolean isFragmentShader) {
	struct sljit_compiler *C = ctx->compiler;
	GLsizei contextFieldOffset;
	GLsizei varOffset;
	GLubyte swizzle;

	/* Get swizzle for this component */
	switch (component) {
		case 0: swizzle = src->selectX; break;
		case 1: swizzle = src->selectY; break;
		case 2: swizzle = src->selectZ; break;
		case 3: swizzle = src->selectW; break;
		default: return GL_FALSE;
	}

	/* Get context field offset and variable offset */
	contextFieldOffset = GetContextFieldOffset(&src->reference, &varOffset, isFragmentShader);

	/* Add component offset (each component is a float) */
	varOffset += swizzle * sizeof(GLfloat);

	/* Load pointer from context structure into REG_TEMP1
	 * REG_TEMP1 = *(context + contextFieldOffset)
	 */
	sljit_emit_op1(C, SLJIT_MOV_P,
				   REG_TEMP1, 0,
				   SLJIT_MEM1(REG_CONTEXT), contextFieldOffset);

	/* Load float from the array
	 * dstReg = *(REG_TEMP1 + varOffset)
	 */
	sljit_emit_fop1(C, SLJIT_MOV_F32,
					dstReg, 0,
					SLJIT_MEM1(REG_TEMP1), varOffset);

	/* Handle negation if needed */
	if (src->negate) {
		sljit_emit_fop1(C, SLJIT_NEG_F32, dstReg, 0, dstReg, 0);
	}

	return GL_TRUE;
}

/**
 * Apply saturation to a value (clamp to [0, 1])
 * Implementation: result = max(0, min(1, value))
 */
static void ApplySaturation(JitContext *ctx, sljit_s32 reg) {
	struct sljit_compiler *C = ctx->compiler;
	struct sljit_jump *jump1, *jump2;
	struct sljit_label *label1, *label2;

	/* Load 0.0 into FR3 for comparison */
	sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR3, 0, SLJIT_IMM, 0);

	/* Compare with 0.0: if (reg < 0.0) reg = 0.0 */
	sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_LESS_F,
					reg, 0, SLJIT_FR3, 0);

	jump1 = sljit_emit_jump(C, SLJIT_GREATER_EQUAL_F);
	/* Value is < 0, set to 0 */
	sljit_emit_fop1(C, SLJIT_MOV_F32, reg, 0, SLJIT_FR3, 0);
	label1 = sljit_emit_label(C);
	sljit_set_label(jump1, label1);

	/* Load 1.0 into FR3 for comparison */
	sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR3, 0, SLJIT_IMM, 0x3F800000);

	/* Compare with 1.0: if (reg > 1.0) reg = 1.0 */
	sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_GREATER_F,
					reg, 0, SLJIT_FR3, 0);

	jump2 = sljit_emit_jump(C, SLJIT_LESS_EQUAL_F);
	/* Value is > 1, set to 1 */
	sljit_emit_fop1(C, SLJIT_MOV_F32, reg, 0, SLJIT_FR3, 0);
	label2 = sljit_emit_label(C);
	sljit_set_label(jump2, label2);
}

/**
 * Store a register component to a shader variable
 * Handles write masks and offset calculations
 */
static GLboolean StoreComponent(JitContext *ctx, const DstReg *dst, sljit_s32 srcReg, GLuint component, GLboolean isFragmentShader) {
	struct sljit_compiler *C = ctx->compiler;
	GLsizei contextFieldOffset;
	GLsizei varOffset;
	GLboolean mask;

	/* Check write mask for this component */
	switch (component) {
		case 0: mask = dst->maskX; break;
		case 1: mask = dst->maskY; break;
		case 2: mask = dst->maskZ; break;
		case 3: mask = dst->maskW; break;
		default: return GL_FALSE;
	}

	/* Skip if component is masked out */
	if (!mask) {
		return GL_TRUE;
	}

	/* Get context field offset and variable offset */
	contextFieldOffset = GetContextFieldOffset((const SrcRef *)&dst->reference, &varOffset, isFragmentShader);

	/* Add component offset */
	varOffset += component * sizeof(GLfloat);

	/* Load pointer from context structure into REG_TEMP1
	 * REG_TEMP1 = *(context + contextFieldOffset)
	 */
	sljit_emit_op1(C, SLJIT_MOV_P,
				   REG_TEMP1, 0,
				   SLJIT_MEM1(REG_CONTEXT), contextFieldOffset);

	/* Store float to the array
	 * *(REG_TEMP1 + varOffset) = srcReg
	 */
	sljit_emit_fop1(C, SLJIT_MOV_F32,
					SLJIT_MEM1(REG_TEMP1), varOffset,
					srcReg, 0);

	return GL_TRUE;
}

/**
 * Translate a single IL instruction to sljit native code
 */
static GLboolean TranslateInstruction(JitContext *ctx, Inst *inst) {
	struct sljit_compiler *C = ctx->compiler;
	GLuint i;

	/* For now, we'll implement basic instruction translation
	 * Full implementation will handle all opcodes, swizzling, and write masks
	 */
	switch (inst->base.op) {
		case OpcodeMOV:
		case OpcodeMOV_SAT: {
			/* Move instruction: dst = src
			 * Process each component with swizzling and write masks
			 */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source component with swizzling */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Handle saturation for _SAT variant */
				if (inst->base.op == OpcodeMOV_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store to destination with write mask */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeADD:
		case OpcodeADD_SAT: {
			/* Add instruction: dst = left + right */
			InstBinary *binary = &inst->binary;

			for (i = 0; i < 4; i++) {
				/* Load left operand */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load right operand */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Add: FR0 = FR0 + FR1 */
				sljit_emit_fop2(C, SLJIT_ADD_F32,
								SLJIT_FR0, 0,
								SLJIT_FR0, 0,
								SLJIT_FR1, 0);

				/* Handle saturation */ if (inst->base.op == OpcodeADD_SAT) { ApplySaturation(ctx, SLJIT_FR0); }

				/* Store result */
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeMUL:
		case OpcodeMUL_SAT: {
			/* Multiply instruction: dst = left * right */
			InstBinary *binary = &inst->binary;

			for (i = 0; i < 4; i++) {
				/* Load left operand */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load right operand */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Multiply: FR0 = FR0 * FR1 */
				sljit_emit_fop2(C, SLJIT_MUL_F32,
								SLJIT_FR0, 0,
								SLJIT_FR0, 0,
								SLJIT_FR1, 0);

				/* Handle saturation */
				if (inst->base.op == OpcodeMUL_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeSUB:
		case OpcodeSUB_SAT: {
			/* Subtract instruction: dst = left - right */
			InstBinary *binary = &inst->binary;

			for (i = 0; i < 4; i++) {
				/* Load left operand */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load right operand */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Subtract: FR0 = FR0 - FR1 */
				sljit_emit_fop2(C, SLJIT_SUB_F32,
								SLJIT_FR0, 0,
								SLJIT_FR0, 0,
								SLJIT_FR1, 0);

				/* Handle saturation */
				if (inst->base.op == OpcodeSUB_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeMAD:
		case OpcodeMAD_SAT: {
			/* Multiply-add: dst = arg0 * arg1 + arg2 */
			InstTernary *ternary = &inst->ternary;

			for (i = 0; i < 4; i++) {
				/* Load arg0 */
				if (!LoadComponent(ctx, SLJIT_FR0, &ternary->arg0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load arg1 */
				if (!LoadComponent(ctx, SLJIT_FR1, &ternary->arg1, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Multiply: FR0 = arg0 * arg1 */
				sljit_emit_fop2(C, SLJIT_MUL_F32,
								SLJIT_FR0, 0,
								SLJIT_FR0, 0,
								SLJIT_FR1, 0);

				/* Load arg2 */
				if (!LoadComponent(ctx, SLJIT_FR1, &ternary->arg2, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Add: FR0 = FR0 + arg2 */
				sljit_emit_fop2(C, SLJIT_ADD_F32,
								SLJIT_FR0, 0,
								SLJIT_FR0, 0,
								SLJIT_FR1, 0);

				/* Handle saturation */
				if (inst->base.op == OpcodeMAD_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &ternary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeDP4:
		case OpcodeDP4_SAT: {
			/* 4-component dot product: dst = left.x*right.x + left.y*right.y + left.z*right.z + left.w*right.w */
			InstBinary *binary = &inst->binary;

			/* Initialize accumulator to 0 */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR2, 0, SLJIT_IMM, 0);

			for (i = 0; i < 4; i++) {
				/* Load left component */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load right component */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Multiply: FR0 = left[i] * right[i] */
				sljit_emit_fop2(C, SLJIT_MUL_F32,
								SLJIT_FR0, 0,
								SLJIT_FR0, 0,
								SLJIT_FR1, 0);

				/* Accumulate: FR2 = FR2 + FR0 */
				sljit_emit_fop2(C, SLJIT_ADD_F32,
								SLJIT_FR2, 0,
								SLJIT_FR2, 0,
								SLJIT_FR0, 0);
			}

			/* Handle saturation */
			if (inst->base.op == OpcodeDP4_SAT) {
				ApplySaturation(ctx, SLJIT_FR2);
			}

			/* Store result to all components (broadcast) */
			for (i = 0; i < 4; i++) {
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR2, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeDP3:
		case OpcodeDP3_SAT: {
			/* 3-component dot product */
			InstBinary *binary = &inst->binary;

			/* Initialize accumulator to 0 */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR2, 0, SLJIT_IMM, 0);

			for (i = 0; i < 3; i++) {
				/* Load left component */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load right component */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Multiply and accumulate */
				sljit_emit_fop2(C, SLJIT_MUL_F32,
								SLJIT_FR0, 0,
								SLJIT_FR0, 0,
								SLJIT_FR1, 0);

				sljit_emit_fop2(C, SLJIT_ADD_F32,
								SLJIT_FR2, 0,
								SLJIT_FR2, 0,
								SLJIT_FR0, 0);
			}

			/* Handle saturation */
			if (inst->base.op == OpcodeDP3_SAT) {
				ApplySaturation(ctx, SLJIT_FR2);
			}

			/* Store result (broadcast to all components) */
			for (i = 0; i < 4; i++) {
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR2, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeRCP:
		case OpcodeRCP_SAT: {
			/* Reciprocal: dst = 1.0 / src */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load 1.0 into FR0 */
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0x3FF0000000000000LL);

				/* Load source into FR1 */
				if (!LoadComponent(ctx, SLJIT_FR1, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Divide: FR0 = 1.0 / src */
				sljit_emit_fop2(C, SLJIT_DIV_F32,
								SLJIT_FR0, 0,
								SLJIT_FR0, 0,
								SLJIT_FR1, 0);

				/* Handle saturation */
				if (inst->base.op == OpcodeRCP_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeRSQ:
		case OpcodeRSQ_SAT: {
			/* Reciprocal square root: dst = 1.0 / sqrt(src)
			 * Call sqrtf from libm, then divide
			 */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Call sqrtf from libm */
				sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(sqrtf));

				/* Result is in FR0, now compute 1.0 / sqrt */
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR1, 0, SLJIT_IMM, 0x3F800000); /* 1.0 */
				sljit_emit_fop2(C, SLJIT_DIV_F32, SLJIT_FR0, 0, SLJIT_FR1, 0, SLJIT_FR0, 0);

				/* Handle saturation */
				if (inst->base.op == OpcodeRSQ_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeABS:
		case OpcodeABS_SAT: {
			/* Absolute value: dst = abs(src) */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Compute absolute value using SLJIT_ABS_F32 */
				sljit_emit_fop1(C, SLJIT_ABS_F32,
								SLJIT_FR0, 0,
								SLJIT_FR0, 0);

				/* Handle saturation */
				if (inst->base.op == OpcodeABS_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeMIN:
		case OpcodeMIN_SAT: {
			/* Minimum: dst = min(left, right) */
			InstBinary *binary = &inst->binary;

			for (i = 0; i < 4; i++) {
				/* Load left operand */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load right operand */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Compare and select minimum
				 * FR0 = (FR0 < FR1) ? FR0 : FR1
				 */
				sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_LESS_F,
								SLJIT_FR0, 0,
								SLJIT_FR1, 0);

				/* Use conditional move or fselect if available
				 * For simplicity, use conditional logic
				 */
				struct sljit_jump *jump = sljit_emit_jump(C, SLJIT_LESS_F);
				sljit_emit_fop1(C, SLJIT_MOV_F32,
								SLJIT_FR0, 0,
								SLJIT_FR1, 0);
				sljit_set_label(jump, sljit_emit_label(C));

				/* Handle saturation */
				if (inst->base.op == OpcodeMIN_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeMAX:
		case OpcodeMAX_SAT: {
			/* Maximum: dst = max(left, right) */
			InstBinary *binary = &inst->binary;

			for (i = 0; i < 4; i++) {
				/* Load left operand */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load right operand */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Compare and select maximum
				 * FR0 = (FR0 > FR1) ? FR0 : FR1
				 */
				sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_GREATER_F,
								SLJIT_FR0, 0,
								SLJIT_FR1, 0);

				struct sljit_jump *jump = sljit_emit_jump(C, SLJIT_GREATER_F);
				sljit_emit_fop1(C, SLJIT_MOV_F32,
								SLJIT_FR0, 0,
								SLJIT_FR1, 0);
				sljit_set_label(jump, sljit_emit_label(C));

				/* Handle saturation */
				if (inst->base.op == OpcodeMAX_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeFLR:
		case OpcodeFLR_SAT: {
			/* Floor: dst = floor(src) - call floorf from libm */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Call floorf from libm */
				sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(floorf));

				/* Handle saturation */
				if (inst->base.op == OpcodeFLR_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeFRC:
		case OpcodeFRC_SAT: {
			/* Fraction: dst = frac(src) = src - floor(src)
			 * Inline implementation: call floorf then subtract
			 */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source into FR0 (this is x) */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Save original value to FR1 */
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR1, 0, SLJIT_FR0, 0);

				/* Call floorf to get floor(x) in FR0 */
				sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(floorf));

				/* Compute x - floor(x): FR0 = FR1 - FR0 */
				sljit_emit_fop2(C, SLJIT_SUB_F32, SLJIT_FR0, 0, SLJIT_FR1, 0, SLJIT_FR0, 0);

				/* Handle saturation */
				if (inst->base.op == OpcodeFRC_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeSGE: {
			/* Set on Greater or Equal: dst = (left >= right) ? 1.0 : 0.0 */
			InstBinary *binary = &inst->binary;

			for (i = 0; i < 4; i++) {
				/* Load left operand */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load right operand */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Compare: FR0 >= FR1 */
				sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_GREATER_EQUAL_F,
								SLJIT_FR0, 0, SLJIT_FR1, 0);

				/* Set result based on comparison */
				struct sljit_jump *jump = sljit_emit_jump(C, SLJIT_GREATER_EQUAL_F);
				/* False case: set to 0.0 */
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0);
				struct sljit_jump *jump_end = sljit_emit_jump(C, SLJIT_JUMP);
				/* True case: set to 1.0 */
				sljit_set_label(jump, sljit_emit_label(C));
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0x3F800000);
				sljit_set_label(jump_end, sljit_emit_label(C));

				/* Store result */
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeSLT: {
			/* Set on Less Than: dst = (left < right) ? 1.0 : 0.0 */
			InstBinary *binary = &inst->binary;

			for (i = 0; i < 4; i++) {
				/* Load left operand */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load right operand */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Compare: FR0 < FR1 */
				sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_LESS_F,
								SLJIT_FR0, 0, SLJIT_FR1, 0);

				/* Set result based on comparison */
				struct sljit_jump *jump = sljit_emit_jump(C, SLJIT_LESS_F);
				/* False case: set to 0.0 */
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0);
				struct sljit_jump *jump_end = sljit_emit_jump(C, SLJIT_JUMP);
				/* True case: set to 1.0 */
				sljit_set_label(jump, sljit_emit_label(C));
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0x3F800000);
				sljit_set_label(jump_end, sljit_emit_label(C));

				/* Store result */
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeSEQ: {
			/* Set on Equal: dst = (left == right) ? 1.0 : 0.0 */
			InstBinary *binary = &inst->binary;

			for (i = 0; i < 4; i++) {
				/* Load left operand */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load right operand */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Compare: FR0 == FR1 */
				sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_EQUAL_F,
								SLJIT_FR0, 0, SLJIT_FR1, 0);

				/* Set result based on comparison */
				struct sljit_jump *jump = sljit_emit_jump(C, SLJIT_EQUAL_F);
				/* False case: set to 0.0 */
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0);
				struct sljit_jump *jump_end = sljit_emit_jump(C, SLJIT_JUMP);
				/* True case: set to 1.0 */
				sljit_set_label(jump, sljit_emit_label(C));
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0x3F800000);
				sljit_set_label(jump_end, sljit_emit_label(C));

				/* Store result */
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeSNE: {
			/* Set on Not Equal: dst = (left != right) ? 1.0 : 0.0 */
			InstBinary *binary = &inst->binary;

			for (i = 0; i < 4; i++) {
				/* Load left operand */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load right operand */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Compare: FR0 != FR1 */
				sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_NOT_EQUAL_F,
								SLJIT_FR0, 0, SLJIT_FR1, 0);

				/* Set result based on comparison */
				struct sljit_jump *jump = sljit_emit_jump(C, SLJIT_NOT_EQUAL_F);
				/* False case: set to 0.0 */
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0);
				struct sljit_jump *jump_end = sljit_emit_jump(C, SLJIT_JUMP);
				/* True case: set to 1.0 */
				sljit_set_label(jump, sljit_emit_label(C));
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0x3F800000);
				sljit_set_label(jump_end, sljit_emit_label(C));

				/* Store result */
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeCMP:
		case OpcodeCMP_SAT: {
			/* Compare: dst = (src0 < 0) ? src1 : src2 */
			InstTernary *ternary = &inst->ternary;

			for (i = 0; i < 4; i++) {
				/* Load src0 (compare operand) */
				if (!LoadComponent(ctx, SLJIT_FR0, &ternary->arg1, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load src1 (true branch) */
				if (!LoadComponent(ctx, SLJIT_FR1, &ternary->arg2, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load src2 (false branch) */
				if (!LoadComponent(ctx, SLJIT_FR2, &ternary->arg3, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Compare FR0 < 0 */
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR3, 0, SLJIT_IMM, 0); /* 0.0 */
				sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_LESS_F, SLJIT_FR0, 0, SLJIT_FR3, 0);

				/* Select result based on comparison */
				struct sljit_jump *jump = sljit_emit_jump(C, SLJIT_LESS_F);
				/* False case: result = src2 */
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_FR2, 0);
				struct sljit_jump *jump_end = sljit_emit_jump(C, SLJIT_JUMP);
				/* True case: result = src1 */
				sljit_set_label(jump, sljit_emit_label(C));
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_FR1, 0);
				sljit_set_label(jump_end, sljit_emit_label(C));

				/* Handle saturation */
				if (inst->base.op == OpcodeCMP_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &ternary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeLRP:
		case OpcodeLRP_SAT: {
			/* Linear interpolation: dst = src0 * src1 + (1 - src0) * src2 */
			InstTernary *ternary = &inst->ternary;

			for (i = 0; i < 4; i++) {
				/* Load src0 (interpolation factor) */
				if (!LoadComponent(ctx, SLJIT_FR0, &ternary->arg1, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load src1 (first value) */
				if (!LoadComponent(ctx, SLJIT_FR1, &ternary->arg2, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load src2 (second value) */
				if (!LoadComponent(ctx, SLJIT_FR2, &ternary->arg3, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Compute src0 * src1 */
				sljit_emit_fop2(C, SLJIT_MUL_F32, SLJIT_FR3, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);

				/* Compute 1 - src0 */
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR4, 0, SLJIT_IMM, 0x3F800000); /* 1.0 */
				sljit_emit_fop2(C, SLJIT_SUB_F32, SLJIT_FR4, 0, SLJIT_FR4, 0, SLJIT_FR0, 0);

				/* Compute (1 - src0) * src2 */
				sljit_emit_fop2(C, SLJIT_MUL_F32, SLJIT_FR4, 0, SLJIT_FR4, 0, SLJIT_FR2, 0);

				/* Add results: src0 * src1 + (1 - src0) * src2 */
				sljit_emit_fop2(C, SLJIT_ADD_F32, SLJIT_FR0, 0, SLJIT_FR3, 0, SLJIT_FR4, 0);

				/* Handle saturation */
				if (inst->base.op == OpcodeLRP_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &ternary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeXPD:
		case OpcodeXPD_SAT: {
			/* Cross product: dst = src0 x src1
			 * result.x = src0.y * src1.z - src0.z * src1.y
			 * result.y = src0.z * src1.x - src0.x * src1.z
			 * result.z = src0.x * src1.y - src0.y * src1.x
			 * result.w = 1.0
			 */
			InstBinary *binary = &inst->binary;

			/* X component: src0.y * src1.z - src0.z * src1.y */
			if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, 1, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, 2, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			sljit_emit_fop2(C, SLJIT_MUL_F32, SLJIT_FR2, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);

			if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, 2, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, 1, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			sljit_emit_fop2(C, SLJIT_MUL_F32, SLJIT_FR3, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);
			sljit_emit_fop2(C, SLJIT_SUB_F32, SLJIT_FR4, 0, SLJIT_FR2, 0, SLJIT_FR3, 0);

			if (inst->base.op == OpcodeXPD_SAT) {
				ApplySaturation(ctx, SLJIT_FR4);
			}
			if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR4, 0, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* Y component: src0.z * src1.x - src0.x * src1.z */
			if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, 2, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, 0, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			sljit_emit_fop2(C, SLJIT_MUL_F32, SLJIT_FR2, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);

			if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, 0, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, 2, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			sljit_emit_fop2(C, SLJIT_MUL_F32, SLJIT_FR3, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);
			sljit_emit_fop2(C, SLJIT_SUB_F32, SLJIT_FR4, 0, SLJIT_FR2, 0, SLJIT_FR3, 0);

			if (inst->base.op == OpcodeXPD_SAT) {
				ApplySaturation(ctx, SLJIT_FR4);
			}
			if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR4, 1, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* Z component: src0.x * src1.y - src0.y * src1.x */
			if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, 0, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, 1, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			sljit_emit_fop2(C, SLJIT_MUL_F32, SLJIT_FR2, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);

			if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, 1, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, 0, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			sljit_emit_fop2(C, SLJIT_MUL_F32, SLJIT_FR3, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);
			sljit_emit_fop2(C, SLJIT_SUB_F32, SLJIT_FR4, 0, SLJIT_FR2, 0, SLJIT_FR3, 0);

			if (inst->base.op == OpcodeXPD_SAT) {
				ApplySaturation(ctx, SLJIT_FR4);
			}
			if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR4, 2, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* W component: 1.0 */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0x3F800000);
			if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, 3, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			break;
		}

		case OpcodePOW: {
			/* Power: dst = src0 ^ src1 - call powf from libm */
			InstBinary *binary = &inst->binary;

			for (i = 0; i < 4; i++) {
				/* Load base (src0) into FR0 */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load exponent (src1) into FR1 */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Call powf(base, exp) */
				sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2(F32, F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(powf));

				/* Store result */
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeSIN:
		case OpcodeSIN_SAT: {
			/* Sine: dst = sin(src) - call sinf from libm */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Call sinf */
				sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(sinf));

				/* Handle saturation */
				if (inst->base.op == OpcodeSIN_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeCOS:
		case OpcodeCOS_SAT: {
			/* Cosine: dst = cos(src) - call cosf from libm */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Call cosf */
				sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(cosf));

				/* Handle saturation */
				if (inst->base.op == OpcodeCOS_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeEXP:
		case OpcodeEXP_SAT: {
			/* Exponential base 2: dst = 2^src - call exp2f from libm */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Call exp2f */
				sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(exp2f));

				/* Handle saturation */
				if (inst->base.op == OpcodeEXP_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeLOG:
		case OpcodeLOG_SAT: {
			/* Logarithm base 2: dst = log2(src) - call log2f from libm */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Call log2f */
				sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(log2f));

				/* Handle saturation */
				if (inst->base.op == OpcodeLOG_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeTEX:
		case OpcodeTEX_SAT:
		case OpcodeTXB:
		case OpcodeTXB_SAT:
		case OpcodeTXP:
		case OpcodeTXP_SAT:
		case OpcodeTXL:
		case OpcodeTXL_SAT:
			/* Texture sampling operations
			 * These require calling into runtime texture sampling functions
			 * Fall back to interpreter for now
			 */
			return GL_FALSE;

		/* Control flow and other complex operations - fall back to interpreter */
		case OpcodeCAL:
		case OpcodeRET:
		case OpcodeBRK:
		case OpcodeIF:
		case OpcodeELSE:
		case OpcodeENDIF:
		case OpcodeLOOP:
		case OpcodeENDLOOP:
		case OpcodeREP:
		case OpcodeENDREP:
		case OpcodeBRA:
		case OpcodeKIL:
		case OpcodeSCC:
		case OpcodePHI:
			return GL_FALSE;

		/* Declaration instructions - these are handled during linking, not execution */
		case OpcodeOUTPUT:
		case OpcodePARAM:
		case OpcodeTEMP:
		case OpcodeADDRESS:
			/* These are metadata, not executable instructions */
			break;

		default:
			/* Unsupported instruction - use interpreter fallback */
			return GL_FALSE;
	}

	return GL_TRUE;
}

/**
 * Compile shader IL to native code using sljit
 *
 * Traverses the shader program's IL blocks and instructions,
 * translating each to native code via sljit.
 */
static void *CompileShaderWithSljit(JitContext *ctx, ShaderProgram *program) {
	struct sljit_compiler *C = ctx->compiler;
	Block *block;
	Inst *inst;
	void *code;
	GLboolean success = GL_TRUE;

	if (!program) {
		return NULL;
	}

	/* Generate function prologue */
	if (!GeneratePrologue(ctx)) {
		return NULL;
	}

	/* Traverse IL blocks and instructions
	 * Each block is a linear sequence of instructions
	 * Instructions within a block form a doubly-linked list
	 */
	for (block = program->blocks.head; block; block = block->next) {
		/* Iterate through instructions in this block */
		for (inst = block->first; inst; inst = inst->base.next) {
			/* Translate instruction to native code */
			if (!TranslateInstruction(ctx, inst)) {
				/* Translation failed for this instruction
				 * Fall back to interpreter for now
				 */
				success = GL_FALSE;
				break;
			}
		}

		if (!success) {
			break;
		}
	}

	/* If translation failed, return NULL to use interpreter fallback */
	if (!success) {
		return NULL;
	}

	/* Generate function epilogue */
	if (!GenerateEpilogue(ctx)) {
		return NULL;
	}

	/* Generate executable code */
	code = sljit_generate_code(C, 0);
	if (!code) {
		return NULL;
	}

	return code;
}

/**
 * Compile vertex shader IL to native code
 */
static void *CompileVertexShader(Linker *linker) {
	JitContext ctx;
	void *code = NULL;

	if (!InitJitContext(&ctx, linker, GL_FALSE)) {  /* GL_FALSE = vertex shader */
		return NULL;
	}

	/* Get vertex shader IL from linker and compile it */
	if (linker->vertex) {
		code = CompileShaderWithSljit(&ctx, linker->vertex);
	}

	CleanupJitContext(&ctx);
	return code;
}

/**
 * Compile fragment shader IL to native code
 */
static void *CompileFragmentShader(Linker *linker) {
	JitContext ctx;
	void *code = NULL;

	if (!InitJitContext(&ctx, linker, GL_TRUE)) {  /* GL_TRUE = fragment shader */
		return NULL;
	}

	/* Get fragment shader IL from linker and compile it */
	if (linker->fragment) {
		code = CompileShaderWithSljit(&ctx, linker->fragment);
	}

	CleanupJitContext(&ctx);
	return code;
}

/* Interpreter functions moved to interpreter.c */

/**
 * Generate an executable from linked shader IL
 * Uses sljit for JIT compilation with interpreter fallback
 */
Executable *GlesGenerateExecutable(Linker *linker) {
	Executable *executable;
	Memory *memory;
	void *vertexCode;
	void *fragmentCode;

	if (!linker || !linker->program) {
		return NULL;
	}

	memory = linker->resultMemory;
	if (!memory) {
		return NULL;
	}

	/* Allocate executable structure */
	executable = GlesMemoryPoolAllocate(memory, sizeof(Executable));
	if (!executable) {
		return NULL;
	}

	GlesMemset(executable, 0, sizeof(Executable));

	/* Copy metadata from linker */
	executable->numUniforms = linker->numUniforms;
	executable->uniforms = linker->uniforms;
	executable->numVertexAttribs = linker->numAttribs;
	executable->attribs = linker->attribs;
	executable->numVarying = linker->numVarying;
	executable->sizeUniforms = linker->sizeUniforms;

	/* Try to compile vertex shader to native code using sljit
	 * Fall back to interpreter if JIT compilation fails
	 */
	vertexCode = CompileVertexShader(linker);
	if (vertexCode) {
		executable->vertex.code.base = vertexCode;
		executable->vertex.code.size = 0; /* Size managed by sljit */
	} else {
		/* JIT failed, use interpreter */
		executable->vertex.code.base = (void *)GlesInterpretVertexShader;
		executable->vertex.code.size = 0;
	}

	/* Try to compile fragment shader to native code using sljit
	 * Fall back to interpreter if JIT compilation fails
	 */
	fragmentCode = CompileFragmentShader(linker);
	if (fragmentCode) {
		executable->fragment.code.base = fragmentCode;
		executable->fragment.code.size = 0; /* Size managed by sljit */
	} else {
		/* JIT failed, use interpreter */
		executable->fragment.code.base = (void *)GlesInterpretFragmentShader;
		executable->fragment.code.size = 0;
	}

	/* Initialize data and bss segments */
	executable->vertex.data.base = NULL;
	executable->vertex.data.size = 0;
	executable->vertex.bssSize = 0;
	executable->fragment.data.base = NULL;
	executable->fragment.data.size = 0;
	executable->fragment.bssSize = 0;

	/* Architecture-specific optimization hook */
	GlesOptimizeExecutable(executable, linker);

	return executable;
}

/*
** --------------------------------------------------------------------------
** Exported functions
** --------------------------------------------------------------------------
*/

GLboolean GlesJitProgram(State *state, Program *program) {
	/* This function is called to JIT-compile a shader program
	 * For now, we use the interpreter-based backend
	 */
	return GL_TRUE;
}
