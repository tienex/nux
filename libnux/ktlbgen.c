/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <assert.h>
#include <nux/types.h>
#include <nux/nux.h>

#include "internal.h"

static volatile struct ktlb ktlb;

/*
  Update kernel TLB based on the required global TLB operation.
*/
void
ktlbgen_markdirty (hal_tlbop_t op)
{
  switch (op)
    {
    case HAL_TLBOP_FLUSHALL:
      __atomic_add_fetch (&ktlb.global, 1, __ATOMIC_RELEASE);
      break;
    case HAL_TLBOP_FLUSH:
      __atomic_add_fetch (&ktlb.normal, 1, __ATOMIC_RELEASE);
      break;
    default:
      break;
    }
}

tlbgen_t
ktlbgen_global (void)
{
  tlbgen_t ret;
  __atomic_load (&ktlb.global, &ret, __ATOMIC_ACQUIRE);
  return ret;
}

tlbgen_t
ktlbgen_normal (void)
{
  tlbgen_t ret;
  __atomic_load (&ktlb.normal, &ret, __ATOMIC_ACQUIRE);
  return ret;
}
