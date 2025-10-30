/** @file
  RISC-V Sv48 Page Table Implementation

  Implements 4-level page table walking and management for RISC-V Sv48
  virtual memory architecture. Provides functions for page table traversal,
  user address space management, and TLB operations.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <assert.h>
#include <hal/internal.h>

#define SATP_PFN_MASK ((1L << 44) - 1)

#define L1_SHIFT PAGE_SHIFT
#define L2_SHIFT (L1_SHIFT + 9)
#define L3_SHIFT (L2_SHIFT + 9)
#define L4_SHIFT (L3_SHIFT + 9)

#define L4OFF(_va) (((_va) >> L4_SHIFT) & 0x1ff)
#define L3OFF(_va) (((_va) >> L3_SHIFT) & 0x1ff)
#define L2OFF(_va) (((_va) >> L2_SHIFT) & 0x1ff)
#define L1OFF(_va) (((_va) >> L1_SHIFT) & 0x1ff)

#define MKCANON(_va) ((INT64)((_va) << 16) >> 16)
#define UNCANON(_va) ((_va) & ((1LL << 48) - 1))

#define PTE_INVALID ((UINT64)0)
#define PTEP_INVALID L1P_INVALID

/**
  Construct canonical virtual address from page table offsets.

  @param[in] L4off  Level 4 table offset.
  @param[in] L3off  Level 3 table offset.
  @param[in] L2off  Level 2 table offset.
  @param[in] L1off  Level 1 table offset.

  @return Canonical virtual address.
**/
static UINT64
MakeAddress (
  IN UINT64  L4off,
  IN UINT64  L3off,
  IN UINT64  L2off,
  IN UINT64  L1off
  )
{
  return
    MKCANON (((L4off << L4_SHIFT) | (L3off << L3_SHIFT) | (L2off << L2_SHIFT)
	      | (L1off << L1_SHIFT)));
}

/**
  Get current CPU's L4 page table PFN from SATP register.

  @return L4 page table PFN.
**/
static PFN
GetCpuMapL4Pfn (
  VOID
  )
{
  UINTN satp;

  satp = riscv_satp ();
  return (PFN) (satp & SATP_PFN_MASK);
}

/**
  Get pointer to L4 page table entry at offset.

  @param[in] Off  L4 table offset.

  @return Pointer to L4 page table entry.
**/
static PTE *
GetCpuMapL4Off (
  IN UINT32  Off
  )
{
  PTE *t;

  t = (PTE *) pfn_get (GetCpuMapL4Pfn ());

  assert (Off < 512);
  return t + Off;
}

/**
  Get pointer to L4 page table entry for virtual address.

  @param[in] Va  Virtual address.

  @return Pointer to L4 page table entry.
**/
static PTE *
GetCpuMapL4Ptr (
  IN UINTN  Va
  )
{

  return GetCpuMapL4Off (L4OFF (Va));
}

/**
  Release L4 page table entry pointer.

  @param[in] Va   Virtual address (unused).
  @param[in] Pte Pointer to L4 page table entry.
**/
static VOID
PutCpuMapL4Ptr (
  IN UINTN  Va,
  IN PTE    *Pte
  )
{
  pfn_put (GetCpuMapL4Pfn (), Pte);
}


/**
  Walk to L3 page table PFN, allocating if needed.

  @param[in] pL4Ptr  Pointer to L4 page table entry.
  @param[in] Va      Virtual address.
  @param[in] Alloc   TRUE to allocate missing tables.

  @return L3 page table PFN, or PFN_INVALID if not found/allocated.
**/
static PFN
WalkL3Pfn (
  IN PTE      *pL4Ptr,
  IN UINTN    Va,
  IN BOOLEAN  Alloc
  )
{
  PTE L4e;

  L4e = *pL4Ptr;

  if (!pte_valid (L4e))
    {
      if (!Alloc)
	return PFN_INVALID;
      L4e = alloc_table ();
      if (L4e == PTE_INVALID)
	return PFN_INVALID;
      *pL4Ptr = L4e;
      /* NP->P, NO SVVPTC ONLY */
      riscv_invlpg (Va, TRUE);
    }

  assert (pte_valid_table (L4e));

  return pte_pfn (L4e);
}

