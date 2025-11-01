/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 SSE2-optimized string operations
 *
 * Requires: SSE2 (Pentium 4 and later, available in 32-bit mode)
 *
 * SSE2 features:
 * - 128-bit SIMD operations (XMM registers)
 * - Integer operations on packed data
 * - Fast string scanning with pcmpeqb
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength_SSE2 - SSE2-optimized string length
 * 4(%esp): string pointer
 * Returns: length in bytes
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength_SSE2)
	movl	4(%esp),%eax		/* String pointer */
	movl	%eax,%edx		/* Save original */

	/* Align to 16 bytes */
	movl	%eax,%ecx
	andl	$15,%ecx
	jz	.Laligned_strlen_sse2

.Lhead_strlen_sse2:
	cmpb	$0,(%eax)
	je	.Ldone_strlen_sse2
	incl	%eax
	testl	$15,%eax
	jnz	.Lhead_strlen_sse2

.Laligned_strlen_sse2:
	pxor	%xmm0,%xmm0		/* Zero XMM0 */

.Lloop_strlen_sse2:
	movdqa	(%eax),%xmm1		/* Load 16 bytes aligned */
	pcmpeqb	%xmm0,%xmm1		/* Compare with zero */
	pmovmskb %xmm1,%ecx		/* Get mask */

	testl	%ecx,%ecx
	jnz	.Lfound_strlen_sse2

	addl	$16,%eax
	jmp	.Lloop_strlen_sse2

.Lfound_strlen_sse2:
	bsf	%ecx,%ecx		/* Find first zero bit */
	addl	%ecx,%eax

.Ldone_strlen_sse2:
	subl	%edx,%eax		/* Calculate length */
	ret
ANX_ASM_FUNCTION_END(RtlStringLength_SSE2)

/*
 * RtlFindChar_SSE2 - SSE2-optimized character search
 * 4(%esp): string pointer
 * 8(%esp): character to find
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar_SSE2)
	pushl	%edi
	movl	8(%esp),%edi		/* String pointer */
	movl	12(%esp),%eax		/* Character */

	/* Broadcast character to all bytes in XMM */
	movd	%eax,%xmm0
	punpcklbw %xmm0,%xmm0		/* Replicate byte */
	punpcklwd %xmm0,%xmm0
	pshufd	$0,%xmm0,%xmm0		/* Broadcast to all 16 bytes */

	pxor	%xmm1,%xmm1		/* Zero for null check */

	/* Handle unaligned head */
	movl	%edi,%eax
	andl	$15,%eax
	jz	.Laligned_findchr_sse2

.Lhead_findchr_sse2:
	movzbl	(%edi),%ecx
	cmpb	8(%esp),%cl
	je	.Lfound_findchr_sse2
	testb	%cl,%cl
	jz	.Lnotfound_findchr_sse2
	incl	%edi
	testl	$15,%edi
	jnz	.Lhead_findchr_sse2

.Laligned_findchr_sse2:
.Lloop_findchr_sse2:
	movdqa	(%edi),%xmm2

	/* Check for target */
	movdqa	%xmm2,%xmm3
	pcmpeqb	%xmm0,%xmm3
	pmovmskb %xmm3,%eax		/* Target matches */

	/* Check for null */
	pcmpeqb	%xmm1,%xmm2
	pmovmskb %xmm2,%ecx		/* Null matches */

	testl	%eax,%eax
	jnz	.Lfound_vec_findchr_sse2

	testl	%ecx,%ecx
	jnz	.Lnotfound_findchr_sse2

	addl	$16,%edi
	jmp	.Lloop_findchr_sse2

.Lfound_vec_findchr_sse2:
	bsf	%eax,%eax		/* Find first match */
	addl	%eax,%edi

.Lfound_findchr_sse2:
	movl	%edi,%eax
	popl	%edi
	ret

.Lnotfound_findchr_sse2:
	xorl	%eax,%eax
	popl	%edi
	ret
ANX_ASM_FUNCTION_END(RtlFindChar_SSE2)

