/** @file
  AMD64 PAE64 4-Level Page Table Implementation

  Implements 4-level page table walking, manipulation, and scanning for
  AMD64 architecture using PAE (Physical Address Extension) with 48-bit
  virtual addresses. Manages both kernel linear mappings and user address
  space mappings.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <nux/nux.h>
#include <nux/types.h>
#include <hal/internal.h>

#define L1_SHIFT PAGE_SHIFT
#define L2_SHIFT (L1_SHIFT + 9)
#define L3_SHIFT (L2_SHIFT + 9)
#define L4_SHIFT (L3_SHIFT + 9)

#define ROUND_UP(_x, _s) ((_x) + ((1L << _s) - 1))

#define L4OFF(_va) (((_va) >> L4_SHIFT) & 0x1ff)
#define L3OFF(_va) (((_va) >> L3_SHIFT) & 0x1ff)
#define L2OFF(_va) (((_va) >> L2_SHIFT) & 0x1ff)
#define L1OFF(_va) (((_va) >> L1_SHIFT) & 0x1ff)

#define MKCANON(_va) ((int64_t)((_va) << 16) >> 16)
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
#define pte_pfn(_p) ((PFN)(((uint64_t) (_p) & ~PTE_NX) >> PAGE_SHIFT))
#define pte_present(_pte) ((_pte) & PTE_P)

#define PTE_INVALID ((uint64_t)0)
#define PTEP_INVALID L1P_INVALID
#define mkptep_cur(_p) ((ptep_t)(UINTN)(_p))
#define mkptep_fgn(_p) ((ptep_t)(UINTN)(_p) | 1)
#define ptep_is_foreign(_p) ((_p) & 1)

extern int _linear_start;
static CONST pte_t *linaddr = (CONST pte_t *) &_linear_start;
static pte_t *linaddr_l2;
static pte_t *linaddr_l3;
static pte_t *linaddr_l4;

/**
  Construct canonical virtual address from page table offsets.

  @param[in] L4off  L4 page table offset.
  @param[in] L3off  L3 page table offset.
  @param[in] L2off  L2 page table offset.
  @param[in] L1off  L1 page table offset.

  @return Canonical virtual address.
**/
uint64_t
MakeAddress (
  IN uint64_t  L4off,
  IN uint64_t  L3off,
  IN uint64_t  L2off,
  IN uint64_t  L1off
  )
{
  return
    MKCANON (((L4off << L4_SHIFT) | (L3off << L3_SHIFT) | (L2off << L2_SHIFT)
	      | (L1off << L1_SHIFT)));
}

/**
  Get page table entry value.

  @param[in] Ptep  Page table entry pointer.

  @return Page table entry value.
**/
pte_t
GetPte (
  IN ptep_t  Ptep
  )
{
  if (ptep_is_foreign (Ptep))
    {
      pte_t *T, Pte;
      PFN Pfn;
      unsigned Offset;

      Pfn = Ptep >> PAGE_SHIFT;
      Offset = Ptep >> 3 & 0x1ff;

      T = (pte_t *) pfn_get (Pfn);
      Pte = T[Offset];
      pfn_put (Pfn, T);

      return Pte;
    }
  else
    {
      return *(pte_t *) (UINTN) Ptep;
    }

}

/**
  Set page table entry value.

  @param[in] Ptep  Page table entry pointer.
  @param[in] Pte   New page table entry value.

  @return Previous page table entry value.
**/
pte_t
SetPte (
  IN ptep_t  Ptep,
  IN pte_t   Pte
  )
{
  pte_t Old;

  if (ptep_is_foreign (Ptep))
    {
      pte_t *T;
      PFN Pfn;
      unsigned Offset;

      Pfn = Ptep >> PAGE_SHIFT;
      Offset = Ptep >> 3 & 0x1ff;

      T = (pte_t *) pfn_get (Pfn);
      Old = T[Offset];
      T[Offset] = Pte;
      pfn_put (Pfn, T);
    }
  else
    {
      Old = *(pte_t *) Ptep;
      *(pte_t *) Ptep = Pte;
    }

  return Old;
}

/**
  Allocate page table.

  @param[in] User  TRUE if user-accessible page table.

  @return Page table entry pointing to new table, or PTE_INVALID.
**/
static pte_t
AllocTable (
  IN bool  User
  )
{
  PFN Pfn;

  Pfn = pfn_alloc (0);
  if (Pfn == PFN_INVALID)
    return PTE_INVALID;

  return mkpte (Pfn, PTE_P | PTE_W | (User ? PTE_U : 0));
}

/**
  Get L4 page table entry pointer for linear-mapped virtual address.

  @param[in] Va  Virtual address.

  @return L4 page table entry pointer.
**/
static ptep_t
LinMapGetL4p (
  IN unsigned long  Va
  )
{
  return mkptep_cur (linaddr_l4 + (UNCANON (Va) >> L4_SHIFT));
}

