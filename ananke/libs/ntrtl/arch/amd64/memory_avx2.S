/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 AVX2-optimized memory operations
 *
 * Requires: AVX2 (Haswell and later, Zen and later)
 * Uses 256-bit YMM registers for maximum throughput
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_AVX2 - AVX2-optimized memory copy
 * %rdi: destination
 * %rsi: source
 * %rdx: length
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_AVX2)
	movq	%rdi,%rax		/* Save destination for return */
	movq	%rdx,%rcx		/* Copy length */

	/* Check for small copies */
	cmpq	$128,%rcx
	jb	.Lsmall_copy_avx2

	/* Check alignment */
	movq	%rdi,%r8
	andq	$31,%r8			/* Check if destination is 32-byte aligned */
	jz	.Laligned_copy_avx2

	/* Align destination to 32 bytes */
	movq	$32,%r9
	subq	%r8,%r9			/* Bytes to align */
	subq	%r9,%rcx		/* Adjust remaining count */

	/* Copy alignment bytes */
.Lalign_loop_avx2:
	movb	(%rsi),%r8b
	movb	%r8b,(%rdi)
	incq	%rsi
	incq	%rdi
	decq	%r9
	jnz	.Lalign_loop_avx2

.Laligned_copy_avx2:
	/* Main AVX2 copy loop - 128 bytes per iteration */
	movq	%rcx,%r9
	shrq	$7,%r9			/* Number of 128-byte blocks */
	jz	.Lremainder_avx2

.Lavx2_loop:
	vmovdqu	(%rsi),%ymm0
	vmovdqu	32(%rsi),%ymm1
	vmovdqu	64(%rsi),%ymm2
	vmovdqu	96(%rsi),%ymm3

	vmovdqa	%ymm0,(%rdi)
	vmovdqa	%ymm1,32(%rdi)
	vmovdqa	%ymm2,64(%rdi)
	vmovdqa	%ymm3,96(%rdi)

	addq	$128,%rsi
	addq	$128,%rdi
	decq	%r9
	jnz	.Lavx2_loop

	vzeroupper			/* Clear upper YMM bits */

.Lremainder_avx2:
	andq	$127,%rcx		/* Remaining bytes */
	jz	.Ldone_avx2

	/* Copy remaining 32-byte chunks */
	movq	%rcx,%r9
	shrq	$5,%r9
	jz	.Ltail_avx2

.Lrem32_loop_avx2:
	vmovdqu	(%rsi),%ymm0
	vmovdqu	%ymm0,(%rdi)
	addq	$32,%rsi
	addq	$32,%rdi
	decq	%r9
	jnz	.Lrem32_loop_avx2

	vzeroupper
	andq	$31,%rcx

.Ltail_avx2:
	/* Copy remaining bytes */
	testq	%rcx,%rcx
	jz	.Ldone_avx2
.Ltail_loop_avx2:
	movb	(%rsi),%r8b
	movb	%r8b,(%rdi)
	incq	%rsi
	incq	%rdi
	decq	%rcx
	jnz	.Ltail_loop_avx2
	jmp	.Ldone_avx2

.Lsmall_copy_avx2:
	/* Small copy without AVX2 */
	rep	movsb

.Ldone_avx2:
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_AVX2)

/*
 * RtlFillMemory_AVX2 - AVX2-optimized memory fill
 * %rdi: buffer
 * %rsi: byte value
 * %rdx: length
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_AVX2)
	movq	%rdi,%rax		/* Save buffer for return */
	movzbq	%sil,%rsi		/* Zero-extend byte value */

	/* Replicate byte to all bytes in YMM register */
	movq	%rsi,%r8
	shlq	$8,%rsi
	orq	%r8,%rsi		/* 16-bit pattern */
	movq	%rsi,%r8
	shlq	$16,%rsi
	orq	%r8,%rsi		/* 32-bit pattern */
	movq	%rsi,%r8
	shlq	$32,%rsi
	orq	%r8,%rsi		/* 64-bit pattern */

	vmovq	%rsi,%xmm0
	vpbroadcastq %xmm0,%ymm0	/* Broadcast to entire YMM */

	movq	%rdx,%rcx		/* Copy length */

	/* Check for small fills */
	cmpq	$128,%rcx
	jb	.Lsmall_fill_avx2

	/* Align destination to 32 bytes */
	movq	%rdi,%r8
	andq	$31,%r8
	jz	.Laligned_fill_avx2

	movq	$32,%r9
	subq	%r8,%r9
	subq	%r9,%rcx

.Lalign_fill_loop_avx2:
	movb	%sil,(%rdi)
	incq	%rdi
	decq	%r9
	jnz	.Lalign_fill_loop_avx2

.Laligned_fill_avx2:
	/* Main AVX2 fill loop - 128 bytes per iteration */
	movq	%rcx,%r9
	shrq	$7,%r9
	jz	.Lfill_remainder_avx2

.Lavx2_fill_loop:
	vmovdqa	%ymm0,(%rdi)
	vmovdqa	%ymm0,32(%rdi)
	vmovdqa	%ymm0,64(%rdi)
	vmovdqa	%ymm0,96(%rdi)

	addq	$128,%rdi
	decq	%r9
	jnz	.Lavx2_fill_loop

	vzeroupper

.Lfill_remainder_avx2:
	andq	$127,%rcx
	jz	.Lfill_done_avx2

	/* Fill remaining 32-byte chunks */
	movq	%rcx,%r9
	shrq	$5,%r9
	jz	.Lfill_tail_avx2

.Lrem32_fill_loop_avx2:
	vmovdqa	%ymm0,(%rdi)
	addq	$32,%rdi
	decq	%r9
	jnz	.Lrem32_fill_loop_avx2

	vzeroupper
	andq	$31,%rcx

.Lfill_tail_avx2:
	testq	%rcx,%rcx
	jz	.Lfill_done_avx2
.Lfill_tail_loop_avx2:
	movb	%sil,(%rdi)
	incq	%rdi
	decq	%rcx
	jnz	.Lfill_tail_loop_avx2
	jmp	.Lfill_done_avx2

.Lsmall_fill_avx2:
	rep	stosb

.Lfill_done_avx2:
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_AVX2)
