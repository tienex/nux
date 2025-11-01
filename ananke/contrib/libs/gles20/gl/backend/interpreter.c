/*
** ==========================================================================
**
** Shader IL Interpreter
**
** Fallback interpreter for when JIT compilation is not available
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
#include "frontend/il.h"
#include "frontend/linker.h"
#include "backend/interpreter.h"

#include <string.h>
#include <math.h>

#define MAX_CONTROL_FLOW_DEPTH 32
#define MAX_CALL_STACK_DEPTH 32
#define MAX_INSTRUCTIONS 4096

/**
 * Control flow frame types
 */
typedef enum {
	CONTROL_FLOW_IF,
	CONTROL_FLOW_LOOP,
	CONTROL_FLOW_REP
} ControlFlowType;

/**
 * Control flow stack frame
 */
typedef struct {
	ControlFlowType type;
	GLsizei startIP;      /* For LOOP/REP: instruction pointer to loop start */
	GLsizei endIP;        /* For IF/LOOP/REP: instruction pointer to end (ENDIF/ENDLOOP/ENDREP) */
	GLsizei elseIP;       /* For IF: instruction pointer to ELSE */
	GLint repCounter;     /* For REP: remaining iterations */
} ControlFlowFrame;

/**
 * Call stack frame for CAL/RET
 */
typedef struct {
	GLsizei returnIP;     /* Return address after CAL */
} CallStackFrame;

/**
 * Execution state for control flow
 */
typedef struct {
	/* Instruction pointer */
	GLsizei IP;

	/* Control flow stack */
	ControlFlowFrame controlStack[MAX_CONTROL_FLOW_DEPTH];
	GLint controlDepth;

	/* Call stack */
	CallStackFrame callStack[MAX_CALL_STACK_DEPTH];
	GLint callDepth;

	/* Condition code register (for SCC/BRA) */
	GLint conditionCode;

	/* Label map: maps Label* to instruction index */
	Inst *instructions[MAX_INSTRUCTIONS];
	GLsizei numInstructions;

	/* Flag to break out of loop */
	GLboolean breakRequested;
} ExecState;

/**
 * Helper: Get register base pointer from source reference
 */
static const Vec4f *GetSourceRegisterBase(const SrcReg *src, const VertexContext *vctx, const FragContext *fctx) {
	if (vctx) {
		/* Vertex shader context */
		switch (src->reference.base.kind) {
			case VarKindAttrib:   return vctx->attrib;
			case VarKindUniform:  return vctx->uniform;
			case VarKindConstant: return vctx->constant;
			case VarKindTemp:     return vctx->temp;
			default:              return NULL;
		}
	} else if (fctx) {
		/* Fragment shader context */
		switch (src->reference.base.kind) {
			case VarKindUniform:  return fctx->uniform;
			case VarKindConstant: return fctx->constant;
			case VarKindTemp:     return fctx->temp;
			default:              return NULL;
		}
	}
	return NULL;
}

/**
 * Helper: Get register base pointer from destination reference
 */
static Vec4f *GetDestRegisterBase(const DstReg *dst, VertexContext *vctx, FragContext *fctx) {
	if (vctx) {
		/* Vertex shader context */
		switch (dst->reference.base.kind) {
			case VarKindTemp:   return vctx->temp;
			case VarKindResult: return (Vec4f *)vctx->varying;  /* Varying output */
			default:            return NULL;
		}
	} else if (fctx) {
		/* Fragment shader context */
		switch (dst->reference.base.kind) {
			case VarKindTemp:   return fctx->temp;
			case VarKindResult: return fctx->result;
			default:            return NULL;
		}
	}
	return NULL;
}

/**
 * Helper: Load a vec4 from source operand with swizzling
 */
static void LoadVec4(Vec4f *dest, const SrcReg *src, const VertexContext *vctx, const FragContext *fctx) {
	const Vec4f *base = GetSourceRegisterBase(src, vctx, fctx);
	GLuint swizzle[4] = { src->selectX, src->selectY, src->selectZ, src->selectW };
	GLuint i;

	if (!base) {
		GlesMemset(dest, 0, sizeof(Vec4f));
		return;
	}

	/* Apply index if present */
	GLsizei index = src->reference.base.index;
	if (src->index) {
		/* Dynamic indexing - for simplicity, use index 0 */
		index = 0;
	}

	/* Read with swizzling */
	for (i = 0; i < 4; i++) {
		dest->base[i] = base[index].base[swizzle[i]];
		if (src->negate) {
			dest->base[i] = -dest->base[i];
		}
	}
}