/**
  Get L4 page table entry for linear-mapped virtual address.

  @param[in] Va  Virtual address.

  @return L4 page table entry value.
**/
static pte_t
LinMapGetL4e (
  IN unsigned long  Va
  )
{
  return *(pte_t *) (linaddr_l4 + (UNCANON (Va) >> L4_SHIFT));
}

/**
  Get L3 page table entry pointer for linear-mapped virtual address.

  @param[in] Va     Virtual address.
  @param[in] Alloc  TRUE to allocate missing tables.
  @param[in] User   TRUE if user-accessible table.

  @return L3 page table entry pointer, or PTEP_INVALID.
**/
static ptep_t
LinMapGetL3p (
  IN unsigned long  Va,
  IN bool           Alloc,
  IN bool           User
  )
{
  ptep_t L4p;
  pte_t L4e;

  L4p = LinMapGetL4p (Va);
  L4e = GetPte (L4p);

  if (!pte_present (L4e))
    {
      if (!Alloc)
	return PTEP_INVALID;
      L4e = AllocTable (User);
      if (L4e == PTE_INVALID)
	return PTEP_INVALID;
      SetPte (L4p, L4e);
      /* Not present, no TLB flush necessary. */
    }

  assert (!l4e_reserved (L4e) && "Invalid L4E.");

  return mkptep_cur (linaddr_l3 + (UNCANON (Va) >> L3_SHIFT));
}

/**
  Get L2 page table entry pointer for linear-mapped virtual address.

  @param[in] Va     Virtual address.
  @param[in] Alloc  TRUE to allocate missing tables.
  @param[in] User   TRUE if user-accessible table.

  @return L2 page table entry pointer, or PTEP_INVALID.
**/
static ptep_t
LinMapGetL2p (
  IN unsigned long  Va,
  IN bool           Alloc,
  IN bool           User
  )
{
  ptep_t L3p;
  pte_t L3e;

  L3p = LinMapGetL3p (Va, Alloc, User);
  if (L3p == PTEP_INVALID)
    return L3p;
  L3e = GetPte (L3p);

  if (!pte_present (L3e))
    {
      if (!Alloc)
	return PTEP_INVALID;
      L3e = AllocTable (User);
      if (L3e == PTE_INVALID)
	return PTEP_INVALID;
      SetPte (L3p, L3e);
      /* Not present, no TLB flush necessary. */
    }

  assert (!l3e_reserved (L3e) && "Invalid L3E.");
  assert (!l3e_bigpage (L3e) && "Invalid page size.");

  return mkptep_cur (linaddr_l2 + (UNCANON (Va) >> L2_SHIFT));
}

/**
  Get L1 page table entry pointer for linear-mapped virtual address.

  @param[in] Va     Virtual address.
  @param[in] Alloc  TRUE to allocate missing tables.
  @param[in] User   TRUE if user-accessible table.

  @return L1 page table entry pointer, or PTEP_INVALID.
**/
static ptep_t
LinMapGetL1p (
  IN unsigned long  Va,
  IN bool           Alloc,
  IN bool           User
  )
{
  ptep_t L2p;
  pte_t L2e;

  L2p = LinMapGetL2p (Va, Alloc, User);
  if (L2p == PTEP_INVALID)
    return L2p;
  L2e = GetPte (L2p);

  if (!pte_present (L2e))
    {
      if (!Alloc)
	return PTEP_INVALID;
      L2e = AllocTable (User);
      if (L2e == PTE_INVALID)
	return PTEP_INVALID;
      SetPte (L2p, L2e);
      /* Not present, no TLB flush necessary. */
    }

  assert (!l2e_reserved (L2e) && "Invalid L2E.");
  assert (!l2e_bigpage (L2e) && "Invalid page size.");

  return mkptep_cur (linaddr + (UNCANON (Va) >> L1_SHIFT));
}

/**
  Get L4 page table entry pointer for user virtual address.

  @param[in] Umap  User address space map (NULL for current).
  @param[in] Va     Virtual address.

  @return L4 page table entry pointer.
**/
static ptep_t
GetUmapL4p (
  IN struct hal_umap  *Umap,
  IN unsigned long    Va
  )
{
  assert (L4OFF (Va) < UMAP_L4PTES);

  if (Umap == NULL)
    {
      /* Use LINMAP if current. */
      assert (L4OFF (Va) < UMAP_L4PTES);
      return LinMapGetL4p (Va);
    }

  return mkptep_cur (Umap->l4 + L4OFF (Va));
}


/**
  Get L3 page frame number for user virtual address.

  @param[in] Umap  User address space map (NULL for current).
  @param[in] Va     Virtual address.
  @param[in] Alloc  TRUE to allocate missing tables.

  @return L3 page frame number, or PFN_INVALID.
**/
static PFN
GetUmapL3Pfn (
  IN struct hal_umap  *Umap,
  IN unsigned long    Va,
  IN bool             Alloc
  )
{
  ptep_t L4p;
  pte_t L4e;

  L4p = GetUmapL4p (Umap, Va);
  L4e = GetPte (L4p);

  if (!pte_present (L4e))
    {
      if (!Alloc)
	return PFN_INVALID;
      L4e = AllocTable (true /* user */ );
      if (L4e == PTE_INVALID)
	return PFN_INVALID;
      SetPte (L4p, L4e);
      /* Not present, no TLB flush necessary. */
    }

  assert (!l4e_reserved (L4e) && "Invalid L4E.");

  return pte_pfn (L4e);
}

