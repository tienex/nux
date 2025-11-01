/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 optimized 16-bit character string operations
 *
 * Adapted from 8-bit string operations for CHAR16/Unicode strings
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength16 - Calculate 16-bit string length
 * 4(%esp): string pointer (CHAR16*)
 * Returns: length in characters (not bytes)
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength16)
	movl	4(%esp),%eax		/* String pointer */
	movl	%eax,%edx		/* Save original */

1:	movw	(%eax),%cx
	testw	%cx,%cx
	jz	2f			/* Found null terminator */
	addl	$2,%eax
	jmp	1b

2:	subl	%edx,%eax		/* Length in bytes */
	shrl	$1,%eax			/* Convert to characters */
	ret
ANX_ASM_FUNCTION_END(RtlStringLength16)

/*
 * RtlFindChar16 - Find 16-bit character in string
 * 4(%esp): string pointer (CHAR16*)
 * 8(%esp): character to find
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar16)
	movl	4(%esp),%eax		/* String pointer */
	movw	8(%esp),%dx		/* Character to find */

1:	movw	(%eax),%cx
	cmpw	%dx,%cx
	je	3f			/* Found character */
	testw	%cx,%cx
	jz	2f			/* End of string */
	addl	$2,%eax
	jmp	1b

2:	/* Not found - check if looking for null */
	testw	%dx,%dx
	jnz	4f
3:	ret				/* Found or null match */

4:	xorl	%eax,%eax		/* Not found, return NULL */
	ret
ANX_ASM_FUNCTION_END(RtlFindChar16)

/*
 * RtlFindLastChar16 - Find last occurrence of 16-bit character
 * 4(%esp): string pointer (CHAR16*)
 * 8(%esp): character to find
 * Returns: pointer to last occurrence or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastChar16)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%esi		/* String pointer (after push) */
	movw	16(%esp),%dx		/* Character to find (after push) */
	xorl	%edi,%edi		/* Last match */

1:	movw	(%esi),%cx
	cmpw	%dx,%cx
	jne	2f
	movl	%esi,%edi		/* Update last match */

2:	testw	%cx,%cx
	jz	3f			/* End of string */
	addl	$2,%esi
	jmp	1b

3:	/* Check if we found it or looking for null */
	testl	%edi,%edi
	jnz	4f
	testw	%dx,%dx
	jnz	5f
	movl	%esi,%eax		/* Return pointer to null */
	popl	%edi
	popl	%esi
	ret

4:	movl	%edi,%eax		/* Return last match */
	popl	%edi
	popl	%esi
	ret

5:	xorl	%eax,%eax		/* Not found */
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlFindLastChar16)

/*
 * RtlCompareChars16 - Compare 16-bit strings up to N characters
 * 4(%esp): string1 (CHAR16*)
 * 8(%esp): string2 (CHAR16*)
 * 12(%esp): max length (in characters)
 * Returns: difference of first differing chars, or 0 if equal
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars16)
	pushl	%esi
	movl	8(%esp),%eax		/* String1 (after push) */
	movl	12(%esp),%esi		/* String2 (after push) */
	movl	16(%esp),%edx		/* Max length (after push) */
	testl	%edx,%edx
	jmp	L2_16			/* Jump into the loop */

L1_16:	addl	$2,%eax
	addl	$2,%esi
	decl	%edx
L2_16:	jz	L4_16			/* Strings are equal */
	movw	(%eax),%cx
	testw	%cx,%cx
	jz	L3_16
	cmpw	(%esi),%cx
	jne	L3_16

	addl	$2,%eax
	addl	$2,%esi
	decl	%edx
	jz	L4_16
	movw	(%eax),%cx
	testw	%cx,%cx
	jz	L3_16
	cmpw	(%esi),%cx
	jne	L3_16

	addl	$2,%eax
	addl	$2,%esi
	decl	%edx
	jz	L4_16
	movw	(%eax),%cx
	testw	%cx,%cx
	jz	L3_16
	cmpw	(%esi),%cx
	jne	L3_16

	addl	$2,%eax
	addl	$2,%esi
	decl	%edx
	jz	L4_16
	movw	(%eax),%cx
	testw	%cx,%cx
	jz	L3_16
	cmpw	(%esi),%cx
	je	L1_16

L3_16:	movzwl	(%eax),%eax		/* Unsigned comparison */
	movzwl	(%esi),%ecx
	subl	%ecx,%eax
	popl	%esi
	ret
L4_16:	xorl	%eax,%eax
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars16)

/*
 * RtlCopyString16 - Copy 16-bit string
 * 4(%esp): destination (CHAR16*)
 * 8(%esp): source (CHAR16*)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyString16)
	pushl	%esi
	movl	8(%esp),%eax		/* Destination (after push) */
	movl	12(%esp),%esi		/* Source (after push) */
	movl	%eax,%edx		/* Save destination */

1:	movw	(%esi),%cx
	movw	%cx,(%eax)
	testw	%cx,%cx
	jz	2f			/* Copied null terminator */
	addl	$2,%eax
	addl	$2,%esi
	jmp	1b

2:	movl	%edx,%eax		/* Return destination */
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCopyString16)
