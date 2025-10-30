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
  CONST CHAR8  *Name;

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
  CONST CHAR8     *Name;

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
  NUXPERF_MEASURE  *WaitCycles;

  ///
  /// Measure for held cycles (time holding lock).
  ///
  NUXPERF_MEASURE  *HeldCycles;
} NUXPERF_LOCK_MEASURE;

//
// Performance Counter Functions
//

/**
  Increment a performance counter.

  Atomically increments the counter value.

  @param[in,out] Counter  Pointer to the performance counter.
**/
static inline
VOID
NuxPerfCounterIncrement (
  IN OUT NUXPERF_COUNTER  *Counter
  )
{
  __atomic_fetch_add (&Counter->Value, 1, __ATOMIC_RELAXED);
}

/**
  Iterate over all performance counters.

  Calls the provided callback function for each performance counter
  registered in the .perfctr section.

  @param[in] CallbackFn  Callback function to invoke for each counter.
  @param[in] Context    Opaque context pointer passed to callback.
**/
static inline
VOID
NuxPerfCounterForEach (
  IN VOID  (*CallbackFn)(VOID *Context, NUXPERF_COUNTER *Counter),
  IN VOID  *Context
  )
{
  extern NUXPERF_COUNTER _nuxperf_start[];
  extern NUXPERF_COUNTER _nuxperf_end[];
  NUXPERF_COUNTER *Ptr = _nuxperf_start;

  while (Ptr < _nuxperf_end) {
    CallbackFn (Context, Ptr++);
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
  NUXPERF_COUNTER *Ptr = _nuxperf_start;

  while (Ptr < _nuxperf_end) {
    printf ("ctr: %-20s\t%16ld\n", Ptr->Name, Ptr->Value);
    Ptr++;
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
  NUXPERF_COUNTER *Ptr = _nuxperf_start;

  while (Ptr < _nuxperf_end) {
    *(volatile UINTN *)&Ptr->Value = 0;
    Ptr++;
  }
}

//
// Performance Measure Functions
//

/**
  Add a sample to a performance measure.

  Updates the min, max, average, and count statistics with the new sample.

  @param[in,out] Measure  Pointer to the performance measure.
  @param[in]     Data      Sample value to add.
**/
static inline
VOID
NuxPerfMeasureAdd (
  IN OUT NUXPERF_MEASURE  *Measure,
  IN     UINT64           Data
  )
{
  while (__sync_lock_test_and_set (&Measure->Lock, 1)) {
    hal_cpu_relax ();
  }
  Measure->Max = Data > Measure->Max ? Data : Measure->Max;
  Measure->Min = Data < Measure->Min ? Data : Measure->Min;
  Measure->Count++;
  Measure->Average = (Measure->Average * (Measure->Count - 1) + Data) / Measure->Count;
  __sync_lock_release (&Measure->Lock);
}

/**
  Iterate over all performance measures.

  Calls the provided callback function for each performance measure
  registered in the .measure section. The measure is locked during
  the callback to ensure consistent reads.

  @param[in] CallbackFn  Callback function to invoke for each measure.
  @param[in] Context    Opaque context pointer passed to callback.
**/
static inline
VOID
NuxPerfMeasureForEach (
  IN VOID  (*CallbackFn)(VOID *Context, NUXPERF_MEASURE *Measure),
  IN VOID  *Context
  )
{
  extern NUXPERF_MEASURE _nuxmeasure_start[];
  extern NUXPERF_MEASURE _nuxmeasure_end[];
  NUXPERF_MEASURE *Ptr = _nuxmeasure_start;

  while (Ptr < _nuxmeasure_end) {
    while (__sync_lock_test_and_set (&Ptr->Lock, 1)) {
      hal_cpu_relax ();
    }
    CallbackFn (Context, Ptr);
    __sync_lock_release (&Ptr->Lock);
    Ptr++;
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
  NUXPERF_MEASURE *Ptr = _nuxmeasure_start;

  //
  // Note: This proceeds unlocked. Use with caution.
  //
  while (Ptr < _nuxmeasure_end) {
    while (__sync_lock_test_and_set (&Ptr->Lock, 1)) {
      hal_cpu_relax ();
    }

    Ptr->Min = (UINT64)-1;
    Ptr->Average = 0;
    Ptr->Max = 0;
    Ptr->Count = 0;

    __sync_lock_release (&Ptr->Lock);
    Ptr++;
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
  NUXPERF_MEASURE *Ptr = _nuxmeasure_start;

  while (Ptr < _nuxmeasure_end) {
    while (__sync_lock_test_and_set (&Ptr->Lock, 1)) {
      hal_cpu_relax ();
    }

    printf ("msr: %-20s\t%16" PRId64 "\n    min/avg/max [ %" PRId64" / %" PRId64" / %" PRId64 " ]\n",
            Ptr->Name, Ptr->Count, Ptr->Min, Ptr->Average, Ptr->Max);
    __sync_lock_release (&Ptr->Lock);
    Ptr++;
  }
}

//
// Measured Spinlock Functions
//

/**
  Acquire a spinlock with measurement.

  Measures the number of cycles spent waiting to acquire the lock.

  @param[in,out] Lock          Pointer to the spinlock.
  @param[in,out] LockMeasure   Pointer to the lock measurement structure.
**/
static inline
VOID
SpinLockMeasured (
  IN OUT SPINLOCK                *Lock,
  IN OUT NUXPERF_LOCK_MEASURE    *LockMeasure
  )
{
  //
  // Measure the number of wait cycles.
  //
  NuxPerfMeasureAdd (LockMeasure->WaitCycles, spinlock_msr(Lock));
}

/**
  Release a spinlock with measurement.

  Measures the number of cycles the lock was held.

  @param[in,out] Lock          Pointer to the spinlock.
  @param[in,out] LockMeasure   Pointer to the lock measurement structure.
**/
static inline
VOID
SpinUnlockMeasured (
  IN OUT SPINLOCK                *Lock,
  IN OUT NUXPERF_LOCK_MEASURE    *LockMeasure
  )
{
  //
  // Measure the number of cycles spent holding the lock.
  //
  NuxPerfMeasureAdd (LockMeasure->HeldCycles, spinunlock_msr(Lock));
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
    .Name = #MeasureName,            \
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
    .Name = #LockName "_waitcy",            \
    .Lock = 0,                               \
    .Min = (UINT64)-1,                       \
    .Max = 0,                                \
    .Average = 0,                            \
    .Count = 0,                              \
  };                                         \
  NUXPERF_MEASURE __measure LockName##_heldcy = {  \
    .Name = #LockName "_heldcy",            \
    .Lock = 0,                               \
    .Min = (UINT64)-1,                       \
    .Max = 0,                                \
    .Average = 0,                            \
    .Count = 0,                              \
  };                                         \
  NUXPERF_LOCK_MEASURE LockName = {         \
    .WaitCycles = &LockName##_waitcy,       \
    .HeldCycles = &LockName##_heldcy,       \
  }

#endif // NUX_NUXPERF_H