/**
  Get L3 page table entry pointer for user virtual address.

  @param[in] Umap  User address space map (NULL for current).
  @param[in] Va     Virtual address.
  @param[in] Alloc  TRUE to allocate missing tables.

  @return L3 page table entry pointer, or PTEP_INVALID.
**/
static ptep_t
GetUmapL3p (
  IN struct hal_umap  *Umap,
  IN unsigned long    Va,
  IN bool             Alloc
  )
{
  PFN L3Pfn;

  if (Umap == NULL)
    {
      /* Use LINMAP if current. */
      assert (L4OFF (Va) < UMAP_L4PTES);
      return LinMapGetL3p (Va, Alloc, true /* user */ );
    }

  L3Pfn = GetUmapL3Pfn (Umap, Va, Alloc);
  if (L3Pfn == PFN_INVALID)
    {
      return PTEP_INVALID;
    }

  return mkptep_fgn ((L3Pfn << PAGE_SHIFT) + (L3OFF (Va) << 3));
}

/**
  Get L2 page frame number for user virtual address.

  @param[in] Umap  User address space map (NULL for current).
  @param[in] Va     Virtual address.
  @param[in] Alloc  TRUE to allocate missing tables.

  @return L2 page frame number, or PFN_INVALID.
**/
static PFN
GetUmapL2Pfn (
  IN struct hal_umap  *Umap,
  IN unsigned long    Va,
  IN bool             Alloc
  )
{
  ptep_t L3p;
  pte_t L3e;

  L3p = GetUmapL3p (Umap, Va, Alloc);
  if (L3p == PTEP_INVALID)
    return PFN_INVALID;
  L3e = GetPte (L3p);

  if (!pte_present (L3e))
    {
      if (!Alloc)
	return PFN_INVALID;
      L3e = AllocTable (true /* user */ );
      if (L3e == PTE_INVALID)
	return PFN_INVALID;
      SetPte (L3p, L3e);
      /* Not present, no TLB flush necessary. */
    }

  assert (!l3e_reserved (L3e) && "Invalid L3E.");
  assert (!l3e_bigpage (L3e) && "Invalid page size.");

  return pte_pfn (L3e);
}

/**
  Get L2 page table entry pointer for user virtual address.

  @param[in] Umap  User address space map (NULL for current).
  @param[in] Va     Virtual address.
  @param[in] Alloc  TRUE to allocate missing tables.

  @return L2 page table entry pointer, or PTEP_INVALID.
**/
static ptep_t
GetUmapL2p (
  IN struct hal_umap  *Umap,
  IN unsigned long    Va,
  IN bool             Alloc
  )
{
  PFN L2Pfn;

  if (Umap == NULL)
    {
      /* Use LINMAP if current. */
      assert (L4OFF (Va) < UMAP_L4PTES);
      return LinMapGetL2p (Va, Alloc, true /* user */ );
    }

  L2Pfn = GetUmapL2Pfn (Umap, Va, Alloc);
  if (L2Pfn == PFN_INVALID)
    {
      return PTEP_INVALID;
    }

  return mkptep_fgn ((L2Pfn << PAGE_SHIFT) + (L2OFF (Va) << 3));
}

/**
  Get L1 page frame number for user virtual address.

  @param[in] Umap  User address space map (NULL for current).
  @param[in] Va     Virtual address.
  @param[in] Alloc  TRUE to allocate missing tables.

  @return L1 page frame number, or PFN_INVALID.
**/
static PFN
GetUmapL1Pfn (
  IN struct hal_umap  *Umap,
  IN unsigned long    Va,
  IN bool             Alloc
  )
{
  ptep_t L2p;
  pte_t L2e;

  L2p = GetUmapL2p (Umap, Va, Alloc);
  if (L2p == PTEP_INVALID)
    return PFN_INVALID;
  L2e = GetPte (L2p);

  if (!pte_present (L2e))
    {
      if (!Alloc)
	return PFN_INVALID;
      L2e = AllocTable (true /* user */ );
      if (L2e == PTE_INVALID)
	return PFN_INVALID;
      SetPte (L2p, L2e);
      /* Not present, no TLB flush necessary. */
    }


  assert (!l2e_reserved (L2e) && "Invalid L2E.");
  assert (!l2e_bigpage (L2e) && "Invalid page size.");

  return pte_pfn (L2e);
}



