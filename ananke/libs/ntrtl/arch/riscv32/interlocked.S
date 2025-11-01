/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V32 atomic operations
 *
 * Provides lock-free atomic operations using RISC-V A (atomic) extension.
 * Supports 32-bit atomic operations only on RV32.
 */

#include <ananke/asmdefs.h>

/*
 * RtlAtomicCompareExchange32 - Atomic compare-and-swap (32-bit)
 * a0: pointer to value
 * a1: expected value
 * a2: new value
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicCompareExchange32)
1:	lr.w	t0, (a0)		/* Load-reserved word */
	bne	t0, a1, 2f		/* If not equal to expected, fail */
	sc.w	t1, a2, (a0)		/* Store-conditional new value */
	bnez	t1, 1b			/* Retry if store failed */
2:	mv	a0, t0			/* Return previous value */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicCompareExchange32)

/*
 * RtlAtomicExchange32 - Atomic exchange (32-bit)
 * a0: pointer to value
 * a1: new value
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicExchange32)
	amoswap.w a0, a1, (a0)		/* Atomic swap, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicExchange32)

/*
 * RtlAtomicFetchAdd32 - Atomic fetch-and-add (32-bit)
 * a0: pointer to value
 * a1: addend
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAdd32)
	amoadd.w a0, a1, (a0)		/* Atomic add, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchAdd32)

/*
 * RtlAtomicFetchOr32 - Atomic fetch-and-or (32-bit)
 * a0: pointer to value
 * a1: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchOr32)
	amoor.w	a0, a1, (a0)		/* Atomic or, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchOr32)

/*
 * RtlAtomicFetchAnd32 - Atomic fetch-and-and (32-bit)
 * a0: pointer to value
 * a1: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAnd32)
	amoand.w a0, a1, (a0)		/* Atomic and, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchAnd32)

/*
 * RtlAtomicFetchXor32 - Atomic fetch-and-xor (32-bit)
 * a0: pointer to value
 * a1: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchXor32)
	amoxor.w a0, a1, (a0)		/* Atomic xor, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchXor32)

/*
 * RtlAtomicFetchMin32 - Atomic fetch-and-min (32-bit signed)
 * a0: pointer to value
 * a1: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchMin32)
	amomin.w a0, a1, (a0)		/* Atomic min, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchMin32)

/*
 * RtlAtomicFetchMax32 - Atomic fetch-and-max (32-bit signed)
 * a0: pointer to value
 * a1: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchMax32)
	amomax.w a0, a1, (a0)		/* Atomic max, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchMax32)

/*
 * RtlAtomicIncrement32 - Atomic increment (32-bit)
 * a0: pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicIncrement32)
	li	a1, 1
	amoadd.w a0, a1, (a0)		/* Atomic add 1, old value in a0 */
	addi	a0, a0, 1		/* Return new value */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicIncrement32)

/*
 * RtlAtomicDecrement32 - Atomic decrement (32-bit)
 * a0: pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicDecrement32)
	li	a1, -1
	amoadd.w a0, a1, (a0)		/* Atomic add -1, old value in a0 */
	addi	a0, a0, -1		/* Return new value */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicDecrement32)

/*
 * Memory barriers / fences
 */

/*
 * RtlMemoryBarrier - Full memory barrier
 * Ensures all loads and stores before this point complete before
 * any loads or stores after this point begin
 */
ANX_ASM_FUNCTION_BEGIN(RtlMemoryBarrier)
	fence	rw, rw
	ret
ANX_ASM_FUNCTION_END(RtlMemoryBarrier)

/*
 * RtlLoadBarrier - Load (acquire) barrier
 * Ensures all loads before this point complete before any loads
 * or stores after this point begin
 */
ANX_ASM_FUNCTION_BEGIN(RtlLoadBarrier)
	fence	r, rw
	ret
ANX_ASM_FUNCTION_END(RtlLoadBarrier)

/*
 * RtlStoreBarrier - Store (release) barrier
 * Ensures all loads and stores before this point complete before
 * any stores after this point begin
 */
ANX_ASM_FUNCTION_BEGIN(RtlStoreBarrier)
	fence	rw, w
	ret
ANX_ASM_FUNCTION_END(RtlStoreBarrier)
