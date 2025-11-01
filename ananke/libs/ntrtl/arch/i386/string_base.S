/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 optimized string operations
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
 * 4(%esp): string pointer
 * Returns: length in bytes
 *
 * Based on NetBSD strlen.S - uses branch-free word scanning algorithm
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength)
	movl	4(%esp),%eax

.Lalign:
	/* Consider unrolling loop? */
	testb	$3,%al
	je	.Lword_aligned
	cmpb	$0,(%eax)
	je	.Ldone
	incl	%eax
	jmp	.Lalign

	_ALIGN_TEXT
.Lword_aligned:
.Lloop:
	movl	(%eax),%ecx
	addl	$4,%eax
	leal	-0x01010101(%ecx),%edx
	testl	$0x80808080,%edx
	je	.Lloop

	/*
	 * In rare cases, the above loop may exit prematurely. We must
	 * return to the loop if none of the bytes in the word equal 0.
	 */

	testb	%cl,%cl		/* 1st byte == 0? */
	jne	1f
	subl	$4,%eax
	jmp	.Ldone

1:	testb	%ch,%ch		/* 2nd byte == 0? */
	jne	1f
	subl	$3,%eax
	jmp	.Ldone

1:	shrl	$16,%ecx
	testb	%cl,%cl		/* 3rd byte == 0? */
	jne	1f
	subl	$2,%eax
	jmp	.Ldone

1:	testb	%ch,%ch		/* 4th byte == 0? */
	jne	.Lloop
	decl	%eax

.Ldone:
	subl	4(%esp),%eax
	ret
ANX_ASM_FUNCTION_END(RtlStringLength)

/*
 * RtlFindChar - Find character in string
 * 12(%esp): string pointer (after push)
 * 16(%esp): character to find (after push)
 * Returns: pointer to character or NULL
 *
 * Based on NetBSD strchr.S - uses word-at-a-time scanning
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar)
	pushl	%esi
	pushl	%ebx
	movl	12(%esp),%eax
	movzbl	16(%esp),%ecx

	/*
	 * Align to word boundary.
	 * Consider unrolling loop?
	 */
.Lalign_fc:
	testb	$3,%al
	je	.Lword_aligned_fc
	movb	(%eax),%bl
	cmpb	%cl,%bl
	je	.Ldone_fc
	testb	%bl,%bl
	je	.Lzero_fc
	incl	%eax
	jmp	.Lalign_fc

.Lword_aligned_fc:
	/* copy char to all bytes in word */
	movb	%cl,%ch
	movl	%ecx,%edx
	sall	$16,%ecx
	orl	%edx,%ecx

	/* Check whether any byte in the word is equal to ch or 0. */
	_ALIGN_TEXT
.Lloop_fc:
	movl	(%eax),%ebx
	addl	$4,%eax
	movl	%ebx,%esi
	leal	-0x01010101(%ebx),%edx
	xorl	%ecx,%esi
	subl	$0x01010101,%esi
	orl	%esi,%edx
	testl	$0x80808080,%edx
	je	.Lloop_fc

	/*
	 * In rare cases, the above loop may exit prematurely. We must
	 * return to the loop if none of the bytes in the word match
	 * ch or are equal to 0.
	 */

	_ALIGN_TEXT
	cmpb	%cl,%bl		/* 1st byte == ch? */
	jne	1f
	subl	$4,%eax
	jmp	.Ldone_fc
1:	testb	%bl,%bl		/* 1st byte == 0? */
	je	.Lzero_fc

	cmpb	%cl,%bh		/* 2nd byte == ch? */
	jne	1f
	subl	$3,%eax
	jmp	.Ldone_fc
1:	testb	%bh,%bh		/* 2nd byte == 0? */
	je	.Lzero_fc

	shrl	$16,%ebx
	cmpb	%cl,%bl		/* 3rd byte == ch? */
	jne	1f
	subl	$2,%eax
	jmp	.Ldone_fc
1:	testb	%bl,%bl		/* 3rd byte == 0? */
	je	.Lzero_fc

	cmpb	%cl,%bh		/* 4th byte == ch? */
	jne	1f
	decl	%eax
	jmp	.Ldone_fc
1:	testb	%bh,%bh		/* 4th byte == 0? */
	jne	.Lloop_fc

.Lzero_fc:
	/* If a ch wasn't found, return 0. */
	xorl	%eax,%eax

.Ldone_fc:
	popl	%ebx
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlFindChar)

/*
 * RtlFindLastChar - Find last occurrence of character
 * 16(%esp): string pointer (after push)
 * 20(%esp): character to find (after push)
 * Returns: pointer to last occurrence or NULL
 *
 * Based on NetBSD strrchr.S
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar)
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	movl	16(%esp),%edx
	movzbl	20(%esp),%ecx

	/* zero return value */
	xorl	%eax,%eax

	/*
	 * Align to word boundary.
	 * Consider unrolling loop?
	 */
