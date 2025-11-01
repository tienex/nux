/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 APX + BMI/BMI2 optimized bit operations
 *
 * Requires: APX + BMI + BMI2
 *
 * Combines:
 * - APX extended GPRs (R16-R31) for complex multi-register operations
 * - APX NDD forms for non-destructive operations
 * - BMI: TZCNT, LZCNT, ANDN, BLSI, BLSMSK, BLSR
 * - BMI2: PEXT, PDEP, BZHI, SARX, SHLX, SHRX, MULX, RORX
 *
 * This combination is ideal for operations requiring both bit manipulation
 * and multiple intermediate values without register pressure.
 */

#include <ananke/asmdefs.h>

/*
 * RtlFindFirstSetBit_APX_BMI - Find first set bit (1-indexed)
 * %rdi: value
 * Returns: 1-indexed bit position, or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindFirstSetBit_APX_BMI)
	tzcnt	%rdi,%rax		/* BMI: count trailing zeros */
	xorl	%r16d,%r16d		/* R16 = 0 (APX register) */
	cmovz	%r16,%rax		/* If zero, return 0 */
	incq	%rax			/* 1-indexed */
	ret
ANX_ASM_FUNCTION_END(RtlFindFirstSetBit_APX_BMI)

/*
 * RtlFindLastSetBit_APX_BMI - Find last set bit (1-indexed)
 * %rdi: value
 * Returns: 1-indexed bit position, or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastSetBit_APX_BMI)
	lzcnt	%rdi,%rax		/* BMI: count leading zeros */
	movq	$64,%r16		/* R16 = 64 */
	subq	%rax,%r16,%rax		/* RAX = 64 - lzcnt (NDD form) */
	ret
ANX_ASM_FUNCTION_END(RtlFindLastSetBit_APX_BMI)

/*
 * RtlPopulationCount - Count set bits
 * %rdi: value
 * Returns: number of set bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlPopulationCount_APX_BMI)
	popcnt	%rdi,%rax		/* POPCNT instruction */
	ret
ANX_ASM_FUNCTION_END(RtlPopulationCount_APX_BMI)

/*
 * RtlExtractBits_APX_BMI2 - Extract bits using mask
 * %rdi: source value
 * %rsi: mask
 * Returns: extracted and compressed bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlExtractBits_APX_BMI2)
	pext	%rsi,%rdi,%rax		/* BMI2: parallel extract */
	ret
ANX_ASM_FUNCTION_END(RtlExtractBits_APX_BMI2)

/*
 * RtlDepositBits_APX_BMI2 - Deposit bits using mask
 * %rdi: source value
 * %rsi: mask
 * Returns: deposited bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlDepositBits_APX_BMI2)
	pdep	%rsi,%rdi,%rax		/* BMI2: parallel deposit */
	ret
ANX_ASM_FUNCTION_END(RtlDepositBits_APX_BMI2)

/*
 * RtlIsolateLowBit_APX_BMI - Isolate lowest set bit
 * %rdi: value
 * Returns: value with only lowest bit set
 */
ANX_ASM_FUNCTION_BEGIN(RtlIsolateLowBit_APX_BMI)
	blsi	%rdi,%rax		/* BMI: isolate lowest set bit */
	ret
ANX_ASM_FUNCTION_END(RtlIsolateLowBit_APX_BMI)

/*
 * RtlClearLowBit_APX_BMI - Clear lowest set bit
 * %rdi: value
 * Returns: value with lowest bit cleared
 */
ANX_ASM_FUNCTION_BEGIN(RtlClearLowBit_APX_BMI)
	blsr	%rdi,%rax		/* BMI: reset lowest set bit */
	ret
ANX_ASM_FUNCTION_END(RtlClearLowBit_APX_BMI)

/*
 * RtlMaskUpToLowBit_APX_BMI - Get mask up to lowest set bit
 * %rdi: value
 * Returns: mask of bits up to and including lowest set bit
 */
ANX_ASM_FUNCTION_BEGIN(RtlMaskUpToLowBit_APX_BMI)
	blsmsk	%rdi,%rax		/* BMI: mask up to lowest set bit */
	ret
