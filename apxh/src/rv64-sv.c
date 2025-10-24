/** @file
  APXH RISC-V SV48 Paging Support

  Implements SV48 (Supervisor Virtual Memory with 48-bit addresses)
  page table management for RISC-V 64-bit systems. Provides 4-level
  page tables with support for 1GB, 2MB, and 4KB mappings.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "project.h"

#define PTE_V (1LL << 0)
#define PTE_R (1LL << 1)
#define PTE_W (1LL << 2)
#define PTE_X (1LL << 3)
#define PTE_U (1LL << 4)

typedef UINT64 pte_t;

static pte_t *gSv48Root;

/**
  Set page table entry.

  Constructs and writes a PTE with the given PFN and flags.

  @param[out] pPtep  Pointer to PTE.
  @param[in]  Pfn    Page frame number.
  @param[in]  Flags  PTE flags (PTE_V, PTE_R, PTE_W, PTE_X, PTE_U).
**/
static VOID
SetPte (
  OUT pte_t   *pPtep,
  IN UINT64   Pfn,
  IN UINT64   Flags
  )
{
  *pPtep = (Pfn << 10) | Flags;
}

/**
  Get physical address from PTE.

  Extracts the physical address from a page table entry.

  @param[in] pPtep  Pointer to PTE.

  @return Physical address, or NULL if entry not valid.
**/
static VOID *
PteGetAddr (
  IN pte_t  *pPtep
  )
{
  pte_t Pte = *pPtep;

  if (!(Pte & PTE_V))
    return NULL;

  return (VOID *) (UINTN) ((Pte & 0x7ffffffffffffc00LL) << 2);
}

/**
  Get flags from PTE.

  Extracts the flag bits from a page table entry.

  @param[in] pPtep  Pointer to PTE.

  @return PTE flags.
**/
static UINT64
PteGetFlags (
  IN pte_t  *pPtep
  )
{
  pte_t Pte = *pPtep;

  return Pte & ~0x7ffffffffffffc00LL;
}

/**
  Merge PTE flags.

  Combines two sets of PTE flags according to RISC-V rules.
  Validates consistency of user/kernel bits.

  @param[in] Fl1  First set of flags.
  @param[in] Fl2  Second set of flags.

  @return Merged flags.
**/
static UINT64
PteMergeFlags (
  IN UINT64  Fl1,
  IN UINT64  Fl2
  )
{
  UINT64 NewF;

  //PTE_R and V always present
  assert (Fl1 & Fl2 & PTE_V);
  assert (Fl1 & Fl2 & PTE_R);
  NewF = PTE_R | PTE_V;

  //PTE_U is either present on both or absent on both
  if ((Fl1 & PTE_U) != (Fl2 & PTE_U))
    fatal ("Mixed user/kernel addresses not allowed.");
  NewF |= Fl1 & PTE_U;

  // Write must be OR'd.
  NewF |= (Fl1 | Fl2) & PTE_W;

  // Executable must be OR'd.
  NewF |= (Fl1 | Fl2) & PTE_X;

#if 0
  printf ("Merging: %llx (+) %llx = %llx\n", Fl1, Fl2, NewF);
#endif
  return NewF;
}

/*
  RISC-V SV48 Support.
*/

#define SV48_DIRECTMAP_START 0
#define SV48_DIRECTMAP_END (1LL << 30)

#define L4OFF64(_va) (((_va) >> 39) & 0x1ff)
#define L3OFF64(_va) (((_va) >> 30) & 0x1ff)
#define L2OFF64(_va) (((_va) >> 21) & 0x1ff)
#define L1OFF64(_va) (((_va) >> 12) & 0x1ff)

/**
  Get L3 page table pointer.

  Retrieves or allocates L3 page table for the given virtual address.

  @param[in] pRoot    Root page table.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE if allocating from payload pages.

  @return Pointer to L3 PTE.
**/
static pte_t *
Sv48GetL3p (
  IN pte_t   *pRoot,
  IN vaddr_t  Va,
  IN INT32    Payload
  )
{
  pte_t *pL4p, *pL3p;
  UINT32 L4off = L4OFF64 (Va);
  UINT32 L3off = L3OFF64 (Va);

  pL4p = pRoot + L4off;

  pL3p = (pte_t *) PteGetAddr (pL4p);
  if (pL3p == NULL)
    {
      UINTN L3page;

      /* Populating L3. */
      L3page = Payload ? GetPayloadPage () : GetPage ();

      SetPte (pL4p, L3page >> PAGE_SHIFT, PTE_V);
      pL3p = (pte_t *) L3page;
    }

  return pL3p + L3off;
}

