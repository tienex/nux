/** @file
  APXH x86 PAE32 Paging Implementation

  32-bit Physical Address Extension (PAE) paging implementation using
  3-level page tables (PDPT, PD, PT). Supports 36-bit physical addressing
  with 4KB pages on 32-bit x86 processors.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/x86/pae.h>

/* 1 Gb direct map in the Payload Page Table. */
#define PAE_DIRECTMAP_START 0
#define PAE_DIRECTMAP_END   (1LL << 30)

#define L3OFF(_va) (((_va) >> 30) & 0x3)
#define L2OFF(_va) (((_va) >> 21) & 0x1ff)
#define L1OFF(_va) (((_va) >> 12) & 0x1ff)

static pte_t *gPaeCr3;
static pte_t *gL2s[4];

/**
  Set PTE value.

  Writes a page table entry with the specified page frame number
  and flags.

  @param[out] Ptep  Pointer to PTE.
  @param[in]  Pfn    Page frame number.
  @param[in]  Flags  PTE flags.
**/
static VOID
SetPte (
  OUT pte_t   *Ptep,
  IN  UINT64  Pfn,
  IN  UINT64  Flags
  )
{
  *Ptep = (Pfn << PAGE_SHIFT) | Flags;
}

/**
  Get physical address from PTE.

  Extracts the physical address from a page table entry.
  Returns NULL if PTE is not present.

  @param[in] Ptep  Pointer to PTE.

  @return Physical address from PTE, or NULL if not present.
**/
static VOID *
PteGetAddr (
  IN pte_t  *Ptep
  )
{
  pte_t Pte = *Ptep;

  if (!(Pte & PTE_P))
    return NULL;

  return (VOID *) (uintptr_t) (Pte & 0x7ffffffffffff000LL);
}

/**
  Get flags from PTE.

  Extracts the flag bits from a page table entry.

  @param[in] Ptep  Pointer to PTE.

  @return PTE flags.
**/
static UINT64
PteGetFlags (
  IN pte_t  *Ptep
  )
{
  pte_t Pte = *Ptep;

  return Pte & ~0x7ffffffffffff000LL;
}

/**
  Merge two PTE flag sets.

  Combines flags from two PTEs with proper handling of Present,
  User, Write, and NX bits. Ensures consistent user/kernel mode
  and computes most permissive flags.

  @param[in] Fl1  First flag set.
  @param[in] Fl2  Second flag set.

  @return Merged flags.
**/
static UINT64
PteMergeFlags (
  IN UINT64  Fl1,
  IN UINT64  Fl2
  )
{
  UINT64 NewF;


  //PTE_P always present
  assert (Fl1 & Fl2 & PTE_P);
  NewF = PTE_P;

  //PTE_U is either present on both or absent on both
  if ((Fl1 & PTE_U) != (Fl2 & PTE_U))
    fatal ("Mixed user/kernel addresses not allowed.");
  NewF |= Fl1 & PTE_U;

  // Write must be OR'd.
  NewF |= (Fl1 | Fl2) & PTE_W;

  // NX must be AND'd.
  NewF |= Fl1 & Fl2 & PTE_NX;

#if 0
  printf ("Merging: %llx (+) %llx = %llx\n", Fl1, Fl2, NewF);
#endif
  return NewF;
}

/**
  Verify PAE address range.

  Validates that a virtual address range is suitable for PAE paging.
  Currently performs no checks.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
PaeVerify (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  /* Nothing to check. */
}

/**
  Initialize PAE paging.

  Sets up PAE (Physical Address Extension) paging with 3-level page
  tables. Allocates CR3 and 4 L2 page directory tables. Enables NX
  if supported and configures PAT table.
**/
VOID
PaeInitialize (
  VOID
  )
{
  int i;

  /* Enable NX */
  assert (CpuSupportsPae ());

  gNxEnabled = CpuSupportsNx ();

  /* In PAE is only 64 bytes, but we allocate a full page for it. */
  gPaeCr3 = (pte_t *) get_payload_page ();

  /* Set PDPTEs */
  for (i = 0; i < 4; i++)
    {
      uintptr_t L2Page = get_payload_page ();

      SetPte (gPaeCr3 + i, L2Page >> PAGE_SHIFT, PTE_P);
      gL2s[i] = (pte_t *) L2Page;
    }

  SetupPatTable ();

  printf ("Using PAE paging (CR3: %08lx, NX: %d).\n", gPaeCr3, gNxEnabled);
}

