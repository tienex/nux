/** @file
  NUX Synchronization Primitives

  Provides spinlock and reader-writer lock implementations for
  multiprocessor synchronization using atomic operations.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __nux_locks_h__
#define __nux_locks_h__

#include <string.h>
#include <hal/hal.h>

//
// Spinlock Structure
//

/**
  Spinlock

  A basic spinlock using atomic operations for mutual exclusion.
  Includes cycle counter for measuring lock hold time.
**/
typedef struct _SPINLOCK {
  VOLATILE INT32  Lock;    ///< Lock state (0=unlocked, 1=locked)
  UINT64          LockCy;  ///< Cycle count when lock was acquired
} SPINLOCK;

//
// Reader-Writer Lock Structure
//

/**
  Reader-Writer Lock

  Allows multiple concurrent readers or a single writer.
  Implements reader preference semantics.
**/
typedef struct _RWLOCK {
  UINTN     ReaderCount;   ///< Number of active readers
  SPINLOCK  ReaderLock;    ///< Protects reader count
  SPINLOCK  GlobalLock;    ///< Global lock for writer exclusion
} RWLOCK;

//
// Spinlock Operations
//

/**
  Initialize a spinlock.

  Initializes the spinlock to the unlocked state.

  @param[out] Lock  Pointer to the spinlock to initialize.
**/
static INLINE
VOID
SpinLockInitialize (
  OUT SPINLOCK  *Lock
  )
{
  memset (Lock, 0, sizeof (*Lock));
}

/**
  Acquire a spinlock.

  Spins until the lock is acquired. Uses atomic compare-exchange
  with relaxed memory ordering on failure and acquire ordering on success.

  @param[in,out] Lock  Pointer to the spinlock to acquire.
**/
static INLINE
VOID
SpinLockAcquire (
  IN OUT SPINLOCK  *Lock
  )
{
  while (1) {
    //
    // Spin while lock is held
    //
    while (ANX_ATOMIC_LOAD_N (&Lock->Lock, __ATOMIC_RELAXED)) {
      hal_cpu_relax ();
    }

    //
    // Attempt to acquire lock
    //
    INT32 Expected = 0;
    if (ANX_ATOMIC_COMPARE_EXCHANGE_N (
          &Lock->Lock,
          &Expected,
          1,
          TRUE,
          __ATOMIC_ACQUIRE,
          __ATOMIC_RELAXED
          ))
    {
      break;
    }
  }
}

/**
  Acquire a spinlock with measurement.

  Acquires the spinlock and records the cycle count at acquisition time.
  Returns the number of cycles spent waiting for the lock.

  @param[in,out] Lock  Pointer to the spinlock to acquire.

  @return Number of CPU cycles spent waiting for the lock.
**/
static INLINE
UINT64
SpinLockAcquireMeasured (
  IN OUT SPINLOCK  *Lock
  )
{
  UINT64  StartCycles;

  StartCycles = hal_cpu_cycles ();
  SpinLockAcquire (Lock);
  Lock->LockCy = hal_cpu_cycles ();

  return Lock->LockCy - StartCycles;
}

/**
  Release a spinlock.

  Releases the lock using atomic store with release memory ordering.

  @param[in,out] Lock  Pointer to the spinlock to release.
**/
static INLINE
VOID
SpinLockRelease (
  IN OUT SPINLOCK  *Lock
  )
{
  ANX_ATOMIC_STORE_N (&Lock->Lock, 0, __ATOMIC_RELEASE);
}

/**
  Release a spinlock with measurement.

  Releases the lock and returns the number of cycles the lock was held.

  @param[in,out] Lock  Pointer to the spinlock to release.

  @return Number of CPU cycles the lock was held.
**/
static INLINE
UINT64
SpinLockReleaseMeasured (
  IN OUT SPINLOCK  *Lock
  )
{
  UINT64  HoldCycles;

  HoldCycles = hal_cpu_cycles () - Lock->LockCy;
  SpinLockRelease (Lock);

  return HoldCycles;
}

//
// Reader-Writer Lock Operations
//

/**
  Initialize a reader-writer lock.

  Initializes the lock to the unlocked state with no readers.

  @param[out] RwLock  Pointer to the reader-writer lock to initialize.
**/
static INLINE
VOID
RwLockInitialize (
  OUT RWLOCK  *RwLock
  )
{
  RwLock->ReaderCount = 0;
  SpinLockInitialize (&RwLock->ReaderLock);
  SpinLockInitialize (&RwLock->GlobalLock);
}

/**
  Acquire reader lock.

  Allows multiple concurrent readers. The first reader acquires the
  global lock to block writers.

  @param[in,out] RwLock  Pointer to the reader-writer lock.
**/
static INLINE
VOID
RwLockAcquireRead (
  IN OUT RWLOCK  *RwLock
  )
{
  SpinLockAcquire (&RwLock->ReaderLock);

  if (RwLock->ReaderCount++ == 0) {
    //
    // First reader acquires global lock to block writers
    //
    SpinLockAcquire (&RwLock->GlobalLock);
  }

  SpinLockRelease (&RwLock->ReaderLock);
}

/**
  Release reader lock.

  Decrements reader count. The last reader releases the global lock
  to allow writers.

  @param[in,out] RwLock  Pointer to the reader-writer lock.
**/
static INLINE
VOID
RwLockReleaseRead (
  IN OUT RWLOCK  *RwLock
  )
{
  SpinLockAcquire (&RwLock->ReaderLock);

  if (--RwLock->ReaderCount == 0) {
    //
    // Last reader releases global lock for writers
    //
    SpinLockRelease (&RwLock->GlobalLock);
  }

  SpinLockRelease (&RwLock->ReaderLock);
}

/**
  Acquire writer lock.

  Provides exclusive access by acquiring the global lock.
  Blocks all readers and other writers.

  @param[in,out] RwLock  Pointer to the reader-writer lock.
**/
static INLINE
VOID
RwLockAcquireWrite (
  IN OUT RWLOCK  *RwLock
  )
{
  SpinLockAcquire (&RwLock->GlobalLock);
}

/**
  Release writer lock.

  Releases exclusive access by releasing the global lock.

  @param[in,out] RwLock  Pointer to the reader-writer lock.
**/
static INLINE
VOID
RwLockReleaseWrite (
  IN OUT RWLOCK  *RwLock
  )
{
  SpinLockRelease (&RwLock->GlobalLock);
}

#endif // NUX_LOCKS_H
