/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V32 bit manipulation operations
 */

#include <ananke/asmdefs.h>

/*
 * RtlFindFirstSetBit - Find first set bit (1-indexed)
 * a0: value
 * Returns: bit position (1-based), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindFirstSetBit)
	beqz	a0, 2f
	li	t0, 0		/* Bit position */

1:	andi	t1, a0, 1
	bnez	t1, 3f
	srli	a0, a0, 1
	addi	t0, t0, 1
	bnez	a0, 1b

2:	li	a0, 0		/* No bits set */
	ret

3:	addi	a0, t0, 1	/* Return 1-based position */
	ret
ANX_ASM_FUNCTION_END(RtlFindFirstSetBit)

/*
 * RtlFindLastSetBit - Find last set bit (1-indexed from LSB)
 * a0: value
 * Returns: bit position (1-based), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastSetBit)
	beqz	a0, 2f
	li	t0, 32		/* Start from bit 31 */
	li	t1, 1
	slli	t1, t1, 31	/* 0x80000000 */

1:	and	t2, a0, t1
	bnez	t2, 3f
	srli	t1, t1, 1
	addi	t0, t0, -1
	bnez	t1, 1b

2:	li	a0, 0		/* No bits set */
	ret

3:	mv	a0, t0
	ret
ANX_ASM_FUNCTION_END(RtlFindLastSetBit)

/*
 * RtlCountSetBits - Count number of set bits (popcount)
 * a0: value
 * Returns: number of set bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlCountSetBits)
	li	t0, 0		/* Count */
	beqz	a0, 2f

1:	andi	t1, a0, 1
	add	t0, t0, t1
	srli	a0, a0, 1
	bnez	a0, 1b

2:	mv	a0, t0
	ret
ANX_ASM_FUNCTION_END(RtlCountSetBits)

/*
 * RtlByteSwap16 - Swap bytes in 16-bit value
 * a0: value
 * Returns: byte-swapped value
 */
ANX_ASM_FUNCTION_BEGIN(RtlByteSwap16)
	andi	t0, a0, 0xff	/* Low byte */
	srli	t1, a0, 8
	andi	t1, t1, 0xff	/* High byte */
	slli	t0, t0, 8
	or	a0, t0, t1
	ret
ANX_ASM_FUNCTION_END(RtlByteSwap16)

/*
 * RtlByteSwap32 - Swap bytes in 32-bit value
 * a0: value
 * Returns: byte-swapped value
 */
ANX_ASM_FUNCTION_BEGIN(RtlByteSwap32)
	/* Swap bytes: ABCD -> DCBA */
	srli	t0, a0, 24		/* A -> t0[7:0] */
	srli	t1, a0, 8
	andi	t1, t1, 0xff00		/* B -> t1[15:8] */
	slli	t2, a0, 8
	lui	t3, 0xff
	and	t2, t2, t3		/* C -> t2[23:16] */
	slli	t3, a0, 24		/* D -> t3[31:24] */
	or	t0, t0, t1
	or	t0, t0, t2
	or	a0, t0, t3
	ret
ANX_ASM_FUNCTION_END(RtlByteSwap32)
