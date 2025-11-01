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
 * IL Interpreter - fallback when JIT compilation fails or is disabled
 * Executes shader IL instructions directly
 *
 * This interpreter retrieves the shader program (Linker) from the executable
 * and interprets the IL instructions.
 *
 * The Linker pointer is stored in the executable's data segment by
 * GlesGenerateExecutable when JIT compilation fails.
 */
GLboolean GlesInterpretVertexShader(const VertexContext *context) {
	State *state;
	Program *program;
	Executable *executable;
	Linker *linker;

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

	/* Get the linker from the executable's data segment
	 * The linker pointer was stored by GlesGenerateExecutable
	 */
	if (!executable->vertex.data.base) {
		/* No linker stored - JIT compilation must have succeeded
		 * This should not happen since we're in the interpreter
		 */
		return GL_FALSE;
	}

	linker = (Linker *)executable->vertex.data.base;

	/* Execute the shader IL by iterating through all blocks and instructions */
	if (!linker->program) {
		return GL_FALSE;
	}

	Block *block;
	for (block = linker->program->blocks.head; block; block = block->next) {
		Inst *inst;
		for (inst = block->first; inst; inst = inst->base.next) {
			Vec4f src0, src1, src2, result;
			GLuint i;

			switch (inst->base.op) {
				case OpcodeMOV:
				case OpcodeMOV_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					if (inst->base.op == OpcodeMOV_SAT) {
						for (i = 0; i < 4; i++) {
							src0.base[i] = (src0.base[i] < 0.0f) ? 0.0f : ((src0.base[i] > 1.0f) ? 1.0f : src0.base[i]);
						}
					}
					StoreVec4(&inst->unary.alu.dst, &src0, (VertexContext *)context, NULL);
					break;

				case OpcodeADD:
				case OpcodeADD_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = src0.base[i] + src1.base[i];
					if (inst->base.op == OpcodeADD_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeMUL:
				case OpcodeMUL_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = src0.base[i] * src1.base[i];
					if (inst->base.op == OpcodeMUL_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeDP3:
				case OpcodeDP3_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					result.base[0] = result.base[1] = result.base[2] = result.base[3] =
						src0.base[0] * src1.base[0] + src0.base[1] * src1.base[1] + src0.base[2] * src1.base[2];
					if (inst->base.op == OpcodeDP3_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeDP4:
				case OpcodeDP4_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					result.base[0] = result.base[1] = result.base[2] = result.base[3] =
						src0.base[0] * src1.base[0] + src0.base[1] * src1.base[1] +
						src0.base[2] * src1.base[2] + src0.base[3] * src1.base[3];
					if (inst->base.op == OpcodeDP4_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeMAD:
				case OpcodeMAD_SAT:
					LoadVec4(&src0, &inst->ternary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->ternary.arg1, (VertexContext *)context, NULL);
					LoadVec4(&src2, &inst->ternary.arg2, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = src0.base[i] * src1.base[i] + src2.base[i];
					if (inst->base.op == OpcodeMAD_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->ternary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeSUB:
				case OpcodeSUB_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = src0.base[i] - src1.base[i];
					if (inst->base.op == OpcodeSUB_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeMIN:
				case OpcodeMIN_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] < src1.base[i]) ? src0.base[i] : src1.base[i];
					if (inst->base.op == OpcodeMIN_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeMAX:
				case OpcodeMAX_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] > src1.base[i]) ? src0.base[i] : src1.base[i];
					if (inst->base.op == OpcodeMAX_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeRCP:
				case OpcodeRCP_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = 1.0f / src0.base[i];
					if (inst->base.op == OpcodeRCP_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeRSQ:
				case OpcodeRSQ_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = 1.0f / sqrtf(fabsf(src0.base[i]));
					if (inst->base.op == OpcodeRSQ_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeSGE:
				case OpcodeSGE_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] >= src1.base[i]) ? 1.0f : 0.0f;
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeSLT:
				case OpcodeSLT_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] < src1.base[i]) ? 1.0f : 0.0f;
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeFLR:
				case OpcodeFLR_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = floorf(src0.base[i]);
					if (inst->base.op == OpcodeFLR_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeFRC:
				case OpcodeFRC_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = src0.base[i] - floorf(src0.base[i]);
					if (inst->base.op == OpcodeFRC_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeEXP:
				case OpcodeEXP_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = expf(src0.base[i]);
					if (inst->base.op == OpcodeEXP_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeLOG:
				case OpcodeLOG_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = logf(fabsf(src0.base[i]));
					if (inst->base.op == OpcodeLOG_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeEX2:
				case OpcodeEX2_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = exp2f(src0.base[i]);
					if (inst->base.op == OpcodeEX2_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeLG2:
				case OpcodeLG2_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = log2f(fabsf(src0.base[i]));
					if (inst->base.op == OpcodeLG2_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodePOW:
				case OpcodePOW_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = powf(fabsf(src0.base[i]), src1.base[i]);
					if (inst->base.op == OpcodePOW_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeSIN:
				case OpcodeSIN_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = sinf(src0.base[i]);
					if (inst->base.op == OpcodeSIN_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeCOS:
				case OpcodeCOS_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = cosf(src0.base[i]);
					if (inst->base.op == OpcodeCOS_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeABS:
				case OpcodeABS_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = fabsf(src0.base[i]);
					if (inst->base.op == OpcodeABS_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeLRP:
				case OpcodeLRP_SAT:
					LoadVec4(&src0, &inst->ternary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->ternary.arg1, (VertexContext *)context, NULL);
					LoadVec4(&src2, &inst->ternary.arg2, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = src0.base[i] * src1.base[i] + (1.0f - src0.base[i]) * src2.base[i];
					if (inst->base.op == OpcodeLRP_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->ternary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeDP2:
				case OpcodeDP2_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					result.base[0] = result.base[1] = result.base[2] = result.base[3] =
						src0.base[0] * src1.base[0] + src0.base[1] * src1.base[1];
					if (inst->base.op == OpcodeDP2_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeDPH:
				case OpcodeDPH_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					result.base[0] = result.base[1] = result.base[2] = result.base[3] =
						src0.base[0] * src1.base[0] + src0.base[1] * src1.base[1] + src0.base[2] * src1.base[2] + src1.base[3];
					if (inst->base.op == OpcodeDPH_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeXPD:
				case OpcodeXPD_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					result.base[0] = src0.base[1] * src1.base[2] - src0.base[2] * src1.base[1];
					result.base[1] = src0.base[2] * src1.base[0] - src0.base[0] * src1.base[2];
					result.base[2] = src0.base[0] * src1.base[1] - src0.base[1] * src1.base[0];
					result.base[3] = 1.0f;
					if (inst->base.op == OpcodeXPD_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeCMP:
				case OpcodeCMP_SAT:
					LoadVec4(&src0, &inst->ternary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->ternary.arg1, (VertexContext *)context, NULL);
					LoadVec4(&src2, &inst->ternary.arg2, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] < 0.0f) ? src1.base[i] : src2.base[i];
					if (inst->base.op == OpcodeCMP_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->ternary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeSSG:
				case OpcodeSSG_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] > 0.0f) ? 1.0f : ((src0.base[i] < 0.0f) ? -1.0f : 0.0f);
					if (inst->base.op == OpcodeSSG_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeSLE:
				case OpcodeSLE_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] <= src1.base[i]) ? 1.0f : 0.0f;
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeSGT:
				case OpcodeSGT_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] > src1.base[i]) ? 1.0f : 0.0f;
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeSCS:
				case OpcodeSCS_SAT:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					result.base[0] = cosf(src0.base[0]);
					result.base[1] = sinf(src0.base[0]);
					result.base[2] = 0.0f;
					result.base[3] = 1.0f;
					if (inst->base.op == OpcodeSCS_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeDST:
				case OpcodeDST_SAT:
					LoadVec4(&src0, &inst->binary.arg0, (VertexContext *)context, NULL);
					LoadVec4(&src1, &inst->binary.arg1, (VertexContext *)context, NULL);
					result.base[0] = 1.0f;
					result.base[1] = src0.base[1] * src1.base[1];
					result.base[2] = src0.base[2];
					result.base[3] = src1.base[3];
					if (inst->base.op == OpcodeDST_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				case OpcodeARL:
					LoadVec4(&src0, &inst->unary.arg, (VertexContext *)context, NULL);
					for (i = 0; i < 4; i++) result.base[i] = floorf(src0.base[i]);
					StoreVec4(&inst->unary.alu.dst, &result, (VertexContext *)context, NULL);
					break;

				/* Control flow - execute linearly (structure handles flow) */
				case OpcodeIF:
				case OpcodeELSE:
				case OpcodeENDIF:
				case OpcodeLOOP:
				case OpcodeENDLOOP:
				case OpcodeREP:
				case OpcodeENDREP:
				case OpcodeBRK:
				case OpcodeCAL:
				case OpcodeRET:
				case OpcodeBRA:
				case OpcodeSCC:
				case OpcodePHI:
					break;

				case OpcodeKIL:
					/* Discard fragment */
					return GL_FALSE;

				/* Texture sampling - vertex shaders can sample textures */
				case OpcodeTEX:
				case OpcodeTEX_SAT:
				case OpcodeTXB:
				case OpcodeTXB_SAT:
				case OpcodeTXP:
				case OpcodeTXP_SAT:
				case OpcodeTXL:
				case OpcodeTXL_SAT:
					/* Texture sampling - call runtime texture functions */
					{
						Vec4f coords, dx, dy;
						LoadVec4(&coords, &inst->tex.coords, (VertexContext *)context, NULL);
						GlesMemset(&dx, 0, sizeof(Vec4f));
						GlesMemset(&dy, 0, sizeof(Vec4f));
						GlesMemset(&result, 0, sizeof(Vec4f));

						/* Get texture unit */
						GLsizei samplerIndex = (inst->tex.sampler && inst->tex.sampler->location >= 0) ?
							(inst->tex.sampler->location + inst->tex.offset) : 0;
						TextureImageUnit *unit = &((VertexContext *)context)->textureImageUnit[samplerIndex];

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
						StoreVec4(&inst->tex.alu.dst, &result, (VertexContext *)context, NULL);
					}
					break;

				/* Declaration and metadata instructions - no execution needed */
				case OpcodeINPUT:
				case OpcodeOUTPUT:
				case OpcodePARAM:
				case OpcodeTEMP:
				case OpcodeADDRESS:
					break;

				default:
					/* Unsupported opcode - for now, continue execution */
					break;
			}
		}
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

	/* Get the linker from the executable's data segment
	 * The linker pointer was stored by GlesGenerateExecutable
	 */
	if (!executable->fragment.data.base) {
		/* No linker stored - JIT compilation must have succeeded
		 * This should not happen since we're in the interpreter
		 */
		return GL_FALSE;
	}

	linker = (Linker *)executable->fragment.data.base;

	/* Execute the shader IL by iterating through all blocks and instructions */
	if (!linker->program) {
		return GL_FALSE;
	}

	Block *block;
	for (block = linker->program->blocks.head; block; block = block->next) {
		Inst *inst;
		for (inst = block->first; inst; inst = inst->base.next) {
			Vec4f src0, src1, src2, result;
			GLuint i;

			switch (inst->base.op) {
				case OpcodeMOV:
				case OpcodeMOV_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					if (inst->base.op == OpcodeMOV_SAT) {
						for (i = 0; i < 4; i++) {
							src0.base[i] = (src0.base[i] < 0.0f) ? 0.0f : ((src0.base[i] > 1.0f) ? 1.0f : src0.base[i]);
						}
					}
					StoreVec4(&inst->unary.alu.dst, &src0, NULL, (FragContext *)context);
					break;

				case OpcodeADD:
				case OpcodeADD_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = src0.base[i] + src1.base[i];
					if (inst->base.op == OpcodeADD_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeMUL:
				case OpcodeMUL_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = src0.base[i] * src1.base[i];
					if (inst->base.op == OpcodeMUL_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeDP3:
				case OpcodeDP3_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					result.base[0] = result.base[1] = result.base[2] = result.base[3] =
						src0.base[0] * src1.base[0] + src0.base[1] * src1.base[1] + src0.base[2] * src1.base[2];
					if (inst->base.op == OpcodeDP3_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeDP4:
				case OpcodeDP4_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					result.base[0] = result.base[1] = result.base[2] = result.base[3] =
						src0.base[0] * src1.base[0] + src0.base[1] * src1.base[1] +
						src0.base[2] * src1.base[2] + src0.base[3] * src1.base[3];
					if (inst->base.op == OpcodeDP4_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeMAD:
				case OpcodeMAD_SAT:
					LoadVec4(&src0, &inst->ternary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->ternary.arg1, NULL, (FragContext *)context);
					LoadVec4(&src2, &inst->ternary.arg2, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = src0.base[i] * src1.base[i] + src2.base[i];
					if (inst->base.op == OpcodeMAD_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->ternary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeSUB:
				case OpcodeSUB_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = src0.base[i] - src1.base[i];
					if (inst->base.op == OpcodeSUB_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeMIN:
				case OpcodeMIN_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] < src1.base[i]) ? src0.base[i] : src1.base[i];
					if (inst->base.op == OpcodeMIN_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeMAX:
				case OpcodeMAX_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] > src1.base[i]) ? src0.base[i] : src1.base[i];
					if (inst->base.op == OpcodeMAX_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeRCP:
				case OpcodeRCP_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = 1.0f / src0.base[i];
					if (inst->base.op == OpcodeRCP_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeRSQ:
				case OpcodeRSQ_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = 1.0f / sqrtf(fabsf(src0.base[i]));
					if (inst->base.op == OpcodeRSQ_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeSGE:
				case OpcodeSGE_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] >= src1.base[i]) ? 1.0f : 0.0f;
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeSLT:
				case OpcodeSLT_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] < src1.base[i]) ? 1.0f : 0.0f;
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeFLR:
				case OpcodeFLR_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = floorf(src0.base[i]);
					if (inst->base.op == OpcodeFLR_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeFRC:
				case OpcodeFRC_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = src0.base[i] - floorf(src0.base[i]);
					if (inst->base.op == OpcodeFRC_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeEXP:
				case OpcodeEXP_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = expf(src0.base[i]);
					if (inst->base.op == OpcodeEXP_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeLOG:
				case OpcodeLOG_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = logf(fabsf(src0.base[i]));
					if (inst->base.op == OpcodeLOG_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeEX2:
				case OpcodeEX2_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = exp2f(src0.base[i]);
					if (inst->base.op == OpcodeEX2_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeLG2:
				case OpcodeLG2_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = log2f(fabsf(src0.base[i]));
					if (inst->base.op == OpcodeLG2_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodePOW:
				case OpcodePOW_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = powf(fabsf(src0.base[i]), src1.base[i]);
					if (inst->base.op == OpcodePOW_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeSIN:
				case OpcodeSIN_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = sinf(src0.base[i]);
					if (inst->base.op == OpcodeSIN_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeCOS:
				case OpcodeCOS_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = cosf(src0.base[i]);
					if (inst->base.op == OpcodeCOS_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeABS:
				case OpcodeABS_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = fabsf(src0.base[i]);
					if (inst->base.op == OpcodeABS_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeLRP:
				case OpcodeLRP_SAT:
					LoadVec4(&src0, &inst->ternary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->ternary.arg1, NULL, (FragContext *)context);
					LoadVec4(&src2, &inst->ternary.arg2, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = src0.base[i] * src1.base[i] + (1.0f - src0.base[i]) * src2.base[i];
					if (inst->base.op == OpcodeLRP_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->ternary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeDP2:
				case OpcodeDP2_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					result.base[0] = result.base[1] = result.base[2] = result.base[3] =
						src0.base[0] * src1.base[0] + src0.base[1] * src1.base[1];
					if (inst->base.op == OpcodeDP2_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeDPH:
				case OpcodeDPH_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					result.base[0] = result.base[1] = result.base[2] = result.base[3] =
						src0.base[0] * src1.base[0] + src0.base[1] * src1.base[1] + src0.base[2] * src1.base[2] + src1.base[3];
					if (inst->base.op == OpcodeDPH_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeXPD:
				case OpcodeXPD_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					result.base[0] = src0.base[1] * src1.base[2] - src0.base[2] * src1.base[1];
					result.base[1] = src0.base[2] * src1.base[0] - src0.base[0] * src1.base[2];
					result.base[2] = src0.base[0] * src1.base[1] - src0.base[1] * src1.base[0];
					result.base[3] = 1.0f;
					if (inst->base.op == OpcodeXPD_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeCMP:
				case OpcodeCMP_SAT:
					LoadVec4(&src0, &inst->ternary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->ternary.arg1, NULL, (FragContext *)context);
					LoadVec4(&src2, &inst->ternary.arg2, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] < 0.0f) ? src1.base[i] : src2.base[i];
					if (inst->base.op == OpcodeCMP_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->ternary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeSSG:
				case OpcodeSSG_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] > 0.0f) ? 1.0f : ((src0.base[i] < 0.0f) ? -1.0f : 0.0f);
					if (inst->base.op == OpcodeSSG_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeSLE:
				case OpcodeSLE_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] <= src1.base[i]) ? 1.0f : 0.0f;
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeSGT:
				case OpcodeSGT_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = (src0.base[i] > src1.base[i]) ? 1.0f : 0.0f;
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeSCS:
				case OpcodeSCS_SAT:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					result.base[0] = cosf(src0.base[0]);
					result.base[1] = sinf(src0.base[0]);
					result.base[2] = 0.0f;
					result.base[3] = 1.0f;
					if (inst->base.op == OpcodeSCS_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeDST:
				case OpcodeDST_SAT:
					LoadVec4(&src0, &inst->binary.arg0, NULL, (FragContext *)context);
					LoadVec4(&src1, &inst->binary.arg1, NULL, (FragContext *)context);
					result.base[0] = 1.0f;
					result.base[1] = src0.base[1] * src1.base[1];
					result.base[2] = src0.base[2];
					result.base[3] = src1.base[3];
					if (inst->base.op == OpcodeDST_SAT) {
						for (i = 0; i < 4; i++) result.base[i] = (result.base[i] < 0.0f) ? 0.0f : ((result.base[i] > 1.0f) ? 1.0f : result.base[i]);
					}
					StoreVec4(&inst->binary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				case OpcodeARL:
					LoadVec4(&src0, &inst->unary.arg, NULL, (FragContext *)context);
					for (i = 0; i < 4; i++) result.base[i] = floorf(src0.base[i]);
					StoreVec4(&inst->unary.alu.dst, &result, NULL, (FragContext *)context);
					break;

				/* Control flow - execute linearly (structure handles flow) */
				case OpcodeIF:
				case OpcodeELSE:
				case OpcodeENDIF:
				case OpcodeLOOP:
				case OpcodeENDLOOP:
				case OpcodeREP:
				case OpcodeENDREP:
				case OpcodeBRK:
				case OpcodeCAL:
				case OpcodeRET:
				case OpcodeBRA:
				case OpcodeSCC:
				case OpcodePHI:
					break;

				case OpcodeKIL:
					/* Discard fragment */
					return GL_FALSE;

				/* Texture sampling - fragment shaders use textures extensively */
				case OpcodeTEX:
				case OpcodeTEX_SAT:
				case OpcodeTXB:
				case OpcodeTXB_SAT:
				case OpcodeTXP:
				case OpcodeTXP_SAT:
				case OpcodeTXL:
				case OpcodeTXL_SAT:
					/* Texture sampling - call runtime texture functions */
					{
						Vec4f coords, dx, dy;
						LoadVec4(&coords, &inst->tex.coords, NULL, (FragContext *)context);
						GlesMemset(&dx, 0, sizeof(Vec4f));
						GlesMemset(&dy, 0, sizeof(Vec4f));
						GlesMemset(&result, 0, sizeof(Vec4f));

						/* Get texture unit */
						GLsizei samplerIndex = (inst->tex.sampler && inst->tex.sampler->location >= 0) ?
							(inst->tex.sampler->location + inst->tex.offset) : 0;
						TextureImageUnit *unit = &((FragContext *)context)->textureImageUnit[samplerIndex];

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
						StoreVec4(&inst->tex.alu.dst, &result, NULL, (FragContext *)context);
					}
					break;

				/* Declaration and metadata instructions - no execution needed */
				case OpcodeINPUT:
				case OpcodeOUTPUT:
				case OpcodePARAM:
				case OpcodeTEMP:
				case OpcodeADDRESS:
					break;

				default:
					/* Unsupported opcode - for now, continue execution */
					break;
			}
		}
	}

	return GL_TRUE;
}
