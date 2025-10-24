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

vaddr_t gPfnCacheBase;

static pfn_t gMaxDmapPfn;

static struct cache gCache;
static struct slot *gSlots;

static volatile tlbgen_t gPfncTlbGen = 0;

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
  tlbgen_t TlbGen;
  vaddr_t Va = (vaddr_t) gPfnCacheBase + ((vaddr_t) Slot << PAGE_SHIFT);

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
  __atomic_store (&gPfncTlbGen, &TlbGen, __ATOMIC_RELEASE);
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
  IN pfn_t  Pfn
  )
{
  UINTN Slot;
  tlbgen_t Target;

  assert (Pfn != PFN_INVALID);

  if (Pfn < gMaxDmapPfn)
    return (VOID *) (hal_virtmem_dmapbase () + (Pfn << PAGE_SHIFT));

  Slot = cache_get (&gCache, Pfn);

  /* Update tlb if we have stale entries in our PFN cache. */
  __atomic_load (&gPfncTlbGen, &Target, __ATOMIC_ACQUIRE);
  CpuKernelTlbReach (Target);
  return (VOID *) gPfnCacheBase + (Slot << PAGE_SHIFT);
}

/**
  Release page frame from cache.

  Returns a cache slot back to the pool. No action needed for
  PFNs in the direct map region.

  @param[in] Pfn  Page frame number being released.
  @param[in] pVa  Virtual address that was returned by PfnGet.
**/
VOID
PfnPut (
  IN pfn_t  Pfn,
  IN VOID   *pVa
  )
{
  UINTN Slot;

  assert (Pfn != PFN_INVALID);

  if (Pfn < gMaxDmapPfn)
    return;

  Slot = ((UINTN) pVa - (UINTN) gPfnCacheBase) >> PAGE_SHIFT;
  cache_put (&gCache, (UINTN) Slot);
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

  gSlots = (struct slot *) KmemBrkGrow (1, sizeof (struct slot) * NumSlots);

  cache_init (&gCache, gSlots, 256, PfnCacheFill);
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
static struct slot gBootSlot;

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
  cache_init (&gCache, &gBootSlot, 1, PfnCacheFill);
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use PfnCacheFill instead **/
static void _pfncache_fill (unsigned slot, uintptr_t old, uintptr_t new) {
  PfnCacheFill (slot, old, new);
}

/** @deprecated Use PfnGet instead **/
void *pfn_get (pfn_t pfn) {
  return PfnGet (pfn);
}

/** @deprecated Use PfnPut instead **/
void pfn_put (pfn_t pfn, void *va) {
  PfnPut (pfn, va);
}

/** @deprecated Use PfnCacheInitialize instead **/
void pfncacheinit (void) {
  PfnCacheInitialize ();
}

/** @deprecated Use PfnCacheBootstrap instead **/
void _pfncache_bootstrap (void) {
  PfnCacheBootstrap ();
}

// Legacy global variable aliases
vaddr_t pfncache_base __attribute__((alias("gPfnCacheBase")));
static pfn_t max_dmap_pfn __attribute__((alias("gMaxDmapPfn")));
static struct cache cache __attribute__((alias("gCache")));
static struct slot *slots __attribute__((alias("gSlots")));
static volatile tlbgen_t pfnc_tlbgen __attribute__((alias("gPfncTlbGen")));
static struct slot boot_slot __attribute__((alias("gBootSlot")));