/**
  Get L1 page table entry pointer for user virtual address.

  @param[in] Umap  User address space map (NULL for current).
  @param[in] Va     Virtual address.
  @param[in] Alloc  TRUE to allocate missing tables.

  @return L1 page table entry pointer, or PTEP_INVALID.
**/
ptep_t
UmapGetL1p (
  IN struct hal_umap  *Umap,
  IN unsigned long    Va,
  IN bool             Alloc
  )
{
  PFN L1Pfn;

  if (Umap == NULL)
    {
      /* Use LINMAP if current. */
      assert (L4OFF (Va) < UMAP_L4PTES);
      return LinMapGetL1p (Va, Alloc, true /* user */ );
    }

  L1Pfn = GetUmapL1Pfn (Umap, Va, Alloc);
  if (L1Pfn == PFN_INVALID)
    {
      return PTEP_INVALID;
    }
  return mkptep_fgn ((L1Pfn << PAGE_SHIFT) + (L1OFF (Va) << 3));
}

/**
  Get minimum user virtual address.

  @return Minimum user virtual address.
**/
unsigned long
PtUmapMinAddr (
  VOID
  )
{
  return 0;
}

/**
  Get maximum user virtual address.

  @return Maximum user virtual address.
**/
unsigned long
PtUmapMaxAddr (
  VOID
  )
{
  return 1L << (39 + UMAP_LOG2_L4PTES);
}

/**
  Get L1 page table entry pointer for kernel virtual address.

  Note: This does not check for va being in user range.

  @param[in] Va     Virtual address.
  @param[in] Alloc  Non-zero to allocate missing tables.

  @return L1 page table entry pointer, or L1P_INVALID.
**/
hal_l1p_t
KmapGetL1p (
  IN unsigned long  Va,
  IN int            Alloc
  )
{
  return (hal_l1p_t) LinMapGetL1p (Va, Alloc, false /* !user */ );
}

/**
  Debug walk page tables and print entries.

  @param[in] Umap  User address space map.
  @param[in] Va     Virtual address to walk.
**/
VOID
PtUmapDebugWalk (
  IN struct hal_umap  *Umap,
  IN unsigned long    Va
  )
{
  unsigned i;
  pte_t Pte;
  ptep_t Ptep;

  i = 4;
  Ptep = LinMapGetL4p (Va);
  printf ("    L%d -", i);
  if (Ptep == PTEP_INVALID)
    {
      printf (" <PTE PTR INVALID>\n\n");
      return;
    }
  Pte = GetPte (Ptep);
  printf (" <%lx>\n", Pte);
  if (!pte_present(Pte))
    {
      printf("\n");
      return;
    }

  i = 3;
  Ptep = LinMapGetL3p (Va, false, false);
  printf ("    L%d -", i);
  if (Ptep == PTEP_INVALID)
    {
      printf (" <PTE PTR INVALID>\n\n");
      return;
    }
  Pte = GetPte (Ptep);
  printf (" <%lx>\n", Pte);
  if (!pte_present(Pte))
    {
      printf("\n");
      return;
    }

  i = 2;
  Ptep = LinMapGetL2p (Va, false, false);
  printf ("    L%d -", i);
  if (Ptep == PTEP_INVALID)
    {
      printf (" <PTE PTR INVALID>\n\n");
      return;
    }
  Pte = GetPte (Ptep);
  printf (" <%lx>\n", Pte);
  if (!pte_present(Pte))
    {
      printf("\n");
      return;
    }

  i = 1;
  Ptep = LinMapGetL1p (Va, false, false);
  printf ("    L%d -", i);
  if (Ptep == PTEP_INVALID)
    {
      printf (" <PTE PTR INVALID>\n\n");
      return;
    }
  Pte = GetPte (Ptep);
  printf (" <%lx>\n", Pte);
  return;
}

/**
  Initialize user address space map.

  @param[in] Umap  User address space map to initialize.
**/
VOID
HalUmapInit (
  IN struct hal_umap  *Umap
  )
{
  for (int i = 0; i < UMAP_L4PTES; i++)
    {
      Umap->l4[i] = AllocTable (true);;
    }
}

/**
  Bootstrap user address space map from current page tables.

  @param[in] Umap  User address space map to bootstrap.
**/
VOID
HalUmapBootstrap (
  IN struct hal_umap  *Umap
  )
{
  VIRTUAL_ADDRESS Va = hal_virtmem_userbase ();
  int i;

  for (i = 0; i < UMAP_L4PTES; i++, Va += (1L << L4_SHIFT))
    {
      ptep_t L4p;
      pte_t L4e;

      L4p = LinMapGetL4p (Va);
      L4e = GetPte (L4p);

      if (!pte_present (L4e))
	{
	  L4e = AllocTable (true);
	  /* We're in bootstrap. Can assert. */
	  assert (L4e != PTE_INVALID);
	  SetPte (L4p, L4e);
	  /* Not present, no TLB flush necessary. */
	}
      Umap->l4[i] = L4e;
    }

  /* Panic if the boot user mapping doesn't fit in a UMAP. */
  for (; i < 256; i++, Va += (1L << L4_SHIFT))
    if (pte_present (LinMapGetL4e (Va)))
      {
	halfatal ("Boot user mapping do not fit into UMAP");
      }
}

