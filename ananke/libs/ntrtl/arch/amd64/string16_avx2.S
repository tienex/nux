/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 AVX2-optimized 16-bit character string operations
 *
 * Requires: AVX2 (Haswell and later)
 *
 * AVX2 features for CHAR16:
 * - 256-bit operations process 16 characters at once
 * - vpcmpeqw for word comparison
 * - vpbroadcastw for character broadcast
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength16_AVX2 - AVX2-optimized 16-bit string length
 * %rdi: string pointer (CHAR16*)
 * Returns: length in characters (not bytes)
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength16_AVX2)
	movq	%rdi,%rax		/* Save original pointer */

	/* Align to 32 bytes */
	movq	%rdi,%rcx
	andq	$31,%rcx
	jz	.Laligned_strlen16_avx2

.Lhead_strlen16_avx2:
	cmpw	$0,(%rdi)
	je	.Ldone_strlen16_avx2
	addq	$2,%rdi
	testq	$31,%rdi
	jnz	.Lhead_strlen16_avx2

.Laligned_strlen16_avx2:
	vpxor	%ymm0,%ymm0,%ymm0

.Lloop_strlen16_avx2:
	vmovdqa	(%rdi),%ymm1
	vpcmpeqw %ymm0,%ymm1,%ymm2	/* Compare words */
	vpmovmskb %ymm2,%ecx

	testl	%ecx,%ecx
	jnz	.Lfound_strlen16_avx2

	addq	$32,%rdi
	jmp	.Lloop_strlen16_avx2

.Lfound_strlen16_avx2:
	tzcntl	%ecx,%ecx
	shrl	$1,%ecx			/* Convert to word offset */
	leaq	(%rdi,%rcx,2),%rdi

.Ldone_strlen16_avx2:
	subq	%rax,%rdi		/* Length in bytes */
	shrq	$1,%rdi			/* Convert to characters */
	movq	%rdi,%rax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlStringLength16_AVX2)

/*
 * RtlFindChar16_AVX2 - AVX2-optimized 16-bit character search
 * %rdi: string pointer (CHAR16*)
 * %rsi: character to find (low 16 bits)
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar16_AVX2)
	/* Broadcast character to all words */
	vmovd	%esi,%xmm0
	vpbroadcastw %xmm0,%ymm0

	vpxor	%ymm1,%ymm1,%ymm1

	/* Handle unaligned head */
	movq	%rdi,%rax
	andq	$31,%rax
	jz	.Laligned_findchr16_avx2

.Lhead_findchr16_avx2:
	movzwl	(%rdi),%ecx
	cmpw	%si,%cx
	je	.Lfound_findchr16_avx2
	testw	%cx,%cx
	jz	.Lnotfound_findchr16_avx2
	addq	$2,%rdi
	testq	$31,%rdi
	jnz	.Lhead_findchr16_avx2

.Laligned_findchr16_avx2:
.Lloop_findchr16_avx2:
	vmovdqa	(%rdi),%ymm2

	/* Check for target */
	vpcmpeqw %ymm0,%ymm2,%ymm3
	vpmovmskb %ymm3,%eax

	/* Check for null */
	vpcmpeqw %ymm1,%ymm2,%ymm3
	vpmovmskb %ymm3,%ecx

	testl	%eax,%eax
	jnz	.Lfound_vec_findchr16_avx2

	testl	%ecx,%ecx
	jnz	.Lnotfound_findchr16_avx2

	addq	$32,%rdi
	jmp	.Lloop_findchr16_avx2

.Lfound_vec_findchr16_avx2:
	tzcntl	%eax,%eax
	shrl	$1,%eax
	leaq	(%rdi,%rax,2),%rdi

.Lfound_findchr16_avx2:
	movq	%rdi,%rax
	vzeroupper
	ret

.Lnotfound_findchr16_avx2:
	xorq	%rax,%rax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlFindChar16_AVX2)

/*
 * RtlFindLastChar16_AVX2 - AVX2-optimized 16-bit reverse search
 * %rdi: string pointer (CHAR16*)
 * %rsi: character to find (low 16 bits)
 * Returns: pointer to last occurrence or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar16_AVX2)
	movq	%rdi,%r8
	xorq	%r9,%r9			/* Last match */

	/* Broadcast character */
	vmovd	%esi,%xmm0
	vpbroadcastw %xmm0,%ymm0
	vpxor	%ymm1,%ymm1,%ymm1

	/* Find end */
	movq	%rdi,%rax
	andq	$31,%rax
	jz	.Laligned_end16_avx2

.Lhead_end16_avx2:
	cmpw	$0,(%rdi)
	je	.Lfound_end16_avx2
	addq	$2,%rdi
	testq	$31,%rdi
	jnz	.Lhead_end16_avx2

.Laligned_end16_avx2:
.Lloop_end16_avx2:
	vmovdqa	(%rdi),%ymm2
	vpcmpeqw %ymm1,%ymm2,%ymm3
	vpmovmskb %ymm3,%eax
	testl	%eax,%eax
	jnz	.Lfound_end_partial16_avx2
	addq	$32,%rdi
	jmp	.Lloop_end16_avx2

