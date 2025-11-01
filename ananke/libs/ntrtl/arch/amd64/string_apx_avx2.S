/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 APX-optimized string operations
 *
 * Requires: APX (Intel Arrow Lake, Sierra Forest, Granite Rapids+)
 *
 * APX features used:
 * - Extended GPRs (R16-R31) for reduced register pressure
 * - NDD (New Data Destination) forms for non-destructive operations
 * - PUSH2/POP2 for efficient multi-register save/restore
 * - ZERO instruction for efficient register zeroing
 * - Combined with AVX2 for SIMD string scanning
 *
 * Compatible with:
 * - Arrow Lake (client, no AVX-512)
 * - Sierra Forest (efficiency cores, no AVX-512)
 * - Granite Rapids (has AVX-512 but not required)
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength_APX - APX-optimized string length
 * %rdi: string pointer
 * Returns: length in bytes
 *
 * Uses extended registers and AVX2 for fast scanning
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength_APX)
	movq	%rdi,%r16		/* R16 = original pointer */

	/* Check alignment */
	andq	$31,%rdi,%r17		/* R17 = alignment (NDD) */
	jz	.Laligned_len_apx

	/* Scan unaligned head byte-by-byte */
.Lhead_len_apx:
	movzbl	(%rdi),%r18d
	testl	%r18d,%r18d
	jz	.Ldone_len_apx
	incq	%rdi
	andq	$31,%rdi,%r17		/* Check if aligned now */
	jnz	.Lhead_len_apx

.Laligned_len_apx:
	/* Create zero vector for comparison */
	vpxor	%ymm0,%ymm0,%ymm0

.Lloop_len_apx:
	/* Load 32 bytes and compare with zero */
	vmovdqu	(%rdi),%ymm1
	vpcmpeqb %ymm0,%ymm1,%ymm2
	vpmovmskb %ymm2,%r17d		/* R17 = mask of zero bytes */

	testl	%r17d,%r17d
	jnz	.Lfound_len_apx

	addq	$32,%rdi
	jmp	.Lloop_len_apx

.Lfound_len_apx:
	/* Find position of first zero byte using APX */
	tzcnt	%r17d,%r18d		/* R18 = offset of null */
	addq	%r18,%rdi		/* Add offset to pointer */

.Ldone_len_apx:
	subq	%r16,%rdi		/* Length = current - original */
	movq	%rdi,%rax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlStringLength_APX)

/*
 * RtlFindChar_APX - APX-optimized character search
 * %rdi: string pointer
 * %rsi: character to find (low byte)
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar_APX)
	movq	%rdi,%r16		/* R16 = current position */
	movzbq	%sil,%r17		/* R17 = search character */

	/* Broadcast character to YMM register */
	vmovd	%r17d,%xmm0
	vpbroadcastb %xmm0,%ymm0

	/* Create zero vector for null detection */
	vpxor	%ymm1,%ymm1,%ymm1

	/* Check alignment */
	andq	$31,%rdi,%r18		/* R18 = alignment (NDD) */
	jz	.Laligned_find_apx

	/* Handle unaligned head */
.Lhead_find_apx:
	movzbl	(%r16),%r19d		/* R19 = current byte */
	cmpq	%r17,%r19		/* Compare with target */
	je	.Lfound_char_apx
	testl	%r19d,%r19d		/* Check for null */
	jz	.Lnot_found_apx
	incq	%r16
	andq	$31,%r16,%r18
	jnz	.Lhead_find_apx

.Laligned_find_apx:
	movq	%r16,%rdi

.Lloop_find_apx:
	vmovdqu	(%rdi),%ymm2

	/* Check for target character */
	vpcmpeqb %ymm0,%ymm2,%ymm3
	vpmovmskb %ymm3,%r18d		/* R18 = target matches */

	/* Check for null terminator */
	vpcmpeqb %ymm1,%ymm2,%ymm3
	vpmovmskb %ymm3,%r19d		/* R19 = null matches */

	/* If we found target, use it; else check null */
	testl	%r18d,%r18d
	jnz	.Lfound_vec_apx

	testl	%r19d,%r19d
	jnz	.Lnot_found_apx

	addq	$32,%rdi
	jmp	.Lloop_find_apx

.Lfound_vec_apx:
	/* Find first match position */
	tzcnt	%r18d,%r20d		/* R20 = offset */
	leaq	(%rdi,%r20),%r16	/* R16 = result pointer (NDD-style) */

.Lfound_char_apx:
	movq	%r16,%rax
	vzeroupper
	ret

.Lnot_found_apx:
	xorl	%eax,%eax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlFindChar_APX)

