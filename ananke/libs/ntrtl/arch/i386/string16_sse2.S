/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 SSE2-optimized 16-bit character string operations
 *
 * Requires: SSE2 (Pentium 4 and later, available in 32-bit mode)
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength16_SSE2 - SSE2-optimized 16-bit string length
 * 4(%esp): string pointer (CHAR16*)
 * Returns: length in characters (not bytes)
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength16_SSE2)
	movl	4(%esp),%eax		/* String pointer */
	movl	%eax,%edx		/* Save original */

	/* Align to 16 bytes */
	movl	%eax,%ecx
	andl	$15,%ecx
	jz	.Laligned_strlen16_sse2

.Lhead_strlen16_sse2:
	cmpw	$0,(%eax)
	je	.Ldone_strlen16_sse2
	addl	$2,%eax
	testl	$15,%eax
	jnz	.Lhead_strlen16_sse2

.Laligned_strlen16_sse2:
	pxor	%xmm0,%xmm0

.Lloop_strlen16_sse2:
	movdqa	(%eax),%xmm1
	pcmpeqw	%xmm0,%xmm1		/* Compare words */
	pmovmskb %xmm1,%ecx

	testl	%ecx,%ecx
	jnz	.Lfound_strlen16_sse2

	addl	$16,%eax
	jmp	.Lloop_strlen16_sse2

.Lfound_strlen16_sse2:
	bsf	%ecx,%ecx
	shrl	$1,%ecx			/* Convert to word offset */
	leal	(%eax,%ecx,2),%eax

.Ldone_strlen16_sse2:
	subl	%edx,%eax		/* Length in bytes */
	shrl	$1,%eax			/* Convert to characters */
	ret
ANX_ASM_FUNCTION_END(RtlStringLength16_SSE2)

/*
 * RtlFindChar16_SSE2 - SSE2-optimized 16-bit character search
 * 4(%esp): string pointer (CHAR16*)
 * 8(%esp): character to find (low 16 bits)
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar16_SSE2)
	pushl	%edi
	movl	8(%esp),%edi		/* String pointer */
	movl	12(%esp),%eax		/* Character */

	/* Broadcast character to all words */
	movd	%eax,%xmm0
	punpcklwd %xmm0,%xmm0
	pshufd	$0,%xmm0,%xmm0

	pxor	%xmm1,%xmm1

	/* Handle unaligned head */
	movl	%edi,%eax
	andl	$15,%eax
	jz	.Laligned_findchr16_sse2

.Lhead_findchr16_sse2:
	movzwl	(%edi),%ecx
	cmpw	8(%esp),%cx
	je	.Lfound_findchr16_sse2
	testw	%cx,%cx
	jz	.Lnotfound_findchr16_sse2
	addl	$2,%edi
	testl	$15,%edi
	jnz	.Lhead_findchr16_sse2

.Laligned_findchr16_sse2:
.Lloop_findchr16_sse2:
	movdqa	(%edi),%xmm2

	/* Check for target */
	movdqa	%xmm2,%xmm3
	pcmpeqw	%xmm0,%xmm3
	pmovmskb %xmm3,%eax

	/* Check for null */
	pcmpeqw	%xmm1,%xmm2
	pmovmskb %xmm2,%ecx

	testl	%eax,%eax
	jnz	.Lfound_vec_findchr16_sse2

	testl	%ecx,%ecx
	jnz	.Lnotfound_findchr16_sse2

	addl	$16,%edi
	jmp	.Lloop_findchr16_sse2

.Lfound_vec_findchr16_sse2:
	bsf	%eax,%eax
	shrl	$1,%eax
	leal	(%edi,%eax,2),%edi

.Lfound_findchr16_sse2:
	movl	%edi,%eax
	popl	%edi
	ret

.Lnotfound_findchr16_sse2:
	xorl	%eax,%eax
	popl	%edi
	ret
ANX_ASM_FUNCTION_END(RtlFindChar16_SSE2)