.Lfound_end_partial16_avx2:
	tzcntl	%eax,%eax
	shrl	$1,%eax
	leaq	(%rdi,%rax,2),%rdi

.Lfound_end16_avx2:
	/* Scan for matches */
	movq	%r8,%rax

.Lscan_matches16_avx2:
	cmpq	%rdi,%rax
	jae	.Ldone_findlast16_avx2

	vmovdqu	(%rax),%ymm2
	vpcmpeqw %ymm0,%ymm2,%ymm3
	vpmovmskb %ymm3,%ecx

	testl	%ecx,%ecx
	jz	.Lnext_block_findlast16_avx2

	/* Find last match in block */
	bsrl	%ecx,%ecx
	shrl	$1,%ecx
	leaq	(%rax,%rcx,2),%r9

.Lnext_block_findlast16_avx2:
	addq	$32,%rax
	jmp	.Lscan_matches16_avx2

.Ldone_findlast16_avx2:
	movq	%r9,%rax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar16_AVX2)

/*
 * RtlCompareChars16_AVX2 - AVX2-optimized 16-bit string comparison
 * %rdi: string1 (CHAR16*)
 * %rsi: string2 (CHAR16*)
 * %rdx: max length (in characters)
 * Returns: difference of first differing chars
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars16_AVX2)
	testq	%rdx,%rdx
	jz	.Lequal_cmp16_avx2

	cmpq	$16,%rdx
	jb	.Lword_cmp16_avx2

	vpxor	%ymm2,%ymm2,%ymm2

.Lvec_cmp16_avx2:
	vmovdqu	(%rdi),%ymm0
	vmovdqu	(%rsi),%ymm1

	/* Compare */
	vpcmpeqw %ymm0,%ymm1,%ymm3
	vpmovmskb %ymm3,%eax
	xorl	$0xFFFFFFFF,%eax

	/* Check for null */
	vpcmpeqw %ymm2,%ymm0,%ymm3
	vpmovmskb %ymm3,%ecx
	vpcmpeqw %ymm2,%ymm1,%ymm3
	vpmovmskb %ymm3,%r8d
	orl	%ecx,%r8d

	testl	%eax,%eax
	jnz	.Ldiff_vec_cmp16_avx2
	testl	%r8d,%r8d
	jnz	.Lequal_cmp16_avx2

	addq	$32,%rdi
	addq	$32,%rsi
	subq	$16,%rdx
	cmpq	$16,%rdx
	jae	.Lvec_cmp16_avx2

.Lword_cmp16_avx2:
	testq	%rdx,%rdx
	jz	.Lequal_cmp16_avx2

.Lword_loop_cmp16_avx2:
	movzwl	(%rdi),%eax
	movzwl	(%rsi),%ecx
	subl	%ecx,%eax
	jnz	.Ldone_cmp16_avx2
	testw	%cx,%cx
	jz	.Lequal_cmp16_avx2
	addq	$2,%rdi
	addq	$2,%rsi
	decq	%rdx
	jnz	.Lword_loop_cmp16_avx2

.Lequal_cmp16_avx2:
	xorl	%eax,%eax
	vzeroupper
	ret

.Ldiff_vec_cmp16_avx2:
	tzcntl	%eax,%ecx
	shrl	$1,%ecx
	movzwl	(%rdi,%rcx,2),%eax
	movzwl	(%rsi,%rcx,2),%edx
	subl	%edx,%eax

.Ldone_cmp16_avx2:
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars16_AVX2)

/*
 * RtlCopyString16_AVX2 - AVX2-optimized 16-bit string copy
 * %rdi: destination (CHAR16*)
 * %rsi: source (CHAR16*)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyString16_AVX2)
	movq	%rdi,%rax		/* Save destination */
	vpxor	%ymm0,%ymm0,%ymm0

	/* Check alignment */
	movq	%rdi,%rcx
	orq	%rsi,%rcx
	andq	$31,%rcx
	jnz	.Lword_copy16_avx2

.Laligned_copy16_avx2:
	vmovdqa	(%rsi),%ymm1
	vpcmpeqw %ymm0,%ymm1,%ymm2
	vpmovmskb %ymm2,%ecx
	testl	%ecx,%ecx
	jnz	.Lpartial_copy16_avx2

	vmovdqa	%ymm1,(%rdi)
	addq	$32,%rdi
	addq	$32,%rsi
	jmp	.Laligned_copy16_avx2

.Lpartial_copy16_avx2:
	/* Copy remaining words */
	tzcntl	%ecx,%ecx
	shrl	$1,%ecx
	incl	%ecx			/* Include null */
	rep	movsw
	vzeroupper
	ret

.Lword_copy16_avx2:
	movzwl	(%rsi),%ecx
	movw	%cx,(%rdi)
	testw	%cx,%cx
	jz	.Ldone_copy16_avx2
	addq	$2,%rdi
	addq	$2,%rsi
	jmp	.Lword_copy16_avx2

.Ldone_copy16_avx2:
	ret
ANX_ASM_FUNCTION_END(RtlCopyString16_AVX2)
