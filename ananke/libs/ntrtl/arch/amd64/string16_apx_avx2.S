/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 APX-optimized 16-bit character string operations
 *
 * Requires: APX + AVX2
 *
 * APX features used:
 * - Extended GPRs (R16-R31) for reduced register pressure
 * - NDD (New Data Destination) forms for non-destructive operations
 * - Combined with AVX2 for SIMD 16-bit character scanning
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength16_APX - APX-optimized 16-bit string length
 * %rdi: string pointer (CHAR16*)
 * Returns: length in characters (not bytes)
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength16_APX)
	movq	%rdi,%r16		/* R16 = original pointer */

	/* Check alignment */
	andq	$31,%rdi,%r17		/* R17 = alignment (NDD) */
	jz	.Laligned_len16_apx

	/* Scan unaligned head word-by-word */
.Lhead_len16_apx:
	movzwl	(%rdi),%r18d
	testl	%r18d,%r18d
	jz	.Ldone_len16_apx
	addq	$2,%rdi
	andq	$31,%rdi,%r17
	jnz	.Lhead_len16_apx

.Laligned_len16_apx:
	/* Create zero vector for comparison */
	vpxor	%ymm0,%ymm0,%ymm0

.Lloop_len16_apx:
	/* Load 32 bytes (16 chars) and compare with zero */
	vmovdqu	(%rdi),%ymm1
	vpcmpeqw %ymm0,%ymm1,%ymm2	/* Compare words */
	vpmovmskb %ymm2,%r17d		/* R17 = mask */

	testl	%r17d,%r17d
	jnz	.Lfound_len16_apx

	addq	$32,%rdi
	jmp	.Lloop_len16_apx

.Lfound_len16_apx:
	/* Find position of first zero word */
	tzcnt	%r17d,%r18d		/* R18 = bit offset */
	shrq	$1,%r18			/* Convert to byte offset (word offset * 2) */
	addq	%r18,%rdi

.Ldone_len16_apx:
	subq	%r16,%rdi		/* Length in bytes */
	shrq	$1,%rdi			/* Convert to characters */
	movq	%rdi,%rax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlStringLength16_APX)

/*
 * RtlFindChar16_APX - APX-optimized 16-bit character search
 * %rdi: string pointer (CHAR16*)
 * %rsi: character to find (low 16 bits)
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar16_APX)
	movq	%rdi,%r16		/* R16 = current position */
	movzwq	%si,%r17		/* R17 = search character */

	/* Broadcast character to YMM register */
	vmovd	%r17d,%xmm0
	vpbroadcastw %xmm0,%ymm0	/* Broadcast as words */

	/* Create zero vector */
	vpxor	%ymm1,%ymm1,%ymm1

	/* Check alignment */
	andq	$31,%rdi,%r18		/* R18 = alignment (NDD) */
	jz	.Laligned_find16_apx

	/* Handle unaligned head */
.Lhead_find16_apx:
	movzwl	(%r16),%r19d		/* R19 = current word */
	cmpq	%r17,%r19
	je	.Lfound_char16_apx
	testl	%r19d,%r19d
	jz	.Lnot_found16_apx
	addq	$2,%r16
	andq	$31,%r16,%r18
	jnz	.Lhead_find16_apx

.Laligned_find16_apx:
	movq	%r16,%rdi

.Lloop_find16_apx:
	vmovdqu	(%rdi),%ymm2

	/* Check for target character */
	vpcmpeqw %ymm0,%ymm2,%ymm3	/* Compare words */
	vpmovmskb %ymm3,%r18d		/* R18 = target matches */

	/* Check for null terminator */
	vpcmpeqw %ymm1,%ymm2,%ymm3
	vpmovmskb %ymm3,%r19d		/* R19 = null matches */

	testl	%r18d,%r18d
	jnz	.Lfound_vec16_apx

	testl	%r19d,%r19d
	jnz	.Lnot_found16_apx

	addq	$32,%rdi
	jmp	.Lloop_find16_apx

.Lfound_vec16_apx:
	/* Find first match position (bit offset / 2 for word offset) */
	tzcnt	%r18d,%r20d		/* R20 = bit offset */
	shrq	$1,%r20			/* Convert to byte offset */
	leaq	(%rdi,%r20),%r16

.Lfound_char16_apx:
	movq	%r16,%rax
	vzeroupper
	ret

.Lnot_found16_apx:
	xorl	%eax,%eax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlFindChar16_APX)

/*
 * RtlFindLastChar16_APX - APX-optimized 16-bit reverse search
 * %rdi: string pointer (CHAR16*)
 * %rsi: character to find (low 16 bits)
 * Returns: pointer to last occurrence or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar16_APX)
	movq	%rdi,%r16		/* R16 = string start */
	movzwq	%si,%r17		/* R17 = search character */
	xorl	%r18d,%r18d		/* R18 = last match (NULL) */

	/* Broadcast character */
	vmovd	%r17d,%xmm0
	vpbroadcastw %xmm0,%ymm0
	vpxor	%ymm1,%ymm1,%ymm1

	/* Find string end */
	movq	%rdi,%r19		/* R19 = scan position */

.Lfind_end16_apx:
	vmovdqu	(%r19),%ymm2
	vpcmpeqw %ymm1,%ymm2,%ymm3
	vpmovmskb %ymm3,%r20d
	testl	%r20d,%r20d
	jnz	.Lend_found16_apx
	addq	$32,%r19
	jmp	.Lfind_end16_apx