/*
 * RtlCompareChars16_SSE2 - SSE2-optimized 16-bit string comparison
 * 4(%esp): string1 (CHAR16*)
 * 8(%esp): string2 (CHAR16*)
 * 12(%esp): max length (in characters)
 * Returns: difference of first differing chars
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars16_SSE2)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi		/* string1 */
	movl	16(%esp),%esi		/* string2 */
	movl	20(%esp),%ecx		/* max length */

	testl	%ecx,%ecx
	jz	.Lequal_cmp16_sse2

	cmpl	$8,%ecx			/* 8 characters = 16 bytes */
	jb	.Lword_cmp16_sse2

	pxor	%xmm2,%xmm2

.Lvec_cmp16_sse2:
	movdqu	(%edi),%xmm0
	movdqu	(%esi),%xmm1

	/* Compare */
	movdqa	%xmm0,%xmm3
	pcmpeqw	%xmm1,%xmm3
	pmovmskb %xmm3,%eax
	xorl	$0xFFFF,%eax

	/* Check for null */
	movdqa	%xmm0,%xmm3
	pcmpeqw	%xmm2,%xmm3
	pmovmskb %xmm3,%edx
	movdqa	%xmm1,%xmm3
	pcmpeqw	%xmm2,%xmm3
	pmovmskb %xmm3,%ebx
	orl	%edx,%ebx

	testl	%eax,%eax
	jnz	.Ldiff_vec_cmp16_sse2
	testl	%ebx,%ebx
	jnz	.Lequal_cmp16_sse2

	addl	$16,%edi
	addl	$16,%esi
	subl	$8,%ecx
	cmpl	$8,%ecx
	jae	.Lvec_cmp16_sse2

.Lword_cmp16_sse2:
	testl	%ecx,%ecx
	jz	.Lequal_cmp16_sse2

.Lword_loop_cmp16_sse2:
	movzwl	(%edi),%eax
	movzwl	(%esi),%edx
	subl	%edx,%eax
	jnz	.Ldone_cmp16_sse2
	testw	%dx,%dx
	jz	.Lequal_cmp16_sse2
	addl	$2,%edi
	addl	$2,%esi
	decl	%ecx
	jnz	.Lword_loop_cmp16_sse2

.Lequal_cmp16_sse2:
	xorl	%eax,%eax
	popl	%edi
	popl	%esi
	ret

.Ldiff_vec_cmp16_sse2:
	bsf	%eax,%ecx
	shrl	$1,%ecx
	movzwl	(%edi,%ecx,2),%eax
	movzwl	(%esi,%ecx,2),%edx
	subl	%edx,%eax

.Ldone_cmp16_sse2:
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars16_SSE2)

/*
 * RtlCopyString16_SSE2 - SSE2-optimized 16-bit string copy
 * 4(%esp): destination (CHAR16*)
 * 8(%esp): source (CHAR16*)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyString16_SSE2)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi		/* destination */
	movl	16(%esp),%esi		/* source */
	movl	%edi,%eax		/* Save for return */

	pxor	%xmm0,%xmm0

	/* Check alignment */
	movl	%edi,%ecx
	orl	%esi,%ecx
	andl	$15,%ecx
	jnz	.Lword_copy16_sse2

.Laligned_copy16_sse2:
	movdqa	(%esi),%xmm1
	movdqa	%xmm1,%xmm2
	pcmpeqw	%xmm0,%xmm2
	pmovmskb %xmm2,%ecx
	testl	%ecx,%ecx
	jnz	.Lpartial_copy16_sse2

	movdqa	%xmm1,(%edi)
	addl	$16,%edi
	addl	$16,%esi
	jmp	.Laligned_copy16_sse2

.Lpartial_copy16_sse2:
	/* Copy remaining words */
	bsf	%ecx,%ecx
	shrl	$1,%ecx
	incl	%ecx
	rep	movsw
	popl	%edi
	popl	%esi
	ret

.Lword_copy16_sse2:
	movzwl	(%esi),%ecx
	movw	%cx,(%edi)
	testw	%cx,%cx
	jz	.Ldone_copy16_sse2
	addl	$2,%edi
	addl	$2,%esi
	jmp	.Lword_copy16_sse2

.Ldone_copy16_sse2:
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCopyString16_SSE2)
