/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 AVX2-optimized memory operations
 *
 * Requires: AVX2 (Haswell and later, available in 32-bit mode)
 *
 * AVX2 provides:
 * - 256-bit YMM registers
 * - Integer operations on 256-bit vectors
 * - Gather operations
 * - Broadcast instructions
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_AVX2 - AVX2-optimized memory copy
 * 4(%esp): destination (after push)
 * 8(%esp): source (after push)
 * 12(%esp): length (after push)
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_AVX2)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi		/* destination */
	movl	16(%esp),%esi		/* source */
	movl	20(%esp),%ecx		/* length */

	movl	%edi,%eax		/* Save for return */

	/* Check for small copies */
	cmpl	$128,%ecx
	jb	.Lsmall_copy_avx2_i386

	/* Main loop - 128 bytes per iteration */
	movl	%ecx,%edx
	shrl	$7,%edx			/* edx = blocks of 128 */
	jz	.Lremainder_avx2_i386

.Lloop_avx2_i386:
	vmovdqu	(%esi),%ymm0
	vmovdqu	32(%esi),%ymm1
	vmovdqu	64(%esi),%ymm2
	vmovdqu	96(%esi),%ymm3

	vmovdqa	%ymm0,(%edi)
	vmovdqa	%ymm1,32(%edi)
	vmovdqa	%ymm2,64(%edi)
	vmovdqa	%ymm3,96(%edi)

	addl	$128,%esi
	addl	$128,%edi
	decl	%edx
	jnz	.Lloop_avx2_i386

	vzeroupper

.Lremainder_avx2_i386:
	andl	$127,%ecx
	jz	.Ldone_avx2_i386

	/* Copy remaining bytes */
	rep	movsb
	jmp	.Ldone_avx2_i386

.Lsmall_copy_avx2_i386:
	rep	movsb

.Ldone_avx2_i386:
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_AVX2)

/*
 * RtlFillMemory_AVX2 - AVX2-optimized memory fill
 * 4(%esp): buffer (after push)
 * 8(%esp): byte value (after push)
 * 12(%esp): length (after push)
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_AVX2)
	pushl	%edi

	movl	8(%esp),%edi		/* buffer */
	movzbl	12(%esp),%eax		/* byte value */
	movl	16(%esp),%ecx		/* length */

	movl	%edi,%edx		/* Save for return */

	/* Replicate byte to 32-bit */
	movl	%eax,%esi
	shll	$8,%eax
	orl	%esi,%eax
	movl	%eax,%esi
	shll	$16,%eax
	orl	%esi,%eax

	/* Broadcast to YMM */
	vmovd	%eax,%xmm0
	vpbroadcastd %xmm0,%ymm0

	/* Check size */
	cmpl	$128,%ecx
	jb	.Lsmall_fill_avx2_i386

	/* Main fill - 128 bytes per iteration */
	movl	%ecx,%esi
	shrl	$7,%esi
	jz	.Lfill_remainder_avx2_i386

.Lfill_loop_avx2_i386:
	vmovdqa	%ymm0,(%edi)
	vmovdqa	%ymm0,32(%edi)
	vmovdqa	%ymm0,64(%edi)
	vmovdqa	%ymm0,96(%edi)

	addl	$128,%edi
	decl	%esi
	jnz	.Lfill_loop_avx2_i386

	vzeroupper

.Lfill_remainder_avx2_i386:
	andl	$127,%ecx
	jz	.Lfill_done_avx2_i386

	rep	stosb
	jmp	.Lfill_done_avx2_i386

.Lsmall_fill_avx2_i386:
	rep	stosb

.Lfill_done_avx2_i386:
	movl	%edx,%eax
	popl	%edi
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_AVX2)