/**
  Walk to L3 page table pointer, allocating if needed.

  @param[in] pL4Ptr  Pointer to L4 page table entry.
  @param[in] Va      Virtual address.
  @param[in] Alloc   TRUE to allocate missing tables.

  @return L3 page table pointer, or PTEP_INVALID if not found/allocated.
**/
static PTEP
WalkL3p (
  IN PTE      *pL4Ptr,
  IN UINTN    Va,
  IN BOOLEAN  Alloc
  )
{
  PFN L3Pfn;

  L3Pfn = WalkL3Pfn (pL4Ptr, Va, Alloc);
  if (L3Pfn == PFN_INVALID)
    return PTEP_INVALID;

  return mkptep (L3Pfn, L3OFF (Va));
}

/**
  Walk to L2 page table PFN, allocating if needed.

  @param[in] pL4Ptr  Pointer to L4 page table entry.
  @param[in] Va      Virtual address.
  @param[in] Alloc   TRUE to allocate missing tables.

  @return L2 page table PFN, or PFN_INVALID if not found/allocated.
**/
static PFN
WalkL2Pfn (
  IN PTE      *pL4Ptr,
  IN UINTN    Va,
  IN BOOLEAN  Alloc
  )
{
  PTEP L3p;
  PTE L3e;

  L3p = WalkL3p (pL4Ptr, Va, Alloc);
  if (L3p == PTEP_INVALID)
    return PFN_INVALID;
  L3e = GetPte (L3p);

  if (!pte_valid (L3e))
    {
      if (!Alloc)
	return PFN_INVALID;
      L3e = alloc_table ();
      if (L3e == PTE_INVALID)
	return PFN_INVALID;
      SetPte (L3p, L3e);
      /* NP->P, NO SVVPTC ONLY */
      riscv_invlpg (Va, TRUE);
    }

  assert (pte_valid_table (L3e));

  return pte_pfn (L3e);
}

/**
  Walk to L2 page table pointer, allocating if needed.

  @param[in] pL4Ptr  Pointer to L4 page table entry.
  @param[in] Va      Virtual address.
  @param[in] Alloc   TRUE to allocate missing tables.

  @return L2 page table pointer, or PTEP_INVALID if not found/allocated.
**/
static PTEP
WalkL2p (
  IN PTE      *pL4Ptr,
  IN UINTN    Va,
  IN BOOLEAN  Alloc
  )
{
  PFN L2Pfn;

  L2Pfn = WalkL2Pfn (pL4Ptr, Va, Alloc);
  if (L2Pfn == PFN_INVALID)
    return PTEP_INVALID;

  return mkptep (L2Pfn, L2OFF (Va));
}

/**
  Walk to L1 page table PFN, allocating if needed.

  @param[in] pL4Ptr  Pointer to L4 page table entry.
  @param[in] Va      Virtual address.
  @param[in] Alloc   TRUE to allocate missing tables.

  @return L1 page table PFN, or PFN_INVALID if not found/allocated.
**/
static PFN
WalkL1Pfn (
  IN PTE      *pL4Ptr,
  IN UINTN    Va,
  IN BOOLEAN  Alloc
  )
{
  PTEP L2p;
  PTE L2e;

  L2p = WalkL2p (pL4Ptr, Va, Alloc);
  if (L2p == PTEP_INVALID)
    return PFN_INVALID;
  L2e = GetPte (L2p);

  if (!pte_valid (L2e))
    {
      if (!Alloc)
	return PFN_INVALID;
      L2e = alloc_table ();
      if (L2e == PTE_INVALID)
	return PFN_INVALID;
      SetPte (L2p, L2e);
      /* NP->P, NO SVVPTC ONLY */
      riscv_invlpg (Va, TRUE);
    }

  assert (pte_valid_table (L2e));

  return pte_pfn (L2e);
}

