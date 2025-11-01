/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * x32 string operations (CHAR8)
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength - Get string length
 * %edi: string pointer (32-bit)
 * Returns: length (not including null terminator)
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength)
	mov	%edi,%ecx
	xor	%eax,%eax
	mov	$-1,%edx
	repne	scasb
	not	%edx
	lea	-1(%rdx),%rax
	ret
ANX_ASM_FUNCTION_END(RtlStringLength)

/*
 * RtlFindChar - Find character in string
 * %edi: string pointer
 * %esi: character to find (in %sil)
 * Returns: pointer to first occurrence, or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar)
	mov	%sil,%al
1:	mov	(%rdi),%cl
	cmp	%al,%cl
	je	2f
	test	%cl,%cl
	jz	3f
	add	$1,%edi
	jmp	1b
2:	mov	%edi,%eax
	ret
3:	xor	%eax,%eax
	ret
ANX_ASM_FUNCTION_END(RtlFindChar)

/*
 * RtlCompareChars - Compare two strings
 * %edi: string1
 * %esi: string2
 * Returns: 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars)
1:	mov	(%rdi),%al
	mov	(%rsi),%cl
	cmp	%cl,%al
	jne	2f
	test	%al,%al
	jz	3f
	add	$1,%edi
	add	$1,%esi
	jmp	1b
2:	movzbl	%al,%eax
	movzbl	%cl,%ecx
	sub	%ecx,%eax
	ret
3:	xor	%eax,%eax
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars)

/*
 * RtlCopyChars - Copy string
 * %edi: destination
 * %esi: source
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyChars)
	mov	%edi,%eax
1:	mov	(%rsi),%cl
	mov	%cl,(%rdi)
	test	%cl,%cl
	jz	2f
	add	$1,%edi
	add	$1,%esi
	jmp	1b
2:	ret
ANX_ASM_FUNCTION_END(RtlCopyChars)
