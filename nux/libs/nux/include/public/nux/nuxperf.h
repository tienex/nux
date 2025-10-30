/** @file
  NUX Performance Measurement System

  Provides performance counters and measures for profiling NUX kernel operations.

  Features:
  - Performance Counters: Simple atomic counters for event counting
  - Performance Measures: Statistical measures (min/avg/max) for timing
  - Measured Spinlocks: Lock contention and hold time measurement

  All counters and measures are stored in special linker sections and can be
  enumerated, printed, and reset at runtime.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __nux_nuxperf_h__
#define __nux_nuxperf_h__

#include <string.h>
#include <nux/locks.h>

//
// Section Attributes
//

#define __perf     __attribute__((section(".perfctr")))       ///< Performance counter section
#define __measure  __attribute__((section(".measure"), aligned(1)))  ///< Performance measure section

//
// NUXPERF_COUNTER - Performance Counter
//

/**
  Performance counter structure.

  A simple atomic counter for tracking event occurrences. Counters are
  stored in the .perfctr linker section and can be enumerated at runtime.
**/
typedef struct _NUXPERF_COUNTER {
  ///
  /// Counter name (for display purposes).
  ///
  CONST CHAR8  *pName;

  ///
  /// Counter value (atomically incremented).
  ///
  UINTN        Value;
} NUXPERF_COUNTER;

//
// NUXPERF_MEASURE - Performance Measure
//

/**
  Performance measure structure.

  Tracks statistical information (min, avg, max) for timed operations.
  Measures are stored in the .measure linker section.
**/
typedef struct _NUXPERF_MEASURE {
  ///
  /// Measure name (for display purposes).
  ///
  CONST CHAR8     *pName;

  ///
  /// Spinlock for updating statistics.
  ///
  volatile UINTN  Lock;

  ///
  /// Minimum value observed.
  ///
  UINT64          Min;

  ///
  /// Maximum value observed.
  ///
  UINT64          Max;

  ///
  /// Average value (running average).
  ///
  UINT64          Average;

  ///
  /// Number of samples.
  ///
  UINT64          Count;
} NUXPERF_MEASURE;

//
// NUXPERF_LOCK_MEASURE - Lock Performance Measurement
//

/**
  Lock performance measurement structure.

  Tracks both wait cycles (time spent acquiring the lock) and
  held cycles (time spent holding the lock).
**/
typedef struct _NUXPERF_LOCK_MEASURE {
  ///
  /// Measure for wait cycles (time acquiring lock).
  ///
  NUXPERF_MEASURE  *pWaitCycles;

  ///
  /// Measure for held cycles (time holding lock).
  ///
  NUXPERF_MEASURE  *pHeldCycles;
} NUXPERF_LOCK_MEASURE;

//
// Performance Counter Functions
//

/**
  Increment a performance counter.

  Atomically increments the counter value.

  @param[in,out] pCounter  Pointer to the performance counter.
**/
static inline
VOID
NuxPerfCounterIncrement (
  IN OUT NUXPERF_COUNTER  *pCounter
  )
{
  __atomic_fetch_add (&pCounter->Value, 1, __ATOMIC_RELAXED);
}

/**
  Iterate over all performance counters.

  Calls the provided callback function for each performance counter
  registered in the .perfctr section.

  @param[in] CallbackFn  Callback function to invoke for each counter.
  @param[in] pContext    Opaque context pointer passed to callback.
**/
static inline
VOID
NuxPerfCounterForEach (
  IN VOID  (*CallbackFn)(VOID *pContext, NUXPERF_COUNTER *pCounter),
  IN VOID  *pContext
  )
{
  extern NUXPERF_COUNTER _nuxperf_start[];
  extern NUXPERF_COUNTER _nuxperf_end[];
  NUXPERF_COUNTER *pPtr = _nuxperf_start;

  while (pPtr < _nuxperf_end) {
    CallbackFn (pContext, pPtr++);
  }
}

/**
  Print all performance counters.

  Displays all registered performance counters and their values.
**/
static inline
VOID
NuxPerfCounterPrint (
  VOID
  )
{
  extern NUXPERF_COUNTER _nuxperf_start[];
  extern NUXPERF_COUNTER _nuxperf_end[];
  NUXPERF_COUNTER *pPtr = _nuxperf_start;

  while (pPtr < _nuxperf_end) {
    printf ("ctr: %-20s\t%16ld\n", pPtr->pName, pPtr->Value);
    pPtr++;
  }
}

/**
  Reset all performance counters to zero.
**/
static inline
VOID
NuxPerfCounterReset (
  VOID
  )
{
  extern NUXPERF_COUNTER _nuxperf_start[];
  extern NUXPERF_COUNTER _nuxperf_end[];
  NUXPERF_COUNTER *pPtr = _nuxperf_start;

  while (pPtr < _nuxperf_end) {
    *(volatile UINTN *)&pPtr->Value = 0;
    pPtr++;
  }
}

