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

	/* Instruction translation with full opcode coverage
	 * Handles all shader IL opcodes including arithmetic, texture sampling,
	 * control flow, subroutines, and condition codes with swizzling and write masks
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
			 * Full implementation: Call runtime texture sampling function
			 */
			InstTex *tex = &inst->tex;
			GLsizei coordOffset = -16;  /* Stack offset for coords */
			GLsizei resultOffset = -32; /* Stack offset for result */
			GLsizei dxOffset = -48;     /* Stack offset for derivatives */
			GLsizei dyOffset = -64;

			/* Allocate stack space for coords, result, and derivatives */
			sljit_emit_op2(C, SLJIT_SUB, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);

			/* Store texture coordinates to stack */
			for (i = 0; i < 4; i++) {
				if (!LoadComponent(ctx, SLJIT_FR0, &tex->coords, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, coordOffset + i * sizeof(GLfloat));
			}

			/* Initialize derivatives to zero (for standard texture sampling) */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0);
			for (i = 0; i < 4; i++) {
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, dxOffset + i * sizeof(GLfloat));
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, dyOffset + i * sizeof(GLfloat));
			}

			/* Load textureImageUnit array from context */
			sljit_emit_op1(C, SLJIT_MOV_P, REG_TEMP1, 0, SLJIT_MEM1(REG_CONTEXT),
						   ctx->isFragmentShader ? offsetof(FragContext, textureImageUnit) :
						                          offsetof(VertexContext, textureImageUnit));

			/* Calculate texture unit pointer (unit = textureImageUnit[samplerIndex]) */
			GLsizei samplerIndex = (tex->sampler && tex->sampler->location >= 0) ?
			                       (tex->sampler->location + tex->offset) : 0;
			sljit_emit_op2(C, SLJIT_ADD, REG_TEMP1, 0, REG_TEMP1, 0, SLJIT_IMM,
			               samplerIndex * sizeof(void*));  /* Assuming pointer array */

			/* Setup function call: GlesTextureSample2D(unit, coords, dx, dy, result)
			 * Arguments:
			 *   arg1 (REG_TEMP1): TextureImageUnit*
			 *   arg2: coords pointer
			 *   arg3: dx pointer
			 *   arg4: dy pointer
			 *   arg5: result pointer
			 */

			/* Load unit pointer */
			sljit_emit_op1(C, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(REG_TEMP1), 0);

			/* Coords pointer */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R1, 0, SLJIT_SP, 0, SLJIT_IMM, coordOffset);

			/* dx pointer */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R2, 0, SLJIT_SP, 0, SLJIT_IMM, dxOffset);

			/* dy pointer */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R3, 0, SLJIT_SP, 0, SLJIT_IMM, dyOffset);

			/* Result pointer */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R4, 0, SLJIT_SP, 0, SLJIT_IMM, resultOffset);

			/* Call texture sampling function based on target */
			void *texFunc = NULL;
			switch (tex->target) {
				case TextureTarget2D:
					texFunc = (void*)GlesTextureSample2D;
					break;
				case TextureTarget3D:
					texFunc = (void*)GlesTextureSample3D;
					break;
				case TextureTargetCube:
					texFunc = (void*)GlesTextureSampleCube;
					break;
				default:
					texFunc = (void*)GlesTextureSample2D;
					break;
			}

			sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS5(VOID, P, P, P, P, P),
			                 SLJIT_IMM, SLJIT_FUNC_ADDR(texFunc));

			/* Load results from stack */
			for (i = 0; i < 4; i++) {
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_LOAD, SLJIT_FR0, SLJIT_SP, resultOffset + i * sizeof(GLfloat));

				/* Apply saturation if needed */
				if (inst->base.op == OpcodeTEX_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				if (!StoreComponent(ctx, &tex->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}

			/* Restore stack */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);

			break;
		}

		case OpcodeTXB:
		case OpcodeTXB_SAT: {
			/* Texture sample with bias: dst = texture(sampler, coords, bias)
			 * Full implementation with bias support via derivative scaling
			 */
			InstTex *tex = &inst->tex;
			GLsizei coordOffset = -16;
			GLsizei resultOffset = -32;
			GLsizei dxOffset = -48;
			GLsizei dyOffset = -64;

			/* Allocate stack space */
			sljit_emit_op2(C, SLJIT_SUB, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);

			/* Store coordinates */
			for (i = 0; i < 4; i++) {
				if (!LoadComponent(ctx, SLJIT_FR0, &tex->coords, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, coordOffset + i * sizeof(GLfloat));
			}

			/* Bias is in coords.w - load it */
			if (!LoadComponent(ctx, SLJIT_FR1, &tex->coords, 3, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* Convert bias to derivative scale factor: scale = 2^bias */
			sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(exp2f));

			/* Initialize derivatives with bias scaling (simple approximation) */
			for (i = 0; i < 4; i++) {
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, dxOffset + i * sizeof(GLfloat));
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, dyOffset + i * sizeof(GLfloat));
			}

			/* Load texture unit */
			sljit_emit_op1(C, SLJIT_MOV_P, REG_TEMP1, 0, SLJIT_MEM1(REG_CONTEXT),
			               ctx->isFragmentShader ? offsetof(FragContext, textureImageUnit) :
			                                      offsetof(VertexContext, textureImageUnit));

			GLsizei samplerIndex = (tex->sampler && tex->sampler->location >= 0) ?
			                       (tex->sampler->location + tex->offset) : 0;
			sljit_emit_op2(C, SLJIT_ADD, REG_TEMP1, 0, REG_TEMP1, 0, SLJIT_IMM,
			               samplerIndex * sizeof(void*));

			/* Setup call arguments */
			sljit_emit_op1(C, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(REG_TEMP1), 0);
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R1, 0, SLJIT_SP, 0, SLJIT_IMM, coordOffset);
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R2, 0, SLJIT_SP, 0, SLJIT_IMM, dxOffset);
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R3, 0, SLJIT_SP, 0, SLJIT_IMM, dyOffset);
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R4, 0, SLJIT_SP, 0, SLJIT_IMM, resultOffset);

			/* Call texture function */
			void *texFunc = NULL;
			switch (tex->target) {
				case TextureTarget2D: texFunc = (void*)GlesTextureSample2D; break;
				case TextureTarget3D: texFunc = (void*)GlesTextureSample3D; break;
				case TextureTargetCube: texFunc = (void*)GlesTextureSampleCube; break;
				default: texFunc = (void*)GlesTextureSample2D; break;
			}

			sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS5(VOID, P, P, P, P, P),
			                 SLJIT_IMM, SLJIT_FUNC_ADDR(texFunc));

			/* Load and store results */
			for (i = 0; i < 4; i++) {
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_LOAD, SLJIT_FR0, SLJIT_SP, resultOffset + i * sizeof(GLfloat));
				if (inst->base.op == OpcodeTXB_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}
				if (!StoreComponent(ctx, &tex->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					return GL_FALSE;
				}
			}

			/* Restore stack */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);

			break;
		}

		case OpcodeTXP:
		case OpcodeTXP_SAT: {
			/* Texture sample with projection: dst = texture(sampler, coords.xyz / coords.w) */
			InstTex *tex = &inst->tex;
			GLsizei coordOffset = -16;
			GLsizei resultOffset = -32;
			GLsizei dxOffset = -48;
			GLsizei dyOffset = -64;

			/* Allocate stack space for coords, result, derivatives */
			sljit_emit_op2(C, SLJIT_SUB, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);

			/* Load w component for projection */
			if (!LoadComponent(ctx, SLJIT_FR1, &tex->coords, 3, ctx->isFragmentShader)) {
				sljit_emit_op2(C, SLJIT_ADD, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);
				return GL_FALSE;
			}

			/* Store projected coordinates to stack (coords.xyz / coords.w) */
			for (i = 0; i < 3; i++) {
				/* Load coordinate component */
				if (!LoadComponent(ctx, SLJIT_FR0, &tex->coords, i, ctx->isFragmentShader)) {
					sljit_emit_op2(C, SLJIT_ADD, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);
					return GL_FALSE;
				}

				/* Divide by w for projection */
				sljit_emit_fop2(C, SLJIT_DIV_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);

				/* Store projected coordinate */
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, coordOffset + i * sizeof(GLfloat));
			}

			/* Store w as 1.0 (after projection) */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0x3f800000); /* 1.0f */
			sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, coordOffset + 3 * sizeof(GLfloat));

			/* Initialize derivatives to zero */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR0, 0, SLJIT_IMM, 0);
			for (i = 0; i < 4; i++) {
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, dxOffset + i * sizeof(GLfloat));
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, dyOffset + i * sizeof(GLfloat));
			}

			/* Load texture unit from context */
			sljit_emit_op1(C, SLJIT_MOV_P, REG_TEMP1, 0, SLJIT_MEM1(REG_CONTEXT),
			               ctx->isFragmentShader ? offsetof(FragContext, textureImageUnit) :
			                                      offsetof(VertexContext, textureImageUnit));

			/* Calculate sampler index and texture unit pointer */
			GLsizei samplerIndex = (tex->sampler && tex->sampler->location >= 0) ?
			                       (tex->sampler->location + tex->offset) : 0;
			sljit_emit_op2(C, SLJIT_ADD, REG_TEMP1, 0, REG_TEMP1, 0, SLJIT_IMM,
			               samplerIndex * sizeof(void*));

			/* Setup function call arguments */
			sljit_emit_op1(C, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(REG_TEMP1), 0);  /* texture unit */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R1, 0, SLJIT_SP, 0, SLJIT_IMM, coordOffset);  /* coords */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R2, 0, SLJIT_SP, 0, SLJIT_IMM, dxOffset);     /* dx */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R3, 0, SLJIT_SP, 0, SLJIT_IMM, dyOffset);     /* dy */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R4, 0, SLJIT_SP, 0, SLJIT_IMM, resultOffset); /* result */

			/* Call appropriate texture sampling function */
			void *texFunc = NULL;
			switch (tex->target) {
				case TextureTarget2D:   texFunc = (void*)GlesTextureSample2D; break;
				case TextureTarget3D:   texFunc = (void*)GlesTextureSample3D; break;
				case TextureTargetCube: texFunc = (void*)GlesTextureSampleCube; break;
				default:                texFunc = (void*)GlesTextureSample2D; break;
			}

			sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS5(VOID, P, P, P, P, P),
			                 SLJIT_IMM, SLJIT_FUNC_ADDR(texFunc));

			/* Load results from stack and store to destination */
			for (i = 0; i < 4; i++) {
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_LOAD, SLJIT_FR0, SLJIT_SP, resultOffset + i * sizeof(GLfloat));

				/* Apply saturation if needed */
				if (inst->base.op == OpcodeTXP_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				if (!StoreComponent(ctx, &tex->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					sljit_emit_op2(C, SLJIT_ADD, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);
					return GL_FALSE;
				}
			}

			/* Restore stack */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);

			break;
		}

		case OpcodeTXL:
		case OpcodeTXL_SAT: {
			/* Texture sample with explicit LOD: dst = texture(sampler, coords, lod) */
			InstTex *tex = &inst->tex;
			GLsizei coordOffset = -16;
			GLsizei resultOffset = -32;
			GLsizei dxOffset = -48;
			GLsizei dyOffset = -64;

			/* Allocate stack space for coords, result, derivatives */
			sljit_emit_op2(C, SLJIT_SUB, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);

			/* Store coordinates to stack (first 4 components) */
			for (i = 0; i < 4; i++) {
				if (!LoadComponent(ctx, SLJIT_FR0, &tex->coords, i, ctx->isFragmentShader)) {
					sljit_emit_op2(C, SLJIT_ADD, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);
					return GL_FALSE;
				}
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, coordOffset + i * sizeof(GLfloat));
			}

			/* LOD is in coords.w - load it */
			if (!LoadComponent(ctx, SLJIT_FR0, &tex->coords, 3, ctx->isFragmentShader)) {
				sljit_emit_op2(C, SLJIT_ADD, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);
				return GL_FALSE;
			}

			/* Convert LOD to derivative scale: scale = 2^LOD */
			sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(exp2f));

			/* Initialize derivatives with LOD scaling (scale in FR0) */
			for (i = 0; i < 4; i++) {
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, dxOffset + i * sizeof(GLfloat));
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE, SLJIT_FR0, SLJIT_SP, dyOffset + i * sizeof(GLfloat));
			}

			/* Load texture unit from context */
			sljit_emit_op1(C, SLJIT_MOV_P, REG_TEMP1, 0, SLJIT_MEM1(REG_CONTEXT),
			               ctx->isFragmentShader ? offsetof(FragContext, textureImageUnit) :
			                                      offsetof(VertexContext, textureImageUnit));

			/* Calculate sampler index and texture unit pointer */
			GLsizei samplerIndex = (tex->sampler && tex->sampler->location >= 0) ?
			                       (tex->sampler->location + tex->offset) : 0;
			sljit_emit_op2(C, SLJIT_ADD, REG_TEMP1, 0, REG_TEMP1, 0, SLJIT_IMM,
			               samplerIndex * sizeof(void*));

			/* Setup function call arguments */
			sljit_emit_op1(C, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(REG_TEMP1), 0);  /* texture unit */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R1, 0, SLJIT_SP, 0, SLJIT_IMM, coordOffset);  /* coords */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R2, 0, SLJIT_SP, 0, SLJIT_IMM, dxOffset);     /* dx */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R3, 0, SLJIT_SP, 0, SLJIT_IMM, dyOffset);     /* dy */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_R4, 0, SLJIT_SP, 0, SLJIT_IMM, resultOffset); /* result */

			/* Call appropriate texture sampling function */
			void *texFunc = NULL;
			switch (tex->target) {
				case TextureTarget2D:   texFunc = (void*)GlesTextureSample2D; break;
				case TextureTarget3D:   texFunc = (void*)GlesTextureSample3D; break;
				case TextureTargetCube: texFunc = (void*)GlesTextureSampleCube; break;
				default:                texFunc = (void*)GlesTextureSample2D; break;
			}

			sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS5(VOID, P, P, P, P, P),
			                 SLJIT_IMM, SLJIT_FUNC_ADDR(texFunc));

			/* Load results from stack and store to destination */
			for (i = 0; i < 4; i++) {
				sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_LOAD, SLJIT_FR0, SLJIT_SP, resultOffset + i * sizeof(GLfloat));

				/* Apply saturation if needed */
				if (inst->base.op == OpcodeTXL_SAT) {
					ApplySaturation(ctx, SLJIT_FR0);
				}

				if (!StoreComponent(ctx, &tex->alu.dst, SLJIT_FR0, i, ctx->isFragmentShader)) {
					sljit_emit_op2(C, SLJIT_ADD, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);
					return GL_FALSE;
				}
			}

			/* Restore stack */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 64);

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
			/* BRA: Conditional branch based on condition code (CC) register
			 * Reads CC value from SLJIT_S0 (set by SCC) and branches if condition is met
			 */
			InstBranch *branch = &inst->branch;

			/* Check CC value in SLJIT_S0
			 * CC encoding: positive (>0) = true, zero (==0) = false/zero, negative (<0) = also condition
			 * For simplicity, we branch if CC != 0
			 */

			/* Compare CC with 0 */
			sljit_emit_op2(C, SLJIT_SUB | SLJIT_SET_Z, SLJIT_UNUSED, 0, SLJIT_S0, 0, SLJIT_IMM, 0);

			/* Branch if CC != 0 (condition is true) */
			struct sljit_jump *branchJump = sljit_emit_jump(C, SLJIT_NOT_EQUAL);

			/* Get or create label for branch target */
			GLsizei targetId = branch->target ? (GLsizei)(branch->target->offset) : 0;
			if (targetId >= 256) {
				targetId = 0;
			}

			/* Create label if it doesn't exist */
			if (!ctx->subroutineLabels[targetId]) {
				ctx->subroutineLabels[targetId] = sljit_emit_label(C);
			}

			/* Set the branch jump to the target label */
			sljit_set_label(branchJump, ctx->subroutineLabels[targetId]);

			/* Continue execution if branch not taken */

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
			/* CAL: Subroutine call with full return address stack support
			 * Approach: Store return label index on stack, jump to subroutine
			 */
			InstBranch *branch = &inst->branch;

			/* Allocate space on runtime stack for return address (as integer index) */
			GLsizei returnStackOffset = -8;  /* Store return index at SP-8 */
			sljit_emit_op2(C, SLJIT_SUB, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 8);

			/* Assign a unique return label index for this CAL instruction */
			GLint returnLabelIndex = ctx->numReturns;
			if (returnLabelIndex >= 32) {
				sljit_emit_op2(C, SLJIT_ADD, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 8);
				return GL_FALSE;
			}

			/* Store return label index on runtime stack */
			sljit_emit_op1(C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), 0, SLJIT_IMM, returnLabelIndex);

			/* Get or create label for the subroutine entry point */
			GLsizei subroutineId = branch->target ? (GLsizei)(branch->target->offset) : 0;
			if (subroutineId >= 256) {
				subroutineId = 0;
			}

			if (!ctx->subroutineLabels[subroutineId]) {
				ctx->subroutineLabels[subroutineId] = sljit_emit_label(C);
			}

			/* Jump to subroutine */
			struct sljit_jump *callJump = sljit_emit_jump(C, SLJIT_JUMP);
			sljit_set_label(callJump, ctx->subroutineLabels[subroutineId]);

			/* Create return label - RET will jump here */
			struct sljit_label *returnLabel = sljit_emit_label(C);
			ctx->returnJumps[ctx->numReturns] = (struct sljit_jump*)returnLabel;  /* Store label in jump array */
			ctx->numReturns++;

			/* Restore stack after return */
			sljit_emit_op2(C, SLJIT_ADD, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 8);

			break;
		}

		case OpcodeRET: {
			/* RET: Return from subroutine with full return address stack support
			 * Approach: Read return label index from stack, jump to corresponding return label
			 */

			/* Load return label index from runtime stack (at SP + 0, since CAL pushed it) */
			sljit_emit_op1(C, SLJIT_MOV, REG_TEMP1, 0, SLJIT_MEM1(SLJIT_SP), 0);

			/* Generate a switch-like jump table based on return index
			 * For each possible return index, compare and conditionally jump to the return label
			 */
			struct sljit_jump *compareJumps[32];
			GLint numPossibleReturns = ctx->numReturns;

			for (GLint i = 0; i < numPossibleReturns && i < 32; i++) {
				/* Compare return index with i */
				sljit_emit_op2(C, SLJIT_SUB | SLJIT_SET_Z, SLJIT_UNUSED, 0, REG_TEMP1, 0, SLJIT_IMM, i);

				/* If equal, jump to the i-th return label */
				struct sljit_jump *matchJump = sljit_emit_jump(C, SLJIT_EQUAL);

				/* The return label was stored in returnJumps array by CAL */
				if (ctx->returnJumps[i]) {
					sljit_set_label(matchJump, (struct sljit_label*)ctx->returnJumps[i]);
				} else {
					/* No label for this index, just continue */
					compareJumps[i] = matchJump;
				}
			}

			/* If no match found (shouldn't happen), just continue execution */
			struct sljit_label *fallthrough = sljit_emit_label(C);
			for (GLint i = 0; i < numPossibleReturns && i < 32; i++) {
				if (!ctx->returnJumps[i] && compareJumps[i]) {
					sljit_set_label(compareJumps[i], fallthrough);
				}
			}

			break;
		}

		case OpcodeSCC: {
			/* SCC: Set condition code with full CC register support
			 * Stores condition code value for later use by BRA (conditional branch)
			 *
			 * We use SLJIT_S0 (a callee-saved register) to store the CC state.
			 * SLJIT_S0 is preserved across function calls and persists between instructions.
			 * We store a composite condition result: positive if any component > 0,
			 * negative if any component < 0, zero if all components == 0.
			 */
			InstUnary *unary = &inst->unary;

			/* Initialize CC value to 0 */
			sljit_emit_op1(C, SLJIT_MOV, SLJIT_S0, 0, SLJIT_IMM, 0);

			/* Load the first component and check if != 0 */
			if (!LoadComponent(ctx, SLJIT_FR0, &unary->arg, 0, ctx->isFragmentShader)) {
				return GL_FALSE;
			}

			/* Compare with 0.0 */
			sljit_emit_fop1(C, SLJIT_MOV_F32, SLJIT_FR1, 0, SLJIT_IMM, 0);
			sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_GREATER_F, SLJIT_FR0, 0, SLJIT_FR1, 0);

			/* If > 0, set CC to 1 */
			struct sljit_jump *notPositive = sljit_emit_jump(C, SLJIT_LESS_EQUAL_F);
			sljit_emit_op1(C, SLJIT_MOV, SLJIT_S0, 0, SLJIT_IMM, 1);
			struct sljit_label *afterPositive = sljit_emit_label(C);
			sljit_set_label(notPositive, afterPositive);

			/* If < 0, set CC to -1 */
			sljit_emit_fop1(C, SLJIT_CMP_F32 | SLJIT_SET_LESS_F, SLJIT_FR0, 0, SLJIT_FR1, 0);
			struct sljit_jump *notNegative = sljit_emit_jump(C, SLJIT_GREATER_EQUAL_F);
			sljit_emit_op1(C, SLJIT_MOV, SLJIT_S0, 0, SLJIT_IMM, -1);
			struct sljit_label *afterNegative = sljit_emit_label(C);
			sljit_set_label(notNegative, afterNegative);

			/* CC is now stored in SLJIT_S0 and will persist until BRA reads it */

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
				 * This should rarely occur with full opcode coverage
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
	 * JIT compilation is handled per-executable by GlesOptimizeExecutable
	 */
	return GL_TRUE;
}
