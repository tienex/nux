/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 SSE4.2-optimized string operations
 *
 * Requires: SSE4.2 (Nehalem and later, available in 32-bit mode)
 *
 * SSE4.2 provides PCMPISTRI/PCMPISTRM for fast string operations:
 * - PCMPISTRI: Packed Compare Implicit Length Strings, Return Index
 * - PCMPISTRM: Packed Compare Implicit Length Strings, Return Mask
 *
 * These instructions can search for null terminators and specific
 * characters in a single instruction.
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength_SSE42 - SSE4.2-optimized string length
 * 4(%esp): string pointer
 * Returns: length in bytes
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength_SSE42)
	movl	4(%esp),%eax		/* String pointer */
	movl	%eax,%edx		/* Save original */

	/* Align to 16 bytes */
	movl	%eax,%ecx
	andl	$15,%ecx
	jz	.Laligned_strlen_sse42

.Lhead_strlen_sse42:
	cmpb	$0,(%eax)
	je	.Ldone_strlen_sse42
	incl	%eax
	testl	$15,%eax
	jnz	.Lhead_strlen_sse42

.Laligned_strlen_sse42:
	pxor	%xmm1,%xmm1		/* Null terminator pattern */

.Lloop_strlen_sse42:
	movdqa	(%eax),%xmm0
	/* PCMPISTRI: Compare 16 bytes for null (implicit length) */
	/* imm8 = 0x08: unsigned bytes, equal each, positive polarity */
	pcmpistri $0x08,%xmm1,%xmm0
	jz	.Lfound_strlen_sse42	/* CF=1 if null found */

	addl	$16,%eax
	jmp	.Lloop_strlen_sse42

.Lfound_strlen_sse42:
	addl	%ecx,%eax		/* Add offset from PCMPISTRI */

.Ldone_strlen_sse42:
	subl	%edx,%eax
	ret
ANX_ASM_FUNCTION_END(RtlStringLength_SSE42)

/*
 * RtlFindChar_SSE42 - SSE4.2-optimized character search
 * 4(%esp): string pointer
 * 8(%esp): character to find
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar_SSE42)
	pushl	%edi
	movl	8(%esp),%edi		/* String pointer */
	movl	12(%esp),%eax		/* Character */

	/* Create search pattern */
	movd	%eax,%xmm1
	pxor	%xmm2,%xmm2		/* Null pattern */

	/* Handle unaligned head */
	movl	%edi,%eax
	andl	$15,%eax
	jz	.Laligned_findchr_sse42

.Lhead_findchr_sse42:
	movzbl	(%edi),%ecx
	cmpb	8(%esp),%cl
	je	.Lfound_findchr_sse42
	testb	%cl,%cl
	jz	.Lnotfound_findchr_sse42
	incl	%edi
	testl	$15,%edi
	jnz	.Lhead_findchr_sse42

.Laligned_findchr_sse42:
.Lloop_findchr_sse42:
	movdqa	(%edi),%xmm0

	/* Search for character using PCMPISTRI */
	/* imm8 = 0x00: unsigned bytes, equal any, positive polarity */
	pcmpistri $0x00,%xmm1,%xmm0
	jc	.Lfound_vec_findchr_sse42	/* CF=1 if match found */

	/* Check for null terminator */
	pcmpistri $0x08,%xmm2,%xmm0
	jz	.Lnotfound_findchr_sse42	/* ZF=1 if null found */

	addl	$16,%edi
	jmp	.Lloop_findchr_sse42

.Lfound_vec_findchr_sse42:
	addl	%ecx,%edi		/* ECX has index from PCMPISTRI */

.Lfound_findchr_sse42:
	movl	%edi,%eax
	popl	%edi
	ret

.Lnotfound_findchr_sse42:
	xorl	%eax,%eax
	popl	%edi
	ret
ANX_ASM_FUNCTION_END(RtlFindChar_SSE42)

/*
 * RtlCompareChars_SSE42 - SSE4.2-optimized string comparison
 * 4(%esp): string1
 * 8(%esp): string2
 * 12(%esp): max length
 * Returns: difference of first differing chars
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars_SSE42)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi		/* string1 */
	movl	16(%esp),%esi		/* string2 */
	movl	20(%esp),%ecx		/* max length */

	testl	%ecx,%ecx
	jz	.Lequal_cmp_sse42

	cmpl	$16,%ecx
	jb	.Lbyte_cmp_sse42

	/* Vector comparison using PCMPISTRI */
