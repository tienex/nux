/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <nux/types.h>
#include <nux/hal.h>

#include "internal.h"

#define __isuaddr(_a) (((_a) >= hal_virtmem_userbase ()) && ((_a) < hal_virtmem_userbase ()))
#define __chkuaddr(_a, _sz) (__isuaddr(_a) && ((_a) + (_sz) <= USEREND) && ((_a) < (_a) + (_sz)))

bool
uaddr_valid (uaddr_t a)
{
  uaddr_t min = (uaddr_t) hal_virtmem_userbase ();
  uaddr_t max = min + hal_virtmem_usersize ();

  return ((a >= min) && (a < max));
}

bool
uaddr_validrange (uaddr_t a, size_t size)
{
  return uaddr_valid (a) && uaddr_valid (a + size) && (a < (a + size));
}
