/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 AVX2-optimized string operations
 *
 * Requires: AVX2 (Haswell and later)
 *
 * AVX2 features:
 * - 256-bit SIMD operations (YMM registers)
 * - Integer operations on 256-bit vectors
 * - Broadcast instructions
 * - Gather operations
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength_AVX2 - AVX2-optimized string length
 * %rdi: string pointer
 * Returns: length in bytes
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength_AVX2)
	movq	%rdi,%rax		/* Save original pointer */

	/* Align to 32 bytes for optimal performance */
	movq	%rdi,%rcx
	andq	$31,%rcx
	jz	.Laligned_strlen_avx2

	/* Handle unaligned head */
.Lhead_strlen_avx2:
	cmpb	$0,(%rdi)
	je	.Ldone_strlen_avx2
	incq	%rdi
	testq	$31,%rdi
	jnz	.Lhead_strlen_avx2

.Laligned_strlen_avx2:
	vpxor	%ymm0,%ymm0,%ymm0	/* Zero vector */

.Lloop_strlen_avx2:
	vmovdqa	(%rdi),%ymm1		/* Load 32 bytes aligned */
	vpcmpeqb %ymm0,%ymm1,%ymm2	/* Compare with zero */
	vpmovmskb %ymm2,%ecx		/* Get mask of matches */

	testl	%ecx,%ecx
	jnz	.Lfound_strlen_avx2

	addq	$32,%rdi
	jmp	.Lloop_strlen_avx2

.Lfound_strlen_avx2:
	tzcntl	%ecx,%ecx		/* Find position of null */
	addq	%rcx,%rdi

.Ldone_strlen_avx2:
	subq	%rax,%rdi		/* Calculate length */
	movq	%rdi,%rax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlStringLength_AVX2)

/*
 * RtlFindChar_AVX2 - AVX2-optimized character search
 * %rdi: string pointer
 * %rsi: character to find
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar_AVX2)
	/* Broadcast character to all bytes in YMM */
	vmovd	%esi,%xmm0
	vpbroadcastb %xmm0,%ymm0

	vpxor	%ymm1,%ymm1,%ymm1	/* Zero for null check */

	/* Handle unaligned head */
	movq	%rdi,%rax
	andq	$31,%rax
	jz	.Laligned_findchr_avx2

.Lhead_findchr_avx2:
	movzbl	(%rdi),%ecx
	cmpb	%sil,%cl
	je	.Lfound_findchr_avx2
	testb	%cl,%cl
	jz	.Lnotfound_findchr_avx2
	incq	%rdi
	testq	$31,%rdi
	jnz	.Lhead_findchr_avx2

.Laligned_findchr_avx2:
.Lloop_findchr_avx2:
	vmovdqa	(%rdi),%ymm2

	/* Check for target character */
	vpcmpeqb %ymm0,%ymm2,%ymm3
	vpmovmskb %ymm3,%eax		/* Matches for target */

	/* Check for null */
	vpcmpeqb %ymm1,%ymm2,%ymm3
	vpmovmskb %ymm3,%ecx		/* Matches for null */

	testl	%eax,%eax
	jnz	.Lfound_vec_findchr_avx2

	testl	%ecx,%ecx
	jnz	.Lnotfound_findchr_avx2

	addq	$32,%rdi
	jmp	.Lloop_findchr_avx2

.Lfound_vec_findchr_avx2:
	tzcntl	%eax,%eax		/* Find first match */
	addq	%rax,%rdi

.Lfound_findchr_avx2:
	movq	%rdi,%rax
	vzeroupper
	ret

.Lnotfound_findchr_avx2:
	xorq	%rax,%rax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlFindChar_AVX2)

/*
 * RtlFindLastChar_AVX2 - AVX2-optimized reverse character search
 * %rdi: string pointer
 * %rsi: character to find
 * Returns: pointer to last occurrence or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar_AVX2)
	movq	%rdi,%r8		/* Save start */
	xorq	%r9,%r9			/* Last match = NULL */

	/* Broadcast character */
	vmovd	%esi,%xmm0
	vpbroadcastb %xmm0,%ymm0
	vpxor	%ymm1,%ymm1,%ymm1

	/* Find string end first */
	movq	%rdi,%rax
	andq	$31,%rax
	jz	.Laligned_end_avx2