/*
 * RtlFindLastChar_SSE2 - SSE2-optimized reverse search
 * 4(%esp): string pointer
 * 8(%esp): character to find
 * Returns: pointer to last occurrence or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar_SSE2)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%esi		/* String pointer */
	movl	16(%esp),%eax		/* Character */
	xorl	%edi,%edi		/* Last match = NULL */

	/* Broadcast character */
	movd	%eax,%xmm0
	punpcklbw %xmm0,%xmm0
	punpcklwd %xmm0,%xmm0
	pshufd	$0,%xmm0,%xmm0

	pxor	%xmm1,%xmm1

	/* Find string end */
	movl	%esi,%eax
	andl	$15,%eax
	jz	.Laligned_end_sse2

.Lhead_end_sse2:
	cmpb	$0,(%esi)
	je	.Lfound_end_sse2
	incl	%esi
	testl	$15,%esi
	jnz	.Lhead_end_sse2

.Laligned_end_sse2:
.Lloop_end_sse2:
	movdqa	(%esi),%xmm2
	pcmpeqb	%xmm1,%xmm2
	pmovmskb %xmm2,%eax
	testl	%eax,%eax
	jnz	.Lfound_end_partial_sse2
	addl	$16,%esi
	jmp	.Lloop_end_sse2

.Lfound_end_partial_sse2:
	bsf	%eax,%eax
	addl	%eax,%esi

.Lfound_end_sse2:
	/* Scan from start to end */
	movl	12(%esp),%eax

.Lscan_matches_sse2:
	cmpl	%esi,%eax
	jae	.Ldone_findlast_sse2

	movdqu	(%eax),%xmm2
	pcmpeqb	%xmm0,%xmm2
	pmovmskb %xmm2,%ecx

	testl	%ecx,%ecx
	jz	.Lnext_block_findlast_sse2

	/* Find last match in block */
	bsr	%ecx,%ecx
	leal	(%eax,%ecx),%edi

.Lnext_block_findlast_sse2:
	addl	$16,%eax
	jmp	.Lscan_matches_sse2

.Ldone_findlast_sse2:
	movl	%edi,%eax
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar_SSE2)

/*
 * RtlCompareChars_SSE2 - SSE2-optimized string comparison
 * 4(%esp): string1
 * 8(%esp): string2
 * 12(%esp): max length
 * Returns: difference of first differing chars
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars_SSE2)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi		/* string1 */
	movl	16(%esp),%esi		/* string2 */
	movl	20(%esp),%ecx		/* max length */

	testl	%ecx,%ecx
	jz	.Lequal_cmp_sse2

	cmpl	$16,%ecx
	jb	.Lbyte_cmp_sse2

	pxor	%xmm2,%xmm2

.Lvec_cmp_sse2:
	movdqu	(%edi),%xmm0
	movdqu	(%esi),%xmm1

	/* Compare for equality */
	movdqa	%xmm0,%xmm3
	pcmpeqb	%xmm1,%xmm3
	pmovmskb %xmm3,%eax
	xorl	$0xFFFF,%eax		/* Invert */

	/* Check for null */
	movdqa	%xmm0,%xmm3
	pcmpeqb	%xmm2,%xmm3
	pmovmskb %xmm3,%edx
	movdqa	%xmm1,%xmm3
	pcmpeqb	%xmm2,%xmm3
	pmovmskb %xmm3,%ebx
	orl	%edx,%ebx

	testl	%eax,%eax
	jnz	.Ldiff_vec_cmp_sse2
	testl	%ebx,%ebx
	jnz	.Lequal_cmp_sse2

	addl	$16,%edi
	addl	$16,%esi
	subl	$16,%ecx
	cmpl	$16,%ecx
	jae	.Lvec_cmp_sse2

.Lbyte_cmp_sse2:
	testl	%ecx,%ecx
	jz	.Lequal_cmp_sse2

.Lbyte_loop_cmp_sse2:
	movzbl	(%edi),%eax
	movzbl	(%esi),%edx
	subl	%edx,%eax
	jnz	.Ldone_cmp_sse2
	testb	%dl,%dl
	jz	.Lequal_cmp_sse2
	incl	%edi
	incl	%esi
	decl	%ecx
	jnz	.Lbyte_loop_cmp_sse2

.Lequal_cmp_sse2:
	xorl	%eax,%eax
	popl	%edi
	popl	%esi
	ret

.Ldiff_vec_cmp_sse2:
	bsf	%eax,%ecx
	movzbl	(%edi,%ecx),%eax
	movzbl	(%esi,%ecx),%edx
	subl	%edx,%eax

.Ldone_cmp_sse2:
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars_SSE2)
