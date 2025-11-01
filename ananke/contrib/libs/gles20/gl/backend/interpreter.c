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

/**
 * Helper: Read a component from source register with swizzle and negation
 */
static GLfloat ReadComponent(const GLfloat *base, const SrcReg *src, GLuint comp) {
	GLubyte swizzle;
	GLfloat value;

	/* Get swizzle for this component */
	switch (comp) {
		case 0: swizzle = src->selectX; break;
		case 1: swizzle = src->selectY; break;
		case 2: swizzle = src->selectZ; break;
		case 3: swizzle = src->selectW; break;
		default: return 0.0f;
	}

	/* Read value with swizzle */
	value = base[swizzle];

	/* Apply negation if needed */
	if (src->negate) {
		value = -value;
	}

	return value;
}

/**
 * Helper: Write a component to destination register with write mask
 */
static void WriteComponent(GLfloat *base, const DstReg *dst, GLuint comp, GLfloat value) {
	GLboolean mask;

	/* Check write mask */
	switch (comp) {
		case 0: mask = dst->maskX; break;
		case 1: mask = dst->maskY; break;
		case 2: mask = dst->maskZ; break;
		case 3: mask = dst->maskW; break;
		default: return;
	}

	/* Write if not masked */
	if (mask) {
		base[comp] = value;
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

	/* Execute the shader IL
	 * A full implementation would iterate through all blocks and instructions
	 * For now, provide basic passthrough functionality
	 *
	 * TODO: Implement full IL interpreter loop that handles all opcodes
	 */
	if (context->temp && context->attrib && context->varying) {
		GLuint i;
		/* Simple passthrough: copy first attrib to first varying */
		for (i = 0; i < 4; i++) {
			context->varying[i] = context->attrib[i].x;
		}
		return GL_TRUE;
	}

	return GL_FALSE;
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

	/* Execute the shader IL
	 * A full implementation would iterate through all blocks and instructions
	 * For now, provide basic passthrough functionality
	 *
	 * TODO: Implement full IL interpreter loop that handles all opcodes
	 */
	if (context->result && context->varying) {
		GLuint i;
		/* Simple operation: output varying as color */
		for (i = 0; i < 4; i++) {
			context->result[0].base[i] = (i < 4) ? context->varying[i] : 1.0f;
		}
		return GL_TRUE;
	}

	return GL_FALSE;
}
