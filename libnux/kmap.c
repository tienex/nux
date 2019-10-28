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

#include <nux/hal.h>
#include <nux/nux.h>

/*
  Low level routines to handle kernel mappings.

  Unlocked, dangerous, use with care and consideration.
*/

hal_tlbop_t kmap_tlbop;

void
kmapinit (void)
{
}

void
kmap_map (vaddr_t va, pfn_t pfn, unsigned prot)
{
  hal_l1e_t l1e = hal_pmap_boxl1e (pfn, prot);
  hal_l1e_t oldl1e = hal_pmap_setl1e (NULL, va, l1e);
  __sync_or_and_fetch (&kmap_tlbop, hal_pmap_tlbop (oldl1e, l1e));
}


/*
  Check if va is mapped.
*/
int
kmap_mapped (vaddr_t va)
{
  hal_l1e_t l1e;
  unsigned prot;

  l1e = hal_pmap_getl1e (NULL, va);
  hal_pmap_unboxl1e (l1e, NULL, &prot);
  return !!(prot & HAL_PTE_P);
}

int
kmap_mapped_range (vaddr_t va, size_t size)
{
  vaddr_t i, s, e;

  s = trunc_page (va);
  e = va + size;

  for (i = s; i < e; i+= PAGE_SIZE)
    if (!kmap_mapped (i))
      return 0;

  return 1;
}

int
kmap_ensure (vaddr_t va, unsigned reqprot)
{
  int ret = -1;
  hal_l1e_t oldl1e, l1e;
  pfn_t pfn;
  unsigned prot;

  l1e = hal_pmap_getl1e (NULL, va);
  hal_pmap_unboxl1e (l1e, &pfn, &prot);

  if (!(reqprot ^ prot))
    {
      /* same, exit */
      ret = 0;
      goto out;
    }

  /* Check present bit. If we are adding a P bit allocate, if we are
     removing it free the page. */
  if ((reqprot & HAL_PTE_P) !=  (prot & HAL_PTE_P))
    {
      if (reqprot & HAL_PTE_P)
	{
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
  oldl1e = hal_pmap_setl1e (NULL, va, l1e);
  __sync_or_and_fetch (&kmap_tlbop, hal_pmap_tlbop (oldl1e, l1e));
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

  for (i = s; i < e; i+= PAGE_SIZE)
    if (kmap_ensure (i, reqprot))
      return -1;

  return 0;
}

void
kmap_commit (void)
{
  hal_tlbop_t tlbop;

  tlbop = __sync_fetch_and_and (&kmap_tlbop, 0);
  hal_cpu_tlbop (tlbop);
}
