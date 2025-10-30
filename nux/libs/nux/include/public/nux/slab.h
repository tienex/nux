/** @file
  NUX Slab Allocator Interface

  Provides a slab-based memory allocation system for efficient allocation
  of fixed-size objects with optional constructors and cache alignment.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __nux_slab_h__
#define __nux_slab_h__

#include <nux/locks.h>

//
// Spinlock Macro Definitions for Slab Implementation
//

#define DECLARE_SPIN_LOCK(_x)  SPINLOCK _x
#define SPIN_LOCK_INIT(_x)     SpinLockInitialize(&(_x))
#define SPIN_LOCK(_x)          SpinLockAcquire(&(_x))
#define SPIN_UNLOCK(_x)        SpinLockRelease(&(_x))
#define SPIN_LOCK_FREE(_x)

//
// Include Slab Implementation
//

#include <nux/slabinc.h>

//
// Cleanup Macro Definitions
//

#undef DECLARE_SPIN_LOCK
#undef SPIN_LOCK_INIT
#undef SPIN_LOCK
#undef SPIN_UNLOCK
#undef SPIN_LOCK_FREE

//
// Slab Allocator Functions
//

/**
  Register a slab cache.

  Creates and registers a new slab cache for allocating objects of a
  fixed size. Optionally allows specifying a constructor and cache alignment.

  @param[in,out] Slab       Pointer to slab structure to initialize.
  @param[in]     Name       Name of the slab cache for debugging.
  @param[in]     ObjectSize  Size of each object in bytes.
  @param[in]     Constructor Optional constructor function called on new objects.
  @param[in]     CacheAlign  If non-zero, align objects to cache line boundaries.
**/
VOID
SlabRegister (
  IN OUT struct slab  *Slab,
  IN     CONST CHAR8  *Name,
  IN     UINTN        ObjectSize,
  IN     VOID         (*Constructor)(VOID *, VOID *, INT32) OPTIONAL,
  IN     INT32        CacheAlign
  );

/**
  Deregister a slab cache.

  Frees all resources associated with the slab cache and removes it
  from the system. All objects must be freed before calling this function.

  @param[in,out] Slab  Pointer to the slab structure to deregister.
**/
VOID
SlabDeregister (
  IN OUT struct slab  *Slab
  );

/**
  Shrink a slab cache.

  Attempts to free empty slabs to reduce memory usage.

  @param[in,out] Slab  Pointer to the slab structure to shrink.

  @return Number of slabs freed.
**/
INT32
SlabShrink (
  IN OUT struct slab  *Slab
  );

/**
  Allocate an object from a slab cache with constructor data.

  Allocates an object from the slab cache, passing optional data to the
  constructor if one is defined.

  @param[in,out] Slab           Pointer to the slab cache.
  @param[in]     ConstructorData Optional data passed to constructor, or NULL.

  @return Pointer to allocated object, or NULL on failure.
**/
VOID *
SlabAllocateOpaque (
  IN OUT struct slab  *Slab,
  IN     VOID         *ConstructorData OPTIONAL
  );

/**
  Free an object back to its slab cache.

  Returns the object to the slab cache it was allocated from.

  @param[in] Object  Pointer to the object to free.
**/
VOID
SlabFree (
  IN VOID  *Object
  );

/**
  Print slab allocator statistics.

  Outputs statistics for all registered slab caches to the console,
  useful for debugging and monitoring memory usage.
**/
VOID
SlabPrintStatistics (
  VOID
  );

/**
  Allocate an object from a slab cache.

  Convenience function that allocates an object without passing constructor
  data to the constructor.

  @param[in,out] Slab  Pointer to the slab cache.

  @return Pointer to allocated object, or NULL on failure.
**/
static INLINE
VOID *
SlabAllocate (
  IN OUT struct slab  *Slab
  )
{
  return SlabAllocateOpaque (Slab, NULL);
}

#endif // NUX_SLAB_H
