/** @file
  NUX CPU Mask Operations

  Provides atomic and non-atomic operations for manipulating CPU bitmasks,
  used for tracking sets of CPUs for IPI, TLB operations, and scheduling.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef NUX_CPUMASK_H
#define NUX_CPUMASK_H

#include <types.h>
#include <nux.h>

//
// Atomic CPU Mask Operations
//

/**
  Atomically set a CPU bit in the mask.

  Uses atomic OR operation to set the bit for the specified CPU.

  @param[in,out] pCpuMask  Pointer to the CPU mask.
  @param[in]     CpuId     CPU number to set in the mask.
**/
static inline
VOID
AtomicCpuMaskSet (
  IN OUT CPU_MASK  *pCpuMask,
  IN     UINTN     CpuId
  )
{
  __sync_fetch_and_or (pCpuMask, (CPU_MASK)1 << CpuId);
}

/**
  Atomically AND a mask with the CPU mask.

  Uses atomic AND operation to mask out specified CPUs.

  @param[in,out] pCpuMask  Pointer to the CPU mask.
  @param[in]     Mask      Mask value to AND with the CPU mask.
**/
static inline
VOID
AtomicCpuMaskAnd (
  IN OUT CPU_MASK  *pCpuMask,
  IN     CPU_MASK  Mask
  )
{
  __sync_fetch_and_and (pCpuMask, Mask);
}

/**
  Atomically clear a CPU bit in the mask.

  Uses atomic AND operation with inverted bit to clear the specified CPU.

  @param[in,out] pCpuMask  Pointer to the CPU mask.
  @param[in]     CpuId     CPU number to clear from the mask.
**/
static inline
VOID
AtomicCpuMaskClear (
  IN OUT CPU_MASK  *pCpuMask,
  IN     UINTN     CpuId
  )
{
  CPU_MASK  Mask;

  Mask = ~((CPU_MASK)1 << CpuId);
  AtomicCpuMaskAnd (pCpuMask, Mask);
}

/**
  Atomically read the CPU mask.

  Uses atomic add-and-fetch with zero to safely read the mask value.

  @param[in] pCpuMask  Pointer to the CPU mask.

  @return Current CPU mask value.
**/
static inline
CPU_MASK
AtomicCpuMaskRead (
  IN CPU_MASK  *pCpuMask
  )
{
  return __sync_add_and_fetch (pCpuMask, 0);
}

//
// Non-Atomic CPU Mask Operations
//

/**
  Set a CPU bit in the mask (non-atomic).

  @param[in,out] pCpuMask  Pointer to the CPU mask.
  @param[in]     CpuId     CPU number to set in the mask.
**/
static inline
VOID
CpuMaskSet (
  IN OUT CPU_MASK  *pCpuMask,
  IN     UINTN     CpuId
  )
{
  *pCpuMask |= (1 << CpuId);
}

/**
  Clear a CPU bit in the mask (non-atomic).

  @param[in,out] pCpuMask  Pointer to the CPU mask.
  @param[in]     CpuId     CPU number to clear from the mask.
**/
static inline
VOID
CpuMaskClear (
  IN OUT CPU_MASK  *pCpuMask,
  IN     UINTN     CpuId
  )
{
  *pCpuMask &= ~(1 << CpuId);
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

//
// Legacy Function and Macro Aliases (for backward compatibility)
//

/** @deprecated Use AtomicCpuMaskSet instead **/
static inline void atomic_cpumask_set (cpumask_t *cpumask, unsigned cpu) {
  AtomicCpuMaskSet (cpumask, cpu);
}

/** @deprecated Use AtomicCpuMaskAnd instead **/
static inline void atomic_cpumask_and (cpumask_t *cpumask, cpumask_t mask) {
  AtomicCpuMaskAnd (cpumask, mask);
}

/** @deprecated Use AtomicCpuMaskClear instead **/
static inline void atomic_cpumask_clear (cpumask_t *cpumask, unsigned cpu) {
  AtomicCpuMaskClear (cpumask, cpu);
}

/** @deprecated Use AtomicCpuMaskRead instead **/
static inline cpumask_t atomic_cpumask (cpumask_t *cpumask) {
  return AtomicCpuMaskRead (cpumask);
}

/** @deprecated Use CpuMaskSet instead **/
static inline void cpumask_set (cpumask_t *cpumask, unsigned cpu) {
  CpuMaskSet (cpumask, cpu);
}

/** @deprecated Use CpuMaskClear instead **/
static inline void cpumask_clear (cpumask_t *cpumask, unsigned cpu) {
  CpuMaskClear (cpumask, cpu);
}

/** @deprecated Use ONCE_CPUMASK instead **/
#define once_cpumask(mask, op)  ONCE_CPUMASK(mask, op)

/** @deprecated Use FOREACH_CPUMASK instead **/
#define foreach_cpumask(mask, op)  FOREACH_CPUMASK(mask, op)

/** @deprecated Internal macro, use FOREACH_CPUMASK instead **/
#define _foreach_cpumask(mask, op, label)  FOREACH_CPUMASK(mask, op)

#endif // NUX_CPUMASK_H
