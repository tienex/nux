/** @file
  NUX Kernel Mapping (KMAP) Operations

  Provides low-level kernel virtual address to physical page mapping
  operations. These functions are unlocked and do not flush TLBs
  automatically - use with care and call KmapCommit() to flush.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <assert.h>
#include <hal/hal.h>
#include <nux/nux.h>

#include <nux/internal.h>

//
// Low level routines to handle kernel mappings.
//
// Unlocked, unflushing, use with care.
//

/**
  Initialize kernel mapping subsystem.

  Currently a no-op placeholder for future initialization needs.
**/
VOID
KmapInitialize (
  VOID
  )
{
}

/**
  Internal kernel map operation with allocation control.

  Maps or remaps a kernel virtual address to a physical page frame.

  @param[in] Va     Virtual address to map.
  @param[in] Pfn    Page frame number to map.
  @param[in] Prot   Protection flags (HAL_PTE_*).
  @param[in] Alloc  TRUE to allow page table allocation, FALSE otherwise.

  @return Previous PFN if page was present, or PFN_INVALID if not mapped.
**/
static PFN
KmapMapInternal (
  IN VIRTUAL_ADDRESS  Va,
  IN PFN    Pfn,
  IN UINT32   Prot,
  IN CONST INT32  Alloc
  )
{
  hal_l1p_t L1p;
  hal_l1e_t L1e, OldL1e;
  PFN OldPfn;
  UINT32 OldProt;

  L1e = hal_l1e_box (Pfn, Prot);

  assert (hal_kmap_getl1p (Va, Alloc, &L1p));
  OldL1e = hal_l1e_set (L1p, L1e);
  KtlbGenMarkDirty (hal_l1e_tlbop (OldL1e, L1e));

  hal_l1e_unbox (OldL1e, &OldPfn, &OldProt);

  return OldProt & HAL_PTE_P ? OldPfn : PFN_INVALID;
}

/**
  Get physical page frame number for kernel virtual address.

  @param[in] Va  Virtual address to query.

  @return Page frame number if mapped, or PFN_INVALID if not present.
**/
PFN
KmapGetPfn (
  IN VIRTUAL_ADDRESS  Va
  )
{
  PFN Pfn;
  UINT32 Flags;
  hal_l1e_t L1e;
  hal_l1p_t L1p;

  if (!hal_kmap_getl1p (Va, FALSE, &L1p))
    return PFN_INVALID;

  L1e = hal_l1e_get (L1p);
  hal_l1e_unbox (L1e, &Pfn, &Flags);

  return Flags & HAL_PTE_P ? Pfn : PFN_INVALID;
}

/**
  Map kernel virtual address to physical page frame.

  Allocates page tables as needed. Does not flush TLB - call
  KmapCommit() to synchronize changes across CPUs.

  @param[in] Va    Virtual address to map.
  @param[in] Pfn   Page frame number to map.
  @param[in] Prot  Protection flags (HAL_PTE_*).

  @return Previous PFN if page was present, or PFN_INVALID if not mapped.
**/
PFN
KmapMap (
  IN VIRTUAL_ADDRESS  Va,
  IN PFN    Pfn,
  IN UINT32   Prot
  )
{
  return KmapMapInternal (Va, Pfn, Prot, 1);
}

/**
  Map kernel virtual address without allocating page tables.

  Only maps if page table structures already exist. Does not flush TLB -
  call KmapCommit() to synchronize.

  @param[in] Va    Virtual address to map.
  @param[in] Pfn   Page frame number to map.
  @param[in] Prot  Protection flags (HAL_PTE_*).

  @return Previous PFN if page was present, or PFN_INVALID if not mapped.
**/
PFN
KmapMapNoAlloc (
  IN VIRTUAL_ADDRESS  Va,
  IN PFN    Pfn,
  IN UINT32   Prot
  )
{
  return KmapMapInternal (Va, Pfn, Prot, 0);
}

/**
  Unmap kernel virtual address.

  Removes mapping for specified virtual address. Does not flush TLB -
  call KmapCommit() to synchronize.

  @param[in] Va  Virtual address to unmap.

  @return Previous PFN if page was present, or PFN_INVALID if not mapped.
**/
PFN
KmapUnmap (
  IN VIRTUAL_ADDRESS  Va
  )
{
  hal_l1p_t L1p;
  hal_l1e_t L1e, OldL1e;
  PFN OldPfn;
  UINT32 OldProt;

  L1e = hal_l1e_box (0, 0);
  if (hal_kmap_getl1p (Va, 0, &L1p))
    {
      OldL1e = hal_l1e_set (L1p, L1e);
      KtlbGenMarkDirty (hal_l1e_tlbop (OldL1e, L1e));

      hal_l1e_unbox (OldL1e, &OldPfn, &OldProt);

      return OldProt & HAL_PTE_P ? OldPfn : PFN_INVALID;
    }

  return PFN_INVALID;
}

/**
  Check if kernel virtual address is mapped.

  @param[in] Va  Virtual address to check.

  @retval TRUE   Address is mapped.
  @retval FALSE  Address is not mapped.
**/
INT32
KmapIsMapped (
  IN VIRTUAL_ADDRESS  Va
  )
{
  return hal_kmap_getl1p (Va, 0, NULL);
}

/**
  Check if kernel virtual address range is mapped.

  Verifies that all pages in specified range have valid mappings.

  @param[in] Va    Starting virtual address.
  @param[in] Size  Size of range in bytes.

  @retval TRUE   Entire range is mapped.
  @retval FALSE  At least one page in range is not mapped.
**/
INT32
KmapIsMappedRange (
  IN VIRTUAL_ADDRESS  Va,
  IN UINTN   Size
  )
{
  VIRTUAL_ADDRESS i, S, E;

  S = trunc_page (Va);
  E = Va + Size;

  for (i = S; i < E; i += PAGE_SIZE)
    if (!KmapIsMapped (i))
      return 0;

  return 1;
}

