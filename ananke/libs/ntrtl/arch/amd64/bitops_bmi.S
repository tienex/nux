/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 BMI/BMI2-optimized bit operations
 *
 * Requires: BMI1 (Haswell+, Piledriver+) for TZCNT/LZCNT
 * Optionally: BMI2 (Haswell+, Excavator+) for PDEP/PEXT
 *
 * BMI1 provides:
 * - TZCNT: Count trailing zeros (like BSF but defined for zero)
 * - LZCNT: Count leading zeros
 * - ANDN: Bitwise AND-NOT
 * - BLSI, BLSMSK, BLSR: Bit manipulation
 *
 * BMI2 provides:
 * - PDEP: Parallel deposit bits
 * - PEXT: Parallel extract bits
 * - BZHI, MULX, RORX, SARX, SHRX, SHLX
 */

#include <ananke/asmdefs.h>

/*
 * RtlFindFirstSetBit_BMI - BMI-optimized find first set bit
 * %rdi: value
 * Returns: bit position (1-64), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindFirstSetBit_BMI)
	testq	%rdi,%rdi
	jz	1f			/* Return 0 if value is 0 */

	tzcnt	%rdi,%rax		/* Count trailing zeros */
	incq	%rax			/* Convert to 1-indexed */
	ret

1:	xorq	%rax,%rax
	ret
ANX_ASM_FUNCTION_END(RtlFindFirstSetBit_BMI)

/*
 * RtlFindLastSetBit_BMI - BMI-optimized find last set bit
 * %rdi: value
 * Returns: bit position (1-64), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastSetBit_BMI)
	testq	%rdi,%rdi
	jz	1f			/* Return 0 if value is 0 */

	lzcnt	%rdi,%rax		/* Count leading zeros */
	movq	$64,%rcx
	subq	%rax,%rcx		/* 64 - leading_zeros = bit position */
	movq	%rcx,%rax
	ret

1:	xorq	%rax,%rax
	ret
ANX_ASM_FUNCTION_END(RtlFindLastSetBit_BMI)

/*
 * RtlCountSetBits - Count number of set bits (population count)
 * %rdi: value
 * Returns: number of set bits
 *
 * Uses POPCNT instruction (part of SSE4.2, but often grouped with BMI)
 */
ANX_ASM_FUNCTION_BEGIN(RtlCountSetBits)
	popcnt	%rdi,%rax
	ret
ANX_ASM_FUNCTION_END(RtlCountSetBits)

/*
 * RtlExtractBits_BMI2 - Extract bits using mask (BMI2)
 * %rdi: source value
 * %rsi: mask (which bits to extract)
 * Returns: extracted bits packed to the right
 *
 * Example: RtlExtractBits(0xABCD1234, 0x0F0F0F0F) = 0x0000D234
 */
ANX_ASM_FUNCTION_BEGIN(RtlExtractBits_BMI2)
	pext	%rsi,%rdi,%rax		/* Parallel extract */
	ret
ANX_ASM_FUNCTION_END(RtlExtractBits_BMI2)

/*
 * RtlDepositBits_BMI2 - Deposit bits using mask (BMI2)
 * %rdi: source value (packed bits)
 * %rsi: mask (where to deposit bits)
 * Returns: bits deposited according to mask
 *
 * Example: RtlDepositBits(0x1234, 0x0F0F0F0F) = 0x01020304
 */
ANX_ASM_FUNCTION_BEGIN(RtlDepositBits_BMI2)
	pdep	%rsi,%rdi,%rax		/* Parallel deposit */
	ret
ANX_ASM_FUNCTION_END(RtlDepositBits_BMI2)

/*
 * RtlResetLowestSetBit - Clear lowest set bit (BMI1)
 * %rdi: value
 * Returns: value with lowest set bit cleared
 *
 * Example: RtlResetLowestSetBit(0b1010) = 0b1000
 */
ANX_ASM_FUNCTION_BEGIN(RtlResetLowestSetBit)
	blsr	%rdi,%rax		/* Bit field reset */
	ret
ANX_ASM_FUNCTION_END(RtlResetLowestSetBit)

/*
 * RtlIsolateLowestSetBit - Get only the lowest set bit (BMI1)
 * %rdi: value
 * Returns: value with only lowest set bit
 *
 * Example: RtlIsolateLowestSetBit(0b1010) = 0b0010
 */
ANX_ASM_FUNCTION_BEGIN(RtlIsolateLowestSetBit)
	blsi	%rdi,%rax		/* Bit field isolate */
	ret
ANX_ASM_FUNCTION_END(RtlIsolateLowestSetBit)

/*
 * RtlMaskUpToLowestSetBit - Create mask up to lowest set bit (BMI1)
 * %rdi: value
 * Returns: mask with all bits set up to and including lowest set bit
 *
 * Example: RtlMaskUpToLowestSetBit(0b10100) = 0b00111
 */
ANX_ASM_FUNCTION_BEGIN(RtlMaskUpToLowestSetBit)
	blsmsk	%rdi,%rax		/* Bit field mask */
	ret
ANX_ASM_FUNCTION_END(RtlMaskUpToLowestSetBit)

/*
 * RtlAndNot - Bitwise AND-NOT (BMI1)
 * %rdi: value1
 * %rsi: value2
 * Returns: (~value1) & value2
 *
 * More efficient than NOT + AND separately
 */
ANX_ASM_FUNCTION_BEGIN(RtlAndNot)
	andn	%rsi,%rdi,%rax		/* AND-NOT */
	ret
ANX_ASM_FUNCTION_END(RtlAndNot)
