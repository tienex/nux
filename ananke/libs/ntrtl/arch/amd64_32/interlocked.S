/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * amd64_32 (x32) atomic operations
 *
 * Adapted for x32 ABI (ILP32 - 32-bit pointers, 64-bit instructions)
 */

#include <ananke/asmdefs.h>

/*
 * RtlAtomicCompareExchange32 - Atomic compare-and-swap (32-bit)
 * %edi: pointer to value (32-bit pointer)
 * %esi: expected value
 * %edx: new value
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicCompareExchange32)
	mov	%esi,%eax
	lock cmpxchgl %edx,(%rdi)
	ret
ANX_ASM_FUNCTION_END(RtlAtomicCompareExchange32)

/*
 * RtlAtomicCompareExchange64 - Atomic compare-and-swap (64-bit)
 * %edi: pointer to value
 * %rsi: expected value (64-bit)
 * %rdx: new value (64-bit)
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicCompareExchange64)
	mov	%rsi,%rax
	lock cmpxchgq %rdx,(%rdi)
	ret
ANX_ASM_FUNCTION_END(RtlAtomicCompareExchange64)

/*
 * RtlAtomicExchange32 - Atomic exchange (32-bit)
 * %edi: pointer to value
 * %esi: new value
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicExchange32)
	mov	%esi,%eax
	xchgl	%eax,(%rdi)
	ret
ANX_ASM_FUNCTION_END(RtlAtomicExchange32)

/*
 * RtlAtomicExchange64 - Atomic exchange (64-bit)
 * %edi: pointer to value
 * %rsi: new value
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicExchange64)
	mov	%rsi,%rax
	xchgq	%rax,(%rdi)
	ret
ANX_ASM_FUNCTION_END(RtlAtomicExchange64)

/*
 * RtlAtomicFetchAdd32 - Atomic fetch-and-add (32-bit)
 * %edi: pointer to value
 * %esi: addend
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAdd32)
	mov	%esi,%eax
	lock xaddl %eax,(%rdi)
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchAdd32)

/*
 * RtlAtomicFetchAdd64 - Atomic fetch-and-add (64-bit)
 * %edi: pointer to value
 * %rsi: addend
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAdd64)
	mov	%rsi,%rax
	lock xaddq %rax,(%rdi)
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchAdd64)

/*
 * RtlAtomicFetchSub32 - Atomic fetch-and-sub (32-bit)
 * %edi: pointer to value
 * %esi: subtrahend
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchSub32)
	neg	%esi
	mov	%esi,%eax
	lock xaddl %eax,(%rdi)
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchSub32)

/*
 * RtlAtomicFetchSub64 - Atomic fetch-and-sub (64-bit)
 * %edi: pointer to value
 * %rsi: subtrahend
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchSub64)
	neg	%rsi
	mov	%rsi,%rax
	lock xaddq %rax,(%rdi)
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchSub64)

/*
 * RtlAtomicFetchOr32 - Atomic fetch-and-or (32-bit)
 * %edi: pointer to value
 * %esi: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchOr32)
	mov	(%rdi),%eax
1:	mov	%eax,%ecx
	or	%esi,%ecx
	lock cmpxchgl %ecx,(%rdi)
	jne	1b
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchOr32)

/*
 * RtlAtomicFetchOr64 - Atomic fetch-and-or (64-bit)
 * %edi: pointer to value
 * %rsi: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchOr64)
	mov	(%rdi),%rax
1:	mov	%rax,%rcx
	or	%rsi,%rcx
	lock cmpxchgq %rcx,(%rdi)
	jne	1b
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchOr64)

/*
 * RtlAtomicFetchAnd32 - Atomic fetch-and-and (32-bit)
 * %edi: pointer to value
 * %esi: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAnd32)
	mov	(%rdi),%eax
1:	mov	%eax,%ecx
	and	%esi,%ecx
	lock cmpxchgl %ecx,(%rdi)
	jne	1b
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchAnd32)

/*
 * RtlAtomicFetchAnd64 - Atomic fetch-and-and (64-bit)
 * %edi: pointer to value
 * %rsi: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAnd64)
	mov	(%rdi),%rax
1:	mov	%rax,%rcx
	and	%rsi,%rcx
	lock cmpxchgq %rcx,(%rdi)
	jne	1b
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchAnd64)

/*
 * RtlAtomicFetchXor32 - Atomic fetch-and-xor (32-bit)
 * %edi: pointer to value
 * %esi: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchXor32)
	mov	(%rdi),%eax
1:	mov	%eax,%ecx
	xor	%esi,%ecx
	lock cmpxchgl %ecx,(%rdi)
	jne	1b
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchXor32)

/*
 * RtlAtomicFetchXor64 - Atomic fetch-and-xor (64-bit)
 * %edi: pointer to value
 * %rsi: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchXor64)
	mov	(%rdi),%rax
1:	mov	%rax,%rcx
	xor	%rsi,%rcx
	lock cmpxchgq %rcx,(%rdi)
	jne	1b
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchXor64)

/*
 * RtlAtomicIncrement32 - Atomic increment (32-bit)
 * %edi: pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicIncrement32)
	mov	$1,%eax
	lock xaddl %eax,(%rdi)
	inc	%eax
	ret
ANX_ASM_FUNCTION_END(RtlAtomicIncrement32)

/*
 * RtlAtomicIncrement64 - Atomic increment (64-bit)
 * %edi: pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicIncrement64)
	mov	$1,%rax
	lock xaddq %rax,(%rdi)
	inc	%rax
	ret
ANX_ASM_FUNCTION_END(RtlAtomicIncrement64)

/*
 * RtlAtomicDecrement32 - Atomic decrement (32-bit)
 * %edi: pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicDecrement32)
	mov	$-1,%eax
	lock xaddl %eax,(%rdi)
	dec	%eax
	ret
ANX_ASM_FUNCTION_END(RtlAtomicDecrement32)

/*
 * RtlAtomicDecrement64 - Atomic decrement (64-bit)
 * %edi: pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicDecrement64)
	mov	$-1,%rax
	lock xaddq %rax,(%rdi)
	dec	%rax
	ret
ANX_ASM_FUNCTION_END(RtlAtomicDecrement64)

/*
 * Memory barriers / fences
 */

/*
 * RtlMemoryBarrier - Full memory barrier
 */
ANX_ASM_FUNCTION_BEGIN(RtlMemoryBarrier)
	mfence
	ret
ANX_ASM_FUNCTION_END(RtlMemoryBarrier)

/*
 * RtlLoadBarrier - Load (acquire) barrier
 */
ANX_ASM_FUNCTION_BEGIN(RtlLoadBarrier)
	lfence
	ret
ANX_ASM_FUNCTION_END(RtlLoadBarrier)

/*
 * RtlStoreBarrier - Store (release) barrier
 */
ANX_ASM_FUNCTION_BEGIN(RtlStoreBarrier)
	sfence
	ret
ANX_ASM_FUNCTION_END(RtlStoreBarrier)
