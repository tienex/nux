/*
 * NUX: A kernel Library. Copyright (C) 2019 Gianluca Guida,
 * glguida@tlbflush.org
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * See COPYING file for the full license.
 * 
 * SPDX-License-Identifier:	GPL2.0+
 */

#include <assert.h>
#include <nux/hal.h>
#include <nux/nux.h>

/*
 * Low level routines to handle kernel mappings.
 * 
 * Unlocked, dangerous, use with care and consideration.
 */

static volatile tlbgen_t _tlbgen, _tlbgen_global;

void
kmapinit (void)
{
}

static void
_kmap_dirty (hal_tlbop_t op)
{
  switch (op)
    {
    case HAL_TLBOP_FLUSHALL:
      (void) __sync_fetch_and_add (&_tlbgen_global, 1);
      break;
    case HAL_TLBOP_FLUSH:
      (void) __sync_fetch_and_add (&_tlbgen, 1);
      break;
    default:
      break;
    }
}

static pfn_t
_kmap_map (vaddr_t va, pfn_t pfn, unsigned prot, const int alloc)
{
  hal_l1p_t l1p;
  hal_l1e_t l1e, oldl1e;
  pfn_t oldpfn;
  unsigned oldprot;

  l1e = hal_pmap_boxl1e (pfn, prot);

  assert (hal_pmap_getl1p (NULL, va, alloc, &l1p));
  oldl1e = hal_pmap_setl1e (NULL, l1p, l1e);
  _kmap_dirty (hal_pmap_tlbop (oldl1e, l1e));

  hal_pmap_unboxl1e (oldl1e, &oldpfn, &oldprot);

  return oldprot & HAL_PTE_P ? oldpfn : PFN_INVALID;
}

pfn_t
kmap_map (vaddr_t va, pfn_t pfn, unsigned prot)
{
  return _kmap_map (va, pfn, prot, 1);
}

/*
 * Only map a VA if no pagetable allocations are needed.
 */
pfn_t
kmap_map_noalloc (vaddr_t va, pfn_t pfn, unsigned prot)
{
  return _kmap_map (va, pfn, prot, 0);
}

/*
 * Check if va is mapped.
 */
int
kmap_mapped (vaddr_t va)
{

  return hal_pmap_getl1p (NULL, va, 0, NULL);
}

int
kmap_mapped_range (vaddr_t va, size_t size)
{
  vaddr_t i, s, e;

  s = trunc_page (va);
  e = va + size;

  for (i = s; i < e; i += PAGE_SIZE)
    if (!kmap_mapped (i))
      return 0;

  return 1;
}

int
kmap_ensure (vaddr_t va, unsigned reqprot)
{
  int ret = -1;
  hal_l1p_t l1p = L1P_INVALID;
  hal_l1e_t oldl1e, l1e;
  pfn_t pfn;
  unsigned prot;

  if (hal_pmap_getl1p (NULL, va, 0, &l1p))
    {
      l1e = hal_pmap_getl1e (NULL, l1p);
      hal_pmap_unboxl1e (l1e, &pfn, &prot);
    }
  else
    {
      pfn = PFN_INVALID;
      prot = 0;
    }

  if (!(reqprot ^ prot))
    {
      /* same, exit */
      ret = 0;
      goto out;
    }
  /*
   * Check present bit. If we are adding a P bit allocate, if we are
   * removing it free the page.
   */
  if ((reqprot & HAL_PTE_P) != (prot & HAL_PTE_P))
    {
      if (reqprot & HAL_PTE_P)
	{
	  /* Ensure pagetable populated. */
	  if (l1p == L1P_INVALID)
	    assert (hal_pmap_getl1p (NULL, va, 1, &l1p));
	  /* Populate page. */
	  pfn = pfn_alloc (0);
	  if (pfn == PFN_INVALID)
	    goto out;
	}
      else
	{
	  /* Freeing page. */
	  pfn_free (pfn);
	  pfn = PFN_INVALID;
	}
    }
  l1e = hal_pmap_boxl1e (pfn, reqprot);
  oldl1e = hal_pmap_setl1e (NULL, l1p, l1e);
  _kmap_dirty (hal_pmap_tlbop (oldl1e, l1e));
  ret = 0;

out:
  return ret;
}

int
kmap_ensure_range (vaddr_t va, size_t size, unsigned reqprot)
{
  vaddr_t i, s, e;

  s = trunc_page (va);
  e = va + size;

  for (i = s; i < e; i += PAGE_SIZE)
    if (kmap_ensure (i, reqprot))
      return -1;

  return 0;
}

volatile tlbgen_t
kmap_tlbgen (void)
{
  /* smp barrier */
  return _tlbgen;
}

volatile tlbgen_t
kmap_tlbgen_global (void)
{
  /* smp barrier */
  return _tlbgen_global;
}

void
kmap_commit (void)
{
  /*
   * This is extremely slow, but guarantees KMAP to be aligned in all
   * CPUs.
   */
  //cpu_tlbflush_broadcast(TLBOP_KMAPUPDATE, true);
}