/**
 * Helper: Store a vec4 to destination operand with write mask
 */
static void StoreVec4(const DstReg *dst, const Vec4f *src, VertexContext *vctx, FragContext *fctx) {
	Vec4f *base = GetDestRegisterBase(dst, vctx, fctx);
	GLboolean mask[4] = { dst->maskX, dst->maskY, dst->maskZ, dst->maskW };
	GLuint i;

	if (!base) {
		return;
	}

	/* Apply index if present */
	GLsizei index = dst->reference.base.index;

	/* Write with masking */
	for (i = 0; i < 4; i++) {
		if (mask[i]) {
			base[index].base[i] = src->base[i];
		}
	}
}

/**
 * Helper: Find instruction index for a label target
 */
static GLsizei FindLabelTarget(const ExecState *exec, const Label *label) {
	Block *targetBlock;
	Inst *inst;
	GLsizei ip;

	if (!label || !label->target) {
		return -1;
	}

	targetBlock = label->target;

	/* Search for the first instruction of the target block */
	for (ip = 0; ip < exec->numInstructions; ip++) {
		inst = exec->instructions[ip];

		/* Check if this instruction belongs to the target block by traversing from block->first */
		Inst *blockInst = targetBlock->first;
		while (blockInst) {
			if (blockInst == inst) {
				return ip;
			}
			blockInst = blockInst->base.next;
		}
	}

	return -1;
}

/**
 * Helper: Find matching control flow instruction
 * For IF, find matching ENDIF or ELSE
 * For LOOP, find matching ENDLOOP
 * For REP, find matching ENDREP
 */
static GLsizei FindMatchingControlFlow(const ExecState *exec, GLsizei startIP, InstKind startKind) {
	GLint depth = 1;
	GLsizei ip;

	for (ip = startIP + 1; ip < exec->numInstructions; ip++) {
		Inst *inst = exec->instructions[ip];
		InstKind op = inst->base.op;

		if (startKind == OpcodeIF) {
			if (op == OpcodeIF) {
				depth++;
			} else if (op == OpcodeENDIF) {
				depth--;
				if (depth == 0) {
					return ip;
				}
			} else if (op == OpcodeELSE && depth == 1) {
				return ip;  /* Return ELSE for IF at depth 1 */
			}
		} else if (startKind == OpcodeLOOP) {
			if (op == OpcodeLOOP) {
				depth++;
			} else if (op == OpcodeENDLOOP) {
				depth--;
				if (depth == 0) {
					return ip;
				}
			}
		} else if (startKind == OpcodeREP) {
			if (op == OpcodeREP) {
				depth++;
			} else if (op == OpcodeENDREP) {
				depth--;
				if (depth == 0) {
					return ip;
				}
			}
		}
	}

	return -1;
}

/**
 * Helper: Evaluate condition for BRA/KIL
 */
static GLboolean EvaluateCondition(const InstCond *cond, GLfloat condValue) {
	/* Simple condition evaluation based on condition code
	 * condValue: result from comparison (1.0 = true, 0.0 = false)
	 */
	switch (cond->cond) {
		case CondF:  return GL_FALSE;  /* Always false */
		case CondT:  return GL_TRUE;   /* Always true */
		case CondEQ: return (condValue == 0.0f);
		case CondNE: return (condValue != 0.0f);
		case CondLT: return (condValue < 0.0f);
		case CondLE: return (condValue <= 0.0f);
		case CondGT: return (condValue > 0.0f);
		case CondGE: return (condValue >= 0.0f);
		default:     return GL_FALSE;
	}
}

/**
 * Execute a single instruction
 * Returns: GL_TRUE to continue, GL_FALSE to exit (e.g., KIL)
 */
