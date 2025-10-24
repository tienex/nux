/** @file
  NUX Basic Definitions

  Provides fundamental constants and macros for page size calculations,
  address conversions, and alignment operations.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef NUX_DEFS_H
#define NUX_DEFS_H

#include <hal/config.h>

//
// Page Size Definitions
//

/**
  Number of bits to shift for page size conversion.
  Architecture-specific value from HAL configuration.
**/
#define PAGE_SHIFT HAL_PAGE_SHIFT

/**
  Size of a page in bytes.
  Calculated as 2^PAGE_SHIFT.
**/
#define PAGE_SIZE (1 << PAGE_SHIFT)

/**
  Mask for page offset bits.
  Used to extract the offset within a page.
**/
#define PAGE_MASK (PAGE_SIZE - 1)

//
// Page Alignment Macros
//

/**
  Truncate address to page boundary.

  Rounds down the given address to the nearest page boundary.

  @param  x  Address to truncate.

  @return Address aligned to page boundary (rounded down).
**/
#define TRUNC_PAGE(x)  ((x) & (~(PAGE_SIZE - 1)))

/**
  Round address up to page boundary.

  Rounds up the given address to the next page boundary.

  @param  x  Address to round.

  @return Address aligned to page boundary (rounded up).
**/
#define ROUND_PAGE(x)  TRUNC_PAGE((x) + PAGE_SIZE - 1)

//
// Address Conversion Macros
//

/**
  Convert page number to byte address.

  Converts a page frame number to its corresponding physical byte address
  by shifting left by PAGE_SHIFT bits.

  @param  x  Page frame number.

  @return Physical address in bytes.
**/
#define PTOB(x)  ((PHYSICAL_ADDRESS)(x) << PAGE_SHIFT)

/**
  Convert byte address to page number.

  Converts a physical byte address to its corresponding page frame number
  by shifting right by PAGE_SHIFT bits.

  @param  x  Physical address in bytes.

  @return Page frame number.
**/
#define BTOP(x)  ((x) >> PAGE_SHIFT)

//
// Legacy Macro Aliases (for backward compatibility)
//

/** @deprecated Use TRUNC_PAGE instead **/
#define trunc_page(x)  TRUNC_PAGE(x)

/** @deprecated Use ROUND_PAGE instead **/
#define round_page(x)  ROUND_PAGE(x)

/** @deprecated Use PTOB instead **/
#define ptob(x)  PTOB(x)

/** @deprecated Use BTOP instead **/
#define btop(x)  BTOP(x)

#endif // NUX_DEFS_H
