/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 AVX2-optimized 16-bit character string operations
 *
 * Requires: AVX2 (Haswell and later, available in 32-bit mode)
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength16_AVX2 - AVX2-optimized 16-bit string length
 * 4(%esp): string pointer (CHAR16*)
 * Returns: length in characters (not bytes)
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength16_AVX2)
	movl	4(%esp),%eax		/* String pointer */
	movl	%eax,%edx		/* Save original */

	/* Align to 32 bytes */
	movl	%eax,%ecx
	andl	$31,%ecx
	jz	.Laligned_strlen16_avx2

.Lhead_strlen16_avx2:
	cmpw	$0,(%eax)
	je	.Ldone_strlen16_avx2
	addl	$2,%eax
	testl	$31,%eax
	jnz	.Lhead_strlen16_avx2

.Laligned_strlen16_avx2:
	vpxor	%ymm0,%ymm0,%ymm0

.Lloop_strlen16_avx2:
	vmovdqa	(%eax),%ymm1
	vpcmpeqw %ymm0,%ymm1,%ymm2
	vpmovmskb %ymm2,%ecx

	testl	%ecx,%ecx
	jnz	.Lfound_strlen16_avx2

	addl	$32,%eax
	jmp	.Lloop_strlen16_avx2

.Lfound_strlen16_avx2:
	bsf	%ecx,%ecx
	shrl	$1,%ecx
	leal	(%eax,%ecx,2),%eax

.Ldone_strlen16_avx2:
	subl	%edx,%eax
	shrl	$1,%eax
	vzeroupper
	ret
ANX_ASM_FUNCTION_END(RtlStringLength16_AVX2)

/*
 * RtlFindChar16_AVX2 - AVX2-optimized 16-bit character search
 * 4(%esp): string pointer (CHAR16*)
 * 8(%esp): character to find (low 16 bits)
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar16_AVX2)
	pushl	%edi
	movl	8(%esp),%edi		/* String pointer */
	movl	12(%esp),%eax		/* Character */

	/* Broadcast character */
	vmovd	%eax,%xmm0
	vpbroadcastw %xmm0,%ymm0

	vpxor	%ymm1,%ymm1,%ymm1

	/* Handle unaligned head */
	movl	%edi,%eax
	andl	$31,%eax
	jz	.Laligned_findchr16_avx2

.Lhead_findchr16_avx2:
	movzwl	(%edi),%ecx
	cmpw	8(%esp),%cx
	je	.Lfound_findchr16_avx2
	testw	%cx,%cx
	jz	.Lnotfound_findchr16_avx2
	addl	$2,%edi
	testl	$31,%edi
	jnz	.Lhead_findchr16_avx2

.Laligned_findchr16_avx2:
.Lloop_findchr16_avx2:
	vmovdqa	(%edi),%ymm2

	/* Check for target */
	vpcmpeqw %ymm0,%ymm2,%ymm3
	vpmovmskb %ymm3,%eax

	/* Check for null */
	vpcmpeqw %ymm1,%ymm2,%ymm3
	vpmovmskb %ymm3,%ecx

	testl	%eax,%eax
	jnz	.Lfound_vec_findchr16_avx2

	testl	%ecx,%ecx
	jnz	.Lnotfound_findchr16_avx2

	addl	$32,%edi
	jmp	.Lloop_findchr16_avx2

.Lfound_vec_findchr16_avx2:
	bsf	%eax,%eax
	shrl	$1,%eax
	leal	(%edi,%eax,2),%edi

.Lfound_findchr16_avx2:
	movl	%edi,%eax
	vzeroupper
	popl	%edi
	ret

.Lnotfound_findchr16_avx2:
	xorl	%eax,%eax
	vzeroupper
	popl	%edi
	ret
ANX_ASM_FUNCTION_END(RtlFindChar16_AVX2)