.Lend_found16_apx:
	tzcnt	%r20d,%r21d
	shrq	$1,%r21
	leaq	(%r19,%r21,2),%r19	/* R19 = string end */

	/* Scan backwards for matches */
	movq	%r16,%rdi

.Lscan_matches16_apx:
	cmpq	%r19,%rdi
	jae	.Ldone_last16_apx

	vmovdqu	(%rdi),%ymm2
	vpcmpeqw %ymm0,%ymm2,%ymm3
	vpmovmskb %ymm3,%r20d

	testl	%r20d,%r20d
	jz	.Lnext_block16_apx

	/* Find highest set bit (last match in block) */
	bsr	%r20d,%r21d
	shrq	$1,%r21
	leaq	(%rdi,%r21,2),%r18	/* Update last match */

.Lnext_block16_apx:
	addq	$32,%rdi
	jmp	.Lscan_matches16_apx

.Ldone_last16_apx:
	movq	%r18,%rax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar16_APX)

/*
 * RtlCompareChars16_APX - APX-optimized 16-bit string comparison
 * %rdi: string1 (CHAR16*)
 * %rsi: string2 (CHAR16*)
 * %rdx: max length (in characters)
 * Returns: difference of first differing chars
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars16_APX)
	movq	%rdx,%r16		/* R16 = remaining length */
	testq	%r16,%r16
	jz	.Lequal_cmp16_apx

	/* Check if we can do vector comparison (16 chars = 32 bytes) */
	cmpq	$16,%r16
	jb	.Lword_cmp16_apx

	vpxor	%ymm2,%ymm2,%ymm2

.Lvec_cmp16_apx:
	/* Load both strings */
	vmovdqu	(%rdi),%ymm0
	vmovdqu	(%rsi),%ymm1

	/* Compare for equality */
	vpcmpeqw %ymm0,%ymm1,%ymm3	/* Compare words */
	vpmovmskb %ymm3,%r17d
	xorl	$0xFFFFFFFF,%r17d	/* Invert: 1 = different */

	/* Check for null */
	vpcmpeqw %ymm2,%ymm0,%ymm3
	vpmovmskb %ymm3,%r18d
	vpcmpeqw %ymm2,%ymm1,%ymm3
	vpmovmskb %ymm3,%r19d
	orl	%r18d,%r19d

	testl	%r17d,%r17d
	jnz	.Ldiff_vec16_apx
	testl	%r19d,%r19d
	jnz	.Lequal_cmp16_apx

	addq	$32,%rdi
	addq	$32,%rsi
	subq	$16,%r16
	cmpq	$16,%r16
	jae	.Lvec_cmp16_apx

.Lword_cmp16_apx:
	testq	%r16,%r16
	jz	.Lequal_cmp16_apx

.Lword_loop16_apx:
	movzwl	(%rdi),%r17d
	movzwl	(%rsi),%r18d
	subq	%r18,%r17,%r19		/* R19 = diff (NDD) */
	jnz	.Ldiff_word16_apx
	testl	%r17d,%r17d
	jz	.Lequal_cmp16_apx
	addq	$2,%rdi
	addq	$2,%rsi
	decq	%r16
	jnz	.Lword_loop16_apx

.Lequal_cmp16_apx:
	xorl	%eax,%eax
	vzeroupper
	ret

.Ldiff_vec16_apx:
	/* Find first difference */
	tzcnt	%r17d,%r20d
	shrq	$1,%r20
	movzwl	(%rdi,%r20,2),%r17d
	movzwl	(%rsi,%r20,2),%r18d
	subq	%r18,%r17,%r19

.Ldiff_word16_apx:
	movq	%r19,%rax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars16_APX)

/*
 * RtlCopyString16_APX - APX-optimized 16-bit string copy
 * %rdi: destination (CHAR16*)
 * %rsi: source (CHAR16*)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyString16_APX)
	movq	%rdi,%r16		/* R16 = destination */
	vpxor	%ymm0,%ymm0,%ymm0

	/* Check alignment */
	orq	%rdi,%rsi,%r17
	andq	$31,%r17
	jnz	.Lword_copy16_apx

.Laligned_copy16_apx:
	vmovdqu	(%rsi),%ymm1
	vpcmpeqw %ymm0,%ymm1,%ymm2
	vpmovmskb %ymm2,%r17d
	testl	%r17d,%r17d
	jnz	.Lpartial_copy16_apx

	vmovdqa	%ymm1,(%rdi)
	addq	$32,%rdi
	addq	$32,%rsi
	jmp	.Laligned_copy16_apx

.Lpartial_copy16_apx:
	/* Copy remaining words until null */
	tzcnt	%r17d,%r18d
	shrq	$1,%r18
	incq	%r18			/* Include null */
	movq	%r18,%rcx
	rep	movsw
	movq	%r16,%rax
	vzeroupper
	ret

.Lword_copy16_apx:
	movzwl	(%rsi),%r17d
	movw	%r17w,(%rdi)
	testl	%r17d,%r17d
	jz	.Ldone_copy16_apx
	addq	$2,%rdi
	addq	$2,%rsi
	jmp	.Lword_copy16_apx

.Ldone_copy16_apx:
	movq	%r16,%rax
	ret
ANX_ASM_FUNCTION_END(RtlCopyString16_APX)
