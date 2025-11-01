/* @file
 * cCRT - Compiler Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*	$NetBSD: crtbegin.S,v 1.2 2010/11/30 18:37:59 joerg Exp $	*/
/*-
 * Copyright (c) 2010 Joerg Sonnenberger <joerg@NetBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <ananke/asmdefs.h>

//RCSID("$NetBSD: crtbegin.S,v 1.2 2010/11/30 18:37:59 joerg Exp $")

	.section	.ctors, "aw", @progbits
	.align 4
__CTOR_LIST__:
	.long -1

	.section	.dtors, "aw", @progbits
	.align 4
__DTOR_LIST__:
	.long -1

	.section	.data.rel, "aw", @progbits
	.align 4

__initialized:
	.zero	1
__finished:
	.zero	1

	.text
	.weak	__cxa_finalize

__do_global_dtors_aux:
	cmpb	$0, __finished
	je	1f
	ret
1:
	pushl	%ebx
	movb	$1, __finished


	leal	4+__DTOR_LIST__, %ebx
3:
	movl	(%ebx), %eax
	testl	%eax, %eax
	je	4f
	call	*%eax
	addl	$4, %ebx
	jmp	3b
4:

	popl	%ebx
	ret


__do_global_ctors_aux:
	cmpb	$0, __initialized
	je	1f
	ret
1:
	pushl	%ebx
	movb	$1, __initialized


	leal	-4+__CTOR_LIST_END__, %ebx
4:
	movl	(%ebx), %eax
	cmpl	$-1, %eax
	je	5f
	call	*%eax
	subl	$4, %ebx
	jmp	4b

5:
	popl	%ebx

	ret

	.section	.init, "ax", @progbits
	call	__do_global_ctors_aux
	.section	.fini, "ax", @progbits
	call	__do_global_dtors_aux
