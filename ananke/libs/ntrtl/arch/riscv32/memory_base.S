/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V32 optimized memory operations
 * Uses 32-bit loads/stores (RV32I base)
 */

#include <ananke/asmdefs.h>

/*
 * RtlCopyMemory - Copy memory (non-overlapping)
 * a0: destination
 * a1: source
 * a2: length
 */
ANX_ASM_FUNCTION_BEGIN(RtlCopyMemory)
	mv	t6, a0		/* Save return value */

	/* Check for zero length */
	beqz	a2, 3f

	/* Check alignment - if both src and dest are 4-byte aligned, use words */
	or	t0, a0, a1
	andi	t0, t0, 3
	bnez	t0, 2f

	/* 4-byte aligned copy */
1:	andi	t0, a2, ~3
	beqz	t0, 2f
	lw	t1, 0(a1)
	sw	t1, 0(a0)
	addi	a0, a0, 4
	addi	a1, a1, 4
	addi	a2, a2, -4
	bnez	a2, 1b

2:	/* Byte-by-byte for remainder or unaligned */
	beqz	a2, 3f
	lb	t1, 0(a1)
	sb	t1, 0(a0)
	addi	a0, a0, 1
	addi	a1, a1, 1
	addi	a2, a2, -1
	j	2b

3:	mv	a0, t6		/* Restore return value */
	ret
ANX_ASM_FUNCTION_END(RtlCopyMemory)

/*
 * RtlMoveMemory - Move memory (may overlap)
 * a0: destination
 * a1: source
 * a2: length
 */
ANX_ASM_FUNCTION_BEGIN(RtlMoveMemory)
	mv	t6, a0		/* Save return value */

	/* Check for zero length */
	beqz	a2, 3f

	/* Check for overlap: if dest > src && dest < src + length, copy backwards */
	bgeu	a1, a0, 1f	/* src >= dest, safe to copy forward */
	add	t0, a1, a2
	bgeu	a0, t0, 1f	/* dest >= src + length, safe to copy forward */

	/* Copy backwards */
	add	a0, a0, a2
	add	a1, a1, a2
2:	addi	a0, a0, -1
	addi	a1, a1, -1
	lb	t1, 0(a1)
	sb	t1, 0(a0)
	addi	a2, a2, -1
	bnez	a2, 2b
	mv	a0, t6
	ret

1:	/* Copy forward (same as RtlCopyMemory) */
	or	t0, a0, a1
	andi	t0, t0, 3
	bnez	t0, 5f

4:	/* 4-byte aligned copy */
	andi	t0, a2, ~3
	beqz	t0, 5f
	lw	t1, 0(a1)
	sw	t1, 0(a0)
	addi	a0, a0, 4
	addi	a1, a1, 4
	addi	a2, a2, -4
	bnez	a2, 4b

5:	/* Byte-by-byte for remainder */
	beqz	a2, 3f
	lb	t1, 0(a1)
	sb	t1, 0(a0)
	addi	a0, a0, 1
	addi	a1, a1, 1
	addi	a2, a2, -1
	j	5b

3:	mv	a0, t6
	ret
ANX_ASM_FUNCTION_END(RtlMoveMemory)

/*
 * RtlCompareMemory - Compare memory regions
 * a0: buffer1
 * a1: buffer2
 * a2: length
 * Returns: number of matching bytes
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareMemory)
	li	t0, 0		/* Match counter */

1:	beqz	a2, 2f
	lb	t1, 0(a0)
	lb	t2, 0(a1)
	bne	t1, t2, 2f	/* Mismatch, return current count */
	addi	t0, t0, 1
	addi	a0, a0, 1
	addi	a1, a1, 1
	addi	a2, a2, -1
	j	1b

2:	mv	a0, t0
	ret
ANX_ASM_FUNCTION_END(RtlCompareMemory)

/*
 * RtlFillMemory - Fill memory with byte value
 * a0: destination
 * a1: fill value (byte)
 * a2: length
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory)
	beqz	a2, 2f

1:	sb	a1, 0(a0)
	addi	a0, a0, 1
	addi	a2, a2, -1
	bnez	a2, 1b

2:	ret
ANX_ASM_FUNCTION_END(RtlFillMemory)

/*
 * RtlZeroMemory - Zero memory
 * a0: destination
 * a1: length
 */
ANX_ASM_FUNCTION_BEGIN(RtlZeroMemory)
	mv	a2, a1
	li	a1, 0
	beqz	a2, 2f

1:	sb	a1, 0(a0)
	addi	a0, a0, 1
	addi	a2, a2, -1
	bnez	a2, 1b

2:	ret
ANX_ASM_FUNCTION_END(RtlZeroMemory)

/*
 * RtlFindByteInMemory - Find byte in memory region
 * a0: buffer
 * a1: length
 * a2: byte to find
 * Returns: pointer to first occurrence, or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindByteInMemory)
	beqz	a1, 2f

1:	lb	t0, 0(a0)
	beq	t0, a2, 3f	/* Found */
	addi	a0, a0, 1
	addi	a1, a1, -1
	bnez	a1, 1b

2:	li	a0, 0		/* Not found */
	ret

3:	ret			/* Found, pointer in a0 */
ANX_ASM_FUNCTION_END(RtlFindByteInMemory)
