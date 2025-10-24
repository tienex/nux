/** @file
  RISC-V Page Mapping Operations

  Provides page table entry manipulation and L1 page mapping operations
  for RISC-V Sv48 architecture.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "internal.h"
#include <nux/hal.h>
#include <nux/nux.h>

/**
  Get L1 page table pointer for kernel virtual address.

  @param[in]  Va      Virtual address.
  @param[in]  Alloc   TRUE to allocate page tables if needed.
  @param[out] pL1p    Pointer to receive L1 page table pointer.

  @retval TRUE   L1 page table pointer obtained successfully.
  @retval FALSE  Address is not in kernel range or allocation failed.
**/
BOOLEAN
HalKmapGetL1p (
  IN  UINTN      Va,
  IN  BOOLEAN    Alloc,
  OUT hal_l1p_t  *pL1p OPTIONAL
  )
{
  hal_l1p_t l1p;

  if (Va < PtUmapMaxAddr ())
    {
      if (pL1p != NULL)
	*pL1p = L1P_INVALID;
      return FALSE;
    }

  l1p = cpumap_get_l1p (Va, Alloc);

  if (pL1p != NULL)
    *pL1p = l1p;

  return l1p != L1P_INVALID;
}

/**
  Get L1 page table pointer for user virtual address.

  @param[in]  pUmap   User address space map.
  @param[in]  Uaddr   User virtual address.
  @param[in]  Alloc   TRUE to allocate page tables if needed.
  @param[out] pL1p    Pointer to receive L1 page table pointer.

  @retval TRUE   L1 page table pointer obtained successfully.
  @retval FALSE  Address is out of range or allocation failed.
**/
BOOLEAN
HalUmapGetL1p (
  IN  struct hal_umap  *pUmap,
  IN  UINTN            Uaddr,
  IN  BOOLEAN          Alloc,
  OUT hal_l1p_t        *pL1p OPTIONAL
  )
{
  hal_l1p_t l1p;

  if ((Uaddr >= PtUmapMaxAddr ()) && (Uaddr < PtUmapMinAddr ()))
    {
      if (pL1p != NULL)
	*pL1p = L1P_INVALID;
      return FALSE;
    }

  l1p = UmapGetL1p (pUmap, Uaddr, Alloc);
  if (pL1p != NULL)
    *pL1p = l1p;

  return l1p != L1P_INVALID;
}

/**
  Get L1 page table entry.

  @param[in] L1p  L1 page table pointer.

  @return L1 page table entry value.
**/
hal_l1e_t
HalL1eGet (
  IN hal_l1p_t  L1p
  )
{
  return (hal_l1e_t) GetPte (L1p);
}

/**
  Set L1 page table entry.

  @param[in] L1p  L1 page table pointer.
  @param[in] L1e  L1 page table entry value to set.

  @return Previous L1 page table entry value.
**/
hal_l1e_t
HalL1eSet (
  IN hal_l1p_t  L1p,
  IN hal_l1e_t  L1e
  )
{
  hal_l1e_t OldL1e;

  OldL1e = SetPte ((PTEP) L1p, (PTE) L1e);
  return OldL1e;
}


/**
  Create L1 page table entry from PFN and protection flags.

  @param[in] Pfn   Page frame number.
  @param[in] Prot  Protection flags (HAL_PTE_*).

  @return Constructed L1 page table entry.
**/
hal_l1e_t
HalL1eBox (
  IN UINTN   Pfn,
  IN UINT32  Prot
  )
{
  hal_l1e_t l1e;

  l1e = (UINT64) Pfn << PTE_PFN_SHIFT;

  if (Prot & HAL_PTE_P)
    l1e |= (PTE_V | PTE_R);
  if (Prot & HAL_PTE_W)
    l1e |= PTE_W;
  if (Prot & HAL_PTE_X)
    l1e |= PTE_X;
  if (Prot & HAL_PTE_U)
    l1e |= PTE_U;
  if (Prot & HAL_PTE_GLOBAL)
    l1e |= PTE_GLOBAL;
  if (Prot & HAL_PTE_A)
    l1e |= PTE_A;
  if (Prot & HAL_PTE_D)
    l1e |= PTE_D;
  if (Prot & HAL_PTE_AVL0)
    l1e |= PTE_AVL0;
  if (Prot & HAL_PTE_AVL1)
    l1e |= PTE_AVL1;

  return l1e;
}

