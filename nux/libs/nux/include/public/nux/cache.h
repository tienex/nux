/** @file
  NUX Generic Cache Implementation

  Provides a simple, efficient cache implementation using red-black trees
  for fast lookups and LRU (Least Recently Used) eviction policy.

  This is a template-style implementation that requires the user to define
  a fill callback function for cache misses.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __nux_cache_h__
#define __nux_cache_h__

#include <assert.h>
#include <stdint.h>
#include <rbtree.h>

//
// Cache Slot Structure
//

/**
  Cache Slot

  Represents a single cache entry with LRU tracking and reference counting.
  Uses red-black tree node for fast lookups by address.
**/
typedef struct _SLOT
{
  rb_node_t           RbNode;     ///< Red-black tree node (must be first)
  TAILQ_ENTRY (_SLOT) LruEntry;   ///< LRU queue entry

  uintptr_t  Address;             ///< Cached address
  struct {
    uint32_t  Valid:1;            ///< Entry is valid
    uint32_t  RefCount:31;        ///< Reference count
  };
} SLOT, *PSLOT, *PCSLOT;

/** Legacy type alias for compatibility **/
#define slot SLOT

//
// Cache Structure
//

/**
  Cache

  Main cache structure managing a fixed number of slots with LRU eviction.
  Uses red-black tree for O(log n) lookups and TAILQ for LRU ordering.
**/
typedef struct _CACHE
{
  rb_tree_t           Map;        ///< Red-black tree for address lookup (must be first)
  TAILQ_HEAD (, _SLOT) FreeList;  ///< LRU free list
  lock_t              Lock;       ///< Protects cache structure

  /**
    Fill callback function.

    Called when a cache miss occurs to populate the slot with new data.

    @param[in] SlotNumber  Slot number being filled (0 to NumSlots-1).
    @param[in] OldAddress  Address of entry being evicted.
    @param[in] NewAddress  Address of entry being loaded.
  **/
  VOID (*Fill)(UINTN SlotNumber, uintptr_t OldAddress, uintptr_t NewAddress);

  UINTN  NumSlots;                ///< Total number of cache slots
  SLOT   *Slots;                  ///< Array of cache slots
} CACHE, *PCACHE, *PCCACHE;

/** Legacy type alias for compatibility **/
#define cache CACHE

//
// Internal Comparison Functions
//

/**
  Compare two cache slots by address.

  Used by red-black tree for slot ordering.

  @param[in] Context  Unused context pointer.
  @param[in] SlotA    First slot to compare.
  @param[in] SlotB    Second slot to compare.

  @return -1 if A < B, 0 if A == B, 1 if A > B.
**/
static
INT32
CacheSlotCompare (
  IN VOID        *Context,
  IN CONST VOID  *SlotA,
  IN CONST VOID  *SlotB
  )
{
  CONST SLOT *Slot1;
  CONST SLOT *Slot2;

  Slot1 = (CONST SLOT *)SlotA;
  Slot2 = (CONST SLOT *)SlotB;

  if (Slot1->Address < Slot2->Address) {
    return -1;
  }

  if (Slot1->Address > Slot2->Address) {
    return 1;
  }

  return 0;
}

/**
  Compare cache slot address with a key address.

  Used by red-black tree for address lookups.

  @param[in] Context  Unused context pointer.
  @param[in] SlotPtr  Slot to compare.
  @param[in] Key      Key address to compare.

  @return -1 if slot address < key, 0 if equal, 1 if slot address > key.
**/
static
INT32
CacheSlotKeyCompare (
  IN VOID        *Context,
  IN CONST VOID  *SlotPtr,
  IN CONST VOID  *Key
  )
{
  CONST SLOT  *Slot;
  uintptr_t   KeyAddress;

  Slot       = (CONST SLOT *)SlotPtr;
  KeyAddress = (uintptr_t)Key;

  if (Slot->Address < KeyAddress) {
    return -1;
  }

  if (Slot->Address > KeyAddress) {
    return 1;
  }

  return 0;
}

//
// Red-Black Tree Operations
//

static const rb_tree_ops_t CacheOps = {
  CacheSlotCompare,
  CacheSlotKeyCompare,
  0,
  NULL
};

//
// Cache Helper Functions
//

/**
  Get slot number from slot pointer.

  Calculates the slot index from the slot pointer relative to the
  slot array base.

  @param[in] pCache  Pointer to the cache structure.
  @param[in] pSlot   Pointer to the slot.

  @return Slot number (0 to NumSlots-1).
**/
static inline
UINTN
CacheGetSlotNumber (
  IN CACHE *Cache,
  IN SLOT *Slot
  )
{
  return ((uintptr_t)Slot - (uintptr_t)(Cache->Slots)) / sizeof (SLOT);
}