/**
  Load user address space map into current page tables.

  @param[in] Umap  User address space map to load (NULL for none).

  @return Required TLB operation.
**/
hal_tlbop_t
HalUmapLoad (
  IN struct hal_umap  *Umap
  )
{
  VIRTUAL_ADDRESS Va = hal_virtmem_userbase ();
  hal_tlbop_t TlbOp = HAL_TLBOP_NONE;
  int i;

  for (i = 0; i < UMAP_L4PTES; i++, Va += (1L << L4_SHIFT))
    {
      ptep_t L4p;
      pte_t OldL4e, NewL4e;

      if (Umap != NULL)
	NewL4e = Umap->l4[i];
      else
	NewL4e = 0;

      L4p = LinMapGetL4p (Va);
      OldL4e = SetPte (L4p, NewL4e);
      TlbOp |= hal_l1e_tlbop (OldL4e, NewL4e);
    }
  return TlbOp;
}

/**
  Scan L1 page table for next mapped entry.

  @param[in]  L1Pfn     L1 page frame number.
  @param[in]  Off       Starting offset.
  @param[out] pL1OffOut Pointer to receive L1 offset.
  @param[out] pL1pOut   Pointer to receive L1 entry pointer.
  @param[out] pL1eOut   Pointer to receive L1 entry value.

  @retval TRUE   Found mapped entry.
  @retval FALSE  No mapped entry found.
**/
static bool
ScanL1 (
  IN  PFN       L1Pfn,
  IN  unsigned    Off,
  OUT unsigned   *pL1OffOut,
  OUT hal_l1p_t  *pL1pOut OPTIONAL,
  OUT hal_l1e_t  *pL1eOut OPTIONAL
  )
{
  pte_t *pL1Ptr, L1e;

  pL1Ptr = pfn_get (L1Pfn);
  for (unsigned i = Off; i < 512; i++)
    {
      L1e = pL1Ptr[i];
      if (L1e != 0)
	{
	  if (pL1pOut != NULL)
	    *pL1pOut = mkptep_fgn ((L1Pfn << PAGE_SHIFT) + (i << 3));
	  if (pL1eOut != NULL)
	    *pL1eOut = L1e;
	  *pL1OffOut = i;
	  pfn_put (L1Pfn, pL1Ptr);
	  return true;
	}
    }
  pfn_put (L1Pfn, pL1Ptr);
  return false;
}

/**
  Scan L2 page table for next mapped entry.

  @param[in]  L2Pfn     L2 page frame number.
  @param[in]  Off       Starting offset.
  @param[out] pL2OffOut Pointer to receive L2 offset.
  @param[out] pL1OffOut Pointer to receive L1 offset.
  @param[out] pL1pOut   Pointer to receive L1 entry pointer.
  @param[out] pL1eOut   Pointer to receive L1 entry value.

  @retval TRUE   Found mapped entry.
  @retval FALSE  No mapped entry found.
**/
static bool
ScanL2 (
  IN  PFN       L2Pfn,
  IN  unsigned    Off,
  OUT unsigned   *pL2OffOut,
  OUT unsigned   *pL1OffOut,
  OUT hal_l1p_t  *pL1pOut OPTIONAL,
  OUT hal_l1e_t  *pL1eOut OPTIONAL
  )
{
  pte_t *pL2Ptr, L2e;
  PFN L1Pfn;

  pL2Ptr = pfn_get (L2Pfn);
  for (unsigned i = Off; i < 512; i++)
    {
      L2e = pL2Ptr[i];
      if (pte_present (L2e))
	{
	  L1Pfn = pte_pfn (L2e);
	  if (ScanL1 (L1Pfn, 0, pL1OffOut, pL1pOut, pL1eOut))
	    {
	      *pL2OffOut = i;
	      pfn_put (L2Pfn, pL2Ptr);
	      return true;
	    }
	}
    }
  pfn_put (L2Pfn, pL2Ptr);
  return false;
}

/**
  Scan L3 page table for next mapped entry.

  @param[in]  L3Pfn     L3 page frame number.
  @param[in]  Off       Starting offset.
  @param[out] pL3OffOut Pointer to receive L3 offset.
  @param[out] pL2OffOut Pointer to receive L2 offset.
  @param[out] pL1OffOut Pointer to receive L1 offset.
  @param[out] pL1pOut   Pointer to receive L1 entry pointer.
  @param[out] pL1eOut   Pointer to receive L1 entry value.

  @retval TRUE   Found mapped entry.
  @retval FALSE  No mapped entry found.
**/
static bool
ScanL3 (
  IN  PFN       L3Pfn,
  IN  unsigned    Off,
  OUT unsigned   *pL3OffOut,
  OUT unsigned   *pL2OffOut,
  OUT unsigned   *pL1OffOut,
  OUT hal_l1p_t  *pL1pOut OPTIONAL,
  OUT hal_l1e_t  *pL1eOut OPTIONAL
  )
{
  pte_t *pL3Ptr, L3e;
  PFN L2Pfn;

  pL3Ptr = pfn_get (L3Pfn);
  for (unsigned i = Off; i < 512; i++)
    {
      L3e = pL3Ptr[i];
      if (pte_present (L3e))
	{
	  L2Pfn = pte_pfn (L3e);
	  if (ScanL2 (L2Pfn, 0, pL2OffOut, pL1OffOut, pL1pOut, pL1eOut))
	    {
	      *pL3OffOut = i;
	      pfn_put (L3Pfn, pL3Ptr);
	      return true;
	    }
	}
    }
  pfn_put (L3Pfn, pL3Ptr);
  return false;
}

