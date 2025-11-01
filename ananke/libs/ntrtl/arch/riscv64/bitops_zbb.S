/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V64 Zbb-optimized bit operations
 *
 * Requires: Zbb (Basic Bit Manipulation) extension
 *
 * Zbb instructions used:
 * - clz/ctz: count leading/trailing zeros
 * - cpop: count population (set bits)
 * - min/max/minu/maxu: min/max operations
 * - sext.b/sext.h: sign extension
 * - zext.h: zero extension
 * - rol/ror: rotate left/right
 * - rev8: byte reverse (endian swap)
 * - orc.b: OR-combine bytes
 * - andn/orn/xnor: logical operations with NOT
 */

#include <ananke/asmdefs.h>

/*
 * RtlFindFirstSetBit_Zbb - Find first set bit (1-indexed)
 * a0: value
 * Returns: 1-indexed bit position, or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindFirstSetBit_Zbb)
	ctz	a0,a0			/* Count trailing zeros (Zbb) */
	addi	a0,a0,1			/* 1-indexed */
	ret
ANX_ASM_FUNCTION_END(RtlFindFirstSetBit_Zbb)

/*
 * RtlFindLastSetBit_Zbb - Find last set bit (1-indexed)
 * a0: value
 * Returns: 1-indexed bit position, or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastSetBit_Zbb)
	clz	a1,a0			/* Count leading zeros (Zbb) */
	li	a0,64
	sub	a0,a0,a1		/* 64 - clz */
	ret
ANX_ASM_FUNCTION_END(RtlFindLastSetBit_Zbb)

/*
 * RtlPopulationCount_Zbb - Count set bits
 * a0: value
 * Returns: number of set bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlPopulationCount_Zbb)
	cpop	a0,a0			/* Count population (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlPopulationCount_Zbb)

/*
 * RtlRotateLeft64_Zbb - Rotate 64-bit value left
 * a0: value
 * a1: count
 * Returns: rotated value
 */
ANX_ASM_FUNCTION_BEGIN(RtlRotateLeft64_Zbb)
	rol	a0,a0,a1		/* Rotate left (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlRotateLeft64_Zbb)

/*
 * RtlRotateRight64_Zbb - Rotate 64-bit value right
 * a0: value
 * a1: count
 * Returns: rotated value
 */
ANX_ASM_FUNCTION_BEGIN(RtlRotateRight64_Zbb)
	ror	a0,a0,a1		/* Rotate right (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlRotateRight64_Zbb)

/*
 * RtlByteSwap64_Zbb - Byte swap 64-bit value (endian conversion)
 * a0: value
 * Returns: byte-swapped value
 */
ANX_ASM_FUNCTION_BEGIN(RtlByteSwap64_Zbb)
	rev8	a0,a0			/* Reverse bytes (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlByteSwap64_Zbb)

/*
 * RtlByteSwap32_Zbb - Byte swap 32-bit value
 * a0: value
 * Returns: byte-swapped value
 */
ANX_ASM_FUNCTION_BEGIN(RtlByteSwap32_Zbb)
	rev8	a0,a0			/* Reverse all 64 bits */
	srli	a0,a0,32		/* Shift down to get 32-bit result */
	ret
ANX_ASM_FUNCTION_END(RtlByteSwap32_Zbb)

/*
 * RtlSignExtend8_Zbb - Sign extend 8-bit to 64-bit
 * a0: 8-bit value (low byte)
 * Returns: sign-extended 64-bit value
 */
ANX_ASM_FUNCTION_BEGIN(RtlSignExtend8_Zbb)
	sext.b	a0,a0			/* Sign extend byte (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlSignExtend8_Zbb)

/*
 * RtlSignExtend16_Zbb - Sign extend 16-bit to 64-bit
 * a0: 16-bit value (low halfword)
 * Returns: sign-extended 64-bit value
 */
ANX_ASM_FUNCTION_BEGIN(RtlSignExtend16_Zbb)
	sext.h	a0,a0			/* Sign extend halfword (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlSignExtend16_Zbb)

/*
 * RtlZeroExtend16_Zbb - Zero extend 16-bit to 64-bit
 * a0: 16-bit value (low halfword)
 * Returns: zero-extended 64-bit value
 */
ANX_ASM_FUNCTION_BEGIN(RtlZeroExtend16_Zbb)
	zext.h	a0,a0			/* Zero extend halfword (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlZeroExtend16_Zbb)

/*
 * RtlMin64_Zbb - Minimum of two signed 64-bit values
 * a0: value1
 * a1: value2
 * Returns: minimum value
 */
