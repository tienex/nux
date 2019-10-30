/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  See COPYING file for the full license.

  SPDX-License-Identifier:	GPL2.0+
*/

#include <assert.h>
#include <stdio.h>
#include <nux/nux.h>
#include <nux/cache.h>
#include "internal.h"

/*
  Don't take this too seriously. SPINLOCK usage is awful.
*/

extern int _pfncache_start;
extern int _pfncache_end;
const void *pfncache_base = (const void *)&_pfncache_start;

static pfn_t max_dmap_pfn;

static struct cache cache;
static struct slot *slots;


static void
_pfncache_fill (unsigned slot, uintptr_t old, uintptr_t new)
{
  pfn_t upfn;
  vaddr_t va = (vaddr_t)pfncache_base + ((vaddr_t)slot << PAGE_SHIFT);

  printf ("slot: %ld (%lx)", slot, slot);

  kmap_map (va, new, HAL_PTE_P|HAL_PTE_W);
  kmap_commit ();
}

void *
pfn_get (pfn_t pfn)
{
  uintptr_t slot;

  printf ("(MAX:%lx) (pfncache_base: %lx) pfn: %lx\n", max_dmap_pfn, pfncache_base, pfn);

  if (pfn < max_dmap_pfn)
    return (void *)(hal_virtmem_dmapbase () + (pfn << PAGE_SHIFT));

  slot = cache_get (&cache, pfn);
  return (void *)pfncache_base + (slot << PAGE_SHIFT);
}

void
pfn_put (pfn_t pfn, void *va)
{
  uintptr_t slot;

  printf ("(MAX:%lx) (pfncache_base: %lx) pfn: %lx, va: %p\n", max_dmap_pfn, pfncache_base, pfn, va);

  if (pfn < max_dmap_pfn)
    return;

  slot = ((uintptr_t)va - (uintptr_t)pfncache_base) >> PAGE_SHIFT;
  cache_put (&cache, (uintptr_t)slot);
}

void
pfncacheinit (void)
{
  uintptr_t pfncache_size = (uintptr_t)&_pfncache_end - (uintptr_t)pfncache_base;
  unsigned numslots = pfncache_size / PAGE_SIZE;

  printf ("PFN Cache from %p to %p (%u entries)\n",
	  &_pfncache_start, &_pfncache_end, numslots);
  assert (numslots != 0);

  slots = (struct slot *)kmem_brkgrow (1, sizeof (struct slot) * numslots);

  cache_init (&cache, slots, 256, _pfncache_fill);
}

/*
  We need a pfncache to enable pfnalloc, which in turns enable the
  kmem.

  Before the kmem starts, we can't reserve the kmem space needed to
  hold all the slots. We start with a single, static entry in the
  pfncache.

  Once kmem is setup _nux_init() will call pfncacheinit(), that will
  reserve the required amount of slots and start the real, full size
  cache.
*/
static struct slot boot_slot;

void
_pfncache_bootstrap (void)
{
  max_dmap_pfn = hal_virtmem_dmapsize () >> PAGE_SHIFT;

  printf ("Initializing PFN boot cache.");
  cache_init (&cache, &boot_slot, 1, _pfncache_fill);
}

