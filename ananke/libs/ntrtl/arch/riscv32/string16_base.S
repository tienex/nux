/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V32 string operations (CHAR16)
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength16 - Get wide string length
 * a0: string pointer
 * Returns: length in CHAR16 units (not including null terminator)
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength16)
	mv	t0, a0
	li	a0, 0

1:	lh	t1, 0(t0)
	beqz	t1, 2f
	addi	t0, t0, 2
	addi	a0, a0, 1
	j	1b

2:	ret
ANX_ASM_FUNCTION_END(RtlStringLength16)

/*
 * RtlFindChar16 - Find wide character in string
 * a0: string pointer
 * a1: character to find
 * Returns: pointer to first occurrence, or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar16)
1:	lh	t0, 0(a0)
	beqz	t0, 2f		/* End of string */
	beq	t0, a1, 3f	/* Found */
	addi	a0, a0, 2
	j	1b

2:	li	a0, 0		/* Not found */
3:	ret
ANX_ASM_FUNCTION_END(RtlFindChar16)

/*
 * RtlCompareChars16 - Compare two wide strings
 * a0: string1
 * a1: string2
 * Returns: 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars16)
1:	lh	t0, 0(a0)
	lh	t1, 0(a1)
	bne	t0, t1, 2f
	beqz	t0, 3f		/* Both null, equal */
	addi	a0, a0, 2
	addi	a1, a1, 2
	j	1b

2:	sub	a0, t0, t1
	ret

3:	li	a0, 0
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars16)

/*
 * RtlCopyChars16 - Copy wide string
 * a0: destination
 * a1: source
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyChars16)
	mv	t6, a0		/* Save return value */

1:	lh	t0, 0(a1)
	sh	t0, 0(a0)
	beqz	t0, 2f
	addi	a0, a0, 2
	addi	a1, a1, 2
	j	1b

2:	mv	a0, t6
	ret
ANX_ASM_FUNCTION_END(RtlCopyChars16)
