/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 atomic operations
 *
 * Provides lock-free atomic operations using x86-64 LOCK prefix
 * and specialized atomic instructions (CMPXCHG, XCHG, XADD, etc.)
 */

#include <ananke/asmdefs.h>

/*
 * RtlAtomicCompareExchange32 - Atomic compare-and-swap (32-bit)
 * %rdi: pointer to value
 * %esi: expected value
 * %edx: new value
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicCompareExchange32)
	movl	%esi,%eax		/* Load expected into EAX */
	lock cmpxchgl %edx,(%rdi)	/* Compare and exchange */
	ret				/* Previous value in EAX */
ANX_ASM_FUNCTION_END(RtlAtomicCompareExchange32)

/*
 * RtlAtomicCompareExchange64 - Atomic compare-and-swap (64-bit)
 * %rdi: pointer to value
 * %rsi: expected value
 * %rdx: new value
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicCompareExchange64)
	movq	%rsi,%rax		/* Load expected into RAX */
	lock cmpxchgq %rdx,(%rdi)	/* Compare and exchange */
	ret				/* Previous value in RAX */
ANX_ASM_FUNCTION_END(RtlAtomicCompareExchange64)

/*
 * RtlAtomicExchange32 - Atomic exchange (32-bit)
 * %rdi: pointer to value
 * %esi: new value
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicExchange32)
	movl	%esi,%eax
	xchgl	%eax,(%rdi)		/* XCHG is implicitly locked */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicExchange32)

/*
 * RtlAtomicExchange64 - Atomic exchange (64-bit)
 * %rdi: pointer to value
 * %rsi: new value
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicExchange64)
	movq	%rsi,%rax
	xchgq	%rax,(%rdi)		/* XCHG is implicitly locked */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicExchange64)

/*
 * RtlAtomicFetchAdd32 - Atomic fetch-and-add (32-bit)
 * %rdi: pointer to value
 * %esi: addend
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAdd32)
	movl	%esi,%eax
	lock xaddl %eax,(%rdi)		/* Exchange and add */
	ret				/* Previous value in EAX */
ANX_ASM_FUNCTION_END(RtlAtomicFetchAdd32)

/*
 * RtlAtomicFetchAdd64 - Atomic fetch-and-add (64-bit)
 * %rdi: pointer to value
 * %rsi: addend
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAdd64)
	movq	%rsi,%rax
	lock xaddq %rax,(%rdi)		/* Exchange and add */
	ret				/* Previous value in RAX */
ANX_ASM_FUNCTION_END(RtlAtomicFetchAdd64)

/*
 * RtlAtomicFetchSub32 - Atomic fetch-and-sub (32-bit)
 * %rdi: pointer to value
 * %esi: subtrahend
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchSub32)
	negl	%esi			/* Negate for subtraction */
	movl	%esi,%eax
	lock xaddl %eax,(%rdi)
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchSub32)

/*
 * RtlAtomicFetchSub64 - Atomic fetch-and-sub (64-bit)
 * %rdi: pointer to value
 * %rsi: subtrahend
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchSub64)
	negq	%rsi			/* Negate for subtraction */
	movq	%rsi,%rax
	lock xaddq %rax,(%rdi)
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchSub64)

/*
 * RtlAtomicFetchOr32 - Atomic fetch-and-or (32-bit)
 * %rdi: pointer to value
 * %esi: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchOr32)
	movl	(%rdi),%eax		/* Load current value */
1:	movl	%eax,%ecx		/* Save for comparison */
	orl	%esi,%ecx		/* Compute new value */
	lock cmpxchgl %ecx,(%rdi)	/* Try to update */
	jne	1b			/* Retry if changed */
	ret				/* Return old value */
ANX_ASM_FUNCTION_END(RtlAtomicFetchOr32)

/*
 * RtlAtomicFetchOr64 - Atomic fetch-and-or (64-bit)
 * %rdi: pointer to value
 * %rsi: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchOr64)
	movq	(%rdi),%rax
1:	movq	%rax,%rcx
	orq	%rsi,%rcx
	lock cmpxchgq %rcx,(%rdi)
	jne	1b
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchOr64)

/*
 * RtlAtomicFetchAnd32 - Atomic fetch-and-and (32-bit)
 * %rdi: pointer to value
 * %esi: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAnd32)
	movl	(%rdi),%eax
1:	movl	%eax,%ecx
	andl	%esi,%ecx
	lock cmpxchgl %ecx,(%rdi)
	jne	1b
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchAnd32)

/*
 * RtlAtomicFetchAnd64 - Atomic fetch-and-and (64-bit)
 * %rdi: pointer to value
 * %rsi: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAnd64)
	movq	(%rdi),%rax
1:	movq	%rax,%rcx
	andq	%rsi,%rcx
	lock cmpxchgq %rcx,(%rdi)
	jne	1b
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchAnd64)

/*
 * RtlAtomicFetchXor32 - Atomic fetch-and-xor (32-bit)
 * %rdi: pointer to value
 * %esi: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchXor32)
	movl	(%rdi),%eax
1:	movl	%eax,%ecx
	xorl	%esi,%ecx
	lock cmpxchgl %ecx,(%rdi)
	jne	1b
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchXor32)

/*
 * RtlAtomicFetchXor64 - Atomic fetch-and-xor (64-bit)
 * %rdi: pointer to value
 * %rsi: operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchXor64)
	movq	(%rdi),%rax
1:	movq	%rax,%rcx
	xorq	%rsi,%rcx
	lock cmpxchgq %rcx,(%rdi)
	jne	1b
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchXor64)

/*
 * RtlAtomicIncrement32 - Atomic increment (32-bit)
 * %rdi: pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicIncrement32)
	movl	$1,%eax
	lock xaddl %eax,(%rdi)
	incl	%eax			/* Return new value */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicIncrement32)

/*
 * RtlAtomicIncrement64 - Atomic increment (64-bit)
 * %rdi: pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicIncrement64)
	movq	$1,%rax
	lock xaddq %rax,(%rdi)
	incq	%rax			/* Return new value */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicIncrement64)

/*
 * RtlAtomicDecrement32 - Atomic decrement (32-bit)
 * %rdi: pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicDecrement32)
	movl	$-1,%eax
	lock xaddl %eax,(%rdi)
	decl	%eax			/* Return new value */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicDecrement32)

/*
 * RtlAtomicDecrement64 - Atomic decrement (64-bit)
 * %rdi: pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicDecrement64)
	movq	$-1,%rax
	lock xaddq %rax,(%rdi)
	decq	%rax			/* Return new value */
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
