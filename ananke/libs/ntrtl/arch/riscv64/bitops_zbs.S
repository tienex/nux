/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V64 Zbs-optimized bit operations
 *
 * Requires: Zbs (Single-Bit Manipulation) extension
 *
 * Zbs instructions:
 * - bset: Set single bit
 * - bclr: Clear single bit
 * - binv: Invert single bit
 * - bext: Extract single bit
 *
 * These operations are single-cycle and avoid branches.
 */

#include <ananke/asmdefs.h>

/*
 * RtlSetBit_Zbs - Set bit at position
 * a0: value
 * a1: bit position (0-63)
 * Returns: value with bit set
 */
ANX_ASM_FUNCTION_BEGIN(RtlSetBit_Zbs)
	bset	a0,a0,a1		/* Set bit (Zbs) */
	ret
ANX_ASM_FUNCTION_END(RtlSetBit_Zbs)

/*
 * RtlClearBit_Zbs - Clear bit at position
 * a0: value
 * a1: bit position (0-63)
 * Returns: value with bit cleared
 */
ANX_ASM_FUNCTION_BEGIN(RtlClearBit_Zbs)
	bclr	a0,a0,a1		/* Clear bit (Zbs) */
	ret
ANX_ASM_FUNCTION_END(RtlClearBit_Zbs)

/*
 * RtlInvertBit_Zbs - Invert bit at position
 * a0: value
 * a1: bit position (0-63)
 * Returns: value with bit inverted
 */
ANX_ASM_FUNCTION_BEGIN(RtlInvertBit_Zbs)
	binv	a0,a0,a1		/* Invert bit (Zbs) */
	ret
ANX_ASM_FUNCTION_END(RtlInvertBit_Zbs)

/*
 * RtlExtractBit_Zbs - Extract single bit
 * a0: value
 * a1: bit position (0-63)
 * Returns: 0 or 1
 */
ANX_ASM_FUNCTION_BEGIN(RtlExtractBit_Zbs)
	bext	a0,a0,a1		/* Extract bit (Zbs) */
	ret
ANX_ASM_FUNCTION_END(RtlExtractBit_Zbs)

/*
 * RtlTestBit_Zbs - Test if bit is set
 * a0: value
 * a1: bit position (0-63)
 * Returns: 1 if bit is set, 0 otherwise
 */
ANX_ASM_FUNCTION_BEGIN(RtlTestBit_Zbs)
	bext	a0,a0,a1		/* Extract bit (Zbs) */
	ret
ANX_ASM_FUNCTION_END(RtlTestBit_Zbs)

/*
 * RtlSetBitImm_Zbs - Set bit at immediate position
 * a0: value
 * Returns: value with bit set (position hardcoded in instruction)
 *
 * This is an example showing immediate form
 */
ANX_ASM_FUNCTION_BEGIN(RtlSetBitImm0_Zbs)
	bseti	a0,a0,0			/* Set bit 0 (Zbs immediate) */
	ret
ANX_ASM_FUNCTION_END(RtlSetBitImm0_Zbs)

ANX_ASM_FUNCTION_BEGIN(RtlSetBitImm31_Zbs)
	bseti	a0,a0,31		/* Set bit 31 (Zbs immediate) */
	ret
ANX_ASM_FUNCTION_END(RtlSetBitImm31_Zbs)

ANX_ASM_FUNCTION_BEGIN(RtlSetBitImm63_Zbs)
	bseti	a0,a0,63		/* Set bit 63 (Zbs immediate) */
	ret
ANX_ASM_FUNCTION_END(RtlSetBitImm63_Zbs)

/*
 * RtlClearBitImm_Zbs - Clear bit at immediate position
 */
ANX_ASM_FUNCTION_BEGIN(RtlClearBitImm0_Zbs)
	bclri	a0,a0,0			/* Clear bit 0 (Zbs immediate) */
	ret
ANX_ASM_FUNCTION_END(RtlClearBitImm0_Zbs)

ANX_ASM_FUNCTION_BEGIN(RtlClearBitImm31_Zbs)
	bclri	a0,a0,31		/* Clear bit 31 (Zbs immediate) */
	ret
