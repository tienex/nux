/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V64 optimized memory operations
 * Uses 64-bit loads/stores for better performance
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

	/* Check alignment - if both src and dest are 8-byte aligned, use doublewords */
	or	t0, a0, a1
	andi	t0, t0, 7
	bnez	t0, 2f

	/* 8-byte aligned copy */
1:	andi	t0, a2, ~7
	beqz	t0, 2f
	ld	t1, 0(a1)
	sd	t1, 0(a0)
	addi	a0, a0, 8
	addi	a1, a1, 8
	addi	a2, a2, -8
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
	andi	t0, t0, 7
	bnez	t0, 5f

4:	/* 8-byte aligned copy */
	andi	t0, a2, ~7
	beqz	t0, 5f
	ld	t1, 0(a1)
	sd	t1, 0(a0)
	addi	a0, a0, 8
	addi	a1, a1, 8
	addi	a2, a2, -8
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
 * RtlFillMemory - Fill memory with byte value
 * a0: destination
 * a1: length
 * a2: fill byte
 */
ANX_ASM_FUNCTION_BEGIN(RtlFillMemory)
	mv	t6, a0		/* Save return value */

	/* Check for zero length */
	beqz	a1, 3f

	/* Replicate byte to doubleword */
	andi	a2, a2, 0xff
	slli	t0, a2, 8
	or	a2, a2, t0
	slli	t0, a2, 16
	or	a2, a2, t0
	slli	t0, a2, 32
	or	a2, a2, t0

	/* Check alignment */
	andi	t0, a0, 7
	bnez	t0, 2f

	/* 8-byte aligned fill */
1:	andi	t0, a1, ~7
	beqz	t0, 2f
	sd	a2, 0(a0)
	addi	a0, a0, 8
	addi	a1, a1, -8
	bnez	a1, 1b

2:	/* Byte-by-byte for remainder or unaligned */
	beqz	a1, 3f
	sb	a2, 0(a0)
	addi	a0, a0, 1
	addi	a1, a1, -1
	j	2b

3:	mv	a0, t6
	ret
ANX_ASM_FUNCTION_END(RtlFillMemory)

/*
 * RtlCompareMemory - Compare memory buffers
 * a0: source1
 * a1: source2
 * a2: length
 * Returns: number of matching bytes
 */
ANX_ASM_FUNCTION_BEGIN(RtlCompareMemory)
	li	t0, 0		/* Match counter */

1:	beqz	a2, 2f
	lb	t1, 0(a0)
	lb	t2, 0(a1)
	bne	t1, t2, 2f	/* Mismatch found */
	addi	a0, a0, 1
	addi	a1, a1, 1
	addi	t0, t0, 1
	addi	a2, a2, -1
	j	1b

2:	mv	a0, t0		/* Return match count */
	ret
ANX_ASM_FUNCTION_END(RtlCompareMemory)

/*
 * RtlFindByteInMemory - Find byte in memory
 * a0: buffer
 * a1: length
 * a2: byte value
 * Returns: pointer to first occurrence or NULL
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindByteInMemory)
	andi	a2, a2, 0xff	/* Ensure byte value */

1:	beqz	a1, 2f
	lb	t0, 0(a0)
	beq	t0, a2, 3f	/* Found */
	addi	a0, a0, 1
	addi	a1, a1, -1
	j	1b

2:	li	a0, 0		/* Not found, return NULL */
3:	ret
ANX_ASM_FUNCTION_END(RtlFindByteInMemory)