/**
  Get L2 page table pointer.

  Retrieves or allocates L2 page table for the given virtual address.

  @param[in] pRoot    Root page table.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE if allocating from payload pages.

  @return Pointer to L2 PTE.
**/
static pte_t *
Sv48GetL2p (
  IN pte_t   *pRoot,
  IN vaddr_t  Va,
  IN INT32    Payload
  )
{
  pte_t *pL3p, *pL2p;

  UINT32 L2off = L2OFF64 (Va);

  pL3p = Sv48GetL3p (pRoot, Va, Payload);

  pL2p = (pte_t *) PteGetAddr (pL3p);
  if (pL2p == NULL)
    {
      UINTN L2page;

      /* Populating L2. */
      L2page = Payload ? GetPayloadPage () : GetPage ();

      SetPte (pL3p, L2page >> PAGE_SHIFT, PTE_V);
      pL2p = (pte_t *) L2page;
    }

  return pL2p + L2off;
}

/**
  Get L1 page table pointer.

  Retrieves or allocates L1 page table for the given virtual address.

  @param[in] pRoot    Root page table.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE if allocating from payload pages.

  @return Pointer to L1 PTE.
**/
static pte_t *
Sv48GetL1p (
  IN pte_t   *pRoot,
  IN vaddr_t  Va,
  IN INT32    Payload
  )
{
  pte_t *pL2p, *pL1p;
  UINT32 L1off = L1OFF64 (Va);

  pL2p = Sv48GetL2p (pRoot, Va, Payload);

  pL1p = (pte_t *) PteGetAddr (pL2p);
  if (pL1p == NULL)
    {
      UINTN L1page;

      /* Populating L1. */
      L1page = Payload ? GetPayloadPage () : GetPage ();

      SetPte (pL2p, L1page >> PAGE_SHIFT, PTE_V);
      pL1p = (pte_t *) L1page;
    }

  return pL1p + L1off;
}

/**
  Verify virtual address range.

  Validates that a virtual address range is appropriate for SV48.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
Sv48Verify (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  /* Nothing to check. */
}

/**
  Initialize SV48 paging.

  Allocates and initializes the root page table for SV48.
**/
VOID
Sv48Initialize (
  VOID
  )
{
  gSv48Root = (pte_t *) GetPayloadPage ();

  printf ("Using SV48 paging (root: %08lx).\n", gSv48Root);
}

/**
  Map single page.

  Creates a page mapping at the specified virtual address.

  @param[in] pPt      Page table root.
  @param[in] Va       Virtual address.
  @param[in] Pa       Physical address.
  @param[in] Payload  TRUE if allocating from payload pages.
  @param[in] W        TRUE if writable.
  @param[in] X        TRUE if executable.
**/
VOID
Sv48MapPage (
  IN VOID     *pPt,
  IN vaddr_t  Va,
  IN UINTN    Pa,
  IN INT32    Payload,
  IN INT32    W,
  IN INT32    X
  )
{
  pte_t *pL1p, *pRoot;
  UINT64 L1f;
  UINTN Page;

  pRoot = (pte_t *) pPt;

  printf ("Mapping at va %llx PA %lx (p:%d, w:%d, x:%d)\n", Va, Pa, Payload,
	  W, X);

  pL1p = Sv48GetL1p (pRoot, Va, Payload);
  L1f = (W ? PTE_W : 0) | (X ? PTE_X : 0) | PTE_R | PTE_V;

  Page = (UINTN) PteGetAddr (pL1p);
  assert (Page == 0);
  Page = Pa >> PAGE_SHIFT;
  SetPte (pL1p, Page, L1f);
}

