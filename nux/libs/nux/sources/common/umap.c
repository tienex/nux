/** @file
  NUX User Address Space Mapping Operations

  Provides low-level user virtual address to physical page mapping
  operations. These functions are unlocked - caller must ensure no
  concurrent operations on the same umap. Call UmapCommit() to
  synchronize TLB changes across CPUs.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <nux/internal.h>
#include <assert.h>
#include <nux/nux.h>

/**
  Set L1 page table entry for user mapping.

  Internal helper to modify a user page table entry with optional
  page table allocation. Updates TLB operation flags atomically.

  @param[in]  Umap  User address space map.
  @param[in]  Va     Virtual address to modify.
  @param[in]  L1e    New L1 page table entry value.
  @param[in]  Alloc  TRUE to allocate page tables if needed.
  @param[out] OldPfn Optional pointer to receive previous PFN.

  @retval TRUE   Page table entry was successfully set.
  @retval FALSE  Failed to get page table pointer (no allocation).
**/
static BOOLEAN
UmapSetL1e (
  IN  struct umap  *Umap,
  IN  vaddr_t      Va,
  IN  hal_l1e_t    L1e,
  IN  BOOLEAN      Alloc,
  OUT pfn_t        *OldPfn OPTIONAL
  )
{
  hal_l1p_t L1p;
  hal_l1e_t OldL1e;
  pfn_t OldPfn;
  UINT32 OldProt;
  struct hal_umap *Hal;

  Hal = (Umap == CpuGetCurrentUserMap ()) ? NULL : &Umap->hal;
  if (!hal_umap_getl1p (Hal, Va, Alloc, &L1p))
    {
      if (OldPfn)
	*OldPfn = PFN_INVALID;
      return FALSE;
    }
  OldL1e = hal_l1e_set (L1p, L1e);
  __atomic_or_fetch (&Umap->tlbop, hal_l1e_tlbop (OldL1e, L1e),
		     __ATOMIC_RELEASE);

  hal_l1e_unbox (OldL1e, &OldPfn, &OldProt);
  if (OldPfn != NULL)
    *OldPfn = OldProt & HAL_PTE_P ? OldPfn : PFN_INVALID;

  return TRUE;
}

/**
  Change protection flags on user mapping.

  Modifies protection flags for an existing mapping by setting
  and clearing specified flag bits. Does not flush TLB - call
  UmapCommit() to synchronize.

  @param[in] Umap    User address space map.
  @param[in] Va       Virtual address to modify.
  @param[in] ProtSet  Protection flags to set (HAL_PTE_*).
  @param[in] ProtClr  Protection flags to clear (HAL_PTE_*).

  @return Previous protection flags, or 0 if page not mapped.
**/
UINT32
UmapChangeFlags (
  IN struct umap  *Umap,
  IN vaddr_t      Va,
  IN UINT32       ProtSet,
  IN UINT32       ProtClr
  )
{
  hal_l1p_t L1p;
  hal_l1e_t OldL1e, L1e;
  pfn_t Pfn;
  UINT32 OldFlags, Flags;

  if (!hal_umap_getl1p (&Umap->hal, Va, FALSE, &L1p))
    return 0;

  L1e = hal_l1e_get (L1p);
  hal_l1e_unbox (L1e, &Pfn, &OldFlags);
  Flags = OldFlags | ProtSet;
  Flags &= ~ProtClr;
  L1e = hal_l1e_box (Pfn, Flags);
  OldL1e = hal_l1e_set (L1p, L1e);
  __atomic_or_fetch (&Umap->tlbop, hal_l1e_tlbop (OldL1e, L1e),
		     __ATOMIC_RELEASE);
  return OldFlags;
}

/**
  Map user virtual address to physical page frame.

  Creates or updates a user mapping. Allocates page tables as needed.
  Does not flush TLB - call UmapCommit() to synchronize.

  @param[in]  Umap   User address space map.
  @param[in]  Va      Virtual address to map.
  @param[in]  Pfn     Page frame number to map.
  @param[in]  Prot    Protection flags (HAL_PTE_*).
  @param[out] OldPfn Optional pointer to receive previous PFN.

  @retval TRUE   Mapping created successfully.
  @retval FALSE  Failed to allocate page tables.
**/
BOOLEAN
UmapMap (
  IN  struct umap  *Umap,
  IN  vaddr_t      Va,
  IN  pfn_t        Pfn,
  IN  UINT32       Prot,
  OUT pfn_t        *OldPfn OPTIONAL
  )
{
  hal_l1e_t L1e;

  L1e = hal_l1e_box (Pfn, Prot);
  return UmapSetL1e (Umap, Va, L1e, TRUE, OldPfn);
}

