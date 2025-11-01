/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * x32 bit manipulation operations
 */

#include <ananke/asmdefs.h>

/*
 * RtlFindFirstSetBit - Find first set bit (1-indexed)
 * %edi: value
 * Returns: bit position (1-based), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindFirstSetBit)
	bsf	%edi,%eax
	jz	1f
	add	$1,%eax
	ret
1:	xor	%eax,%eax
	ret
ANX_ASM_FUNCTION_END(RtlFindFirstSetBit)

/*
 * RtlFindLastSetBit - Find last set bit (1-indexed from LSB)
 * %edi: value
 * Returns: bit position (1-based), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastSetBit)
	bsr	%edi,%eax
	jz	1f
	add	$1,%eax
	ret
1:	xor	%eax,%eax
	ret
ANX_ASM_FUNCTION_END(RtlFindLastSetBit)

/*
 * RtlCountSetBits - Count number of set bits (popcount)
 * %edi: value
 * Returns: number of set bits
 */
ANX_ASM_FUNCTION_BEGIN(RtlCountSetBits)
	xor	%eax,%eax
	test	%edi,%edi
	jz	2f
1:	mov	%edi,%ecx
	sub	$1,%ecx
	and	%ecx,%edi
	add	$1,%eax
	jnz	1b
2:	ret
ANX_ASM_FUNCTION_END(RtlCountSetBits)

/*
 * RtlByteSwap16 - Swap bytes in 16-bit value
 * %edi: value (in %di)
 * Returns: byte-swapped value
 */
ANX_ASM_FUNCTION_BEGIN(RtlByteSwap16)
	mov	%di,%ax
	xchg	%al,%ah
	ret
ANX_ASM_FUNCTION_END(RtlByteSwap16)

/*
 * RtlByteSwap32 - Swap bytes in 32-bit value
 * %edi: value
 * Returns: byte-swapped value
 */
ANX_ASM_FUNCTION_BEGIN(RtlByteSwap32)
	mov	%edi,%eax
	bswap	%eax
	ret
ANX_ASM_FUNCTION_END(RtlByteSwap32)

/*
 * RtlByteSwap64 - Swap bytes in 64-bit value
 * %rdi: value (64-bit even in x32)
 * Returns: byte-swapped value
 */
ANX_ASM_FUNCTION_BEGIN(RtlByteSwap64)
	mov	%rdi,%rax
	bswap	%rax
	ret
ANX_ASM_FUNCTION_END(RtlByteSwap64)