/**
  Get L2 page directory entry pointer (PAE).

  Walks PAE page tables to L2 level, allocating missing levels.

  @param[in] pCr3     Page table root.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE to allocate from payload pages.

  @return Pointer to L2 PTE.
**/
static pte_t *
PaeGetL2p (
  IN pte_t    *pCr3,
  IN vaddr_t  Va,
  IN int      Payload
  )
{
  pte_t *pL3p, *pL2p;
  unsigned L3Off = L3OFF (Va);
  unsigned L2Off = L2OFF (Va);

  pL3p = pCr3 + L3Off;

  pL2p = (pte_t *) PteGetAddr (pL3p);
  if (pL2p == NULL)
    {
      uintptr_t L2Page;

      /* Populating L2. */
      L2Page = Payload ? get_payload_page () : get_page ();

      SetPte (pL3p, L2Page >> PAGE_SHIFT, PTE_P);
      pL2p = (pte_t *) L2Page;
    }

  return pL2p + L2Off;
}

/**
  Get L1 page table entry pointer (PAE).

  Walks PAE page tables to L1 level, allocating missing levels.

  @param[in] pCr3     Page table root.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE to allocate from payload pages.

  @return Pointer to L1 PTE.
**/
static pte_t *
PaeGetL1p (
  IN pte_t    *pCr3,
  IN vaddr_t  Va,
  IN int      Payload
  )
{
  pte_t *pL2p, *pL1p;
  unsigned L1Off = L1OFF (Va);

  pL2p = PaeGetL2p (pCr3, Va, Payload);

  pL1p = (pte_t *) PteGetAddr (pL2p);
  if (pL1p == NULL)
    {
      uintptr_t L1Page;

      /* Populating L1. */
      L1Page = Payload ? get_payload_page () : get_page ();

      SetPte (pL2p, L1Page >> PAGE_SHIFT, PTE_U | PTE_W | PTE_P);
      pL1p = (pte_t *) L1Page;
    }

  return pL1p + L1Off;
}

/**
  Map page with PAE.

  Creates a page mapping with specified permissions.

  @param[in] Pt      Page table root.
  @param[in] Va       Virtual address.
  @param[in] Pa       Physical address.
  @param[in] Payload  TRUE to allocate from payload pages.
  @param[in] W        TRUE for writable.
  @param[in] X        TRUE for executable.
**/
VOID
PaeMapPage (
  IN VOID      *Pt,
  IN vaddr_t   Va,
  IN uintptr_t Pa,
  IN int       Payload,
  IN int       W,
  IN int       X
  )
{
  pte_t *pL1p, *pCr3;
  UINT64 L1F;
  uintptr_t Page;

  pCr3 = (pte_t *) Pt;

  pL1p = PaeGetL1p (pCr3, Va, Payload);
  L1F = (W ? PTE_W : 0) | (X ? 0 : PTE_NX) | PTE_P;

  Page = (uintptr_t) PteGetAddr (pL1p);
  assert (Page == 0);
  Page = Pa >> PAGE_SHIFT;
  SetPte (pL1p, Page, L1F);
}

/**
  Populate page with PAE.

  Allocates and maps a page with specified permissions, or updates
  flags if page already exists.

  @param[in] Va  Virtual address.
  @param[in] U   TRUE for user-accessible.
  @param[in] W   TRUE for writable.
  @param[in] X   TRUE for executable.

  @return Physical address of page.
**/
static uintptr_t
PaePopulatePage (
  IN vaddr_t  Va,
  IN int      U,
  IN int      W,
  IN int      X
  )
{
  pte_t *pL1p;
  UINT64 L1F;
  uintptr_t Page;

  pL1p = PaeGetL1p (gPaeCr3, Va, 1);

  L1F = (U ? PTE_U : 0) | (W ? PTE_W : 0) | (X ? 0 : PTE_NX) | PTE_P;

  Page = (uintptr_t) PteGetAddr (pL1p);
  if (PteGetAddr (pL1p) == NULL)
    {
      Page = get_payload_page ();
      SetPte (pL1p, Page >> PAGE_SHIFT, L1F);
    }
  else
    {
      UINT64 NewF;
      UINT64 OldF = PteGetFlags (pL1p);

      NewF = PteMergeFlags (L1F, OldF);
      if (NewF != OldF)
	{
	  printf ("Flags changed from %llx to %llx\n", OldF, NewF);
	  SetPte (pL1p, Page >> PAGE_SHIFT, NewF);
	}
    }

  return Page;
}

/**
  Get physical address (PAE).

  Translates virtual address to physical address using current
  PAE page tables.

  @param[in] Va  Virtual address.

  @return Physical address.
**/
uintptr_t
PaeGetPhys (
  IN vaddr_t  Va
  )
{
  uintptr_t Page;
  pte_t *pL2e, *pL1, *pL1e;
  unsigned L3Off = L3OFF (Va);
  unsigned L2Off = L2OFF (Va);
  unsigned L1Off = L1OFF (Va);

  pL2e = gL2s[L3Off] + L2Off;

  pL1 = (pte_t *) PteGetAddr (pL2e);
  assert (pL1 != NULL);

  pL1e = pL1 + L1Off;

  Page = (uintptr_t) PteGetAddr (pL1e);
  assert (Page != 0);

  return Page | (Va & ~(PAGE_MASK));
}

