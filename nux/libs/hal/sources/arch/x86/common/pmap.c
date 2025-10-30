/** @file
  x86 Page Mapping Operations

  Provides page table entry manipulation, L1 page mapping operations,
  and page table initialization for x86/AMD64 architectures.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <assert.h>
#include <stdbool.h>
#include <hal/hal.h>
#include <nux/nux.h>
#include <hal/internal.h>

UINT64 gPteNx = 0;

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

  l1p = KmapGetL1p (Va, Alloc);

  if (pL1p != NULL)
    *pL1p = l1p;

  return l1p != L1P_INVALID;
}

/**
  Get L1 page table pointer for user virtual address.

  @param[in]  Umap   User address space map.
  @param[in]  Uaddr   User virtual address.
  @param[in]  Alloc   TRUE to allocate page tables if needed.
  @param[out] pL1p    Pointer to receive L1 page table pointer.

  @retval TRUE   L1 page table pointer obtained successfully.
  @retval FALSE  Address is out of range or allocation failed.
**/
BOOLEAN
HalUmapGetL1p (
  IN  struct hal_umap  *Umap,
  IN  UINTN            Uaddr,
  IN  BOOLEAN          Alloc,
  OUT hal_l1p_t        *pL1p OPTIONAL
  )
{
  hal_l1p_t l1p;

  if ((Uaddr >= PtUmapMaxAddr ()) || (Uaddr < PtUmapMinAddr ()))
    {
      if (pL1p != NULL)
	*pL1p = L1P_INVALID;
      return FALSE;
    }

  l1p = UmapGetL1p (Umap, Uaddr, Alloc);
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

  l1e = (UINT64) Pfn << HAL_PAGE_SHIFT;

  if (Prot & HAL_PTE_P)
    l1e |= PTE_P;
  if (Prot & HAL_PTE_W)
    l1e |= PTE_W;
  if (!(Prot & HAL_PTE_X) && (Prot & HAL_PTE_P))
    l1e |= gPteNx;
  if (Prot & HAL_PTE_U)
    l1e |= PTE_U;
  if (Prot & HAL_PTE_GLOBAL)
    l1e |= PTE_G;
  if (Prot & HAL_PTE_A)
    l1e |= PTE_A;
  if (Prot & HAL_PTE_D)
    l1e |= PTE_D;
  if (Prot & HAL_PTE_AVL0)
    l1e |= PTE_AVAIL0;
  if (Prot & HAL_PTE_AVL1)
    l1e |= PTE_AVAIL1;
  if (Prot & HAL_PTE_AVL2)
    l1e |= PTE_AVAIL2;

  return l1e;
}

/**
  Extract PFN and protection flags from L1 page table entry.

  @param[in]  L1e    L1 page table entry.
  @param[out] Pfn   Pointer to receive page frame number.
  @param[out] Prot  Pointer to receive protection flags.
**/
VOID
HalL1eUnbox (
  IN  hal_l1e_t  L1e,
  OUT UINTN      *Pfn OPTIONAL,
  OUT UINT32     *Prot OPTIONAL
  )
{
  UINT32 Prot = 0;

  if (L1e & PTE_P)
    Prot |= HAL_PTE_P;
  if (L1e & PTE_W)
    Prot |= HAL_PTE_W;
  if (!(L1e & PTE_NX) && (L1e & PTE_P))
    Prot |= HAL_PTE_X;
  if (L1e & PTE_U)
    Prot |= HAL_PTE_U;
  if (L1e & PTE_G)
    Prot |= HAL_PTE_GLOBAL;
  if (L1e & PTE_A)
    Prot |= HAL_PTE_A;
  if (L1e & PTE_D)
    Prot |= HAL_PTE_D;
  if (L1e & PTE_AVAIL0)
    Prot |= HAL_PTE_AVL0;
  if (L1e & PTE_AVAIL1)
    Prot |= HAL_PTE_AVL1;
  if (L1e & PTE_AVAIL2)
    Prot |= HAL_PTE_AVL2;

  if (Pfn)
    *Pfn = l1epfn (L1e);
  if (Prot)
    *Prot = Prot;
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
#define restricts_permissions(_o, _n) 1

  /* Previous not present. Don't flush. */
  if (!(l1eflags (Old) & PTE_P))
    return 0;

  /* Mapping a different page. Flush. */
  if ((l1epfn (Old) != l1epfn (New)) || restricts_permissions (Old, New))
    {
      if ((l1eflags (Old) & PTE_G) || (l1eflags (New) & PTE_G))
	{
	  return HAL_TLBOP_FLUSHALL;
	}
      else
	{
	  return HAL_TLBOP_FLUSH;
	}
    }

  return HAL_TLBOP_NONE;
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
HalUmapNext (
  IN  struct hal_umap  *Umap,
  IN  USER_ADDRESS          Uaddr,
  OUT hal_l1p_t        *pL1p OPTIONAL,
  OUT hal_l1e_t        *pL1e OPTIONAL
  )
{
  if (Uaddr < hal_virtmem_userbase ())
    Uaddr = hal_virtmem_userbase ();

  return PtUmapNext (Umap, Uaddr, pL1p, pL1e);
}

/**
  Free user address space page tables.

  @param[in] Umap  User address space map to free.
**/
VOID
HalUmapFree (
  IN struct hal_umap  *Umap
  )
{
  return PtUmapFree (Umap);
}

/**
  Check if CPU supports NX (No Execute) bit.

  @retval TRUE   CPU supports NX bit.
  @retval FALSE  CPU does not support NX bit.
**/
static BOOLEAN
CpuSupportsNx (
  VOID
  )
{
  UINT64 Efer;

  Efer = ReadMsr (MSR_IA32_EFER);
  return !!(Efer & _MSR_IA32_EFER_NXE);
}

/**
  Initialize page mapping subsystem.

  Detects NX support and initializes architecture-specific page tables.
**/
VOID
PmapInitialize (
  VOID
  )
{
  if (CpuSupportsNx ())
    gPteNx = PTE_NX;
  else
    printf ("CPU does not support NX.\n");

#ifdef __i386__
  Pae32Initialize ();
#endif
#ifdef __amd64__
  Pae64Initialize ();
#endif
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use HalKmapGetL1p instead **/
BOOLEAN hal_kmap_getl1p (unsigned long va, BOOLEAN alloc, hal_l1p_t * l1popq) {
  return HalKmapGetL1p (va, alloc, l1popq);
}

/** @deprecated Use HalUmapGetL1p instead **/
BOOLEAN hal_umap_getl1p (struct hal_umap *umap, unsigned long uaddr, BOOLEAN alloc,
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
hal_l1e_t hal_l1e_box (unsigned long pfn, UINT32 prot) {
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
USER_ADDRESS hal_umap_next (struct hal_umap *umap, USER_ADDRESS uaddr, hal_l1p_t * l1p,
	       hal_l1e_t * l1e) {
  return HalUmapNext (umap, uaddr, l1p, l1e);
}

/** @deprecated Use HalUmapFree instead **/
void hal_umap_free (struct hal_umap *umap) {
  HalUmapFree (umap);
}

/** @deprecated Use PmapInitialize instead **/
void pmap_init (void) {
  PmapInitialize ();
}

/** @deprecated Use CpuSupportsNx instead **/
static BOOLEAN cpu_supports_nx (void) {
  return CpuSupportsNx ();
}

// Legacy global variable alias
UINT64 pte_nx = 0;