.Lalign_lc:
	testb	$3,%dl
	je	.Lword_aligned_lc
	movb	(%edx),%bl
	cmpb	%cl,%bl
	jne	1f
	movl	%edx,%eax
1:	testb	%bl,%bl
	je	.Ldone_lc
	incl	%edx
	jmp	.Lalign_lc

.Lword_aligned_lc:
	/* copy char to all bytes in word */
	movb	%cl,%ch
	movl	%ecx,%edi
	sall	$16,%ecx
	orl	%edi,%ecx

	/* Check whether any byte in the word is equal to ch or 0.  */
	_ALIGN_TEXT
.Lloop_lc:
	movl	(%edx),%ebx
	addl	$4,%edx
	movl	%ebx,%esi
	leal	-0x01010101(%ebx),%edi
	xorl	%ecx,%esi
	subl	$0x01010101,%esi
	orl	%esi,%edi
	testl	$0x80808080,%edi
	je	.Lloop_lc

	/*
	 * In rare cases, the above loop may exit prematurely. We must
	 * return to the loop if none of the bytes in the word match
	 * ch or are equal to 0.
	 */

	_ALIGN_TEXT
	cmpb	%cl,%bl		/* 1st byte == ch? */
	jne	1f
	leal	-4(%edx),%eax
1:	testb	%bl,%bl		/* 1st byte == 0? */
	je	.Ldone_lc

	cmpb	%cl,%bh		/* 2nd byte == ch? */
	jne	1f
	leal	-3(%edx),%eax
1:	testb	%bh,%bh		/* 2nd byte == 0? */
	je	.Ldone_lc

	shrl	$16,%ebx
	cmpb	%cl,%bl		/* 3rd byte == ch? */
	jne	1f
	leal	-2(%edx),%eax
1:	testb	%bl,%bl		/* 3rd byte == 0? */
	je	.Ldone_lc

	cmpb	%cl,%bh		/* 4th byte == ch? */
	jne	1f
	leal	-1(%edx),%eax
1:	testb	%bh,%bh		/* 4th byte == 0? */
	jne	.Lloop_lc

.Ldone_lc:
	popl	%ebx
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar)

/*
 * RtlCompareChars - Compare strings up to N characters
 * 8(%esp): string1 (after push)
 * 12(%esp): string2 (after push)
 * 16(%esp): max length (after push)
 * Returns: difference of first differing chars, or 0 if equal
 *
 * Based on NetBSD strncmp.S - unrolled loop for performance
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars)
	pushl	%ebx
	movl	8(%esp),%eax
	movl	12(%esp),%ecx
	movl	16(%esp),%edx
	testl	%edx,%edx
	jmp	L2			/* Jump into the loop! */

	_ALIGN_TEXT,0x90
L1:	incl	%eax
	incl	%ecx
	decl	%edx
L2:	jz	L4			/* strings are equal */
	movb	(%eax),%bl
	testb	%bl,%bl
	jz	L3
	cmpb	%bl,(%ecx)
	jne	L3

	incl	%eax
	incl	%ecx
	decl	%edx
	jz	L4
	movb	(%eax),%bl
	testb	%bl,%bl
	jz	L3
	cmpb	%bl,(%ecx)
	jne	L3

	incl	%eax
	incl	%ecx
	decl	%edx
	jz	L4
	movb	(%eax),%bl
	testb	%bl,%bl
	jz	L3
	cmpb	%bl,(%ecx)
	jne	L3

	incl	%eax
	incl	%ecx
	decl	%edx
	jz	L4
	movb	(%eax),%bl
	testb	%bl,%bl
	jz	L3
	cmpb	%bl,(%ecx)
	jne	L3

	incl	%eax
	incl	%ecx
	decl	%edx
	jz	L4
	movb	(%eax),%bl
	testb	%bl,%bl
	jz	L3
	cmpb	%bl,(%ecx)
	jne	L3

	incl	%eax
	incl	%ecx
	decl	%edx
	jz	L4
	movb	(%eax),%bl
	testb	%bl,%bl
	jz	L3
	cmpb	%bl,(%ecx)
	jne	L3

	incl	%eax
	incl	%ecx
	decl	%edx
	jz	L4
	movb	(%eax),%bl
	testb	%bl,%bl
	jz	L3
	cmpb	%bl,(%ecx)
	jne	L3

	incl	%eax
	incl	%ecx
	decl	%edx
	jz	L4
	movb	(%eax),%bl
	testb	%bl,%bl
	jz	L3
	cmpb	%bl,(%ecx)
	je	L1

	_ALIGN_TEXT,0x90
L3:	movzbl	(%eax),%eax		/* unsigned comparison */
	movzbl	(%ecx),%ecx
	subl	%ecx,%eax
	popl	%ebx
	ret
	_ALIGN_TEXT,0x90
L4:	xorl	%eax,%eax
	popl	%ebx
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars)
