/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 atomic operations
 *
 * Provides lock-free atomic operations using x86 LOCK prefix
 * and specialized atomic instructions (CMPXCHG, XCHG, XADD, etc.)
 *
 * Calling convention: cdecl (arguments pushed right-to-left on stack)
 * - First argument at 4(%esp)
 * - Second argument at 8(%esp)
 * - Third argument at 12(%esp)
 * - Return value in %eax
 */

#include <ananke/asmdefs.h>

/*
 * RtlAtomicCompareExchange32 - Atomic compare-and-swap (32-bit)
 * 4(%esp): pointer to value
 * 8(%esp): expected value
 * 12(%esp): new value
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicCompareExchange32)
	movl	4(%esp),%edx		/* Load pointer into EDX */
	movl	8(%esp),%eax		/* Load expected into EAX */
	movl	12(%esp),%ecx		/* Load new value into ECX */
	lock cmpxchgl %ecx,(%edx)	/* Compare and exchange */
	ret				/* Previous value in EAX */
ANX_ASM_FUNCTION_END(RtlAtomicCompareExchange32)

/*
 * RtlAtomicExchange32 - Atomic exchange (32-bit)
 * 4(%esp): pointer to value
 * 8(%esp): new value
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicExchange32)
	movl	4(%esp),%edx
	movl	8(%esp),%eax
	xchgl	%eax,(%edx)		/* XCHG is implicitly locked */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicExchange32)

/*
 * RtlAtomicFetchAdd32 - Atomic fetch-and-add (32-bit)
 * 4(%esp): pointer to value
 * 8(%esp): addend
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAdd32)
	movl	4(%esp),%edx
	movl	8(%esp),%eax
	lock xaddl %eax,(%edx)		/* Exchange and add */
	ret				/* Previous value in EAX */
ANX_ASM_FUNCTION_END(RtlAtomicFetchAdd32)

/*
 * RtlAtomicFetchSub32 - Atomic fetch-and-sub (32-bit)
 * 4(%esp): pointer to value
 * 8(%esp): subtrahend
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchSub32)
	movl	8(%esp),%eax		/* Load subtrahend */
	negl	%eax			/* Negate for subtraction */
	movl	4(%esp),%edx		/* Load pointer */
	lock xaddl %eax,(%edx)
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchSub32)

/*
 * RtlAtomicFetchOr32 - Atomic fetch-and-or (32-bit)
 * 4(%esp): pointer to value
 * 8(%esp): operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchOr32)
	movl	4(%esp),%edx		/* Load pointer */
	movl	(%edx),%eax		/* Load current value */
1:	movl	%eax,%ecx		/* Save for comparison */
	orl	8(%esp),%ecx		/* Compute new value */
	lock cmpxchgl %ecx,(%edx)	/* Try to update */
	jne	1b			/* Retry if changed */
	ret				/* Return old value */
ANX_ASM_FUNCTION_END(RtlAtomicFetchOr32)

/*
 * RtlAtomicFetchAnd32 - Atomic fetch-and-and (32-bit)
 * 4(%esp): pointer to value
 * 8(%esp): operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchAnd32)
	movl	4(%esp),%edx
	movl	(%edx),%eax
1:	movl	%eax,%ecx
	andl	8(%esp),%ecx
	lock cmpxchgl %ecx,(%edx)
	jne	1b
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchAnd32)

/*
 * RtlAtomicFetchXor32 - Atomic fetch-and-xor (32-bit)
 * 4(%esp): pointer to value
 * 8(%esp): operand
 * Returns: previous value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicFetchXor32)
	movl	4(%esp),%edx
	movl	(%edx),%eax
1:	movl	%eax,%ecx
	xorl	8(%esp),%ecx
	lock cmpxchgl %ecx,(%edx)
	jne	1b
	ret
ANX_ASM_FUNCTION_END(RtlAtomicFetchXor32)

/*
 * RtlAtomicIncrement32 - Atomic increment (32-bit)
 * 4(%esp): pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicIncrement32)
	movl	4(%esp),%edx
	movl	$1,%eax
	lock xaddl %eax,(%edx)
	incl	%eax			/* Return new value */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicIncrement32)

/*
 * RtlAtomicDecrement32 - Atomic decrement (32-bit)
 * 4(%esp): pointer to value
 * Returns: new value
 */
ANX_ASM_FUNCTION_BEGIN(RtlAtomicDecrement32)
	movl	4(%esp),%edx
	movl	$-1,%eax
	lock xaddl %eax,(%edx)
	decl	%eax			/* Return new value */
	ret
ANX_ASM_FUNCTION_END(RtlAtomicDecrement32)

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
