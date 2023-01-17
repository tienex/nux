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
#include <stdint.h>
#include <nux/nux.h>
#include <nux/types.h>
#include "../internal.h"

#define L1_SHIFT PAGE_SHIFT
#define L2_SHIFT (9 + PAGE_SHIFT)
#define L3_SHIFT (9 + 9 + PAGE_SHIFT)

#define L3_OFF(_va) (((_va) >> L3_SHIFT) & 3)
#define L2_OFF(_va) (((_va) >> L2_SHIFT) & 0x1ff)
#define L1_OFF(_va) (((_va) >> L1_SHIFT) & 0x1ff)

/* The following RES definitions assume 48-bit MAX PA */
#define L3_RESPT 0xFFFF000000000000LL
#define L2_RESPT 0x7FFF000000000000LL
#define L2_RES2M 0x7FFF0000001FE000LL
#define L1_RESPT 0x7FFF000000000000LL

#define l3e_reserved(_pte) ((_pte) & L3_RESPT)
#define l2e_bigpage(_pte) ((_pte) & PTE_PS)
#define l2e_reserved(_pte) ((_pte) & (l2e_bigpage(_pte) ? L2_RES2M : L2_RESPT))
#define l1e_reserved(_pte) ((_pte) & L1_RESPT)

#define mkpte(_p, _f) (((uint64_t)(_p) << PAGE_SHIFT) | (_f))
#define pte_pfn(_p) ((_p & ~PTE_NX) >> PAGE_SHIFT)
#define pte_present(_pte) ((_pte) & PTE_P)

#define PTE_INVALID ((uint64_t)-1)
#define PTEP_INVALID L1P_INVALID
#define mkptep_cur(_p) ((ptep_t)(uintptr_t)(_p))
#define mkptep_fgn(_p) ((ptep_t)(uintptr_t)(_p) | 1)
#define ptep_is_foreign(_p) ((_p) & 1)

extern int _linear_start;
extern int _linear_l2table;
extern int _linear_l3table;
const vaddr_t linaddr = (vaddr_t) & _linear_start;
const vaddr_t l2_linaddr = (vaddr_t) & _linear_l2table;
pte_t *l3_linaddr = (pte_t *) & _linear_l3table;

struct hal_pmap boot_pmap;

pte_t
get_pte (ptep_t ptep)
{
  if (ptep_is_foreign (ptep))
    {
      pte_t *t, pte;
      pfn_t pfn;
      unsigned offset;

      pfn = ptep >> PAGE_SHIFT;
      offset = ptep >> 3 & 0x1ff;

      t = (pte_t *) pfn_get (pfn);
      pte = t[offset];
      pfn_put (pfn, t);

      return pte;
    }
  else
    {
      return *(pte_t *) (uintptr_t) ptep;
    }

}

pte_t
set_pte (ptep_t ptep, pte_t pte)
{
  pte_t old;

  if (ptep_is_foreign (ptep))
    {
      pte_t *t;
      pfn_t pfn;
      unsigned offset;

      pfn = ptep >> PAGE_SHIFT;
      offset = ptep >> 3 & 0x1ff;

      t = (pte_t *) pfn_get (pfn);
      old = t[offset];
      t[offset] = pte;
      pfn_put (pfn, t);
    }
  else
    {
      old = *(pte_t *) ptep;
      *(pte_t *) ptep = pte;
    }

  return old;
}

static pte_t
alloc_table (bool user)
{
  pfn_t pfn;

  pfn = pfn_alloc (0);
  if (pfn == PFN_INVALID)
    return PTE_INVALID;

  return mkpte (pfn, PTE_P | PTE_W | (user ? PTE_U : 0));
}

static ptep_t
_get_l2p (struct align_pmap *ap, unsigned long va)
{
  ptep_t l2p;

  /* Assume all L3 PTE to be present */
  if (ap != NULL)
    {
      pte_t l3e, l2pfn;
      l3e = ap->pdptr[L3_OFF (va)];
      assert (pte_present (l3e));
      assert (!l3e_reserved (l3e) && "Invalid L3E.");
      l2pfn = pte_pfn (l3e);
      l2p = mkptep_fgn ((l2pfn << PAGE_SHIFT) + (L2_OFF (va) << 3));
    }
  else
    {
      va &= ~((1L << L2_SHIFT) - 1);
      l2p = mkptep_cur (l2_linaddr + (va >> 18));
    }
  return l2p;
}

static ptep_t
_get_l1p (struct align_pmap *ap, unsigned long va, bool alloc)
{
  ptep_t l2p, l1p;
  pte_t l2e;

  l2p = _get_l2p (ap, va);
  l2e = get_pte (l2p);

  if (!pte_present (l2e))
    {
      if (!alloc)
	return PTEP_INVALID;
      l2e = alloc_table (true);
      if (l2e == PTE_INVALID)
	return PTEP_INVALID;
      set_pte (l2p, l2e);
      /* Not present, no TLB flush necessary. */
    }

  assert (!l2e_reserved (l2e) && "Invalid L2E.");
  assert (!l2e_bigpage (l2e) && "Invalid page size.");

  if (ap == NULL)
    {
      va &= ~((1L << L1_SHIFT) - 1);
      l1p = mkptep_cur (linaddr + (va >> 9));
    }
  else
    {
      pfn_t l1pfn = pte_pfn (l2e);

      l1p = mkptep_fgn ((l1pfn << PAGE_SHIFT) + (L1_OFF (va) << 3));
    }

  return l1p;
}

static struct align_pmap *
get_align_pmap (struct hal_pmap *hal_pmap)
{
  uintptr_t ptr = (uintptr_t) hal_pmap;
  ptr += PAE_PDPTR_ALIGN - 1;
  ptr &= ~(PAE_PDPTR_ALIGN - 1);
  return (struct align_pmap *) ptr;

}

static unsigned
get_align_pmap_offset (struct hal_pmap *hal_pmap)
{
  uintptr_t hp = (uintptr_t) hal_pmap;
  uintptr_t ap = (uintptr_t) get_align_pmap (hal_pmap);
  return (unsigned) (ap - hp);
}

#if 0
static struct hal_pmap *
get_hal_pmap (struct align_pmap *pmap)
{
  uintptr_t ptr = (uintptr_t) pmap;
  ptr -= pmap->align;
  return (struct hal_pmap *) ptr;
}
#endif


hal_l1p_t
get_l1p (struct hal_pmap *pmap, unsigned long va, int alloc)
{
  struct align_pmap *ap;
  hal_l1p_t l1p;

  if (pmap != NULL)
    {
      ap = get_align_pmap (pmap);
    }
  else
    {
      ap = NULL;
    }

  l1p = (hal_l1p_t) _get_l1p (ap, va, alloc);
  return l1p;
}

void
pae32_init (void)
{
  struct align_pmap *ap;

  /* Initialize boot_pmap */
  ap = get_align_pmap (&boot_pmap);
  ap->pdptr[0] = l3_linaddr[0];
  ap->pdptr[1] = l3_linaddr[1];
  ap->pdptr[2] = l3_linaddr[2];
  ap->pdptr[3] = l3_linaddr[3];
  ap->align = get_align_pmap_offset (&boot_pmap);
  printf ("LIN PTES: %llx, %llx, %llx, %llx\n", ap->pdptr[0], ap->pdptr[1],
	  ap->pdptr[2], ap->pdptr[3]);
}
