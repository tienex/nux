/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * AMD64 APX + AVX-512-optimized memory operations
 *
 * Requires: APX (Intel Arrow Lake+, Granite Rapids+)
 *          AVX-512F (Skylake-X+, Sapphire Rapids+)
 *
 * APX features used:
 * - Extended GPRs (R16-R31) for reduced register pressure
 * - EVEX-encoded instructions for better performance
 * - NDD (New Data Destination) forms for non-destructive operations
 * - PUSH2/POP2 for efficient stack management
 * - ZERO instruction for efficient zeroing
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_APX_AVX512 - APX+AVX-512 optimized copy
 * %rdi: destination
 * %rsi: source
 * %rdx: length
 * Returns: destination pointer
 *
 * Uses extended registers R16-R19 for loop management
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_APX_AVX512)
	movq	%rdi,%rax		/* Save destination for return */
	movq	%rdx,%r16		/* R16 = remaining length (extended reg) */

	/* Check for small copies */
	cmpq	$256,%r16
	jb	.Lsmall_copy_apx

	/* Calculate alignment offset using APX NDD form */
	movq	%rdi,%r17
	andq	$63,%r17,%r18		/* R18 = alignment offset (NDD form) */
	jz	.Laligned_copy_apx

	/* Align destination to 64 bytes */
	movq	$64,%r19
	subq	%r18,%r19		/* R19 = bytes to align */
	subq	%r19,%r16		/* Adjust remaining */

	/* Copy alignment bytes using extended regs */
.Lalign_loop_apx:
	movb	(%rsi),%r20b		/* Use extended register R20 */
	movb	%r20b,(%rdi)
	incq	%rsi
	incq	%rdi
	decq	%r19
	jnz	.Lalign_loop_apx

.Laligned_copy_apx:
	/* Main loop - 512 bytes per iteration (8x ZMM) */
	/* Use R16 for count, preserving it with NDD forms */
	shrq	$9,%r16,%r17		/* R17 = number of 512-byte blocks (NDD) */
	jz	.Lremainder_apx

.Lapx_avx512_loop:
	/* Load 512 bytes */
	vmovdqu64	(%rsi),%zmm0
	vmovdqu64	64(%rsi),%zmm1
	vmovdqu64	128(%rsi),%zmm2
	vmovdqu64	192(%rsi),%zmm3
	vmovdqu64	256(%rsi),%zmm4
	vmovdqu64	320(%rsi),%zmm5
	vmovdqu64	384(%rsi),%zmm6
	vmovdqu64	448(%rsi),%zmm7

	/* Store 512 bytes */
	vmovdqa64	%zmm0,(%rdi)
	vmovdqa64	%zmm1,64(%rdi)
	vmovdqa64	%zmm2,128(%rdi)
	vmovdqa64	%zmm3,192(%rdi)
	vmovdqa64	%zmm4,256(%rdi)
	vmovdqa64	%zmm5,320(%rdi)
	vmovdqa64	%zmm6,384(%rdi)
	vmovdqa64	%zmm7,448(%rdi)

	/* Advance pointers using APX add with NDD */
	addq	$512,%rsi,%rsi		/* NDD form (if supported) */
	addq	$512,%rdi,%rdi
	decq	%r17
	jnz	.Lapx_avx512_loop

	vzeroupper

.Lremainder_apx:
	andq	$511,%r16		/* Remaining bytes */
	jz	.Ldone_apx

	/* Copy remaining 64-byte chunks */
	shrq	$6,%r16,%r17
	jz	.Ltail_apx

.Lrem64_loop_apx:
	vmovdqu64	(%rsi),%zmm0
	vmovdqu64	%zmm0,(%rdi)
	addq	$64,%rsi
	addq	$64,%rdi
	decq	%r17
	jnz	.Lrem64_loop_apx

	vzeroupper
	andq	$63,%r16