/**
  Populate page with flags.

  Allocates and maps a page with the specified attributes, merging
  flags if page already exists.

  @param[in] pRoot    Root page table.
  @param[in] Va       Virtual address.
  @param[in] U        TRUE if user-accessible.
  @param[in] W        TRUE if writable.
  @param[in] X        TRUE if executable.
  @param[in] Payload  TRUE if allocating from payload pages.

  @return Physical address of page.
**/
static UINTN
Sv48PopulatePage (
  IN pte_t   *pRoot,
  IN vaddr_t  Va,
  IN INT32    U,
  IN INT32    W,
  IN INT32    X,
  IN INT32    Payload
  )
{
  pte_t *pL1p;
  UINT64 L1f;
  UINTN Page;

  pL1p = Sv48GetL1p (pRoot, Va, Payload);
  L1f = (U ? PTE_U : 0) | (W ? PTE_W : 0) | (X ? PTE_X : 0) | PTE_R | PTE_V;

  Page = (UINTN) PteGetAddr (pL1p);
  if (Page == 0)
    {
      Page = Payload ? GetPayloadPage () : GetPage ();
      SetPte (pL1p, Page >> PAGE_SHIFT, L1f);
    }
  else
    {
      UINT64 NewF;
      UINT64 OldF = PteGetFlags (pL1p);

      NewF = PteMergeFlags (L1f, OldF);
      if (NewF != OldF)
	{
	  printf ("Flags changed from %llx to %llx\n", OldF, NewF);
	  SetPte (pL1p, Page >> PAGE_SHIFT, NewF);
	}
    }

  return Page;
}

/**
  Get physical address for virtual address.

  Walks page tables to translate virtual to physical address.

  @param[in] Va  Virtual address.

  @return Physical address.
**/
UINTN
Sv48GetPhys (
  IN vaddr_t  Va
  )
{
  UINTN Page;
  pte_t *pL4p, *pL3p, *pL2p, *pL1p;
  UINT32 L4off = L4OFF64 (Va);
  UINT32 L3off = L3OFF64 (Va);
  UINT32 L2off = L2OFF64 (Va);
  UINT32 L1off = L1OFF64 (Va);

  pL4p = gSv48Root + L4off;

  pL3p = (pte_t *) PteGetAddr (pL4p);
  assert (pL3p != NULL);
  pL3p += L3off;

  pL2p = (pte_t *) PteGetAddr (pL3p);
  assert (pL2p != NULL);
  pL2p += L2off;

  pL1p = (pte_t *) PteGetAddr (pL2p);
  assert (pL1p != NULL);
  pL1p += L1off;

  Page = (UINTN) PteGetAddr (pL1p);
  assert (Page != 0);

  return Page |= (Va & ~(PAGE_MASK));
}

/**
  Direct map physical region.

  Creates direct (1:1) physical to virtual mappings using large pages
  where possible (1GB or 2MB).

  @param[in] pPt      Page table root.
  @param[in] PaBase   Physical base address.
  @param[in] Va       Virtual address.
  @param[in] Size     Size of region.
  @param[in] Mt       Memory type.
  @param[in] Payload  TRUE if allocating from payload pages.
  @param[in] X        TRUE if executable.
**/
VOID
Sv48DirectMap (
  IN VOID              *pPt,
  IN UINT64            PaBase,
  IN vaddr_t           Va,
  IN size64_t          Size,
  IN enum memory_type  Mt,
  IN INT32             Payload,
  IN INT32             X
  )
{
  ssize64_t Len;
  UINT64 Pa;
  pte_t *pRoot = (pte_t *) pPt;
  UINT64 L3cnt = 0, L2cnt = 0, L1cnt = 0;

#define GB1ALIGNED(_a) (((_a) & ((1L << 30) - 1)) == 0)
#define MB2ALIGNED(_a) (((_a) & ((1L << 21) - 1)) == 0)

  /* Signed to unsigned: no one will ask us a 1<<64 bytes physmap. */
  Len = (ssize64_t) Size;
  Pa = PaBase;

  while (Len > 0)
    {

      if (GB1ALIGNED (Pa) && GB1ALIGNED (Va) && Len >= (1L << 30))
	{
	  pte_t *pL3p = Sv48GetL3p (pRoot, Va, Payload);

	  SetPte (pL3p, Pa >> PAGE_SHIFT,
		   (X ? PTE_X : 0) | PTE_W | PTE_R | PTE_V);
	  Va += (1L << 30);
	  Pa += (1L << 30);
	  Len -= (1L << 30);
	  L3cnt++;
	}
      else if (MB2ALIGNED (Pa) && MB2ALIGNED (Va) && Len >= (1 << 21))
	{
	  pte_t *pL2p = Sv48GetL2p (pRoot, Va, Payload);

	  SetPte (pL2p, Pa >> PAGE_SHIFT,
		   (X ? PTE_X : 0) | PTE_W | PTE_R | PTE_V);
	  Va += (1L << 21);
	  Pa += (1L << 21);
	  Len -= (1L << 21);
	  L2cnt++;
	}
      else
	{
	  pte_t *pL1p = Sv48GetL1p (pRoot, Va, Payload);

	  SetPte (pL1p, Pa >> PAGE_SHIFT,
		   (X ? PTE_X : 0) | PTE_W | PTE_R | PTE_V);
	  Va += (1L << PAGE_SHIFT);
	  Pa += (1L << PAGE_SHIFT);
	  Len -= (1L << PAGE_SHIFT);
	  L1cnt++;
	}
    }
}

