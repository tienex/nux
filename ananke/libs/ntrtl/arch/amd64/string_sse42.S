/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 SSE4.2-optimized string operations
 *
 * Requires: SSE4.2 (Nehalem+, Bulldozer+)
 * Uses PCMP* instructions for fast string processing
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength_SSE42 - SSE4.2-optimized string length
 * %rdi: string pointer
 * Returns: length in bytes
 *
 * Uses PCMPISTRI to find null terminator in 16-byte chunks
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength_SSE42)
	movq	%rdi,%rax		/* Save original pointer */
	pxor	%xmm0,%xmm0		/* XMM0 = zero (searching for null) */

	/* Check alignment */
	movq	%rdi,%r8
	andq	$15,%r8
	jz	.Laligned_strlen

	/* Handle misaligned start */
	andq	$~15,%rdi		/* Align down to 16 bytes */
	movdqa	(%rdi),%xmm1		/* Load aligned */

	/* PCMPISTRI: Compare for equality, return index in ECX */
	/* imm8 = 0x08: unsigned bytes, equal each, negative polarity */
	pcmpistri $0x08,%xmm1,%xmm0
	jz	.Lfound_in_first	/* ZF=1 means null found */

	/* Null not in first chunk, adjust for alignment offset */
	subq	%r8,%rcx		/* Subtract alignment offset */
	js	.Lnot_in_first		/* If negative, null is before start */

	/* Null is within valid range */
	movq	%rcx,%rax
	ret

.Lnot_in_first:
	addq	$16,%rdi		/* Move to next chunk */

.Laligned_strlen:
	/* Main loop - scan 16 bytes at a time */
1:	movdqa	(%rdi),%xmm1
	pcmpistri $0x08,%xmm1,%xmm0
	jz	2f			/* Found null */
	addq	$16,%rdi
	jmp	1b

2:	/* Found null at offset ECX in current chunk */
	subq	%rax,%rdi		/* Calculate distance from start */
	addq	%rcx,%rdi		/* Add offset within chunk */
	movq	%rdi,%rax
	ret

.Lfound_in_first:
	/* Found in first chunk, calculate position */
	subq	%rax,%rdi
	addq	%rcx,%rdi
	subq	%r8,%rdi		/* Adjust for alignment */
	movq	%rdi,%rax
	ret
ANX_ASM_FUNCTION_END(RtlStringLength_SSE42)

/*
 * RtlFindChar_SSE42 - SSE4.2-optimized character search
 * %rdi: string pointer
 * %sil: character to find
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar_SSE42)
	movd	%esi,%xmm0		/* XMM0 = character to find */
	pxor	%xmm2,%xmm2		/* XMM2 = zero (for null check) */

	/* Check alignment */
	movq	%rdi,%r8
	andq	$15,%r8
	movq	%rdi,%rax
	jz	.Laligned_findchar

	/* Handle misaligned start */
	andq	$~15,%rax		/* Align down */
	movdqa	(%rax),%xmm1		/* Load aligned */

	/* Search for character */
	pcmpistri $0x00,%xmm1,%xmm0	/* Equal each, positive polarity */
	jc	.Lfound_char_first	/* CF=1 means character found */

	/* Check for null */
	pcmpistri $0x08,%xmm1,%xmm2
	jz	.Lnot_found		/* Found null before character */

	addq	$16,%rax

.Laligned_findchar:
	/* Main loop */
1:	movdqa	(%rax),%xmm1

	/* Search for character */
	pcmpistri $0x00,%xmm1,%xmm0
	jc	.Lfound_char		/* CF=1 means found */

	/* Check for null terminator */
	pcmpistri $0x08,%xmm1,%xmm2
	jz	.Lnot_found		/* ZF=1 means null found */

	addq	$16,%rax
	jmp	1b

.Lfound_char:
	/* Found character at offset ECX */
	addq	%rcx,%rax
	ret

.Lfound_char_first:
	/* Found in first chunk */
	addq	%rcx,%rax
	cmpq	%rax,%rdi		/* Make sure it's after string start */
	jbe	.Lnot_found
	movq	%rax,%rax
	ret

.Lnot_found:
	xorq	%rax,%rax
	ret
ANX_ASM_FUNCTION_END(RtlFindChar_SSE42)

/*
 * RtlCompareChars_SSE42 - SSE4.2-optimized string compare
 * %rdi: string1
 * %rsi: string2
 * %rdx: max length
 * Returns: 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars_SSE42)
	testq	%rdx,%rdx
	jz	.Lequal

	/* Compare 16 bytes at a time */
1:	cmpq	$16,%rdx
	jb	.Ltail_compare

	movdqu	(%rdi),%xmm0
	movdqu	(%rsi),%xmm1

	/* Compare for equality */
	pcmpistri $0x18,%xmm1,%xmm0	/* Equal each, negative polarity */
	jnc	2f			/* CF=0 means all equal */

	/* Found difference at position ECX */
	movzbl	(%rdi,%rcx),%eax
	movzbl	(%rsi,%rcx),%ecx
	subl	%ecx,%eax
	ret

2:	/* Check if we hit null */
	jz	.Lequal			/* ZF=1 means null found, strings equal */

	addq	$16,%rdi
	addq	$16,%rsi
	subq	$16,%rdx
	jmp	1b

.Ltail_compare:
	/* Compare remaining bytes */
	testq	%rdx,%rdx
	jz	.Lequal
3:	movzbl	(%rdi),%eax
	movzbl	(%rsi),%ecx
	subl	%ecx,%eax
	jnz	4f
	testb	%al,%al			/* Check for null */
	jz	.Lequal
	incq	%rdi
	incq	%rsi
	decq	%rdx
	jnz	3b

.Lequal:
	xorl	%eax,%eax
4:	ret
ANX_ASM_FUNCTION_END(RtlCompareChars_SSE42)