/**
  Direct map memory region (PAE).

  Creates direct 1:1 mapping of physical memory with specified type
  and permissions using 4KB pages.

  @param[in] Pt      Page table root.
  @param[in] Pa       Physical address base.
  @param[in] Va       Virtual address base.
  @param[in] Size     Size of region.
  @param[in] Mt       Memory type (WC, WB, UC).
  @param[in] Payload  TRUE to allocate from payload pages.
  @param[in] X        TRUE for executable.
**/
VOID
PaeDirectMap (
  IN VOID              *Pt,
  IN UINT64            Pa,
  IN vaddr_t           Va,
  IN size64_t          Size,
  IN enum memory_type  Mt,
  IN int               Payload,
  IN int               X
  )
{
  UINT64 PaPfn = Pa >> PAGE_SHIFT;
  unsigned i, n;
  pte_t *Pte;

  n = Size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    {
      Pte = PaeGetL1p (Pt, Va + (i << PAGE_SHIFT), Payload);
      SetPte (Pte, PaPfn + i,
	       MemtypeToFlags (Mt, true /*4k */ ) | PTE_P | PTE_W | (X ? 0 :
								       PTE_NX));
    }
}

/**
  Map physical memory (PAE).

  Creates mapping of physical memory region using current PAE root.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
  @param[in] Pa    Physical address.
  @param[in] Mt    Memory type.
**/
VOID
PaePhysmap (
  IN vaddr_t           Va,
  IN size64_t          Size,
  IN UINT64            Pa,
  IN enum memory_type  Mt
  )
{
  PaeDirectMap (gPaeCr3, Pa, Va, Size, Mt, 1, 0);
}

/**
  Allocate top-level page tables (PAE).

  Pre-allocates page table structures for address range.
  In PAE, equivalent to PTALLOC since there's no L4 level.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
PaeTopPtAlloc (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  /* In PAE, TOPPTALLOC is equivalent to PTALLOC. */

  PaePtAlloc (Va, Size);
}

/**
  Allocate page tables (PAE).

  Pre-allocates L1 page tables for address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
PaePtAlloc (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  unsigned i, n;

  n = Size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    (void) PaeGetL1p (gPaeCr3, Va + (i << PAGE_SHIFT), 1);
}

#define PAE_LINEAR_SHIFT (PAGE_SHIFT + 9 + 2)
#define PAE_LINEAR_SIZE (1L << PAE_LINEAR_SHIFT)
#define PAE_LINEAR_ALIGN (PAE_LINEAR_SIZE - 1)

/**
  Set up linear (recursive) mapping (PAE).

  Creates recursive page table mapping allowing page tables to be
  accessed as regular memory. Requires specific alignment.

  @param[in] Va    Virtual address for linear mapping.
  @param[in] Size  Size of region (must be >= PAE_LINEAR_SIZE).
**/
VOID
PaeLinear (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  int i;
  unsigned L3Off = L3OFF (Va);
  unsigned L2Off = L2OFF (Va);

  if (Va & PAE_LINEAR_ALIGN)
    {
      printf ("PAE Linear VA %llx not aligned (align mask: %lx).\n", Va,
	      PAE_LINEAR_ALIGN);
      exit (-1);
    }

  if (Size < PAE_LINEAR_SIZE)
    {
      printf ("PAE Linear size %llx too small.\n", Size);
      exit (-1);
    }

  for (i = 0; i < 4; i++)
    {
      pte_t *pL2p;

      pL2p = gL2s[L3Off] + L2Off + i;
      SetPte (pL2p, (UINT64) (uintptr_t) gL2s[i] >> PAGE_SHIFT,
	       PTE_NX | PTE_W | PTE_P);
    }
}

/**
  Populate memory region (PAE).

  Allocates and maps pages for entire region with specified
  permissions.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
  @param[in] U     TRUE for user-accessible.
  @param[in] W     TRUE for writable.
  @param[in] X     TRUE for executable.
**/
VOID
PaePopulate (
  IN vaddr_t   Va,
  IN size64_t  Size,
  IN int       U,
  IN int       W,
  IN int       X
  )
{
  ssize64_t Len = Size;

  while (Len > 0)
    {
      PaePopulatePage (Va, U, W, X);

      Len -= PAGE_CEILING (Va) - Va;
      Va += PAGE_CEILING (Va) - Va;

    }
}

/**
  Transfer control to PAE kernel.

  Prepares final environment and transfers control to kernel entry
  point using PAE paging mode.

  @param[in] Entry  Kernel entry point address.
**/
VOID
PaeEntry (
  IN vaddr_t  Entry
  )
{
  md_entry (ARCH_386, (vaddr_t) (uintptr_t) gPaeCr3, Entry);
}


/*
  AMD64 PAE support.
*/

