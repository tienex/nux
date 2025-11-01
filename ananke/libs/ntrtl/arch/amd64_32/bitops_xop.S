/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * x32 XOP-optimized bit operations
 *
 * Requires: XOP (AMD Bulldozer, Piledriver, Steamroller, Excavator only)
 * Note: XOP was removed in Zen - this is for legacy AMD FX/A-series CPUs
 *
 * XOP provides unique instructions:
 * - VPROTB/W/D/Q: Variable bit rotation
 * - VPSHAB/W/D/Q: Variable arithmetic shift
 * - VPSHLB/W/D/Q: Variable logical shift
 * - VPCOMB/W/D/Q: Extended comparison with mask generation
 * - VPMACSDD, VPMADCSWD: Multiply-accumulate operations
 */

#include <ananke/asmdefs.h>

/*
 * RtlRotateLeft32_XOP - Rotate left 32-bit value
 * %edi: value
 * %esi: rotation count
 * Returns: rotated value
 *
 * Uses VPROTD for hardware rotation
 */
ANX_ASM_FUNCTION_BEGIN(RtlRotateLeft32_XOP)
	vmovd	%edi,%xmm0		/* Move value to XMM */
	vmovd	%esi,%xmm1		/* Move count to XMM */
	vprotd	%xmm1,%xmm0,%xmm0	/* Rotate */
	vmovd	%xmm0,%eax		/* Move result back */
	ret
ANX_ASM_FUNCTION_END(RtlRotateLeft32_XOP)

/*
 * RtlRotateRight32_XOP - Rotate right 32-bit value
 * %edi: value
 * %esi: rotation count
 * Returns: rotated value
 */
ANX_ASM_FUNCTION_BEGIN(RtlRotateRight32_XOP)
	vmovd	%edi,%xmm0		/* Move value to XMM */
	negl	%esi			/* Negate count for right rotation */
	vmovd	%esi,%xmm1		/* Move negated count to XMM */
	vprotd	%xmm1,%xmm0,%xmm0	/* Rotate (negative = right) */
	vmovd	%xmm0,%eax		/* Move result back */
	ret
ANX_ASM_FUNCTION_END(RtlRotateRight32_XOP)

/*
 * RtlRotateLeft64_XOP - Rotate left 64-bit value
 * %edi: value (low 32 bits)
 * %esi: rotation count
 * Returns: rotated value (64-bit in %eax:%edx for x32)
 *
 * Note: In x32, 64-bit values are passed/returned using register pairs
 * But we can still use 64-bit operations in XMM registers
 */
ANX_ASM_FUNCTION_BEGIN(RtlRotateLeft64_XOP)
	/* For x32, we receive a 64-bit value that spans the full register */
	vmovq	%rdi,%xmm0		/* Move 64-bit value to XMM */
	vmovd	%esi,%xmm1		/* Move count to XMM */
	vprotq	%xmm1,%xmm0,%xmm0	/* Rotate */
	vmovq	%xmm0,%rax		/* Move result back (64-bit) */
	ret
ANX_ASM_FUNCTION_END(RtlRotateLeft64_XOP)

/*
 * RtlRotateRight64_XOP - Rotate right 64-bit value
 * %edi: value (low 32 bits)
 * %esi: rotation count
 * Returns: rotated value (64-bit)
 */
ANX_ASM_FUNCTION_BEGIN(RtlRotateRight64_XOP)
	vmovq	%rdi,%xmm0		/* Move 64-bit value to XMM */
	negl	%esi			/* Negate count for right rotation */
	vmovd	%esi,%xmm1		/* Move negated count to XMM */
	vprotq	%xmm1,%xmm0,%xmm0	/* Rotate (negative = right) */
	vmovq	%xmm0,%rax		/* Move result back (64-bit) */
	ret
ANX_ASM_FUNCTION_END(RtlRotateRight64_XOP)

/*
 * RtlShiftLeftVariable_XOP - Variable shift left
 * %edi: value
 * %esi: shift counts (packed bytes, each byte is shift count for corresponding byte)
 * Returns: shifted value
 *
 * Useful for parallel processing of multiple values
 */
ANX_ASM_FUNCTION_BEGIN(RtlShiftLeftVariable_XOP)
	vmovq	%rdi,%xmm0		/* Move value to XMM */
	vmovq	%rsi,%xmm1		/* Move shift counts to XMM */
	vpshlb	%xmm1,%xmm0,%xmm0	/* Variable logical shift left */
	vmovq	%xmm0,%rax		/* Move result back */
	ret
ANX_ASM_FUNCTION_END(RtlShiftLeftVariable_XOP)

/*
 * RtlShiftRightVariable_XOP - Variable shift right (logical)
 * %edi: value
 * %esi: shift counts (packed bytes)
 * Returns: shifted value
 */
ANX_ASM_FUNCTION_BEGIN(RtlShiftRightVariable_XOP)
	vmovq	%rdi,%xmm0		/* Move value to XMM */
	vmovq	%rsi,%xmm1		/* Move shift counts to XMM */
	vnegb	%xmm1,%xmm1		/* Negate for right shift */
	vpshlb	%xmm1,%xmm0,%xmm0	/* Variable logical shift (neg = right) */
	vmovq	%xmm0,%rax		/* Move result back */
	ret
ANX_ASM_FUNCTION_END(RtlShiftRightVariable_XOP)

/*
 * RtlCompareAndMask_XOP - Compare and generate mask
 * %edi: value1
 * %esi: value2
 * %edx: comparison mode (0=LT, 1=LE, 2=GT, 3=GE, 4=EQ, 5=NE, 6=FALSE, 7=TRUE)
 * Returns: mask with 0xFF where condition true, 0x00 where false
 *
 * Uses VPCOMB for flexible comparison
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareAndMask_XOP)
	vmovq	%rdi,%xmm0		/* Move value1 to XMM */
	vmovq	%rsi,%xmm1		/* Move value2 to XMM */

	/* Use VPCOMB with comparison mode from edx */
	cmp	$0,%edx
	je	.Lcmp_lt
	cmp	$1,%edx
	je	.Lcmp_le
	cmp	$2,%edx
	je	.Lcmp_gt
	cmp	$3,%edx
	je	.Lcmp_ge
	cmp	$4,%edx
	je	.Lcmp_eq
	cmp	$5,%edx
	je	.Lcmp_ne
	/* Default to FALSE */
	vpxor	%xmm0,%xmm0,%xmm0
	jmp	.Lcmp_done

.Lcmp_lt:
	vpcomb	$0,%xmm1,%xmm0,%xmm0	/* Less than */
	jmp	.Lcmp_done
.Lcmp_le:
	vpcomb	$1,%xmm1,%xmm0,%xmm0	/* Less than or equal */
	jmp	.Lcmp_done
.Lcmp_gt:
	vpcomb	$2,%xmm1,%xmm0,%xmm0	/* Greater than */
	jmp	.Lcmp_done
.Lcmp_ge:
	vpcomb	$3,%xmm1,%xmm0,%xmm0	/* Greater than or equal */
	jmp	.Lcmp_done
.Lcmp_eq:
	vpcomb	$4,%xmm1,%xmm0,%xmm0	/* Equal */
	jmp	.Lcmp_done
.Lcmp_ne:
	vpcomb	$5,%xmm1,%xmm0,%xmm0	/* Not equal */

.Lcmp_done:
	vmovq	%xmm0,%rax		/* Move result back */
	ret
ANX_ASM_FUNCTION_END(RtlCompareAndMask_XOP)