/*
 * RtlCompareChars16_AVX2 - AVX2-optimized 16-bit string comparison
 * 4(%esp): string1 (CHAR16*)
 * 8(%esp): string2 (CHAR16*)
 * 12(%esp): max length (in characters)
 * Returns: difference of first differing chars
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars16_AVX2)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi		/* string1 */
	movl	16(%esp),%esi		/* string2 */
	movl	20(%esp),%ecx		/* max length */

	testl	%ecx,%ecx
	jz	.Lequal_cmp16_avx2

	cmpl	$16,%ecx		/* 16 chars = 32 bytes */
	jb	.Lword_cmp16_avx2

	vpxor	%ymm2,%ymm2,%ymm2

.Lvec_cmp16_avx2:
	vmovdqu	(%edi),%ymm0
	vmovdqu	(%esi),%ymm1

	/* Compare */
	vpcmpeqw %ymm0,%ymm1,%ymm3
	vpmovmskb %ymm3,%eax
	xorl	$0xFFFFFFFF,%eax

	/* Check for null */
	vpcmpeqw %ymm2,%ymm0,%ymm3
	vpmovmskb %ymm3,%edx
	vpcmpeqw %ymm2,%ymm1,%ymm3
	vpmovmskb %ymm3,%ebx
	orl	%edx,%ebx

	testl	%eax,%eax
	jnz	.Ldiff_vec_cmp16_avx2
	testl	%ebx,%ebx
	jnz	.Lequal_cmp16_avx2

	addl	$32,%edi
	addl	$32,%esi
	subl	$16,%ecx
	cmpl	$16,%ecx
	jae	.Lvec_cmp16_avx2

.Lword_cmp16_avx2:
	testl	%ecx,%ecx
	jz	.Lequal_cmp16_avx2

.Lword_loop_cmp16_avx2:
	movzwl	(%edi),%eax
	movzwl	(%esi),%edx
	subl	%edx,%eax
	jnz	.Ldone_cmp16_avx2
	testw	%dx,%dx
	jz	.Lequal_cmp16_avx2
	addl	$2,%edi
	addl	$2,%esi
	decl	%ecx
	jnz	.Lword_loop_cmp16_avx2

.Lequal_cmp16_avx2:
	xorl	%eax,%eax
	vzeroupper
	popl	%edi
	popl	%esi
	ret

.Ldiff_vec_cmp16_avx2:
	bsf	%eax,%ecx
	shrl	$1,%ecx
	movzwl	(%edi,%ecx,2),%eax
	movzwl	(%esi,%ecx,2),%edx
	subl	%edx,%eax

.Ldone_cmp16_avx2:
	vzeroupper
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCompareChars16_AVX2)

/*
 * RtlCopyString16_AVX2 - AVX2-optimized 16-bit string copy
 * 4(%esp): destination (CHAR16*)
 * 8(%esp): source (CHAR16*)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyString16_AVX2)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi		/* destination */
	movl	16(%esp),%esi		/* source */
	movl	%edi,%eax		/* Save for return */

	vpxor	%ymm0,%ymm0,%ymm0

	/* Check alignment */
	movl	%edi,%ecx
	orl	%esi,%ecx
	andl	$31,%ecx
	jnz	.Lword_copy16_avx2

.Laligned_copy16_avx2:
	vmovdqa	(%esi),%ymm1
	vpcmpeqw %ymm0,%ymm1,%ymm2
	vpmovmskb %ymm2,%ecx
	testl	%ecx,%ecx
	jnz	.Lpartial_copy16_avx2

	vmovdqa	%ymm1,(%edi)
	addl	$32,%edi
	addl	$32,%esi
	jmp	.Laligned_copy16_avx2

.Lpartial_copy16_avx2:
	/* Copy remaining words */
	bsf	%ecx,%ecx
	shrl	$1,%ecx
	incl	%ecx
	rep	movsw
	vzeroupper
	popl	%edi
	popl	%esi
	ret

.Lword_copy16_avx2:
	movzwl	(%esi),%ecx
	movw	%cx,(%edi)
	testw	%cx,%cx
	jz	.Ldone_copy16_avx2
	addl	$2,%edi
	addl	$2,%esi
	jmp	.Lword_copy16_avx2

.Ldone_copy16_avx2:
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCopyString16_AVX2)