/**
  Create physical memory map.

  Maps physical memory region into virtual address space.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
  @param[in] Pa    Physical address.
  @param[in] Mt    Memory type.
**/
VOID
Sv48Physmap (
  IN vaddr_t           Va,
  IN size64_t          Size,
  IN UINT64            Pa,
  IN enum memory_type  Mt
  )
{
  Sv48DirectMap (gSv48Root, Pa, Va, Size, Mt, 1, 0);
}

/**
  Allocate top-level page tables.

  Pre-allocates L3 page tables for a virtual address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
Sv48TopPtAlloc (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  UINT32 i, n;

  n = (Size + (1 << 30) - 1) >> 30;

  for (i = 0; i < n; i++)
    (VOID) Sv48GetL3p (gSv48Root, Va + (i << PAGE_SHIFT), 1);
}

/**
  Allocate page tables.

  Pre-allocates L1 page tables for a virtual address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
Sv48PtAlloc (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  UINT32 i, n;

  n = Size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    (VOID) Sv48GetL1p (gSv48Root, Va + (i << PAGE_SHIFT), 1);
}

#define SV48_LINEAR_SHIFT (PAGE_SHIFT + 9 + 9 + 9)
#define SV48_LINEAR_SIZE  (1LL << SV48_LINEAR_SHIFT)
#define SV48_LINEAR_ALIGN (SV48_LINEAR_SIZE - 1)

/**
  Create linear (recursive) mapping.

  Maps page table recursively for linear address space access.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
Sv48Linear (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  UINT32 L4off = L4OFF64 (Va);
  pte_t *pL4p;

  if (Va & SV48_LINEAR_ALIGN)
    {
      printf ("PAE Linear VA %llx not aligned (align mask: %llx).\n",
	      Va, SV48_LINEAR_ALIGN);
      exit (-1);
    }

  if (Size < SV48_LINEAR_SIZE)
    {
      printf ("PAE Linear size %llx too small.\n", Size);
      exit (-1);
    }

  pL4p = gSv48Root + L4off;
  SetPte (pL4p, (UINTN) gSv48Root >> PAGE_SHIFT, PTE_V);
  printf ("Wrote %llx at %p\n", *pL4p, pL4p);
}

/**
  Populate virtual address range.

  Allocates and maps pages for a virtual address range with
  specified attributes.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
  @param[in] U     TRUE if user-accessible.
  @param[in] W     TRUE if writable.
  @param[in] X     TRUE if executable.
**/
VOID
Sv48Populate (
  IN vaddr_t   Va,
  IN size64_t  Size,
  IN INT32     U,
  IN INT32     W,
  IN INT32     X
  )
{
  ssize64_t Len = Size;

  while (Len > 0)
    {
      Sv48PopulatePage (gSv48Root, Va, U, W, X, 1);

      Len -= PAGE_CEILING (Va) - Va;
      Va += PAGE_CEILING (Va) - Va;

    }
}

