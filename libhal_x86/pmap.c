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
#include <stdbool.h>
#include <nux/hal.h>
#include <nux/nux.h>
#include "internal.h"

uint64_t pte_nx = 0;

bool
hal_pmap_getl1p (struct hal_pmap *pmap, unsigned long va, bool alloc,
		 hal_l1p_t * l1popq)
{
  hal_l1p_t l1p = get_l1p (pmap, va, alloc);

  if (l1popq != NULL)
    *l1popq = l1p;

  return l1p != L1P_INVALID;
}

hal_l1e_t
hal_pmap_getl1e (struct hal_pmap *pmap, hal_l1p_t l1popq)
{
  return (hal_l1e_t) get_pte (l1popq);
}

hal_l1e_t
hal_pmap_setl1e (struct hal_pmap *pmap, hal_l1p_t l1popq, hal_l1e_t l1e)
{
  hal_l1e_t ol1e;

  ol1e = set_pte ((ptep_t) l1popq, (pte_t) l1e);
  return ol1e;
}

hal_l1e_t
hal_pmap_boxl1e (unsigned long pfn, unsigned prot)
{
  hal_l1e_t l1e;

  l1e = (uint64_t) pfn << HAL_PAGE_SHIFT;

  if (prot & HAL_PTE_P)
    l1e |= PTE_P;
  if (prot & HAL_PTE_W)
    l1e |= PTE_W;
  if (!(prot & HAL_PTE_X) && (prot & HAL_PTE_P))
    l1e |= pte_nx;
  if (prot & HAL_PTE_U)
    l1e |= PTE_U;
  if (prot & HAL_PTE_GLOBAL)
    l1e |= PTE_G;
  if (prot & HAL_PTE_AVL0)
    l1e |= PTE_AVAIL0;
  if (prot & HAL_PTE_AVL1)
    l1e |= PTE_AVAIL1;
  if (prot & HAL_PTE_AVL2)
    l1e |= PTE_AVAIL2;

  return l1e;
}

void
hal_pmap_unboxl1e (hal_l1e_t l1e, unsigned long *pfnp, unsigned *protp)
{
  unsigned prot = 0;

  if (l1e & PTE_P)
    prot |= HAL_PTE_P;
  if (l1e & PTE_W)
    prot |= HAL_PTE_W;
  if (!(l1e & PTE_NX) && (l1e & PTE_P))
    prot |= HAL_PTE_X;
  if (l1e & PTE_U)
    prot |= HAL_PTE_U;
  if (l1e & PTE_G)
    prot |= HAL_PTE_GLOBAL;
  if (l1e & PTE_AVAIL0)
    prot |= HAL_PTE_AVL0;
  if (l1e & PTE_AVAIL1)
    prot |= HAL_PTE_AVL1;
  if (l1e & PTE_AVAIL2)
    prot |= HAL_PTE_AVL2;

  if (pfnp)
    *pfnp = l1epfn (l1e);
  if (protp)
    *protp = prot;
}

unsigned
hal_pmap_tlbop (hal_l1e_t old, hal_l1e_t new)
{
#define restricts_permissions(_o, _n) 1

  /* Previous not present. Don't flush. */
  if (!(l1eflags (old) & PTE_P))
    return 0;

  /* Mapping a different page. Flush. */
  if ((l1epfn (old) != l1epfn (new)) || restricts_permissions (old, new))
    {
      if ((l1eflags (old) & PTE_G) || (l1eflags (new) & PTE_G))
	{
	  return HAL_TLBOP_FLUSHALL;
	}
      else
	{
	  return HAL_TLBOP_FLUSH;
	}
    }

  return HAL_TLBOP_NONE;
}

vaddr_t
hal_virtmem_userbase (void)
{
  return 0;
}

const size_t
hal_virtmem_usersize (void)
{
#ifdef __i386__
  return (3L << 30);		/* 3 GB */
#endif
#ifdef __amd64__
  return (size_t) 0x0000800000000000UL;
#endif
}

static bool
cpu_supports_nx (void)
{
  uint64_t efer;

  efer = rdmsr (MSR_IA32_EFER);
  return !!(efer & _MSR_IA32_EFER_NXE);
}

void
pmap_init (void)
{
  if (cpu_supports_nx ())
    pte_nx = PTE_NX;
  else
    printf ("CPU does not support NX.\n");

#ifdef __i386__
  pae32_init ();
#endif
#ifdef __amd64__
  pae64_init ();
#endif
}