/**
  Unmap user virtual address.

  Removes mapping for specified virtual address. Does not flush TLB -
  call UmapCommit() to synchronize.

  @param[in] Umap  User address space map.
  @param[in] Va     Virtual address to unmap.

  @return Previous PFN if page was present, or PFN_INVALID if not mapped.
**/
pfn_t
UmapUnmap (
  IN struct umap  *Umap,
  IN vaddr_t      Va
  )
{
  hal_l1p_t L1p;
  hal_l1e_t L1e, OldL1e;
  pfn_t OldPfn;
  UINT32 OldProt;

  if (!hal_umap_getl1p (&Umap->hal, Va, FALSE, &L1p))
    {
      return PFN_INVALID;
    }

  L1e = hal_l1e_box (PFN_INVALID, 0);
  OldL1e = hal_l1e_set (L1p, L1e);
  __atomic_or_fetch (&Umap->tlbop, hal_l1e_tlbop (OldL1e, L1e),
		     __ATOMIC_RELEASE);

  hal_l1e_unbox (OldL1e, &OldPfn, &OldProt);
  return OldProt & HAL_PTE_P ? OldPfn : PFN_INVALID;
}

/**
  Commit user mapping changes.

  Broadcasts TLB flush to all CPUs using this address space,
  ensuring mapping changes are visible. Clears pending TLB
  operation flags.

  @param[in] Umap  User address space map.
**/
VOID
UmapCommit (
  IN struct umap  *Umap
  )
{
  __atomic_clear (&Umap->tlbop, __ATOMIC_RELEASE);
  CpuTlbFlushMask (Umap->cpumask);
}

/**
  Bootstrap user address space map.

  Initializes a minimal user map for early boot using HAL
  bootstrap functionality.

  @param[in] Umap  User address space map to bootstrap.
**/
VOID
UmapBootstrap (
  IN struct umap  *Umap
  )
{
  Umap->tlbop = 0;
  Umap->cpumask = 0;
  hal_umap_bootstrap (&Umap->hal);
}

/**
  Free user address space map.

  Releases all page tables associated with user address space.
  Caller must ensure no CPUs are using this map (cpumask == 0).

  @param[in] Umap  User address space map to free.
**/
VOID
UmapFree (
  IN struct umap  *Umap
  )
{
  assert (Umap->cpumask == 0);
  hal_umap_free (&Umap->hal);
}

/**
  Initialize user address space map.

  Creates a new user address space map with clean state.

  @param[in] Umap  User address space map to initialize.
**/
VOID
UmapInitialize (
  IN struct umap  *Umap
  )
{
  Umap->tlbop = 0;
  Umap->cpumask = 0;
  hal_umap_init (&Umap->hal);
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use UmapSetL1e instead **/
static bool _umap_setl1e (struct umap *umap, vaddr_t va, hal_l1e_t l1e, bool alloc,
	      pfn_t * opfn) {
  return UmapSetL1e (umap, va, l1e, alloc, opfn);
}

/** @deprecated Use UmapChangeFlags instead **/
unsigned umap_chflags (struct umap *umap, vaddr_t va,
	      unsigned prot_set, unsigned prot_clr) {
  return UmapChangeFlags (umap, va, prot_set, prot_clr);
}

/** @deprecated Use UmapMap instead **/
bool umap_map (struct umap *umap, vaddr_t va, pfn_t pfn, unsigned prot,
	  pfn_t * opfn) {
  return UmapMap (umap, va, pfn, prot, opfn);
}

/** @deprecated Use UmapUnmap instead **/
pfn_t umap_unmap (struct umap *umap, vaddr_t va) {
  return UmapUnmap (umap, va);
}

/** @deprecated Use UmapCommit instead **/
void umap_commit (struct umap *umap) {
  UmapCommit (umap);
}

/** @deprecated Use UmapBootstrap instead **/
void umap_bootstrap (struct umap *umap) {
  UmapBootstrap (umap);
}

/** @deprecated Use UmapFree instead **/
void umap_free (struct umap *umap) {
  UmapFree (umap);
}

/** @deprecated Use UmapInitialize instead **/
void umap_init (struct umap *umap) {
  UmapInitialize (umap);
}
