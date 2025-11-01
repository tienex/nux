/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory - Copy memory (non-overlapping buffers)
 * RtlMoveMemory - Move memory (may overlap)
 *
 * Optimized memory copy/move for AMD64
 */

/* RtlCopyMemory - buffers must not overlap */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory)
	movq	%rdx,%rcx
	movq	%rdi,%rax	/* return destination */
	mov	%rdi,%r11

	shrq	$3,%rcx
	jz	8f

	lea	-8(%rdi,%rdx),%r9
	mov	-8(%rsi,%rdx),%r8
	lea	-8(%rsi,%rdx),%r10

	/* Check for partial overlap (dest < src) */
	cmpq	%rdi,%r9
	j b	10f

	/* Copy forwards */
	rep
	movsq
	movq	%r8,(%r9)
	ret

8:	/* Less than 8 bytes */
	testb	$4,%dl
	jz	1f
	movsl
1:	testb	$2,%dl
	jz	1f
	movsw
1:	testb	$1,%dl
	jz	1f
	movsb
1:	ret

10:	/* Potential overlap - use memmove logic */
	cmpq	%rsi,%r10
	jae	12f

	/* Copy backwards */
	leaq	-8(%rdi,%rdx),%rdi
	leaq	-8(%rsi,%rdx),%rsi
	std
	rep
	movsq
	cld
	movq	%r8,(%r9)
	ret

12:	/* No overlap, copy forwards */
	rep
	movsq
	movq	%r8,(%r9)
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory)

/* RtlMoveMemory - buffers may overlap */
ANX_ASM_FUNCTION_BEGIN(RtlMoveMemory)
	movq	%rdx,%rcx
	movq	%rdi,%rax	/* return destination */
	mov	%rdi,%r11

	movq	%rdi,%r8
	subq	%rsi,%r8

	shrq	$3,%rcx
	jz	8f

	lea	-8(%rdi,%rdx),%r9
	mov	-8(%rsi,%rdx),%r8
	lea	-8(%rsi,%rdx),%r10

	cmpq	%rdi,%r9
	jb	10f

	cmpq	%rsi,%r10
	jae	12f

	/* Copy backwards */
	leaq	-8(%rdi,%rdx),%rdi
	leaq	-8(%rsi,%rdx),%rsi
	std
	rep
	movsq
	cld
	movq	%r8,(%r9)
	ret

8:	/* Less than 8 bytes */
	testb	$4,%dl
	jz	1f
	movsl
1:	testb	$2,%dl
	jz	1f
	movsw
1:	testb	$1,%dl
	jz	1f
	movsb
1:	ret

10:	/* Copy forwards */
	rep
	movsq
	movq	%r8,(%r9)
	ret

12:	/* Copy forwards */
	rep
	movsq
	movq	%r8,(%r9)
	ret
ANX_ASM_FUNCTION_END(RtlMoveMemory)