ANX_ASM_FUNCTION_END(RtlMaskUpToLowBit_APX_BMI)

/*
 * RtlAndNot_APX_BMI - Compute (~value1) & value2
 * %rdi: value1
 * %rsi: value2
 * Returns: (~value1) & value2
 */
ANX_ASM_FUNCTION_BEGIN(RtlAndNot_APX_BMI)
	andn	%rsi,%rdi,%rax		/* BMI: and-not */
	ret
ANX_ASM_FUNCTION_END(RtlAndNot_APX_BMI)

/*
 * RtlZeroHighBits_APX_BMI2 - Zero high bits starting at index
 * %rdi: value
 * %rsi: index (bits 0..index-1 preserved)
 * Returns: value with high bits zeroed
 */
ANX_ASM_FUNCTION_BEGIN(RtlZeroHighBits_APX_BMI2)
	bzhi	%rsi,%rdi,%rax		/* BMI2: zero high bits */
	ret
ANX_ASM_FUNCTION_END(RtlZeroHighBits_APX_BMI2)

/*
 * RtlRotateLeft32_APX_BMI2 - Rotate 32-bit value left
 * %rdi: value (32-bit)
 * %rsi: count
 * Returns: rotated value
 */
ANX_ASM_FUNCTION_BEGIN(RtlRotateLeft32_APX_BMI2)
	movl	%edi,%r16d		/* R16 = value */
	andl	$31,%esi,%r17d		/* R17 = count & 31 (NDD) */
	shlx	%r17d,%r16d,%r18d	/* R18 = value << count (BMI2) */
	movl	$32,%r19d
	subl	%r17d,%r19d		/* R19 = 32 - count */
	shrx	%r19d,%r16d,%r20d	/* R20 = value >> (32-count) (BMI2) */
	orl	%r18d,%r20d,%eax	/* RAX = result (NDD) */
	ret
ANX_ASM_FUNCTION_END(RtlRotateLeft32_APX_BMI2)

/*
 * RtlRotateRight32_APX_BMI2 - Rotate 32-bit value right
 * %rdi: value (32-bit)
 * %rsi: count
 * Returns: rotated value
 */
ANX_ASM_FUNCTION_BEGIN(RtlRotateRight32_APX_BMI2)
	andl	$31,%esi,%r16d		/* R16 = count & 31 */
	rorx	%r16d,%edi,%eax		/* BMI2: rotate right */
	ret
ANX_ASM_FUNCTION_END(RtlRotateRight32_APX_BMI2)

/*
 * RtlMultiplyHigh_APX_BMI2 - Multiply and get high 64 bits
 * %rdi: multiplicand
 * %rsi: multiplier
 * Returns: high 64 bits of product
 */
ANX_ASM_FUNCTION_BEGIN(RtlMultiplyHigh_APX_BMI2)
	mulx	%rsi,%r16,%rax		/* BMI2: RDX*RSI -> R16:RAX */
	/* R16 = low 64 bits, RAX = high 64 bits */
	ret
ANX_ASM_FUNCTION_END(RtlMultiplyHigh_APX_BMI2)

/*
 * RtlBitFieldExtract_APX_BMI - Extract bit field
 * %rdi: value
 * %rsi: start bit
 * %rdx: length
 * Returns: extracted bit field
 *
 * Demonstrates APX extended registers for complex operations
 */
ANX_ASM_FUNCTION_BEGIN(RtlBitFieldExtract_APX_BMI)
	movq	%rsi,%rcx		/* RCX = start bit */
	shrx	%rcx,%rdi,%r16		/* R16 = value >> start (BMI2) */

	/* Create mask: (1 << length) - 1 */
	movq	$1,%r17
	shlx	%rdx,%r17,%r18		/* R18 = 1 << length (BMI2) */
	decq	%r18			/* R18 = mask */

	andq	%r18,%r16,%rax		/* RAX = result (NDD) */
	ret
ANX_ASM_FUNCTION_END(RtlBitFieldExtract_APX_BMI)

