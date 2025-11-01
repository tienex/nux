/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * x32 SSE2-optimized memory operations
 * Uses 128-bit XMM registers for faster memory operations
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_SSE2 - SSE2-optimized memory copy
 * %edi: destination (32-bit pointer)
 * %esi: source (32-bit pointer)
 * %edx: length (32-bit size)
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_SSE2)
	mov	%edi,%eax	/* return destination */
	mov	%edx,%ecx

	cmp	$16,%ecx
	jb	8f

	/* 16-byte aligned copy using SSE2 */
1:	movdqu	(%rsi),%xmm0
	movdqu	%xmm0,(%rdi)
	add	$16,%edi
	add	$16,%esi
	sub	$16,%ecx
	cmp	$16,%ecx
	jae	1b

8:	/* Handle remaining bytes < 16 */
	test	%ecx,%ecx
	jz	9f
	rep
	movsb

9:	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_SSE2)

/*
 * RtlMoveMemory_SSE2 - SSE2-optimized memory move
 */
ANX_ASM_FUNCTION_BEGIN(RtlMoveMemory_SSE2)
	mov	%edi,%eax	/* return destination */
	mov	%edx,%ecx

	/* Check for overlap */
	mov	%edi,%r8d
	sub	%esi,%r8d
	cmp	%edx,%r8d
	jb	10f		/* Overlapping, handle specially */

	/* No overlap, use fast copy */
	cmp	$16,%ecx
	jb	8f

1:	movdqu	(%rsi),%xmm0
	movdqu	%xmm0,(%rdi)
	add	$16,%edi
	add	$16,%esi
	sub	$16,%ecx
	cmp	$16,%ecx
	jae	1b

8:	test	%ecx,%ecx
	jz	9f
	rep
	movsb

9:	ret

10:	/* Overlapping regions - copy backwards */
	lea	-1(%rdi,%rdx),%edi
	lea	-1(%rsi,%rdx),%esi
	std
	rep
	movsb
	cld
	ret
ANX_ASM_FUNCTION_END(RtlMoveMemory_SSE2)

/*
 * RtlFillMemory_SSE2 - SSE2-optimized memory fill
 * %edi: destination
 * %esi: fill value (byte)
 * %edx: length
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_SSE2)
	mov	%sil,%al
	mov	%edx,%ecx

	cmp	$16,%ecx
	jb	8f

	/* Broadcast byte to XMM register */
	movd	%eax,%xmm0
	punpcklbw %xmm0,%xmm0	/* Replicate byte to word */
	pshuflw	$0,%xmm0,%xmm0	/* Replicate to low 64 bits */
	pshufd	$0,%xmm0,%xmm0	/* Replicate to all 128 bits */

1:	movdqu	%xmm0,(%rdi)
	add	$16,%edi
	sub	$16,%ecx
	cmp	$16,%ecx
	jae	1b

8:	test	%ecx,%ecx
	jz	9f
	rep
	stosb

9:	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_SSE2)