//
// Performance Measure Functions
//

/**
  Add a sample to a performance measure.

  Updates the min, max, average, and count statistics with the new sample.

  @param[in,out] pMeasure  Pointer to the performance measure.
  @param[in]     Data      Sample value to add.
**/
static inline
VOID
NuxPerfMeasureAdd (
  IN OUT NUXPERF_MEASURE  *pMeasure,
  IN     UINT64           Data
  )
{
  while (__sync_lock_test_and_set (&pMeasure->Lock, 1)) {
    hal_cpu_relax ();
  }
  pMeasure->Max = Data > pMeasure->Max ? Data : pMeasure->Max;
  pMeasure->Min = Data < pMeasure->Min ? Data : pMeasure->Min;
  pMeasure->Count++;
  pMeasure->Average = (pMeasure->Average * (pMeasure->Count - 1) + Data) / pMeasure->Count;
  __sync_lock_release (&pMeasure->Lock);
}

/**
  Iterate over all performance measures.

  Calls the provided callback function for each performance measure
  registered in the .measure section. The measure is locked during
  the callback to ensure consistent reads.

  @param[in] CallbackFn  Callback function to invoke for each measure.
  @param[in] pContext    Opaque context pointer passed to callback.
**/
static inline
VOID
NuxPerfMeasureForEach (
  IN VOID  (*CallbackFn)(VOID *pContext, NUXPERF_MEASURE *pMeasure),
  IN VOID  *pContext
  )
{
  extern NUXPERF_MEASURE _nuxmeasure_start[];
  extern NUXPERF_MEASURE _nuxmeasure_end[];
  NUXPERF_MEASURE *pPtr = _nuxmeasure_start;

  while (pPtr < _nuxmeasure_end) {
    while (__sync_lock_test_and_set (&pPtr->Lock, 1)) {
      hal_cpu_relax ();
    }
    CallbackFn (pContext, pPtr);
    __sync_lock_release (&pPtr->Lock);
    pPtr++;
  }
}

/**
  Reset all performance measures.

  Resets min/avg/max/count to initial values for all registered measures.
**/
static inline
VOID
NuxPerfMeasureReset (
  VOID
  )
{
  extern NUXPERF_MEASURE _nuxmeasure_start[];
  extern NUXPERF_MEASURE _nuxmeasure_end[];
  NUXPERF_MEASURE *pPtr = _nuxmeasure_start;

  //
  // Note: This proceeds unlocked. Use with caution.
  //
  while (pPtr < _nuxmeasure_end) {
    while (__sync_lock_test_and_set (&pPtr->Lock, 1)) {
      hal_cpu_relax ();
    }

    pPtr->Min = (UINT64)-1;
    pPtr->Average = 0;
    pPtr->Max = 0;
    pPtr->Count = 0;

    __sync_lock_release (&pPtr->Lock);
    pPtr++;
  }
}

/**
  Print all performance measures.

  Displays all registered performance measures with their statistics.
**/
static inline
VOID
NuxPerfMeasurePrint (
  VOID
  )
{
  extern NUXPERF_MEASURE _nuxmeasure_start[];
  extern NUXPERF_MEASURE _nuxmeasure_end[];
  NUXPERF_MEASURE *pPtr = _nuxmeasure_start;

  while (pPtr < _nuxmeasure_end) {
    while (__sync_lock_test_and_set (&pPtr->Lock, 1)) {
      hal_cpu_relax ();
    }

    printf ("msr: %-20s\t%16" PRId64 "\n    min/avg/max [ %" PRId64" / %" PRId64" / %" PRId64 " ]\n",
            pPtr->pName, pPtr->Count, pPtr->Min, pPtr->Average, pPtr->Max);
    __sync_lock_release (&pPtr->Lock);
    pPtr++;
  }
}

//
// Measured Spinlock Functions
//

/**
  Acquire a spinlock with measurement.

  Measures the number of cycles spent waiting to acquire the lock.

  @param[in,out] pLock          Pointer to the spinlock.
  @param[in,out] pLockMeasure   Pointer to the lock measurement structure.
**/
static inline
VOID
SpinLockMeasured (
  IN OUT SPINLOCK                *pLock,
  IN OUT NUXPERF_LOCK_MEASURE    *pLockMeasure
  )
{
  //
  // Measure the number of wait cycles.
  //
  NuxPerfMeasureAdd (pLockMeasure->pWaitCycles, spinlock_msr(pLock));
}