/**
  Jump to kernel entry point.

  Transfers control to the loaded kernel with SV48 paging enabled.

  @param[in] Entry  Kernel entry point virtual address.
**/
VOID
Sv48Entry (
  IN vaddr_t  Entry
  )
{
  MdEntry (ARCH_RISCV64, (vaddr_t) (UINTN) gSv48Root, Entry);
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use SetPte instead **/
static void set_pte (pte_t *ptep, uint64_t pfn, uint64_t flags) {
  SetPte (ptep, pfn, flags);
}

/** @deprecated Use PteGetAddr instead **/
static void *pte_getaddr (pte_t *ptep) {
  return PteGetAddr (ptep);
}

/** @deprecated Use PteGetFlags instead **/
static uint64_t pte_getflags (pte_t *ptep) {
  return PteGetFlags (ptep);
}

/** @deprecated Use PteMergeFlags instead **/
static uint64_t pte_mergeflags (uint64_t fl1, uint64_t fl2) {
  return PteMergeFlags (fl1, fl2);
}

/** @deprecated Use Sv48GetL3p instead **/
static pte_t *sv48_get_l3p (pte_t *root, vaddr_t va, int payload) {
  return Sv48GetL3p (root, va, payload);
}

/** @deprecated Use Sv48GetL2p instead **/
static pte_t *sv48_get_l2p (pte_t *root, vaddr_t va, int payload) {
  return Sv48GetL2p (root, va, payload);
}

/** @deprecated Use Sv48GetL1p instead **/
static pte_t *sv48_get_l1p (pte_t *root, vaddr_t va, int payload) {
  return Sv48GetL1p (root, va, payload);
}

/** @deprecated Use Sv48Verify instead **/
void sv48_verify (vaddr_t va, size64_t size) {
  Sv48Verify (va, size);
}

/** @deprecated Use Sv48Initialize instead **/
void sv48_init (void) {
  Sv48Initialize ();
}

/** @deprecated Use Sv48MapPage instead **/
void sv48_map_page (void *pt, vaddr_t va, uintptr_t pa, int payload, int w, int x) {
  Sv48MapPage (pt, va, pa, payload, w, x);
}

/** @deprecated Use Sv48PopulatePage instead **/
static uintptr_t sv48_populate_page (pte_t *root, vaddr_t va, int u, int w, int x, int payload) {
  return Sv48PopulatePage (root, va, u, w, x, payload);
}

/** @deprecated Use Sv48GetPhys instead **/
uintptr_t sv48_getphys (vaddr_t va) {
  return Sv48GetPhys (va);
}

/** @deprecated Use Sv48DirectMap instead **/
void sv48_directmap (void *pt, uint64_t pabase, vaddr_t va, size64_t size,
		     enum memory_type mt, int payload, int x) {
  Sv48DirectMap (pt, pabase, va, size, mt, payload, x);
}

/** @deprecated Use Sv48Physmap instead **/
void sv48_physmap (vaddr_t va, size64_t size, uint64_t pa, enum memory_type mt) {
  Sv48Physmap (va, size, pa, mt);
}

/** @deprecated Use Sv48TopPtAlloc instead **/
void sv48_topptalloc (vaddr_t va, size64_t size) {
  Sv48TopPtAlloc (va, size);
}

/** @deprecated Use Sv48PtAlloc instead **/
void sv48_ptalloc (vaddr_t va, size64_t size) {
  Sv48PtAlloc (va, size);
}

/** @deprecated Use Sv48Linear instead **/
void sv48_linear (vaddr_t va, size64_t size) {
  Sv48Linear (va, size);
}

/** @deprecated Use Sv48Populate instead **/
void sv48_populate (vaddr_t va, size64_t size, int u, int w, int x) {
  Sv48Populate (va, size, u, w, x);
}

/** @deprecated Use Sv48Entry instead **/
void sv48_entry (vaddr_t entry) {
  Sv48Entry (entry);
}

// Legacy global variable aliases
static pte_t *sv48_root __attribute__((alias("gSv48Root")));