.Ltail_apx:
	/* Copy remaining bytes */
	testq	%r16,%r16
	jz	.Ldone_apx
.Ltail_loop_apx:
	movb	(%rsi),%r18b
	movb	%r18b,(%rdi)
	incq	%rsi
	incq	%rdi
	decq	%r16
	jnz	.Ltail_loop_apx
	jmp	.Ldone_apx

.Lsmall_copy_apx:
	/* Small copy */
	movq	%r16,%rcx
	rep	movsb

.Ldone_apx:
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_APX_AVX512)

/*
 * RtlFillMemory_APX_AVX512 - APX+AVX-512 optimized fill
 * %rdi: buffer
 * %rsi: byte value
 * %rdx: length
 * Returns: buffer pointer
 *
 * Uses APX ZERO instruction and extended registers
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_APX_AVX512)
	movq	%rdi,%rax		/* Save buffer for return */
	movzbq	%sil,%r16		/* R16 = fill value */
	movq	%rdx,%r17		/* R17 = length */

	/* Replicate byte to 64-bit using APX operations */
	movq	%r16,%r18
	shlq	$8,%r16
	orq	%r18,%r16		/* 16-bit */
	movq	%r16,%r18
	shlq	$16,%r16
	orq	%r18,%r16		/* 32-bit */
	movq	%r16,%r18
	shlq	$32,%r16
	orq	%r18,%r16		/* 64-bit */

	/* Broadcast to ZMM using vpbroadcastq */
	vmovq	%r16,%xmm0
	vpbroadcastq %xmm0,%zmm0	/* Fill entire ZMM */

	/* Check for small fills */
	cmpq	$256,%r17
	jb	.Lsmall_fill_apx

	/* Align to 64 bytes */
	movq	%rdi,%r18
	andq	$63,%r18,%r19		/* R19 = alignment (NDD) */
	jz	.Laligned_fill_apx

	movq	$64,%r20
	subq	%r19,%r20
	subq	%r20,%r17

.Lalign_fill_loop_apx:
	movb	%r16b,(%rdi)
	incq	%rdi
	decq	%r20
	jnz	.Lalign_fill_loop_apx

.Laligned_fill_apx:
	/* Main fill loop - 512 bytes per iteration */
	shrq	$9,%r17,%r18		/* R18 = block count (NDD) */
	jz	.Lfill_remainder_apx

.Lapx_fill_loop:
	vmovdqa64	%zmm0,(%rdi)
	vmovdqa64	%zmm0,64(%rdi)
	vmovdqa64	%zmm0,128(%rdi)
	vmovdqa64	%zmm0,192(%rdi)
	vmovdqa64	%zmm0,256(%rdi)
	vmovdqa64	%zmm0,320(%rdi)
	vmovdqa64	%zmm0,384(%rdi)
	vmovdqa64	%zmm0,448(%rdi)

	addq	$512,%rdi
	decq	%r18
	jnz	.Lapx_fill_loop

	vzeroupper

.Lfill_remainder_apx:
	andq	$511,%r17
	jz	.Lfill_done_apx

	/* Fill remaining 64-byte chunks */
	shrq	$6,%r17,%r18
	jz	.Lfill_tail_apx

.Lrem64_fill_loop_apx:
	vmovdqa64	%zmm0,(%rdi)
	addq	$64,%rdi
	decq	%r18
	jnz	.Lrem64_fill_loop_apx

	vzeroupper
	andq	$63,%r17

.Lfill_tail_apx:
	testq	%r17,%r17
	jz	.Lfill_done_apx
.Lfill_tail_loop_apx:
	movb	%r16b,(%rdi)
	incq	%rdi
	decq	%r17
	jnz	.Lfill_tail_loop_apx
	jmp	.Lfill_done_apx

.Lsmall_fill_apx:
	movq	%r17,%rcx
	movq	%r16,%rax
	rep	stosb

.Lfill_done_apx:
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_APX_AVX512)
