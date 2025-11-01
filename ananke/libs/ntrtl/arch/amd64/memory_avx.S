/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 AVX-optimized memory operations
 *
 * Requires: AVX (Sandy Bridge+, Bulldozer+)
 * Uses 256-bit YMM registers with floating-point move instructions
 * Note: AVX lacks integer SIMD, so uses vmov* instead of vpbroadcast
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_AVX - AVX-optimized memory copy
 * %rdi: destination
 * %rsi: source
 * %rdx: length
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_AVX)
	movq	%rdi,%rax		/* Save destination for return */
	movq	%rdx,%rcx		/* Copy length */

	/* Check for small copies */
	cmpq	$128,%rcx
	jb	.Lsmall_copy_avx

	/* Check alignment */
	movq	%rdi,%r8
	andq	$31,%r8			/* Check if destination is 32-byte aligned */
	jz	.Laligned_copy_avx

	/* Align destination to 32 bytes */
	movq	$32,%r9
	subq	%r8,%r9			/* Bytes to align */
	subq	%r9,%rcx		/* Adjust remaining count */

	/* Copy alignment bytes */
.Lalign_loop_avx:
	movb	(%rsi),%r8b
	movb	%r8b,(%rdi)
	incq	%rsi
	incq	%rdi
	decq	%r9
	jnz	.Lalign_loop_avx

.Laligned_copy_avx:
	/* Main AVX copy loop - 128 bytes per iteration */
	movq	%rcx,%r9
	shrq	$7,%r9			/* Number of 128-byte blocks */
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

	addq	$128,%rsi
	addq	$128,%rdi
	decq	%r9
	jnz	.Lavx_loop

	vzeroupper			/* Clear upper YMM bits */

.Lremainder_avx:
	andq	$127,%rcx		/* Remaining bytes */
	jz	.Ldone_avx

	/* Copy remaining 32-byte chunks */
	movq	%rcx,%r9
	shrq	$5,%r9
	jz	.Ltail_avx

.Lrem32_loop_avx:
	vmovdqu	(%rsi),%ymm0
	vmovdqu	%ymm0,(%rdi)
	addq	$32,%rsi
	addq	$32,%rdi
	decq	%r9
	jnz	.Lrem32_loop_avx

	vzeroupper
	andq	$31,%rcx

.Ltail_avx:
	/* Copy remaining bytes */
	testq	%rcx,%rcx
	jz	.Ldone_avx
.Ltail_loop_avx:
	movb	(%rsi),%r8b
	movb	%r8b,(%rdi)
	incq	%rsi
	incq	%rdi
	decq	%rcx
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
 * %rdi: buffer
 * %rsi: byte value
 * %rdx: length
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_AVX)
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

	/* Move to XMM then broadcast to YMM using vinsertf128 */
	vmovq	%rsi,%xmm0
	vinsertf128 $1,%xmm0,%ymm0,%ymm0  /* Duplicate lower half to upper */

	movq	%rdx,%rcx		/* Copy length */

	/* Check for small fills */
	cmpq	$128,%rcx
	jb	.Lsmall_fill_avx

	/* Align destination to 32 bytes */
	movq	%rdi,%r8
	andq	$31,%r8
	jz	.Laligned_fill_avx

	movq	$32,%r9
	subq	%r8,%r9
	subq	%r9,%rcx

.Lalign_fill_loop_avx:
	movb	%sil,(%rdi)
	incq	%rdi
	decq	%r9
	jnz	.Lalign_fill_loop_avx

.Laligned_fill_avx:
	/* Main AVX fill loop - 128 bytes per iteration */
	movq	%rcx,%r9
	shrq	$7,%r9
	jz	.Lfill_remainder_avx

.Lavx_fill_loop:
	vmovdqa	%ymm0,(%rdi)
	vmovdqa	%ymm0,32(%rdi)
	vmovdqa	%ymm0,64(%rdi)
	vmovdqa	%ymm0,96(%rdi)

	addq	$128,%rdi
	decq	%r9
	jnz	.Lavx_fill_loop

	vzeroupper

.Lfill_remainder_avx:
	andq	$127,%rcx
	jz	.Lfill_done_avx

	/* Fill remaining 32-byte chunks */
	movq	%rcx,%r9
	shrq	$5,%r9
	jz	.Lfill_tail_avx

.Lrem32_fill_loop_avx:
	vmovdqa	%ymm0,(%rdi)
	addq	$32,%rdi
	decq	%r9
	jnz	.Lrem32_fill_loop_avx

	vzeroupper
	andq	$31,%rcx

.Lfill_tail_avx:
	testq	%rcx,%rcx
	jz	.Lfill_done_avx
.Lfill_tail_loop_avx:
	movb	%sil,(%rdi)
	incq	%rdi
	decq	%rcx
	jnz	.Lfill_tail_loop_avx
	jmp	.Lfill_done_avx

.Lsmall_fill_avx:
	rep	stosb

.Lfill_done_avx:
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_AVX)
