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
#include "internal.h"

/*
  Don't take this too seriously. SPINLOCK usage is awful.
*/

extern int _pfncache_start;
extern int _pfncache_end;
const void *pfncache_base = (const void *)&_pfncache_start;

#define CACHE_SLOTS 256 /* 1MB page cache */

static void
___cache_fill (uintptr_t slot, uintptr_t old, uintptr_t new)
{
  vaddr_t va = (vaddr_t)pfncache_base + (slot << PAGE_SHIFT);

  assert (slot < CACHE_SLOTS);

  kmap_map (va, new, HAL_PTE_P|HAL_PTE_W);
  kmap_commit ();
}

#include <nux/cache.h>

void *
hal_physmem_getpfn (pfn_t pfn)
{
  uintptr_t slot;

  slot = cache_get (pfn);
  return pfncache_base + (slot << PAGE_SHIFT);
}

void
hal_physmem_putpfn (pfn_t pfn, void *va)
{
  uintptr_t slot;

  slot = ((uintptr_t)va - (uintptr_t)pfncache_base) >> PAGE_SHIFT;
  cache_put ((uintptr_t)slot);
}


void
pfncache_init (void)
{
  uintptr_t pfncache_size = (uintptr_t)&_pfncache_end - (uintptr_t)pfncache_base;
  assert (pfncache_size == PAGE_SIZE * CACHE_SLOTS);
  cache_init ();
}
