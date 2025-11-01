/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 optimized memory operations
 *
 * Original implementations from NetBSD, adapted to NT naming conventions:
 * - RtlCompareMemory: Compare memory regions
 * - RtlFillMemory: Fill memory with byte value
 * - RtlFindByteInMemory: Find byte in memory region
 *
 * Note: RtlCopyMemory and RtlMoveMemory use generic C implementations
 */

#include <ananke/asmdefs.h>

/*
 * RtlCompareMemory - Compare memory regions
 * 12(%esp): buffer1 (after push)
 * 16(%esp): buffer2 (after push)
 * 20(%esp): length (after push)
 * Returns: 0 if equal, <0 if buf1<buf2, >0 if buf1>buf2
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareMemory)
	pushl	%edi
	pushl	%esi
	movl	12(%esp),%edi
	movl	16(%esp),%esi

	movl	20(%esp),%ecx		/* compare by words */
	shrl	$2,%ecx
	repe
	cmpsl
	jne	L5			/* do we match so far? */

	movl	20(%esp),%ecx		/* compare remainder by bytes */
	andl	$3,%ecx
	repe
	cmpsb
	jne	L6			/* do we match? */

	xorl	%eax,%eax		/* we match, return zero	*/
	popl	%esi
	popl	%edi
	ret

L5:	movl	$4,%ecx			/* We know that one of the next	*/
	subl	%ecx,%edi		/* four pairs of bytes do not	*/
	subl	%ecx,%esi		/* match.			*/
	repe
	cmpsb
L6:	xorl	%eax,%eax		/* Perform unsigned comparison	*/
	movb	-1(%edi),%al
	xorl	%edx,%edx
	movb	-1(%esi),%dl
	subl    %edx,%eax
	popl	%esi
	popl	%edi
	ret
ANX_ASM_FUNCTION_END(RtlCompareMemory)

/*
 * RtlFillMemory - Fill memory with byte value
 * 8(%esp): buffer (after push)
 * 12(%esp): byte value (after push)
 * 16(%esp): length (after push in bzero path)
 * Returns: buffer pointer
 */
#ifdef BZERO
ANX_ASM_FUNCTION_BEGIN(bzero)
#else
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory)
#endif
#ifdef BZERO
	movl	8(%esp),%ecx
	xor	%eax,%eax
#else
	movl	12(%esp),%ecx
	movzbl	8(%esp),%eax		/* unsigned char, zero extend */
#endif
	cmpl	$0x0f,%ecx		/* avoid mispredicted branch... */

	pushl	%edi
	movl	8(%esp),%edi

	/*
	 * if the string is too short, it's really not worth the overhead
	 * of aligning to word boundries, etc.  So we jump to a plain
	 * unaligned set.
	 */
	jbe	.Lby_bytes

#ifndef BZERO
	movb	%al,%ah			/* copy char to all bytes in word */
	movl	%eax,%edx
	sall	$16,%eax
	orl	%edx,%eax
#endif

	movl	%edi,%edx		/* detect misalignment */
	neg	%edx
	andl	$7,%edx
	jnz	.Lalign
.Laligned:
	movl	%eax,-4(%edi,%ecx)	/* zap last 4 bytes */
	shrl	$2,%ecx			/* zero by words */
	rep
	stosl
.Ldone:
#ifndef BZERO
	movl	8(%esp),%eax		/* return address of buffer */
#endif
	pop	%edi
	ret

.Lalign:
	movl	%eax,(%edi)		/* zap first 8 bytes */
	movl	%eax,4(%edi)
	subl	%edx,%ecx		/* remove from main count */
	add	%edx,%edi
	jmp	.Laligned

.Lby_bytes:
	rep
	stosb

#ifndef BZERO
	movl	8(%esp),%eax		/* return address of buffer */
#endif
	popl	%edi
	ret
#ifdef BZERO
ANX_ASM_FUNCTION_END(bzero)
#else
ANX_ASM_FUNCTION_END(RtlFillMemory)
#endif

/*
 * RtlFindByteInMemory - Find byte in memory region
 * 8(%esp): buffer (after push)
 * 12(%esp): byte value to find (after push)
 * 16(%esp): length (after push)
 * Returns: pointer to byte or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindByteInMemory)
	pushl	%esi
	movl	8(%esp),%eax
	movzbl	12(%esp),%ecx
	movl	16(%esp),%esi

	/*
	 * Align to word boundary.
	 * Consider unrolling loop?
	 */
	testl	%esi,%esi	/* nbytes == 0? */
	je	.Lzero
.Lalign_fb:
	testb	$3,%al
	je	.Lword_aligned_fb
	cmpb	(%eax),%cl
	je	.Ldone_fb
	incl	%eax
	decl	%esi
	jnz	.Lalign_fb
	jmp	.Lzero_fb

.Lword_aligned_fb:
	/* copy char to all bytes in word */
	movb	%cl,%ch
	movl	%ecx,%edx
	sall	$16,%ecx
	orl	%edx,%ecx

	_ALIGN_TEXT
.Lloop_fb:
	cmpl	$3,%esi		/* nbytes > 4 */
	jbe	.Lbyte_fb
	movl	(%eax),%edx
	addl	$4,%eax
	xorl	%ecx,%edx
	subl	$4,%esi
	subl	$0x01010101,%edx
	testl	$0x80808080,%edx
	je	.Lloop_fb

	/*
	 * In rare cases, the above loop may exit prematurely. We must
	 * return to the loop if none of the bytes in the word are
	 * equal to ch.
	 */

	_ALIGN_TEXT
	cmpb	-4(%eax),%cl	/* 1st byte == ch? */
	movb	-3(%eax),%dl
	jne	1f
	subl	$4,%eax
	jmp	.Ldone_fb

	_ALIGN_TEXT
1:	cmpb	%dl,%cl		/* 2nd byte == ch? */
	movb	-2(%eax),%dl
	jne	1f
	subl	$3,%eax
	jmp	.Ldone_fb

	_ALIGN_TEXT
1:	cmpb	%dl,%cl		/* 3rd byte == ch? */
	movb	-1(%eax),%dl
	jne	1f
	subl	$2,%eax
	jmp	.Ldone_fb

	_ALIGN_TEXT
1:	cmpb	%dl,%cl		/* 4th byte == ch? */
	jne	.Lloop_fb
	decl	%eax
	jmp	.Ldone_fb

.Lbyte_fb:
	testl	%esi,%esi
	je	.Lzero_fb
.Lbyte_loop_fb:
	cmpb	(%eax),%cl
	je	.Ldone_fb
	incl	%eax
	decl	%esi
	jnz	.Lbyte_loop_fb

.Lzero_fb:
	xorl	%eax,%eax

.Ldone_fb:
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlFindByteInMemory)
