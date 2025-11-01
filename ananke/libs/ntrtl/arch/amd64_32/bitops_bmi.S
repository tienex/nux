/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * x32 BMI/BMI2-optimized bit operations
 * Uses TZCNT, LZCNT, POPCNT, PEXT, PDEP instructions
 */

#include <ananke/asmdefs.h>

/*
 * RtlFindFirstSetBit_BMI - BMI-optimized find first set bit
 * %edi: value
 * Returns: bit position (1-based), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindFirstSetBit_BMI)
	tzcnt	%edi,%eax
	cmp	$32,%eax
	jne	1f
	xor	%eax,%eax	/* No bits set */
	ret
1:	add	$1,%eax		/* 1-based indexing */
	ret
ANX_ASM_FUNCTION_END(RtlFindFirstSetBit_BMI)

/*
 * RtlFindLastSetBit_BMI - BMI-optimized find last set bit
 * %edi: value
 * Returns: bit position (1-based), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastSetBit_BMI)
	lzcnt	%edi,%eax
	cmp	$32,%eax
	je	1f
	mov	$32,%ecx
	sub	%eax,%ecx
	mov	%ecx,%eax
	ret
1:	xor	%eax,%eax	/* No bits set */
	ret
ANX_ASM_FUNCTION_END(RtlFindLastSetBit_BMI)

/*
 * RtlCountSetBits_BMI - BMI-optimized popcount
 * %edi: value
 * Returns: number of set bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlCountSetBits_BMI)
	popcnt	%edi,%eax
	ret
ANX_ASM_FUNCTION_END(RtlCountSetBits_BMI)

/*
 * RtlExtractBits_BMI2 - Extract bits using mask (PEXT)
 * %edi: value
 * %esi: mask
 * Returns: extracted bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlExtractBits_BMI2)
	pext	%esi,%edi,%eax
	ret
ANX_ASM_FUNCTION_END(RtlExtractBits_BMI2)

/*
 * RtlDepositBits_BMI2 - Deposit bits using mask (PDEP)
 * %edi: value
 * %esi: mask
 * Returns: deposited bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlDepositBits_BMI2)
	pdep	%esi,%edi,%eax
	ret
ANX_ASM_FUNCTION_END(RtlDepositBits_BMI2)
