/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * x32 AVX2-optimized 16-bit character string operations
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
 * %edi: string pointer (CHAR16*, 32-bit pointer)
 * Returns: length in characters (not bytes)
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength16_AVX2)
	mov	%edi,%eax		/* Save original pointer */

	/* Align to 32 bytes */
	mov	%edi,%ecx
	and	$31,%ecx
	jz	.Laligned_strlen16_avx2

.Lhead_strlen16_avx2:
	cmpw	$0,(%rdi)
	je	.Ldone_strlen16_avx2
	add	$2,%edi
	test	$31,%edi
	jnz	.Lhead_strlen16_avx2

.Laligned_strlen16_avx2:
	vpxor	%ymm0,%ymm0,%ymm0

.Lloop_strlen16_avx2:
	vmovdqa	(%rdi),%ymm1
	vpcmpeqw %ymm0,%ymm1,%ymm2	/* Compare words */
	vpmovmskb %ymm2,%ecx

	testl	%ecx,%ecx
	jnz	.Lfound_strlen16_avx2

	add	$32,%edi
	jmp	.Lloop_strlen16_avx2

.Lfound_strlen16_avx2:
	tzcntl	%ecx,%ecx
	shrl	$1,%ecx			/* Convert to word offset */
	lea	(%rdi,%rcx,2),%edi

.Ldone_strlen16_avx2:
	sub	%eax,%edi		/* Length in bytes */
	shr	$1,%edi			/* Convert to characters */
	mov	%edi,%eax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlStringLength16_AVX2)

/*
 * RtlFindChar16_AVX2 - AVX2-optimized 16-bit character search
 * %edi: string pointer (CHAR16*, 32-bit pointer)
 * %esi: character to find (low 16 bits)
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar16_AVX2)
	/* Broadcast character to all words */
	vmovd	%esi,%xmm0
	vpbroadcastw %xmm0,%ymm0

	vpxor	%ymm1,%ymm1,%ymm1

	/* Handle unaligned head */
	mov	%edi,%eax
	and	$31,%eax
	jz	.Laligned_findchr16_avx2

.Lhead_findchr16_avx2:
	movzwl	(%rdi),%ecx
	cmpw	%si,%cx
	je	.Lfound_findchr16_avx2
	testw	%cx,%cx
	jz	.Lnotfound_findchr16_avx2
	add	$2,%edi
	test	$31,%edi
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

	add	$32,%edi
	jmp	.Lloop_findchr16_avx2

.Lfound_vec_findchr16_avx2:
	tzcntl	%eax,%eax
	shrl	$1,%eax
	lea	(%rdi,%rax,2),%edi

.Lfound_findchr16_avx2:
	mov	%edi,%eax
	vzeroupper
	ret

.Lnotfound_findchr16_avx2:
	xor	%eax,%eax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlFindChar16_AVX2)

/*
 * RtlFindLastChar16_AVX2 - AVX2-optimized 16-bit reverse search
 * %edi: string pointer (CHAR16*, 32-bit pointer)
 * %esi: character to find (low 16 bits)
 * Returns: pointer to last occurrence or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar16_AVX2)
	mov	%edi,%r8d
	xor	%r9d,%r9d		/* Last match */

	/* Broadcast character */
	vmovd	%esi,%xmm0
	vpbroadcastw %xmm0,%ymm0
	vpxor	%ymm1,%ymm1,%ymm1

	/* Find end */
	mov	%edi,%eax
	and	$31,%eax
	jz	.Laligned_end16_avx2

.Lhead_end16_avx2:
	cmpw	$0,(%rdi)
	je	.Lfound_end16_avx2
	add	$2,%edi
	test	$31,%edi
	jnz	.Lhead_end16_avx2

.Laligned_end16_avx2:
.Lloop_end16_avx2:
	vmovdqa	(%rdi),%ymm2
	vpcmpeqw %ymm1,%ymm2,%ymm3
	vpmovmskb %ymm3,%eax
	testl	%eax,%eax
	jnz	.Lfound_end_partial16_avx2
	add	$32,%edi
	jmp	.Lloop_end16_avx2

.Lfound_end_partial16_avx2:
	tzcntl	%eax,%eax
	shrl	$1,%eax
	lea	(%rdi,%rax,2),%edi

.Lfound_end16_avx2:
	/* Scan for matches */
	mov	%r8d,%eax

.Lscan_matches16_avx2:
	cmp	%edi,%eax
	jae	.Ldone_findlast16_avx2

	vmovdqu	(%rax),%ymm2
	vpcmpeqw %ymm0,%ymm2,%ymm3
	vpmovmskb %ymm3,%ecx

	testl	%ecx,%ecx
	jz	.Lnext_block_findlast16_avx2

	/* Find last match in block */
	bsrl	%ecx,%ecx
	shrl	$1,%ecx
	lea	(%rax,%rcx,2),%r9d

.Lnext_block_findlast16_avx2:
	add	$32,%eax
	jmp	.Lscan_matches16_avx2

.Ldone_findlast16_avx2:
	mov	%r9d,%eax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar16_AVX2)

/*
 * RtlCompareChars16_AVX2 - AVX2-optimized 16-bit string comparison
 * %edi: string1 (CHAR16*, 32-bit pointer)
 * %esi: string2 (CHAR16*, 32-bit pointer)
 * %edx: max length (in characters, 32-bit size)
 * Returns: difference of first differing chars
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars16_AVX2)
	test	%edx,%edx
	jz	.Lequal_cmp16_avx2

	cmp	$16,%edx
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

	add	$32,%edi
	add	$32,%esi
	sub	$16,%edx
	cmp	$16,%edx
	jae	.Lvec_cmp16_avx2

.Lword_cmp16_avx2:
	test	%edx,%edx
	jz	.Lequal_cmp16_avx2

.Lword_loop_cmp16_avx2:
	movzwl	(%rdi),%eax
	movzwl	(%rsi),%ecx
	subl	%ecx,%eax
	jnz	.Ldone_cmp16_avx2
	testw	%cx,%cx
	jz	.Lequal_cmp16_avx2
	add	$2,%edi
	add	$2,%esi
	dec	%edx
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
 * %edi: destination (CHAR16*, 32-bit pointer)
 * %esi: source (CHAR16*, 32-bit pointer)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyString16_AVX2)
	mov	%edi,%eax		/* Save destination */
	vpxor	%ymm0,%ymm0,%ymm0

	/* Check alignment */
	mov	%edi,%ecx
	or	%esi,%ecx
	and	$31,%ecx
	jnz	.Lword_copy16_avx2

.Laligned_copy16_avx2:
	vmovdqa	(%rsi),%ymm1
	vpcmpeqw %ymm0,%ymm1,%ymm2
	vpmovmskb %ymm2,%ecx
	testl	%ecx,%ecx
	jnz	.Lpartial_copy16_avx2

	vmovdqa	%ymm1,(%rdi)
	add	$32,%edi
	add	$32,%esi
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
	add	$2,%edi
	add	$2,%esi
	jmp	.Lword_copy16_avx2

.Ldone_copy16_avx2:
	ret
ANX_ASM_FUNCTION_END(RtlCopyString16_AVX2)