static GLboolean ExecuteInstruction(Inst *inst, ExecState *exec, VertexContext *vctx, FragContext *fctx) {
	Vec4f src0, src1, src2, result;
	GLuint i;

	switch (inst->base.op) {
		case OpcodeMOV:
		case OpcodeMOV_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			if (inst->base.op == OpcodeMOV_SAT) {
				for (i = 0; i < 4; i++) {
					src0.base[i] = (src0.base[i] < 0.0f) ? 0.0f : ((src0.base[i] > 1.0f) ? 1.0f : src0.base[i]);
				}
			}
			StoreVec4(&inst->unary.alu.dst, &src0, vctx, fctx);
			break;

		case OpcodeADD:
		case OpcodeADD_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = src0.base[i] + src1.base[i];
			if (inst->base.op == OpcodeADD_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeMUL:
		case OpcodeMUL_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = src0.base[i] * src1.base[i];
			if (inst->base.op == OpcodeMUL_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeDP3:
		case OpcodeDP3_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			result.base[0] = result.base[1] = result.base[2] = result.base[3] =
				src0.base[0] * src1.base[0] + src0.base[1] * src1.base[1] + src0.base[2] * src1.base[2];
			if (inst->base.op == OpcodeDP3_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeDP4:
		case OpcodeDP4_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			result.base[0] = result.base[1] = result.base[2] = result.base[3] =
				src0.base[0] * src1.base[0] + src0.base[1] * src1.base[1] +
				src0.base[2] * src1.base[2] + src0.base[3] * src1.base[3];
			if (inst->base.op == OpcodeDP4_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeMAD:
		case OpcodeMAD_SAT:
			LoadVec4(&src0, &inst->ternary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->ternary.arg1, vctx, fctx);
			LoadVec4(&src2, &inst->ternary.arg2, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = src0.base[i] * src1.base[i] + src2.base[i];
			if (inst->base.op == OpcodeMAD_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->ternary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeSUB:
		case OpcodeSUB_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = src0.base[i] - src1.base[i];
			if (inst->base.op == OpcodeSUB_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeMIN:
		case OpcodeMIN_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] < src1.base[i]) ? src0.base[i] : src1.base[i];
			if (inst->base.op == OpcodeMIN_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeMAX:
		case OpcodeMAX_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] > src1.base[i]) ? src0.base[i] : src1.base[i];
			if (inst->base.op == OpcodeMAX_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeRCP:
		case OpcodeRCP_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = 1.0f / src0.base[i];
			if (inst->base.op == OpcodeRCP_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeRSQ:
		case OpcodeRSQ_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = 1.0f / sqrtf(fabsf(src0.base[i]));
			if (inst->base.op == OpcodeRSQ_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeSGE:
		case OpcodeSGE_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] >= src1.base[i]) ? 1.0f : 0.0f;
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeSLT:
		case OpcodeSLT_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] < src1.base[i]) ? 1.0f : 0.0f;
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeFLR:
		case OpcodeFLR_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = floorf(src0.base[i]);
			if (inst->base.op == OpcodeFLR_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeFRC:
		case OpcodeFRC_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = src0.base[i] - floorf(src0.base[i]);
			if (inst->base.op == OpcodeFRC_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeEXP:
		case OpcodeEXP_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = expf(src0.base[i]);
			if (inst->base.op == OpcodeEXP_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeLOG:
		case OpcodeLOG_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = logf(fabsf(src0.base[i]));
			if (inst->base.op == OpcodeLOG_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeEX2:
		case OpcodeEX2_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = exp2f(src0.base[i]);
			if (inst->base.op == OpcodeEX2_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeLG2:
		case OpcodeLG2_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = log2f(fabsf(src0.base[i]));
			if (inst->base.op == OpcodeLG2_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodePOW:
		case OpcodePOW_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = powf(fabsf(src0.base[i]), src1.base[i]);
			if (inst->base.op == OpcodePOW_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeSIN:
		case OpcodeSIN_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = sinf(src0.base[i]);
			if (inst->base.op == OpcodeSIN_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeCOS:
		case OpcodeCOS_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = cosf(src0.base[i]);
			if (inst->base.op == OpcodeCOS_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeABS:
		case OpcodeABS_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = fabsf(src0.base[i]);
			if (inst->base.op == OpcodeABS_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeLRP:
		case OpcodeLRP_SAT:
			LoadVec4(&src0, &inst->ternary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->ternary.arg1, vctx, fctx);
			LoadVec4(&src2, &inst->ternary.arg2, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = src0.base[i] * src1.base[i] + (1.0f - src0.base[i]) * src2.base[i];
			if (inst->base.op == OpcodeLRP_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->ternary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeDP2:
		case OpcodeDP2_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			result.base[0] = result.base[1] = result.base[2] = result.base[3] =
				src0.base[0] * src1.base[0] + src0.base[1] * src1.base[1];
			if (inst->base.op == OpcodeDP2_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeDPH:
		case OpcodeDPH_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			result.base[0] = result.base[1] = result.base[2] = result.base[3] =
				src0.base[0] * src1.base[0] + src0.base[1] * src1.base[1] + src0.base[2] * src1.base[2] + src1.base[3];
			if (inst->base.op == OpcodeDPH_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeXPD:
		case OpcodeXPD_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			result.base[0] = src0.base[1] * src1.base[2] - src0.base[2] * src1.base[1];
			result.base[1] = src0.base[2] * src1.base[0] - src0.base[0] * src1.base[2];
			result.base[2] = src0.base[0] * src1.base[1] - src0.base[1] * src1.base[0];
			result.base[3] = 1.0f;
			if (inst->base.op == OpcodeXPD_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeCMP:
		case OpcodeCMP_SAT:
			LoadVec4(&src0, &inst->ternary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->ternary.arg1, vctx, fctx);
			LoadVec4(&src2, &inst->ternary.arg2, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] < 0.0f) ? src1.base[i] : src2.base[i];
			if (inst->base.op == OpcodeCMP_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->ternary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeSSG:
		case OpcodeSSG_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] > 0.0f) ? 1.0f : ((src0.base[i] < 0.0f) ? -1.0f : 0.0f);
			if (inst->base.op == OpcodeSSG_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeSLE:
		case OpcodeSLE_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] <= src1.base[i]) ? 1.0f : 0.0f;
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeSGT:
		case OpcodeSGT_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] > src1.base[i]) ? 1.0f : 0.0f;
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeSCS:
		case OpcodeSCS_SAT:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			result.base[0] = cosf(src0.base[0]);
			result.base[1] = sinf(src0.base[0]);
			result.base[2] = 0.0f;
			result.base[3] = 1.0f;
			if (inst->base.op == OpcodeSCS_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeDST:
		case OpcodeDST_SAT:
			LoadVec4(&src0, &inst->binary.arg0, vctx, fctx);
			LoadVec4(&src1, &inst->binary.arg1, vctx, fctx);
			result.base[0] = 1.0f;
			result.base[1] = src0.base[1] * src1.base[1];
			result.base[2] = src0.base[2];
			result.base[3] = src1.base[3];
			if (inst->base.op == OpcodeDST_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->binary.alu.dst, &result, vctx, fctx);
			break;

		case OpcodeARL:
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			for (i = 0; i < 4; i++) result.base[i] = floorf(src0.base[i]);
			StoreVec4(&inst->unary.alu.dst, &result, vctx, fctx);
			break;

		/* ===== CONTROL FLOW INSTRUCTIONS ===== */

		case OpcodeIF: {
			/* IF: Begin conditional block - test condition */
			InstCond *cond = &inst->cond;
			LoadVec4(&src0, &cond->arg, vctx, fctx);

			/* Evaluate condition on first component */
			GLboolean condTrue = (src0.base[0] != 0.0f);

			if (!condTrue) {
				/* Condition false - skip to ELSE or ENDIF */
				GLsizei targetIP = FindMatchingControlFlow(exec, exec->IP, OpcodeIF);
				if (targetIP >= 0) {
					Inst *targetInst = exec->instructions[targetIP];
					if (targetInst->base.op == OpcodeELSE) {
						/* Jump to ELSE, push frame to remember we're in the else branch */
						if (exec->controlDepth < MAX_CONTROL_FLOW_DEPTH) {
							ControlFlowFrame *frame = &exec->controlStack[exec->controlDepth++];
							frame->type = CONTROL_FLOW_IF;
							frame->startIP = exec->IP;
							frame->elseIP = targetIP;
							frame->endIP = FindMatchingControlFlow(exec, targetIP, OpcodeIF);  /* Find ENDIF from ELSE */
						}
						exec->IP = targetIP;
					} else if (targetInst->base.op == OpcodeENDIF) {
						/* Jump to ENDIF */
						exec->IP = targetIP;
					}
				}
			} else {
				/* Condition true - push control frame and continue */
				if (exec->controlDepth < MAX_CONTROL_FLOW_DEPTH) {
					ControlFlowFrame *frame = &exec->controlStack[exec->controlDepth++];
					frame->type = CONTROL_FLOW_IF;
					frame->startIP = exec->IP;
					frame->elseIP = -1;
					frame->endIP = FindMatchingControlFlow(exec, exec->IP, OpcodeIF);
				}
			}
			break;
		}

		case OpcodeELSE: {
			/* ELSE: Executed when IF was true - skip to ENDIF */
			if (exec->controlDepth > 0 && exec->controlStack[exec->controlDepth - 1].type == CONTROL_FLOW_IF) {
				ControlFlowFrame *frame = &exec->controlStack[exec->controlDepth - 1];
				if (frame->endIP >= 0) {
					exec->IP = frame->endIP;
				}
			}
			break;
		}

		case OpcodeENDIF: {
			/* ENDIF: End conditional block */
			if (exec->controlDepth > 0 && exec->controlStack[exec->controlDepth - 1].type == CONTROL_FLOW_IF) {
				exec->controlDepth--;
			}
			break;
		}

		case OpcodeLOOP: {
			/* LOOP: Begin infinite loop (terminated by BRK or condition) */
			if (exec->controlDepth < MAX_CONTROL_FLOW_DEPTH) {
				ControlFlowFrame *frame = &exec->controlStack[exec->controlDepth++];
				frame->type = CONTROL_FLOW_LOOP;
				frame->startIP = exec->IP;
				frame->endIP = FindMatchingControlFlow(exec, exec->IP, OpcodeLOOP);
			}
			break;
		}

		case OpcodeENDLOOP: {
			/* ENDLOOP: End loop - jump back to LOOP start */
			if (exec->controlDepth > 0 && exec->controlStack[exec->controlDepth - 1].type == CONTROL_FLOW_LOOP) {
				ControlFlowFrame *frame = &exec->controlStack[exec->controlDepth - 1];

				if (exec->breakRequested) {
					/* BRK was executed - exit loop */
					exec->breakRequested = GL_FALSE;
					exec->controlDepth--;
				} else {
					/* Jump back to loop start */
					exec->IP = frame->startIP;
				}
			}
			break;
		}

		case OpcodeREP: {
			/* REP: Begin repeat block (repeat N times) */
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);
			GLint count = (GLint)src0.base[0];

			if (count <= 0) {
				/* Skip loop entirely */
				GLsizei endIP = FindMatchingControlFlow(exec, exec->IP, OpcodeREP);
				if (endIP >= 0) {
					exec->IP = endIP;
				}
			} else {
				/* Push control frame with counter */
				if (exec->controlDepth < MAX_CONTROL_FLOW_DEPTH) {
					ControlFlowFrame *frame = &exec->controlStack[exec->controlDepth++];
					frame->type = CONTROL_FLOW_REP;
					frame->startIP = exec->IP;
					frame->endIP = FindMatchingControlFlow(exec, exec->IP, OpcodeREP);
					frame->repCounter = count;
				}
			}
			break;
		}

		case OpcodeENDREP: {
			/* ENDREP: End repeat block - decrement counter and loop or exit */
			if (exec->controlDepth > 0 && exec->controlStack[exec->controlDepth - 1].type == CONTROL_FLOW_REP) {
				ControlFlowFrame *frame = &exec->controlStack[exec->controlDepth - 1];

				if (exec->breakRequested) {
					/* BRK was executed - exit loop */
					exec->breakRequested = GL_FALSE;
					exec->controlDepth--;
				} else {
					frame->repCounter--;
					if (frame->repCounter > 0) {
						/* Jump back to loop start */
						exec->IP = frame->startIP;
					} else {
						/* Counter exhausted - exit loop */
						exec->controlDepth--;
					}
				}
			}
			break;
		}

		case OpcodeBRK: {
			/* BRK: Break out of innermost loop (LOOP or REP) */
			/* Find innermost loop on control stack */
			for (GLint depth = exec->controlDepth - 1; depth >= 0; depth--) {
				if (exec->controlStack[depth].type == CONTROL_FLOW_LOOP ||
					exec->controlStack[depth].type == CONTROL_FLOW_REP) {
					/* Set break flag and jump to end of loop */
					exec->breakRequested = GL_TRUE;
					GLsizei endIP = exec->controlStack[depth].endIP;
					if (endIP >= 0) {
						exec->IP = endIP;
					}
					break;
				}
			}
			break;
		}

		case OpcodeCAL: {
			/* CAL: Subroutine call */
			InstBranch *branch = &inst->branch;

			/* Push return address on call stack */
			if (exec->callDepth < MAX_CALL_STACK_DEPTH) {
				CallStackFrame *frame = &exec->callStack[exec->callDepth++];
				frame->returnIP = exec->IP + 1;  /* Return to next instruction */
			}

			/* Jump to subroutine label */
			GLsizei targetIP = FindLabelTarget(exec, branch->target);
			if (targetIP >= 0) {
				exec->IP = targetIP - 1;  /* -1 because IP will be incremented */
			}
			break;
		}

		case OpcodeRET: {
			/* RET: Return from subroutine */
			if (exec->callDepth > 0) {
				CallStackFrame *frame = &exec->callStack[--exec->callDepth];
				exec->IP = frame->returnIP - 1;  /* -1 because IP will be incremented */
			}
			break;
		}

		case OpcodeBRA: {
			/* BRA: Conditional branch based on condition code */
			InstBranch *branch = &inst->branch;

			/* Evaluate condition using condition code register */
			if (EvaluateCondition(&inst->cond, (GLfloat)exec->conditionCode)) {
				/* Branch taken - jump to label */
				GLsizei targetIP = FindLabelTarget(exec, branch->target);
				if (targetIP >= 0) {
					exec->IP = targetIP - 1;  /* -1 because IP will be incremented */
				}
			}
			break;
		}

		case OpcodeSCC: {
			/* SCC: Set condition code register */
			LoadVec4(&src0, &inst->unary.arg, vctx, fctx);

			/* Store condition code based on first component
			 * Use simple encoding: 0 = false, non-zero = true
			 */
			exec->conditionCode = (src0.base[0] != 0.0f) ? 1 : 0;
			break;
		}

		case OpcodePHI: {
			/* PHI: SSA phi node - no runtime operation needed
			 * The SSA form has already been resolved by the compiler
			 */
			break;
		}

		case OpcodeKIL: {
			/* KIL: Kill/discard fragment */
			InstCond *cond = &inst->cond;

			if (fctx) {
				/* Fragment shader - evaluate kill condition */
				LoadVec4(&src0, &cond->arg, NULL, fctx);

				/* Discard if any component < 0 */
				for (i = 0; i < 4; i++) {
					if (src0.base[i] < 0.0f) {
						return GL_FALSE;  /* Discard fragment */
					}
				}
			}
			break;
		}

		/* ===== TEXTURE SAMPLING ===== */

		case OpcodeTEX:
		case OpcodeTEX_SAT:
		case OpcodeTXB:
		case OpcodeTXB_SAT:
		case OpcodeTXP:
		case OpcodeTXP_SAT:
		case OpcodeTXL:
		case OpcodeTXL_SAT: {
			Vec4f coords, dx, dy;
			LoadVec4(&coords, &inst->tex.coords, vctx, fctx);
			GlesMemset(&dx, 0, sizeof(Vec4f));
			GlesMemset(&dy, 0, sizeof(Vec4f));
			GlesMemset(&result, 0, sizeof(Vec4f));

			/* Get texture unit */
			GLsizei samplerIndex = (inst->tex.sampler && inst->tex.sampler->location >= 0) ?
				(inst->tex.sampler->location + inst->tex.offset) : 0;

			TextureImageUnit *unit = vctx ?
				&vctx->textureImageUnit[samplerIndex] :
				&fctx->textureImageUnit[samplerIndex];

			/* Call appropriate sampling function based on target */
			switch (inst->tex.target) {
				case TextureTarget2D:
					GlesTextureSample2D(unit, coords.base, dx.base, dy.base, result.base);
					break;
				case TextureTarget3D:
					GlesTextureSample3D(unit, coords.base, dx.base, dy.base, result.base);
					break;
				case TextureTargetCube:
					GlesTextureSampleCube(unit, coords.base, dx.base, dy.base, result.base);
					break;
				default:
					GlesMemset(&result, 0, sizeof(Vec4f));
					break;
			}

			if (inst->base.op == OpcodeTEX_SAT || inst->base.op == OpcodeTXB_SAT ||
			    inst->base.op == OpcodeTXP_SAT || inst->base.op == OpcodeTXL_SAT) {
				for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
			}
			StoreVec4(&inst->tex.alu.dst, &result, vctx, fctx);
			break;
		}

		/* ===== METADATA INSTRUCTIONS ===== */

		case OpcodeINPUT:
		case OpcodeOUTPUT:
		case OpcodePARAM:
		case OpcodeTEMP:
		case OpcodeADDRESS:
			/* No runtime operation */
			break;

		default:
			/* Unsupported opcode - continue execution */
			break;
	}

	return GL_TRUE;
}

/**
 * IL Interpreter - fallback when JIT compilation fails or is disabled
 */
GLboolean GlesInterpretVertexShader(const VertexContext *context) {
	State *state;
	Program *program;
	Executable *executable;
	Linker *linker;
	ExecState exec;
	Block *block;
	Inst *inst;

	/* Get the shader program from the context */
	if (!context || !context->state) {
		return GL_FALSE;
	}

	state = context->state;
	program = GlesGetProgramObject(state, state->program);
	if (!program || !program->executable) {
		return GL_FALSE;
	}

	executable = program->executable;

	/* Get the linker from the executable's data segment */
	if (!executable->vertex.data.base) {
		return GL_FALSE;
	}

	linker = (Linker *)executable->vertex.data.base;

	if (!linker->program) {
		return GL_FALSE;
	}

	/* Initialize execution state */
	GlesMemset(&exec, 0, sizeof(ExecState));
	exec.numInstructions = 0;
	exec.IP = 0;
	exec.controlDepth = 0;
	exec.callDepth = 0;
	exec.conditionCode = 0;
	exec.breakRequested = GL_FALSE;

	/* First pass: Build instruction array */
	for (block = linker->program->blocks.head; block; block = block->next) {
		for (inst = block->first; inst; inst = inst->base.next) {
			if (exec.numInstructions < MAX_INSTRUCTIONS) {
				exec.instructions[exec.numInstructions++] = inst;
			}
		}
	}

	/* Second pass: Execute with control flow */
	while (exec.IP < exec.numInstructions) {
		inst = exec.instructions[exec.IP];

		if (!ExecuteInstruction(inst, &exec, (VertexContext *)context, NULL)) {
			return GL_FALSE;  /* KIL or error */
		}

		exec.IP++;
	}

	return GL_TRUE;
}

/**
 * IL Interpreter for fragment shaders
 */
GLboolean GlesInterpretFragmentShader(const FragContext *context) {
	State *state;
	Program *program;
	Executable *executable;
	Linker *linker;
	ExecState exec;
	Block *block;
	Inst *inst;

	/* Get the shader program from the context */
	if (!context || !context->state) {
		return GL_FALSE;
	}

	state = context->state;
	program = GlesGetProgramObject(state, state->program);
	if (!program || !program->executable) {
		return GL_FALSE;
	}

	executable = program->executable;

	/* Get the linker from the executable's data segment */
	if (!executable->fragment.data.base) {
		return GL_FALSE;
	}

	linker = (Linker *)executable->fragment.data.base;

	if (!linker->program) {
		return GL_FALSE;
	}

	/* Initialize execution state */
	GlesMemset(&exec, 0, sizeof(ExecState));
	exec.numInstructions = 0;
	exec.IP = 0;
	exec.controlDepth = 0;
	exec.callDepth = 0;
	exec.conditionCode = 0;
	exec.breakRequested = GL_FALSE;

	/* First pass: Build instruction array */
	for (block = linker->program->blocks.head; block; block = block->next) {
		for (inst = block->first; inst; inst = inst->base.next) {
			if (exec.numInstructions < MAX_INSTRUCTIONS) {
				exec.instructions[exec.numInstructions++] = inst;
			}
		}
	}

	/* Second pass: Execute with control flow */
	while (exec.IP < exec.numInstructions) {
		inst = exec.instructions[exec.IP];

		if (!ExecuteInstruction(inst, &exec, NULL, (FragContext *)context)) {
			return GL_FALSE;  /* KIL or error */
		}

		exec.IP++;
	}

	return GL_TRUE;
}
