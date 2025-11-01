/* @file
 * eCRT - An embedded C runtime library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <ananke/asmdefs.h>

ENTRY (fls)
	bsr	%rdi,%rax
	jz	1f
	inc	%rax
	ret

1:	xor	%rax, %rax
	ret
END (fls)
