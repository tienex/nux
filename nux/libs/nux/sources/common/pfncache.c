/** @file
  NUX Page Frame Number Cache

  Provides efficient virtual address mapping for physical page frames
  outside the direct map region. Uses a cache with TLB generation
  tracking to minimize TLB flushes. Supports bootstrap mode for
  early initialization before full memory management is available.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <assert.h>
#include <stdio.h>
#include <nux/nux.h>
#include <nux/cache.h>
#include <nux/internal.h>

VIRTUAL_ADDRESS gPfnCacheBase;

static PFN gMaxDmapPfn;

static CACHE gCache;
static SLOT *gSlots;

static VOLATILE TLB_GENERATION gPfncTlbGen = 0;

/**
  Fill PFN cache slot with new mapping.

  Maps a page frame to the cache slot's virtual address without
  allocating page tables. Updates TLB generation counter to track
  when TLB flushes are needed.

  @param[in] Slot  Cache slot number.
  @param[in] Old   Previous PFN value (unused).
  @param[in] New   New PFN to map.
**/
static VOID
PfnCacheFill (
  IN UINT32    Slot,
  IN UINTN     Old,
  IN UINTN     New
  )
{
  TLB_GENERATION TlbGen;
  VIRTUAL_ADDRESS Va = (VIRTUAL_ADDRESS) gPfnCacheBase + ((VIRTUAL_ADDRESS) Slot << PAGE_SHIFT);

  /*
     NEVER allocate pagetables while mapping PFN Cache.

     The pagetables themselves are cleared, possibly using the PFN Cache,
     which would result in a deadlock.

     PFN Cache's pagetables must be allocated during boot.
   */
  KmapMapNoAlloc (Va, New, HAL_PTE_P | HAL_PTE_W);

  /*
     Save the new TLB generation target for pfn cache.
   */
  TlbGen = KtlbGenNormal ();
  ANX_ATOMIC_STORE (&gPfncTlbGen, &TlbGen, __ATOMIC_RELEASE);
}

/**
  Get virtual address for page frame number.

  Returns a virtual address that maps to the specified physical
  page frame. Uses direct map for low PFNs, cache for high PFNs.
  Ensures TLB is synchronized before returning cached addresses.

  @param[in] Pfn  Page frame number to map.

  @return Virtual address mapping to the page frame.
**/
VOID *
PfnGet (
  IN PFN  Pfn
  )
{
  UINTN Slot;
  TLB_GENERATION Target;

  assert (Pfn != PFN_INVALID);

  if (Pfn < gMaxDmapPfn)
    return (VOID *) (hal_virtmem_dmapbase () + (Pfn << PAGE_SHIFT));

  Slot = CacheGet (&gCache, Pfn);

  /* Update tlb if we have stale entries in our PFN cache. */
  ANX_ATOMIC_LOAD (&gPfncTlbGen, &Target, __ATOMIC_ACQUIRE);
  CpuKtlbReach (Target);
  return (VOID *) gPfnCacheBase + (Slot << PAGE_SHIFT);
}

/**
  Release page frame from cache.

  Returns a cache slot back to the pool. No action needed for
  PFNs in the direct map region.

  @param[in] Pfn  Page frame number being released.
  @param[in] Va   Virtual address that was returned by PfnGet.
**/
VOID
PfnPut (
  IN PFN   Pfn,
  IN VOID  *Va
  )
{
  UINTN Slot;

  assert (Pfn != PFN_INVALID);

  if (Pfn < gMaxDmapPfn)
    return;

  Slot = ((UINTN) Va - (UINTN) gPfnCacheBase) >> PAGE_SHIFT;
  CachePut (&gCache, (UINTN) Slot);
}

/**
  Initialize full PFN cache.

  Allocates slots from kernel memory and initializes the full-size
  cache. Called after kmem is ready, replacing bootstrap cache.
**/
VOID
PfnCacheInitialize (
  VOID
  )
{
  UINTN PfnCacheSize = hal_virtmem_pfn$size ();
  UINT32 NumSlots = PfnCacheSize / PAGE_SIZE;

  printf ("PFN Cache from %p to %p (%u entries)\n",
	  gPfnCacheBase, gPfnCacheBase + PfnCacheSize, NumSlots);
  assert (NumSlots != 0);

  gSlots = (SLOT *) KmemBrkGrow (1, sizeof (SLOT) * NumSlots);

  CacheInitialize (&gCache, gSlots, 256, PfnCacheFill);
}

/*
  We need a pfncache to enable pfnalloc, which in turns enable the
  kmem.

  Before the kmem starts, we can't reserve the kmem space needed to
  hold all the slots. We start with a single, static entry in the
  pfncache.

  Once kmem is setup _nux_init() will call PfnCacheInitialize(), that will
  reserve the required amount of slots and start the real, full size
  cache.
*/
static SLOT gBootSlot;

/**
  Bootstrap PFN cache with minimal resources.

  Initializes a minimal single-slot cache for early boot before
  full memory management is available. This allows basic PFN
  operations during system initialization.
**/
VOID
PfnCacheBootstrap (
  VOID
  )
{
  gMaxDmapPfn = hal_virtmem_dmapsize () >> PAGE_SHIFT;
  gPfnCacheBase = hal_virtmem_pfn$base ();

  printf ("Initializing PFN boot cache.\n");
  CacheInitialize (&gCache, &gBootSlot, 1, PfnCacheFill);
}