ANX_ASM_FUNCTION_END(RtlClearBitImm31_Zbs)

ANX_ASM_FUNCTION_BEGIN(RtlClearBitImm63_Zbs)
	bclri	a0,a0,63		/* Clear bit 63 (Zbs immediate) */
	ret
ANX_ASM_FUNCTION_END(RtlClearBitImm63_Zbs)

/*
 * RtlInvertBitImm_Zbs - Invert bit at immediate position
 */
ANX_ASM_FUNCTION_BEGIN(RtlInvertBitImm0_Zbs)
	binvi	a0,a0,0			/* Invert bit 0 (Zbs immediate) */
	ret
ANX_ASM_FUNCTION_END(RtlInvertBitImm0_Zbs)

ANX_ASM_FUNCTION_BEGIN(RtlInvertBitImm31_Zbs)
	binvi	a0,a0,31		/* Invert bit 31 (Zbs immediate) */
	ret
ANX_ASM_FUNCTION_END(RtlInvertBitImm31_Zbs)

ANX_ASM_FUNCTION_BEGIN(RtlInvertBitImm63_Zbs)
	binvi	a0,a0,63		/* Invert bit 63 (Zbs immediate) */
	ret
ANX_ASM_FUNCTION_END(RtlInvertBitImm63_Zbs)

/*
 * RtlExtractBitImm_Zbs - Extract bit at immediate position
 */
ANX_ASM_FUNCTION_BEGIN(RtlExtractBitImm0_Zbs)
	bexti	a0,a0,0			/* Extract bit 0 (Zbs immediate) */
	ret
ANX_ASM_FUNCTION_END(RtlExtractBitImm0_Zbs)

ANX_ASM_FUNCTION_BEGIN(RtlExtractBitImm31_Zbs)
	bexti	a0,a0,31		/* Extract bit 31 (Zbs immediate) */
	ret
ANX_ASM_FUNCTION_END(RtlExtractBitImm31_Zbs)

ANX_ASM_FUNCTION_BEGIN(RtlExtractBitImm63_Zbs)
	bexti	a0,a0,63		/* Extract bit 63 (Zbs immediate) */
	ret
ANX_ASM_FUNCTION_END(RtlExtractBitImm63_Zbs)

/*
 * RtlBitMask_Zbs - Create mask with single bit set
 * a0: bit position (0-63)
 * Returns: value with only specified bit set
 */
ANX_ASM_FUNCTION_BEGIN(RtlBitMask_Zbs)
	li	a1,0
	bset	a0,a1,a0		/* Set bit in zero value */
	ret
ANX_ASM_FUNCTION_END(RtlBitMask_Zbs)

/*
 * RtlToggleBits_Zbs - Toggle multiple bits
 * a0: value
 * a1: bit 0 position
 * a2: bit 1 position
 * a3: bit 2 position
 * Returns: value with three bits toggled
 *
 * Example of chaining Zbs operations
 */
ANX_ASM_FUNCTION_BEGIN(RtlToggle3Bits_Zbs)
	binv	a0,a0,a1		/* Invert bit 0 */
	binv	a0,a0,a2		/* Invert bit 1 */
	binv	a0,a0,a3		/* Invert bit 2 */
	ret
ANX_ASM_FUNCTION_END(RtlToggle3Bits_Zbs)

/*
 * RtlBitfieldModify_Zbs - Modify bit field using Zbs
 * a0: original value
 * a1: start bit
 * a2: end bit (inclusive)
 * a3: new bit value (0 or 1)
 * Returns: modified value
 *
 * This shows how Zbs can be used for bit field operations
 */
ANX_ASM_FUNCTION_BEGIN(RtlBitfieldModify_Zbs)
	mv	t0,a0			/* Save original */
	mv	t1,a1			/* Current bit */

1:	beqz	a3,2f			/* If new value is 0, clear */
	bset	t0,t0,t1		/* Set bit */
	j	3f
2:	bclr	t0,t0,t1		/* Clear bit */

3:	addi	t1,t1,1			/* Next bit */
	bleu	t1,a2,1b		/* Continue if <= end */

	mv	a0,t0
	ret
ANX_ASM_FUNCTION_END(RtlBitfieldModify_Zbs)
