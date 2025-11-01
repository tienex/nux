/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * x32 SSE4.2-optimized string operations
 *
 * Requires: SSE4.2 (Nehalem+, Bulldozer+)
 * Uses PCMP* instructions for fast string processing
 */

#include <ananke/asmdefs.h>

/*
 * RtlStringLength_SSE42 - SSE4.2-optimized string length
 * %edi: string pointer (32-bit pointer)
 * Returns: length in bytes
 *
 * Uses PCMPISTRI to find null terminator in 16-byte chunks
 */
ANX_ASM_FUNCTION_BEGIN(RtlStringLength_SSE42)
	mov	%edi,%eax		/* Save original pointer */
	pxor	%xmm0,%xmm0		/* XMM0 = zero (searching for null) */

	/* Check alignment */
	mov	%edi,%r8d
	and	$15,%r8d
	jz	.Laligned_strlen

	/* Handle misaligned start */
	and	$~15,%edi		/* Align down to 16 bytes */
	movdqa	(%rdi),%xmm1		/* Load aligned */

	/* PCMPISTRI: Compare for equality, return index in ECX */
	/* imm8 = 0x08: unsigned bytes, equal each, negative polarity */
	pcmpistri $0x08,%xmm1,%xmm0
	jz	.Lfound_in_first	/* ZF=1 means null found */

	/* Null not in first chunk, adjust for alignment offset */
	sub	%r8d,%ecx		/* Subtract alignment offset */
	js	.Lnot_in_first		/* If negative, null is before start */

	/* Null is within valid range */
	mov	%ecx,%eax
	ret

.Lnot_in_first:
	add	$16,%edi		/* Move to next chunk */

.Laligned_strlen:
	/* Main loop - scan 16 bytes at a time */
1:	movdqa	(%rdi),%xmm1
	pcmpistri $0x08,%xmm1,%xmm0
	jz	2f			/* Found null */
	add	$16,%edi
	jmp	1b

2:	/* Found null at offset ECX in current chunk */
	sub	%eax,%edi		/* Calculate distance from start */
	add	%ecx,%edi		/* Add offset within chunk */
	mov	%edi,%eax
	ret

.Lfound_in_first:
	/* Found in first chunk, calculate position */
	sub	%eax,%edi
	add	%ecx,%edi
	sub	%r8d,%edi		/* Adjust for alignment */
	mov	%edi,%eax
	ret
ANX_ASM_FUNCTION_END(RtlStringLength_SSE42)

/*
 * RtlFindChar_SSE42 - SSE4.2-optimized character search
 * %edi: string pointer (32-bit pointer)
 * %esi: character to find
 * Returns: pointer to character or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindChar_SSE42)
	movd	%esi,%xmm0		/* XMM0 = character to find */
	pxor	%xmm2,%xmm2		/* XMM2 = zero (for null check) */

	/* Check alignment */
	mov	%edi,%r8d
	and	$15,%r8d
	mov	%edi,%eax
	jz	.Laligned_findchar

	/* Handle misaligned start */
	and	$~15,%eax		/* Align down */
	movdqa	(%rax),%xmm1		/* Load aligned */

	/* Search for character */
	pcmpistri $0x00,%xmm1,%xmm0	/* Equal each, positive polarity */
	jc	.Lfound_char_first	/* CF=1 means character found */

	/* Check for null */
	pcmpistri $0x08,%xmm1,%xmm2
	jz	.Lnot_found		/* Found null before character */

	add	$16,%eax

.Laligned_findchar:
	/* Main loop */
1:	movdqa	(%rax),%xmm1

	/* Search for character */
	pcmpistri $0x00,%xmm1,%xmm0
	jc	.Lfound_char		/* CF=1 means found */

	/* Check for null terminator */
	pcmpistri $0x08,%xmm1,%xmm2
	jz	.Lnot_found		/* ZF=1 means null found */

	add	$16,%eax
	jmp	1b

.Lfound_char:
	/* Found character at offset ECX */
	add	%ecx,%eax
	ret

.Lfound_char_first:
	/* Found in first chunk */
	add	%ecx,%eax
	cmp	%eax,%edi		/* Make sure it's after string start */
	jbe	.Lnot_found
	mov	%eax,%eax
	ret

.Lnot_found:
	xor	%eax,%eax
	ret
ANX_ASM_FUNCTION_END(RtlFindChar_SSE42)

/*
 * RtlCompareChars_SSE42 - SSE4.2-optimized string compare
 * %edi: string1 (32-bit pointer)
 * %esi: string2 (32-bit pointer)
 * %edx: max length (32-bit size)
 * Returns: 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareChars_SSE42)
	test	%edx,%edx
	jz	.Lequal

	/* Compare 16 bytes at a time */
1:	cmp	$16,%edx
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

	add	$16,%edi
	add	$16,%esi
	sub	$16,%edx
	jmp	1b

.Ltail_compare:
	/* Compare remaining bytes */
	test	%edx,%edx
	jz	.Lequal
3:	movzbl	(%rdi),%eax
	movzbl	(%rsi),%ecx
	subl	%ecx,%eax
	jnz	4f
	testb	%al,%al			/* Check for null */
	jz	.Lequal
	inc	%edi
	inc	%esi
	dec	%edx
	jnz	3b

.Lequal:
	xorl	%eax,%eax
4:	ret
ANX_ASM_FUNCTION_END(RtlCompareChars_SSE42)
