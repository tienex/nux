/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V64 optimized string operations
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength - Get string length
 * a0: string pointer
 * Returns: length
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength)
	mv	t0, a0		/* Save original pointer */

1:	lb	t1, 0(a0)
	beqz	t1, 2f		/* Found null terminator */
	addi	a0, a0, 1
	j	1b

2:	sub	a0, a0, t0	/* Calculate length */
	ret
ANX_ASM_FUNCTION_END(RtlStringLength)

/*
 * RtlFindChar - Find character in string
 * a0: string pointer
 * a1: character to find
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar)
	andi	a1, a1, 0xff	/* Ensure byte value */

1:	lb	t0, 0(a0)
	beq	t0, a1, 3f	/* Found (including null) */
	beqz	t0, 2f		/* End of string */
	addi	a0, a0, 1
	j	1b

2:	/* Not found - check if looking for null */
	bnez	a1, 4f
3:	ret			/* Found or null match */

4:	li	a0, 0		/* Not found, return NULL */
	ret
ANX_ASM_FUNCTION_END(RtlFindChar)

/*
 * RtlFindLastChar - Find last occurrence of character
 * a0: string pointer
 * a1: character to find
 * Returns: pointer to last occurrence or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar)
	mv	t2, a0		/* Current position */
	li	t3, 0		/* Last match */
	andi	a1, a1, 0xff

1:	lb	t0, 0(t2)
	bne	t0, a1, 2f
	mv	t3, t2		/* Update last match */

2:	beqz	t0, 3f		/* End of string */
	addi	t2, t2, 1
	j	1b

3:	/* Check if we found it or looking for null */
	bnez	t3, 4f
	bnez	a1, 5f
	mv	a0, t2		/* Return pointer to null */
	ret

4:	mv	a0, t3		/* Return last match */
	ret

5:	li	a0, 0		/* Not found */
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar)

/*
 * RtlCompareChars - Compare strings (up to N chars)
 * a0: string1
 * a1: string2
 * a2: max length
 * Returns: difference of first differing chars, or 0 if equal
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars)
1:	beqz	a2, 3f		/* Reached length limit */
	lb	t0, 0(a0)
	lb	t1, 0(a1)
	bne	t0, t1, 2f	/* Characters differ */
	beqz	t0, 3f		/* Both null, equal */
	addi	a0, a0, 1
	addi	a1, a1, 1
	addi	a2, a2, -1
	j	1b

2:	sub	a0, t0, t1	/* Return difference */
	ret

3:	li	a0, 0		/* Equal */
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars)
