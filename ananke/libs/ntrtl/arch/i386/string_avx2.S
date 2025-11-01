/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 AVX2-optimized string operations
 *
 * Requires: AVX2 (Haswell and later, available in 32-bit mode)
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength_AVX2 - AVX2-optimized string length
 * 4(%esp): string pointer
 * Returns: length in bytes
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength_AVX2)
	movl	4(%esp),%eax		/* String pointer */
	movl	%eax,%edx		/* Save original */

	/* Align to 32 bytes */
	movl	%eax,%ecx
	andl	$31,%ecx
	jz	.Laligned_strlen_avx2

.Lhead_strlen_avx2:
	cmpb	$0,(%eax)
	je	.Ldone_strlen_avx2
	incl	%eax
	testl	$31,%eax
	jnz	.Lhead_strlen_avx2

.Laligned_strlen_avx2:
	vpxor	%ymm0,%ymm0,%ymm0

.Lloop_strlen_avx2:
	vmovdqa	(%eax),%ymm1
	vpcmpeqb %ymm0,%ymm1,%ymm2
	vpmovmskb %ymm2,%ecx

	testl	%ecx,%ecx
	jnz	.Lfound_strlen_avx2

	addl	$32,%eax
	jmp	.Lloop_strlen_avx2

.Lfound_strlen_avx2:
	bsf	%ecx,%ecx
	addl	%ecx,%eax

.Ldone_strlen_avx2:
	subl	%edx,%eax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlStringLength_AVX2)

/*
 * RtlFindChar_AVX2 - AVX2-optimized character search
 * 4(%esp): string pointer
 * 8(%esp): character to find
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar_AVX2)
	pushl	%edi
	movl	8(%esp),%edi		/* String pointer */
	movl	12(%esp),%eax		/* Character */

	/* Broadcast character */
	vmovd	%eax,%xmm0
	vpbroadcastb %xmm0,%ymm0

	vpxor	%ymm1,%ymm1,%ymm1

	/* Handle unaligned head */
	movl	%edi,%eax
	andl	$31,%eax
	jz	.Laligned_findchr_avx2

.Lhead_findchr_avx2:
	movzbl	(%edi),%ecx
	cmpb	8(%esp),%cl
	je	.Lfound_findchr_avx2
	testb	%cl,%cl
	jz	.Lnotfound_findchr_avx2
	incl	%edi
	testl	$31,%edi
	jnz	.Lhead_findchr_avx2

.Laligned_findchr_avx2:
.Lloop_findchr_avx2:
	vmovdqa	(%edi),%ymm2

	/* Check for target */
	vpcmpeqb %ymm0,%ymm2,%ymm3
	vpmovmskb %ymm3,%eax

	/* Check for null */
	vpcmpeqb %ymm1,%ymm2,%ymm3
	vpmovmskb %ymm3,%ecx

	testl	%eax,%eax
	jnz	.Lfound_vec_findchr_avx2

	testl	%ecx,%ecx
	jnz	.Lnotfound_findchr_avx2

	addl	$32,%edi
	jmp	.Lloop_findchr_avx2

.Lfound_vec_findchr_avx2:
	bsf	%eax,%eax
	addl	%eax,%edi

.Lfound_findchr_avx2:
	movl	%edi,%eax
	vzeroupper
	popl	%edi
	ret

.Lnotfound_findchr_avx2:
	xorl	%eax,%eax
	vzeroupper
	popl	%edi
	ret
ANX_ASM_FUNCTION_END(RtlFindChar_AVX2)

/*
 * RtlFindLastChar_AVX2 - AVX2-optimized reverse search
 * 4(%esp): string pointer
 * 8(%esp): character to find
 * Returns: pointer to last occurrence or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar_AVX2)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%esi		/* String pointer */
	movl	16(%esp),%eax		/* Character */
	xorl	%edi,%edi		/* Last match = NULL */

	/* Broadcast character */
	vmovd	%eax,%xmm0
	vpbroadcastb %xmm0,%ymm0
	vpxor	%ymm1,%ymm1,%ymm1

	/* Find end */
	movl	%esi,%eax
	andl	$31,%eax
	jz	.Laligned_end_avx2

