/** @file
  eCRT - An embedded C runtime library

  Bit operation functions

  Implements fls (find last set) and ffs (find first set) using
  compiler intrinsics for optimal performance.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ecrt/string.h>

#ifdef USE_NTRTL
#include <ccrt/intrinsics/builtins.h>

/**
  Find last (most significant) bit set.

  @param[in] mask  Value to search

  @return Bit position (1-based), or 0 if no bits set

  Examples:
    fls(0x0) = 0
    fls(0x1) = 1
    fls(0x8) = 4
    fls(0x80) = 8
    fls(0xFFFFFFFF) = 32
**/
unsigned long
fls (
    unsigned long mask
    )
{
    if (mask == 0) {
        return 0;
    }

#if defined(__LP64__) || defined(_WIN64)
    /* 64-bit */
    return 64 - ccrt_clz64(mask);
#else
    /* 32-bit */
    return 32 - ccrt_clz32((uint32_t)mask);
#endif
}

/**
  Find first (least significant) bit set.

  @param[in] mask  Value to search

  @return Bit position (1-based), or 0 if no bits set

  Examples:
    ffs(0x0) = 0
    ffs(0x1) = 1
    ffs(0x2) = 2
    ffs(0x8) = 4
    ffs(0x80) = 8
    ffs(0xFFFFFFFF) = 1
**/
unsigned long
ffs (
    unsigned long mask
    )
{
    if (mask == 0) {
        return 0;
    }

#if defined(__LP64__) || defined(_WIN64)
    /* 64-bit */
    return ccrt_ctz64(mask) + 1;
#else
    /* 32-bit */
    return ccrt_ctz32((uint32_t)mask) + 1;
#endif
}

#else
/*
 * Portable implementations without intrinsics
 */

/**
  Find last (most significant) bit set (portable version).

  @param[in] mask  Value to search

  @return Bit position (1-based), or 0 if no bits set
**/
unsigned long
fls (
    unsigned long mask
    )
{
    unsigned long bit = 0;

    if (mask == 0) {
        return 0;
    }

    /* Count bits from MSB */
#if defined(__LP64__) || defined(_WIN64)
    if (mask & 0xFFFFFFFF00000000UL) { bit += 32; mask >>= 32; }
#endif
    if (mask & 0xFFFF0000UL) { bit += 16; mask >>= 16; }
    if (mask & 0xFF00UL)     { bit += 8;  mask >>= 8;  }
    if (mask & 0xF0UL)       { bit += 4;  mask >>= 4;  }
    if (mask & 0xCUL)        { bit += 2;  mask >>= 2;  }
    if (mask & 0x2UL)        { bit += 1;  mask >>= 1;  }
    if (mask & 0x1UL)        { bit += 1; }

    return bit;
}

/**
  Find first (least significant) bit set (portable version).

  @param[in] mask  Value to search

  @return Bit position (1-based), or 0 if no bits set
**/
unsigned long
ffs (
    unsigned long mask
    )
{
    unsigned long bit = 1;

    if (mask == 0) {
        return 0;
    }

    /* Count trailing zeros */
#if defined(__LP64__) || defined(_WIN64)
    if ((mask & 0xFFFFFFFFUL) == 0) { bit += 32; mask >>= 32; }
#endif
    if ((mask & 0xFFFFUL) == 0)     { bit += 16; mask >>= 16; }
    if ((mask & 0xFFUL) == 0)       { bit += 8;  mask >>= 8;  }
    if ((mask & 0xFUL) == 0)        { bit += 4;  mask >>= 4;  }
    if ((mask & 0x3UL) == 0)        { bit += 2;  mask >>= 2;  }
    if ((mask & 0x1UL) == 0)        { bit += 1; }

    return bit;
}

#endif /* USE_NTRTL */