/**
  Walk to L1 page table pointer, allocating if needed.

  @param[in] pL4Ptr  Pointer to L4 page table entry.
  @param[in] Va      Virtual address.
  @param[in] Alloc   TRUE to allocate missing tables.

  @return L1 page table pointer, or PTEP_INVALID if not found/allocated.
**/
static PTEP
WalkL1p (
  IN PTE      *pL4Ptr,
  IN UINTN    Va,
  IN BOOLEAN  Alloc
  )
{
  PFN L1Pfn;

  L1Pfn = WalkL1Pfn (pL4Ptr, Va, Alloc);
  if (L1Pfn == PFN_INVALID)
    return PTEP_INVALID;

  return mkptep (L1Pfn, L1OFF (Va));
}

/**
  Get L1 page table pointer for CPU map virtual address.

  @param[in] Va     Virtual address.
  @param[in] Alloc  TRUE to allocate missing tables.

  @return L1 page table pointer, or L1P_INVALID if not found/allocated.
**/
hal_l1p_t
cpumap_get_l1p (
  IN UINTN  Va,
  IN INT32  Alloc
  )
{
  PTE *pL4Ptr;
  PTEP L1p;

  pL4Ptr = GetCpuMapL4Ptr (Va);
  L1p = WalkL1p (pL4Ptr, Va, Alloc);
  PutCpuMapL4Ptr (Va, pL4Ptr);

  return L1p;
}

/**
  Get L1 page table pointer for user map virtual address.

  @param[in] Umap  User address space map.
  @param[in] Va     Virtual address.
  @param[in] Alloc  TRUE to allocate missing tables.

  @return L1 page table pointer, or L1P_INVALID if not found/allocated.
**/
hal_l1p_t
UmapGetL1p (
  IN struct hal_umap  *Umap,
  IN UINTN            Va,
  IN BOOLEAN          Alloc
  )
{
  PTE *pL4Ptr;
  PTEP L1p;

  assert (L4OFF (Va) < UMAP_L4PTES);
  if (Umap == NULL)
    pL4Ptr = GetCpuMapL4Ptr (Va);
  else
    pL4Ptr = Umap->l4 + L4OFF (Va);


  L1p = WalkL1p (pL4Ptr, Va, Alloc);

  if (Umap == NULL)
    PutCpuMapL4Ptr (Va, pL4Ptr);

  return L1p;
}

/**
  Scan L1 page table for next mapped entry.

  @param[in]  L1Pfn     L1 page table PFN.
  @param[in]  Off       Starting offset.
  @param[out] pL1Off    Pointer to receive L1 offset.
  @param[out] pL1p      Pointer to receive L1 page table pointer.
  @param[out] pL1e      Pointer to receive L1 page table entry.

  @retval TRUE   Found mapped entry.
  @retval FALSE  No mapped entries found.
**/
static BOOLEAN
ScanL1 (
  IN  PFN        L1Pfn,
  IN  UINT32     Off,
  OUT UINT32     *pL1Off,
  OUT hal_l1p_t  *pL1p OPTIONAL,
  OUT hal_l1e_t  *pL1e OPTIONAL
  )
{
  PTE *pL1Ptr, L1e;

  pL1Ptr = pfn_get (L1Pfn);
  for (UINT32 i = Off; i < 512; i++)
    {
      L1e = pL1Ptr[i];
      if (L1e != 0)
	{
	  if (pL1p != NULL)
	    *pL1p = mkpte (L1Pfn, i);
	  if (pL1e != NULL)
	    *pL1e = L1e;
	  *pL1Off = i;
	  pfn_put (L1Pfn, pL1Ptr);
	  return TRUE;
	}
    }
  pfn_put (L1Pfn, pL1Ptr);
  return FALSE;
}

