/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * amd64_32 (x32) APX + AVX2 optimized string operations (CHAR8)
 *
 * Requires: APX + AVX2
 *
 * APX features used:
 * - Extended GPRs (R16-R31) for loop control
 * - NDD forms for non-destructive operations
 * - Combines with AVX2 for fast string processing
 *
 * Adapted for x32 ABI (ILP32 - 32-bit pointers)
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength_APX_AVX2 - APX + AVX2 optimized string length
 * %edi: string pointer (32-bit)
 * Returns: length
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength_APX_AVX2)
	mov	%edi,%r16d		/* R16 = original pointer */
	vpxor	%ymm0,%ymm0,%ymm0	/* Zero register for comparison */

.Lloop_apx_strlen:
	vmovdqu	(%rdi),%ymm1
	vpcmpeqb %ymm0,%ymm1,%ymm2	/* Compare with zero */
	vpmovmskb %ymm2,%r17d		/* R17 = bitmask */
	test	%r17d,%r17d
	jnz	.Lfound_apx_strlen

	add	$32,%edi
	jmp	.Lloop_apx_strlen

.Lfound_apx_strlen:
	tzcnt	%r17d,%r18d		/* R18 = position of null */
	sub	%r16d,%edi,%eax		/* eax = offset (NDD) */
	add	%r18d,%eax		/* Add position within chunk */
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlStringLength_APX_AVX2)

/*
 * RtlFindChar_APX_AVX2 - APX + AVX2 optimized character search
 * %edi: string pointer
 * %esi: character to find (in %sil)
 * Returns: pointer or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar_APX_AVX2)
	mov	%edi,%r16d		/* R16 = original pointer */
	vpxor	%ymm0,%ymm0,%ymm0	/* Zero for null check */
	vpbroadcastb %esi,%ymm1		/* Broadcast search char */

.Lloop_apx_findchr:
	vmovdqu	(%rdi),%ymm2
	vpcmpeqb %ymm0,%ymm2,%ymm3	/* Check for null */
	vpcmpeqb %ymm1,%ymm2,%ymm4	/* Check for character */
	vpmovmskb %ymm3,%r17d		/* R17 = null mask */
	vpmovmskb %ymm4,%r18d		/* R18 = char mask */

	test	%r18d,%r18d
	jnz	.Lfound_char_apx

	test	%r17d,%r17d
	jnz	.Lnot_found_apx		/* Hit null before char */

	add	$32,%edi
	jmp	.Lloop_apx_findchr

.Lfound_char_apx:
	tzcnt	%r18d,%r19d		/* R19 = char position */
	test	%r17d,%r17d
	jz	.Lreturn_found_apx
	tzcnt	%r17d,%r20d		/* R20 = null position */
	cmp	%r20d,%r19d
	jae	.Lnot_found_apx		/* Char after null */

.Lreturn_found_apx:
	add	%r19d,%edi,%eax		/* Return pointer (NDD) */
	vzeroupper
	ret

.Lnot_found_apx:
	xor	%eax,%eax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlFindChar_APX_AVX2)

/*
 * RtlCompareChars_APX_AVX2 - APX + AVX2 optimized string compare
 * %edi: string1
 * %esi: string2
 * Returns: 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars_APX_AVX2)
	vpxor	%ymm0,%ymm0,%ymm0	/* Zero register */

.Lloop_apx_strcmp:
	vmovdqu	(%rdi),%ymm1		/* Load s1 */
	vmovdqu	(%rsi),%ymm2		/* Load s2 */

	vpcmpeqb %ymm1,%ymm2,%ymm3	/* Compare bytes */
	vpcmpeqb %ymm0,%ymm1,%ymm4	/* Check s1 for null */
	vpcmpeqb %ymm0,%ymm2,%ymm5	/* Check s2 for null */

	vpmovmskb %ymm3,%r16d		/* R16 = equal mask */
	cmp	$0xFFFFFFFF,%r16d
	jne	.Ldiff_found_apx

	vpmovmskb %ymm4,%r17d		/* R17 = s1 null mask */
	vpmovmskb %ymm5,%r18d		/* R18 = s2 null mask */

	test	%r17d,%r17d
	jnz	.Lnull_in_s1_apx
	test	%r18d,%r18d
	jnz	.Lnull_in_s2_apx

	add	$32,%edi
	add	$32,%esi
	jmp	.Lloop_apx_strcmp

.Ldiff_found_apx:
	not	%r16d			/* Invert to get diff mask */
	tzcnt	%r16d,%r19d		/* R19 = first diff position */
	movzbl	(%rdi,%r19),%r20d	/* R20 = s1 byte */
	movzbl	(%rsi,%r19),%r21d	/* R21 = s2 byte */
	sub	%r21d,%r20d,%eax	/* Result (NDD) */
	vzeroupper
	ret

.Lnull_in_s1_apx:
	test	%r18d,%r18d
	jz	.Ls1_shorter_apx	/* s1 has null, s2 doesn't */
	xor	%eax,%eax		/* Both have null at same pos */
	vzeroupper
	ret

.Ls1_shorter_apx:
	mov	$-1,%eax
	vzeroupper
	ret

.Lnull_in_s2_apx:
	mov	$1,%eax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars_APX_AVX2)
