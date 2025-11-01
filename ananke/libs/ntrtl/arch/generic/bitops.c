/** @file
  NTRTL - NT Runtime Library

  Generic (portable) bit operations

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>

/**
  Find first set bit (1-indexed, returns 0 if no bits set)
**/
INT32
ANXAPI
RtlFindFirstSetBit (
    IN UINT32 Value
    )
{
    INT32 Bit;

    if (Value == 0) {
        return 0;
    }

    for (Bit = 1; Bit <= 32; Bit++) {
        if (Value & 1) {
            return Bit;
        }
        Value >>= 1;
    }

    return 0;
}

/**
  Find last set bit (1-indexed, returns 0 if no bits set)
**/
INT32
ANXAPI
RtlFindLastSetBit (
    IN UINT32 Value
    )
{
    INT32 Bit;

    if (Value == 0) {
        return 0;
    }

    for (Bit = 32; Bit >= 1; Bit--) {
        if (Value & (1U << (Bit - 1))) {
            return Bit;
        }
    }

    return 0;
}
