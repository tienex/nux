/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V64 Zba-optimized string operations
 *
 * Requires: Zba (Address Generation) extension
 *
 * Zba instructions for efficient pointer arithmetic:
 * - sh1add: (rs1 << 1) + rs2 (for 16-bit indexing)
 * - sh2add: (rs1 << 2) + rs2 (for 32-bit indexing)
 * - sh3add: (rs1 << 3) + rs2 (for 64-bit indexing)
 * - add.uw: (zero-extend rs1 to 64-bit) + rs2
 * - slli.uw: (zero-extend rs1 to 64-bit) << shamt
 *
 * These are particularly useful for array indexing and string operations
 * with 16-bit characters (CHAR16).
 */

#include <ananke/asmdefs.h>

/*
 * RtlFindChar16_Zba - Find 16-bit character using Zba for indexing
 * a0: string pointer (CHAR16*)
 * a1: character to find (low 16 bits)
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar16_Zba)
	mv	t0,a0			/* Save start pointer */
	andi	a1,a1,0xFFFF		/* Ensure 16-bit value */

1:	lh	t1,0(a0)
	beq	t1,a1,3f		/* Found */
	beqz	t1,2f			/* End of string */

	/* Advance by 2 bytes using simple add */
	addi	a0,a0,2
	j	1b

2:	/* Not found - check if looking for null */
	bnez	a1,4f
3:	ret

4:	li	a0,0			/* Return NULL */
	ret
ANX_ASM_FUNCTION_END(RtlFindChar16_Zba)

/*
 * RtlCompareChars16_Zba - Compare 16-bit strings with Zba indexing
 * a0: string1 (CHAR16*)
 * a1: string2 (CHAR16*)
 * a2: max length (in characters)
 * Returns: difference of first differing chars
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars16_Zba)
	beqz	a2,3f			/* Equal if length 0 */

	li	t0,0			/* Index = 0 */

1:	/* Load chars using index */
	sh1add	t1,t0,a0		/* t1 = a0 + (t0 << 1) (Zba) */
	sh1add	t2,t0,a1		/* t2 = a1 + (t0 << 1) (Zba) */

	lh	t3,0(t1)		/* char from string1 */
	lh	t4,0(t2)		/* char from string2 */

	bne	t3,t4,2f		/* Characters differ */
	beqz	t3,3f			/* Both null, equal */

	addi	t0,t0,1			/* index++ */
	blt	t0,a2,1b		/* Continue if index < length */

3:	/* Equal */
	li	a0,0
	ret

2:	/* Different */
	sub	a0,t3,t4		/* Return difference */
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars16_Zba)

/*
 * RtlGetChar16At_Zba - Get character at index using Zba
 * a0: string pointer (CHAR16*)
 * a1: index
 * Returns: character at index
 */
ANX_ASM_FUNCTION_BEGIN(RtlGetChar16At_Zba)
	sh1add	a0,a1,a0		/* a0 = base + (index << 1) (Zba) */
	lh	a0,0(a0)
	ret
ANX_ASM_FUNCTION_END(RtlGetChar16At_Zba)

/*
 * RtlSetChar16At_Zba - Set character at index using Zba
 * a0: string pointer (CHAR16*)
 * a1: index
 * a2: character value
 * Returns: string pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlSetChar16At_Zba)
	mv	t0,a0			/* Save pointer */
	sh1add	a0,a1,a0		/* a0 = base + (index << 1) (Zba) */
	sh	a2,0(a0)
	mv	a0,t0			/* Restore pointer */
	ret
ANX_ASM_FUNCTION_END(RtlSetChar16At_Zba)

/*
 * RtlCopyChars16_Zba - Copy N 16-bit characters using Zba
 * a0: destination (CHAR16*)
 * a1: source (CHAR16*)
 * a2: count (in characters)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyChars16_Zba)
	mv	t6,a0			/* Save destination */
	beqz	a2,3f			/* Nothing to copy */

	li	t0,0			/* Index = 0 */

1:	sh1add	t1,t0,a1		/* t1 = source + (index << 1) (Zba) */
	sh1add	t2,t0,a0		/* t2 = dest + (index << 1) (Zba) */

	lh	t3,0(t1)
	sh	t3,0(t2)

	addi	t0,t0,1			/* index++ */
	blt	t0,a2,1b		/* Continue if more */

3:	mv	a0,t6			/* Restore destination */
	ret
ANX_ASM_FUNCTION_END(RtlCopyChars16_Zba)

/*
 * RtlFillChars16_Zba - Fill N 16-bit characters using Zba
 * a0: buffer (CHAR16*)
 * a1: count (in characters)
 * a2: character value
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillChars16_Zba)
	mv	t6,a0			/* Save buffer */
	beqz	a1,3f

	li	t0,0			/* Index = 0 */

1:	sh1add	t1,t0,a0		/* t1 = buffer + (index << 1) (Zba) */
	sh	a2,0(t1)

	addi	t0,t0,1
	blt	t0,a1,1b

3:	mv	a0,t6
	ret
ANX_ASM_FUNCTION_END(RtlFillChars16_Zba)

/*
 * RtlIndexOf16_Zba - Find index of character in string
 * a0: string pointer (CHAR16*)
 * a1: character to find
 * a2: max length
 * Returns: index of character, or -1 if not found
 */
ANX_ASM_FUNCTION_BEGIN(RtlIndexOf16_Zba)
	andi	a1,a1,0xFFFF
	beqz	a2,2f			/* Not found if length 0 */

	li	t0,0			/* Index = 0 */

1:	sh1add	t1,t0,a0		/* t1 = string + (index << 1) (Zba) */
	lh	t2,0(t1)

	beq	t2,a1,3f		/* Found */
	beqz	t2,2f			/* End of string */

	addi	t0,t0,1
	blt	t0,a2,1b

2:	/* Not found */
	li	a0,-1
	ret

3:	/* Found */
	mv	a0,t0			/* Return index */
	ret
ANX_ASM_FUNCTION_END(RtlIndexOf16_Zba)

/*
 * RtlLastIndexOf16_Zba - Find last index of character in string
 * a0: string pointer (CHAR16*)
 * a1: character to find
 * a2: max length
 * Returns: last index of character, or -1 if not found
 */
ANX_ASM_FUNCTION_BEGIN(RtlLastIndexOf16_Zba)
	andi	a1,a1,0xFFFF
	beqz	a2,2f

	li	t0,0			/* Index = 0 */
	li	t3,-1			/* Last match = -1 */

1:	sh1add	t1,t0,a0		/* t1 = string + (index << 1) (Zba) */
	lh	t2,0(t1)

	beqz	t2,3f			/* End of string */
	bne	t2,a1,4f		/* Not a match */
	mv	t3,t0			/* Update last match */

4:	addi	t0,t0,1
	blt	t0,a2,1b

3:	mv	a0,t3			/* Return last match index */
	ret

2:	li	a0,-1
	ret
ANX_ASM_FUNCTION_END(RtlLastIndexOf16_Zba)
