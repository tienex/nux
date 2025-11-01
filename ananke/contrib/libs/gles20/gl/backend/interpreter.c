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