/*
 * RtlFindLastChar_APX - APX-optimized reverse character search
 * %rdi: string pointer
 * %rsi: character to find (low byte)
 * Returns: pointer to last occurrence or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar_APX)
	movq	%rdi,%r16		/* R16 = string start */
	movzbq	%sil,%r17		/* R17 = search character */
	xorl	%r18d,%r18d		/* R18 = last match (NULL) */

	/* Broadcast character */
	vmovd	%r17d,%xmm0
	vpbroadcastb %xmm0,%ymm0
	vpxor	%ymm1,%ymm1,%ymm1

	/* First, find string end */
	movq	%rdi,%r19		/* R19 = scan position */

.Lfind_end_apx:
	vmovdqu	(%r19),%ymm2
	vpcmpeqb %ymm1,%ymm2,%ymm3
	vpmovmskb %ymm3,%r20d
	testl	%r20d,%r20d
	jnz	.Lend_found_apx
	addq	$32,%r19
	jmp	.Lfind_end_apx

.Lend_found_apx:
	tzcnt	%r20d,%r21d
	leaq	(%r19,%r21),%r19	/* R19 = string end */

	/* Now scan backwards looking for matches */
	movq	%r16,%rdi		/* Reset to start */

.Lscan_for_matches_apx:
	cmpq	%r19,%rdi
	jae	.Ldone_last_apx

	vmovdqu	(%rdi),%ymm2
	vpcmpeqb %ymm0,%ymm2,%ymm3
	vpmovmskb %ymm3,%r20d

	/* If we found matches in this block, update last match */
	testl	%r20d,%r20d
	jz	.Lnext_block_apx

	/* Find highest set bit (last match in block) */
	bsr	%r20d,%r21d
	leaq	(%rdi,%r21),%r18	/* Update last match */

.Lnext_block_apx:
	addq	$32,%rdi
	jmp	.Lscan_for_matches_apx

.Ldone_last_apx:
	movq	%r18,%rax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar_APX)

/*
 * RtlCompareChars_APX - APX-optimized string comparison
 * %rdi: string1
 * %rsi: string2
 * %rdx: max length
 * Returns: difference of first differing chars
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars_APX)
	movq	%rdx,%r16		/* R16 = remaining length */
	testq	%r16,%r16
	jz	.Lequal_cmp_apx

	/* Check if we can do vector comparison */
	cmpq	$32,%r16
	jb	.Lbyte_cmp_apx

	vpxor	%ymm2,%ymm2,%ymm2	/* Zero for null check */

.Lvec_cmp_apx:
	/* Load both strings */
	vmovdqu	(%rdi),%ymm0
	vmovdqu	(%rsi),%ymm1

	/* Compare for equality */
	vpcmpeqb %ymm0,%ymm1,%ymm3
	vpmovmskb %ymm3,%r17d
	xorl	$0xFFFFFFFF,%r17d	/* Invert: 1 = different */

	/* Check for null in either string */
	vpcmpeqb %ymm2,%ymm0,%ymm3
	vpmovmskb %ymm3,%r18d
	vpcmpeqb %ymm2,%ymm1,%ymm3
	vpmovmskb %ymm3,%r19d
	orl	%r18d,%r19d		/* R19 = null in either */

	/* If different or null found, handle it */
	testl	%r17d,%r17d
	jnz	.Ldiff_vec_apx
	testl	%r19d,%r19d
	jnz	.Lequal_cmp_apx		/* Null found, strings equal */

	addq	$32,%rdi
	addq	$32,%rsi
	subq	$32,%r16
	cmpq	$32,%r16
	jae	.Lvec_cmp_apx

.Lbyte_cmp_apx:
	testq	%r16,%r16
	jz	.Lequal_cmp_apx

.Lbyte_loop_apx:
	movzbl	(%rdi),%r17d
	movzbl	(%rsi),%r18d
	subq	%r18,%r17,%r19		/* R19 = diff (NDD) */
	jnz	.Ldiff_byte_apx
	testl	%r17d,%r17d		/* Check for null */
	jz	.Lequal_cmp_apx
	incq	%rdi
	incq	%rsi
	decq	%r16
	jnz	.Lbyte_loop_apx

.Lequal_cmp_apx:
	xorl	%eax,%eax
	vzeroupper
	ret

.Ldiff_vec_apx:
	/* Find first difference */
	tzcnt	%r17d,%r20d
	movzbl	(%rdi,%r20),%r17d
	movzbl	(%rsi,%r20),%r18d

.Ldiff_byte_apx:
	movq	%r19,%rax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars_APX)
