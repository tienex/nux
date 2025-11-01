/** @file
  NTRTL - NT Runtime Library

  Generic (portable) bit operations using compiler intrinsics

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>
#include <ccrt/intrinsics/builtins.h>

/**
  Find first set bit (1-indexed, returns 0 if no bits set)
  Uses count-trailing-zeros intrinsic for optimal performance
**/
INT32
ANXAPI
RtlFindFirstSetBit (
    IN UINT32 Value
    )
{
    if (Value == 0) {
        return 0;
    }

    /* ctz returns number of trailing zeros (0-indexed from LSB)
     * We want 1-indexed bit position, so add 1
     */
    return ccrt_ctz32(Value) + 1;
}

/**
  Find last set bit (1-indexed, returns 0 if no bits set)
  Uses count-leading-zeros intrinsic for optimal performance
**/
INT32
ANXAPI
RtlFindLastSetBit (
    IN UINT32 Value
    )
{
    if (Value == 0) {
        return 0;
    }

    /* clz returns number of leading zeros (0-indexed from MSB)
     * Bit position is 32 - clz (1-indexed)
     */
    return 32 - ccrt_clz32(Value);
}
