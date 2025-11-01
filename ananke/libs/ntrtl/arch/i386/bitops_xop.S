/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * i386 XOP-optimized bit operations
 *
 * Requires: XOP (AMD Bulldozer, Piledriver, Steamroller, Excavator)
 * XOP is available in both 32-bit and 64-bit modes
 * Note: Removed in Zen - legacy AMD FX/A-series support
 */

#include <ananke/asmdefs.h>

/*
 * RtlRotateLeft32_XOP - Rotate left 32-bit value
 * 4(%esp): value
 * 8(%esp): rotation count
 * Returns: rotated value
 */
ANX_ASM_FUNCTION_BEGIN(RtlRotateLeft32_XOP)
	movl	4(%esp),%eax		/* Load value */
	movl	8(%esp),%ecx		/* Load count */

	vmovd	%eax,%xmm0		/* Move value to XMM */
	vmovd	%ecx,%xmm1		/* Move count to XMM */
	vprotd	%xmm1,%xmm0,%xmm0	/* Rotate */
	vmovd	%xmm0,%eax		/* Move result back */
	ret
ANX_ASM_FUNCTION_END(RtlRotateLeft32_XOP)

/*
 * RtlRotateRight32_XOP - Rotate right 32-bit value
 * 4(%esp): value
 * 8(%esp): rotation count
 * Returns: rotated value
 */
ANX_ASM_FUNCTION_BEGIN(RtlRotateRight32_XOP)
	movl	4(%esp),%eax		/* Load value */
	movl	8(%esp),%ecx		/* Load count */
	negl	%ecx			/* Negate for right rotation */

	vmovd	%eax,%xmm0		/* Move value to XMM */
	vmovd	%ecx,%xmm1		/* Move negated count to XMM */
	vprotd	%xmm1,%xmm0,%xmm0	/* Rotate (negative = right) */
	vmovd	%xmm0,%eax		/* Move result back */
	ret
ANX_ASM_FUNCTION_END(RtlRotateRight32_XOP)

/*
 * RtlShiftLeftVariable_XOP - Variable shift left (bytes)
 * 4(%esp): value (low 32 bits)
 * 8(%esp): shift counts (packed bytes)
 * Returns: shifted value
 */
ANX_ASM_FUNCTION_BEGIN(RtlShiftLeftVariable_XOP)
	movl	4(%esp),%eax		/* Load value */
	movl	8(%esp),%ecx		/* Load shift counts */

	vmovd	%eax,%xmm0		/* Move value to XMM */
	vmovd	%ecx,%xmm1		/* Move shift counts to XMM */
	vpshlb	%xmm1,%xmm0,%xmm0	/* Variable logical shift left */
	vmovd	%xmm0,%eax		/* Move result back */
	ret
ANX_ASM_FUNCTION_END(RtlShiftLeftVariable_XOP)

/*
 * RtlShiftRightVariable_XOP - Variable shift right (bytes, logical)
 * 4(%esp): value
 * 8(%esp): shift counts (packed bytes)
 * Returns: shifted value
 */
ANX_ASM_FUNCTION_BEGIN(RtlShiftRightVariable_XOP)
	movl	4(%esp),%eax		/* Load value */
	movl	8(%esp),%ecx		/* Load shift counts */

	vmovd	%eax,%xmm0		/* Move value to XMM */
	vmovd	%ecx,%xmm1		/* Move shift counts to XMM */
	vnegb	%xmm1,%xmm1		/* Negate for right shift */
	vpshlb	%xmm1,%xmm0,%xmm0	/* Variable logical shift (neg = right) */
	vmovd	%xmm0,%eax		/* Move result back */
	ret
ANX_ASM_FUNCTION_END(RtlShiftRightVariable_XOP)

/*
 * RtlCompareBytes_XOP - Compare bytes and generate mask
 * 4(%esp): value1
 * 8(%esp): value2
 * 12(%esp): comparison mode
 * Returns: mask (0xFF where true, 0x00 where false)
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareBytes_XOP)
	pushl	%ebx

	movl	8(%esp),%eax		/* Load value1 (after push) */
	movl	12(%esp),%ecx		/* Load value2 */
	movl	16(%esp),%edx		/* Load comparison mode */

	vmovd	%eax,%xmm0		/* Move value1 to XMM */
	vmovd	%ecx,%xmm1		/* Move value2 to XMM */

	/* Select comparison mode */
	cmpl	$0,%edx
	je	.Lcmp_lt_i386
	cmpl	$1,%edx
	je	.Lcmp_le_i386
	cmpl	$2,%edx
	je	.Lcmp_gt_i386
	cmpl	$3,%edx
	je	.Lcmp_ge_i386
	cmpl	$4,%edx
	je	.Lcmp_eq_i386
	cmpl	$5,%edx
	je	.Lcmp_ne_i386
	/* Default to FALSE */
	vpxor	%xmm0,%xmm0,%xmm0
	jmp	.Lcmp_done_i386

.Lcmp_lt_i386:
	vpcomb	$0,%xmm1,%xmm0,%xmm0	/* Less than */
	jmp	.Lcmp_done_i386
.Lcmp_le_i386:
	vpcomb	$1,%xmm1,%xmm0,%xmm0	/* Less than or equal */
	jmp	.Lcmp_done_i386
.Lcmp_gt_i386:
	vpcomb	$2,%xmm1,%xmm0,%xmm0	/* Greater than */
	jmp	.Lcmp_done_i386
.Lcmp_ge_i386:
	vpcomb	$3,%xmm1,%xmm0,%xmm0	/* Greater than or equal */
	jmp	.Lcmp_done_i386
.Lcmp_eq_i386:
	vpcomb	$4,%xmm1,%xmm0,%xmm0	/* Equal */
	jmp	.Lcmp_done_i386
.Lcmp_ne_i386:
	vpcomb	$5,%xmm1,%xmm0,%xmm0	/* Not equal */

.Lcmp_done_i386:
	vmovd	%xmm0,%eax		/* Move result back */
	popl	%ebx
	ret
ANX_ASM_FUNCTION_END(RtlCompareBytes_XOP)

/*
 * RtlRotateBytes_XOP - Rotate bytes in parallel
 * 4(%esp): value (4 bytes to rotate)
 * 8(%esp): rotation counts (one count per byte)
 * Returns: rotated bytes
 *
 * Each byte is rotated independently
 */
ANX_ASM_FUNCTION_BEGIN(RtlRotateBytes_XOP)
	movl	4(%esp),%eax		/* Load value */
	movl	8(%esp),%ecx		/* Load rotation counts */

	vmovd	%eax,%xmm0		/* Move value to XMM */
	vmovd	%ecx,%xmm1		/* Move counts to XMM */
	vprotb	%xmm1,%xmm0,%xmm0	/* Rotate each byte */
	vmovd	%xmm0,%eax		/* Move result back */
	ret
ANX_ASM_FUNCTION_END(RtlRotateBytes_XOP)
