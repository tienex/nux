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
#define L2_SHIFT (L1_SHIFT + 9)
#define L3_SHIFT (L2_SHIFT + 9)
#define L4_SHIFT (L3_SHIFT + 9)

#define L4OFF(_va) (((_va) >> L4_SHIFT) & 0x1ff)
#define L3OFF(_va) (((_va) >> L3_SHIFT) & 0x1ff)
#define L2OFF(_va) (((_va) >> L2_SHIFT) & 0x1ff)
#define L1OFF(_va) (((_va) >> L1_SHIFT) & 0x1ff)

#define UNCANON(_va) ((_va) & ((1LL << 48) - 1))

/* The following RES definitions assume 48-bit MAX PA */
#define L4_RESPT 0x000F000000000080
#define L3_RESPT 0x000F000000000000
#define L3_RES1G 0x000F00003fffe000
#define L2_RESPT 0x000F000000000000
#define L2_RES2M 0x000F0000001fe000
#define L1_RESPT 0x000F000000000000

/* Assume PTE_P is set */
#define l4e_reserved(_pte) ((_pte) & L4_RESPT)
#define l3e_bigpage(_pte) ((_pte) & PTE_PS)
#define l3e_bigpage(_pte) ((_pte) & PTE_PS)
#define l3e_reserved(_pte) ((_pte) & (l3e_bigpage(_pte) ? L3_RES1G : L3_RESPT))
#define l2e_bigpage(_pte) ((_pte) & PTE_PS)
#define l2e_reserved(_pte) ((_pte) & (l2e_bigpage(_pte) ? L2_RES2M : L2_RESPT))
#define l1e_reserved(_pte) ((_pte) & L1_RESPT)

#define mkpte(_p, _f) (((uint64_t)(_p) << PAGE_SHIFT) | (_f))
#define pte_pfn(_p) ((pfn_t)(((uint64_t) (_p) & ~PTE_NX) >> PAGE_SHIFT))
#define pte_present(_pte) ((_pte) & PTE_P)

#define PTE_INVALID ((uint64_t)-1)
#define PTEP_INVALID L1P_INVALID
#define mkptep_cur(_p) ((ptep_t)(uintptr_t)(_p))
#define mkptep_fgn(_p) ((ptep_t)(uintptr_t)(_p) | 1)
#define ptep_is_foreign(_p) ((_p) & 1)

extern int _linear_start;
static const pte_t *linaddr = (const pte_t *) &_linear_start;
static pte_t *linaddr_l2;
static pte_t *linaddr_l3;
static pte_t *linaddr_l4;

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
get_pmap_l4p (unsigned long va)
{
  return mkptep_cur (linaddr_l4 + (UNCANON (va) >> L4_SHIFT));
}

static pte_t
get_pmap_l4e (unsigned long va)
{
  return *(pte_t *)(linaddr_l4 + (UNCANON (va) >> L4_SHIFT));
}

static ptep_t
get_pmap_l3p (unsigned long va, bool alloc, bool user)
{
  ptep_t l4p;
  pte_t l4e;

  l4p = get_pmap_l4p (va);
  l4e = get_pte (l4p);

  if (!pte_present (l4e))
    {
      if (!alloc)
	return PTEP_INVALID;
      l4e = alloc_table (user);
      if (l4e == PTE_INVALID)
	return PTEP_INVALID;
      set_pte (l4p, l4e);
      /* Not present, no TLB flush necessary. */
    }

  assert (!l4e_reserved (l4e) && "Invalid L4E.");

  return mkptep_cur (linaddr_l3 + (UNCANON (va) >> L3_SHIFT));
}

static ptep_t
get_pmap_l2p (unsigned long va, bool alloc, bool user)
{
  ptep_t l3p;
  pte_t l3e;

  l3p = get_pmap_l3p (va, alloc, user);
  l3e = get_pte (l3p);

  if (!pte_present (l3e))
    {
      if (!alloc)
	return PTEP_INVALID;
      l3e = alloc_table (user);
      if (l3e == PTE_INVALID)
	return PTEP_INVALID;
      set_pte (l3p, l3e);
      /* Not present, no TLB flush necessary. */
    }

  assert (!l3e_reserved (l3e) && "Invalid L3E.");
  assert (!l3e_bigpage (l3e) && "Invalid page size.");

  return mkptep_cur (linaddr_l2 + (UNCANON (va) >> L2_SHIFT));
}

