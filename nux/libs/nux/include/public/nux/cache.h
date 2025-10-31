/** @file
  NUX Generic Cache Implementation

  Provides a simple, efficient cache implementation using NTRTL AVL trees
  for fast lookups and LRU (Least Recently Used) eviction policy.

  This is a template-style implementation that requires the user to define
  a fill callback function for cache misses.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __nux_cache_h__
#define __nux_cache_h__

#include <assert.h>
#include <ananke/ntrtl.h>
#include <nux/types.h>
#include <nux/locks.h>

//
// Cache Slot Structure
//

/**
  Cache Slot

  Represents a single cache entry with LRU tracking and reference counting.
  Uses NTRTL AVL tree node for fast lookups by address.
**/
typedef struct _SLOT
{
  RTL_AVL_TREE_NODE   AvlNode;    ///< NTRTL AVL tree node (must be first)
  LIST_ENTRY          LruEntry;   ///< LRU queue entry

  UINTN  Address;                 ///< Cached address
  struct {
    UINT32  Valid:1;              ///< Entry is valid
    UINT32  RefCount:31;          ///< Reference count
  };
} SLOT, *PSLOT, *PCSLOT;

//
// Cache Structure
//

/**
  Cache

  Main cache structure managing a fixed number of slots with LRU eviction.
  Uses NTRTL AVL tree for O(log n) lookups and LIST_ENTRY for LRU ordering.
**/
typedef struct _CACHE
{
  RTL_AVL_TREE        Map;        ///< NTRTL AVL tree for address lookup (must be first)
  LIST_ENTRY          FreeList;   ///< LRU free list head
  SPINLOCK            Lock;       ///< Protects cache structure

  /**
    Fill callback function.

    Called when a cache miss occurs to populate the slot with new data.

    @param[in] SlotNumber  Slot number being filled (0 to NumSlots-1).
    @param[in] OldAddress  Address of entry being evicted.
    @param[in] NewAddress  Address of entry being loaded.
  **/
  VOID (*Fill)(UINTN SlotNumber, UINTN OldAddress, UINTN NewAddress);

  UINTN  NumSlots;                ///< Total number of cache slots
  SLOT   *Slots;                  ///< Array of cache slots
} CACHE, *PCACHE, *PCCACHE;

//
// Internal Comparison Functions
//

/**
  Compare two cache slots by address.

  Used by NTRTL AVL tree for slot ordering.

  @param[in] Node1    First AVL tree node to compare.
  @param[in] Node2    Second AVL tree node to compare.
  @param[in] Context  Cache structure pointer.

  @return -1 if A < B, 0 if A == B, 1 if A > B.
**/
static
INTN
ANXAPI
CacheSlotCompare (
  IN PRTL_AVL_TREE_NODE  Node1,
  IN PRTL_AVL_TREE_NODE  Node2,
  IN VOID                *Context
  )
{
  PCSLOT Slot1;
  PCSLOT Slot2;

  Slot1 = CONTAINING_RECORD(Node1, SLOT, AvlNode);
  Slot2 = CONTAINING_RECORD(Node2, SLOT, AvlNode);

  if (Slot1->Address < Slot2->Address) {
    return -1;
  }

  if (Slot1->Address > Slot2->Address) {
    return 1;
  }

  return 0;
}

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
static INLINE
UINTN
CacheGetSlotNumber (
  IN CACHE *Cache,
  IN SLOT *Slot
  )
{
  return ((UINTN)Slot - (UINTN)(Cache->Slots)) / sizeof (SLOT);
}

/**
  Find a slot in the AVL tree by address.

  @param[in] Cache    Cache structure.
  @param[in] Address  Address to search for.

  @return Pointer to slot if found, NULL otherwise.
**/
static INLINE
SLOT *
CacheFindSlot (
  IN CACHE *Cache,
  IN UINTN Address
  )
{
  PRTL_AVL_TREE_NODE Current = Cache->Map.Root;

  while (Current != NULL) {
    SLOT *Slot = CONTAINING_RECORD(Current, SLOT, AvlNode);

    if (Address < Slot->Address) {
      Current = Current->Left;
    } else if (Address > Slot->Address) {
      Current = Current->Right;
    } else {
      return Slot;
    }
  }

  return NULL;
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
static INLINE
VOID
CacheInitialize (
  OUT CACHE *Cache,
  OUT SLOT *Slots,
  IN  UINTN         NumSlots,
  IN  VOID          (*FillFunc)(UINTN, UINTN, UINTN)
  )
{
  UINTN  i;

  SpinLockInitialize (&Cache->Lock);
  RtlInitializeAvlTree (&Cache->Map, CacheSlotCompare, NULL, NULL, (VOID *)Cache);
  InitializeListHead (&Cache->FreeList);

  Cache->NumSlots = NumSlots;
  Cache->Slots    = Slots;
  Cache->Fill     = FillFunc;

  //
  // Initialize all slots to free state
  //
  for (i = 0; i < NumSlots; i++) {
    Slots[i].Valid    = 0;
    Slots[i].RefCount = 0;
    RtlInitializeAvlTreeNode (&Slots[i].AvlNode);
    InsertTailList (&Cache->FreeList, &Slots[i].LruEntry);
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
static INLINE
UINTN
CacheGet (
  IN OUT CACHE *Cache,
  IN     UINTN     Address
  )
{
  SLOT *Slot;
  UINTN        SlotNumber;

  SpinLockAcquire (&Cache->Lock);

  //
  // Look up address in NTRTL AVL tree
  //
  Slot = CacheFindSlot (Cache, Address);

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
      RemoveEntryList (&Slot->LruEntry);
      Slot->RefCount = 1;
      goto Exit;
    }
  }

  //
  // Not present in cache - evict from free list
  //
  if (!IsListEmpty (&Cache->FreeList)) {
    PLIST_ENTRY Entry = Cache->FreeList.Flink;
    Slot = CONTAINING_RECORD(Entry, SLOT, LruEntry);

    UINTN  SlotNo = CacheGetSlotNumber (Cache, Slot);

    RemoveEntryList (&Slot->LruEntry);
    assert (Slot->RefCount == 0);

    //
    // Remove old entry from AVL tree if it was valid
    //
    if (Slot->Valid) {
      RtlRemoveAvlTreeNode (&Cache->Map, &Slot->AvlNode, TRUE);
    }

    //
    // Call fill callback to populate the slot
    //
    Cache->Fill (SlotNo, Slot->Address, Address);

    Slot->Address  = Address;
    Slot->RefCount = 1;
    Slot->Valid    = 1;

    RtlInitializeAvlTreeNode (&Slot->AvlNode);
    RtlInsertAvlTreeNode (&Cache->Map, &Slot->AvlNode, TRUE);
  } else {
    Slot = NULL;
  }

Exit:
  if (Slot != NULL) {
    SlotNumber = CacheGetSlotNumber (Cache, Slot);
  } else {
    SlotNumber = (UINTN)-1;
  }

  SpinLockRelease (&Cache->Lock);

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

  SpinLockAcquire (&Cache->Lock);

  assert (SlotNumber < Cache->NumSlots);
  Slot = Cache->Slots + SlotNumber;

  assert (Slot->RefCount > 0);
  Slot->RefCount--;

  if (Slot->RefCount == 0) {
    //
    // No more references - add to LRU free list for potential eviction
    //
    InsertTailList (&Cache->FreeList, &Slot->LruEntry);
  }

  SpinLockRelease (&Cache->Lock);
}

#endif // __nux_cache_h__
