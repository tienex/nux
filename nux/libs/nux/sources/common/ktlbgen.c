/** @file
  NUX Kernel TLB Generation Tracking

  Provides atomic TLB generation counters to track when kernel TLB
  flushes are required. Separate counters for global and normal
  TLB operations enable efficient TLB synchronization.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <assert.h>
#include <nux/types.h>
#include <nux/nux.h>

#include <nux/internal.h>

static VOLATILE KTLB gKtlb;

/**
  Mark kernel TLB as dirty based on operation type.

  Atomically increments the appropriate TLB generation counter
  based on the required TLB operation. This allows efficient
  tracking of when TLB flushes are needed across CPUs.

  @param[in] Op  TLB operation type (HAL_TLBOP_*).
**/
VOID
KtlbGenMarkDirty (
  IN hal_tlbop_t  Op
  )
{
  switch (Op)
    {
    case HAL_TLBOP_FLUSHALL:
      ANX_ATOMIC_ADD_FETCH (&gKtlb.Global, 1, __ATOMIC_RELEASE);
      break;
    case HAL_TLBOP_FLUSH:
      ANX_ATOMIC_ADD_FETCH (&gKtlb.Normal, 1, __ATOMIC_RELEASE);
      break;
    default:
      break;
    }
}

/**
  Get current global TLB generation counter.

  Returns the current global TLB generation. Used to track
  when global TLB flushes affecting all CPUs are needed.

  @return Current global TLB generation value.
**/
TLB_GENERATION
KtlbGenGlobal (
  VOID
  )
{
  TLB_GENERATION Ret;
  ANX_ATOMIC_LOAD (&gKtlb.Global, &Ret, __ATOMIC_ACQUIRE);
  return Ret;
}

/**
  Get current normal TLB generation counter.

  Returns the current normal TLB generation. Used to track
  when standard TLB flushes are needed.

  @return Current normal TLB generation value.
**/
TLB_GENERATION
KtlbGenNormal (
  VOID
  )
{
  TLB_GENERATION Ret;
  ANX_ATOMIC_LOAD (&gKtlb.Normal, &Ret, __ATOMIC_ACQUIRE);
  return Ret;
}