static ptep_t
get_pmap_l1p (unsigned long va, bool alloc, bool user)
{
  ptep_t l2p;
  pte_t l2e;

  l2p = get_pmap_l2p (va, alloc, user);
  l2e = get_pte (l2p);

  if (!pte_present (l2e))
    {
      if (!alloc)
	return PTEP_INVALID;
      l2e = alloc_table (user);
      if (l2e == PTE_INVALID)
	return PTEP_INVALID;
      set_pte (l2p, l2e);
      /* Not present, no TLB flush necessary. */
    }

  assert (!l2e_reserved (l2e) && "Invalid L2E.");
  assert (!l2e_bigpage (l2e) && "Invalid page size.");

  return mkptep_cur (linaddr + (UNCANON (va) >> L1_SHIFT));
}

static ptep_t
get_umap_l4p (struct hal_umap *umap, unsigned long va)
{
  assert (umap != NULL);
  assert (L4OFF(va) < UMAP_L4PTES);

  return mkptep_cur(umap->l4 + L4OFF(va));
}


static ptep_t
get_umap_l3p (struct hal_umap *umap, unsigned long va, bool alloc)
{
  ptep_t l4p;
  pte_t l4e;
  pfn_t l3pfn;

  l4p = get_umap_l4p (umap, va);
  l4e = get_pte (l4p);

  if (!pte_present (l4e))
    {
      if (!alloc)
	return PTEP_INVALID;
      l4e = alloc_table (true /* user */);
      if (l4e == PTE_INVALID)
	return PTEP_INVALID;
      set_pte (l4p, l4e);
      /* Not present, no TLB flush necessary. */
    }
  
  assert (!l4e_reserved (l4e) && "Invalid L4E.");

  l3pfn = pte_pfn (l4e);
  
  return mkptep_fgn ((l3pfn << PAGE_SHIFT) + (L3OFF (va) << 3));
}

static ptep_t
get_umap_l2p (struct hal_umap *umap, unsigned long va, bool alloc)
{
  ptep_t l3p;
  pte_t l3e;
  pfn_t l2pfn;

  l3p = get_umap_l3p (umap, va, alloc);
  l3e = get_pte (l3p);

  if (!pte_present (l3e))
    {
      if (!alloc)
	return PTEP_INVALID;
      l3e = alloc_table (true /* user */);
      if (l3e == PTE_INVALID)
	return PTEP_INVALID;
      set_pte (l3p, l3e);
      /* Not present, no TLB flush necessary. */
    }

  assert (!l3e_reserved (l3e) && "Invalid L3E.");
  assert (!l3e_bigpage (l3e) && "Invalid page size.");

  l2pfn = pte_pfn (l3e);

  return mkptep_fgn ((l2pfn << PAGE_SHIFT) + (L2OFF (va) << 3));
}

static ptep_t
get_umap_l1p (struct hal_umap *umap, unsigned long va, bool alloc)
{
  ptep_t l2p;
  pte_t l2e;
  pfn_t l1pfn;

  if (umap == NULL)
    {
      /* Use PMAP if current. */
      return get_pmap_l1p (va, alloc, true /* user */);
    }

  l2p = get_umap_l2p (umap, va, alloc);
  l2e = get_pte (l2p);

  if (!pte_present (l2e))
    {
      if (!alloc)
	return PTEP_INVALID;
      l2e = alloc_table (true /* user */);
      if (l2e == PTE_INVALID)
	return PTEP_INVALID;
      set_pte (l2p, l2e);
      /* Not present, no TLB flush necessary. */
    }


  assert (!l2e_reserved (l2e) && "Invalid L2E.");
  assert (!l2e_bigpage (l2e) && "Invalid page size.");

  l1pfn = pte_pfn (l2e);

  return mkptep_fgn ((l1pfn << PAGE_SHIFT) + (L1OFF (va) << 3));
}

/*
static ptep_t
_get_l4p (struct hal_pmap *pmap, unsigned long va)
{


  if (pmap == NULL)
    {
      
    }
  else
    {
      l4p = mkptep_fgn ((pmap->l4pfn << PAGE_SHIFT) + (L4OFF (va) << 3));
    }
  return l4p;
}
*/
/*
static ptep_t
_get_l3p (struct hal_pmap *pmap, unsigned long va, bool alloc)
{




  if (pmap == NULL)
    {

    }
  else
    {
      pfn_t l3pfn = pte_pfn (l4e);

      l3p = mkptep_fgn ((l3pfn << PAGE_SHIFT) + (L3OFF (va) << 3));
    }

}
*/