.Lhead_end_avx2:
	cmpb	$0,(%esi)
	je	.Lfound_end_avx2
	incl	%esi
	testl	$31,%esi
	jnz	.Lhead_end_avx2

.Laligned_end_avx2:
.Lloop_end_avx2:
	vmovdqa	(%esi),%ymm2
	vpcmpeqb %ymm1,%ymm2,%ymm3
	vpmovmskb %ymm3,%eax
	testl	%eax,%eax
	jnz	.Lfound_end_partial_avx2
	addl	$32,%esi
	jmp	.Lloop_end_avx2

.Lfound_end_partial_avx2:
	bsf	%eax,%eax
	addl	%eax,%esi

.Lfound_end_avx2:
	/* Scan for matches */
	movl	12(%esp),%eax

.Lscan_matches_avx2:
	cmpl	%esi,%eax
	jae	.Ldone_findlast_avx2

	vmovdqu	(%eax),%ymm2
	vpcmpeqb %ymm0,%ymm2,%ymm3
	vpmovmskb %ymm3,%ecx

	testl	%ecx,%ecx
	jz	.Lnext_block_findlast_avx2

	bsr	%ecx,%ecx
	leal	(%eax,%ecx),%edi

.Lnext_block_findlast_avx2:
	addl	$32,%eax
	jmp	.Lscan_matches_avx2

.Ldone_findlast_avx2:
	movl	%edi,%eax
	vzeroupper
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar_AVX2)

/*
 * RtlCompareChars_AVX2 - AVX2-optimized string comparison
 * 4(%esp): string1
 * 8(%esp): string2
 * 12(%esp): max length
 * Returns: difference of first differing chars
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars_AVX2)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi		/* string1 */
	movl	16(%esp),%esi		/* string2 */
	movl	20(%esp),%ecx		/* max length */

	testl	%ecx,%ecx
	jz	.Lequal_cmp_avx2

	cmpl	$32,%ecx
	jb	.Lbyte_cmp_avx2

	vpxor	%ymm2,%ymm2,%ymm2

.Lvec_cmp_avx2:
	vmovdqu	(%edi),%ymm0
	vmovdqu	(%esi),%ymm1

	/* Compare */
	vpcmpeqb %ymm0,%ymm1,%ymm3
	vpmovmskb %ymm3,%eax
	xorl	$0xFFFFFFFF,%eax

	/* Check for null */
	vpcmpeqb %ymm2,%ymm0,%ymm3
	vpmovmskb %ymm3,%edx
	vpcmpeqb %ymm2,%ymm1,%ymm3
	vpmovmskb %ymm3,%ebx
	orl	%edx,%ebx

	testl	%eax,%eax
	jnz	.Ldiff_vec_cmp_avx2
	testl	%ebx,%ebx
	jnz	.Lequal_cmp_avx2

	addl	$32,%edi
	addl	$32,%esi
	subl	$32,%ecx
	cmpl	$32,%ecx
	jae	.Lvec_cmp_avx2

.Lbyte_cmp_avx2:
	testl	%ecx,%ecx
	jz	.Lequal_cmp_avx2

.Lbyte_loop_cmp_avx2:
	movzbl	(%edi),%eax
	movzbl	(%esi),%edx
	subl	%edx,%eax
	jnz	.Ldone_cmp_avx2
	testb	%dl,%dl
	jz	.Lequal_cmp_avx2
	incl	%edi
	incl	%esi
	decl	%ecx
	jnz	.Lbyte_loop_cmp_avx2

.Lequal_cmp_avx2:
	xorl	%eax,%eax
	vzeroupper
	popl	%edi
	popl	%esi
	ret

.Ldiff_vec_cmp_avx2:
	bsf	%eax,%ecx
	movzbl	(%edi,%ecx),%eax
	movzbl	(%esi,%ecx),%edx
	subl	%edx,%eax

.Ldone_cmp_avx2:
	vzeroupper
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars_AVX2)
