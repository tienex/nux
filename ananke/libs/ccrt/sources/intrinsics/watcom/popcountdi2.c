/** @file
  cCRT - Compiler Runtime Library

  Watcom wrapper for __popcountdi2 intrinsic

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ccrt/intrinsics/int_lib.h>

/*
 * Watcom intrinsic wrapper
 *
 * Watcom uses different naming conventions for compiler-rt functions.
 * This wrapper provides Watcom-compatible names while calling the
 * standard GNU implementation.
 *
 * Watcom typically uses __popcount64 for intrinsics, but for compiler-rt
 * soft float/integer operations, it may expect the GNU names.
 */

/* Forward declaration of GNU implementation */
extern COMPILER_RT_ABI int __popcountdi2(di_int a);

/*
 * Watcom may call this for software popcount implementation
 * when hardware support is unavailable.
 */
#if defined(__WATCOMC__)

/* Watcom wrapper - simply call GNU implementation */
int __popcountdi2_watcom(long long a) {
    return __popcountdi2(a);
}

/* Watcom uses pragma aux for function aliasing */
#pragma aux __popcountdi2_watcom "__popcountdi2"

#endif /* __WATCOMC__ */
