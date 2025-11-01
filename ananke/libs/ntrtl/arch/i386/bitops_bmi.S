/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 BMI/BMI2 optimized bit operations
 *
 * Requires: BMI + BMI2 (Haswell and later, available in 32-bit mode)
 *
 * BMI instructions:
 * - TZCNT, LZCNT: count trailing/leading zeros
 * - ANDN: and-not operation
 * - BLSI, BLSMSK, BLSR: bit manipulation
 *
 * BMI2 instructions:
 * - PEXT, PDEP: parallel extract/deposit
 * - BZHI: zero high bits
 * - SARX, SHLX, SHRX: shifts with register count
 * - RORX: rotate right immediate
 * - MULX: unsigned multiply
 */

#include <ananke/asmdefs.h>

/*
 * RtlFindFirstSetBit_BMI - Find first set bit (1-indexed)
 * 4(%esp): value
 * Returns: 1-indexed bit position, or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindFirstSetBit_BMI)
	movl	4(%esp),%eax
	tzcnt	%eax,%ecx		/* BMI: count trailing zeros */
	xorl	%eax,%eax
	testl	%ecx,%ecx
	cmovz	%eax,%ecx		/* If zero, return 0 */
	leal	1(%ecx),%eax		/* 1-indexed */
	ret
ANX_ASM_FUNCTION_END(RtlFindFirstSetBit_BMI)

/*
 * RtlFindLastSetBit_BMI - Find last set bit (1-indexed)
 * 4(%esp): value
 * Returns: 1-indexed bit position, or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastSetBit_BMI)
	movl	4(%esp),%eax
	lzcnt	%eax,%ecx		/* BMI: count leading zeros */
	movl	$32,%eax
	subl	%ecx,%eax		/* 32 - lzcnt */
	ret
ANX_ASM_FUNCTION_END(RtlFindLastSetBit_BMI)

/*
 * RtlPopulationCount_BMI - Count set bits
 * 4(%esp): value
 * Returns: number of set bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlPopulationCount_BMI)
	movl	4(%esp),%eax
	popcnt	%eax,%eax		/* POPCNT instruction */
	ret
ANX_ASM_FUNCTION_END(RtlPopulationCount_BMI)

/*
 * RtlExtractBits_BMI2 - Extract bits using mask
 * 4(%esp): source value
 * 8(%esp): mask
 * Returns: extracted and compressed bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlExtractBits_BMI2)
	movl	4(%esp),%eax
	movl	8(%esp),%edx
	pext	%edx,%eax,%eax		/* BMI2: parallel extract */
	ret
ANX_ASM_FUNCTION_END(RtlExtractBits_BMI2)

/*
 * RtlDepositBits_BMI2 - Deposit bits using mask
 * 4(%esp): source value
 * 8(%esp): mask
 * Returns: deposited bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlDepositBits_BMI2)
	movl	4(%esp),%eax
	movl	8(%esp),%edx
	pdep	%edx,%eax,%eax		/* BMI2: parallel deposit */
	ret
ANX_ASM_FUNCTION_END(RtlDepositBits_BMI2)

/*
 * RtlIsolateLowBit_BMI - Isolate lowest set bit
 * 4(%esp): value
 * Returns: value with only lowest bit set
 */
ANX_ASM_FUNCTION_BEGIN(RtlIsolateLowBit_BMI)
	movl	4(%esp),%eax
	blsi	%eax,%eax		/* BMI: isolate lowest set bit */
	ret
ANX_ASM_FUNCTION_END(RtlIsolateLowBit_BMI)

/*
 * RtlClearLowBit_BMI - Clear lowest set bit
 * 4(%esp): value
 * Returns: value with lowest bit cleared
 */
ANX_ASM_FUNCTION_BEGIN(RtlClearLowBit_BMI)
	movl	4(%esp),%eax
	blsr	%eax,%eax		/* BMI: reset lowest set bit */
	ret
ANX_ASM_FUNCTION_END(RtlClearLowBit_BMI)

/*
 * RtlMaskUpToLowBit_BMI - Get mask up to lowest set bit
 * 4(%esp): value
 * Returns: mask of bits up to and including lowest set bit
 */
ANX_ASM_FUNCTION_BEGIN(RtlMaskUpToLowBit_BMI)
	movl	4(%esp),%eax
	blsmsk	%eax,%eax		/* BMI: mask up to lowest set bit */
	ret
ANX_ASM_FUNCTION_END(RtlMaskUpToLowBit_BMI)

/*
 * RtlAndNot_BMI - Compute (~value1) & value2
 * 4(%esp): value1
 * 8(%esp): value2
 * Returns: (~value1) & value2
 */
ANX_ASM_FUNCTION_BEGIN(RtlAndNot_BMI)
	movl	4(%esp),%eax
	movl	8(%esp),%edx
	andn	%edx,%eax,%eax		/* BMI: and-not */
	ret
ANX_ASM_FUNCTION_END(RtlAndNot_BMI)

/*
 * RtlZeroHighBits_BMI2 - Zero high bits starting at index
 * 4(%esp): value
 * 8(%esp): index (bits 0..index-1 preserved)
 * Returns: value with high bits zeroed
 */
ANX_ASM_FUNCTION_BEGIN(RtlZeroHighBits_BMI2)
	movl	4(%esp),%eax
	movl	8(%esp),%ecx
	bzhi	%ecx,%eax,%eax		/* BMI2: zero high bits */
	ret
ANX_ASM_FUNCTION_END(RtlZeroHighBits_BMI2)