/*
static ptep_t
_get_l2p (struct hal_pmap *pmap, unsigned long va, bool alloc)
{
  ptep_t l3p, l2p;
  pte_t l3e;

  l3p = _get_l3p (pmap, va, alloc);
  l3e = get_pte (l3p);

  if (!pte_present (l3e))
    {
      if (!alloc)
	return PTEP_INVALID;
      l3e = alloc_table (true);
      if (l3e == PTE_INVALID)
	return PTEP_INVALID;
      set_pte (l3p, l3e);
      / * Not present, no TLB flush necessary. * /
    }

  assert (!l3e_reserved (l3e) && "Invalid L3E.");
  assert (!l3e_bigpage (l3e) && "Invalid page size.");

  if (pmap == NULL)
    {
      l2p = mkptep_cur (linaddr_l2 + (UNCANON (va) >> L2_SHIFT));
    }
  else
    {
      pfn_t l2pfn = pte_pfn (l3e);

      l2p = mkptep_fgn ((l2pfn << PAGE_SHIFT) + (L2OFF (va) << 3));
    }
  return l2p;
}


static ptep_t
_get_l1p (struct hal_pmap *pmap, unsigned long va, bool alloc)
{
  ptep_t l2p, l1p;
  pte_t l2e;

  l2p = _get_l2p (pmap, va, alloc);
  l2e = get_pte (l2p);

  if (!pte_present (l2e))
    {
      if (!alloc)
	return PTEP_INVALID;
      l2e = alloc_table (true);
      if (l2e == PTE_INVALID)
	return PTEP_INVALID;
      set_pte (l2p, l2e);
      / * Not present, no TLB flush necessary. * /
    }

  assert (!l2e_reserved (l2e) && "Invalid L2E.");
  assert (!l2e_bigpage (l2e) && "Invalid page size.");

  if (pmap == NULL)
    {
      l1p = mkptep_cur (linaddr + (UNCANON (va) >> L1_SHIFT));
    }
  else
    {
      pfn_t l1pfn = pte_pfn (l2e);

      l1p = mkptep_fgn ((l1pfn << PAGE_SHIFT) + (L1OFF (va) << 3));
    }

  return l1p;
}
*/

hal_l1p_t
get_l1p (unsigned long va, int alloc)
{
  return (hal_l1p_t) get_pmap_l1p (va, alloc, false /* !user */);
}

const vaddr_t
hal_virtmem_userbase (void)
{
  return 0;
}

const size_t
hal_virtmem_usersize (void)
{
  return 1L << (39 + UMAP_LOG2_L4PTES);
}

void
hal_umap_init (struct hal_umap *umap)
{
  for (int i = 0; i < UMAP_L4PTES; i++)
    {
      umap->l4[i] = 0;
    }
}

void
hal_umap_bootstrap (struct hal_umap *umap)
{
  vaddr_t va = hal_virtmem_userbase ();
  int i;

  for (i = 0; i < UMAP_L4PTES; i++, va += (1L << L4_SHIFT))
    {
      ptep_t l4p;
      pte_t l4e;

        l4p = get_pmap_l4p (va);
	l4e = get_pte (l4p);

	if (!pte_present (l4e))
	  {
	    l4e = alloc_table (true);
	    /* We're in bootstrap. Can assert. */
	    assert (l4e != PTE_INVALID);
	    set_pte (l4p, l4e);
	    /* Not present, no TLB flush necessary. */
	  }
	umap->l4[i] = l4e;
    }

  /* Panic if the boot user mapping doesn't fit in a UMAP. */
  for (; i < 256; i++, va += (1L << L4_SHIFT))
    {
      assert (!pte_present (get_pmap_l4e (va)));
    }
}

bool
hal_umap_getl1p (struct hal_umap *umap, unsigned long uaddr, bool alloc, hal_l1p_t * l1popq)
{
  hal_l1p_t l1p;

  /* hal_virtmem_base() is zero */
  if (uaddr >= hal_virtmem_usersize())
    {
      *l1popq = L1P_INVALID;
      return false;
    }

  l1p = get_umap_l1p (umap, uaddr, alloc);
  if (l1popq != NULL)
    *l1popq = l1p;

  return l1p != L1P_INVALID;
}

void
pae64_init (void)
{
  linaddr_l2 =
    (pte_t *) linaddr + (((uintptr_t) linaddr & ((1L << 48) - 1)) >> (9 + 3));
  linaddr_l3 =
    linaddr_l2 + (((uintptr_t) linaddr & ((1L << 48) - 1)) >> (18 + 3));
  linaddr_l4 =
    linaddr_l3 + (((uintptr_t) linaddr & ((1L << 48) - 1)) >> (27 + 3));
}
