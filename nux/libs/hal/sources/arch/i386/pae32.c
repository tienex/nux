/** @file
  i386 PAE32 3-Level Page Table Implementation

  Implements 3-level page table walking, manipulation, and scanning for
  i386 architecture using PAE (Physical Address Extension). Manages both
  kernel linear mappings and user address space mappings for 32-bit mode.

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
#define L2_SHIFT (9 + PAGE_SHIFT)
#define L3_SHIFT (9 + 9 + PAGE_SHIFT)

#define L3OFF(_va) (((_va) >> L3_SHIFT) & 3)
#define L2OFF(_va) (((_va) >> L2_SHIFT) & 0x1ff)
#define L1OFF(_va) (((_va) >> L1_SHIFT) & 0x1ff)

/* The following RES definitions assume 48-bit MAX PA */
#define L2_RESPT 0x7FFF000000000000LL
#define L2_RES2M 0x7FFF0000001FE000LL
#define L1_RESPT 0x7FFF000000000000LL

#define l2e_bigpage(_pte) ((_pte) & PTE_PS)
#define l2e_reserved(_pte) ((_pte) & (l2e_bigpage(_pte) ? L2_RES2M : L2_RESPT))
#define l1e_reserved(_pte) ((_pte) & L1_RESPT)

#define mkpte(_p, _f) (((UINT64)(_p) << PAGE_SHIFT) | (_f))
#define pte_pfn(_p) ((_p & ~PTE_NX) >> PAGE_SHIFT)
#define pte_present(_pte) ((_pte) & PTE_P)

#define PTE_INVALID ((UINT64)0)
#define PTEP_INVALID L1P_INVALID
#define mkptep_cur(_p) ((ptep_t)(UINTN)(_p))
#define mkptep_fgn(_p) ((ptep_t)(UINTN)(_p) | 1)
#define ptep_is_foreign(_p) ((_p) & 1)

extern INT32 _linear_start;
extern INT32 _linear_l2table;
extern INT32 _linear_l3table;
CONST VIRTUAL_ADDRESS linaddr = (VIRTUAL_ADDRESS) & _linear_start;
CONST VIRTUAL_ADDRESS l2_linaddr = (VIRTUAL_ADDRESS) & _linear_l2table;
pte_t *l3_linaddr = (pte_t *) & _linear_l3table;

