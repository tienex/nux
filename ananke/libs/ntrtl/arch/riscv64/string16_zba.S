/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V64 Zba-optimized 16-bit character string operations
 *
 * Requires: Zba (Address Generation) extension
 *
 * Zba provides sh1add for efficient CHAR16 pointer arithmetic:
 * sh1add rd, rs1, rs2 = rd = (rs1 << 1) + rs2
 *
 * This is perfect for CHAR16 array indexing where each element is 2 bytes.
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength16_Zba - Calculate 16-bit string length
 * a0: string pointer (CHAR16*)
 * Returns: length in characters (not bytes)
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength16_Zba)
	mv	t0,a0			/* Save original */
	li	t1,0			/* Count = 0 */

1:	sh1add	t2,t1,a0		/* t2 = a0 + (t1 << 1) */
	lh	t3,0(t2)
	beqz	t3,2f			/* Found null */
	addi	t1,t1,1
	j	1b

2:	mv	a0,t1			/* Return count */
	ret
ANX_ASM_FUNCTION_END(RtlStringLength16_Zba)

/*
 * RtlFindChar16_Zba - Find 16-bit character using Zba indexing
 * a0: string pointer (CHAR16*)
 * a1: character to find (low 16 bits)
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar16_Zba)
	andi	a1,a1,0xFFFF		/* Ensure 16-bit */
	li	t0,0			/* Index = 0 */

1:	sh1add	t1,t0,a0		/* t1 = a0 + (index << 1) */
	lh	t2,0(t1)
	beq	t2,a1,3f		/* Found */
	beqz	t2,2f			/* End of string */
	addi	t0,t0,1
	j	1b

2:	/* Not found - check if looking for null */
	bnez	a1,4f
3:	sh1add	a0,t0,a0		/* Return pointer */
	ret

4:	li	a0,0			/* Return NULL */
	ret
ANX_ASM_FUNCTION_END(RtlFindChar16_Zba)

/*
 * RtlFindLastChar16_Zba - Find last occurrence of 16-bit character
 * a0: string pointer (CHAR16*)
 * a1: character to find (low 16 bits)
 * Returns: pointer to last occurrence or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar16_Zba)
	mv	t6,a0			/* Save string pointer */
	andi	a1,a1,0xFFFF
	li	t0,0			/* Index = 0 */
	li	t3,-1			/* Last match = -1 */

1:	sh1add	t1,t0,a0		/* t1 = a0 + (index << 1) */
	lh	t2,0(t1)
	beqz	t2,3f			/* End of string */
	bne	t2,a1,2f		/* Not a match */
	mv	t3,t0			/* Update last match */

2:	addi	t0,t0,1
	j	1b

3:	/* Check if we found it or looking for null */
	bgez	t3,4f			/* Found at least once */
	bnez	a1,5f
	sh1add	a0,t0,t6		/* Return pointer to null */
	ret

4:	sh1add	a0,t3,t6		/* Return pointer to last match */
	ret

5:	li	a0,0			/* Not found */
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar16_Zba)

/*
 * RtlCompareChars16_Zba - Compare 16-bit strings up to N characters
 * a0: string1 (CHAR16*)
 * a1: string2 (CHAR16*)
 * a2: max length (in characters)
 * Returns: difference of first differing chars, or 0 if equal
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars16_Zba)
	beqz	a2,3f			/* Equal if length 0 */
	li	t0,0			/* Index = 0 */

1:	sh1add	t1,t0,a0		/* t1 = string1 + (index << 1) */
	sh1add	t2,t0,a1		/* t2 = string2 + (index << 1) */
	lh	t3,0(t1)
	lh	t4,0(t2)

	bne	t3,t4,2f		/* Characters differ */
	beqz	t3,3f			/* Both null, equal */

	addi	t0,t0,1
	blt	t0,a2,1b		/* Continue if index < length */

3:	li	a0,0			/* Equal */
	ret

2:	sub	a0,t3,t4		/* Return difference */
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars16_Zba)

/*
 * RtlCopyString16_Zba - Copy 16-bit string
 * a0: destination (CHAR16*)
 * a1: source (CHAR16*)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyString16_Zba)
	mv	t6,a0			/* Save destination */
	li	t0,0			/* Index = 0 */

1:	sh1add	t1,t0,a1		/* t1 = source + (index << 1) */
	sh1add	t2,t0,a0		/* t2 = dest + (index << 1) */
	lh	t3,0(t1)
	sh	t3,0(t2)
	beqz	t3,2f			/* Copied null terminator */
	addi	t0,t0,1
	j	1b

2:	mv	a0,t6			/* Restore destination */
	ret
ANX_ASM_FUNCTION_END(RtlCopyString16_Zba)

/*
 * RtlCopyChars16_Zba - Copy N 16-bit characters
 * a0: destination (CHAR16*)
 * a1: source (CHAR16*)
 * a2: count (in characters)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyChars16_Zba)
	mv	t6,a0			/* Save destination */
	beqz	a2,3f			/* Nothing to copy */
	li	t0,0			/* Index = 0 */

1:	sh1add	t1,t0,a1		/* t1 = source + (index << 1) */
	sh1add	t2,t0,a0		/* t2 = dest + (index << 1) */
	lh	t3,0(t1)
	sh	t3,0(t2)

	addi	t0,t0,1
	blt	t0,a2,1b		/* Continue if more */

3:	mv	a0,t6			/* Restore destination */
	ret
ANX_ASM_FUNCTION_END(RtlCopyChars16_Zba)

/*
 * RtlFillChars16_Zba - Fill N 16-bit characters
 * a0: buffer (CHAR16*)
 * a1: count (in characters)
 * a2: character value
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillChars16_Zba)
	mv	t6,a0			/* Save buffer */
	beqz	a1,3f
	li	t0,0			/* Index = 0 */

1:	sh1add	t1,t0,a0		/* t1 = buffer + (index << 1) */
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

1:	sh1add	t1,t0,a0		/* t1 = string + (index << 1) */
	lh	t2,0(t1)

	beq	t2,a1,3f		/* Found */
	beqz	t2,2f			/* End of string */

	addi	t0,t0,1
	blt	t0,a2,1b

2:	li	a0,-1			/* Not found */
	ret

3:	mv	a0,t0			/* Return index */
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

1:	sh1add	t1,t0,a0		/* t1 = string + (index << 1) */
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

/*
 * RtlGetChar16At_Zba - Get character at index using Zba
 * a0: string pointer (CHAR16*)
 * a1: index
 * Returns: character at index
 */
ANX_ASM_FUNCTION_BEGIN(RtlGetChar16At_Zba)
	sh1add	a0,a1,a0		/* a0 = base + (index << 1) */
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
	sh1add	a0,a1,a0		/* a0 = base + (index << 1) */
	sh	a2,0(a0)
	mv	a0,t0			/* Restore pointer */
	ret
ANX_ASM_FUNCTION_END(RtlSetChar16At_Zba)
