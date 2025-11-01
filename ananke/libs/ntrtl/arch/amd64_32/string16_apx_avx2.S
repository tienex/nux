/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * amd64_32 (x32) APX + AVX2 optimized string operations (CHAR16)
 *
 * Requires: APX + AVX2
 *
 * APX features used:
 * - Extended GPRs (R16-R31) for loop control
 * - NDD forms for non-destructive operations
 * - Combines with AVX2 for fast wide string processing
 *
 * Adapted for x32 ABI (ILP32 - 32-bit pointers)
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength16_APX_AVX2 - APX + AVX2 optimized wide string length
 * %edi: string pointer (32-bit)
 * Returns: length in CHAR16 units
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength16_APX_AVX2)
	mov	%edi,%r16d		/* R16 = original pointer */
	vpxor	%ymm0,%ymm0,%ymm0	/* Zero register */

.Lloop_apx_wcslen:
	vmovdqu	(%rdi),%ymm1		/* Load 16 CHAR16s */
	vpcmpeqw %ymm0,%ymm1,%ymm2	/* Compare with zero */
	vpmovmskb %ymm2,%r17d		/* R17 = bitmask */
	test	%r17d,%r17d
	jnz	.Lfound_apx_wcslen

	add	$32,%edi
	jmp	.Lloop_apx_wcslen

.Lfound_apx_wcslen:
	tzcnt	%r17d,%r18d		/* R18 = byte position of null */
	shr	$1,%r18d		/* Convert to CHAR16 position */
	sub	%r16d,%edi,%eax		/* eax = byte offset (NDD) */
	shr	$1,%eax			/* Convert to CHAR16 units */
	add	%r18d,%eax		/* Add position within chunk */
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlStringLength16_APX_AVX2)

/*
 * RtlFindChar16_APX_AVX2 - APX + AVX2 optimized wide character search
 * %edi: string pointer
 * %esi: character to find (in %si)
 * Returns: pointer or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar16_APX_AVX2)
	mov	%edi,%r16d		/* R16 = original pointer */
	vpxor	%ymm0,%ymm0,%ymm0	/* Zero for null check */
	vpbroadcastw %esi,%ymm1		/* Broadcast search char */

.Lloop_apx_wcschr:
	vmovdqu	(%rdi),%ymm2
	vpcmpeqw %ymm0,%ymm2,%ymm3	/* Check for null */
	vpcmpeqw %ymm1,%ymm2,%ymm4	/* Check for character */
	vpmovmskb %ymm3,%r17d		/* R17 = null mask */
	vpmovmskb %ymm4,%r18d		/* R18 = char mask */

	test	%r18d,%r18d
	jnz	.Lfound_wchar_apx

	test	%r17d,%r17d
	jnz	.Lnot_found_wchar_apx	/* Hit null before char */

	add	$32,%edi
	jmp	.Lloop_apx_wcschr

.Lfound_wchar_apx:
	tzcnt	%r18d,%r19d		/* R19 = byte position */
	test	%r17d,%r17d
	jz	.Lreturn_found_wchar_apx
	tzcnt	%r17d,%r20d		/* R20 = null byte position */
	cmp	%r20d,%r19d
	jae	.Lnot_found_wchar_apx	/* Char after null */

.Lreturn_found_wchar_apx:
	add	%r19d,%edi,%eax		/* Return pointer (NDD) */
	vzeroupper
	ret

.Lnot_found_wchar_apx:
	xor	%eax,%eax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlFindChar16_APX_AVX2)

/*
 * RtlCompareChars16_APX_AVX2 - APX + AVX2 optimized wide string compare
 * %edi: string1
 * %esi: string2
 * Returns: 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars16_APX_AVX2)
	vpxor	%ymm0,%ymm0,%ymm0	/* Zero register */

.Lloop_apx_wcscmp:
	vmovdqu	(%rdi),%ymm1		/* Load s1 */
	vmovdqu	(%rsi),%ymm2		/* Load s2 */

	vpcmpeqw %ymm1,%ymm2,%ymm3	/* Compare words */
	vpcmpeqw %ymm0,%ymm1,%ymm4	/* Check s1 for null */
	vpcmpeqw %ymm0,%ymm2,%ymm5	/* Check s2 for null */

	vpmovmskb %ymm3,%r16d		/* R16 = equal mask */
	cmp	$0xFFFFFFFF,%r16d
	jne	.Ldiff_found_wcs_apx

	vpmovmskb %ymm4,%r17d		/* R17 = s1 null mask */
	vpmovmskb %ymm5,%r18d		/* R18 = s2 null mask */

	test	%r17d,%r17d
	jnz	.Lnull_in_wcs1_apx
	test	%r18d,%r18d
	jnz	.Lnull_in_wcs2_apx

	add	$32,%edi
	add	$32,%esi
	jmp	.Lloop_apx_wcscmp

.Ldiff_found_wcs_apx:
	not	%r16d			/* Invert to get diff mask */
	tzcnt	%r16d,%r19d		/* R19 = first diff byte position */
	and	$~1,%r19d		/* Align to CHAR16 */
	movzwl	(%rdi,%r19),%r20d	/* R20 = s1 word */
	movzwl	(%rsi,%r19),%r21d	/* R21 = s2 word */
	sub	%r21d,%r20d,%eax	/* Result (NDD) */
	vzeroupper
	ret

.Lnull_in_wcs1_apx:
	test	%r18d,%r18d
	jz	.Lwcs1_shorter_apx
	xor	%eax,%eax		/* Both have null at same pos */
	vzeroupper
	ret

.Lwcs1_shorter_apx:
	mov	$-1,%eax
	vzeroupper
	ret

.Lnull_in_wcs2_apx:
	mov	$1,%eax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars16_APX_AVX2)

/*
 * RtlCopyString16_APX_AVX2 - APX + AVX2 optimized wide string copy
 * %edi: destination
 * %esi: source
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyString16_APX_AVX2)
	mov	%edi,%r16d		/* R16 = return value */
	vpxor	%ymm0,%ymm0,%ymm0	/* Zero register */

.Lloop_apx_wcscpy:
	vmovdqu	(%rsi),%ymm1		/* Load 16 CHAR16s */
	vpcmpeqw %ymm0,%ymm1,%ymm2	/* Check for null */
	vmovdqu	%ymm1,(%rdi)		/* Store */
	vpmovmskb %ymm2,%r17d		/* R17 = null mask */

	test	%r17d,%r17d
	jnz	.Ldone_apx_wcscpy

	add	$32,%edi
	add	$32,%esi
	jmp	.Lloop_apx_wcscpy

.Ldone_apx_wcscpy:
	mov	%r16d,%eax		/* Return original dest */
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlCopyString16_APX_AVX2)
