/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V64 optimized 16-bit character string operations
 *
 * Adapted from 8-bit string operations for CHAR16/Unicode strings
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength16 - Calculate 16-bit string length
 * a0: string pointer (CHAR16*)
 * Returns: length in characters (not bytes)
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength16)
	mv	t0, a0			/* Save original pointer */

1:	lh	t1, 0(a0)
	beqz	t1, 2f			/* Found null terminator */
	addi	a0, a0, 2
	j	1b

2:	sub	a0, a0, t0		/* Length in bytes */
	srli	a0, a0, 1		/* Convert to characters */
	ret
ANX_ASM_FUNCTION_END(RtlStringLength16)

/*
 * RtlFindChar16 - Find 16-bit character in string
 * a0: string pointer (CHAR16*)
 * a1: character to find (low 16 bits)
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar16)
	andi	a1, a1, 0xFFFF		/* Ensure 16-bit value */

1:	lh	t0, 0(a0)
	beq	t0, a1, 3f		/* Found character */
	beqz	t0, 2f			/* End of string */
	addi	a0, a0, 2
	j	1b

2:	/* Not found - check if looking for null */
	bnez	a1, 4f
3:	ret				/* Found or null match */

4:	li	a0, 0			/* Not found, return NULL */
	ret
ANX_ASM_FUNCTION_END(RtlFindChar16)

/*
 * RtlFindLastChar16 - Find last occurrence of 16-bit character
 * a0: string pointer (CHAR16*)
 * a1: character to find (low 16 bits)
 * Returns: pointer to last occurrence or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar16)
	mv	t2, a0			/* Current position */
	li	t3, 0			/* Last match */
	andi	a1, a1, 0xFFFF

1:	lh	t0, 0(t2)
	bne	t0, a1, 2f
	mv	t3, t2			/* Update last match */

2:	beqz	t0, 3f			/* End of string */
	addi	t2, t2, 2
	j	1b

3:	/* Check if we found it or looking for null */
	bnez	t3, 4f
	bnez	a1, 5f
	mv	a0, t2			/* Return pointer to null */
	ret

4:	mv	a0, t3			/* Return last match */
	ret

5:	li	a0, 0			/* Not found */
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar16)

/*
 * RtlCompareChars16 - Compare 16-bit strings up to N characters
 * a0: string1 (CHAR16*)
 * a1: string2 (CHAR16*)
 * a2: max length (in characters)
 * Returns: difference of first differing chars, or 0 if equal
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars16)
1:	beqz	a2, 3f			/* Reached length limit */
	lh	t0, 0(a0)
	lh	t1, 0(a1)
	bne	t0, t1, 2f		/* Characters differ */
	beqz	t0, 3f			/* Both null, equal */
	addi	a0, a0, 2
	addi	a1, a1, 2
	addi	a2, a2, -1
	j	1b

2:	sub	a0, t0, t1		/* Return difference */
	ret

3:	li	a0, 0			/* Equal */
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars16)

/*
 * RtlCopyString16 - Copy 16-bit string
 * a0: destination (CHAR16*)
 * a1: source (CHAR16*)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyString16)
	mv	t0, a0			/* Save destination */

1:	lh	t1, 0(a1)
	sh	t1, 0(a0)
	beqz	t1, 2f			/* Copied null terminator */
	addi	a0, a0, 2
	addi	a1, a1, 2
	j	1b

2:	mv	a0, t0			/* Restore destination */
	ret
ANX_ASM_FUNCTION_END(RtlCopyString16)
