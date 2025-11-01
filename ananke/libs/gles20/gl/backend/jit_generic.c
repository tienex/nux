/*
** ==========================================================================
**
** JIT Backend - Generic Fallback
**
** Generic fallback for architectures without specific JIT support
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
#include "backend/jit.h"
#include "backend/jit_internal.h"

#if GLES_JIT_ARCH_GENERIC

/*
** --------------------------------------------------------------------------
** Generic architecture-independent implementation
** --------------------------------------------------------------------------
*/

/**
 * Architecture-specific optimization for generic architectures.
 *
 * This function is called for architectures without specific JIT support.
 * It simply uses the interpreter from jit_common.c without any
 * architecture-specific optimizations.
 *
 * @param executable The executable to optimize
 * @param linker The linker containing the shader IL
 */
void GlesOptimizeExecutable(Executable *executable, Linker *linker) {
	/* Generic fallback: Use interpreter (already set in jit_common.c)
	 * No architecture-specific optimizations available
	 */
}

#endif /* GLES_JIT_ARCH_GENERIC */