/*
 * RtlBitFieldInsert_APX_BMI - Insert bit field
 * %rdi: destination value
 * %rsi: source value
 * %rdx: start bit
 * %rcx: length
 * Returns: value with bit field inserted
 *
 * Uses APX extended registers to avoid register pressure
 */
ANX_ASM_FUNCTION_BEGIN(RtlBitFieldInsert_APX_BMI)
	/* Create mask: (1 << length) - 1 */
	movq	$1,%r16
	shlx	%rcx,%r16,%r17		/* R17 = 1 << length (BMI2) */
	decq	%r17			/* R17 = field mask */

	/* Mask source to field width */
	andq	%r17,%rsi,%r18		/* R18 = source & mask (NDD) */

	/* Shift field mask to position */
	shlx	%rdx,%r17,%r19		/* R19 = mask << start (BMI2) */

	/* Clear destination field */
	andn	%rdi,%r19,%r20		/* R20 = dest & ~(mask << start) (BMI) */

	/* Shift source to position */
	shlx	%rdx,%r18,%r21		/* R21 = source << start (BMI2) */

	/* Combine */
	orq	%r21,%r20,%rax		/* RAX = result (NDD) */
	ret
ANX_ASM_FUNCTION_END(RtlBitFieldInsert_APX_BMI)

/*
 * RtlReverseBits32_APX_BMI - Reverse bits in 32-bit value
 * %rdi: value (32-bit)
 * Returns: bit-reversed value
 *
 * Uses BMI2 PEXT/PDEP for efficient bit reversal
 */
ANX_ASM_FUNCTION_BEGIN(RtlReverseBits32_APX_BMI)
	movl	%edi,%r16d		/* R16 = input */

	/* Reverse bits using lookup table approach with PEXT/PDEP */
	/* Split into nibbles and reverse */
	movq	$0x0F0F0F0F,%r17	/* Nibble mask */
	pext	%r17,%r16,%r18		/* R18 = even nibbles */
	andn	%r16,%r17,%r19		/* R19 = odd nibbles */
	shrx	$4,%r19,%r19		/* Shift odd nibbles down */

	/* This is a simplified version - full reversal needs more steps */
	/* For production, use BSWAP + bit swapping */
	bswap	%r16d			/* Byte swap */

	/* Swap nibbles within bytes */
	movl	%r16d,%r17d
	andl	$0x0F0F0F0F,%r17d	/* Low nibbles */
	shll	$4,%r17d
	shrl	$4,%r16d
	andl	$0x0F0F0F0F,%r16d	/* High nibbles */
	orl	%r17d,%r16d

	/* Swap bit pairs */
	movl	%r16d,%r17d
	andl	$0x33333333,%r17d
	shll	$2,%r17d
	shrl	$2,%r16d
	andl	$0x33333333,%r16d
	orl	%r17d,%r16d

	/* Swap individual bits */
	movl	%r16d,%r17d
	andl	$0x55555555,%r17d
	shll	$1,%r17d
	shrl	$1,%r16d
	andl	$0x55555555,%r16d
	orl	%r17d,%r16d,%eax	/* RAX = result (NDD) */

	ret
ANX_ASM_FUNCTION_END(RtlReverseBits32_APX_BMI)

/*
 * RtlFindFirstZeroBit_APX_BMI - Find first zero bit (1-indexed)
 * %rdi: value
 * Returns: 1-indexed position of first zero bit
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindFirstZeroBit_APX_BMI)
	notq	%rdi,%r16		/* R16 = ~value (NDD) */
	tzcnt	%r16,%rax		/* Count trailing zeros in inverted */
	incq	%rax			/* 1-indexed */
	ret
ANX_ASM_FUNCTION_END(RtlFindFirstZeroBit_APX_BMI)

/*
 * RtlByteSwap64_APX - Byte swap 64-bit value
 * %rdi: value
 * Returns: byte-swapped value
 */
ANX_ASM_FUNCTION_BEGIN(RtlByteSwap64_APX)
	bswap	%rdi,%rax		/* APX NDD form of BSWAP */
	ret
ANX_ASM_FUNCTION_END(RtlByteSwap64_APX)