/*
 * RtlRotateLeft32_BMI2 - Rotate 32-bit value left
 * 4(%esp): value
 * 8(%esp): count
 * Returns: rotated value
 */
ANX_ASM_FUNCTION_BEGIN(RtlRotateLeft32_BMI2)
	movl	4(%esp),%eax
	movl	8(%esp),%ecx
	andl	$31,%ecx		/* count & 31 */

	movl	%eax,%edx
	shlx	%ecx,%eax,%eax		/* BMI2: eax << count */

	movl	$32,%edi
	subl	%ecx,%edi		/* 32 - count */
	shrx	%edi,%edx,%edx		/* BMI2: edx >> (32-count) */

	orl	%edx,%eax		/* Combine */
	ret
ANX_ASM_FUNCTION_END(RtlRotateLeft32_BMI2)

/*
 * RtlRotateRight32_BMI2 - Rotate 32-bit value right
 * 4(%esp): value
 * 8(%esp): count
 * Returns: rotated value
 */
ANX_ASM_FUNCTION_BEGIN(RtlRotateRight32_BMI2)
	movl	4(%esp),%eax
	movl	8(%esp),%ecx
	andl	$31,%ecx		/* count & 31 */
	rorx	%ecx,%eax,%eax		/* BMI2: rotate right */
	ret
ANX_ASM_FUNCTION_END(RtlRotateRight32_BMI2)

/*
 * RtlBitFieldExtract_BMI2 - Extract bit field
 * 4(%esp): value
 * 8(%esp): start bit
 * 12(%esp): length
 * Returns: extracted bit field
 */
ANX_ASM_FUNCTION_BEGIN(RtlBitFieldExtract_BMI2)
	movl	4(%esp),%eax
	movl	8(%esp),%ecx		/* start bit */
	movl	12(%esp),%edx		/* length */

	shrx	%ecx,%eax,%eax		/* eax >> start (BMI2) */

	/* Create mask: (1 << length) - 1 */
	movl	$1,%edi
	shlx	%edx,%edi,%edi		/* 1 << length (BMI2) */
	decl	%edi			/* mask */

	andl	%edi,%eax		/* Apply mask */
	ret
ANX_ASM_FUNCTION_END(RtlBitFieldExtract_BMI2)

/*
 * RtlBitFieldInsert_BMI2 - Insert bit field
 * 4(%esp): destination value
 * 8(%esp): source value
 * 12(%esp): start bit
 * 16(%esp): length
 * Returns: value with bit field inserted
 */
ANX_ASM_FUNCTION_BEGIN(RtlBitFieldInsert_BMI2)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi		/* destination */
	movl	16(%esp),%esi		/* source */
	movl	20(%esp),%ecx		/* start */
	movl	24(%esp),%edx		/* length */

	/* Create mask: (1 << length) - 1 */
	movl	$1,%eax
	shlx	%edx,%eax,%eax		/* 1 << length (BMI2) */
	decl	%eax			/* field mask */

	/* Mask source to field width */
	andl	%eax,%esi

	/* Shift field mask to position */
	shlx	%ecx,%eax,%eax		/* mask << start (BMI2) */

	/* Clear destination field */
	andn	%edi,%eax,%edi		/* dest & ~(mask << start) (BMI) */

	/* Shift source to position */
	shlx	%ecx,%esi,%esi		/* source << start (BMI2) */

	/* Combine */
	orl	%esi,%edi
	movl	%edi,%eax

	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlBitFieldInsert_BMI2)

/*
 * RtlReverseBits32_BMI - Reverse bits in 32-bit value
 * 4(%esp): value
 * Returns: bit-reversed value
 */
ANX_ASM_FUNCTION_BEGIN(RtlReverseBits32_BMI)
	movl	4(%esp),%eax

	/* Byte swap first */
	bswap	%eax

	/* Swap nibbles within bytes */
	movl	%eax,%edx
	andl	$0x0F0F0F0F,%edx	/* Low nibbles */
	shll	$4,%edx
	shrl	$4,%eax
	andl	$0x0F0F0F0F,%eax	/* High nibbles */
	orl	%edx,%eax

	/* Swap bit pairs */
	movl	%eax,%edx
	andl	$0x33333333,%edx
	shll	$2,%edx
	shrl	$2,%eax
	andl	$0x33333333,%eax
	orl	%edx,%eax

	/* Swap individual bits */
	movl	%eax,%edx
	andl	$0x55555555,%edx
	shll	$1,%edx
	shrl	$1,%eax
	andl	$0x55555555,%eax
	orl	%edx,%eax

	ret
ANX_ASM_FUNCTION_END(RtlReverseBits32_BMI)

/*
 * RtlFindFirstZeroBit_BMI - Find first zero bit (1-indexed)
 * 4(%esp): value
 * Returns: 1-indexed position of first zero bit
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindFirstZeroBit_BMI)
	movl	4(%esp),%eax
	notl	%eax			/* Invert */
	tzcnt	%eax,%eax		/* Count trailing zeros */
	incl	%eax			/* 1-indexed */
	ret
ANX_ASM_FUNCTION_END(RtlFindFirstZeroBit_BMI)

/*
 * RtlByteSwap32_BMI - Byte swap 32-bit value
 * 4(%esp): value
 * Returns: byte-swapped value
 */
ANX_ASM_FUNCTION_BEGIN(RtlByteSwap32_BMI)
	movl	4(%esp),%eax
	bswap	%eax
	ret
ANX_ASM_FUNCTION_END(RtlByteSwap32_BMI)
