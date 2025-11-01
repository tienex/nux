/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 AVX-optimized memory operations
 *
 * Requires: AVX (Sandy Bridge+, Bulldozer+)
 * Uses 256-bit YMM registers in 32-bit mode
 * Note: AVX is available in 32-bit mode on supported CPUs
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_AVX - AVX-optimized memory copy
 * 4(%esp): destination
 * 8(%esp): source
 * 12(%esp): length
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_AVX)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi		/* Destination (after push) */
	movl	16(%esp),%esi		/* Source (after push) */
	movl	20(%esp),%ecx		/* Length (after push) */

	movl	%edi,%eax		/* Save destination for return */

	/* Check for small copies */
	cmpl	$128,%ecx
	jb	.Lsmall_copy_avx_i386

	/* Check alignment */
	movl	%edi,%edx
	andl	$31,%edx		/* Check if destination is 32-byte aligned */
	jz	.Laligned_copy_avx_i386

	/* Align destination to 32 bytes */
	movl	$32,%ebx
	subl	%edx,%ebx		/* Bytes to align */
	subl	%ebx,%ecx		/* Adjust remaining count */

	/* Copy alignment bytes */
.Lalign_loop_avx_i386:
	movb	(%esi),%dl
	movb	%dl,(%edi)
	incl	%esi
	incl	%edi
	decl	%ebx
	jnz	.Lalign_loop_avx_i386

.Laligned_copy_avx_i386:
	/* Main AVX copy loop - 128 bytes per iteration */
	movl	%ecx,%edx
	shrl	$7,%edx			/* Number of 128-byte blocks */
	jz	.Lremainder_avx_i386

.Lavx_loop_i386:
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
	jnz	.Lavx_loop_i386

	vzeroupper			/* Clear upper YMM bits */

.Lremainder_avx_i386:
	andl	$127,%ecx		/* Remaining bytes */
	jz	.Ldone_avx_i386

	/* Copy remaining 32-byte chunks */
	movl	%ecx,%edx
	shrl	$5,%edx
	jz	.Ltail_avx_i386

.Lrem32_loop_avx_i386:
	vmovdqu	(%esi),%ymm0
	vmovdqu	%ymm0,(%edi)
	addl	$32,%esi
	addl	$32,%edi
	decl	%edx
	jnz	.Lrem32_loop_avx_i386

	vzeroupper
	andl	$31,%ecx

.Ltail_avx_i386:
	/* Copy remaining bytes */
	testl	%ecx,%ecx
	jz	.Ldone_avx_i386
	rep	movsb
	jmp	.Ldone_avx_i386

.Lsmall_copy_avx_i386:
	/* Small copy without AVX */
	rep	movsb

.Ldone_avx_i386:
	popl	%edi
	popl	%esi
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_AVX)

/*
 * RtlFillMemory_AVX - AVX-optimized memory fill
 * 4(%esp): buffer
 * 8(%esp): length
 * 12(%esp): byte value
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_AVX)
	pushl	%edi

	movl	8(%esp),%edi		/* Buffer (after push) */
	movl	12(%esp),%ecx		/* Length (after push) */
	movzbl	16(%esp),%eax		/* Byte value (after push) */

	movl	%edi,%edx		/* Save buffer for return */

	/* Replicate byte to all bytes in YMM register */
	movl	%eax,%ebx
	shll	$8,%eax
	orl	%ebx,%eax		/* 16-bit pattern */
	movl	%eax,%ebx
	shll	$16,%eax
	orl	%ebx,%eax		/* 32-bit pattern */

	/* Move to XMM then broadcast to YMM using vinsertf128 */
	vmovd	%eax,%xmm0
	vpshufd	$0,%xmm0,%xmm0		/* Broadcast within XMM */
	vinsertf128 $1,%xmm0,%ymm0,%ymm0  /* Duplicate to upper half */

	/* Check for small fills */
	cmpl	$128,%ecx
	jb	.Lsmall_fill_avx_i386

	/* Align destination to 32 bytes */
	movl	%edi,%ebx
	andl	$31,%ebx
	jz	.Laligned_fill_avx_i386

	movl	$32,%ebx
	subl	%ebx,%ebx
	subl	%ebx,%ecx

	movzbl	16(%esp),%eax		/* Reload byte for stosb */
.Lalign_fill_loop_avx_i386:
	stosb
	decl	%ebx
	jnz	.Lalign_fill_loop_avx_i386

.Laligned_fill_avx_i386:
	/* Main AVX fill loop - 128 bytes per iteration */
	pushl	%ecx
	shrl	$7,%ecx
	jz	.Lfill_remainder_avx_i386

.Lavx_fill_loop_i386:
	vmovdqa	%ymm0,(%edi)
	vmovdqa	%ymm0,32(%edi)
	vmovdqa	%ymm0,64(%edi)
	vmovdqa	%ymm0,96(%edi)

	addl	$128,%edi
	decl	%ecx
	jnz	.Lavx_fill_loop_i386

	vzeroupper

.Lfill_remainder_avx_i386:
	popl	%ecx
	andl	$127,%ecx
	jz	.Lfill_done_avx_i386

	/* Fill remaining 32-byte chunks */
	pushl	%ecx
	shrl	$5,%ecx
	jz	.Lfill_tail_avx_i386

.Lrem32_fill_loop_avx_i386:
	vmovdqa	%ymm0,(%edi)
	addl	$32,%edi
	decl	%ecx
	jnz	.Lrem32_fill_loop_avx_i386

	vzeroupper
	popl	%ecx
	andl	$31,%ecx
	jmp	.Lfill_tail_bytes_avx_i386

.Lfill_tail_avx_i386:
	popl	%ecx

.Lfill_tail_bytes_avx_i386:
	testl	%ecx,%ecx
	jz	.Lfill_done_avx_i386
	movzbl	16(%esp),%eax
	rep	stosb
	jmp	.Lfill_done_avx_i386

.Lsmall_fill_avx_i386:
	movzbl	16(%esp),%eax
	rep	stosb

.Lfill_done_avx_i386:
	movl	%edx,%eax		/* Return buffer */
	popl	%edi
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_AVX)
