/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V64 RVV 0.9-optimized memory operations
 *
 * Requires: RISC-V Vector Extension 0.9 (older draft spec)
 * Compatibility for early RVV implementations (e.g., SiFive P670, T-Head C920)
 *
 * Key differences from RVV 1.0:
 * - Different register encoding
 * - Slightly different instruction mnemonics
 * - Different mask handling
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory_RVV09 - RVV 0.9-optimized memory copy
 * a0: destination
 * a1: source
 * a2: length
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory_RVV09)
	mv	t6, a0			/* Save destination for return */

	/* Check for zero length */
	beqz	a2, 3f

	/* Vector copy loop (RVV 0.9 syntax) */
1:
	vsetvli	t0, a2, e8, m8		/* Set vector length for 8-bit elements, m8 */
	vle8.v	v0, (a1)		/* Load vector from source */
	vse8.v	v0, (a0)		/* Store vector to destination */

	add	a1, a1, t0		/* Advance source pointer */
	add	a0, a0, t0		/* Advance destination pointer */
	sub	a2, a2, t0		/* Decrease remaining bytes */
	bnez	a2, 1b			/* Continue if more bytes */

3:
	mv	a0, t6			/* Restore destination pointer */
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory_RVV09)

/*
 * RtlMoveMemory_RVV09 - RVV 0.9-optimized memory move (handles overlap)
 * a0: destination
 * a1: source
 * a2: length
 * Returns: destination pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlMoveMemory_RVV09)
	mv	t6, a0			/* Save destination for return */

	/* Check for zero length */
	beqz	a2, 4f

	/* Check for overlap requiring backward copy */
	bltu	a0, a1, 1f		/* If dest < src, forward copy is safe */
	add	t0, a1, a2		/* src + length */
	bgeu	a0, t0, 1f		/* If dest >= src+len, forward copy is safe */

	/* Backward copy needed */
	add	a0, a0, a2		/* Start at end of destination */
	add	a1, a1, a2		/* Start at end of source */

2:
	vsetvli	t0, a2, e8, m8		/* Set vector length */
	sub	a1, a1, t0		/* Move source pointer back */
	sub	a0, a0, t0		/* Move destination pointer back */
	vle8.v	v0, (a1)		/* Load vector from source */
	vse8.v	v0, (a0)		/* Store vector to destination */
	sub	a2, a2, t0		/* Decrease remaining bytes */
	bnez	a2, 2b			/* Continue if more bytes */
	j	4f

	/* Forward copy */
1:
	vsetvli	t0, a2, e8, m8		/* Set vector length */
	vle8.v	v0, (a1)		/* Load vector from source */
	vse8.v	v0, (a0)		/* Store vector to destination */
	add	a1, a1, t0		/* Advance source pointer */
	add	a0, a0, t0		/* Advance destination pointer */
	sub	a2, a2, t0		/* Decrease remaining bytes */
	bnez	a2, 1b			/* Continue if more bytes */

4:
	mv	a0, t6			/* Restore destination pointer */
	ret
ANX_ASM_FUNCTION_END(RtlMoveMemory_RVV09)

/*
 * RtlFillMemory_RVV09 - RVV 0.9-optimized memory fill
 * a0: buffer
 * a1: length
 * a2: byte value (low 8 bits)
 * Returns: buffer pointer
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory_RVV09)
	mv	t6, a0			/* Save buffer for return */

	/* Check for zero length */
	beqz	a1, 3f

	/* Broadcast fill byte to vector register */
	vsetvli	t0, zero, e8, m8	/* Set max vector length */
	vmv.v.x	v0, a2			/* Broadcast byte to all vector elements */

	/* Vector fill loop */
1:
	vsetvli	t0, a1, e8, m8		/* Set vector length for this iteration */
	vse8.v	v0, (a0)		/* Store vector to buffer */
	add	a0, a0, t0		/* Advance buffer pointer */
	sub	a1, a1, t0		/* Decrease remaining bytes */
	bnez	a1, 1b			/* Continue if more bytes */

3:
	mv	a0, t6			/* Restore buffer pointer */
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory_RVV09)

/*
 * RtlCompareMemory_RVV09 - RVV 0.9-optimized memory compare
 * a0: buffer1
 * a1: buffer2
 * a2: length
 * Returns: 0 if equal, <0 if buf1<buf2, >0 if buf1>buf2
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareMemory_RVV09)
	/* Check for zero length */
	beqz	a2, 4f			/* Equal if zero length */

	/* Vector compare loop */
1:
	vsetvli	t0, a2, e8, m8		/* Set vector length */
	vle8.v	v0, (a0)		/* Load from buffer1 */
	vle8.v	v8, (a1)		/* Load from buffer2 */

	vmseq.vv v0, v0, v8		/* Compare for equality (RVV 0.9 destroys v0) */
	vpopc.m	t1, v0			/* Count set bits (equal elements) */

	/* If all bits set (all equal), continue */
	beq	t1, t0, 2f

	/* Found difference - need to find which byte */
	vsetvli	t0, a2, e8, m8		/* Reset vector length */
	vle8.v	v0, (a0)		/* Reload buffer1 */
	vle8.v	v8, (a1)		/* Reload buffer2 */
	vmseq.vv v16, v0, v8		/* Compare again */

	/* Scan for first difference */
	li	t2, 0
3:
	vslidedown.vi v24, v16, 0	/* Get first element of mask */
	/* In RVV 0.9, we need to extract to scalar */
	vmv.x.s	t3, v24			/* Move to scalar */
	beqz	t3, 5f			/* If zero, found difference */

	/* This element matches, continue */
	addi	t2, t2, 1
	vslidedown.vi v16, v16, 1
	blt	t2, t0, 3b

2:	/* All elements equal in this iteration */
	add	a0, a0, t0		/* Advance buffer1 pointer */
	add	a1, a1, t0		/* Advance buffer2 pointer */
	sub	a2, a2, t0		/* Decrease remaining bytes */
	bnez	a2, 1b			/* Continue if more bytes */

4:	/* All bytes equal */
	li	a0, 0
	ret

5:	/* Found difference at position t2 */
	add	a0, a0, t2		/* Point to differing byte in buf1 */
	add	a1, a1, t2		/* Point to differing byte in buf2 */
	lbu	t0, 0(a0)		/* Load byte from buf1 */
	lbu	t1, 0(a1)		/* Load byte from buf2 */
	sub	a0, t0, t1		/* Return difference */
	ret
ANX_ASM_FUNCTION_END(RtlCompareMemory_RVV09)