ANX_ASM_FUNCTION_BEGIN(RtlMin64_Zbb)
	min	a0,a0,a1		/* Minimum (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlMin64_Zbb)

/*
 * RtlMax64_Zbb - Maximum of two signed 64-bit values
 * a0: value1
 * a1: value2
 * Returns: maximum value
 */
ANX_ASM_FUNCTION_BEGIN(RtlMax64_Zbb)
	max	a0,a0,a1		/* Maximum (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlMax64_Zbb)

/*
 * RtlMinU64_Zbb - Minimum of two unsigned 64-bit values
 * a0: value1
 * a1: value2
 * Returns: minimum value
 */
ANX_ASM_FUNCTION_BEGIN(RtlMinU64_Zbb)
	minu	a0,a0,a1		/* Minimum unsigned (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlMinU64_Zbb)

/*
 * RtlMaxU64_Zbb - Maximum of two unsigned 64-bit values
 * a0: value1
 * a1: value2
 * Returns: maximum value
 */
ANX_ASM_FUNCTION_BEGIN(RtlMaxU64_Zbb)
	maxu	a0,a0,a1		/* Maximum unsigned (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlMaxU64_Zbb)

/*
 * RtlAndNot_Zbb - Compute value1 & ~value2
 * a0: value1
 * a1: value2
 * Returns: value1 & ~value2
 */
ANX_ASM_FUNCTION_BEGIN(RtlAndNot_Zbb)
	andn	a0,a0,a1		/* AND-NOT (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlAndNot_Zbb)

/*
 * RtlOrNot_Zbb - Compute value1 | ~value2
 * a0: value1
 * a1: value2
 * Returns: value1 | ~value2
 */
ANX_ASM_FUNCTION_BEGIN(RtlOrNot_Zbb)
	orn	a0,a0,a1		/* OR-NOT (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlOrNot_Zbb)

/*
 * RtlXnor_Zbb - Compute ~(value1 ^ value2)
 * a0: value1
 * a1: value2
 * Returns: XNOR result
 */
ANX_ASM_FUNCTION_BEGIN(RtlXnor_Zbb)
	xnor	a0,a0,a1		/* XNOR (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlXnor_Zbb)

/*
 * RtlOrcByte_Zbb - OR-combine bytes (set byte to 0xFF if any bit set)
 * a0: value
 * Returns: value with bytes either 0x00 or 0xFF
 */
ANX_ASM_FUNCTION_BEGIN(RtlOrcByte_Zbb)
	orc.b	a0,a0			/* OR-combine bytes (Zbb) */
	ret
ANX_ASM_FUNCTION_END(RtlOrcByte_Zbb)

/*
 * RtlFindFirstZeroBit_Zbb - Find first zero bit (1-indexed)
 * a0: value
 * Returns: 1-indexed position of first zero bit
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindFirstZeroBit_Zbb)
	not	a0,a0			/* Invert */
	ctz	a0,a0			/* Count trailing zeros */
	addi	a0,a0,1			/* 1-indexed */
	ret
ANX_ASM_FUNCTION_END(RtlFindFirstZeroBit_Zbb)

/*
 * RtlIsPowerOfTwo_Zbb - Check if value is power of 2
 * a0: value
 * Returns: 1 if power of 2, 0 otherwise
 */
ANX_ASM_FUNCTION_BEGIN(RtlIsPowerOfTwo_Zbb)
	beqz	a0,1f			/* 0 is not power of 2 */
	cpop	a1,a0			/* Count set bits */
	li	a0,1
	beq	a1,a0,2f		/* If exactly 1 bit set */
1:	li	a0,0
2:	ret
ANX_ASM_FUNCTION_END(RtlIsPowerOfTwo_Zbb)

/*
 * RtlRoundUpPowerOfTwo_Zbb - Round up to next power of 2
 * a0: value
 * Returns: next power of 2 >= value
 */
ANX_ASM_FUNCTION_BEGIN(RtlRoundUpPowerOfTwo_Zbb)
	beqz	a0,1f			/* Handle 0 */
	addi	a0,a0,-1		/* value - 1 */
	clz	a1,a0			/* Count leading zeros */
	li	a0,64
	sub	a1,a0,a1		/* bits = 64 - clz */
	li	a0,1
	sll	a0,a0,a1		/* 1 << bits */
	ret
1:	li	a0,1
	ret
ANX_ASM_FUNCTION_END(RtlRoundUpPowerOfTwo_Zbb)