/**
  Construct virtual address from page table offsets.

  @param[in] L3off  L3 page table offset.
  @param[in] L2off  L2 page table offset.
  @param[in] L1off  L1 page table offset.

  @return Virtual address.
**/
UINT64
MakeAddress (
  IN UINT64  L3off,
  IN UINT64  L2off,
  IN UINT64  L1off
  )
{
  return (L3off << L3_SHIFT) | (L2off << L2_SHIFT) | (L1off << L1_SHIFT);
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
  Allocate L3 page table.

  @return Page table entry pointing to new table, or PTE_INVALID.
**/
static pte_t
AllocL3Table (
  VOID
  )
{
  PFN Pfn;

  Pfn = pfn_alloc (0);
  if (Pfn == PFN_INVALID)
    return PTE_INVALID;

  return mkpte (Pfn, PTE_P);
}

/**
  Allocate page table.

  @param[in] User  TRUE if user-accessible page table.

  @return Page table entry pointing to new table, or PTE_INVALID.
**/
static pte_t
AllocTable (
  IN BOOLEAN  User
  )
{
  PFN Pfn;

  Pfn = pfn_alloc (0);
  if (Pfn == PFN_INVALID)
    return PTE_INVALID;

  return mkpte (Pfn, PTE_P | PTE_W | (User ? PTE_U : 0));
}

/**
  Get L3 page table entry pointer for linear-mapped virtual address.

  @param[in] Va  Virtual address.

  @return L3 page table entry pointer.
**/
static ptep_t
LinMapGetL3p (
  IN unsigned long  Va
  )
{
  return mkptep_cur (l3_linaddr + L3OFF (Va));
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
  IN BOOLEAN           Alloc,
  IN BOOLEAN           User
  )
{
  ptep_t L3p;
  pte_t L3e;

  L3p = LinMapGetL3p (Va);
  L3e = GetPte (L3p);

  if (!pte_present (L3e))
    {
      if (!Alloc)
	return PTEP_INVALID;
      L3e = AllocL3Table ();
      if (L3e == PTE_INVALID)
	return PTEP_INVALID;
      SetPte (L3p, L3e);
      /* Not present, no TLB flush necessary. */
    }

  Va &= ~((1L << L2_SHIFT) - 1);
  return mkptep_cur (l2_linaddr + (Va >> 18));
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
  IN BOOLEAN           Alloc,
  IN BOOLEAN           User
  )
{
  ptep_t L2p;
  pte_t L2e;

  L2p = LinMapGetL2p (Va, Alloc, User);
  L2e = GetPte (L2p);

  if (!pte_present (L2e))
    {
      printf ("Not present!");
      if (!Alloc)
	return PTEP_INVALID;
      L2e = AllocTable (User);
      printf ("ALLOCATED %lx\n", L2e);
      if (L2e == PTE_INVALID)
	return PTEP_INVALID;
      SetPte (L2p, L2e);
      /* Not present, no TLB flush necessary. */
    }

  assert (!l2e_reserved (L2e) && "Invalid L2E.");
  assert (!l2e_bigpage (L2e) && "Invalid page size.");

  Va &= ~((1L << L1_SHIFT) - 1);
  return mkptep_cur (linaddr + (Va >> 9));
}

/**
  Get L3 page table entry pointer for user virtual address.

  @param[in] Umap  User address space map (NULL for current).
  @param[in] Va     Virtual address.

  @return L3 page table entry pointer.
**/
static ptep_t
GetUmapL3p (
  IN struct hal_umap  *Umap,
  IN unsigned long    Va
  )
{
  if (Umap == NULL)
    {
      /* Use PMAP if current. */
      assert (L3OFF (Va) < UMAP_L3PTES);
      return LinMapGetL3p (Va);
    }

  assert (L3OFF (Va) < UMAP_L3PTES);

  return mkptep_cur (Umap->l3 + L3OFF (Va));
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
  IN BOOLEAN             Alloc
  )
{
  ptep_t L3p;
  pte_t L3e;

  L3p = GetUmapL3p (Umap, Va);
  L3e = GetPte (L3p);

  if (!pte_present (L3e))
    {
      if (!Alloc)
	return PFN_INVALID;
      L3e = AllocL3Table ();
      if (L3e == PTE_INVALID)
	return PFN_INVALID;
      SetPte (L3p, L3e);
      /* Not present, no TLB flush necessary. */
    }

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
  IN BOOLEAN             Alloc
  )
{
  PFN L2Pfn;

  if (Umap == NULL)
    {
      /* Use PMAP if current. */
      assert (L3OFF (Va) < UMAP_L3PTES);
      return LinMapGetL2p (Va, Alloc, TRUE /* user */ );
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
  IN BOOLEAN             Alloc
  )
{
  ptep_t L2p;
  pte_t L2e;

  L2p = GetUmapL2p (Umap, Va, Alloc);
  L2e = GetPte (L2p);

  if (!pte_present (L2e))
    {
      if (!Alloc)
	return PFN_INVALID;
      L2e = AllocTable (TRUE /* user */ );
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
  IN BOOLEAN             Alloc
  )
{
  PFN L1Pfn;

  if (Umap == NULL)
    {
      /* Use PMAP if current. */
      assert (L3OFF (Va) < UMAP_L3PTES);
      return LinMapGetL1p (Va, Alloc, TRUE /* user */ );
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
  return 3L << L3_SHIFT;
}

/**
  Get L1 page table entry pointer for kernel virtual address.

  Note: This does not check for va being in user range.

  @param[in] Va     Virtual address.
  @param[in] Alloc  TRUE to allocate missing tables.

  @return L1 page table entry pointer, or L1P_INVALID.
**/
hal_l1p_t
KmapGetL1p (
  IN unsigned long  Va,
  IN BOOLEAN           Alloc
  )
{
  return (hal_l1p_t) LinMapGetL1p (Va, Alloc, FALSE /* !user */ );
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
  for (int i = 0; i < UMAP_L3PTES; i++)
    {
      Umap->l3[i] = 0;
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
  INT32 i;

  for (i = 0; i < UMAP_L3PTES; i++, Va += (1L << L3_SHIFT))
    {
      ptep_t L3p;
      pte_t L3e;

      L3p = LinMapGetL3p (Va);
      L3e = GetPte (L3p);

      if (!pte_present (L3e))
	{
	  L3e = AllocL3Table ();
	  /* We're in bootstrap. Can assert. */
	  assert (L3e != PTE_INVALID);
	  SetPte (L3p, L3e);
	  /* Not present, no TLB flush necessary. */
	}
      Umap->l3[i] = L3e & 0x0000fffffffff001LL;	/* Remove flags from PTE. */
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
  unsigned long LinOff = ((unsigned long) l3_linaddr & PAGE_MASK) >> 3;
  hal_tlbop_t TlbOp = HAL_TLBOP_NONE;
  PFN Cr3Pfn, KpdptePfn;
  pte_t *Pdptes, Kpdpte, *Kpd;

  /* Unfortunately, in PAE the linear mappings do not point to the
     root. */
  Cr3Pfn = read_cr3 () >> PAGE_SHIFT;
  Pdptes = (pte_t *) pfn_get (Cr3Pfn);
  for (int i = 0; i < UMAP_L3PTES; i++)
    Pdptes[i] = Umap == NULL ? 0 : Umap->l3[i];
  Kpdpte = Pdptes[3];
  pfn_put (Cr3Pfn, Pdptes);

  hal_l1e_unbox (Kpdpte, &KpdptePfn, NULL);
  assert (KpdptePfn != PFN_INVALID);
  Kpd = (pte_t *) pfn_get (KpdptePfn);
  for (int i = 0; i < UMAP_L3PTES; i++)
    {
      Kpd[LinOff + i] = Umap == NULL ? 0 : Umap->l3[i] | PTE_W;
    }
  Kpd[LinOff + 3] = Kpdpte | PTE_W;
  pfn_put (KpdptePfn, Kpd);

  TlbOp |= HAL_TLBOP_FLUSH;

  return TlbOp;
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
  UINT32 i;
  pte_t Pte;
  ptep_t Ptep;

  i = 3;
  Ptep = LinMapGetL3p (Va);
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
  Ptep = LinMapGetL2p (Va, FALSE, FALSE);
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
  Ptep = LinMapGetL1p (Va, FALSE, FALSE);
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
  Scan L1 page table for next mapped entry.

  @param[in]  L1Pfn     L1 page frame number.
  @param[in]  Off       Starting offset.
  @param[out] pL1OffOut Pointer to receive L1 offset.
  @param[out] pL1pOut   Pointer to receive L1 entry pointer.
  @param[out] pL1eOut   Pointer to receive L1 entry value.

  @retval TRUE   Found mapped entry.
  @retval FALSE  No mapped entry found.
**/
static BOOLEAN
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
  for (UINT32 i = Off; i < 512; i++)
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
	  return TRUE;
	}
    }
  pfn_put (L1Pfn, pL1Ptr);
  return FALSE;
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
static BOOLEAN
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
  for (UINT32 i = Off; i < 512; i++)
    {
      L2e = pL2Ptr[i];
      if (pte_present (L2e))
	{
	  L1Pfn = pte_pfn (L2e);
	  if (ScanL1 (L1Pfn, 0, pL1OffOut, pL1pOut, pL1eOut))
	    {
	      *pL2OffOut = i;
	      pfn_put (L2Pfn, pL2Ptr);
	      return TRUE;
	    }
	}
    }
  pfn_put (L2Pfn, pL2Ptr);
  return FALSE;
}

/**
  Scan L3 page table for next mapped entry.

  @param[in]  Umap     User address space map.
  @param[in]  Off       Starting offset.
  @param[out] pL3OffOut Pointer to receive L3 offset.
  @param[out] pL2OffOut Pointer to receive L2 offset.
  @param[out] pL1OffOut Pointer to receive L1 offset.
  @param[out] pL1pOut   Pointer to receive L1 entry pointer.
  @param[out] pL1eOut   Pointer to receive L1 entry value.

  @retval TRUE   Found mapped entry.
  @retval FALSE  No mapped entry found.
**/
static BOOLEAN
ScanL3 (
  IN  struct hal_umap  *Umap,
  IN  unsigned         Off,
  OUT unsigned        *pL3OffOut,
  OUT unsigned        *pL2OffOut,
  OUT unsigned        *pL1OffOut,
  OUT hal_l1p_t       *pL1pOut OPTIONAL,
  OUT hal_l1e_t       *pL1eOut OPTIONAL
  )
{
  pte_t L3e;
  PFN L2Pfn;
  for (UINT32 i = Off; i < UMAP_L3PTES; i++)
    {
      if (Umap == NULL)
	{
	  ptep_t L3p;

	  L3p = LinMapGetL3p (MakeAddress (i, 0, 0));
	  L3e = GetPte (L3p);
	}
      else
	L3e = Umap->l3[i];

      if (pte_present (L3e))
	{
	  L2Pfn = pte_pfn (L3e);
	  if (ScanL2 (L2Pfn, 0, pL2OffOut, pL1OffOut, pL1pOut, pL1eOut))
	    {
	      *pL3OffOut = i;
	      return TRUE;
	    }
	}
    }
  return FALSE;
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

  unsigned L3Off = L3OFF (Uaddr);
  unsigned L2Off = L2OFF (Uaddr);
  unsigned L1Off = L1OFF (Uaddr);
  PFN L1Pfn, L2Pfn;
  unsigned L3Next, L2Next, L1Next;

  /* Check till end of current l1. */
  L1Pfn = GetUmapL1Pfn (Umap, Uaddr, FALSE);
  if (L1Pfn != PFN_INVALID
      && ScanL1 (L1Pfn, L1Off + 1, &L1Next, pL1pOut, pL1eOut))
    {
      return MakeAddress (L3Off, L2Off, L1Next);
    }

  /* Check till end of current l2. */
  L2Pfn = GetUmapL2Pfn (Umap, Uaddr, FALSE);
  if (L2Pfn != PFN_INVALID
      && ScanL2 (L2Pfn, L2Off + 1, &L2Next, &L1Next, pL1pOut, pL1eOut))
    {
      return MakeAddress (L3Off, L2Next, L1Next);
    }

  if (ScanL3 (Umap, L3Off + 1, &L3Next, &L2Next, &L1Next, pL1pOut, pL1eOut))
    {
      return MakeAddress (L3Next, L2Next, L1Next);
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
  PFN L2Pfn, L1Pfn;
  pte_t L3e, *pL2Ptr, L2e;

  for (UINT32 i = 0; i < UMAP_L3PTES; i++)
    {
      L3e = Umap->l3[i];
      if (pte_present (L3e))
	{
	  L2Pfn = pte_pfn (L3e);
	  pL2Ptr = pfn_get (L2Pfn);
	  for (UINT32 i = 0; i < 512; i++)
	    {
	      L2e = pL2Ptr[i];
	      if (pte_present (L2e))
		{
		  L1Pfn = pte_pfn (L2e);
		  printf ("Freeing L1 %lx\n", L1Pfn);
		  pfn_free (L1Pfn);
		}
	    }
	  printf ("Freeing L2 %lx\n", L2Pfn);
	  pfn_free (L2Pfn);
	}
      Umap->l3[i] = PTE_INVALID;
    }
}

/**
  Initialize PAE32 page tables for Application Processor.

  Creates a new CR3 with kernel mappings copied from BSP.
**/
VOID
Pae32InitializeAp (
  VOID
  )
{
  unsigned long LinOff = ((unsigned long) l3_linaddr & PAGE_MASK) >> 3;
  pte_t *Va;
  PFN L2Pfn[4], L3Pfn;

  for (int i = 0; i < 4; i++)
    {
      L2Pfn[i] = pfn_alloc (0);
      assert (L2Pfn[i] != PFN_INVALID);
    }

  for (int i = 0; i < 4; i++)
    {
      Va = kva_physmap (ptob (L2Pfn[i]), PAGE_SIZE, HAL_PTE_W | HAL_PTE_P);
      memcpy (Va, (void *) (l2_linaddr + ((UINT32) i << 30 >> 18)),
	      PAGE_SIZE);
      kva_unmap (Va, PAGE_SIZE);
    }

  /* Third l2 is special, has kernel and linear mappings. */
  Va = kva_physmap (ptob (L2Pfn[2]), PAGE_SIZE, HAL_PTE_W | HAL_PTE_P);
  for (int i = 0; i < 4; i++)
    Va[LinOff + i] = mkpte (L2Pfn[i], PTE_P | PTE_W | PTE_U);
  kva_unmap (Va, PAGE_SIZE);

  /* Note: we allocate a full page for only 256 bytes. Could be optimized. */
  L3Pfn = pfn_alloc (0);
  assert (L3Pfn != PFN_INVALID);

  Va = kva_physmap (ptob (L3Pfn), PAGE_SIZE, HAL_PTE_W | HAL_PTE_P);
  for (int i = 0; i < 4; i++)
    Va[i] = mkpte (L2Pfn[i], HAL_PTE_P);
  kva_unmap (Va, PAGE_SIZE);

  write_cr3 (ptob (L3Pfn));
}

/**
  Initialize PAE32 page table subsystem.

  No-op for PAE32 as initialization is done elsewhere.
**/
VOID
Pae32Initialize (
  VOID
  )
{
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use MakeAddress instead **/
UINT64 mkaddr (UINT64 l3off, UINT64 l2off, UINT64 l1off) {
  return MakeAddress (l3off, l2off, l1off);
}

/** @deprecated Use GetPte instead **/
pte_t get_pte (ptep_t ptep) {
  return GetPte (ptep);
}

/** @deprecated Use SetPte instead **/
pte_t set_pte (ptep_t ptep, pte_t pte) {
  return SetPte (ptep, pte);
}

/** @deprecated Use AllocL3Table instead **/
static pte_t alloc_l3table () {
  return AllocL3Table ();
}

/** @deprecated Use AllocTable instead **/
static pte_t alloc_table (BOOLEAN user) {
  return AllocTable (user);
}

/** @deprecated Use LinMapGetL3p instead **/
static ptep_t linmap_get_l3p (unsigned INTN va) {
  return LinMapGetL3p (va);
}

/** @deprecated Use LinMapGetL2p instead **/
static ptep_t linmap_get_l2p (unsigned INTN va, BOOLEAN alloc, BOOLEAN user) {
  return LinMapGetL2p (va, alloc, user);
}

/** @deprecated Use LinMapGetL1p instead **/
static ptep_t linmap_get_l1p (unsigned INTN va, BOOLEAN alloc, BOOLEAN user) {
  return LinMapGetL1p (va, alloc, user);
}

/** @deprecated Use GetUmapL3p instead **/
static ptep_t get_umap_l3p (struct hal_umap *umap, unsigned INTN va) {
  return GetUmapL3p (umap, va);
}

/** @deprecated Use GetUmapL2Pfn instead **/
static PFN get_umap_l2pfn (struct hal_umap *umap, unsigned INTN va, BOOLEAN alloc) {
  return GetUmapL2Pfn (umap, va, alloc);
}

/** @deprecated Use GetUmapL2p instead **/
static ptep_t get_umap_l2p (struct hal_umap *umap, unsigned INTN va, BOOLEAN alloc) {
  return GetUmapL2p (umap, va, alloc);
}

/** @deprecated Use GetUmapL1Pfn instead **/
static PFN get_umap_l1pfn (struct hal_umap *umap, unsigned INTN va, BOOLEAN alloc) {
  return GetUmapL1Pfn (umap, va, alloc);
}

/** @deprecated Use UmapGetL1p instead **/
ptep_t umap_get_l1p (struct hal_umap *umap, unsigned INTN va, BOOLEAN alloc) {
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
hal_l1p_t kmap_get_l1p (unsigned INTN va, BOOLEAN alloc) {
  return KmapGetL1p (va, alloc);
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

/** @deprecated Use PtUmapDebugWalk instead **/
void pt_umap_debugwalk (struct hal_umap *umap, unsigned INTN va) {
  PtUmapDebugWalk (umap, va);
}

/** @deprecated Use ScanL1 instead **/
static BOOLEAN scan_l1 (PFN l1pfn, UINT32 off, unsigned *l1off_out, hal_l1p_t * l1p_out,
	 hal_l1e_t * l1e_out) {
  return ScanL1 (l1pfn, off, l1off_out, l1p_out, l1e_out);
}

/** @deprecated Use ScanL2 instead **/
static BOOLEAN scan_l2 (PFN l2pfn, UINT32 off, unsigned *l2off_out, unsigned *l1off_out,
	 hal_l1p_t * l1p_out, hal_l1e_t * l1e_out) {
  return ScanL2 (l2pfn, off, l2off_out, l1off_out, l1p_out, l1e_out);
}

/** @deprecated Use ScanL3 instead **/
static BOOLEAN scan_l3 (struct hal_umap *umap, UINT32 off,
	 unsigned *l3off_out, unsigned *l2off_out, unsigned *l1off_out,
	 hal_l1p_t * l1p_out, hal_l1e_t * l1e_out) {
  return ScanL3 (umap, off, l3off_out, l2off_out, l1off_out, l1p_out, l1e_out);
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

/** @deprecated Use Pae32InitializeAp instead **/
void pae32_init_ap (void) {
  Pae32InitializeAp ();
}

/** @deprecated Use Pae32Initialize instead **/
void pae32_init (void) {
  Pae32Initialize ();
}
