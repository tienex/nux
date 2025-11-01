/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 optimized string operations
 *
 * Original implementations from NetBSD, adapted to NT naming conventions:
 * - RtlStringLength: Optimized strlen using word-at-a-time scanning
 * - RtlFindChar: Optimized strchr using word-at-a-time scanning
 * - RtlFindLastChar: Optimized strrchr finding last occurrence
 * - RtlCompareChars: Optimized strncmp with unrolled loop
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength - Calculate string length
 * %rdi: string pointer
 * Returns: length in bytes
 *
 * Based on NetBSD strlen.S - uses branch-free word scanning algorithm
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength)
	movabsq	$0x0101010101010101,%r8

	test	$7,%dil
	movq	%rdi,%rax		/* Buffer, %rdi unchanged */
	movabsq	$0x8080808080808080,%r9
	jnz	10f			/* Jump if misaligned */

	_ALIGN_TEXT
1:
	movq	(%rax),%rdx		/* get bytes to check */
2:
	addq	$8,%rax
	mov	%rdx,%rcx		/* save for later check */
	subq	%r8,%rdx		/* alg (3) above first */
	not	%rcx			/* Invert of data */
	andq	%r9,%rdx
	je	1b			/* jump if all 0x01-0x80 */

	/* Do check from alg (2) above - loops for 0x81..0xff bytes */
	andq	%rcx,%rdx
	je	1b

	/* Since we are LE, use bit scan for first 0x80 byte */
	sub	%rdi,%rax		/* length to next word */
	bsf	%rdx,%rdx		/* 7, 15, 23 ... 63 */
	shr	$3,%rdx			/* 0, 1, 2 ... 7 */
	lea	-8(%rax,%rdx),%rax
	ret

/* Misaligned, read aligned word and make low bytes non-zero */
	_ALIGN_TEXT
10:
	mov	%al,%cl
	mov	$1,%rsi
	and	$7,%cl			/* offset into word 1..7 */
	and	$~7,%al			/* start of word with buffer */
	shl	$3,%cl			/* bit count 8, 16 .. 56 */
	movq	(%rax),%rdx		/* first data in high bytes */
	shl	%cl,%rsi
	dec	%rsi
	or	%rsi,%rdx		/* low bytes now non-zero */
	jmp	2b
ANX_ASM_FUNCTION_END(RtlStringLength)

/*
 * RtlFindChar - Find character in string
 * %rdi: string pointer
 * %sil: character to find (low byte of %rsi)
 * Returns: pointer to character or NULL
 *
 * Based on NetBSD strchr.S - uses word-at-a-time scanning
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar)
	movabsq	$0x0101010101010101,%r8

	movzbq	%sil,%rdx	/* value to search for (c) */
	/* These imul are 'directpath' on athlons, so are fast */
	imul	$0x80,%r8,%r9	/* 0x8080808080808080 */
	imul	%r8,%rdx	/* (c) copied to all bytes */
	test	$7,%dil
	jnz	20f		/* jump if misaligned */

	_ALIGN_TEXT		/* one byte nop */
1:
	movq	(%rdi),%rax	/* bytes to check (x) */
2:
	addq	$8,%rdi
	mov	%rax,%r10
	mov	%rax,%r11	/* for 'char' check */
	not	%r10		/* invert of data (~x) */

	xorq	%rdx,%r11	/* convert 'char' test to one for NUL */
	subq	%r8,%rax	/* x - 0x10 */
	movq	%r10,%rsi	/* ~x */
	subq	%r8,%r11	/* (x ^ c) - 0x10 */
	andq	%r9,%rax	/* (x - 0x10) & 0x80 */
	xorq	%rdx,%rsi	/* c ^ ~x == ~(c ^ x) */
	andq	%r9,%r11	/* ((x ^ c) - 0x10) & 0x80 */
	andq	%r10,%rax	/* (x - 0x10) & 0x80 & ~x */
	jne	10f		/* jump if string ends */
	andq	%rsi,%r11	/* ((x ^ c) - 0x10) & 0x80 & ~(x ^ c) */
	je	1b		/* jump if no match */

	/* Found char, since LE can use bit scan */
	bsf	%r11,%r11	/* 7, 15, 23 ... 63 */
