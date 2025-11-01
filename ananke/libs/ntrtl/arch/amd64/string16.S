/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 optimized 16-bit character string operations
 *
 * Adapted from 8-bit string operations for CHAR16/Unicode strings
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength16 - Calculate 16-bit string length
 * %rdi: string pointer (CHAR16*)
 * Returns: length in characters (not bytes)
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength16)
	movq	%rdi,%rax		/* Save original pointer */

1:	movw	(%rdi),%cx
	testw	%cx,%cx
	jz	2f			/* Found null terminator */
	addq	$2,%rdi
	jmp	1b

2:	subq	%rax,%rdi
	shrq	$1,%rdi			/* Convert bytes to characters */
	movq	%rdi,%rax
	ret
ANX_ASM_FUNCTION_END(RtlStringLength16)

/*
 * RtlFindChar16 - Find 16-bit character in string
 * %rdi: string pointer (CHAR16*)
 * %si: character to find (low 16 bits of %rsi)
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar16)
1:	movw	(%rdi),%cx
	cmpw	%si,%cx
	je	3f			/* Found character */
	testw	%cx,%cx
	jz	2f			/* End of string */
	addq	$2,%rdi
	jmp	1b

2:	/* Not found - check if looking for null */
	testw	%si,%si
	jnz	4f
3:	movq	%rdi,%rax		/* Found or null match */
	ret

4:	xorq	%rax,%rax		/* Not found, return NULL */
	ret
ANX_ASM_FUNCTION_END(RtlFindChar16)

/*
 * RtlFindLastChar16 - Find last occurrence of 16-bit character
 * %rdi: string pointer (CHAR16*)
 * %si: character to find (low 16 bits of %rsi)
 * Returns: pointer to last occurrence or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar16)
	movq	%rdi,%r8		/* Current position */
	xorq	%rax,%rax		/* Last match */

1:	movw	(%r8),%cx
	cmpw	%si,%cx
	jne	2f
	movq	%r8,%rax		/* Update last match */

2:	testw	%cx,%cx
	jz	3f			/* End of string */
	addq	$2,%r8
	jmp	1b

3:	/* Check if we found it or looking for null */
	testq	%rax,%rax
	jnz	4f
	testw	%si,%si
	jnz	5f
	movq	%r8,%rax		/* Return pointer to null */
	ret

4:	ret				/* Return last match */

5:	xorq	%rax,%rax		/* Not found */
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar16)

/*
 * RtlCompareChars16 - Compare 16-bit strings up to N characters
 * %rdi: string1 (CHAR16*)
 * %rsi: string2 (CHAR16*)
 * %rdx: max length (in characters)
 * Returns: difference of first differing chars, or 0 if equal
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars16)
	testq	%rdx,%rdx
	jmp	L2_16			/* Jump into the loop */

L1_16:	addq	$2,%rdi
	addq	$2,%rsi
	decq	%rdx
L2_16:	jz	L4_16			/* Strings are equal */
	movw	(%rdi),%ax
	testw	%ax,%ax
	jz	L3_16
	cmpw	(%rsi),%ax
	jne	L3_16

	addq	$2,%rdi
	addq	$2,%rsi
	decq	%rdx
	jz	L4_16
	movw	(%rdi),%ax
	testw	%ax,%ax
	jz	L3_16
	cmpw	(%rsi),%ax
	jne	L3_16

	addq	$2,%rdi
	addq	$2,%rsi
	decq	%rdx
	jz	L4_16
	movw	(%rdi),%ax
	testw	%ax,%ax
	jz	L3_16
	cmpw	(%rsi),%ax
	jne	L3_16

	addq	$2,%rdi
	addq	$2,%rsi
	decq	%rdx
	jz	L4_16
	movw	(%rdi),%ax
	testw	%ax,%ax
	jz	L3_16
	cmpw	(%rsi),%ax
	je	L1_16

L3_16:	movzwl	(%rdi),%eax		/* Unsigned comparison */
	movzwl	(%rsi),%ecx
	subl	%ecx,%eax
	ret
L4_16:	xorl	%eax,%eax
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars16)

/*
 * RtlCopyString16 - Copy 16-bit string
 * %rdi: destination (CHAR16*)
 * %rsi: source (CHAR16*)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyString16)
	movq	%rdi,%rax		/* Save destination */

1:	movw	(%rsi),%cx
	movw	%cx,(%rdi)
	testw	%cx,%cx
	jz	2f			/* Copied null terminator */
	addq	$2,%rdi
	addq	$2,%rsi
	jmp	1b

2:	ret
ANX_ASM_FUNCTION_END(RtlCopyString16)