/**
  Scan L4 page table for next mapped entry.

  @param[in]  Umap     User address space map.
  @param[in]  Off       Starting offset.
  @param[out] pL4OffOut Pointer to receive L4 offset.
  @param[out] pL3OffOut Pointer to receive L3 offset.
  @param[out] pL2OffOut Pointer to receive L2 offset.
  @param[out] pL1OffOut Pointer to receive L1 offset.
  @param[out] pL1pOut   Pointer to receive L1 entry pointer.
  @param[out] pL1eOut   Pointer to receive L1 entry value.

  @retval TRUE   Found mapped entry.
  @retval FALSE  No mapped entry found.
**/
static bool
ScanL4 (
  IN  struct hal_umap  *Umap,
  IN  unsigned         Off,
  OUT unsigned        *pL4OffOut,
  OUT unsigned        *pL3OffOut,
  OUT unsigned        *pL2OffOut,
  OUT unsigned        *pL1OffOut,
  OUT hal_l1p_t       *pL1pOut OPTIONAL,
  OUT hal_l1e_t       *pL1eOut OPTIONAL
  )
{
  pte_t L4e;
  PFN L3Pfn;
  for (unsigned i = Off; i < UMAP_L4PTES; i++)
    {
      if (Umap != NULL)
	L4e = Umap->l4[i];
      else
	L4e = LinMapGetL4e (MakeAddress (i, 0, 0, 0));

      if (pte_present (L4e))
	{
	  L3Pfn = pte_pfn (L4e);
	  if (ScanL3
	      (L3Pfn, 0, pL3OffOut, pL2OffOut, pL1OffOut, pL1pOut, pL1eOut))
	    {
	      *pL4OffOut = i;
	      return true;
	    }
	}
    }
  return false;
}

/**
  Get next mapped user address.

  @param[in]  Umap   User address space map.
  @param[in]  Uaddr   Starting user address.
  @param[out] pL1pOut Pointer to receive L1 entry pointer.
  @param[out] pL1eOut Pointer to receive L1 entry value.

  @return Next mapped user address, or UADDR_INVALID.
**/
USER_ADDRESS
PtUmapNext (
  IN  struct hal_umap  *Umap,
  IN  USER_ADDRESS          Uaddr,
  OUT hal_l1p_t        *pL1pOut OPTIONAL,
  OUT hal_l1e_t        *pL1eOut OPTIONAL
  )
{

  unsigned L4Off = L4OFF (Uaddr);
  unsigned L3Off = L3OFF (Uaddr);
  unsigned L2Off = L2OFF (Uaddr);
  unsigned L1Off = L1OFF (Uaddr);
  PFN L1Pfn, L2Pfn, L3Pfn;
  unsigned L4Next, L3Next, L2Next, L1Next;

  /* Check till end of current l1. */
  L1Pfn = GetUmapL1Pfn (Umap, Uaddr, false);
  if (L1Pfn != PFN_INVALID
      && ScanL1 (L1Pfn, L1Off + 1, &L1Next, pL1pOut, pL1eOut))
    {
      return MakeAddress (L4Off, L3Off, L2Off, L1Next);
    }

  /* Check till end of current l2. */
  L2Pfn = GetUmapL2Pfn (Umap, Uaddr, false);
  if (L2Pfn != PFN_INVALID
      && ScanL2 (L2Pfn, L2Off + 1, &L2Next, &L1Next, pL1pOut, pL1eOut))
    {
      return MakeAddress (L4Off, L3Off, L2Next, L1Next);
    }

  /* Check till end of current l3. */
  L3Pfn = GetUmapL3Pfn (Umap, Uaddr, false);
  if (L3Pfn != PFN_INVALID
      && ScanL3 (L3Pfn, L3Off + 1, &L3Next, &L2Next, &L1Next, pL1pOut,
		  pL1eOut))
    {
      return MakeAddress (L4Off, L3Next, L2Next, L1Next);
    }

  /* Scan L4 until end of UMAP area. */
  if (ScanL4
      (Umap, L4Off + 1, &L4Next, &L3Next, &L2Next, &L1Next, pL1pOut, pL1eOut))
    {
      return MakeAddress (L4Next, L3Next, L2Next, L1Next);
    }

  return UADDR_INVALID;
}