/**
  Ensure kernel virtual address has required protection.

  Allocates or frees physical pages as needed to match required protection.
  If mapping must transition from unmapped to mapped, allocates a page.
  If transitioning from mapped to unmapped, frees the page.

  @param[in] Va       Virtual address to ensure.
  @param[in] ReqProt  Required protection flags (HAL_PTE_*).

  @retval 0   Success.
  @retval -1  Failure (e.g., page allocation failed).
**/
INT32
KmapEnsure (
  IN VIRTUAL_ADDRESS  Va,
  IN UINT32   ReqProt
  )
{
  INT32 Ret = -1;
  hal_l1p_t L1p = L1P_INVALID;
  hal_l1e_t OldL1e, L1e;
  PFN Pfn;
  UINT32 Prot;

  if (hal_kmap_getl1p (Va, 0, &L1p))
    {
      L1e = hal_l1e_get (L1p);
      hal_l1e_unbox (L1e, &Pfn, &Prot);
    }
  else
    {
      Pfn = PFN_INVALID;
      Prot = 0;
    }

  if (!(ReqProt ^ Prot))
    {
      /* same, exit */
      Ret = 0;
      goto out;
    }

  /* Check present bit. If we are adding a P bit allocate, if we are
     removing it free the page. */
  if ((ReqProt & HAL_PTE_P) != (Prot & HAL_PTE_P))
    {
      if (ReqProt & HAL_PTE_P)
        {
          /* Ensure pagetable populated. */
          if (L1p == L1P_INVALID)
            assert (hal_kmap_getl1p (Va, 1, &L1p));
          /* Populate page. */
          Pfn = PfnAlloc (0);
          if (Pfn == PFN_INVALID)
            goto out;
        }
      else
        {
          /* Freeing page. */
          PfnFree (Pfn);
          Pfn = PFN_INVALID;
        }
    }

  L1e = hal_l1e_box (Pfn, ReqProt);
  OldL1e = hal_l1e_set (L1p, L1e);
  KtlbGenMarkDirty (hal_l1e_tlbop (OldL1e, L1e));
  Ret = 0;

out:
  return Ret;
}

/**
  Ensure kernel virtual address range has required protection.

  Applies KmapEnsure() to all pages in specified range.

  @param[in] Va       Starting virtual address.
  @param[in] Size     Size of range in bytes.
  @param[in] ReqProt  Required protection flags (HAL_PTE_*).

  @retval 0   Success.
  @retval -1  Failure on any page in range.
**/
INT32
KmapEnsureRange (
  IN VIRTUAL_ADDRESS  Va,
  IN UINTN   Size,
  IN UINT32   ReqProt
  )
{
  VIRTUAL_ADDRESS i, S, E;

  S = trunc_page (Va);
  E = Va + Size;

  for (i = S; i < E; i += PAGE_SIZE)
    if (KmapEnsure (i, ReqProt))
      return -1;

  return 0;
}

/**
  Commit kernel mapping changes.

  Broadcasts kernel map update to all CPUs, causing them to flush
  their TLBs to synchronize with kernel mapping changes. This is
  extremely slow but guarantees KMAP consistency across all CPUs.

  Must be called after any KmapMap/KmapUnmap operations to
  ensure changes are visible on all processors.
**/
VOID
KmapCommit (
  VOID
  )
{
  /*
     This is extremely slow, but guarantees KMAP to be aligned in all
     CPUs.
   */
  CpuKernelMapUpdateBroadcast ();
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use KmapInitialize instead **/
VOID kmapinit (VOID) {
  KmapInitialize ();
}

/** @deprecated Use KmapMapInternal instead **/
static PFN _kmap_map (VIRTUAL_ADDRESS va, PFN pfn, UINT32 prot, CONST INT32 alloc) {
  return KmapMapInternal (va, pfn, prot, alloc);
}

/** @deprecated Use KmapGetPfn instead **/
PFN KmapGetPfn (VIRTUAL_ADDRESS va) {
  return KmapGetPfn (va);
}

/** @deprecated Use KmapMap instead **/
PFN KmapMap (VIRTUAL_ADDRESS va, PFN pfn, UINT32 prot) {
  return KmapMap (va, pfn, prot);
}

/** @deprecated Use KmapMapNoAlloc instead **/
PFN KmapMapNoAlloc (VIRTUAL_ADDRESS va, PFN pfn, UINT32 prot) {
  return KmapMapNoAlloc (va, pfn, prot);
}

/** @deprecated Use KmapUnmap instead **/
PFN KmapUnmap (VIRTUAL_ADDRESS va) {
  return KmapUnmap (va);
}

/** @deprecated Use KmapIsMapped instead **/
int KmapMapped (VIRTUAL_ADDRESS va) {
  return KmapIsMapped (va);
}

/** @deprecated Use KmapIsMappedRange instead **/
int kmap_mapped_range (VIRTUAL_ADDRESS va, UINTN size) {
  return KmapIsMappedRange (va, size);
}

/** @deprecated Use KmapEnsure instead **/
int KmapEnsure (VIRTUAL_ADDRESS va, UINT32 reqprot) {
  return KmapEnsure (va, reqprot);
}

/** @deprecated Use KmapEnsureRange instead **/
int kmap_ensure_range (VIRTUAL_ADDRESS va, UINTN size, UINT32 reqprot) {
  return KmapEnsureRange (va, size, reqprot);
}

/** @deprecated Use KmapCommit instead **/
VOID KmapCommit (VOID) {
  KmapCommit ();
}
