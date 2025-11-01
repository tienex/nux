/** @file
  cCRT - Compiler Runtime Library

  Watcom wrapper for __udivmoddi4 intrinsic

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
 * Watcom typically uses _udiv64/_uldiv for 64-bit division, but for
 * compiler-rt operations, it may expect the GNU names.
 */

/* Forward declaration of GNU implementation */
extern COMPILER_RT_ABI du_int __udivmoddi4(du_int a, du_int b, du_int *rem);

/*
 * Watcom may call this for software 64-bit division implementation
 * when hardware support is unavailable (e.g., 32-bit builds).
 */
#if defined(__WATCOMC__)

/* Watcom wrapper - simply call GNU implementation */
unsigned long long __udivmoddi4_watcom(unsigned long long a,
                                        unsigned long long b,
                                        unsigned long long *rem) {
    return __udivmoddi4(a, b, rem);
}

/* Watcom uses pragma aux for function aliasing */
#pragma aux __udivmoddi4_watcom "__udivmoddi4"

#endif /* __WATCOMC__ */