.Lvec_cmp_sse42:
	movdqu	(%edi),%xmm0
	movdqu	(%esi),%xmm1

	/* Compare strings for differences */
	/* imm8 = 0x18: unsigned bytes, equal each, negative polarity */
	pcmpistri $0x18,%xmm1,%xmm0
	jc	.Ldiff_vec_cmp_sse42	/* CF=1 if difference found */
	jz	.Lequal_cmp_sse42	/* ZF=1 if null found */

	addl	$16,%edi
	addl	$16,%esi
	subl	$16,%ecx
	cmpl	$16,%ecx
	jae	.Lvec_cmp_sse42

.Lbyte_cmp_sse42:
	testl	%ecx,%ecx
	jz	.Lequal_cmp_sse42

.Lbyte_loop_cmp_sse42:
	movzbl	(%edi),%eax
	movzbl	(%esi),%edx
	subl	%edx,%eax
	jnz	.Ldone_cmp_sse42
	testb	%dl,%dl
	jz	.Lequal_cmp_sse42
	incl	%edi
	incl	%esi
	decl	%ecx
	jnz	.Lbyte_loop_cmp_sse42

.Lequal_cmp_sse42:
	xorl	%eax,%eax
	popl	%edi
	popl	%esi
	ret

.Ldiff_vec_cmp_sse42:
	/* ECX has index of first difference */
	movzbl	(%edi,%ecx),%eax
	movzbl	(%esi,%ecx),%edx
	subl	%edx,%eax

.Ldone_cmp_sse42:
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars_SSE42)

/*
 * RtlFindByteInMemory_SSE42 - SSE4.2-optimized byte search in memory
 * 4(%esp): buffer
 * 8(%esp): byte value to find
 * 12(%esp): length
 * Returns: pointer to byte or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindByteInMemory_SSE42)
	pushl	%edi
	movl	8(%esp),%edi		/* Buffer */
	movl	12(%esp),%eax		/* Byte value */
	movl	16(%esp),%ecx		/* Length */

	testl	%ecx,%ecx
	jz	.Lnotfound_findbyte_sse42

	/* Create search pattern */
	movd	%eax,%xmm1

	/* Handle unaligned head */
	movl	%edi,%eax
	andl	$15,%eax
	jz	.Laligned_findbyte_sse42

.Lhead_findbyte_sse42:
	movzbl	(%edi),%edx
	cmpb	8(%esp),%dl
	je	.Lfound_findbyte_sse42
	incl	%edi
	decl	%ecx
	jz	.Lnotfound_findbyte_sse42
	testl	$15,%edi
	jnz	.Lhead_findbyte_sse42

.Laligned_findbyte_sse42:
	cmpl	$16,%ecx
	jb	.Ltail_findbyte_sse42

.Lloop_findbyte_sse42:
	movdqa	(%edi),%xmm0
	pcmpistri $0x00,%xmm1,%xmm0	/* Search for byte */
	jc	.Lfound_vec_findbyte_sse42

	addl	$16,%edi
	subl	$16,%ecx
	cmpl	$16,%ecx
	jae	.Lloop_findbyte_sse42

.Ltail_findbyte_sse42:
	testl	%ecx,%ecx
	jz	.Lnotfound_findbyte_sse42

.Ltail_loop_findbyte_sse42:
	movzbl	(%edi),%edx
	cmpb	8(%esp),%dl
	je	.Lfound_findbyte_sse42
	incl	%edi
	decl	%ecx
	jnz	.Ltail_loop_findbyte_sse42

.Lnotfound_findbyte_sse42:
	xorl	%eax,%eax
	popl	%edi
	ret

.Lfound_vec_findbyte_sse42:
	leal	(%edi,%ecx),%edi

.Lfound_findbyte_sse42:
	movl	%edi,%eax
	popl	%edi
	ret
ANX_ASM_FUNCTION_END(RtlFindByteInMemory_SSE42)