/**
  Scan L2 page table for next mapped entry.

  @param[in]  L2Pfn     L2 page table PFN.
  @param[in]  Off       Starting offset.
  @param[out] pL2Off    Pointer to receive L2 offset.
  @param[out] pL1Off    Pointer to receive L1 offset.
  @param[out] pL1p      Pointer to receive L1 page table pointer.
  @param[out] pL1e      Pointer to receive L1 page table entry.

  @retval TRUE   Found mapped entry.
  @retval FALSE  No mapped entries found.
**/
static BOOLEAN
ScanL2 (
  IN  PFN        L2Pfn,
  IN  UINT32     Off,
  OUT UINT32     *pL2Off,
  OUT UINT32     *pL1Off,
  OUT hal_l1p_t  *pL1p OPTIONAL,
  OUT hal_l1e_t  *pL1e OPTIONAL
  )
{
  PTE *pL2Ptr, L2e;
  PFN L1Pfn;

  pL2Ptr = pfn_get (L2Pfn);
  for (UINT32 i = Off; i < 512; i++)
    {
      L2e = pL2Ptr[i];
      if (pte_valid_table (L2e))
	{
	  L1Pfn = pte_pfn (L2e);
	  if (ScanL1 (L1Pfn, 0, pL1Off, pL1p, pL1e))
	    {
	      *pL2Off = i;
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

  @param[in]  L3Pfn     L3 page table PFN.
  @param[in]  Off       Starting offset.
  @param[out] pL3Off    Pointer to receive L3 offset.
  @param[out] pL2Off    Pointer to receive L2 offset.
  @param[out] pL1Off    Pointer to receive L1 offset.
  @param[out] pL1p      Pointer to receive L1 page table pointer.
  @param[out] pL1e      Pointer to receive L1 page table entry.

  @retval TRUE   Found mapped entry.
  @retval FALSE  No mapped entries found.
**/
static BOOLEAN
ScanL3 (
  IN  PFN        L3Pfn,
  IN  UINT32     Off,
  OUT UINT32     *pL3Off,
  OUT UINT32     *pL2Off,
  OUT UINT32     *pL1Off,
  OUT hal_l1p_t  *pL1p OPTIONAL,
  OUT hal_l1e_t  *pL1e OPTIONAL
  )
{
  PTE *pL3Ptr, L3e;
  PFN L2Pfn;

  pL3Ptr = pfn_get (L3Pfn);
  for (UINT32 i = Off; i < 512; i++)
    {
      L3e = pL3Ptr[i];
      if (pte_valid_table (L3e))
	{
	  L2Pfn = pte_pfn (L3e);
	  if (ScanL2 (L2Pfn, 0, pL2Off, pL1Off, pL1p, pL1e))
	    {
	      *pL3Off = i;
	      pfn_put (L3Pfn, pL3Ptr);
	      return TRUE;
	    }
	}
    }
  pfn_put (L3Pfn, pL3Ptr);
  return FALSE;
}

/**
  Scan L4 page table for next mapped entry.

  @param[in]  Umap     User address space map.
  @param[in]  Off       Starting offset.
  @param[out] pL4Off    Pointer to receive L4 offset.
  @param[out] pL3Off    Pointer to receive L3 offset.
  @param[out] pL2Off    Pointer to receive L2 offset.
  @param[out] pL1Off    Pointer to receive L1 offset.
  @param[out] pL1p      Pointer to receive L1 page table pointer.
  @param[out] pL1e      Pointer to receive L1 page table entry.

  @retval TRUE   Found mapped entry.
  @retval FALSE  No mapped entries found.
**/
static BOOLEAN
ScanL4 (
  IN  struct hal_umap  *Umap,
  IN  UINT32           Off,
  OUT UINT32           *pL4Off,
  OUT UINT32           *pL3Off,
  OUT UINT32           *pL2Off,
  OUT UINT32           *pL1Off,
  OUT hal_l1p_t        *pL1p OPTIONAL,
  OUT hal_l1e_t        *pL1e OPTIONAL
  )
{
  PTE L4e;
  PFN L3Pfn;
  for (UINT32 i = Off; i < UMAP_L4PTES; i++)
    {
      if (Umap == NULL)
	L4e = *GetCpuMapL4Off (Off);
      else
	L4e = Umap->l4[Off];
      if (pte_valid_table (L4e))
	{
	  L3Pfn = pte_pfn (L4e);
	  if (ScanL3
	      (L3Pfn, 0, pL3Off, pL2Off, pL1Off, pL1p, pL1e))
	    {
	      *pL4Off = i;
	      return TRUE;
	    }
	}
    }
  return FALSE;
}

/**
  Get next mapped user address and page table information.

  @param[in]  Umap  User address space map.
  @param[in]  Uaddr  Starting user address.
  @param[out] pL1p   Pointer to receive L1 page table pointer.
  @param[out] pL1e   Pointer to receive L1 page table entry.

  @return Next mapped user address, or UADDR_INVALID if none.
**/
USER_ADDRESS
PtUmapNext (
  IN  struct hal_umap  *Umap,
  IN  USER_ADDRESS          Uaddr,
  OUT hal_l1p_t        *pL1p OPTIONAL,
  OUT hal_l1e_t        *pL1e OPTIONAL
  )
{

  UINT32 L4off = L4OFF (Uaddr);
  UINT32 L3off = L3OFF (Uaddr);
  UINT32 L2off = L2OFF (Uaddr);
  UINT32 L1off = L1OFF (Uaddr);
  PTE *pL4Ptr;
  PFN L1Pfn, L2Pfn, L3Pfn;
  UINT32 L4next, L3next, L2next, L1next;
  USER_ADDRESS ret;

  ret = UADDR_INVALID;

  assert (L4OFF (Uaddr) < UMAP_L4PTES);
  if (Umap == NULL)
    pL4Ptr = GetCpuMapL4Ptr (Uaddr);
  else
    pL4Ptr = Umap->l4 + L4OFF (Uaddr);

  /* Check till end of current l1. */
  L1Pfn = WalkL1Pfn (pL4Ptr, Uaddr, FALSE);
  if (L1Pfn != PFN_INVALID
      && ScanL1 (L1Pfn, L1off + 1, &L1next, pL1p, pL1e))
    {
      ret = MakeAddress (L4off, L3off, L2off, L1next);
      goto _next_exit;
    }

  /* Check till end of current l2. */
  L2Pfn = WalkL2Pfn (pL4Ptr, Uaddr, FALSE);
  if (L2Pfn != PFN_INVALID
      && ScanL2 (L2Pfn, L2off + 1, &L2next, &L1next, pL1p, pL1e))
    {
      ret = MakeAddress (L4off, L3off, L2next, L1next);
      goto _next_exit;
    }

  /* Check till end of current l3. */
  L3Pfn = WalkL3Pfn (pL4Ptr, Uaddr, FALSE);
  if (L3Pfn != PFN_INVALID
      && ScanL3 (L3Pfn, L3off + 1, &L3next, &L2next, &L1next, pL1p,
		  pL1e))
    {
      ret = MakeAddress (L4off, L3next, L2next, L1next);
      goto _next_exit;
    }

  /* Scan L4 until end of UMAP area. */
  if (ScanL4
      (Umap, L4off + 1, &L4next, &L3next, &L2next, &L1next, pL1p, pL1e))
    {
      ret = MakeAddress (L4next, L3next, L2next, L1next);
      goto _next_exit;
    }

_next_exit:
  if (Umap == NULL)
    PutCpuMapL4Ptr (Uaddr, pL4Ptr);
  return ret;
}

/**
  Free all page tables in user address space.

  @param[in] Umap  User address space map to free.
**/
VOID
PtUmapFree (
  IN struct hal_umap  *Umap
  )
{
  PTE L4e;
  PFN L3Pfn;
  PTE *pL3Ptr, L3e;
  PFN L2Pfn;
  PTE *pL2Ptr, L2e;
  PFN L1Pfn;

  for (UINT32 i = 0; i < UMAP_L4PTES; i++)
    {
      L4e = Umap->l4[i];
      assert (!pte_valid_leaf (L4e));
      if (pte_valid_table (L4e))
	{
	  L3Pfn = pte_pfn (L4e);
	  pL3Ptr = pfn_get (L3Pfn);
	  for (UINT32 i = 0; i < 512; i++)
	    {
	      L3e = pL3Ptr[i];
	      assert (!pte_valid_leaf (L3e));
	      if (pte_valid_table (L3e))
		{
		  L2Pfn = pte_pfn (L3e);
		  pL2Ptr = pfn_get (L2Pfn);
		  for (UINT32 i = 0; i < 512; i++)
		    {
		      L2e = pL2Ptr[i];
		      assert (!pte_valid_leaf (L2e));
		      if (pte_valid_table (L2e))
			{
			  L1Pfn = pte_pfn (L2e);
			  printf ("Freeing L1 %lx\n", L1Pfn);
			  pfn_free (L1Pfn);
			}
		    }
		  printf ("Freeing L2 %lx\n", L2Pfn);
		  pfn_put (L2Pfn, pL2Ptr);
		  pfn_free (L2Pfn);
		}
	    }
	  printf ("Freeing L3 %lx\n", L3Pfn);
	  pfn_put (L3Pfn, pL3Ptr);
	  pfn_free (L3Pfn);
	}
      Umap->l4[i] = PTE_INVALID;
    }
}

/**
  Get minimum user address.

  @return Minimum user virtual address.
**/
UINTN
PtUmapMinAddr (
  VOID
  )
{
  return 0;
}

/**
  Get maximum user address.

  @return Maximum user virtual address.
**/
UINTN
PtUmapMaxAddr (
  VOID
  )
{
  return 1L << (39 + UMAP_LOG2_L4PTES);
}

/**
  Initialize user address space map.

  @param[in] Umap  User address space map to initialize.
**/
VOID
hal_umap_init (
  IN struct hal_umap  *Umap
  )
{
  for (INT32 i = 0; i < UMAP_L4PTES; i++)
    {
      Umap->l4[i] = alloc_table ();
    }
}

/**
  Bootstrap user address space from current mappings.

  @param[in] Umap  User address space map to bootstrap.
**/
VOID
hal_umap_bootstrap (
  IN struct hal_umap  *Umap
  )
{
  VIRTUAL_ADDRESS Va = hal_virtmem_userbase ();
  INT32 i;

  for (i = 0; i < UMAP_L4PTES; i++, Va += (1L << L4_SHIFT))
    {
      PTE *pL4Ptr, L4e;

      pL4Ptr = GetCpuMapL4Ptr (Va);
      L4e = *pL4Ptr;

      assert (!pte_valid_leaf (L4e));
      if (!pte_valid_table (L4e))
	{
	  L4e = alloc_table ();
	  /* We're in bootstrap. Can assert. */
	  assert (L4e != PTE_INVALID);
	  *pL4Ptr = L4e;
	  /* Not present to present. Only SVVPTC */
	  riscv_invlpg (Va, TRUE);
	}
      PutCpuMapL4Ptr (Va, pL4Ptr);
      Umap->l4[i] = L4e;
    }

  /* Panic if the boot user mapping doesn't fit in a UMAP. */
  for (; i < 256; i++, Va += (1L << L4_SHIFT))
    {
      PTE *pL4Ptr = GetCpuMapL4Ptr (Va);
      if (pte_valid (*pL4Ptr))
	{
	  fatal ("Boot user mapping do not fit into UMAP");
	}
      PutCpuMapL4Ptr (Va, pL4Ptr);
    }
}

/**
  Load user address space into CPU.

  @param[in] Umap  User address space map to load (NULL for kernel-only).

  @return Required TLB operation.
**/
hal_tlbop_t
hal_umap_load (
  IN struct hal_umap  *Umap OPTIONAL
  )
{
  VIRTUAL_ADDRESS Va = hal_virtmem_userbase ();
  hal_tlbop_t tlbop = HAL_TLBOP_NONE;
  INT32 i;

  for (i = 0; i < UMAP_L4PTES; i++, Va += (1L << L4_SHIFT))
    {
      PTE *pL4Ptr, OldL4e, NewL4e;

      if (Umap != NULL)
	NewL4e = Umap->l4[i];
      else
	NewL4e = 0;

      pL4Ptr = GetCpuMapL4Ptr (Va);
      OldL4e = *pL4Ptr;
      *pL4Ptr = NewL4e;
      PutCpuMapL4Ptr (Va, pL4Ptr);
      tlbop |= HalL1eTlbOp (OldL4e, NewL4e);
    }
  return tlbop;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use MakeAddress instead **/
static UINT64 mkaddr (UINT64 l4off, UINT64 l3off, UINT64 l2off, UINT64 l1off) {
  return MakeAddress (l4off, l3off, l2off, l1off);
}

/** @deprecated Use GetCpuMapL4Pfn instead **/
static PFN get_cpumap_l4pfn (VOID) {
  return GetCpuMapL4Pfn ();
}

/** @deprecated Use GetCpuMapL4Off instead **/
static pte_t *get_cpumap_l4off (UINT32 off) {
  return GetCpuMapL4Off (off);
}

/** @deprecated Use GetCpuMapL4Ptr instead **/
static pte_t *get_cpumap_l4ptr (unsigned INTN va) {
  return GetCpuMapL4Ptr (va);
}

/** @deprecated Use PutCpuMapL4Ptr instead **/
static VOID put_cpumap_l4ptr (unsigned INTN va, pte_t * pte) {
  PutCpuMapL4Ptr (va, pte);
}

/** @deprecated Use WalkL3Pfn instead **/
static PFN walk_l3pfn (pte_t * l4ptr, unsigned INTN va, BOOLEAN alloc) {
  return WalkL3Pfn (l4ptr, va, alloc);
}

/** @deprecated Use WalkL3p instead **/
static ptep_t walk_l3p (pte_t * l4ptr, unsigned INTN va, BOOLEAN alloc) {
  return WalkL3p (l4ptr, va, alloc);
}

/** @deprecated Use WalkL2Pfn instead **/
static PFN walk_l2pfn (pte_t * l4ptr, unsigned INTN va, BOOLEAN alloc) {
  return WalkL2Pfn (l4ptr, va, alloc);
}

/** @deprecated Use WalkL2p instead **/
static ptep_t walk_l2p (pte_t * l4ptr, unsigned INTN va, BOOLEAN alloc) {
  return WalkL2p (l4ptr, va, alloc);
}

/** @deprecated Use WalkL1Pfn instead **/
static PFN walk_l1pfn (pte_t * l4ptr, unsigned INTN va, BOOLEAN alloc) {
  return WalkL1Pfn (l4ptr, va, alloc);
}

/** @deprecated Use WalkL1p instead **/
static ptep_t walk_l1p (pte_t * l4ptr, unsigned INTN va, BOOLEAN alloc) {
  return WalkL1p (l4ptr, va, alloc);
}

/** @deprecated Use UmapGetL1p instead **/
hal_l1p_t umap_get_l1p (struct hal_umap *umap, unsigned INTN va, BOOLEAN alloc) {
  return UmapGetL1p (umap, va, alloc);
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
static BOOLEAN scan_l3 (PFN l3pfn, UINT32 off, unsigned *l3off_out, unsigned *l2off_out,
	 unsigned *l1off_out, hal_l1p_t * l1p_out, hal_l1e_t * l1e_out) {
  return ScanL3 (l3pfn, off, l3off_out, l2off_out, l1off_out, l1p_out, l1e_out);
}

/** @deprecated Use ScanL4 instead **/
static BOOLEAN scan_l4 (struct hal_umap *umap, UINT32 off, unsigned *l4off_out,
	 unsigned *l3off_out, unsigned *l2off_out, unsigned *l1off_out,
	 hal_l1p_t * l1p_out, hal_l1e_t * l1e_out) {
  return ScanL4 (umap, off, l4off_out, l3off_out, l2off_out, l1off_out, l1p_out, l1e_out);
}

/** @deprecated Use PtUmapNext instead **/
USER_ADDRESS pt_umap_next (struct hal_umap *umap, USER_ADDRESS uaddr, hal_l1p_t * l1p,
	   hal_l1e_t * l1e) {
  return PtUmapNext (umap, uaddr, l1p, l1e);
}

/** @deprecated Use PtUmapFree instead **/
VOID pt_umap_free (struct hal_umap *umap) {
  PtUmapFree (umap);
}

/** @deprecated Use PtUmapMinAddr instead **/
unsigned long pt_umap_minaddr (VOID) {
  return PtUmapMinAddr ();
}

/** @deprecated Use PtUmapMaxAddr instead **/
unsigned long pt_umap_maxaddr (VOID) {
  return PtUmapMaxAddr ();
}
