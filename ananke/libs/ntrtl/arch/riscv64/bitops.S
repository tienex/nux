/* @file
 * NTRTL - NT Runtime Library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * RISC-V64 bit operations
 * Note: RISC-V B extension (Zbb) provides clz/ctz instructions
 * but we provide software fallback for base ISA
 */

#include <ananke/asmdefs.h>

/*
 * RtlFindFirstSetBit - Find first set bit (1-indexed)
 * a0: value
 * Returns: bit position (1-32), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindFirstSetBit)
	beqz	a0, 2f		/* Return 0 if value is 0 */

#ifdef __riscv_zbb
	/* Use Zbb extension if available */
	ctz	a0, a0
	addi	a0, a0, 1	/* Convert to 1-indexed */
	ret
#else
	/* Software implementation */
	li	t0, 1		/* Bit position counter */
1:	andi	t1, a0, 1
	bnez	t1, 3f		/* Found set bit */
	srli	a0, a0, 1
	addi	t0, t0, 1
	bnez	a0, 1b		/* Continue if more bits */
#endif

2:	li	a0, 0		/* Not found */
	ret

3:	mv	a0, t0		/* Return position */
	ret
ANX_ASM_FUNCTION_END(RtlFindFirstSetBit)

/*
 * RtlFindLastSetBit - Find last set bit (1-indexed)
 * a0: value
 * Returns: bit position (1-32), or 0 if no bits set
 */
ANX_ASM_FUNCTION_BEGIN(RtlFindLastSetBit)
	beqz	a0, 2f		/* Return 0 if value is 0 */

#ifdef __riscv_zbb
	/* Use Zbb extension if available */
	clz	a0, a0		/* Count leading zeros */
	li	t0, 32
	sub	a0, t0, a0	/* Convert to 1-indexed position */
	ret
#else
	/* Software implementation - find MSB */
	li	t0, 0		/* Last set bit position */
	li	t1, 1		/* Current bit position */
1:	andi	t2, a0, 1
	beqz	t2, 3f
	mv	t0, t1		/* Update last set bit */
3:	srli	a0, a0, 1
	addi	t1, t1, 1
	bnez	a0, 1b		/* Continue if more bits */
#endif

	mv	a0, t0		/* Return last set bit position */
	ret

2:	li	a0, 0		/* Not found */
	ret
ANX_ASM_FUNCTION_END(RtlFindLastSetBit)
