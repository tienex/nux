/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * x32 string operations (CHAR16)
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength16 - Get wide string length
 * %edi: string pointer (32-bit)
 * Returns: length in CHAR16 units (not including null terminator)
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength16)
	mov	%edi,%ecx
	xor	%eax,%eax
	mov	$-1,%edx
	repne	scasw
	not	%edx
	lea	-1(%rdx),%rax
	ret
ANX_ASM_FUNCTION_END(RtlStringLength16)

/*
 * RtlFindChar16 - Find wide character in string
 * %edi: string pointer
 * %esi: character to find (in %si)
 * Returns: pointer to first occurrence, or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar16)
	mov	%si,%ax
1:	mov	(%rdi),%cx
	cmp	%ax,%cx
	je	2f
	test	%cx,%cx
	jz	3f
	add	$2,%edi
	jmp	1b
2:	mov	%edi,%eax
	ret
3:	xor	%eax,%eax
	ret
ANX_ASM_FUNCTION_END(RtlFindChar16)

/*
 * RtlCompareChars16 - Compare two wide strings
 * %edi: string1
 * %esi: string2
 * Returns: 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars16)
1:	mov	(%rdi),%ax
	mov	(%rsi),%cx
	cmp	%cx,%ax
	jne	2f
	test	%ax,%ax
	jz	3f
	add	$2,%edi
	add	$2,%esi
	jmp	1b
2:	movzwl	%ax,%eax
	movzwl	%cx,%ecx
	sub	%ecx,%eax
	ret
3:	xor	%eax,%eax
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars16)

/*
 * RtlCopyChars16 - Copy wide string
 * %edi: destination
 * %esi: source
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyChars16)
	mov	%edi,%eax
1:	mov	(%rsi),%cx
	mov	%cx,(%rdi)
	test	%cx,%cx
	jz	2f
	add	$2,%edi
	add	$2,%esi
	jmp	1b
2:	ret
ANX_ASM_FUNCTION_END(RtlCopyChars16)
