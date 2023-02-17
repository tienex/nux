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

/*
  Boot-loader assumptions:

  All L3s are present. This is to allow UMAP to be consistent across
  all CPUs when new mappings are created.

  All kernel L2s are present. This is to allow the kernel mappings to
  be consistent across all CPUs.
*/

#include <assert.h>
#include <stdint.h>
#include <nux/nux.h>
#include <nux/plt.h>
#include <nux/types.h>
#include "../internal.h"

#define L1_SHIFT PAGE_SHIFT
#define L2_SHIFT (9 + PAGE_SHIFT)
#define L3_SHIFT (9 + 9 + PAGE_SHIFT)

#define L3OFF(_va) (((_va) >> L3_SHIFT) & 3)
#define L2OFF(_va) (((_va) >> L2_SHIFT) & 0x1ff)
#define L1OFF(_va) (((_va) >> L1_SHIFT) & 0x1ff)

unsigned long
l2off (unsigned long _va)
{
  return (((_va) >> L2_SHIFT) & 0x1ff);
}

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

#define PTE_INVALID ((uint64_t)0)
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

struct pdptr
{
#define NPDPTE 4
  uint64_t pdptr[NPDPTE*2];
#undef NPDPTE
} __packed;

struct pdptr pdptrs[MAXCPUS] __aligned (64);
struct pdptr bootstrap_pdptr __aligned (64);

typedef uint64_t hal_l1e_t;

uint64_t
mkaddr (uint64_t l3off, uint64_t l2off, uint64_t l1off)
{
  return (l3off << L3_SHIFT) | (l2off << L2_SHIFT) | (l1off << L1_SHIFT);
}