/**
  Release a spinlock with measurement.

  Measures the number of cycles the lock was held.

  @param[in,out] pLock          Pointer to the spinlock.
  @param[in,out] pLockMeasure   Pointer to the lock measurement structure.
**/
static inline
VOID
SpinUnlockMeasured (
  IN OUT SPINLOCK                *pLock,
  IN OUT NUXPERF_LOCK_MEASURE    *pLockMeasure
  )
{
  //
  // Measure the number of cycles spent holding the lock.
  //
  NuxPerfMeasureAdd (pLockMeasure->pHeldCycles, spinunlock_msr(pLock));
}

//
// Declaration and Definition Macros
//

/**
  Declare an external performance measure.

  @param  MeasureName  Name of the measure variable.
**/
#define DECLARE_MEASURE(MeasureName)  \
  extern __measure NUXPERF_MEASURE MeasureName

/**
  Define a performance measure.

  Creates a performance measure variable in the .measure section.

  @param  MeasureName  Name of the measure variable.
**/
#define DEFINE_MEASURE(MeasureName)   \
  __measure NUXPERF_MEASURE MeasureName = {  \
    .pName = #MeasureName,            \
    .Lock = 0,                        \
    .Min = (UINT64)-1,                \
    .Max = 0,                         \
    .Average = 0,                     \
    .Count = 0,                       \
  }

/**
  Declare an external lock measure.

  @param  LockName  Name of the lock measure variable.
**/
#define DECLARE_LOCK_MEASURE(LockName)  \
  extern NUXPERF_LOCK_MEASURE LockName

/**
  Define a lock measure.

  Creates wait and held cycle measures for a lock and a lock measure
  structure that references them.

  @param  LockName  Name of the lock measure variable.
**/
#define DEFINE_LOCK_MEASURE(LockName)        \
  NUXPERF_MEASURE __measure LockName##_waitcy = {  \
    .pName = #LockName "_waitcy",            \
    .Lock = 0,                               \
    .Min = (UINT64)-1,                       \
    .Max = 0,                                \
    .Average = 0,                            \
    .Count = 0,                              \
  };                                         \
  NUXPERF_MEASURE __measure LockName##_heldcy = {  \
    .pName = #LockName "_heldcy",            \
    .Lock = 0,                               \
    .Min = (UINT64)-1,                       \
    .Max = 0,                                \
    .Average = 0,                            \
    .Count = 0,                              \
  };                                         \
  NUXPERF_LOCK_MEASURE LockName = {         \
    .pWaitCycles = &LockName##_waitcy,       \
    .pHeldCycles = &LockName##_heldcy,       \
  }

//
// Legacy Type Aliases (for backward compatibility)
//

/** @deprecated Use NUXPERF_COUNTER instead **/
typedef NUXPERF_COUNTER nuxperf_t;

/** @deprecated Use NUXPERF_MEASURE instead **/
typedef NUXPERF_MEASURE nuxmeasure_t;

/** @deprecated Use NUXPERF_LOCK_MEASURE instead **/
typedef NUXPERF_LOCK_MEASURE lock_measure_t;

//
// Legacy Function Wrappers
//

/** @deprecated Use NuxPerfCounterIncrement instead **/
static inline void nuxperf_inc(nuxperf_t *ctr) {
  NuxPerfCounterIncrement (ctr);
}

/** @deprecated Use NuxPerfCounterForEach instead **/
static inline void nuxperf_foreach (void (*fn)(void *opq, nuxperf_t *ctr), void *opq) {
  NuxPerfCounterForEach (fn, opq);
}

/** @deprecated Use NuxPerfCounterPrint instead **/
static inline void nuxperf_print (void) {
  NuxPerfCounterPrint ();
}

/** @deprecated Use NuxPerfCounterReset instead **/
static inline void nuxperf_reset (void) {
  NuxPerfCounterReset ();
}

/** @deprecated Use NuxPerfMeasureAdd instead **/
static inline void nuxmeasure_add (nuxmeasure_t *msr, UINT64 data) {
  NuxPerfMeasureAdd (msr, data);
}

/** @deprecated Use NuxPerfMeasureForEach instead **/
static inline void nuxmeasure_foreach (void (*fn)(void *opq, nuxmeasure_t *msr), void *opq) {
  NuxPerfMeasureForEach (fn, opq);
}

/** @deprecated Use NuxPerfMeasureReset instead **/
static inline void nuxmeasure_reset (void) {
  NuxPerfMeasureReset ();
}

/** @deprecated Use NuxPerfMeasurePrint instead **/
static inline void nuxmeasure_print (void) {
  NuxPerfMeasurePrint ();
}

/** @deprecated Use SpinLockMeasured instead **/
static inline void spinlock_measured (lock_t *lock, lock_measure_t *lm) {
  SpinLockMeasured (lock, lm);
}

/** @deprecated Use SpinUnlockMeasured instead **/
static inline void spinunlock_measured (lock_t *lock, lock_measure_t *lm) {
  SpinUnlockMeasured (lock, lm);
}

#endif // NUX_NUXPERF_H
