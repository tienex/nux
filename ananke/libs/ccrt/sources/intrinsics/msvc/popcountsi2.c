/** @file
  cCRT - Compiler Runtime Library

  MSVC wrapper for __popcountsi2 intrinsic

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ccrt/intrinsics/int_lib.h>

/*
 * MSVC intrinsic wrapper
 *
 * MSVC uses different naming conventions for compiler-rt functions.
 * This wrapper provides MSVC-compatible names while calling the
 * standard GNU implementation.
 *
 * MSVC typically uses __popcnt for intrinsics, but for compiler-rt
 * soft float/integer operations, it may expect the GNU names.
 */

/* Forward declaration of GNU implementation */
extern COMPILER_RT_ABI int __popcountsi2(si_int a);

/*
 * MSVC may call this for software popcount implementation
 * when hardware support is unavailable.
 */
#if defined(_MSC_VER)

/* MSVC wrapper - simply call GNU implementation */
int __popcountsi2_msvc(int a) {
    return __popcountsi2(a);
}

/* Provide both names for maximum compatibility */
#pragma comment(linker, "/alternatename:___popcountsi2_msvc=___popcountsi2")

#endif /* _MSC_VER */
