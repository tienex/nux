/*
** ==========================================================================
**
** Shader IL Interpreter - Header
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

#ifndef GLES_BACKEND_INTERPRETER_H
#define GLES_BACKEND_INTERPRETER_H

#include <GLES/gl.h>
#include "gl/state.h"

/**
 * Interpret vertex shader
 *
 * Simple fallback interpreter for when JIT compilation fails
 * or is not available. Provides basic passthrough functionality.
 *
 * @param context Vertex shader execution context
 * @return GL_TRUE on success, GL_FALSE on failure
 */
GLboolean GlesInterpretVertexShader(const VertexContext *context);

/**
 * Interpret fragment shader
 *
 * Simple fallback interpreter for when JIT compilation fails
 * or is not available. Provides basic color output functionality.
 *
 * @param context Fragment shader execution context
 * @return GL_TRUE on success, GL_FALSE on failure
 */
GLboolean GlesInterpretFragmentShader(const FragContext *context);

#endif /* GLES_BACKEND_INTERPRETER_H */
