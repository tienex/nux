/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V64 atomic operations
 *
 * Provides lock-free atomic operations using RISC-V A (atomic) extension.
 * Supports both 32-bit and 64-bit atomic operations on RV64.
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
 * RtlAtomicCompareExchange64 - Atomic compare-and-swap (64-bit)
 * a0: pointer to value
 * a1: expected value
 * a2: new value
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicCompareExchange64)
1:	lr.d	t0, (a0)		/* Load-reserved doubleword */
	bne	t0, a1, 2f		/* If not equal to expected, fail */
	sc.d	t1, a2, (a0)		/* Store-conditional new value */
	bnez	t1, 1b			/* Retry if store failed */
2:	mv	a0, t0			/* Return previous value */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicCompareExchange64)

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
 * RtlAtomicExchange64 - Atomic exchange (64-bit)
 * a0: pointer to value
 * a1: new value
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicExchange64)
	amoswap.d a0, a1, (a0)		/* Atomic swap, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicExchange64)

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
 * RtlAtomicFetchAdd64 - Atomic fetch-and-add (64-bit)
 * a0: pointer to value
 * a1: addend
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAdd64)
	amoadd.d a0, a1, (a0)		/* Atomic add, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchAdd64)

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
 * RtlAtomicFetchOr64 - Atomic fetch-and-or (64-bit)
 * a0: pointer to value
 * a1: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchOr64)
	amoor.d	a0, a1, (a0)		/* Atomic or, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchOr64)

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
 * RtlAtomicFetchAnd64 - Atomic fetch-and-and (64-bit)
 * a0: pointer to value
 * a1: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAnd64)
	amoand.d a0, a1, (a0)		/* Atomic and, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchAnd64)

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
 * RtlAtomicFetchXor64 - Atomic fetch-and-xor (64-bit)
 * a0: pointer to value
 * a1: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchXor64)
	amoxor.d a0, a1, (a0)		/* Atomic xor, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchXor64)

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
 * RtlAtomicFetchMin64 - Atomic fetch-and-min (64-bit signed)
 * a0: pointer to value
 * a1: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchMin64)
	amomin.d a0, a1, (a0)		/* Atomic min, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchMin64)

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
 * RtlAtomicFetchMax64 - Atomic fetch-and-max (64-bit signed)
 * a0: pointer to value
 * a1: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchMax64)
	amomax.d a0, a1, (a0)		/* Atomic max, old value in a0 */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchMax64)

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
 * RtlAtomicIncrement64 - Atomic increment (64-bit)
 * a0: pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicIncrement64)
	li	a1, 1
	amoadd.d a0, a1, (a0)		/* Atomic add 1, old value in a0 */
	addi	a0, a0, 1		/* Return new value */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicIncrement64)

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
 * RtlAtomicDecrement64 - Atomic decrement (64-bit)
 * a0: pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicDecrement64)
	li	a1, -1
	amoadd.d a0, a1, (a0)		/* Atomic add -1, old value in a0 */
	addi	a0, a0, -1		/* Return new value */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicDecrement64)

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
