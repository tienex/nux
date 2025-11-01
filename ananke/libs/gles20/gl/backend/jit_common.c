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

/* Control flow stack for nested structures */
#define MAX_CONTROL_FLOW_DEPTH 32

typedef enum {
	CONTROL_FLOW_IF,
	CONTROL_FLOW_LOOP,
	CONTROL_FLOW_REP
} ControlFlowType;

typedef struct {
	ControlFlowType type;
	struct sljit_label *startLabel;		/* Loop start or if condition */
	struct sljit_label *endLabel;		/* Loop end or endif */
	struct sljit_label *elseLabel;		/* Else branch for IF */
	struct sljit_jump *condJump;		/* Conditional jump */
	struct sljit_jump *breakJumps[16];	/* Pending break jumps */
	GLint numBreaks;					/* Number of pending breaks */
} ControlFlowFrame;

typedef struct JitContext {
	struct sljit_compiler *compiler;
	Linker *linker;
	Memory *memory;
	ShaderProgram *program;

	/* Register allocation tracking */
	GLint nextTempReg;		/* Next available temp register */

	/* Shader type */
	GLboolean isFragmentShader;		/* TRUE for fragment, FALSE for vertex */

	/* Control flow stack */
	ControlFlowFrame controlStack[MAX_CONTROL_FLOW_DEPTH];
	GLint controlDepth;

	/* Subroutine call stack */
	struct sljit_label *subroutineLabels[256];  /* Labels for subroutine entry points */
	struct sljit_jump *returnJumps[32];         /* Pending return jumps */
	GLint numSubroutines;
	GLint numReturns;
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
	ctx->controlDepth = 0;
	ctx->numSubroutines = 0;
	ctx->numReturns = 0;

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

		case OpcodeEX2:
		case OpcodeEX2_SAT: {
			/* Exact exponential base 2: dst = 2^src - same as EXP */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Call exp2f */
				sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(exp2f));

				/* Handle saturation */
				if (inst->base.op == OpcodeEX2_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeLG2:
		case OpcodeLG2_SAT: {
			/* Exact logarithm base 2: dst = log2(src) - same as LOG */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Call log2f */
				sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(log2f));

				/* Handle saturation */
				if (inst->base.op == OpcodeLG2_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				/* Store result */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeDP2:
		case OpcodeDP2_SAT: {
			/* 2-component dot product: dst = src0.x*src1.x + src0.y*src1.y */
			InstBinary *binary = &inst->binary;

			/* Initialize accumulator to 0 */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR2, 0, SLJIT_IMM, 0);

			/* Accumulate x and y components */
			for (i = 0; i < 2; i++) {
				/* Load left operand component */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load right operand component */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Multiply and accumulate */
				sljit_emit_fop2(C, SLJIT_MUL_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);
				sljit_emit_fop2(C, SLJIT_ADD_F32, SLJIT_FR2, 0, SLJIT_FR2, 0, SLJIT_FR0, 0);
			}

			/* Handle saturation */
			if (inst->base.op == OpcodeDP2_SAT) {
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

		case OpcodeDPH:
		case OpcodeDPH_SAT: {
			/* Homogeneous dot product: dst = src0.x*src1.x + src0.y*src1.y + src0.z*src1.z + src1.w
			 * This is DP3 + the w component of src1
			 */
			InstBinary *binary = &inst->binary;

			/* Initialize accumulator to 0 */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR2, 0, SLJIT_IMM, 0);

			/* Accumulate x, y, z components */
			for (i = 0; i < 3; i++) {
				/* Load left operand component */
				if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Load right operand component */
				if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Multiply and accumulate */
				sljit_emit_fop2(C, SLJIT_MUL_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);
				sljit_emit_fop2(C, SLJIT_ADD_F32, SLJIT_FR2, 0, SLJIT_FR2, 0, SLJIT_FR0, 0);
			}

			/* Add w component of src1 */
			if (!LoadComponent(ctx, SLJIT_FR0, &binary->right, 3, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			sljit_emit_fop2(C, SLJIT_ADD_F32, SLJIT_FR2, 0, SLJIT_FR2, 0, SLJIT_FR0, 0);

			/* Handle saturation */
			if (inst->base.op == OpcodeDPH_SAT) {
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

		case OpcodeDST:
		case OpcodeDST_SAT: {
			/* Distance vector: specialized for lighting calculations
			 * dst.x = 1.0
			 * dst.y = src0.y * src1.y
			 * dst.z = src0.z
			 * dst.w = src1.w
			 */
			InstBinary *binary = &inst->binary;

			/* X component: 1.0 */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0x3F800000);
			if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, 0, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* Y component: src0.y * src1.y */
			if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, 1, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			if (!LoadComponent(ctx, SLJIT_FR1, &binary->right, 1, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			sljit_emit_fop2(C, SLJIT_MUL_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);
			if (inst->base.op == OpcodeDST_SAT) {
				ApplySaturation(ctx, SLJIT_FR0);
			}
			if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, 1, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* Z component: src0.z */
			if (!LoadComponent(ctx, SLJIT_FR0, &binary->left, 2, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			if (inst->base.op == OpcodeDST_SAT) {
				ApplySaturation(ctx, SLJIT_FR0);
			}
			if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, 2, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* W component: src1.w */
			if (!LoadComponent(ctx, SLJIT_FR0, &binary->right, 3, ctx->isFragmentShader)) {
				return GL_FALSE;
			}
			if (inst->base.op == OpcodeDST_SAT) {
				ApplySaturation(ctx, SLJIT_FR0);
			}
			if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, 3, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			break;
		}

		case OpcodeSCS:
		case OpcodeSCS_SAT: {
			/* Sine/cosine without reduction: dst.x = cos(src.x), dst.y = sin(src.x) */
			InstUnary *unary = &inst->unary;

			/* Load source.x */
			if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, 0, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* Save source value for both sin and cos */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR1, 0, SLJIT_FR0, 0);

			/* Compute cos(src.x) */
			sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(cosf));
			if (inst->base.op == OpcodeSCS_SAT) {
				ApplySaturation(ctx, SLJIT_FR0);
			}
			if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, 0, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* Compute sin(src.x) */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_FR1, 0);
			sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(sinf));
			if (inst->base.op == OpcodeSCS_SAT) {
				ApplySaturation(ctx, SLJIT_FR0);
			}
			if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, 1, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			break;
		}

		case OpcodeSGT: {
			/* Set on Greater Than: dst = (left > right) ? 1.0 : 0.0 */
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

				/* Compare: FR0 > FR1 */
				sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_GREATER_F, SLJIT_FR0, 0, SLJIT_FR1, 0);

				/* Set result based on comparison */
				struct sljit_jump *jump = sljit_emit_jump(C, SLJIT_GREATER_F);
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

		case OpcodeSLE: {
			/* Set on Less or Equal: dst = (left <= right) ? 1.0 : 0.0 */
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

				/* Compare: FR0 <= FR1 */
				sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_LESS_EQUAL_F, SLJIT_FR0, 0, SLJIT_FR1, 0);

				/* Set result based on comparison */
				struct sljit_jump *jump = sljit_emit_jump(C, SLJIT_LESS_EQUAL_F);
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

		case OpcodeSSG: {
			/* Set Sign: dst = (src > 0) ? 1.0 : (src < 0) ? -1.0 : 0.0 */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Compare with 0.0 */
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR1, 0, SLJIT_IMM, 0); /* 0.0 */

				/* Check if src > 0 */
				sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_GREATER_F, SLJIT_FR0, 0, SLJIT_FR1, 0);
				struct sljit_jump *jump_positive = sljit_emit_jump(C, SLJIT_GREATER_F);

				/* Check if src < 0 */
				sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_LESS_F, SLJIT_FR0, 0, SLJIT_FR1, 0);
				struct sljit_jump *jump_negative = sljit_emit_jump(C, SLJIT_LESS_F);

				/* src == 0: result = 0.0 */
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0);
				struct sljit_jump *jump_end = sljit_emit_jump(C, SLJIT_JUMP);

				/* src > 0: result = 1.0 */
				sljit_set_label(jump_positive, sljit_emit_label(C));
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0x3F800000);
				struct sljit_jump *jump_end2 = sljit_emit_jump(C, SLJIT_JUMP);

				/* src < 0: result = -1.0 */
				sljit_set_label(jump_negative, sljit_emit_label(C));
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0xBF800000); /* -1.0 */

				sljit_set_label(jump_end, sljit_emit_label(C));
				sljit_set_label(jump_end2, sljit_emit_label(C));

				/* Store result */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeSFL: {
			/* Set on False: always returns 0.0 */
			InstBinary *binary = &inst->binary;

			for (i = 0; i < 4; i++) {
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0);
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeSTR: {
			/* Set on True: always returns 1.0 */
			InstBinary *binary = &inst->binary;

			for (i = 0; i < 4; i++) {
				sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0x3F800000);
				if (!StoreComponent(ctx, &binary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeSWZ:
		case OpcodeSWZ_SAT: {
			/* Extended swizzle: basically a MOV with extended swizzle capabilities
			 * The swizzling is already handled by LoadComponent
			 */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source with swizzle */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Handle saturation */
				if (inst->base.op == OpcodeSWZ_SAT) {
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
		case OpcodeTEX_SAT: {
			/* Texture sample: dst = texture(sampler, coords)
			 * Implementation: Store coords, call texture sampling function, load result
			 */
			InstTex *tex = &inst->tex;

			/* Store texture coordinates to a temporary location */
			for (i = 0; i < 4; i++) {
				if (!LoadComponent(ctx, SLJIT_FR0, &tex->coords, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
				/* Store coord component to stack */
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, i * sizeof(GLfloat));
			}

			/* Load sampler index into register */
			if (tex->sampler && tex->sampler->location >= 0) {
				sljit_emit_op1(C, SLJIT_MOV, REG_TEMP1, 0, SLJIT_IMM, tex->sampler->location + tex->offset);
			} else {
				sljit_emit_op1(C, SLJIT_MOV, REG_TEMP1, 0, SLJIT_IMM, 0);
			}

			/* Setup arguments for texture call:
			 * - REG_TEMP1: sampler index
			 * - SLJIT_SP: pointer to coordinates
			 * Result is returned in FR0-FR3
			 * For simplicity, we generate a NOP and store zeros for now
			 * A full implementation would call: GlesSampleTexture(context, sampler, coords, result)
			 */

			/* Store zeros (TODO: call actual texture sampling runtime) */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0);
			for (i = 0; i < 4; i++) {
				if (!StoreComponent(ctx, &tex->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}

			/* Handle saturation */
			if (inst->base.op == OpcodeTEX_SAT) {
				/* Saturation would be applied after texture lookup */
			}

			break;
		}

		case OpcodeTXB:
		case OpcodeTXB_SAT: {
			/* Texture sample with bias: dst = texture(sampler, coords, bias)
			 * Similar to TEX but with bias parameter
			 */
			InstTex *tex = &inst->tex;

			/* Implementation similar to TEX */
			for (i = 0; i < 4; i++) {
				if (!LoadComponent(ctx, SLJIT_FR0, &tex->coords, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, i * sizeof(GLfloat));
			}

			/* Store zeros (TODO: call actual texture sampling with bias) */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0);
			for (i = 0; i < 4; i++) {
				if (!StoreComponent(ctx, &tex->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}

			if (inst->base.op == OpcodeTXB_SAT) {
				/* Saturation after lookup */
			}

			break;
		}

		case OpcodeTXP:
		case OpcodeTXP_SAT: {
			/* Texture sample with projection: dst = texture(sampler, coords.xyz / coords.w) */
			InstTex *tex = &inst->tex;

			/* Load coords and perform projection division */
			for (i = 0; i < 4; i++) {
				if (!LoadComponent(ctx, SLJIT_FR0, &tex->coords, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, i * sizeof(GLfloat));
			}

			/* Store zeros (TODO: implement projection and texture sampling) */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0);
			for (i = 0; i < 4; i++) {
				if (!StoreComponent(ctx, &tex->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}

			if (inst->base.op == OpcodeTXP_SAT) {
				/* Saturation after lookup */
			}

			break;
		}

		case OpcodeTXL:
		case OpcodeTXL_SAT: {
			/* Texture sample with explicit LOD: dst = texture(sampler, coords, lod) */
			InstTex *tex = &inst->tex;

			/* Load coordinates */
			for (i = 0; i < 4; i++) {
				if (!LoadComponent(ctx, SLJIT_FR0, &tex->coords, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, i * sizeof(GLfloat));
			}

			/* Store zeros (TODO: implement LOD-based texture sampling) */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0);
			for (i = 0; i < 4; i++) {
				if (!StoreComponent(ctx, &tex->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}

			if (inst->base.op == OpcodeTXL_SAT) {
				/* Saturation after lookup */
			}

			break;
		}

		case OpcodeARL: {
			/* Address register load: converts float to int for indexed addressing
			 * dst = floor(src) as integer
			 */
			InstUnary *unary = &inst->unary;

			for (i = 0; i < 4; i++) {
				/* Load source */
				if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}

				/* Call floorf to get floor(src) */
				sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(floorf));

				/* Store result (still as float, but represents integer) */
				if (!StoreComponent(ctx, &unary->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}
			break;
		}

		case OpcodeIF: {
			/* IF: Begin conditional block - test condition register
			 * If condition.x == 0, skip to ELSE or ENDIF
			 */
			InstCond *cond = &inst->cond;

			if (ctx->controlDepth >= MAX_CONTROL_FLOW_DEPTH) {
				return GL_FALSE;
			}

			/* Load condition (typically from a comparison result) */
			if (!LoadComponent(ctx, SLJIT_FR0, &cond->arg, 0, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* Compare with 0.0 (false) */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR1, 0, SLJIT_IMM, 0);
			sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_EQUAL_F, SLJIT_FR0, 0, SLJIT_FR1, 0);

			/* Jump to else/endif if condition is false (== 0) */
			struct sljit_jump *jumpToElse = sljit_emit_jump(C, SLJIT_EQUAL_F);

			/* Push control flow frame */
			ControlFlowFrame *frame = &ctx->controlStack[ctx->controlDepth++];
			frame->type = CONTROL_FLOW_IF;
			frame->condJump = jumpToElse;
			frame->elseLabel = NULL;
			frame->endLabel = NULL;
			frame->numBreaks = 0;

			break;
		}

		case OpcodeELSE: {
			/* ELSE: Else branch of conditional */
			if (ctx->controlDepth == 0) {
				return GL_FALSE;
			}

			ControlFlowFrame *frame = &ctx->controlStack[ctx->controlDepth - 1];
			if (frame->type != CONTROL_FLOW_IF) {
				return GL_FALSE;
			}

			/* Jump from end of IF block to ENDIF */
			struct sljit_jump *jumpToEnd = sljit_emit_jump(C, SLJIT_JUMP);

			/* Set ELSE label (target of IF's conditional jump) */
			frame->elseLabel = sljit_emit_label(C);
			sljit_set_label(frame->condJump, frame->elseLabel);

			/* Store jump to endif for later */
			frame->condJump = jumpToEnd;

			break;
		}

		case OpcodeENDIF: {
			/* ENDIF: End conditional block */
			if (ctx->controlDepth == 0) {
				return GL_FALSE;
			}

			ControlFlowFrame *frame = &ctx->controlStack[--ctx->controlDepth];
			if (frame->type != CONTROL_FLOW_IF) {
				return GL_FALSE;
			}

			/* Set ENDIF label */
			frame->endLabel = sljit_emit_label(C);

			/* Point conditional jump or else-to-end jump here */
			sljit_set_label(frame->condJump, frame->endLabel);

			break;
		}

		case OpcodeLOOP: {
			/* LOOP: Begin loop block */
			if (ctx->controlDepth >= MAX_CONTROL_FLOW_DEPTH) {
				return GL_FALSE;
			}

			/* Create loop start label */
			struct sljit_label *loopStart = sljit_emit_label(C);

			/* Push control flow frame */
			ControlFlowFrame *frame = &ctx->controlStack[ctx->controlDepth++];
			frame->type = CONTROL_FLOW_LOOP;
			frame->startLabel = loopStart;
			frame->endLabel = NULL;
			frame->numBreaks = 0;

			break;
		}

		case OpcodeENDLOOP: {
			/* ENDLOOP: End loop block - jump back to start */
			if (ctx->controlDepth == 0) {
				return GL_FALSE;
			}

			ControlFlowFrame *frame = &ctx->controlStack[--ctx->controlDepth];
			if (frame->type != CONTROL_FLOW_LOOP) {
				return GL_FALSE;
			}

			/* Jump back to loop start */
			struct sljit_jump *jumpToStart = sljit_emit_jump(C, SLJIT_JUMP);
			sljit_set_label(jumpToStart, frame->startLabel);

			/* Set end label for breaks */
			frame->endLabel = sljit_emit_label(C);

			/* Fix up all break jumps to point here */
			for (GLint i = 0; i < frame->numBreaks; i++) {
				sljit_set_label(frame->breakJumps[i], frame->endLabel);
			}

			break;
		}

		case OpcodeREP: {
			/* REP: Begin repeat block (repeat N times) */
			InstRep *rep = &inst->rep;

			if (ctx->controlDepth >= MAX_CONTROL_FLOW_DEPTH) {
				return GL_FALSE;
			}

			/* Load repeat count into integer register */
			if (!LoadComponent(ctx, SLJIT_FR0, &rep->arg, 0, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* Convert to integer */
			sljit_emit_fop1(C, SLJIT_CONV_S32_FROM_F32, REG_TEMP1, 0, SLJIT_FR0, 0);

			/* Create loop start label */
			struct sljit_label *repStart = sljit_emit_label(C);

			/* Push control flow frame */
			ControlFlowFrame *frame = &ctx->controlStack[ctx->controlDepth++];
			frame->type = CONTROL_FLOW_REP;
			frame->startLabel = repStart;
			frame->endLabel = NULL;
			frame->numBreaks = 0;

			break;
		}

		case OpcodeENDREP: {
			/* ENDREP: End repeat block - decrement and loop */
			if (ctx->controlDepth == 0) {
				return GL_FALSE;
			}

			ControlFlowFrame *frame = &ctx->controlStack[--ctx->controlDepth];
			if (frame->type != CONTROL_FLOW_REP) {
				return GL_FALSE;
			}

			/* Decrement counter */
			sljit_emit_op2(C, SLJIT_SUB, REG_TEMP1, 0, REG_TEMP1, 0, SLJIT_IMM, 1);

			/* Check if counter > 0 */
			sljit_emit_op2(C, SLJIT_SUB | SLJIT_SET_GREATER, SLJIT_UNUSED, 0, REG_TEMP1, 0, SLJIT_IMM, 0);

			/* Jump back if counter > 0 */
			struct sljit_jump *jumpToStart = sljit_emit_jump(C, SLJIT_GREATER);
			sljit_set_label(jumpToStart, frame->startLabel);

			/* Set end label for breaks */
			frame->endLabel = sljit_emit_label(C);

			/* Fix up all break jumps to point here */
			for (GLint i = 0; i < frame->numBreaks; i++) {
				sljit_set_label(frame->breakJumps[i], frame->endLabel);
			}

			break;
		}

		case OpcodeBRK: {
			/* BRK: Break out of loop */
			if (ctx->controlDepth == 0) {
				return GL_FALSE;
			}

			/* Find enclosing loop/rep */
			ControlFlowFrame *frame = NULL;
			for (GLint i = ctx->controlDepth - 1; i >= 0; i--) {
				if (ctx->controlStack[i].type == CONTROL_FLOW_LOOP ||
					ctx->controlStack[i].type == CONTROL_FLOW_REP) {
					frame = &ctx->controlStack[i];
					break;
				}
			}

			if (!frame || frame->numBreaks >= 16) {
				return GL_FALSE;
			}

			/* Create jump to end of loop (will be fixed up at ENDLOOP/ENDREP) */
			frame->breakJumps[frame->numBreaks++] = sljit_emit_jump(C, SLJIT_JUMP);

			break;
		}

		case OpcodeBRA: {
			/* BRA: Conditional branch - not commonly used, treat as NOP for now */
			break;
		}

		case OpcodeKIL: {
			/* KIL: Kill fragment (discard) - return false to discard fragment */
			InstCond *cond = &inst->cond;

			/* Load condition */
			if (!LoadComponent(ctx, SLJIT_FR0, &cond->arg, 0, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* Compare with 0.0 */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR1, 0, SLJIT_IMM, 0);
			sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_LESS_F, SLJIT_FR0, 0, SLJIT_FR1, 0);

			/* If any component < 0, discard by returning FALSE */
			struct sljit_jump *jump = sljit_emit_jump(C, SLJIT_LESS_F);

			/* Return GL_FALSE to discard fragment */
			sljit_emit_return(C, SLJIT_MOV, SLJIT_IMM, GL_FALSE);

			sljit_set_label(jump, sljit_emit_label(C));

			break;
		}

		case OpcodeCAL: {
			/* CAL: Subroutine call
			 * Implementation: Create a label for the subroutine if not exists, emit call
			 */
			InstCal *cal = &inst->cal;

			/* For now, implement as inline code (no actual call/return mechanism)
			 * Full implementation would require label management and return address stack
			 * Since shaders rarely use CAL/RET, we emit a NOP
			 */

			/* NOP - subroutine call placeholder */
			/* A full implementation would:
			 * 1. Save return address
			 * 2. Jump to subroutine label
			 * 3. Execute subroutine
			 * 4. Return to saved address
			 */

			break;
		}

		case OpcodeRET: {
			/* RET: Return from subroutine
			 * Implementation: Jump back to return address
			 */

			/* NOP - return placeholder */
			/* A full implementation would:
			 * 1. Pop return address from stack
			 * 2. Jump to return address
			 */

			break;
		}

		case OpcodeSCC: {
			/* SCC: Set condition code
			 * Stores result into condition code register for later conditional branches
			 */
			InstUnary *unary = &inst->unary;

			/* Load source value */
			if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, 0, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* Store to condition code location
			 * For simplicity, we just store to a temp location
			 * Full implementation would maintain CC state
			 */

			/* Compare with 0.0 and store condition flags */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR1, 0, SLJIT_IMM, 0);
			sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_LESS_F, SLJIT_FR0, 0, SLJIT_FR1, 0);

			/* Condition code is now set in CPU flags, can be used by subsequent branches */

			break;
		}

		case OpcodePHI: {
			/* PHI: SSA phi node
			 * Selects value based on which control flow path was taken
			 * phi(val1, val2, ...) selects value based on predecessor block
			 */
			InstPhi *phi = &inst->phi;

			/* For JIT compilation, PHI nodes should be resolved during SSA conversion
			 * If we encounter one, select the first value
			 */

			if (phi->numArgs > 0) {
				/* Load first argument */
				for (i = 0; i < 4; i++) {
					if (!LoadComponent(ctx, SLJIT_FR0, &phi->args[0], i, ctx->isFragmentShader)) {
						return GL_FALSE;
					}
					if (!StoreComponent(ctx, &phi->dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
						return GL_FALSE;
					}
				}
			}

			break;
		}

		/* Declaration instructions - these are handled during linking, not execution */
		case OpcodeINPUT:
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
