/** @file
  VINIL Runtime Library - Atomic Operations

  Atomic operations and synchronization primitives for VINIL IL execution.
  Provides platform-independent atomic operations using compiler intrinsics.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#pragma once
#include <vinil/base.h>

//
// Atomic Operations (32-bit)
//

/**
  Atomic fetch-and-add (32-bit signed).

  @param[in,out]  Ptr    Pointer to value.
  @param[in]      Value  Value to add.

  @return  Previous value before addition.
**/
INT32
RtlAtomicFetchAdd32 (
  volatile INT32  *Ptr,
  INT32           Value
  );

/**
  Atomic fetch-and-subtract (32-bit signed).

  @param[in,out]  Ptr    Pointer to value.
  @param[in]      Value  Value to subtract.

  @return  Previous value before subtraction.
**/
INT32
RtlAtomicFetchSub32 (
  volatile INT32  *Ptr,
  INT32           Value
  );

/**
  Atomic fetch-and-OR (32-bit unsigned).

  @param[in,out]  Ptr    Pointer to value.
  @param[in]      Value  Value to OR.

  @return  Previous value before operation.
**/
UINT32
RtlAtomicFetchOr32 (
  volatile UINT32  *Ptr,
  UINT32           Value
  );

/**
  Atomic fetch-and-AND (32-bit unsigned).

  @param[in,out]  Ptr    Pointer to value.
  @param[in]      Value  Value to AND.

  @return  Previous value before operation.
**/
UINT32
RtlAtomicFetchAnd32 (
  volatile UINT32  *Ptr,
  UINT32           Value
  );

/**
  Atomic fetch-and-XOR (32-bit unsigned).

  @param[in,out]  Ptr    Pointer to value.
  @param[in]      Value  Value to XOR.

  @return  Previous value before operation.
**/
UINT32
RtlAtomicFetchXor32 (
  volatile UINT32  *Ptr,
  UINT32           Value
  );

/**
  Atomic exchange (32-bit unsigned).

  @param[in,out]  Ptr    Pointer to value.
  @param[in]      Value  New value.

  @return  Previous value before exchange.
**/
UINT32
RtlAtomicExchange32 (
  volatile UINT32  *Ptr,
  UINT32           Value
  );

/**
  Atomic compare-and-swap (32-bit unsigned).

  @param[in,out]  Ptr       Pointer to value.
  @param[in]      Expected  Expected value.
  @param[in]      Value     New value if comparison succeeds.

  @return  Actual value before operation.
**/
UINT32
RtlAtomicCompareExchange32 (
  volatile UINT32  *Ptr,
  UINT32           Expected,
  UINT32           Value
  );

//
// Memory Barriers
//

/**
  Full memory barrier (sequential consistency).

  Ensures all memory operations before the barrier complete before
  any memory operations after the barrier begin.
**/
void
RtlMemoryBarrier (
  void
  );
