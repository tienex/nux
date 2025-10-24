/** @file
  NUX Slab Allocator Interface

  Provides a slab-based memory allocation system for efficient allocation
  of fixed-size objects with optional constructors and cache alignment.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef NUX_SLAB_H
#define NUX_SLAB_H

#include <nux/locks.h>

//
// Spinlock Macro Definitions for Slab Implementation
//

#define DECLARE_SPIN_LOCK(_x)  lock_t _x
#define SPIN_LOCK_INIT(_x)     do { _x = 0; } while (0)
#define SPIN_LOCK(_x)          spinlock(&_x)
#define SPIN_UNLOCK(_x)        spinunlock(&_x)
#define SPIN_LOCK_FREE(_x)

//
// Include Slab Implementation
//

#include "slabinc.h"

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

  @param[in,out] pSlab       Pointer to slab structure to initialize.
  @param[in]     pName       Name of the slab cache for debugging.
  @param[in]     ObjectSize  Size of each object in bytes.
  @param[in]     Constructor Optional constructor function called on new objects.
  @param[in]     CacheAlign  If non-zero, align objects to cache line boundaries.
**/
VOID
SlabRegister (
  IN OUT struct slab  *pSlab,
  IN     CONST CHAR8  *pName,
  IN     UINTN        ObjectSize,
  IN     VOID         (*Constructor)(VOID *, VOID *, INT32) OPTIONAL,
  IN     INT32        CacheAlign
  );

/**
  Deregister a slab cache.

  Frees all resources associated with the slab cache and removes it
  from the system. All objects must be freed before calling this function.

  @param[in,out] pSlab  Pointer to the slab structure to deregister.
**/
VOID
SlabDeregister (
  IN OUT struct slab  *pSlab
  );

/**
  Shrink a slab cache.

  Attempts to free empty slabs to reduce memory usage.

  @param[in,out] pSlab  Pointer to the slab structure to shrink.

  @return Number of slabs freed.
**/
INT32
SlabShrink (
  IN OUT struct slab  *pSlab
  );

/**
  Allocate an object from a slab cache with opaque data.

  Allocates an object from the slab cache, passing opaque data to the
  constructor if one is defined.

  @param[in,out] pSlab   Pointer to the slab cache.
  @param[in]     pOpaque Opaque data passed to constructor, or NULL.

  @return Pointer to allocated object, or NULL on failure.
**/
VOID *
SlabAllocateOpaque (
  IN OUT struct slab  *pSlab,
  IN     VOID         *pOpaque OPTIONAL
  );

/**
  Free an object back to its slab cache.

  Returns the object to the slab cache it was allocated from.

  @param[in] pObject  Pointer to the object to free.
**/
VOID
SlabFree (
  IN VOID  *pObject
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

  Convenience function that allocates an object without passing opaque
  data to the constructor.

  @param[in,out] pSlab  Pointer to the slab cache.

  @return Pointer to allocated object, or NULL on failure.
**/
static inline
VOID *
SlabAllocate (
  IN OUT struct slab  *pSlab
  )
{
  return SlabAllocateOpaque (pSlab, NULL);
}

//
// Legacy Function Aliases (for backward compatibility)
//

/** @deprecated Use SlabRegister instead **/
static inline void slab_register (
  struct slab *sc,
  const char *name,
  size_t objsize,
  void (*ctr)(void *, void *, int),
  int cachealign
) {
  SlabRegister (sc, name, objsize, ctr, cachealign);
}

/** @deprecated Use SlabDeregister instead **/
static inline void slab_deregister (struct slab *sc) {
  SlabDeregister (sc);
}

/** @deprecated Use SlabShrink instead **/
static inline int slab_shrink (struct slab *sc) {
  return SlabShrink (sc);
}

/** @deprecated Use SlabAllocateOpaque instead **/
static inline void *slab_alloc_opq (struct slab *sc, void *opq) {
  return SlabAllocateOpaque (sc, opq);
}

/** @deprecated Use SlabFree instead **/
static inline void slab_free (void *ptr) {
  SlabFree (ptr);
}

/** @deprecated Use SlabPrintStatistics instead **/
static inline void slab_printstats (void) {
  SlabPrintStatistics ();
}

/** @deprecated Use SlabAllocate instead **/
static inline void *slab_alloc (struct slab *sc) {
  return SlabAllocate (sc);
}

#endif // NUX_SLAB_H
