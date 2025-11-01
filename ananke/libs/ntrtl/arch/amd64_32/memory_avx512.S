/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * x32 AVX-512-optimized memory operations
 *
 * Requires: AVX-512F (Skylake-X and later, Zen 4 and later)
 * Uses 512-bit ZMM registers for maximum throughput
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_AVX512 - AVX-512-optimized memory copy
 * %edi: destination (32-bit pointer)
 * %esi: source (32-bit pointer)
 * %edx: length (32-bit size)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_AVX512)
	mov	%edi,%eax		/* Save destination for return */
	mov	%edx,%ecx		/* Copy length */

	/* Check for small copies */
	cmp	$256,%ecx
	jb	.Lsmall_copy_avx512

	/* Check alignment */
	mov	%edi,%r8d
	and	$63,%r8d		/* Check if destination is 64-byte aligned */
	jz	.Laligned_copy_avx512

	/* Align destination to 64 bytes */
	mov	$64,%r9d
	sub	%r8d,%r9d		/* Bytes to align */
	sub	%r9d,%ecx		/* Adjust remaining count */

	/* Copy alignment bytes */
.Lalign_loop_avx512:
	movb	(%rsi),%r8b
	movb	%r8b,(%rdi)
	inc	%esi
	inc	%edi
	dec	%r9d
	jnz	.Lalign_loop_avx512

.Laligned_copy_avx512:
	/* Main AVX-512 copy loop - 256 bytes per iteration (4x ZMM) */
	mov	%ecx,%r9d
	shr	$8,%r9d			/* Number of 256-byte blocks */
	jz	.Lremainder_avx512

.Lavx512_loop:
	vmovdqu64	(%rsi),%zmm0
	vmovdqu64	64(%rsi),%zmm1
	vmovdqu64	128(%rsi),%zmm2
	vmovdqu64	192(%rsi),%zmm3

	vmovdqa64	%zmm0,(%rdi)
	vmovdqa64	%zmm1,64(%rdi)
	vmovdqa64	%zmm2,128(%rdi)
	vmovdqa64	%zmm3,192(%rdi)

	add	$256,%esi
	add	$256,%edi
	dec	%r9d
	jnz	.Lavx512_loop

	vzeroupper			/* Clear upper ZMM bits */

.Lremainder_avx512:
	and	$255,%ecx		/* Remaining bytes */
	jz	.Ldone_avx512

	/* Copy remaining 64-byte chunks */
	mov	%ecx,%r9d
	shr	$6,%r9d
	jz	.Ltail_avx512

.Lrem64_loop_avx512:
	vmovdqu64	(%rsi),%zmm0
	vmovdqu64	%zmm0,(%rdi)
	add	$64,%esi
	add	$64,%edi
	dec	%r9d
	jnz	.Lrem64_loop_avx512

	vzeroupper
	and	$63,%ecx

.Ltail_avx512:
	/* Copy remaining bytes */
	test	%ecx,%ecx
	jz	.Ldone_avx512
.Ltail_loop_avx512:
	movb	(%rsi),%r8b
	movb	%r8b,(%rdi)
	inc	%esi
	inc	%edi
	dec	%ecx
	jnz	.Ltail_loop_avx512
	jmp	.Ldone_avx512

.Lsmall_copy_avx512:
	/* Small copy without AVX-512 */
	rep	movsb

.Ldone_avx512:
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_AVX512)

/*
 * RtlFillMemory_AVX512 - AVX-512-optimized memory fill
 * %edi: buffer (32-bit pointer)
 * %esi: byte value
 * %edx: length (32-bit size)
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_AVX512)
	mov	%edi,%eax		/* Save buffer for return */
	movzbl	%sil,%esi		/* Zero-extend byte value */

	/* Replicate byte to all bytes in ZMM register */
	mov	%esi,%r8d
	shl	$8,%esi
	or	%r8d,%esi		/* 16-bit pattern */
	mov	%esi,%r8d
	shl	$16,%esi
	or	%r8d,%esi		/* 32-bit pattern */

	vmovd	%esi,%xmm0
	vpbroadcastd %xmm0,%zmm0	/* Broadcast to entire ZMM */

	mov	%edx,%ecx		/* Copy length */

	/* Check for small fills */
	cmp	$256,%ecx
	jb	.Lsmall_fill_avx512

	/* Align destination to 64 bytes */
	mov	%edi,%r8d
	and	$63,%r8d
	jz	.Laligned_fill_avx512

	mov	$64,%r9d
	sub	%r8d,%r9d
	sub	%r9d,%ecx

.Lalign_fill_loop_avx512:
	movb	%sil,(%rdi)
	inc	%edi
	dec	%r9d
	jnz	.Lalign_fill_loop_avx512

.Laligned_fill_avx512:
	/* Main AVX-512 fill loop - 256 bytes per iteration (4x ZMM) */
	mov	%ecx,%r9d
	shr	$8,%r9d
	jz	.Lfill_remainder_avx512

.Lavx512_fill_loop:
	vmovdqa64	%zmm0,(%rdi)
	vmovdqa64	%zmm0,64(%rdi)
	vmovdqa64	%zmm0,128(%rdi)
	vmovdqa64	%zmm0,192(%rdi)

	add	$256,%edi
	dec	%r9d
	jnz	.Lavx512_fill_loop

	vzeroupper

.Lfill_remainder_avx512:
	and	$255,%ecx
	jz	.Lfill_done_avx512

	/* Fill remaining 64-byte chunks */
	mov	%ecx,%r9d
	shr	$6,%r9d
	jz	.Lfill_tail_avx512

.Lrem64_fill_loop_avx512:
	vmovdqa64	%zmm0,(%rdi)
	add	$64,%edi
	dec	%r9d
	jnz	.Lrem64_fill_loop_avx512

	vzeroupper
	and	$63,%ecx

.Lfill_tail_avx512:
	test	%ecx,%ecx
	jz	.Lfill_done_avx512
.Lfill_tail_loop_avx512:
	movb	%sil,(%rdi)
	inc	%edi
	dec	%ecx
	jnz	.Lfill_tail_loop_avx512
	jmp	.Lfill_done_avx512

.Lsmall_fill_avx512:
	rep	stosb

.Lfill_done_avx512:
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_AVX512)
