/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * amd64_32 (x32) APX + AVX-512 optimized memory operations
 *
 * Requires: APX + AVX-512 (Intel Granite Rapids+)
 *
 * APX features used:
 * - Extended GPRs (R16-R31)
 * - NDD (New Data Destination) forms
 * - Combines with AVX-512 for maximum performance
 *
 * Adapted for x32 ABI (ILP32 - 32-bit pointers, 64-bit instructions)
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_APX512 - APX + AVX-512 optimized copy
 * %edi: destination (32-bit pointer)
 * %esi: source (32-bit pointer)
 * %edx: length (32-bit size)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_APX512)
	mov	%edi,%eax		/* Save destination */
	mov	%edx,%r16d		/* R16 = remaining */

	cmp	$256,%r16d
	jb	.Lsmall_copy_apx512

	/* Calculate alignment using NDD */
	and	$63,%edi,%r17d		/* R17 = alignment (NDD) */
	jz	.Laligned_copy_apx512

	/* Align to 64 bytes */
	mov	$64,%r18d
	sub	%r17d,%r18d		/* R18 = bytes to align */
	sub	%r18d,%r16d

.Lalign_loop_apx512:
	mov	(%rsi),%r19b
	mov	%r19b,(%rdi)
	inc	%esi
	inc	%edi
	dec	%r18d
	jnz	.Lalign_loop_apx512

.Laligned_copy_apx512:
	/* Main loop - 256 bytes per iteration using AVX-512 */
	shr	$8,%r16d,%r17d		/* R17 = block count (NDD) */
	jz	.Lremainder_apx512

.Lapx512_loop:
	vmovdqu64 (%rsi),%zmm0
	vmovdqu64 64(%rsi),%zmm1
	vmovdqu64 128(%rsi),%zmm2
	vmovdqu64 192(%rsi),%zmm3

	vmovdqa64 %zmm0,(%rdi)
	vmovdqa64 %zmm1,64(%rdi)
	vmovdqa64 %zmm2,128(%rdi)
	vmovdqa64 %zmm3,192(%rdi)

	/* Use APX for pointer arithmetic */
	lea	256(%rsi),%r18d
	lea	256(%rdi),%r19d
	mov	%r18d,%esi
	mov	%r19d,%edi

	dec	%r17d
	jnz	.Lapx512_loop

.Lremainder_apx512:
	and	$255,%r16d		/* Remaining bytes */
	jz	.Ldone_apx512

	/* Handle 64-byte chunks */
.Lrem_64_apx512:
	cmp	$64,%r16d
	jb	.Lrem_32_apx512
	vmovdqu64 (%rsi),%zmm0
	vmovdqu64 %zmm0,(%rdi)
	add	$64,%esi
	add	$64,%edi
	sub	$64,%r16d
	jnz	.Lrem_64_apx512
	ret

.Lrem_32_apx512:
	cmp	$32,%r16d
	jb	.Lrem_16_apx512
	vmovdqu	(%rsi),%ymm0
	vmovdqu	%ymm0,(%rdi)
	add	$32,%esi
	add	$32,%edi
	sub	$32,%r16d

.Lrem_16_apx512:
	cmp	$16,%r16d
	jb	.Lrem_bytes_apx512
	movdqu	(%rsi),%xmm0
	movdqu	%xmm0,(%rdi)
	add	$16,%esi
	add	$16,%edi
	sub	$16,%r16d

.Lrem_bytes_apx512:
	test	%r16d,%r16d
	jz	.Ldone_apx512
	mov	(%rsi),%r18b
	mov	%r18b,(%rdi)
	inc	%esi
	inc	%edi
	dec	%r16d
	jnz	.Lrem_bytes_apx512

.Ldone_apx512:
	ret

.Lsmall_copy_apx512:
	test	%r16d,%r16d
	jz	.Ldone_apx512
	mov	(%rsi),%r18b
	mov	%r18b,(%rdi)
	inc	%esi
	inc	%edi
	dec	%r16d
	jnz	.Lsmall_copy_apx512
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_APX512)

/*
 * RtlFillMemory_APX512 - APX + AVX-512 optimized fill
 * %edi: destination
 * %esi: fill value (byte)
 * %edx: length
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_APX512)
	mov	%sil,%al
	mov	%edx,%r16d

	cmp	$64,%r16d
	jb	.Lsmall_fill_apx512

	/* Broadcast byte to ZMM */
	vpbroadcastb %eax,%zmm0

.Lfill_loop_apx512:
	vmovdqu64 %zmm0,(%rdi)
	add	$64,%edi
	sub	$64,%r16d
	cmp	$64,%r16d
	jae	.Lfill_loop_apx512

.Lsmall_fill_apx512:
	test	%r16d,%r16d
	jz	.Ldone_fill_apx512
	mov	%al,(%rdi)
	inc	%edi
	dec	%r16d
	jnz	.Lsmall_fill_apx512

.Ldone_fill_apx512:
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_APX512)