8:	shr	$3,%r11		/* 0, 1, 2 .. 7 */
	lea	-8(%r11,%rdi),%rax
	ret

/* End of string, check whether char is before NUL */
	_ALIGN_TEXT		/* adds three byte nop */
10:
	bsf	%rax,%rax	/* count to NUL */
	andq	%rsi,%r11	/* check for char in last 8 bytes */
	je	11f
	bsf	%r11,%r11	/* NUL and char - see which was first */
	cmp	%r11,%rax
	jae	8b		/* return 'found' if same - searching for NUL */
11:	xor	%eax,%eax	/* char not found */
	ret

/* Source misaligned: read aligned word and make low bytes invalid */
20:
	xor	%rcx,%rcx
	sub	%dil,%cl	/* Convert low address values 1..7 ... */
	sbb	%rsi,%rsi	/* carry was set, so %rsi now ~0u! */
	and	$7,%cl		/* ... to 7..1 */
	and	$~7,%dil	/* move address to start of word */
	shl	$3,%cl		/* now 56, 48 ... 16, 8 */
	movq	(%rdi),%rax	/* aligned word containing first data */
	xor	%rdx,%rsi	/* invert of search pattern (~c) */
	je	22f		/* searching for 0xff */
21:	shr	%cl,%rsi	/* ~c in low bytes */
	or	%rsi,%rax	/* set some bits making low bytes invalid */
	jmp	2b

/* We are searching for 0xff, so can't use ~pattern for invalid value */
22:
	mov	%r8,%r10	/* 0x01 pattern */
	lea	(%r8,%r8),%rsi	/* 0x02 - bits gets set (above) */
	not	%r10		/* now 0xfe */
	sar	%cl,%r10	/* top bytes 0xff */
	and	%r10,%rax	/* clear lsb from unwanted low bytes */
	jmp	21b
ANX_ASM_FUNCTION_END(RtlFindChar)

/*
 * RtlFindLastChar - Find last occurrence of character
 * %rdi: string pointer
 * %sil: character to find (low byte of %rsi)
 * Returns: pointer to last occurrence or NULL
 *
 * Based on NetBSD strrchr.S
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar)
	movzbq	%sil,%rcx

	/* zero return value */
	xorq	%rax,%rax

	/*
	 * Align to word boundary.
	 * Consider unrolling loop?
	 */
.Lalign:
	testb	$7,%dil
	je	.Lword_aligned
	movb	(%rdi),%dl
	cmpb	%cl,%dl
	cmoveq	%rdi,%rax
	incq	%rdi
	testb	%dl,%dl
	jne	.Lalign
	jmp	.Ldone

.Lword_aligned:
	/* copy char to all bytes in word */
	movb	%cl,%ch
	movq	%rcx,%rdx
	salq	$16,%rcx
	orq	%rdx,%rcx
	movq	%rcx,%rdx
	salq	$32,%rcx
	orq	%rdx,%rcx

	movabsq	$0x0101010101010101,%r8
	movabsq	$0x8080808080808080,%r9

	/* Check whether any byte in the word is equal to ch or 0. */
	_ALIGN_TEXT
.Lloop:
	movq	(%rdi),%rdx
	addq	$8,%rdi
	movq	%rdx,%rsi
	subq	%r8,%rdx
	xorq	%rcx,%rsi
	subq	%r8,%rsi
	orq	%rsi,%rdx
	testq	%r9,%rdx
	je	.Lloop

	/*
	 * In rare cases, the above loop may exit prematurely. We must
	 * return to the loop if none of the bytes in the word match
	 * ch or are equal to 0.
	 */

	movb	-8(%rdi),%dl
	cmpb	%cl,%dl		/* 1st byte == ch? */
	jne	1f
	leaq	-8(%rdi),%rax
1:	testb	%dl,%dl		/* 1st byte == 0? */
	je	.Ldone

	movb	-7(%rdi),%dl
	cmpb	%cl,%dl		/* 2nd byte == ch? */
	jne	1f
	leaq	-7(%rdi),%rax
