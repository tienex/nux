/** @file
  eCRT - An embedded C runtime library

  NTRTL-based string function implementations

  These implementations are intentionally NOT provided as the existing
  portable C implementations in individual .c files (strlen.c, strchr.c, etc.)
  are already optimal for null-terminated C strings.

  NTRTL's string functions are designed for counted STRING/UNICODE_STRING
  structures, not raw null-terminated strings. For maximum performance with
  SIMD optimizations, use NTRTL's STRING API directly rather than standard
  C string functions.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

/*
 * No implementations needed here.
 *
 * Use the existing portable implementations:
 * - strlen.c, strchr.c, strrchr.c, strncmp.c, strlcpy.c, etc.
 *
 * For SIMD-optimized string operations, use NTRTL's STRING API:
 * - RtlInitString(), RtlCopyString(), RtlCompareString(), etc.
 */
