#include <assert.h>
#include "internal.h"

#define SATP_PFN_MASK ((1L << 44) - 1)

#define L1_SHIFT PAGE_SHIFT
#define L2_SHIFT (L1_SHIFT + 9)
#define L3_SHIFT (L2_SHIFT + 9)
#define L4_SHIFT (L3_SHIFT + 9)

#define L4OFF(_va) (((_va) >> L4_SHIFT) & 0x1ff)
#define L3OFF(_va) (((_va) >> L3_SHIFT) & 0x1ff)
#define L2OFF(_va) (((_va) >> L2_SHIFT) & 0x1ff)
#define L1OFF(_va) (((_va) >> L1_SHIFT) & 0x1ff)

#define MKCANON(_va) ((int64_t)((_va) << 16) >> 16)
#define UNCANON(_va) ((_va) & ((1LL << 48) - 1))

#define PTE_INVALID ((uint64_t)0)
#define PTEP_INVALID L1P_INVALID

static uint64_t
mkaddr (uint64_t l4off, uint64_t l3off, uint64_t l2off, uint64_t l1off)
{
  return
    MKCANON (((l4off << L4_SHIFT) | (l3off << L3_SHIFT) | (l2off << L2_SHIFT)
	      | (l1off << L1_SHIFT)));
}

static pfn_t
get_cpumap_l4pfn (void)
{
  unsigned long satp;

  satp = riscv_satp ();
  return (pfn_t) (satp & SATP_PFN_MASK);
}

static pte_t *
get_cpumap_l4off (unsigned off)
{
  pte_t *t;

  t = (pte_t *) pfn_get (get_cpumap_l4pfn ());

  assert(off < 512);
  return t + off;
}

static pte_t *
get_cpumap_l4ptr (unsigned long va)
{

  return get_cpumap_l4off (L4OFF(va));
}

static void
put_cpumap_l4ptr (unsigned long va /* REMOVE ME */, pte_t * pte)
{
  pfn_put (get_cpumap_l4pfn (), pte);
}


static pfn_t
walk_l3pfn (pte_t * l4ptr, unsigned long va, bool alloc)
{
  pte_t l4e;

  l4e = *l4ptr;

  if (!pte_valid (l4e))
    {
      if (!alloc)
	return PFN_INVALID;
      l4e = alloc_table ();
      if (l4e == PTE_INVALID)
	return PFN_INVALID;
      *l4ptr = l4e;
      /* NP->P, NO SVVPTC ONLY */
      riscv_invlpg (va, true);
    }

  assert (pte_valid_table (l4e));

  return pte_pfn (l4e);
}

static ptep_t
walk_l3p (pte_t * l4ptr, unsigned long va, bool alloc)
{
  pfn_t l3pfn;

  l3pfn = walk_l3pfn (l4ptr, va, alloc);
  if (l3pfn == PFN_INVALID)
    return PTEP_INVALID;

  return mkptep (l3pfn, L3OFF (va));
}

static pfn_t
walk_l2pfn (pte_t * l4ptr, unsigned long va, bool alloc)
{
  ptep_t l3p;
  pte_t l3e;

  l3p = walk_l3p (l4ptr, va, alloc);
  if (l3p == PTEP_INVALID)
    return PFN_INVALID;
  l3e = get_pte (l3p);

  if (!pte_valid (l3e))
    {
      if (!alloc)
	return PFN_INVALID;
      l3e = alloc_table ();
      if (l3e == PTE_INVALID)
	return PFN_INVALID;
      set_pte (l3p, l3e);
      /* NP->P, NO SVVPTC ONLY */
      riscv_invlpg (va, true);
    }

  assert (pte_valid_table (l3e));

  return pte_pfn (l3e);
}

static ptep_t
walk_l2p (pte_t * l4ptr, unsigned long va, bool alloc)
{
  pfn_t l2pfn;

  l2pfn = walk_l2pfn (l4ptr, va, alloc);
  if (l2pfn == PFN_INVALID)
    return PTEP_INVALID;

  return mkptep (l2pfn, L2OFF (va));
}

static pfn_t
walk_l1pfn (pte_t * l4ptr, unsigned long va, bool alloc)
{
  ptep_t l2p;
  pte_t l2e;

  l2p = walk_l2p (l4ptr, va, alloc);
  if (l2p == PTEP_INVALID)
    return PFN_INVALID;
  l2e = get_pte (l2p);

  if (!pte_valid (l2e))
    {
      if (!alloc)
	return PFN_INVALID;
      l2e = alloc_table ();
      if (l2e == PTE_INVALID)
	return PFN_INVALID;
      set_pte (l2p, l2e);
      /* NP->P, NO SVVPTC ONLY */
      riscv_invlpg (va, true);
    }

  assert (pte_valid_table (l2e));

  return pte_pfn (l2e);
}

