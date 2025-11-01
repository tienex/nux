/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * amd64_32 (x32) APX + BMI/BMI2 optimized bit operations
 *
 * Requires: APX + BMI/BMI2
 *
 * APX features used:
 * - Extended GPRs (R16-R31) for intermediate values
 * - NDD (New Data Destination) forms with BMI instructions
 * - Combines with TZCNT, LZCNT, POPCNT, PEXT, PDEP
 *
 * Adapted for x32 ABI (ILP32 - 32-bit pointers, 64-bit instructions)
 */

#include <ananke/asmdefs.h>

/*
 * RtlFindFirstSetBit_APX_BMI - APX + BMI optimized find first set bit
 * %edi: value
 * Returns: bit position (1-based), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindFirstSetBit_APX_BMI)
	tzcnt	%edi,%r16d		/* R16 = trailing zeros (NDD) */
	cmp	$32,%r16d
	jne	1f
	xor	%eax,%eax		/* No bits set */
	ret
1:	lea	1(%r16),%eax		/* 1-based indexing (NDD) */
	ret
ANX_ASM_FUNCTION_END(RtlFindFirstSetBit_APX_BMI)

/*
 * RtlFindLastSetBit_APX_BMI - APX + BMI optimized find last set bit
 * %edi: value
 * Returns: bit position (1-based), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastSetBit_APX_BMI)
	lzcnt	%edi,%r16d		/* R16 = leading zeros (NDD) */
	cmp	$32,%r16d
	je	1f
	mov	$32,%r17d
	sub	%r16d,%r17d,%eax	/* eax = 32 - lz (NDD) */
	ret
1:	xor	%eax,%eax		/* No bits set */
	ret
ANX_ASM_FUNCTION_END(RtlFindLastSetBit_APX_BMI)

/*
 * RtlCountSetBits_APX_BMI - APX + BMI optimized popcount
 * %edi: value
 * Returns: number of set bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlCountSetBits_APX_BMI)
	popcnt	%edi,%eax
	ret
ANX_ASM_FUNCTION_END(RtlCountSetBits_APX_BMI)

/*
 * RtlExtractBits_APX_BMI2 - APX + BMI2 extract bits (PEXT)
 * %edi: value
 * %esi: mask
 * Returns: extracted bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlExtractBits_APX_BMI2)
	pext	%esi,%edi,%eax		/* NDD form supported */
	ret
ANX_ASM_FUNCTION_END(RtlExtractBits_APX_BMI2)

/*
 * RtlDepositBits_APX_BMI2 - APX + BMI2 deposit bits (PDEP)
 * %edi: value
 * %esi: mask
 * Returns: deposited bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlDepositBits_APX_BMI2)
	pdep	%esi,%edi,%eax		/* NDD form supported */
	ret
ANX_ASM_FUNCTION_END(RtlDepositBits_APX_BMI2)

/*
 * RtlAndNot_APX - APX optimized AND-NOT operation
 * %edi: value1
 * %esi: value2
 * Returns: value1 & ~value2
 */
ANX_ASM_FUNCTION_BEGIN(RtlAndNot_APX)
	andn	%edi,%esi,%eax		/* eax = esi & ~edi (NDD) */
	ret
ANX_ASM_FUNCTION_END(RtlAndNot_APX)

/*
 * RtlExtractBitField_APX - APX optimized bit field extraction
 * %edi: value
 * %esi: start bit position
 * %edx: length
 * Returns: extracted bit field
 */
ANX_ASM_FUNCTION_BEGIN(RtlExtractBitField_APX)
	/* Use BEXTR - Extract Bit Field */
	/* Control: bits[7:0] = start, bits[15:8] = length */
	shl	$8,%edx,%r16d		/* R16 = length << 8 */
	or	%esi,%r16d		/* R16 = (length << 8) | start */
	bextr	%r16d,%edi,%eax		/* Extract (NDD form) */
	ret
ANX_ASM_FUNCTION_END(RtlExtractBitField_APX)

/*
 * RtlResetLowestSetBit_APX - APX optimized reset lowest set bit
 * %edi: value
 * Returns: value with lowest set bit cleared
 */
ANX_ASM_FUNCTION_BEGIN(RtlResetLowestSetBit_APX)
	blsr	%edi,%eax		/* Clear lowest set bit (NDD) */
	ret
ANX_ASM_FUNCTION_END(RtlResetLowestSetBit_APX)

/*
 * RtlGetLowestSetBit_APX - APX optimized get lowest set bit
 * %edi: value
 * Returns: mask with only lowest set bit
 */
ANX_ASM_FUNCTION_BEGIN(RtlGetLowestSetBit_APX)
	blsi	%edi,%eax		/* Isolate lowest set bit (NDD) */
	ret
ANX_ASM_FUNCTION_END(RtlGetLowestSetBit_APX)

/*
 * RtlGetMaskUpToLowestSetBit_APX - APX optimized mask up to lowest set bit
 * %edi: value
 * Returns: mask up to and including lowest set bit
 */
ANX_ASM_FUNCTION_BEGIN(RtlGetMaskUpToLowestSetBit_APX)
	blsmsk	%edi,%eax		/* Mask up to lowest set bit (NDD) */
	ret
ANX_ASM_FUNCTION_END(RtlGetMaskUpToLowestSetBit_APX)

/*
 * RtlGetMaskFromLowestSetBit_APX - APX optimized mask from lowest set bit
 * %edi: value
 * Returns: mask from lowest set bit to MSB
 */
ANX_ASM_FUNCTION_BEGIN(RtlGetMaskFromLowestSetBit_APX)
	blsfill	%edi,%eax		/* Fill from lowest set bit (NDD) */
	ret
ANX_ASM_FUNCTION_END(RtlGetMaskFromLowestSetBit_APX)