pte_t
get_pte (ptep_t ptep)
{
  assert (ptep != PTEP_INVALID);
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

  assert (ptep != PTEP_INVALID);
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

pte_t
set_l3e (unsigned long va, pte_t pte)
{
  struct pdptr *pdptr = pdptrs + plt_pcpu_id ();
  pte_t old;

  /*
     Everything is special in PTE.

     The PTEs that create the linear page table are not a reflection of
     the actual top level pagetable. So we need to write both of them.
   */
  old = l3_linaddr[L3OFF (va)];
  l3_linaddr[L3OFF (va)] = pte;
  pdptr->pdptr[L3OFF (va)] = pte;

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
linmap_get_l3p (unsigned long va)
{
  return mkptep_cur (l3_linaddr + L3OFF (va));
}

static ptep_t
linmap_get_l2p (unsigned long va)
{
  ptep_t l3p;
  pte_t l3e;

  l3p = linmap_get_l3p (va);
  l3e = get_pte (l3p);
  /* In PAE32, All L3E must be present. */
  assert (pte_present (l3e));

  va &= ~((1L << L2_SHIFT) - 1);
  return mkptep_cur (l2_linaddr + (va >> 18));
}

static pte_t
linmap_get_l2e (unsigned long va)
{
  return *(pte_t *) (l2_linaddr + (va >> 18));
}

static ptep_t
linmap_get_l1p (unsigned long va, bool alloc, bool user)
{
  ptep_t l2p;
  pte_t l2e;

  l2p = linmap_get_l2p (va);
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

  va &= ~((1L << L1_SHIFT) - 1);
  return mkptep_cur (linaddr + (va >> 9));
}

static ptep_t
get_umap_l3p (struct hal_umap *umap, unsigned long va)
{
  assert (umap != NULL);
  assert (L3OFF (va) < UMAP_L3PTES);

  return mkptep_cur (umap->l3 + L3OFF (va));
}

static pfn_t
get_umap_l2pfn (struct hal_umap *umap, unsigned long va, bool alloc)
{
  ptep_t l3p;
  pte_t l3e;

  l3p = get_umap_l3p (umap, va);
  l3e = get_pte (l3p);

  assert (pte_present (l3e));
  //  assert (!l3e_reserved (l3e) && "Invalid L3E.");

  return pte_pfn (l3e);
}

static ptep_t
get_umap_l2p (struct hal_umap *umap, unsigned long va, bool alloc)
{
  pfn_t l2pfn;

  l2pfn = get_umap_l2pfn (umap, va, alloc);
  if (l2pfn == PFN_INVALID)
    {
      return PTEP_INVALID;
    }

  return mkptep_fgn ((l2pfn << PAGE_SHIFT) + (L2OFF (va) << 3));
}

static pfn_t
get_umap_l1pfn (struct hal_umap *umap, unsigned long va, bool alloc)
{
  ptep_t l2p;
  pte_t l2e;

  l2p = get_umap_l2p (umap, va, alloc);
  l2e = get_pte (l2p);

  if (!pte_present (l2e))
    {
      if (!alloc)
	return PFN_INVALID;
      l2e = alloc_table (true /* user */ );
      if (l2e == PTE_INVALID)
	return PFN_INVALID;
      set_pte (l2p, l2e);
      /* Not present, no TLB flush necessary. */
    }


  assert (!l2e_reserved (l2e) && "Invalid L2E.");
  assert (!l2e_bigpage (l2e) && "Invalid page size.");

  return pte_pfn (l2e);
}

ptep_t
umap_get_l1p (struct hal_umap *umap, unsigned long va, bool alloc)
{
  pfn_t l1pfn;
  if (umap == NULL)
    {
      /* Use PMAP if current. */
      assert (L3OFF (va) < UMAP_L3PTES);
      return linmap_get_l1p (va, alloc, true /* user */ );
    }

  l1pfn = get_umap_l1pfn (umap, va, alloc);
  if (l1pfn == PFN_INVALID)
    {
      return PTEP_INVALID;
    }
  return mkptep_fgn ((l1pfn << PAGE_SHIFT) + (L1OFF (va) << 3));
}

unsigned long
umap_maxaddr (void)
{
  return 3L << L3_SHIFT;
}

/* Note: this does not check for va being user. */
hal_l1p_t
kmap_get_l1p (unsigned long va, bool alloc)
{
  return (hal_l1p_t) linmap_get_l1p (va, alloc, false
				     /* !user */
    );
}

void
hal_umap_init (struct hal_umap *umap)
{
  pfn_t pfn;

  for (int i = 0; i < UMAP_L3PTES; i++)
    {
      pfn = pfn_alloc (0);
      assert (pfn != PFN_INVALID);
      umap->l3[i] = mkpte (pfn, PTE_P);
    }
}

void
hal_umap_bootstrap (struct hal_umap *umap)
{
  vaddr_t va = hal_virtmem_userbase ();
  int i;
  for (i = 0; i < UMAP_L3PTES; i++, va += (1L << L3_SHIFT))
    {
      ptep_t l3p;
      pte_t l3e;
      l3p = linmap_get_l3p (va);
      l3e = get_pte (l3p);
      if (!pte_present(l3e))
	{
	  pfn_t pfn;

	  pfn = pfn_alloc (0);
	  assert (pfn != PFN_INVALID);
	  l3e = mkpte (pfn, PTE_P);
	}
      umap->l3[i] = l3e;
    }
}

hal_tlbop_t
hal_umap_load (struct hal_umap *umap)
{
  vaddr_t va = hal_virtmem_userbase ();
  hal_tlbop_t tlbop = HAL_TLBOP_NONE;
  int i;
  for (i = 0; i < UMAP_L3PTES; i++, va += (1L << L3_SHIFT))
    {
      pte_t oldl3e, newl3e;
      if (umap != NULL)
	{
	  newl3e = umap->l3[i];
	}
      else
	newl3e = 0;
      oldl3e = set_l3e (va, newl3e);
      tlbop |= hal_l1e_tlbop (oldl3e, newl3e);
    }
  return tlbop;
}

static bool
scan_l1 (pfn_t l1pfn, unsigned off,
	 unsigned *l1off_out, hal_l1p_t * l1p_out, hal_l1e_t * l1e_out)
{
  pte_t *l1ptr, l1e;
  l1ptr = pfn_get (l1pfn);
  for (unsigned i = off; i < 512; i++)
    {
      l1e = l1ptr[i];
      if (l1e != 0)
	{
	  if (l1p_out != NULL)
	    *l1p_out = mkptep_fgn ((l1pfn << PAGE_SHIFT) + (i << 3));
	  if (l1e_out != NULL)
	    *l1e_out = l1e;
	  *l1off_out = i;
	  pfn_put (l1pfn, l1ptr);
	  return true;
	}
    }
  pfn_put (l1pfn, l1ptr);
  return false;
}

static bool
scan_l2 (pfn_t l2pfn, unsigned off,
	 unsigned *l2off_out,
	 unsigned *l1off_out, hal_l1p_t * l1p_out, hal_l1e_t * l1e_out)
{
  pte_t *l2ptr, l2e;
  pfn_t l1pfn;
  l2ptr = pfn_get (l2pfn);
  for (unsigned i = off; i < 512; i++)
    {
      l2e = l2ptr[i];
      if (pte_present (l2e))
	{
	  l1pfn = pte_pfn (l2e);
	  if (scan_l1 (l1pfn, 0, l1off_out, l1p_out, l1e_out))
	    {
	      *l2off_out = i;
	      pfn_put (l2pfn, l2ptr);
	      return true;
	    }
	}
    }
  pfn_put (l2pfn, l2ptr);
  return false;
}

static bool
scan_l3 (struct hal_umap *umap, unsigned off,
	 unsigned *l3off_out,
	 unsigned *l2off_out,
	 unsigned *l1off_out, hal_l1p_t * l1p_out, hal_l1e_t * l1e_out)
{
  pte_t l3e;
  pfn_t l2pfn;
  for (unsigned i = off; i < UMAP_L3PTES; i++)
    {
      l3e = umap->l3[i];
      if (pte_present (l3e))
	{
	  l2pfn = pte_pfn (l3e);
	  if (scan_l2 (l2pfn, 0, l2off_out, l1off_out, l1p_out, l1e_out))
	    {
	      *l3off_out = i;
	      return true;
	    }
	}
    }
  return false;
}

uaddr_t
umap_next (struct hal_umap *umap,
	   uaddr_t uaddr, hal_l1p_t * l1p_out, hal_l1e_t * l1e_out)
{
  unsigned l3off = L3OFF (uaddr);
  unsigned l2off = L2OFF (uaddr);
  unsigned l1off = L1OFF (uaddr);
  pfn_t l1pfn, l2pfn;
  unsigned l3next, l2next, l1next;
  /* Check till end of current l1. */
  l1pfn = get_umap_l1pfn (umap, uaddr, false);
  if (l1pfn != PFN_INVALID
      && scan_l1 (l1pfn, l1off + 1, &l1next, l1p_out, l1e_out))
    {
      return mkaddr (l3off, l2off, l1next);
    }

  /* Check till end of current l2. */
  l2pfn = get_umap_l2pfn (umap, uaddr, false);
  if (l2pfn != PFN_INVALID
      && scan_l2 (l2pfn, l2off + 1, &l2next, &l1next, l1p_out, l1e_out))
    {
      return mkaddr (l3off, l2next, l1next);
    }

  if (scan_l3 (umap, l3off + 1, &l3next, &l2next, &l1next, l1p_out, l1e_out))
    {
      return mkaddr (l3next, l2next, l1next);
    }

  return UADDR_INVALID;
}

void
umap_free (struct hal_umap *umap)
{
  pfn_t l2pfn, l1pfn;
  pte_t l3e, *l2ptr, l2e;
  for (unsigned i = 0; i < UMAP_L3PTES; i++)
    {
      l3e = umap->l3[i];
      if (pte_present (l3e))
	{
	  l2pfn = pte_pfn (l3e);
	  l2ptr = pfn_get (l2pfn);
	  for (unsigned i = 0; i < 512; i++)
	    {
	      l2e = l2ptr[i];
	      if (pte_present (l2e))
		{
		  l1pfn = pte_pfn (l2e);
		  printf ("Freeing L1 %lx\n", l1pfn);
		  pfn_free (l1pfn);
		}
	    }
	  printf ("Freeing L2 %lx\n", l2pfn);
	  pfn_put (l2pfn, l2ptr);
	  pfn_free (l2pfn);
	}
      umap->l3[i] = PTE_INVALID;
    }
}

static void _alloc_pagetable(struct pdptr *pdptr)
{
  pfn_t k_l2pfn;
  pte_t *l2ptr;

  /* Alloc new L2 for kernel */
  k_l2pfn = pfn_alloc (0);

  /* Set the L3s. */
  for (unsigned i = 0; i < 4; i++)
    {
      if (i == 3)
	pdptr->pdptr[i] = mkpte (k_l2pfn, PTE_P);
      else
	pdptr->pdptr[i] = 0;
    }

  /* Copy kernel address and fix linear addresses. */
  l2ptr = pfn_get (k_l2pfn);
  for (unsigned i = 0; i < 512; i++)
    {
      if ((i >= L2OFF (linaddr)) && (i < (L2OFF (linaddr) + 3)))
	{
	  printf ("there -- %llx --", mkaddr (3, i, 0));
	  l2ptr[i] = 0;
	}
      else if (i == L2OFF (linaddr) + 3)
	{
	  printf ("-- here: %lx --", mkaddr (3, i, 0));
	  l2ptr[i] = mkpte (k_l2pfn, PTE_P | PTE_W);
	}
      else
	{
	  l2ptr[i] = linmap_get_l2e (mkaddr (3, i, 0));
	}
    }
  pfn_put (k_l2pfn, l2ptr);
}

unsigned long
cpu_pagetable (void)
{
  struct pdptr *pdptr = pdptrs + plt_pcpu_id ();
  ptep_t l1p;
  pte_t l1e;

  _alloc_pagetable(pdptr);

  /* Get physical address of pdptr and return CR3 */
  l1p = linmap_get_l1p ((uintptr_t) pdptr, false, false);
  assert (l1p != L1P_INVALID);
  l1e = get_pte (l1p);
  assert (pte_present (l1e));
  return (pte_pfn (l1e) << PAGE_SHIFT) +
    ((uintptr_t) pdptr & (PAGE_SIZE - 1));
}

unsigned long
bootstrap_pagetable (pfn_t bootstrap_pfn)
{
  struct pdptr *pdptr = &bootstrap_pdptr;;
  pte_t *pteptr;
  unsigned long bootstrap_va;
  pfn_t l2pfn, l1pfn;
  ptep_t l1p;
  pte_t l1e;

  _alloc_pagetable(pdptr);

  l2pfn = pfn_alloc(0);
  assert (l2pfn != PFN_INVALID);
  l1pfn = pfn_alloc(0);
  assert (l1pfn != PFN_INVALID);

  /* Map 1:1 the bootstrap PFN */
  bootstrap_va = ((unsigned long)bootstrap_pfn << PAGE_SHIFT);
  pdptr->pdptr[L3OFF(bootstrap_va)] = mkpte(l2pfn, PTE_P);

  pteptr = pfn_get (l2pfn);
  pteptr[L2OFF(bootstrap_va)] = mkpte(l1pfn, PTE_W|PTE_P);
  pfn_put (l2pfn, pteptr);

  pteptr = pfn_get (l1pfn);
  pteptr[L1OFF(bootstrap_va)] = mkpte(bootstrap_pfn, PTE_W|PTE_P);
  pfn_put (l1pfn, pteptr);

  /* Get physical address of pdptr and return CR3 */
  l1p = linmap_get_l1p ((uintptr_t) pdptr, false, false);
  assert (l1p != L1P_INVALID);
  l1e = get_pte (l1p);
  assert (pte_present (l1e));
  return (pte_pfn (l1e) << PAGE_SHIFT) +
    ((uintptr_t) pdptr & (PAGE_SIZE - 1));
}

void
pae32_init (void)
{
}