static ptep_t
walk_l1p (pte_t * l4ptr, unsigned long va, bool alloc)
{
  pfn_t l1pfn;

  l1pfn = walk_l1pfn (l4ptr, va, alloc);
  if (l1pfn == PFN_INVALID)
    return PTEP_INVALID;

  return mkptep (l1pfn, L1OFF (va));
}

hal_l1p_t
cpumap_get_l1p (unsigned long va, int alloc)
{
  pte_t *l4ptr;
  ptep_t l1p;

  l4ptr = get_cpumap_l4ptr (va);
  l1p = walk_l1p (l4ptr, va, alloc);
  put_cpumap_l4ptr (va, l4ptr);

  return l1p;
}

hal_l1p_t
umap_get_l1p (struct hal_umap *umap, unsigned long va, bool alloc)
{
  pte_t *l4ptr;
  ptep_t l1p;

  assert (L4OFF (va) < UMAP_L4PTES);
  if (umap == NULL)
    l4ptr = get_cpumap_l4ptr (va);
  else
    l4ptr = umap->l4 + L4OFF (va);


  l1p = walk_l1p (l4ptr, va, alloc);

  if (umap == NULL)
    put_cpumap_l4ptr (va, l4ptr);

  return l1p;
}

static bool
scan_l1 (pfn_t l1pfn, unsigned off, unsigned *l1off_out, hal_l1p_t * l1p_out,
	 hal_l1e_t * l1e_out)
{
  pte_t *l1ptr, l1e;

  l1ptr = pfn_get (l1pfn);
  for (unsigned i = off; i < 512; i++)
    {
      l1e = l1ptr[i];
      if (l1e != 0)
	{
	  if (l1p_out != NULL)
	    *l1p_out = mkpte (l1pfn, i);
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
scan_l2 (pfn_t l2pfn, unsigned off, unsigned *l2off_out, unsigned *l1off_out,
	 hal_l1p_t * l1p_out, hal_l1e_t * l1e_out)
{
  pte_t *l2ptr, l2e;
  pfn_t l1pfn;

  l2ptr = pfn_get (l2pfn);
  for (unsigned i = off; i < 512; i++)
    {
      l2e = l2ptr[i];
      if (pte_valid_table (l2e))
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
scan_l3 (pfn_t l3pfn, unsigned off, unsigned *l3off_out, unsigned *l2off_out,
	 unsigned *l1off_out, hal_l1p_t * l1p_out, hal_l1e_t * l1e_out)
{
  pte_t *l3ptr, l3e;
  pfn_t l2pfn;

  l3ptr = pfn_get (l3pfn);
  for (unsigned i = off; i < 512; i++)
    {
      l3e = l3ptr[i];
      if (pte_valid_table (l3e))
	{
	  l2pfn = pte_pfn (l3e);
	  if (scan_l2 (l2pfn, 0, l2off_out, l1off_out, l1p_out, l1e_out))
	    {
	      *l3off_out = i;
	      pfn_put (l3pfn, l3ptr);
	      return true;
	    }
	}
    }
  pfn_put (l3pfn, l3ptr);
  return false;
}

static bool
scan_l4 (struct hal_umap *umap, unsigned off, unsigned *l4off_out,
	 unsigned *l3off_out, unsigned *l2off_out, unsigned *l1off_out,
	 hal_l1p_t * l1p_out, hal_l1e_t * l1e_out)
{
  pte_t l4e;
  pfn_t l3pfn;
  for (unsigned i = off; i < UMAP_L4PTES; i++)
    {
      if (umap == NULL)
	l4e = *get_cpumap_l4off (off);
      else
	l4e = umap->l4[off];
      if (pte_valid_table (l4e))
	{
	  l3pfn = pte_pfn (l4e);
	  if (scan_l3
	      (l3pfn, 0, l3off_out, l2off_out, l1off_out, l1p_out, l1e_out))
	    {
	      *l4off_out = i;
	      return true;
	    }
	}
    }
  return false;
}

uaddr_t
umap_next (struct hal_umap *umap, uaddr_t uaddr, hal_l1p_t * l1p_out,
	   hal_l1e_t * l1e_out)
{

  unsigned l4off = L4OFF (uaddr);
  unsigned l3off = L3OFF (uaddr);
  unsigned l2off = L2OFF (uaddr);
  unsigned l1off = L1OFF (uaddr);
  pte_t *l4ptr;
  pfn_t l1pfn, l2pfn, l3pfn;
  unsigned l4next, l3next, l2next, l1next;
  uaddr_t ret;

  ret = UADDR_INVALID;

  assert (L4OFF (uaddr) < UMAP_L4PTES);
  if (umap == NULL)
    l4ptr = get_cpumap_l4ptr (uaddr);
  else
    l4ptr = umap->l4 + L4OFF (uaddr);

  /* Check till end of current l1. */
  l1pfn = walk_l1pfn (l4ptr, uaddr, false);
  if (l1pfn != PFN_INVALID
      && scan_l1 (l1pfn, l1off + 1, &l1next, l1p_out, l1e_out))
    {
      ret = mkaddr (l4off, l3off, l2off, l1next);
      goto _next_exit;
    }

  /* Check till end of current l2. */
  l2pfn = walk_l2pfn (l4ptr, uaddr, false);
  if (l2pfn != PFN_INVALID
      && scan_l2 (l2pfn, l2off + 1, &l2next, &l1next, l1p_out, l1e_out))
    {
      ret = mkaddr (l4off, l3off, l2next, l1next);
      goto _next_exit;
    }

  /* Check till end of current l3. */
  l3pfn = walk_l3pfn (l4ptr, uaddr, false);
  if (l3pfn != PFN_INVALID
      && scan_l3 (l3pfn, l3off + 1, &l3next, &l2next, &l1next, l1p_out,
		  l1e_out))
    {
      ret = mkaddr (l4off, l3next, l2next, l1next);
      goto _next_exit;
    }

  /* Scan L4 until end of UMAP area. */
  if (scan_l4
      (umap, l4off + 1, &l4next, &l3next, &l2next, &l1next, l1p_out, l1e_out))
    {
      ret = mkaddr (l4next, l3next, l2next, l1next);
      goto _next_exit;
    }

_next_exit:
  if (umap == NULL)
    put_cpumap_l4ptr (uaddr, l4ptr);
  return ret;
}

void
umap_free (struct hal_umap *umap)
{
  pte_t l4e;
  pfn_t l3pfn;
  pte_t *l3ptr, l3e;
  pfn_t l2pfn;
  pte_t *l2ptr, l2e;
  pfn_t l1pfn;

  for (unsigned i = 0; i < UMAP_L4PTES; i++)
    {
      l4e = umap->l4[i];
      assert (!pte_valid_leaf (l4e));
      if (pte_valid_table (l4e))
	{
	  l3pfn = pte_pfn (l4e);
	  l3ptr = pfn_get (l3pfn);
	  for (unsigned i = 0; i < 512; i++)
	    {
	      l3e = l3ptr[i];
	      assert (!pte_valid_leaf (l3e));
	      if (pte_valid_table (l3e))
		{
		  l2pfn = pte_pfn (l3e);
		  l2ptr = pfn_get (l2pfn);
		  for (unsigned i = 0; i < 512; i++)
		    {
		      l2e = l2ptr[i];
		      assert (!pte_valid_leaf (l2e));
		      if (pte_valid_table (l2e))
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
	    }
	  printf ("Freeing L3 %lx\n", l3pfn);
	  pfn_put (l3pfn, l3ptr);
	  pfn_free (l3pfn);
	}
      umap->l4[i] = PTE_INVALID;
    }
}

unsigned long
umap_minaddr (void)
{
  return 0;
}

unsigned long
umap_maxaddr (void)
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
      pte_t *l4ptr, l4e;

      l4ptr = get_cpumap_l4ptr (va);
      l4e = *l4ptr;

      assert (!pte_valid_leaf (l4e));
      if (!pte_valid_table (l4e))
	{
	  l4e = alloc_table ();
	  /* We're in bootstrap. Can assert. */
	  assert (l4e != PTE_INVALID);
	  *l4ptr = l4e;
	  /* Not present to present. Only SVVPTC */
	  riscv_invlpg (va, true);
	}
      put_cpumap_l4ptr (va, l4ptr);
      umap->l4[i] = l4e;
    }

  /* Panic if the boot user mapping doesn't fit in a UMAP. */
  for (; i < 256; i++, va += (1L << L4_SHIFT))
    {
      pte_t *l4ptr = get_cpumap_l4ptr (va);
      if (pte_valid (*l4ptr))
	{
	  fatal ("Boot user mapping do not fit into UMAP");
	}
      put_cpumap_l4ptr (va, l4ptr);
    }
}

hal_tlbop_t
hal_umap_load (struct hal_umap *umap)
{
  vaddr_t va = hal_virtmem_userbase ();
  hal_tlbop_t tlbop = HAL_TLBOP_NONE;
  int i;

  for (i = 0; i < UMAP_L4PTES; i++, va += (1L << L4_SHIFT))
    {
      pte_t *l4ptr, oldl4e, newl4e;

      if (umap != NULL)
	newl4e = umap->l4[i];
      else
	newl4e = 0;

      l4ptr = get_cpumap_l4ptr (va);
      oldl4e = *l4ptr;
      *l4ptr = newl4e;
      put_cpumap_l4ptr (va, l4ptr);
      tlbop |= hal_l1e_tlbop (oldl4e, newl4e);
    }
  return tlbop;
}