/**
  Free user address space page tables.

  @param[in] Umap  User address space map to free.
**/
VOID
PtUmapFree (
  IN struct hal_umap  *Umap
  )
{
  pte_t L4e;
  PFN L3Pfn;
  pte_t *pL3Ptr, L3e;
  PFN L2Pfn;
  pte_t *pL2Ptr, L2e;
  PFN L1Pfn;

  for (unsigned i = 0; i < UMAP_L4PTES; i++)
    {
      L4e = Umap->l4[i];
      if (pte_present (L4e))
	{
	  L3Pfn = pte_pfn (L4e);
	  pL3Ptr = pfn_get (L3Pfn);
	  for (unsigned i = 0; i < 512; i++)
	    {
	      L3e = pL3Ptr[i];
	      if (pte_present (L3e))
		{
		  L2Pfn = pte_pfn (L3e);
		  pL2Ptr = pfn_get (L2Pfn);
		  for (unsigned i = 0; i < 512; i++)
		    {
		      L2e = pL2Ptr[i];
		      if (pte_present (L2e))
			{
			  L1Pfn = pte_pfn (L2e);
			  pfn_free (L1Pfn);
			}
		    }
		  pfn_put (L2Pfn, pL2Ptr);
		  pfn_free (L2Pfn);
		}
	    }
	  pfn_put (L3Pfn, pL3Ptr);
	  pfn_free (L3Pfn);
	}
      Umap->l4[i] = PTE_INVALID;
    }
}

/**
  Initialize PAE64 page tables for Application Processor.

  Creates a new CR3 with kernel mappings copied from BSP.

  @param[in] Esp  Stack pointer (unused).
**/
VOID
Pae64InitializeAp (
  VOID
  )
{
  unsigned long LinOff = L4OFF ((unsigned long) linaddr);
  pte_t *Va, *pCr3Va;
  PFN Pfn, Cr3Pfn;


  Pfn = pfn_alloc (0);
  assert (Pfn != PFN_INVALID);
  Cr3Pfn = btop (read_cr3 ());

  /* Copy the kernel mappings. */
  Va = kva_physmap (ptob (Pfn), PAGE_SIZE, HAL_PTE_W | HAL_PTE_P);
  pCr3Va = kva_physmap (ptob (Cr3Pfn), PAGE_SIZE, HAL_PTE_P);
  memcpy (Va, pCr3Va, PAGE_SIZE);
  Va[LinOff] = mkpte (Pfn, PTE_P | PTE_W);	/* Point the linaddr back at itself. */
  kva_unmap (Va, PAGE_SIZE / 2);
  kva_unmap (pCr3Va, PAGE_SIZE / 2);

  write_cr3 (ptob (Pfn));
}