1:	testb	%dl,%dl		/* 2nd byte == 0? */
	je	.Ldone

	movb	-6(%rdi),%dl
	cmpb	%cl,%dl		/* 3rd byte == ch? */
	jne	1f
	leaq	-6(%rdi),%rax
1:	testb	%dl,%dl		/* 3rd byte == 0? */
	je	.Ldone

	movb	-5(%rdi),%dl
	cmpb	%cl,%dl		/* 4th byte == ch? */
	jne	1f
	leaq	-5(%rdi),%rax
1:	testb	%dl,%dl		/* 4th byte == 0? */
	je	.Ldone

	movb	-4(%rdi),%dl
	cmpb	%cl,%dl		/* 5th byte == ch? */
	jne	1f
	leaq	-4(%rdi),%rax
1:	testb	%dl,%dl		/* 5th byte == 0? */
	je	.Ldone

	movb	-3(%rdi),%dl
	cmpb	%cl,%dl		/* 6th byte == ch? */
	jne	1f
	leaq	-3(%rdi),%rax
1:	testb	%dl,%dl		/* 6th byte == 0? */
	je	.Ldone

	movb	-2(%rdi),%dl
	cmpb	%cl,%dl		/* 7th byte == ch? */
	jne	1f
	leaq	-2(%rdi),%rax
1:	testb	%dl,%dl		/* 7th byte == 0? */
	je	.Ldone

	movb	-1(%rdi),%dl
	cmpb	%cl,%dl		/* 8th byte == ch? */
	jne	1f
	leaq	-1(%rdi),%rax
1:	testb	%dl,%dl		/* 8th byte == 0? */
	jne	.Lloop

.Ldone:
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar)

/*
 * RtlCompareChars - Compare strings up to N characters
 * %rdi: string1
 * %rsi: string2
 * %rdx: max length
 * Returns: difference of first differing chars, or 0 if equal
 *
 * Based on NetBSD strncmp.S - unrolled loop for performance
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars)
	testq	%rdx,%rdx
	jmp	L2			/* Jump into the loop! */

L1:	incq	%rdi
	incq	%rsi
	decq	%rdx
L2:	jz	L4			/* strings are equal */
	movb	(%rdi),%al
	testb	%al,%al
	jz	L3
	cmpb	%al,(%rsi)
	jne	L3

	incq	%rdi
	incq	%rsi
	decq	%rdx
	jz	L4
	movb	(%rdi),%al
	testb	%al,%al
	jz	L3
	cmpb	%al,(%rsi)
	jne	L3

	incq	%rdi
	incq	%rsi
	decq	%rdx
	jz	L4
	movb	(%rdi),%al
	testb	%al,%al
	jz	L3
	cmpb	%al,(%rsi)
	jne	L3

	incq	%rdi
	incq	%rsi
	decq	%rdx
	jz	L4
	movb	(%rdi),%al
	testb	%al,%al
	jz	L3
	cmpb	%al,(%rsi)
	jne	L3

	incq	%rdi
	incq	%rsi
	decq	%rdx
	jz	L4
	movb	(%rdi),%al
	testb	%al,%al
	jz	L3
	cmpb	%al,(%rsi)
	jne	L3

	incq	%rdi
	incq	%rsi
	decq	%rdx
	jz	L4
	movb	(%rdi),%al
	testb	%al,%al
	jz	L3
	cmpb	%al,(%rsi)
	jne	L3

	incq	%rdi
	incq	%rsi
	decq	%rdx
	jz	L4
	movb	(%rdi),%al
	testb	%al,%al
	jz	L3
	cmpb	%al,(%rsi)
	jne	L3

	incq	%rdi
	incq	%rsi
	decq	%rdx
	jz	L4
	movb	(%rdi),%al
	testb	%al,%al
	jz	L3
	cmpb	%al,(%rsi)
	je	L1

L3:	movzbl	(%rdi),%eax		/* unsigned comparison */
	movzbl	(%rsi),%ecx
	subl	%ecx,%eax
	ret
L4:	xorl	%eax,%eax
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars)
