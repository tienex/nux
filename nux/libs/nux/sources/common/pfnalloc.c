/** @file
  NUX Page Frame Number Allocator

  Provides physical page frame allocation using a binary search tree
  (stree) for efficient free page tracking. Supports both low and high
  memory allocation, custom allocator registration, and atomic
  allocation/deallocation operations.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <nux/internal.h>
#include <string.h>
#include <hal/hal.h>
#include <nux/locks.h>
#include <nux/types.h>
#include <nux/nux.h>
#include <stree.h>
#include <assert.h>

static lock_t gPgLock;
static WORD_T *gStree;
static UINT32 gOrder;
static UINTN gFreePages;

rwlock_t gNuxPfnAllocLock;
pfn_t (*gNuxPfnAlloc) (INT32) = &StreePfnAllocate;
VOID (*gNuxPfnFree) (pfn_t) = &StreePfnFree;

/**
  Initialize page frame allocator from HAL.

  Obtains physical memory tree from HAL and initializes the page
  frame allocator. Prints memory availability statistics.
**/
VOID
StreePfnInitialize (
  VOID
  )
{
  INTN First, Last;

  gStree = hal_physmem_stree (&gOrder);
  assert (gStree);

  First = stree_bitsearch (gStree, gOrder, 1);
  Last = stree_bitsearch (gStree, gOrder, 0);
  gFreePages = stree_count (gStree, gOrder);
  assert (First >= 0);
  assert (Last >= 0);
  printf ("Lowest physical page free:  %08lx.\n", First);
  printf ("Highest physical page free: %08lx.\n", Last);
  printf ("Memory available: %ld Kb.\n", gFreePages * PAGE_SIZE / 1024);

  spinlock_init (&gPgLock);
}

/**
  Allocate page frame from stree.

  Searches the binary tree for a free page frame, clears it,
  and returns the page frame number. The allocated page is
  zeroed before being returned.

  @param[in] Low  Non-zero to search from low memory, zero for high memory.

  @return Page frame number, or PFN_INVALID if no pages available.
**/
pfn_t
StreePfnAllocate (
  IN INT32  Low
  )
{
  INTN Pg;
  VOID *Va;

  spinlock (&gPgLock);
  Pg = stree_bitsearch (gStree, gOrder, Low);
  if (Pg >= 0)
    {
      assert (gFreePages != 0);
      gFreePages--;
      stree_clrbit (gStree, gOrder, Pg);
    }
  spinunlock (&gPgLock);

  if (Pg < 0)
    return PFN_INVALID;

  Va = pfn_get (Pg);
  memset (Va, 0, PAGE_SIZE);
  pfn_put (Pg, Va);

  return (pfn_t) Pg;
}

/**
  Free page frame back to stree.

  Returns a page frame to the free pool by setting its bit
  in the stree bitmap.

  @param[in] Pfn  Page frame number to free.
**/
VOID
StreePfnFree (
  IN pfn_t  Pfn
  )
{
  assert (Pfn != PFN_INVALID);
  assert (Pfn < hal_physmem_maxpfn ());
  spinlock (&gPgLock);
  stree_setbit (gStree, gOrder, Pfn);
  spinunlock (&gPgLock);
}

/**
  Set custom page frame allocator.

  Allows registration of custom allocator and free functions
  to replace the default stree-based allocator. Protected by
  write lock to ensure atomic replacement.

  @param[in] Alloc  Pointer to custom allocation function.
  @param[in] Free   Pointer to custom free function.
**/
VOID
NuxSetAllocator (
  IN pfn_t (*Alloc) (INT32),
  IN VOID (*Free) (pfn_t)
  )
{
  writelock (&gNuxPfnAllocLock);
  gNuxPfnAlloc = Alloc;
  gNuxPfnFree = Free;
  writeunlock (&gNuxPfnAllocLock);
}

/**
  Allocate page frame.

  Public API for page frame allocation. Uses currently registered
  allocator function (default: StreePfnAllocate).

  @param[in] Low  Non-zero to search from low memory, zero for high memory.

  @return Page frame number, or PFN_INVALID if no pages available.
**/
pfn_t
PfnAllocate (
  IN INT32  Low
  )
{
  pfn_t Pfn;

  readlock (&gNuxPfnAllocLock);
  Pfn = gNuxPfnAlloc (Low);
  readunlock (&gNuxPfnAllocLock);

  return Pfn;
}

/**
  Free page frame.

  Public API for page frame deallocation. Uses currently registered
  free function (default: StreePfnFree).

  @param[in] Pfn  Page frame number to free.
**/
VOID
PfnFree (
  IN pfn_t  Pfn
  )
{
  readlock (&gNuxPfnAllocLock);
  gNuxPfnFree (Pfn);
  gFreePages++;
  readunlock (&gNuxPfnAllocLock);
}

/**
  Get available page count.

  Returns the current number of free pages available for allocation.

  @return Number of free pages available.
**/
UINTN
PfnAvailable (
  VOID
  )
{
  return gFreePages;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use StreePfnInitialize instead **/
void stree_pfninit (void) {
  StreePfnInitialize ();
}

/** @deprecated Use StreePfnAllocate instead **/
pfn_t stree_pfnalloc (int low) {
  return StreePfnAllocate (low);
}

/** @deprecated Use StreePfnFree instead **/
void stree_pfnfree (pfn_t pfn) {
  StreePfnFree (pfn);
}

/** @deprecated Use NuxSetAllocator instead **/
void nux_set_allocator (pfn_t (*alloc) (int), void (*free) (pfn_t)) {
  NuxSetAllocator (alloc, free);
}

/** @deprecated Use PfnAllocate instead **/
pfn_t pfn_alloc (int low) {
  return PfnAllocate (low);
}

/** @deprecated Use PfnFree instead **/
void pfn_free (pfn_t pfn) {
  PfnFree (pfn);
}

/** @deprecated Use PfnAvailable instead **/
unsigned long pfn_avail (void) {
  return PfnAvailable ();
}

// Legacy global variable aliases
static lock_t pglock __attribute__((alias("gPgLock")));
static WORD_T *stree __attribute__((alias("gStree")));
static unsigned order __attribute__((alias("gOrder")));
static unsigned long free_pages __attribute__((alias("gFreePages")));
rwlock_t _nux_pfnalloc_lock __attribute__((alias("gNuxPfnAllocLock")));
pfn_t (*_nux_pfnalloc) (int) __attribute__((alias("gNuxPfnAlloc")));
void (*_nux_pfnfree) (pfn_t) __attribute__((alias("gNuxPfnFree")));
