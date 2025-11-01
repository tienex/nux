/*
** ==========================================================================
**
** JIT Backend - Internal Interface
**
** Internal structures and functions for the JIT backend
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

#ifndef GLES_BACKEND_JIT_INTERNAL_H
#define GLES_BACKEND_JIT_INTERNAL_H 1

#include "frontend/linker.h"

/*
** --------------------------------------------------------------------------
** Architecture detection
** --------------------------------------------------------------------------
*/

/* Detect architecture at compile time */
#if defined(__i386__) || defined(_M_IX86)
#	define GLES_JIT_ARCH_I386 1
#elif defined(__x86_64__) && defined(__ILP32__)
#	define GLES_JIT_ARCH_X32 1
#elif defined(__riscv) && (__riscv_xlen == 32)
#	define GLES_JIT_ARCH_RISCV32 1
#elif defined(__riscv) && (__riscv_xlen == 64)
#	define GLES_JIT_ARCH_RISCV64 1
#else
#	define GLES_JIT_ARCH_GENERIC 1
#endif

/*
** --------------------------------------------------------------------------
** Internal functions
** --------------------------------------------------------------------------
*/

/**
 * Generate an executable from linked shader IL.
 * This is the main backend entry point.
 *
 * @param linker The linker containing the compiled and linked shader IL
 * @return Executable structure with function pointers, or NULL on failure
 */
Executable *GlesGenerateExecutable(Linker *linker);

/**
 * Architecture-specific optimization hook.
 * Called after creating the interpreter-based executable to allow
 * architecture-specific backends to replace the interpreter with
 * JIT-compiled code.
 *
 * @param executable The executable to optimize
 * @param linker The linker containing the shader IL
 */
void GlesOptimizeExecutable(Executable *executable, Linker *linker);

#endif /* GLES_BACKEND_JIT_INTERNAL_H */
