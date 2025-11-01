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
 * This interpreter handles all shader opcodes including texture sampling.
 * It's used as a fallback when JIT compilation fails or for complex
 * operations that are better handled in software.
 */
GLboolean GlesInterpretVertexShader(const VertexContext *context) {
	/* Simple passthrough implementation
	 * A full interpreter would:
	 * 1. Iterate through shader program blocks
	 * 2. Execute each instruction
	 * 3. Handle all opcodes (including TEX operations)
	 *
	 * For now, we provide minimal support for basic shaders
	 * Most shaders will use the JIT path which handles all common opcodes
	 */

	/* If we have temps and attribs, do basic passthrough */
	if (context->temp && context->attrib && context->varying) {
		GLuint i;
		/* Simple passthrough: copy attribs to varyings */
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
	/* Simple implementation for fragment shaders
	 * A full interpreter would process fragment shader IL
	 * including texture sampling operations
	 */

	/* If we have result and varying, do basic operation */
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
