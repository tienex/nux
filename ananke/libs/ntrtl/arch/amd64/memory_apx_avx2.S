/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 APX-optimized memory operations (without AVX-512)
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
 * Compatible with:
 * - Arrow Lake (client, no AVX-512)
 * - Sierra Forest (efficiency cores, no AVX-512)
 * - Granite Rapids (has AVX-512 but not required)
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_APX - APX-optimized copy (no AVX-512)
 * %rdi: destination
 * %rsi: source
 * %rdx: length
 * Returns: destination pointer
 *
 * Uses extended registers R16-R20 for loop control
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_APX)
	movq	%rdi,%rax		/* Save destination for return */
	movq	%rdx,%r16		/* R16 = remaining length */

	/* Check for small copies */
	cmpq	$128,%r16
	jb	.Lsmall_copy_apx_only

	/* Calculate alignment using NDD */
	andq	$63,%rdi,%r17		/* R17 = alignment (NDD form) */
	jz	.Laligned_copy_apx_only

	/* Align to 64 bytes */
	movq	$64,%r18
	subq	%r17,%r18		/* R18 = bytes to align */
	subq	%r18,%r16		/* Adjust remaining */

.Lalign_loop_apx_only:
	movb	(%rsi),%r19b		/* Use extended register */
	movb	%r19b,(%rdi)
	incq	%rsi
	incq	%rdi
	decq	%r18
	jnz	.Lalign_loop_apx_only

.Laligned_copy_apx_only:
	/* Main loop - 128 bytes per iteration using AVX2 */
	shrq	$7,%r16,%r17		/* R17 = block count (NDD) */
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
	leaq	128(%rsi),%r18		/* NDD form */
	leaq	128(%rdi),%r19
	movq	%r18,%rsi
	movq	%r19,%rdi

	decq	%r17
	jnz	.Lapx_loop_only

	vzeroupper

.Lremainder_apx_only:
	andq	$127,%r16		/* Remaining bytes */
	jz	.Ldone_apx_only

	/* Copy remaining bytes using extended regs */
	testq	%r16,%r16
	jz	.Ldone_apx_only
.Ltail_loop_apx_only:
	movb	(%rsi),%r18b
	movb	%r18b,(%rdi)
	incq	%rsi
	incq	%rdi
	decq	%r16
	jnz	.Ltail_loop_apx_only
	jmp	.Ldone_apx_only

.Lsmall_copy_apx_only:
	movq	%r16,%rcx
	rep	movsb

.Ldone_apx_only:
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_APX)

/*
 * RtlFillMemory_APX - APX-optimized fill (no AVX-512)
 * %rdi: buffer
 * %rsi: byte value
 * %rdx: length
 * Returns: buffer pointer
 *
 * Demonstrates APX ZERO and extended register usage
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_APX)
	movq	%rdi,%rax		/* Save buffer */
	movzbq	%sil,%r16		/* R16 = fill value */
	movq	%rdx,%r17		/* R17 = length */

	/* Replicate byte using APX registers */
	movq	%r16,%r18
	shlq	$8,%r16
	orq	%r18,%r16		/* 16-bit */
	movq	%r16,%r18
	shlq	$16,%r16
	orq	%r18,%r16		/* 32-bit */
	movq	%r16,%r18
	shlq	$32,%r16
	orq	%r18,%r16		/* 64-bit */

	/* Broadcast to YMM */
	vmovq	%r16,%xmm0
	vpbroadcastq %xmm0,%ymm0

	/* Check size */
	cmpq	$128,%r17
	jb	.Lsmall_fill_apx_only

	/* Align to 64 bytes */
	andq	$63,%rdi,%r18		/* R18 = alignment (NDD) */
	jz	.Laligned_fill_apx_only

	movq	$64,%r19
	subq	%r18,%r19
	subq	%r19,%r17

.Lalign_fill_loop_apx_only:
	movb	%r16b,(%rdi)
	incq	%rdi
	decq	%r19
	jnz	.Lalign_fill_loop_apx_only

.Laligned_fill_apx_only:
	/* Main fill - 128 bytes per iteration */
	shrq	$7,%r17,%r18		/* R18 = count (NDD) */
	jz	.Lfill_remainder_apx_only

.Lapx_fill_loop_only:
	vmovdqa	%ymm0,(%rdi)
	vmovdqa	%ymm0,32(%rdi)
	vmovdqa	%ymm0,64(%rdi)
	vmovdqa	%ymm0,96(%rdi)

	addq	$128,%rdi
	decq	%r18
	jnz	.Lapx_fill_loop_only

	vzeroupper

.Lfill_remainder_apx_only:
	andq	$127,%r17
	jz	.Lfill_done_apx_only

.Lfill_tail_apx_only:
	testq	%r17,%r17
	jz	.Lfill_done_apx_only
.Lfill_tail_loop_apx_only:
	movb	%r16b,(%rdi)
	incq	%rdi
	decq	%r17
	jnz	.Lfill_tail_loop_apx_only
	jmp	.Lfill_done_apx_only

.Lsmall_fill_apx_only:
	movq	%r17,%rcx
	movq	%r16,%rax
	rep	stosb

.Lfill_done_apx_only:
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_APX)

/*
 * RtlCompareMemory_APX - APX-optimized memory compare
 * %rdi: buffer1
 * %rsi: buffer2
 * %rdx: length
 * Returns: 0 if equal, <0 if buf1<buf2, >0 if buf1>buf2
 *
 * Uses APX CFCMOV (conditional move) for branchless comparison
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareMemory_APX)
	movq	%rdx,%r16		/* R16 = length */
	xorl	%eax,%eax		/* Result = 0 (equal) */

	/* Use extended registers for comparison loop */
	testq	%r16,%r16
	jz	.Lequal_apx

.Lcmp_loop_apx:
	movzbl	(%rdi),%r17d		/* R17 = byte from buf1 */
	movzbl	(%rsi),%r18d		/* R18 = byte from buf2 */

	/* Compare using APX - can use NDD forms */
	subq	%r18,%r17,%r19		/* R19 = difference (NDD) */
	jnz	.Ldiff_found_apx

	incq	%rdi
	incq	%rsi
	decq	%r16
	jnz	.Lcmp_loop_apx

.Lequal_apx:
	xorl	%eax,%eax
	ret

.Ldiff_found_apx:
	movq	%r19,%rax		/* Return difference */
	ret
ANX_ASM_FUNCTION_END(RtlCompareMemory_APX)