/**
  Initialize PAE64 page table subsystem.

  Sets up linear mapping addresses for all levels.
**/
VOID
Pae64Initialize (
  VOID
  )
{
  unsigned long LinOff = L4OFF ((unsigned long) linaddr);
  linaddr_l2 = (pte_t *) MakeAddress (LinOff, LinOff, 0, 0);
  linaddr_l3 = (pte_t *) MakeAddress (LinOff, LinOff, LinOff, 0);
  linaddr_l4 = (pte_t *) MakeAddress (LinOff, LinOff, LinOff, LinOff);
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use MakeAddress instead **/
uint64_t mkaddr (uint64_t l4off, uint64_t l3off, uint64_t l2off, uint64_t l1off) {
  return MakeAddress (l4off, l3off, l2off, l1off);
}

/** @deprecated Use GetPte instead **/
pte_t get_pte (ptep_t ptep) {
  return GetPte (ptep);
}

/** @deprecated Use SetPte instead **/
pte_t set_pte (ptep_t ptep, pte_t pte) {
  return SetPte (ptep, pte);
}

/** @deprecated Use AllocTable instead **/
static pte_t alloc_table (bool user) {
  return AllocTable (user);
}

/** @deprecated Use LinMapGetL4p instead **/
static ptep_t linmap_get_l4p (unsigned long va) {
  return LinMapGetL4p (va);
}

/** @deprecated Use LinMapGetL4e instead **/
static pte_t linmap_get_l4e (unsigned long va) {
  return LinMapGetL4e (va);
}

/** @deprecated Use LinMapGetL3p instead **/
static ptep_t linmap_get_l3p (unsigned long va, bool alloc, bool user) {
  return LinMapGetL3p (va, alloc, user);
}

/** @deprecated Use LinMapGetL2p instead **/
static ptep_t linmap_get_l2p (unsigned long va, bool alloc, bool user) {
  return LinMapGetL2p (va, alloc, user);
}

/** @deprecated Use LinMapGetL1p instead **/
static ptep_t linmap_get_l1p (unsigned long va, bool alloc, bool user) {
  return LinMapGetL1p (va, alloc, user);
}

/** @deprecated Use GetUmapL4p instead **/
static ptep_t get_umap_l4p (struct hal_umap *umap, unsigned long va) {
  return GetUmapL4p (umap, va);
}

/** @deprecated Use GetUmapL3Pfn instead **/
static PFN get_umap_l3pfn (struct hal_umap *umap, unsigned long va, bool alloc) {
  return GetUmapL3Pfn (umap, va, alloc);
}

/** @deprecated Use GetUmapL3p instead **/
static ptep_t get_umap_l3p (struct hal_umap *umap, unsigned long va, bool alloc) {
  return GetUmapL3p (umap, va, alloc);
}

/** @deprecated Use GetUmapL2Pfn instead **/
static PFN get_umap_l2pfn (struct hal_umap *umap, unsigned long va, bool alloc) {
  return GetUmapL2Pfn (umap, va, alloc);
}

/** @deprecated Use GetUmapL2p instead **/
static ptep_t get_umap_l2p (struct hal_umap *umap, unsigned long va, bool alloc) {
  return GetUmapL2p (umap, va, alloc);
}

/** @deprecated Use GetUmapL1Pfn instead **/
static PFN get_umap_l1pfn (struct hal_umap *umap, unsigned long va, bool alloc) {
  return GetUmapL1Pfn (umap, va, alloc);
}

/** @deprecated Use UmapGetL1p instead **/
ptep_t umap_get_l1p (struct hal_umap *umap, unsigned long va, bool alloc) {
  return UmapGetL1p (umap, va, alloc);
}

/** @deprecated Use PtUmapMinAddr instead **/
unsigned long pt_umap_minaddr (void) {
  return PtUmapMinAddr ();
}

/** @deprecated Use PtUmapMaxAddr instead **/
unsigned long pt_umap_maxaddr (void) {
  return PtUmapMaxAddr ();
}

/** @deprecated Use KmapGetL1p instead **/
hal_l1p_t kmap_get_l1p (unsigned long va, int alloc) {
  return KmapGetL1p (va, alloc);
}

/** @deprecated Use PtUmapDebugWalk instead **/
void pt_umap_debugwalk (struct hal_umap *umap, unsigned long va) {
  PtUmapDebugWalk (umap, va);
}

/** @deprecated Use HalUmapInit instead **/
void hal_umap_init (struct hal_umap *umap) {
  HalUmapInit (umap);
}

/** @deprecated Use HalUmapBootstrap instead **/
void hal_umap_bootstrap (struct hal_umap *umap) {
  HalUmapBootstrap (umap);
}

/** @deprecated Use HalUmapLoad instead **/
hal_tlbop_t hal_umap_load (struct hal_umap *umap) {
  return HalUmapLoad (umap);
}

/** @deprecated Use ScanL1 instead **/
static bool scan_l1 (PFN l1pfn, unsigned off, unsigned *l1off_out, hal_l1p_t * l1p_out,
	 hal_l1e_t * l1e_out) {
  return ScanL1 (l1pfn, off, l1off_out, l1p_out, l1e_out);
}

/** @deprecated Use ScanL2 instead **/
static bool scan_l2 (PFN l2pfn, unsigned off, unsigned *l2off_out, unsigned *l1off_out,
	 hal_l1p_t * l1p_out, hal_l1e_t * l1e_out) {
  return ScanL2 (l2pfn, off, l2off_out, l1off_out, l1p_out, l1e_out);
}

/** @deprecated Use ScanL3 instead **/
static bool scan_l3 (PFN l3pfn, unsigned off, unsigned *l3off_out, unsigned *l2off_out,
	 unsigned *l1off_out, hal_l1p_t * l1p_out, hal_l1e_t * l1e_out) {
  return ScanL3 (l3pfn, off, l3off_out, l2off_out, l1off_out, l1p_out, l1e_out);
}

/** @deprecated Use ScanL4 instead **/
static bool scan_l4 (struct hal_umap *umap, unsigned off, unsigned *l4off_out,
	 unsigned *l3off_out, unsigned *l2off_out, unsigned *l1off_out,
	 hal_l1p_t * l1p_out, hal_l1e_t * l1e_out) {
  return ScanL4 (umap, off, l4off_out, l3off_out, l2off_out, l1off_out, l1p_out, l1e_out);
}

/** @deprecated Use PtUmapNext instead **/
USER_ADDRESS pt_umap_next (struct hal_umap *umap, USER_ADDRESS uaddr, hal_l1p_t * l1p_out,
	      hal_l1e_t * l1e_out) {
  return PtUmapNext (umap, uaddr, l1p_out, l1e_out);
}

/** @deprecated Use PtUmapFree instead **/
void pt_umap_free (struct hal_umap *umap) {
  PtUmapFree (umap);
}

/** @deprecated Use Pae64InitializeAp instead **/
void pae64_init_ap (void) {
  Pae64InitializeAp ();
}

/** @deprecated Use Pae64Initialize instead **/
void pae64_init (void) {
  Pae64Initialize ();
}
