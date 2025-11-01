/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 AVX-512-optimized memory operations
 *
 * Requires: AVX-512F (Skylake-X and later, Zen 4 and later)
 * Uses 512-bit ZMM registers for maximum throughput
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_AVX512 - AVX-512-optimized memory copy
 * %rdi: destination
 * %rsi: source
 * %rdx: length
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_AVX512)
	movq	%rdi,%rax		/* Save destination for return */
	movq	%rdx,%rcx		/* Copy length */

	/* Check for small copies */
	cmpq	$256,%rcx
	jb	.Lsmall_copy_avx512

	/* Check alignment */
	movq	%rdi,%r8
	andq	$63,%r8			/* Check if destination is 64-byte aligned */
	jz	.Laligned_copy_avx512

	/* Align destination to 64 bytes */
	movq	$64,%r9
	subq	%r8,%r9			/* Bytes to align */
	subq	%r9,%rcx		/* Adjust remaining count */

	/* Copy alignment bytes */
.Lalign_loop_avx512:
	movb	(%rsi),%r8b
	movb	%r8b,(%rdi)
	incq	%rsi
	incq	%rdi
	decq	%r9
	jnz	.Lalign_loop_avx512

.Laligned_copy_avx512:
	/* Main AVX-512 copy loop - 256 bytes per iteration (4x ZMM) */
	movq	%rcx,%r9
	shrq	$8,%r9			/* Number of 256-byte blocks */
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

	addq	$256,%rsi
	addq	$256,%rdi
	decq	%r9
	jnz	.Lavx512_loop

	vzeroupper			/* Clear upper ZMM bits */

.Lremainder_avx512:
	andq	$255,%rcx		/* Remaining bytes */
	jz	.Ldone_avx512

	/* Copy remaining 64-byte chunks */
	movq	%rcx,%r9
	shrq	$6,%r9
	jz	.Ltail_avx512

.Lrem64_loop_avx512:
	vmovdqu64	(%rsi),%zmm0
	vmovdqu64	%zmm0,(%rdi)
	addq	$64,%rsi
	addq	$64,%rdi
	decq	%r9
	jnz	.Lrem64_loop_avx512

	vzeroupper
	andq	$63,%rcx

.Ltail_avx512:
	/* Copy remaining bytes */
	testq	%rcx,%rcx
	jz	.Ldone_avx512
.Ltail_loop_avx512:
	movb	(%rsi),%r8b
	movb	%r8b,(%rdi)
	incq	%rsi
	incq	%rdi
	decq	%rcx
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
 * %rdi: buffer
 * %rsi: byte value
 * %rdx: length
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_AVX512)
	movq	%rdi,%rax		/* Save buffer for return */
	movzbq	%sil,%rsi		/* Zero-extend byte value */

	/* Replicate byte to all bytes in ZMM register */
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
	vpbroadcastq %xmm0,%zmm0	/* Broadcast to entire ZMM */

	movq	%rdx,%rcx		/* Copy length */

	/* Check for small fills */
	cmpq	$256,%rcx
	jb	.Lsmall_fill_avx512

	/* Align destination to 64 bytes */
	movq	%rdi,%r8
	andq	$63,%r8
	jz	.Laligned_fill_avx512

	movq	$64,%r9
	subq	%r8,%r9
	subq	%r9,%rcx

.Lalign_fill_loop_avx512:
	movb	%sil,(%rdi)
	incq	%rdi
	decq	%r9
	jnz	.Lalign_fill_loop_avx512

.Laligned_fill_avx512:
	/* Main AVX-512 fill loop - 256 bytes per iteration (4x ZMM) */
	movq	%rcx,%r9
	shrq	$8,%r9
	jz	.Lfill_remainder_avx512

.Lavx512_fill_loop:
	vmovdqa64	%zmm0,(%rdi)
	vmovdqa64	%zmm0,64(%rdi)
	vmovdqa64	%zmm0,128(%rdi)
	vmovdqa64	%zmm0,192(%rdi)

	addq	$256,%rdi
	decq	%r9
	jnz	.Lavx512_fill_loop

	vzeroupper

.Lfill_remainder_avx512:
	andq	$255,%rcx
	jz	.Lfill_done_avx512

	/* Fill remaining 64-byte chunks */
	movq	%rcx,%r9
	shrq	$6,%r9
	jz	.Lfill_tail_avx512

.Lrem64_fill_loop_avx512:
	vmovdqa64	%zmm0,(%rdi)
	addq	$64,%rdi
	decq	%r9
	jnz	.Lrem64_fill_loop_avx512

	vzeroupper
	andq	$63,%rcx

.Lfill_tail_avx512:
	testq	%rcx,%rcx
	jz	.Lfill_done_avx512
.Lfill_tail_loop_avx512:
	movb	%sil,(%rdi)
	incq	%rdi
	decq	%rcx
	jnz	.Lfill_tail_loop_avx512
	jmp	.Lfill_done_avx512

.Lsmall_fill_avx512:
	rep	stosb

.Lfill_done_avx512:
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_AVX512)
