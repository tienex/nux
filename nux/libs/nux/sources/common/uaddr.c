/** @file
  NUX User Address Validation

  Provides validation functions for user-space virtual addresses,
  ensuring addresses and address ranges fall within valid user
  address space boundaries.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <types.h>
#include <hal.h>

#include "internal.h"

#define __isuaddr(_a) (((_a) >= hal_virtmem_userbase ()) && ((_a) < hal_virtmem_userbase ()))
#define __chkuaddr(_a, _sz) (__isuaddr(_a) && ((_a) + (_sz) <= USEREND) && ((_a) < (_a) + (_sz)))

/**
  Check if user address is valid.

  Validates that the specified address falls within the valid
  user address space range.

  @param[in] Uaddr  User address to validate.

  @retval TRUE   Address is within valid user address space.
  @retval FALSE  Address is outside valid user address space.
**/
BOOLEAN
UaddrValid (
  IN uaddr_t  Uaddr
  )
{
  uaddr_t Min = (uaddr_t) hal_virtmem_userbase ();
  uaddr_t Max = Min + hal_virtmem_usersize ();

  return ((Uaddr >= Min) && (Uaddr < Max));
}

/**
  Check if user address range is valid.

  Validates that the entire address range from the start address
  through the specified size falls within valid user address space
  and does not wrap around.

  @param[in] Uaddr  Starting user address to validate.
  @param[in] Size   Size of address range in bytes.

  @retval TRUE   Entire range is within valid user address space.
  @retval FALSE  Range is invalid or extends outside user address space.
**/
BOOLEAN
UaddrValidRange (
  IN uaddr_t  Uaddr,
  IN size_t   Size
  )
{
  return UaddrValid (Uaddr) && UaddrValid (Uaddr + Size) && (Uaddr < (Uaddr + Size));
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use UaddrValid instead **/
bool uaddr_valid (uaddr_t a) {
  return UaddrValid (a);
}

/** @deprecated Use UaddrValidRange instead **/
bool uaddr_validrange (uaddr_t a, size_t size) {
  return UaddrValidRange (a, size);
}