.Lhead_end_avx2:
	cmpb	$0,(%rdi)
	je	.Lfound_end_avx2
	incq	%rdi
	testq	$31,%rdi
	jnz	.Lhead_end_avx2

.Laligned_end_avx2:
.Lloop_end_avx2:
	vmovdqa	(%rdi),%ymm2
	vpcmpeqb %ymm1,%ymm2,%ymm3
	vpmovmskb %ymm3,%eax
	testl	%eax,%eax
	jnz	.Lfound_end_partial_avx2
	addq	$32,%rdi
	jmp	.Lloop_end_avx2

.Lfound_end_partial_avx2:
	tzcntl	%eax,%eax
	addq	%rax,%rdi

.Lfound_end_avx2:
	/* Now scan from start to end looking for matches */
	movq	%r8,%rax

.Lscan_matches_avx2:
	cmpq	%rdi,%rax
	jae	.Ldone_findlast_avx2

	vmovdqu	(%rax),%ymm2
	vpcmpeqb %ymm0,%ymm2,%ymm3
	vpmovmskb %ymm3,%ecx

	testl	%ecx,%ecx
	jz	.Lnext_block_findlast_avx2

	/* Find last match in this block */
	bsrl	%ecx,%ecx
	leaq	(%rax,%rcx),%r9		/* Update last match */

.Lnext_block_findlast_avx2:
	addq	$32,%rax
	jmp	.Lscan_matches_avx2

.Ldone_findlast_avx2:
	movq	%r9,%rax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar_AVX2)

/*
 * RtlCompareChars_AVX2 - AVX2-optimized string comparison
 * %rdi: string1
 * %rsi: string2
 * %rdx: max length
 * Returns: difference of first differing chars
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars_AVX2)
	testq	%rdx,%rdx
	jz	.Lequal_cmp_avx2

	cmpq	$32,%rdx
	jb	.Lbyte_cmp_avx2

	vpxor	%ymm2,%ymm2,%ymm2	/* Zero for null check */

.Lvec_cmp_avx2:
	vmovdqu	(%rdi),%ymm0
	vmovdqu	(%rsi),%ymm1

	/* Compare for equality */
	vpcmpeqb %ymm0,%ymm1,%ymm3
	vpmovmskb %ymm3,%eax
	xorl	$0xFFFFFFFF,%eax	/* Invert: 1 = different */

	/* Check for null in either string */
	vpcmpeqb %ymm2,%ymm0,%ymm3
	vpmovmskb %ymm3,%ecx
	vpcmpeqb %ymm2,%ymm1,%ymm3
	vpmovmskb %ymm3,%r8d
	orl	%ecx,%r8d

	/* Handle differences or nulls */
	testl	%eax,%eax
	jnz	.Ldiff_vec_cmp_avx2
	testl	%r8d,%r8d
	jnz	.Lequal_cmp_avx2

	addq	$32,%rdi
	addq	$32,%rsi
	subq	$32,%rdx
	cmpq	$32,%rdx
	jae	.Lvec_cmp_avx2

.Lbyte_cmp_avx2:
	testq	%rdx,%rdx
	jz	.Lequal_cmp_avx2

.Lbyte_loop_cmp_avx2:
	movzbl	(%rdi),%eax
	movzbl	(%rsi),%ecx
	subl	%ecx,%eax
	jnz	.Ldone_cmp_avx2
	testb	%cl,%cl
	jz	.Lequal_cmp_avx2
	incq	%rdi
	incq	%rsi
	decq	%rdx
	jnz	.Lbyte_loop_cmp_avx2

.Lequal_cmp_avx2:
	xorl	%eax,%eax
	vzeroupper
	ret

.Ldiff_vec_cmp_avx2:
	tzcntl	%eax,%ecx
	movzbl	(%rdi,%rcx),%eax
	movzbl	(%rsi,%rcx),%edx
	subl	%edx,%eax

.Ldone_cmp_avx2:
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars_AVX2)
