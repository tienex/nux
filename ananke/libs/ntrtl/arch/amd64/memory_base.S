/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 optimized memory operations
 *
 * Original implementations from NetBSD, adapted to NT naming conventions:
 * - RtlCopyMemory: Copy memory (non-overlapping buffers)
 * - RtlMoveMemory: Move memory (may overlap)
 * - RtlCompareMemory: Compare memory regions
 * - RtlFillMemory: Fill memory with byte value
 * - RtlFindByteInMemory: Find byte in memory region
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory - Copy memory (non-overlapping buffers)
 * %rdi: destination
 * %rsi: source
 * %rdx: length
 * Returns: destination pointer
 */
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
	jb	10f

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

/*
 * RtlMoveMemory - Move memory (may overlap)
 * %rdi: destination
 * %rsi: source
 * %rdx: length
 * Returns: destination pointer
 */
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

/*
 * RtlCompareMemory - Compare memory regions
 * %rdi: buffer1
 * %rsi: buffer2
 * %rdx: length
 * Returns: 0 if equal, <0 if buf1<buf2, >0 if buf1>buf2
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareMemory)
	movq	%rdx,%rcx		/* compare by longs */
	shrq	$3,%rcx
	repe
	cmpsq
	jne	L5			/* do we match so far? */

	movq	%rdx,%rcx		/* compare remainder by bytes */
	andq	$7,%rcx
	repe
	cmpsb
	jne	L6			/* do we match? */

	xorl	%eax,%eax		/* we match, return zero	*/
	ret

L5:	movl	$8,%ecx			/* We know that one of the next	*/
	subq	%rcx,%rdi		/* eight pairs of bytes do not	*/
	subq	%rcx,%rsi		/* match.			*/
	repe
	cmpsb
L6:	xorl	%eax,%eax		/* Perform unsigned comparison	*/
	movb	-1(%rdi),%al
	xorl	%edx,%edx
	movb	-1(%rsi),%dl
	subl    %edx,%eax
	ret
ANX_ASM_FUNCTION_END(RtlCompareMemory)

/*
 * RtlFillMemory - Fill memory with byte value
 * %rdi: buffer
 * %rsi: byte value
 * %rdx: length
 * Returns: buffer pointer
 */
#ifndef _KERNEL
/* bzero, %rdi is buffer, %rsi length */
ANX_ASM_FUNCTION_BEGIN(bzero)
	mov	%rsi,%rdx		/* length */
	xor	%eax,%eax		/* value to write */
	jmp	1f
#endif

ANX_ASM_FUNCTION_BEGIN(RtlFillMemory)
	movzbq	%sil,%rax		/* byte value to fill */
	mov	%rdx,%rsi		/* copy of length */
	mov	$0x0101010101010101,%r9
	imul	%r9,%rax		/* fill value in all bytes */

1:
	mov	%rdi,%r9		/* Need to return buffer address */
	or	%edi,%edx		/* address | length */
	mov	%rsi,%rcx
	cmp	$7,%rsi
	jbe	10f			/* jump if short fill */
	test	$7,%dl			/* check for misaligned fill */
	jnz	20f			/* jump if misaligned */

/* Target aligned and length multiple of 8 */
2:
	shr	$3,%rcx
	rep	stosq
	mov	%r9,%rax
	ret

/*
 * Short transfer, any faffing here will generate mispredicted branches.
 * So we keep it simple.
 */
10:	rep	stosb
	mov	%r9,%rax
	ret

/*
 * Buffer or length misaligned.
 * Write pattern to first and last word of buffer, then fill middle.
 * (This writes to some bytes more than once - possibly three times!.)
 */
20:
	mov	%rax,(%rdi)
	movzbq	%dil,%rdx		/* low address for alignment */
	mov	%rax,-8(%rcx,%rdi)
	and	$7,%dl			/* offset in word */
	sub	%rdx,%rcx		/* adjust length ... */
	add	%rdx,%rdi		/* ... and target */
	jmp	2b
ANX_ASM_FUNCTION_END(RtlFillMemory)

#ifndef _KERNEL
ANX_ASM_FUNCTION_END(bzero)
#endif

/*
 * RtlFindByteInMemory - Find byte in memory region
 * %rdi: buffer
 * %rsi: byte value to find
 * %rdx: length
 * Returns: pointer to byte or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindByteInMemory)
	movabsq	$0x0101010101010101,%r8
	lea	(%rdi,%rdx),%r10	/* limit of buffer to scan */
	movzbq	%sil,%rsi	/* mask high bits! */

	/* 'directpath' imuls can execute 3 at a time ... (amd) */
	imul	%r8,%rsi	/* search byte replicated in word */
	imul	$0x80,%r8,%r9	/* 0x8080808080808080 */
	test	$7,%dil
	jnz	20f		/* jump if misaligned */
	jmp	1f		/* jump to avoid 4 nops (13 bytes) in gap */

	_ALIGN_TEXT		/* entire loop now in 32 aligned bytes */
1:
	cmpq	%r10,%rdi	/* end of buffer ? */
	jae	30f		/* jump if so */

	movq	(%rdi),%rax	/* value to check */
	addq	$8,%rdi
	xorq	%rsi,%rax	/* now looking for zeros */
2:
	mov	%rax,%rcx
	subq	%r8,%rax	/* x - 0x01 */
	not	%rcx
	andq	%r9,%rax	/* (x - 0x01) & 0x80 */
	andq	%rcx,%rax	/* ((x - 0x01) & 0x80) & ~x */
	je	1b		/* jump if not found */

/* Found byte in word, get its address */
	bsf	%rax,%rax
	shr	$3,%eax
	lea	-8(%rax,%rdi),%rax
	cmpq	%r10,%rax	/* need to check not beyond buffer */
	jae	30f
	rep
	ret			/* amd - no ret after jmp */

/* Input misaligned, read aligned and make low bytes invalid */
20:
	mov	%dil,%cl	/* misalignment amount 1..7 (+high bits )*/
	and	$~7,%dil	/* %rdi now start of word */
	test	%rdx,%rdx	/* zero length, don't read */
	jz	30f

	neg	%cl		/* 7..1 (+high bits) */
	mov	(%rdi),%rax	/* word containing first byte */
	addq	$8,%rdi
	and	$7,%cl		/* 7..1 */

	mov	%r8,%r11	/* any value with bits in each byte */
	shl	$3,%cl		/* 56..8 */
	xorq	%rsi,%rax	/* now looking for zeros */

	/* Set low bytes non-zero */
	shr	%cl,%r11	/* non-zero in unwanted bytes */
	or	%r11,%rax	/* low bytes now set */
	jmp	2b

/* Not found */
30:	xorq	%rax,%rax
	ret
ANX_ASM_FUNCTION_END(RtlFindByteInMemory)
