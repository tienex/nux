/** @file
  NUX CPU Mask Operations

  Provides atomic and non-atomic operations for manipulating CPU bitmasks,
  used for tracking sets of CPUs for IPI, TLB operations, and scheduling.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __nux_cpumask_h__
#define __nux_cpumask_h__

#include <nux/types.h>
#include <nux/nux.h>

//
// Atomic CPU Mask Operations
//

/**
  Atomically set a CPU bit in the mask.

  Uses atomic OR operation to set the bit for the specified CPU.

  @param[in,out] CpuMask  Pointer to the CPU mask.
  @param[in]     CpuId     CPU number to set in the mask.
**/
static INLINE
VOID
AtomicCpuMaskSet (
  IN OUT CPU_MASK  *CpuMask,
  IN     UINTN     CpuId
  )
{
  ANX_SYNC_FETCH_AND_OR (CpuMask, (CPU_MASK)1 << CpuId);
}

/**
  Atomically AND a mask with the CPU mask.

  Uses atomic AND operation to mask out specified CPUs.

  @param[in,out] CpuMask  Pointer to the CPU mask.
  @param[in]     Mask      Mask value to AND with the CPU mask.
**/
static INLINE
VOID
AtomicCpuMaskAnd (
  IN OUT CPU_MASK  *CpuMask,
  IN     CPU_MASK  Mask
  )
{
  ANX_SYNC_FETCH_AND_AND (CpuMask, Mask);
}

/**
  Atomically clear a CPU bit in the mask.

  Uses atomic AND operation with inverted bit to clear the specified CPU.

  @param[in,out] CpuMask  Pointer to the CPU mask.
  @param[in]     CpuId     CPU number to clear from the mask.
**/
static INLINE
VOID
AtomicCpuMaskClear (
  IN OUT CPU_MASK  *CpuMask,
  IN     UINTN     CpuId
  )
{
  CPU_MASK  Mask;

  Mask = ~((CPU_MASK)1 << CpuId);
  AtomicCpuMaskAnd (CpuMask, Mask);
}

/**
  Atomically read the CPU mask.

  Uses atomic add-and-fetch with zero to safely read the mask value.

  @param[in] CpuMask  Pointer to the CPU mask.

  @return Current CPU mask value.
**/
static INLINE
CPU_MASK
AtomicCpuMaskRead (
  IN CPU_MASK  *CpuMask
  )
{
  return ANX_SYNC_ADD_AND_FETCH (CpuMask, 0);
}

//
// Non-Atomic CPU Mask Operations
//

/**
  Set a CPU bit in the mask (non-atomic).

  @param[in,out] CpuMask  Pointer to the CPU mask.
  @param[in]     CpuId     CPU number to set in the mask.
**/
static INLINE
VOID
CpuMaskSet (
  IN OUT CPU_MASK  *CpuMask,
  IN     UINTN     CpuId
  )
{
  *CpuMask |= (1 << CpuId);
}

/**
  Clear a CPU bit in the mask (non-atomic).

  @param[in,out] CpuMask  Pointer to the CPU mask.
  @param[in]     CpuId     CPU number to clear from the mask.
**/
static INLINE
VOID
CpuMaskClear (
  IN OUT CPU_MASK  *CpuMask,
  IN     UINTN     CpuId
  )
{
  *CpuMask &= ~(1 << CpuId);
}

//
// CPU Mask Iteration Macros
//

/**
  Execute operation once for first set CPU in mask.

  Finds the first set bit in the mask (after masking with active CPUs)
  and executes the operation with 'i' set to that CPU number.

  @param  Mask  CPU mask to check.
  @param  Op    Operation to execute (can reference variable 'i').

  @note Uses ffs-style iteration, should be optimized in future.
**/
#define ONCE_CPUMASK(Mask, Op)                                    \
  do {                                                            \
    INT32 i = 0;                                                  \
    CPU_MASK m = (Mask) & cpu_activemask ();                     \
                                                                  \
    while (m != 0) {                                              \
      if (m & 1) {                                                \
        Op;                                                       \
        break;                                                    \
      } else {                                                    \
        i++;                                                      \
        m >>= 1;                                                  \
      }                                                           \
    }                                                             \
  } while (0)

/**
  Execute operation for each set CPU in mask.

  Iterates through all set bits in the mask (after masking with active
  CPUs) and executes the operation for each with 'i' set to the CPU number.

  @param  Mask  CPU mask to iterate over.
  @param  Op    Operation to execute for each CPU (can reference variable 'i').
**/
#define FOREACH_CPUMASK(Mask, Op)                                 \
  do {                                                            \
    INT32 i = 0;                                                  \
    CPU_MASK m = (Mask) & cpu_activemask ();                     \
                                                                  \
    do {                                                          \
      if (m & 1) {                                                \
        Op;                                                       \
      }                                                           \
      m >>= 1;                                                    \
      i++;                                                        \
    } while (m != 0);                                             \
  } while (0)

#endif // NUX_CPUMASK_H