/**
  Initialize a cache.

  Sets up the cache structure with the specified number of slots and
  fill callback function.

  @param[out] pCache    Pointer to cache structure to initialize.
  @param[out] pSlots    Array of cache slots.
  @param[in]  NumSlots  Number of slots in the array.
  @param[in]  FillFunc  Fill callback function for cache misses.
**/
static inline
VOID
CacheInitialize (
  OUT CACHE *Cache,
  OUT SLOT *Slots,
  IN  UINTN         NumSlots,
  IN  VOID          (*FillFunc)(UINTN, uintptr_t, uintptr_t)
  )
{
  UINTN  i;

  spinlock_init (&Cache->Lock);
  rb_tree_init (&Cache->Map, &CacheOps);
  TAILQ_INIT (&Cache->FreeList);

  Cache->NumSlots = NumSlots;
  Cache->Slots    = pSlots;
  Cache->Fill     = FillFunc;

  //
  // Initialize all slots to free state
  //
  for (i = 0; i < NumSlots; i++) {
    Slots[i].Valid    = 0;
    Slots[i].RefCount = 0;
    TAILQ_INSERT_TAIL (&Cache->FreeList, Slots + i, LruEntry);
  }
}

/**
  Get a cache slot for the specified address.

  Looks up the address in the cache. If found, increments the reference
  count. If not found, evicts an LRU entry and calls the fill callback.

  @param[in,out] pCache  Pointer to the cache structure.
  @param[in]     Address Address to cache.

  @return Slot number, or (UINTN)-1 if cache is full and all slots are in use.
**/
static inline
UINTN
CacheGet (
  IN OUT CACHE *Cache,
  IN     uintptr_t     Address
  )
{
  SLOT *Slot;
  UINTN        SlotNumber;

  spinlock (&Cache->Lock);

  //
  // Look up address in red-black tree
  //
  Slot = (SLOT *)rb_tree_find_node (&Cache->Map, (CONST VOID *)Address);

  if (Slot != NULL) {
    assert (Slot->Valid);

    if (Slot->RefCount > 0) {
      //
      // Already present and in use - increment reference count
      //
      Slot->RefCount++;
      assert (Slot->RefCount != 0);
      goto Exit;
    } else {
      //
      // Currently cached but unused - remove from free list and increment ref
      //
      TAILQ_REMOVE (&Cache->FreeList, Slot, LruEntry);
      Slot->RefCount = 1;
      goto Exit;
    }
  }

  //
  // Not present in cache - evict from free list
  //
  Slot = TAILQ_FIRST (&Cache->FreeList);

  if (Slot != NULL) {
    UINTN  SlotNo;

    SlotNo = CacheGetSlotNumber (Cache, Slot);

    TAILQ_REMOVE (&Cache->FreeList, Slot, LruEntry);
    assert (Slot->RefCount == 0);

    //
    // Call fill callback to populate the slot
    //
    Cache->Fill (SlotNo, Slot->Address, Address);

    Slot->Address  = Address;
    Slot->RefCount = 1;
    Slot->Valid    = 1;

    rb_tree_insert_node (&Cache->Map, Slot);
    goto Exit;
  }

Exit:
  if (Slot != NULL) {
    SlotNumber = CacheGetSlotNumber (Cache, Slot);
  } else {
    SlotNumber = (UINTN)-1;
  }

  spinunlock (&Cache->Lock);

  return SlotNumber;
}

/**
  Release a cache slot.

  Decrements the reference count on the slot. When the count reaches zero,
  the slot is added to the LRU free list for potential eviction.

  @param[in,out] pCache     Pointer to the cache structure.
  @param[in]     SlotNumber Slot number to release.
**/
static
VOID
CachePut (
  IN OUT CACHE *Cache,
  IN     UINTN         SlotNumber
  )
{
  SLOT *Slot;

  spinlock (&Cache->Lock);

  assert (SlotNumber < Cache->NumSlots);
  Slot = Cache->Slots + SlotNumber;

  assert (Slot->RefCount > 0);
  Slot->RefCount--;

  if (Slot->RefCount == 0) {
    //
    // No more references - add to LRU free list for potential eviction
    //
    TAILQ_INSERT_TAIL (&Cache->FreeList, Slot, LruEntry);
  }

  spinunlock (&Cache->Lock);
}

//
// Legacy Function Aliases (for backward compatibility)
//

/** @deprecated Use CacheGetSlotNumber instead **/
static inline unsigned cache_getslotno (CACHE *c, SLOT *s) {
  return CacheGetSlotNumber (c, s);
}

/** @deprecated Use CacheInitialize instead **/
static inline void cache_init (
  CACHE *c,
  SLOT *slots,
  unsigned numslots,
  void (*fill)(unsigned, uintptr_t, uintptr_t)
) {
  CacheInitialize (c, slots, numslots, fill);
}

/** @deprecated Use CacheGet instead **/
static inline unsigned cache_get (CACHE *c, uintptr_t addr) {
  return CacheGet (c, addr);
}

/** @deprecated Use CachePut instead **/
static inline void cache_put (CACHE *c, uintptr_t slotno) {
  CachePut (c, slotno);
}

/** @deprecated Internal function, use CacheSlotCompare instead **/
static inline int _slotcmp (void *ctx, const void *a, const void *b) {
  return CacheSlotCompare (ctx, a, b);
}

/** @deprecated Internal function, use CacheSlotKeyCompare instead **/
static inline int _slot_keycmp (void *ctx, const void *a, const void *b) {
  return CacheSlotKeyCompare (ctx, a, b);
}

#endif // _CACHE_H
