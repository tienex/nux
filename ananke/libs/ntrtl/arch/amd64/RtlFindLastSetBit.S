/* @file
 * eCRT - An embedded C runtime library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <ananke/asmdefs.h>

ANX_ASM_FUNCTION_BEGIN(RtlFindLastSetBit)
	bsr	%rdi,%rax
	jz	1f
	inc	%rax
	ret

1:	xor	%rax, %rax
	ret
ANX_ASM_FUNCTION_END(RtlFindLastSetBit)
