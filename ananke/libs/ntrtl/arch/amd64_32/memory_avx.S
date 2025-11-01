/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * x32 AVX-optimized memory operations
 *
 * Requires: AVX (Sandy Bridge+, Bulldozer+)
 * Uses 256-bit YMM registers with floating-point move instructions
 * Note: AVX lacks integer SIMD, so uses vmov* instead of vpbroadcast
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_AVX - AVX-optimized memory copy
 * %edi: destination (32-bit pointer)
 * %esi: source (32-bit pointer)
 * %edx: length (32-bit size)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_AVX)
	mov	%edi,%eax		/* Save destination for return */
	mov	%edx,%ecx		/* Copy length */

	/* Check for small copies */
	cmp	$128,%ecx
	jb	.Lsmall_copy_avx

	/* Check alignment */
	mov	%edi,%r8d
	and	$31,%r8d		/* Check if destination is 32-byte aligned */
	jz	.Laligned_copy_avx

	/* Align destination to 32 bytes */
	mov	$32,%r9d
	sub	%r8d,%r9d		/* Bytes to align */
	sub	%r9d,%ecx		/* Adjust remaining count */

	/* Copy alignment bytes */
.Lalign_loop_avx:
	movb	(%rsi),%r8b
	movb	%r8b,(%rdi)
	inc	%esi
	inc	%edi
	dec	%r9d
	jnz	.Lalign_loop_avx

.Laligned_copy_avx:
	/* Main AVX copy loop - 128 bytes per iteration */
	mov	%ecx,%r9d
	shr	$7,%r9d			/* Number of 128-byte blocks */
	jz	.Lremainder_avx

.Lavx_loop:
	vmovdqu	(%rsi),%ymm0
	vmovdqu	32(%rsi),%ymm1
	vmovdqu	64(%rsi),%ymm2
	vmovdqu	96(%rsi),%ymm3

	vmovdqa	%ymm0,(%rdi)
	vmovdqa	%ymm1,32(%rdi)
	vmovdqa	%ymm2,64(%rdi)
	vmovdqa	%ymm3,96(%rdi)

	add	$128,%esi
	add	$128,%edi
	dec	%r9d
	jnz	.Lavx_loop

	vzeroupper			/* Clear upper YMM bits */

.Lremainder_avx:
	and	$127,%ecx		/* Remaining bytes */
	jz	.Ldone_avx

	/* Copy remaining 32-byte chunks */
	mov	%ecx,%r9d
	shr	$5,%r9d
	jz	.Ltail_avx

.Lrem32_loop_avx:
	vmovdqu	(%rsi),%ymm0
	vmovdqu	%ymm0,(%rdi)
	add	$32,%esi
	add	$32,%edi
	dec	%r9d
	jnz	.Lrem32_loop_avx

	vzeroupper
	and	$31,%ecx

.Ltail_avx:
	/* Copy remaining bytes */
	test	%ecx,%ecx
	jz	.Ldone_avx
.Ltail_loop_avx:
	movb	(%rsi),%r8b
	movb	%r8b,(%rdi)
	inc	%esi
	inc	%edi
	dec	%ecx
	jnz	.Ltail_loop_avx
	jmp	.Ldone_avx

.Lsmall_copy_avx:
	/* Small copy without AVX */
	rep	movsb

.Ldone_avx:
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_AVX)

/*
 * RtlFillMemory_AVX - AVX-optimized memory fill
 * %edi: buffer (32-bit pointer)
 * %esi: byte value
 * %edx: length (32-bit size)
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_AVX)
	mov	%edi,%eax		/* Save buffer for return */
	movzbl	%sil,%esi		/* Zero-extend byte value */

	/* Replicate byte to all bytes in YMM register */
	mov	%esi,%r8d
	shl	$8,%esi
	or	%r8d,%esi		/* 16-bit pattern */
	mov	%esi,%r8d
	shl	$16,%esi
	or	%r8d,%esi		/* 32-bit pattern */

	/* Move to XMM then broadcast to YMM using vinsertf128 */
	vmovd	%esi,%xmm0
	vinsertf128 $1,%xmm0,%ymm0,%ymm0  /* Duplicate lower half to upper */

	mov	%edx,%ecx		/* Copy length */

	/* Check for small fills */
	cmp	$128,%ecx
	jb	.Lsmall_fill_avx

	/* Align destination to 32 bytes */
	mov	%edi,%r8d
	and	$31,%r8d
	jz	.Laligned_fill_avx

	mov	$32,%r9d
	sub	%r8d,%r9d
	sub	%r9d,%ecx

.Lalign_fill_loop_avx:
	movb	%sil,(%rdi)
	inc	%edi
	dec	%r9d
	jnz	.Lalign_fill_loop_avx

.Laligned_fill_avx:
	/* Main AVX fill loop - 128 bytes per iteration */
	mov	%ecx,%r9d
	shr	$7,%r9d
	jz	.Lfill_remainder_avx

.Lavx_fill_loop:
	vmovdqa	%ymm0,(%rdi)
	vmovdqa	%ymm0,32(%rdi)
	vmovdqa	%ymm0,64(%rdi)
	vmovdqa	%ymm0,96(%rdi)

	add	$128,%edi
	dec	%r9d
	jnz	.Lavx_fill_loop

	vzeroupper

.Lfill_remainder_avx:
	and	$127,%ecx
	jz	.Lfill_done_avx

	/* Fill remaining 32-byte chunks */
	mov	%ecx,%r9d
	shr	$5,%r9d
	jz	.Lfill_tail_avx

.Lrem32_fill_loop_avx:
	vmovdqa	%ymm0,(%rdi)
	add	$32,%edi
	dec	%r9d
	jnz	.Lrem32_fill_loop_avx

	vzeroupper
	and	$31,%ecx

.Lfill_tail_avx:
	test	%ecx,%ecx
	jz	.Lfill_done_avx
.Lfill_tail_loop_avx:
	movb	%sil,(%rdi)
	inc	%edi
	dec	%ecx
	jnz	.Lfill_tail_loop_avx
	jmp	.Lfill_done_avx

.Lsmall_fill_avx:
	rep	stosb

.Lfill_done_avx:
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_AVX)