/**
  Extract PFN and protection flags from L1 page table entry.

  @param[in]  L1e    L1 page table entry.
  @param[out] pPfn   Pointer to receive page frame number.
  @param[out] pProt  Pointer to receive protection flags.
**/
VOID
HalL1eUnbox (
  IN  hal_l1e_t  L1e,
  OUT UINTN      *pPfn OPTIONAL,
  OUT UINT32     *pProt OPTIONAL
  )
{
  UINT32 Prot = 0;

  if (L1e & PTE_V)
    {
      if (L1e & PTE_R)
	Prot |= HAL_PTE_P;
      if (L1e & PTE_W)
	Prot |= HAL_PTE_W;
      if (L1e & PTE_X)
	Prot |= HAL_PTE_X;
      if (L1e & PTE_U)
	Prot |= HAL_PTE_U;
      if (L1e & PTE_GLOBAL)
	Prot |= HAL_PTE_GLOBAL;
      if (L1e & PTE_A)
	Prot |= HAL_PTE_A;
      if (L1e & PTE_D)
	Prot |= HAL_PTE_D;
      if (L1e & PTE_AVL0)
	Prot |= HAL_PTE_AVL0;
      if (L1e & PTE_AVL1)
	Prot |= HAL_PTE_AVL1;
    }

  if (pPfn)
    *pPfn = pte_pfn (L1e);
  if (pProt)
    *pProt = Prot;
}

/**
  Determine TLB operation required when changing page table entry.

  @param[in] Old  Old L1 page table entry value.
  @param[in] New  New L1 page table entry value.

  @return Required TLB operation (HAL_TLBOP_*).
**/
hal_tlbop_t
HalL1eTlbOp (
  IN hal_l1e_t  Old,
  IN hal_l1e_t  New
  )
{
  /* Without SVVPTC always flush. */
  return HAL_TLBOP_FLUSH;
}

/**
  Get next mapped user address and page table information.

  @param[in]  pUmap  User address space map.
  @param[in]  Uaddr  Starting user address.
  @param[out] pL1p   Pointer to receive L1 page table pointer.
  @param[out] pL1e   Pointer to receive L1 page table entry.

  @return Next mapped user address, or UADDR_INVALID if none.
**/
uaddr_t
HalUmapNext (
  IN  struct hal_umap  *pUmap,
  IN  uaddr_t          Uaddr,
  OUT hal_l1p_t        *pL1p OPTIONAL,
  OUT hal_l1e_t        *pL1e OPTIONAL
  )
{
  if (Uaddr < hal_virtmem_userbase ())
    Uaddr = hal_virtmem_userbase ();

  return PtUmapNext (pUmap, Uaddr, pL1p, pL1e);
}

/**
  Free user address space page tables.

  @param[in] pUmap  User address space map to free.
**/
VOID
HalUmapFree (
  IN struct hal_umap  *pUmap
  )
{
  return PtUmapFree (pUmap);
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use HalKmapGetL1p instead **/
bool hal_kmap_getl1p (unsigned long va, bool alloc, hal_l1p_t * l1popq) {
  return HalKmapGetL1p (va, alloc, l1popq);
}

/** @deprecated Use HalUmapGetL1p instead **/
bool hal_umap_getl1p (struct hal_umap *umap, unsigned long uaddr, bool alloc,
		 hal_l1p_t * l1popq) {
  return HalUmapGetL1p (umap, uaddr, alloc, l1popq);
}

/** @deprecated Use HalL1eGet instead **/
hal_l1e_t hal_l1e_get (hal_l1p_t l1popq) {
  return HalL1eGet (l1popq);
}

/** @deprecated Use HalL1eSet instead **/
hal_l1e_t hal_l1e_set (hal_l1p_t l1popq, hal_l1e_t l1e) {
  return HalL1eSet (l1popq, l1e);
}

/** @deprecated Use HalL1eBox instead **/
hal_l1e_t hal_l1e_box (unsigned long pfn, unsigned prot) {
  return HalL1eBox (pfn, prot);
}

/** @deprecated Use HalL1eUnbox instead **/
void hal_l1e_unbox (hal_l1e_t l1e, unsigned long *pfnp, unsigned *protp) {
  HalL1eUnbox (l1e, pfnp, protp);
}

/** @deprecated Use HalL1eTlbOp instead **/
hal_tlbop_t hal_l1e_tlbop (hal_l1e_t old, hal_l1e_t new) {
  return HalL1eTlbOp (old, new);
}

/** @deprecated Use HalUmapNext instead **/
uaddr_t hal_umap_next (struct hal_umap *umap, uaddr_t uaddr, hal_l1p_t * l1p,
	       hal_l1e_t * l1e) {
  return HalUmapNext (umap, uaddr, l1p, l1e);
}

/** @deprecated Use HalUmapFree instead **/
void hal_umap_free (struct hal_umap *umap) {
  HalUmapFree (umap);
}
