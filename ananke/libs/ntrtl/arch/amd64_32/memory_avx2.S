/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * x32 AVX2-optimized memory operations
 * Uses 256-bit YMM registers for even faster memory operations
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_AVX2 - AVX2-optimized memory copy
 * %edi: destination (32-bit pointer)
 * %esi: source (32-bit pointer)
 * %edx: length (32-bit size)
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_AVX2)
	mov	%edi,%eax	/* return destination */
	mov	%edx,%ecx

	cmp	$32,%ecx
	jb	8f

	/* 32-byte copy using AVX2 */
1:	vmovdqu	(%rsi),%ymm0
	vmovdqu	%ymm0,(%rdi)
	add	$32,%edi
	add	$32,%esi
	sub	$32,%ecx
	cmp	$32,%ecx
	jae	1b

	vzeroupper		/* Clear upper YMM state */

8:	/* Handle remaining bytes < 32 */
	test	%ecx,%ecx
	jz	9f
	rep
	movsb

9:	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_AVX2)

/*
 * RtlMoveMemory_AVX2 - AVX2-optimized memory move
 */
ANX_ASM_FUNCTION_BEGIN(RtlMoveMemory_AVX2)
	mov	%edi,%eax	/* return destination */
	mov	%edx,%ecx

	/* Check for overlap */
	mov	%edi,%r8d
	sub	%esi,%r8d
	cmp	%edx,%r8d
	jb	10f		/* Overlapping */

	/* No overlap, use fast copy */
	cmp	$32,%ecx
	jb	8f

1:	vmovdqu	(%rsi),%ymm0
	vmovdqu	%ymm0,(%rdi)
	add	$32,%edi
	add	$32,%esi
	sub	$32,%ecx
	cmp	$32,%ecx
	jae	1b

	vzeroupper

8:	test	%ecx,%ecx
	jz	9f
	rep
	movsb

9:	ret

10:	/* Overlapping regions - copy backwards */
	vzeroupper
	lea	-1(%rdi,%rdx),%edi
	lea	-1(%rsi,%rdx),%esi
	std
	rep
	movsb
	cld
	ret
ANX_ASM_FUNCTION_END(RtlMoveMemory_AVX2)

/*
 * RtlFillMemory_AVX2 - AVX2-optimized memory fill
 * %edi: destination
 * %esi: fill value (byte)
 * %edx: length
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_AVX2)
	mov	%sil,%al
	mov	%edx,%ecx

	cmp	$32,%ecx
	jb	8f

	/* Broadcast byte to YMM register */
	movd	%eax,%xmm0
	vpbroadcastb %xmm0,%ymm0	/* Broadcast to all 256 bits */

1:	vmovdqu	%ymm0,(%rdi)
	add	$32,%edi
	sub	$32,%ecx
	cmp	$32,%ecx
	jae	1b

	vzeroupper

8:	test	%ecx,%ecx
	jz	9f
	rep
	stosb

9:	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_AVX2)
