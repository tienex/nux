/** @file
  NUX Synchronization Primitives

  Provides spinlock and reader-writer lock implementations for
  multiprocessor synchronization using atomic operations.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef NUX_LOCKS_H
#define NUX_LOCKS_H

#include <string.h>
#include <hal.h>

//
// Spinlock Structure
//

/**
  Spinlock

  A basic spinlock using atomic operations for mutual exclusion.
  Includes cycle counter for measuring lock hold time.
**/
typedef struct _SPINLOCK {
  volatile INT32  Lock;    ///< Lock state (0=unlocked, 1=locked)
  UINT64          LockCy;  ///< Cycle count when lock was acquired
} SPINLOCK;

/** Legacy type alias for compatibility **/
typedef SPINLOCK lock_t;

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

/** Legacy type alias for compatibility **/
typedef RWLOCK rwlock_t;

//
// Spinlock Operations
//

/**
  Initialize a spinlock.

  Initializes the spinlock to the unlocked state.

  @param[out] pLock  Pointer to the spinlock to initialize.
**/
static inline
VOID
SpinLockInitialize (
  OUT SPINLOCK  *pLock
  )
{
  memset (pLock, 0, sizeof (*pLock));
}

/**
  Acquire a spinlock.

  Spins until the lock is acquired. Uses atomic compare-exchange
  with relaxed memory ordering on failure and acquire ordering on success.

  @param[in,out] pLock  Pointer to the spinlock to acquire.
**/
static inline
VOID
SpinLockAcquire (
  IN OUT SPINLOCK  *pLock
  )
{
  while (1) {
    //
    // Spin while lock is held
    //
    while (__atomic_load_n (&pLock->Lock, __ATOMIC_RELAXED)) {
      hal_cpu_relax ();
    }

    //
    // Attempt to acquire lock
    //
    INT32 Expected = 0;
    if (__atomic_compare_exchange_n (
          &pLock->Lock,
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

  @param[in,out] pLock  Pointer to the spinlock to acquire.

  @return Number of CPU cycles spent waiting for the lock.
**/
static inline
UINT64
SpinLockAcquireMeasured (
  IN OUT SPINLOCK  *pLock
  )
{
  UINT64  StartCycles;

  StartCycles = hal_cpu_cycles ();
  SpinLockAcquire (pLock);
  pLock->LockCy = hal_cpu_cycles ();

  return pLock->LockCy - StartCycles;
}

/**
  Release a spinlock.

  Releases the lock using atomic store with release memory ordering.

  @param[in,out] pLock  Pointer to the spinlock to release.
**/
static inline
VOID
SpinLockRelease (
  IN OUT SPINLOCK  *pLock
  )
{
  __atomic_store_n (&pLock->Lock, 0, __ATOMIC_RELEASE);
}

/**
  Release a spinlock with measurement.

  Releases the lock and returns the number of cycles the lock was held.

  @param[in,out] pLock  Pointer to the spinlock to release.

  @return Number of CPU cycles the lock was held.
**/
static inline
UINT64
SpinLockReleaseMeasured (
  IN OUT SPINLOCK  *pLock
  )
{
  UINT64  HoldCycles;

  HoldCycles = hal_cpu_cycles () - pLock->LockCy;
  SpinLockRelease (pLock);

  return HoldCycles;
}

//
// Reader-Writer Lock Operations
//

/**
  Initialize a reader-writer lock.

  Initializes the lock to the unlocked state with no readers.

  @param[out] pRwLock  Pointer to the reader-writer lock to initialize.
**/
static inline
VOID
RwLockInitialize (
  OUT RWLOCK  *pRwLock
  )
{
  pRwLock->ReaderCount = 0;
  SpinLockInitialize (&pRwLock->ReaderLock);
  SpinLockInitialize (&pRwLock->GlobalLock);
}

/**
  Acquire reader lock.

  Allows multiple concurrent readers. The first reader acquires the
  global lock to block writers.

  @param[in,out] pRwLock  Pointer to the reader-writer lock.
**/
static inline
VOID
RwLockAcquireRead (
  IN OUT RWLOCK  *pRwLock
  )
{
  SpinLockAcquire (&pRwLock->ReaderLock);

  if (pRwLock->ReaderCount++ == 0) {
    //
    // First reader acquires global lock to block writers
    //
    SpinLockAcquire (&pRwLock->GlobalLock);
  }

  SpinLockRelease (&pRwLock->ReaderLock);
}

/**
  Release reader lock.

  Decrements reader count. The last reader releases the global lock
  to allow writers.

  @param[in,out] pRwLock  Pointer to the reader-writer lock.
**/
static inline
VOID
RwLockReleaseRead (
  IN OUT RWLOCK  *pRwLock
  )
{
  SpinLockAcquire (&pRwLock->ReaderLock);

  if (--pRwLock->ReaderCount == 0) {
    //
    // Last reader releases global lock for writers
    //
    SpinLockRelease (&pRwLock->GlobalLock);
  }

  SpinLockRelease (&pRwLock->ReaderLock);
}

/**
  Acquire writer lock.

  Provides exclusive access by acquiring the global lock.
  Blocks all readers and other writers.

  @param[in,out] pRwLock  Pointer to the reader-writer lock.
**/
static inline
VOID
RwLockAcquireWrite (
  IN OUT RWLOCK  *pRwLock
  )
{
  SpinLockAcquire (&pRwLock->GlobalLock);
}

/**
  Release writer lock.

  Releases exclusive access by releasing the global lock.

  @param[in,out] pRwLock  Pointer to the reader-writer lock.
**/
static inline
VOID
RwLockReleaseWrite (
  IN OUT RWLOCK  *pRwLock
  )
{
  SpinLockRelease (&pRwLock->GlobalLock);
}

//
// Legacy Function Aliases (for backward compatibility)
//

/** @deprecated Use SpinLockInitialize instead **/
static inline void spinlock_init (lock_t *l) {
  SpinLockInitialize (l);
}

/** @deprecated Use SpinLockAcquire instead **/
static inline void spinlock (lock_t *l) {
  SpinLockAcquire (l);
}

/** @deprecated Use SpinLockAcquireMeasured instead **/
static inline uint64_t spinlock_msr (lock_t *l) {
  return SpinLockAcquireMeasured (l);
}

/** @deprecated Use SpinLockRelease instead **/
static inline void spinunlock (lock_t *l) {
  SpinLockRelease (l);
}

/** @deprecated Use SpinLockReleaseMeasured instead **/
static inline uint64_t spinunlock_msr (lock_t *l) {
  return SpinLockReleaseMeasured (l);
}

/** @deprecated Use RwLockInitialize instead **/
static inline void rwlock_init (rwlock_t *rw) {
  RwLockInitialize (rw);
}

/** @deprecated Use RwLockAcquireRead instead **/
static inline void readlock (rwlock_t *rw) {
  RwLockAcquireRead (rw);
}

/** @deprecated Use RwLockReleaseRead instead **/
static inline void readunlock (rwlock_t *rw) {
  RwLockReleaseRead (rw);
}

/** @deprecated Use RwLockAcquireWrite instead **/
static inline void writelock (rwlock_t *rw) {
  RwLockAcquireWrite (rw);
}

/** @deprecated Use RwLockReleaseWrite instead **/
static inline void writeunlock (rwlock_t *rw) {
  RwLockReleaseWrite (rw);
}

#endif // NUX_LOCKS_H
