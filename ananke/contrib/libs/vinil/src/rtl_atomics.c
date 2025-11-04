/** @file
  VINIL Runtime Library - Atomic Operations Implementation

  Platform-independent atomic operations using GCC/Clang intrinsics.
  These provide thread-safe operations for multi-threaded IL execution.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#include <vinil/rtl_atomics.h>

//
// Atomic Operations Implementation
//

INT32
RtlAtomicFetchAdd32 (
  volatile INT32  *Ptr,
  INT32           Value
  )
{
  return __sync_fetch_and_add (Ptr, Value);
}

INT32
RtlAtomicFetchSub32 (
  volatile INT32  *Ptr,
  INT32           Value
  )
{
  return __sync_fetch_and_sub (Ptr, Value);
}

UINT32
RtlAtomicFetchOr32 (
  volatile UINT32  *Ptr,
  UINT32           Value
  )
{
  return __sync_fetch_and_or (Ptr, Value);
}

UINT32
RtlAtomicFetchAnd32 (
  volatile UINT32  *Ptr,
  UINT32           Value
  )
{
  return __sync_fetch_and_and (Ptr, Value);
}

UINT32
RtlAtomicFetchXor32 (
  volatile UINT32  *Ptr,
  UINT32           Value
  )
{
  return __sync_fetch_and_xor (Ptr, Value);
}

UINT32
RtlAtomicExchange32 (
  volatile UINT32  *Ptr,
  UINT32           Value
  )
{
  return __sync_lock_test_and_set (Ptr, Value);
}

UINT32
RtlAtomicCompareExchange32 (
  volatile UINT32  *Ptr,
  UINT32           Expected,
  UINT32           Value
  )
{
  UINT32  Old;

  Old = __sync_val_compare_and_swap (Ptr, Expected, Value);
  return Old;
}

//
// Memory Barriers
//

void
RtlMemoryBarrier (
  void
  )
{
  __sync_synchronize ();
}
