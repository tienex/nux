/** @file
  Slab Allocator Internal Definitions

  Internal data structures for the NUX slab allocator implementation.

  WARNING: Do not include this file directly. It must be included indirectly
  by the slab allocator implementation files only.

  This file provides the core data structures for a simple and portable
  slab allocator that manages object caching with optional spinlock support.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __nux_slabinc_h__
#define __nux_slabinc_h__

#include <queue.h>
#include <stddef.h>

//
// Spinlock Integration (Optional)
//

/**
  To enable spinlock support, define the following macros before including:

  #define DECLARE_SPIN_LOCK(Name)   <spinlock type declaration>
  #define SPIN_LOCK_INIT(Lock)      <initialize spinlock>
  #define SPIN_LOCK(Lock)           <acquire spinlock>
  #define SPIN_UNLOCK(Lock)         <release spinlock>
  #define SPIN_LOCK_FREE(Lock)      <destroy spinlock>

  Example:
  #define DECLARE_SPIN_LOCK(_x)  SPINLOCK _x
  #define SPIN_LOCK_INIT(_x)     SpinLockInitialize(&(_x))
  #define SPIN_LOCK(_x)          SpinLockAcquire(&(_x))
  #define SPIN_UNLOCK(_x)        SpinLockRelease(&(_x))
  #define SPIN_LOCK_FREE(_x)     SpinLockDestroy(&(_x))
**/

//
// Forward Declarations
//

struct slab;
struct objhdr;

//
// SLAB_HEADER - Slab Page Header
//

/**
  Slab page header structure.

  Each slab page begins with this header which tracks the page's metadata
  including the owning cache, free object count, and free object list.
  The structure is sized to fit within a cache line (64 bytes).
**/
struct slabhdr {
  union {
    struct {
      ///
      /// Magic number for validation.
      ///
      UINTN          Magic;

      ///
      /// Pointer to owning slab cache.
      ///
      struct slab    *pCache;

      ///
      /// Number of free objects in this slab.
      ///
      UINTN          FreeCount;

      ///
      /// SLIST of free objects.
      ///
      SLIST_HEAD (, objhdr) FreeQueue;

      ///
      /// LIST entry for linking slabs in cache queues.
      ///
      LIST_ENTRY (slabhdr) ListEntry;
    };

    ///
    /// Padding to cache line size (64 bytes).
    ///
    CHAR8  CacheLine[64];
  };
};

//
// SLAB - Slab Cache
//

/**
  Slab cache structure.

  Manages a pool of fixed-size objects with optional constructor callback.
  Objects are organized into slab pages that are linked into three queues:
  - Empty: Slabs with all objects free
  - Free: Slabs with some objects allocated and some free
  - Full: Slabs with all objects allocated

  The cache optionally includes a spinlock for thread-safe operation.
**/
struct slab {
#ifdef DECLARE_SPIN_LOCK
  ///
  /// Spinlock for thread-safe access (if DECLARE_SPIN_LOCK is defined).
  ///
  DECLARE_SPIN_LOCK (Lock);
#endif

  ///
  /// Cache name (for debugging/statistics).
  ///
  CONST CHAR8  *pName;

  ///
  /// Size of each object in bytes.
  ///
  UINTN        ObjectSize;

  ///
  /// Optional constructor/destructor callback.
  /// Called with (object, opaque, deconstruct_flag).
  ///
  VOID         (*Constructor)(VOID *pObject, VOID *pOpaque, INT32 Deconstruct);

  ///
  /// Number of empty slabs.
  ///
  UINTN        EmptyCount;

  ///
  /// Number of partially free slabs.
  ///
  UINTN        FreeCount;

  ///
  /// Number of full slabs.
  ///
  UINTN        FullCount;

  ///
  /// LIST of empty slabs (all objects free).
  ///
  LIST_HEAD (, slabhdr) EmptyQueue;

  ///
  /// LIST of partially free slabs (some objects allocated).
  ///
  LIST_HEAD (, slabhdr) FreeQueue;

  ///
  /// LIST of full slabs (all objects allocated).
  ///
  LIST_HEAD (, slabhdr) FullQueue;

  ///
  /// LIST entry for linking caches together.
  ///
  LIST_ENTRY (slab) ListEntry;
};

#endif // SLABINC_H
