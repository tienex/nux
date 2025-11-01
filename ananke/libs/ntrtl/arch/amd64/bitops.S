/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 optimized bit operations
 *
 * Uses BSF/BSR instructions for fast bit scanning
 */

#include <ananke/asmdefs.h>

/*
 * RtlFindFirstSetBit - Find first set bit (1-indexed)
 * %rdi: value
 * Returns: bit position (1-64), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindFirstSetBit)
	bsf	%rdi,%rax
	jz	1f	 		/* ZF is set if all bits are 0 */
	inc	%rax			/* bits numbered from 1, not 0 */
	ret

1:	xor	%rax, %rax		/* clear result */
	ret
ANX_ASM_FUNCTION_END(RtlFindFirstSetBit)

/*
 * RtlFindLastSetBit - Find last set bit (1-indexed)
 * %rdi: value
 * Returns: bit position (1-64), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastSetBit)
	bsr	%rdi,%rax
	jz	1f
	inc	%rax
	ret

1:	xor	%rax, %rax
	ret
ANX_ASM_FUNCTION_END(RtlFindLastSetBit)
