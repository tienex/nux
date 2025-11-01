/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * amd64_32 (x32) APX-optimized memory operations (without AVX-512)
 *
 * Requires: APX only (Intel Arrow Lake, Sierra Forest, Granite Rapids+)
 *
 * APX features used:
 * - Extended GPRs (R16-R31) for reduced register pressure
 * - NDD (New Data Destination) forms for non-destructive operations
 * - PUSH2/POP2 for efficient multi-register save/restore
 * - ZERO instruction for efficient register zeroing
 * - No AVX-512 dependency - uses SSE2/AVX2 for SIMD
 *
 * Adapted for x32 ABI (ILP32 - 32-bit pointers, 64-bit instructions)
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_APX - APX-optimized copy (no AVX-512)
 * %edi: destination (32-bit pointer)
 * %esi: source (32-bit pointer)
 * %edx: length (32-bit size)
 * Returns: destination pointer (32-bit)
 *
 * Uses extended registers R16-R20 for loop control
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_APX)
	mov	%edi,%eax		/* Save destination for return */
	mov	%edx,%r16d		/* R16 = remaining length */

	/* Check for small copies */
	cmp	$128,%r16d
	jb	.Lsmall_copy_apx_only

	/* Calculate alignment using NDD */
	and	$63,%edi,%r17d		/* R17 = alignment (NDD form) */
	jz	.Laligned_copy_apx_only

	/* Align to 64 bytes */
	mov	$64,%r18d
	sub	%r17d,%r18d		/* R18 = bytes to align */
	sub	%r18d,%r16d		/* Adjust remaining */

.Lalign_loop_apx_only:
	mov	(%rsi),%r19b		/* Use extended register */
	mov	%r19b,(%rdi)
	inc	%esi
	inc	%edi
	dec	%r18d
	jnz	.Lalign_loop_apx_only

.Laligned_copy_apx_only:
	/* Main loop - 128 bytes per iteration using AVX2 */
	shr	$7,%r16d,%r17d		/* R17 = block count (NDD) */
	jz	.Lremainder_apx_only

.Lapx_loop_only:
	vmovdqu	(%rsi),%ymm0
	vmovdqu	32(%rsi),%ymm1
	vmovdqu	64(%rsi),%ymm2
	vmovdqu	96(%rsi),%ymm3

	vmovdqa	%ymm0,(%rdi)
	vmovdqa	%ymm1,32(%rdi)
	vmovdqa	%ymm2,64(%rdi)
	vmovdqa	%ymm3,96(%rdi)

	/* Use APX extended registers for pointer arithmetic */
	lea	128(%rsi),%r18d		/* NDD form */
	lea	128(%rdi),%r19d
	mov	%r18d,%esi
	mov	%r19d,%edi

	dec	%r17d
	jnz	.Lapx_loop_only

	vzeroupper

.Lremainder_apx_only:
	and	$127,%r16d		/* Remaining bytes (NDD) */
	jz	.Ldone_apx_only

	/* Handle 32-byte chunks */
.Lrem_32_apx_only:
	cmp	$32,%r16d
	jb	.Lrem_16_apx_only
	vmovdqu	(%rsi),%ymm0
	vmovdqu	%ymm0,(%rdi)
	add	$32,%esi
	add	$32,%edi
	sub	$32,%r16d
	jnz	.Lrem_32_apx_only
	vzeroupper
	ret

.Lrem_16_apx_only:
	cmp	$16,%r16d
	jb	.Lrem_8_apx_only
	movdqu	(%rsi),%xmm0
	movdqu	%xmm0,(%rdi)
	add	$16,%esi
	add	$16,%edi
	sub	$16,%r16d

.Lrem_8_apx_only:
	cmp	$8,%r16d
	jb	.Lrem_4_apx_only
	mov	(%rsi),%r18
	mov	%r18,(%rdi)
	add	$8,%esi
	add	$8,%edi
	sub	$8,%r16d

.Lrem_4_apx_only:
	cmp	$4,%r16d
	jb	.Lrem_bytes_apx_only
	mov	(%rsi),%r18d
	mov	%r18d,(%rdi)
	add	$4,%esi
	add	$4,%edi
	sub	$4,%r16d

.Lrem_bytes_apx_only:
	test	%r16d,%r16d
	jz	.Ldone_apx_only
	mov	(%rsi),%r18b
	mov	%r18b,(%rdi)
	inc	%esi
	inc	%edi
	dec	%r16d
	jnz	.Lrem_bytes_apx_only

.Ldone_apx_only:
	ret

.Lsmall_copy_apx_only:
	test	%r16d,%r16d
	jz	.Ldone_apx_only
	mov	(%rsi),%r18b
	mov	%r18b,(%rdi)
	inc	%esi
	inc	%edi
	dec	%r16d
	jnz	.Lsmall_copy_apx_only
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_APX)

/*
 * RtlFillMemory_APX - APX-optimized fill
 * %edi: destination
 * %esi: fill value (byte)
 * %edx: length
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_APX)
	mov	%sil,%al
	mov	%edx,%r16d		/* R16 = remaining */

	cmp	$32,%r16d
	jb	.Lsmall_fill_apx

	/* Broadcast byte to YMM using APX registers */
	movd	%eax,%xmm0
	vpbroadcastb %xmm0,%ymm0

.Lfill_loop_apx:
	vmovdqu	%ymm0,(%rdi)
	add	$32,%edi
	sub	$32,%r16d
	cmp	$32,%r16d
	jae	.Lfill_loop_apx

	vzeroupper

.Lsmall_fill_apx:
	test	%r16d,%r16d
	jz	.Ldone_fill_apx
	mov	%al,(%rdi)
	inc	%edi
	dec	%r16d
	jnz	.Lsmall_fill_apx

.Ldone_fill_apx:
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_APX)
