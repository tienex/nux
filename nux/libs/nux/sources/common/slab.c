/** @file
  NUX Slab Allocator

  Provides kernel slab allocator for efficient fixed-size object
  allocation. Uses generic slabinc.c implementation with kernel-
  specific memory management integration.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <nux/nux.h>
#include <nux/slab.h>

#define SLAB_LOG2SZ 1		/* Must be power of 2 for alignment */
#define SLAB_SIZE ((1L << SLAB_LOG2SZ) * PAGE_SIZE)

#define SLABMAGIC 0x80763141
#define SLABFUNC_NAME "slab cache"
#define SLABFUNC(_s) Slab##_s
#define SLABPRINT(...) info(__VA_ARGS__)
#define SLABFATAL(...) fatal(__VA_ARGS__)

/**
  Get slab size in bytes.

  @return Slab size (SLAB_SIZE).
**/
static CONST UINTN
SlabSize (
  VOID
  )
{
  return SLAB_SIZE;
}

/**
  Calculate number of objects per slab.

  @param[in] ObjectSize  Size of each object in bytes.

  @return Number of objects that fit in a slab.
**/
static CONST UINTN
SlabObjectCount (
  IN CONST UINTN  ObjectSize
  )
{
  return (SlabSize () - 2 * sizeof (struct slabhdr)) / ObjectSize;
}

/**
  Allocate aligned slab.

  Allocates virtual address space for a new slab, ensuring
  SLAB_SIZE alignment. Maps physical pages for the slab.

  @param[out] ppObjectHeader  Pointer to receive first object header.

  @return Pointer to slab header, or NULL on failure.
**/
static struct slabhdr *
SlabAllocate (
  OUT struct objhdr  **ppObjectHeader
  )
{
  VIRTUAL_ADDRESS KvaUnaligned, KvaUnalignedEnd, KvaStart, KvaEnd;

  KvaUnaligned = KvaAllocate (2 * SLAB_SIZE - PAGE_SIZE);
  KvaUnalignedEnd = KvaUnaligned + 2 * SLAB_SIZE - PAGE_SIZE;

  if (KvaUnaligned % SLAB_SIZE)
    KvaStart = (KvaUnaligned + SLAB_SIZE - 1) & ~((VIRTUAL_ADDRESS) SLAB_SIZE - 1);
  else
    KvaStart = KvaUnaligned;
  KvaEnd = KvaStart + SLAB_SIZE;

  if (KvaUnaligned < KvaStart)
    {
      KvaFree (KvaUnaligned, KvaStart - KvaUnaligned);
    }

  if (KvaEnd < KvaUnalignedEnd)
    {
      KvaFree (KvaEnd, KvaUnalignedEnd - KvaEnd);
    }

  assert (!KmapEnsureRange (KvaStart, SLAB_SIZE, HAL_PTE_W | HAL_PTE_P));
  KmapCommit ();

  *ppObjectHeader = (struct objhdr *) (KvaStart + sizeof (struct slabhdr));
  return (struct slabhdr *) KvaStart;
}

/**
  Get slab header from object pointer.

  Finds the slab header for a given object by masking the
  address to the slab boundary.

  @param[in] Object  Pointer to object.

  @return Pointer to slab header, or NULL if magic check fails.
**/
static struct slabhdr *
SlabGetHeader (
  IN VOID  *Object
  )
{
  struct slabhdr *SlabHeader;
  UINTN Addr = (UINTN) Object;

  SlabHeader = (struct slabhdr *) (Addr & ~((UINTN) SlabSize () - 1));
  if (SlabHeader->magic != SLABMAGIC)
    return NULL;
  return SlabHeader;
}

/**
  Free slab.

  Unmaps and frees the virtual address space for a slab.

  @param[in] Ptr  Pointer to slab header.
**/
static VOID
SlabFreeInternal (
  IN VOID  *Ptr
  )
{
  KmapEnsureRange ((VIRTUAL_ADDRESS) Ptr, SLAB_SIZE, 0);
  KmapCommit ();
  KvaFree ((VIRTUAL_ADDRESS) Ptr, SLAB_SIZE);
}

//
// Define compatibility macros for slabinc.c
//

#define ___slabsize() SlabSize()
#define ___slabobjs(_sz) SlabObjectCount(_sz)
#define ___slaballoc(_ohptr) SlabAllocate(_ohptr)
#define ___slabgethdr(_obj) SlabGetHeader(_obj)
#define ___slabfree(_ptr) SlabFreeInternal(_ptr)

#define DECLARE_SPIN_LOCK(_x) lock_t _x
#define SPIN_LOCK_INIT(_x) spinlock_init(&_x)
#define SPIN_LOCK(_x) spinlock(&_x)
#define SPIN_UNLOCK(_x) spinunlock(&_x)
#define SPIN_LOCK_FREE(_x)

#include <nux/slabinc.c>

//
// Legacy Function Wrappers (for backward compatibility)
//

#define slab_grow SlabGrow
#define slab_shrink SlabShrink
#define slab_alloc_opq SlabAllocOpq
#define slab_free SlabFree
#define slab_register SlabRegister
#define slab_deregister SlabDeregister
#define slab_printstats SlabPrintStats
