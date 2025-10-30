/** @file
  APXH x86 PAE64 Paging Implementation

  64-bit Physical Address Extension (PAE64/AMD64) paging implementation
  using 4-level page tables (PML4, PDPT, PD, PT). Supports 48-bit virtual
  addressing on AMD64/x86-64 processors.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/x86/pae.h>

#define PAE64_DIRECTMAP_START 0
#define PAE64_DIRECTMAP_END   (1LL << 30)

#define L4OFF64(_va) (((_va) >> 39) & 0x1ff)
#define L3OFF64(_va) (((_va) >> 30) & 0x1ff)
#define L2OFF64(_va) (((_va) >> 21) & 0x1ff)
#define L1OFF64(_va) (((_va) >> 12) & 0x1ff)

static pte_t *gPae64Cr3;

/**
  Get L3 page directory pointer entry (PAE64).

  Walks PAE64 (AMD64) page tables to L3 level, allocating missing
  levels.

  @param[in] pCr3     Page table root.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE to allocate from payload pages.

  @return Pointer to L3 PTE.
**/
static pte_t *
Pae64GetL3p (
  IN pte_t    *pCr3,
  IN vaddr_t  Va,
  IN int      Payload
  )
{
  pte_t *pL4p, *pL3p;
  unsigned L4Off = L4OFF64 (Va);
  unsigned L3Off = L3OFF64 (Va);

  pL4p = pCr3 + L4Off;

  pL3p = (pte_t *) PteGetAddr (pL4p);
  if (pL3p == NULL)
    {
      uintptr_t L3Page;

      /* Populating L3. */
      L3Page = Payload ? get_payload_page () : get_page ();

      SetPte (pL4p, L3Page >> PAGE_SHIFT, PTE_U | PTE_W | PTE_P);
      pL3p = (pte_t *) L3Page;
    }

  return pL3p + L3Off;
}

/**
  Get L2 page directory entry (PAE64).

  Walks PAE64 (AMD64) page tables to L2 level, allocating missing
  levels.

  @param[in] pCr3     Page table root.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE to allocate from payload pages.

  @return Pointer to L2 PTE.
**/
static pte_t *
Pae64GetL2p (
  IN pte_t    *pCr3,
  IN vaddr_t  Va,
  IN int      Payload
  )
{
  pte_t *pL3p, *pL2p;

  unsigned L2Off = L2OFF64 (Va);

  pL3p = Pae64GetL3p (pCr3, Va, Payload);

  pL2p = (pte_t *) PteGetAddr (pL3p);
  if (pL2p == NULL)
    {
      uintptr_t L2Page;

      /* Populating L2. */
      L2Page = Payload ? get_payload_page () : get_page ();

      SetPte (pL3p, L2Page >> PAGE_SHIFT, PTE_U | PTE_W | PTE_P);
      pL2p = (pte_t *) L2Page;
    }

  return pL2p + L2Off;
}

/**
  Get L1 page table entry (PAE64).

  Walks PAE64 (AMD64) page tables to L1 level, allocating missing
  levels.

  @param[in] pCr3     Page table root.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE to allocate from payload pages.

  @return Pointer to L1 PTE.
**/
static pte_t *
Pae64GetL1p (
  IN pte_t    *pCr3,
  IN vaddr_t  Va,
  IN int      Payload
  )
{
  pte_t *pL2p, *pL1p;
  unsigned L1Off = L1OFF64 (Va);

  pL2p = Pae64GetL2p (pCr3, Va, Payload);

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
  Verify PAE64 address range.

  Validates that a virtual address range is suitable for PAE64
  paging. Currently performs no checks.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
Pae64Verify (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  /* Nothing to check. */
}

/**
  Initialize PAE64 paging.

  Sets up PAE64 (AMD64) paging with 4-level page tables. Enables
  NX if supported and configures PAT table.
**/
VOID
Pae64Initialize (
  VOID
  )
{
  assert (CpuSupportsLongmode ());

  gNxEnabled = CpuSupportsNx ();

  SetupPatTable ();

  gPae64Cr3 = (pte_t *) get_payload_page ();

  printf ("Using PAE64 paging (CR3: %08lx, NX: %d).\n", gPae64Cr3,
	  gNxEnabled);
}

/**
  Map page with PAE64.

  Creates a page mapping with specified permissions.

  @param[in] Pt      Page table root.
  @param[in] Va       Virtual address.
  @param[in] Pa       Physical address.
  @param[in] Payload  TRUE to allocate from payload pages.
  @param[in] W        TRUE for writable.
  @param[in] X        TRUE for executable.
**/
VOID
Pae64MapPage (
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

  printf ("Mapping at va %llx PA %lx (p:%d, w:%d, x:%d)\n", Va, Pa, Payload,
	  W, X);

  pL1p = Pae64GetL1p (pCr3, Va, Payload);
  L1F = (W ? PTE_W : 0) | (X ? 0 : PTE_NX) | PTE_P;

  Page = (uintptr_t) PteGetAddr (pL1p);
  assert (Page == 0);
  Page = Pa >> PAGE_SHIFT;
  SetPte (pL1p, Page, L1F);
}

/**
  Populate page with PAE64.

  Allocates and maps a page with specified permissions, or updates
  flags if page already exists.

  @param[in] pCr3     Page table root.
  @param[in] Va       Virtual address.
  @param[in] U        TRUE for user-accessible.
  @param[in] W        TRUE for writable.
  @param[in] X        TRUE for executable.
  @param[in] Payload  TRUE to allocate from payload pages.

  @return Physical address of page.
**/
static uintptr_t
Pae64PopulatePage (
  IN pte_t    *pCr3,
  IN vaddr_t  Va,
  IN int      U,
  IN int      W,
  IN int      X,
  IN int      Payload
  )
{
  pte_t *pL1p;
  UINT64 L1F;
  uintptr_t Page;

  pL1p = Pae64GetL1p (pCr3, Va, Payload);
  L1F = (U ? PTE_U : 0) | (W ? PTE_W : 0) | (X ? 0 : PTE_NX) | PTE_P;

  Page = (uintptr_t) PteGetAddr (pL1p);
  if (Page == 0)
    {
      Page = Payload ? get_payload_page () : get_page ();
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
  Get physical address (PAE64).

  Translates virtual address to physical address using current
  PAE64 page tables.

  @param[in] Va  Virtual address.

  @return Physical address.
**/
uintptr_t
Pae64GetPhys (
  IN vaddr_t  Va
  )
{
  uintptr_t Page;
  pte_t *pL4p, *pL3p, *pL2p, *pL1p;
  unsigned L4Off = L4OFF64 (Va);
  unsigned L3Off = L3OFF64 (Va);
  unsigned L2Off = L2OFF64 (Va);
  unsigned L1Off = L1OFF64 (Va);

  pL4p = gPae64Cr3 + L4Off;

  pL3p = (pte_t *) PteGetAddr (pL4p);
  assert (pL3p != NULL);
  pL3p += L3Off;

  pL2p = (pte_t *) PteGetAddr (pL3p);
  assert (pL2p != NULL);
  pL2p += L2Off;

  pL1p = (pte_t *) PteGetAddr (pL2p);
  assert (pL1p != NULL);
  pL1p += L1Off;

  Page = (uintptr_t) PteGetAddr (pL1p);
  assert (Page != 0);

  return Page |= (Va & ~(PAGE_MASK));
}

/**
  Direct map memory region (PAE64).

  Creates direct 1:1 mapping of physical memory with specified type
  and permissions. Uses 1GB, 2MB, or 4KB pages depending on
  alignment and size.

  @param[in] Pt      Page table root.
  @param[in] PaBase   Physical address base.
  @param[in] Va       Virtual address base.
  @param[in] Size     Size of region.
  @param[in] Mt       Memory type (WC, WB, UC).
  @param[in] Payload  TRUE to allocate from payload pages.
  @param[in] X        TRUE for executable.
**/
VOID
Pae64DirectMap (
  IN VOID              *Pt,
  IN UINT64            PaBase,
  IN vaddr_t           Va,
  IN size64_t          Size,
  IN enum memory_type  Mt,
  IN int               Payload,
  IN int               X
  )
{
  ssize64_t Len;
  UINT64 Pa;
  pte_t *pCr3 = (pte_t *) Pt;
  int P1G = CpuSupports1gbPages ();
  unsigned long L3Cnt = 0, L2Cnt = 0, L1Cnt = 0;

#define GB1ALIGNED(_a) (((_a) & ((1L << 30) - 1)) == 0)
#define MB2ALIGNED(_a) (((_a) & ((1L << 21) - 1)) == 0)


  /* Signed to unsigned: no one will ask us a 1<<64 bytes physmap. */
  Len = (ssize64_t) Size;
  Pa = PaBase;

  while (Len > 0)
    {

      if (P1G && GB1ALIGNED (Pa) && GB1ALIGNED (Va) && Len >= (1L << 30))
	{
	  pte_t *pL3p = Pae64GetL3p (pCr3, Va, Payload);

	  SetPte (pL3p, Pa >> PAGE_SHIFT,
		   MemtypeToFlags (Mt, false /*1GB */ ) |
		   PTE_PS | PTE_W | PTE_P | (X ? 0 : PTE_NX));
	  Va += (1L << 30);
	  Pa += (1L << 30);
	  Len -= (1L << 30);
	  L3Cnt++;
	}
      else if (MB2ALIGNED (Pa) && MB2ALIGNED (Va) && Len >= (1 << 21))
	{
	  pte_t *pL2p = Pae64GetL2p (pCr3, Va, Payload);

	  SetPte (pL2p, Pa >> PAGE_SHIFT,
		   MemtypeToFlags (Mt, false /* 2MB */ ) |
		   PTE_PS | PTE_W | PTE_P | (X ? 0 : PTE_NX));
	  Va += (1L << 21);
	  Pa += (1L << 21);
	  Len -= (1L << 21);
	  L2Cnt++;
	}
      else
	{
	  pte_t *pL1p = Pae64GetL1p (pCr3, Va, Payload);

	  SetPte (pL1p, Pa >> PAGE_SHIFT,
		   MemtypeToFlags (Mt,
				     true /*4kB */ ) | PTE_W | PTE_P | (X ? 0
									:
									PTE_NX));
	  Va += (1L << PAGE_SHIFT);
	  Pa += (1L << PAGE_SHIFT);
	  Len -= (1L << PAGE_SHIFT);
	  L1Cnt++;
	}
    }
}

/**
  Map physical memory (PAE64).

  Creates mapping of physical memory region using current PAE64 root.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
  @param[in] Pa    Physical address.
  @param[in] Mt    Memory type.
**/
VOID
Pae64Physmap (
  IN vaddr_t           Va,
  IN size64_t          Size,
  IN UINT64            Pa,
  IN enum memory_type  Mt
  )
{
  Pae64DirectMap (gPae64Cr3, Pa, Va, Size, Mt, 1, 0);
}

/**
  Allocate top-level page tables (PAE64).

  Pre-allocates L3 page directory pointer tables for address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
Pae64TopPtAlloc (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  unsigned i, n;

  n = (Size + (1 << 30) - 1) >> 30;

  for (i = 0; i < n; i++)
    (void) Pae64GetL3p (gPae64Cr3, Va + (i << PAGE_SHIFT), 1);
}

/**
  Allocate page tables (PAE64).

  Pre-allocates L1 page tables for address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
Pae64PtAlloc (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  unsigned i, n;

  n = Size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    (void) Pae64GetL1p (gPae64Cr3, Va + (i << PAGE_SHIFT), 1);
}

#define PAE64_LINEAR_SHIFT (PAGE_SHIFT + 9 + 9 + 9)
#define PAE64_LINEAR_SIZE  (1LL << PAE64_LINEAR_SHIFT)
#define PAE64_LINEAR_ALIGN (PAE64_LINEAR_SIZE - 1)

/**
  Set up linear (recursive) mapping (PAE64).

  Creates recursive page table mapping allowing page tables to be
  accessed as regular memory. Requires specific alignment.

  @param[in] Va    Virtual address for linear mapping.
  @param[in] Size  Size of region (must be >= PAE64_LINEAR_SIZE).
**/
VOID
Pae64Linear (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  unsigned L4Off = L4OFF64 (Va);
  pte_t *pL4p;

  if (Va & PAE64_LINEAR_ALIGN)
    {
      printf ("PAE Linear VA %llx not aligned (align mask: %llx).\n",
	      Va, PAE64_LINEAR_ALIGN);
      exit (-1);
    }

  if (Size < PAE64_LINEAR_SIZE)
    {
      printf ("PAE Linear size %llx too small.\n", Size);
      exit (-1);
    }

  pL4p = gPae64Cr3 + L4Off;
  SetPte (pL4p, (uintptr_t) gPae64Cr3 >> PAGE_SHIFT, PTE_W | PTE_P);
  printf ("Wrote %llx at %p\n", *pL4p, pL4p);
}

/**
  Populate memory region (PAE64).

  Allocates and maps pages for entire region with specified
  permissions.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
  @param[in] U     TRUE for user-accessible.
  @param[in] W     TRUE for writable.
  @param[in] X     TRUE for executable.
**/
VOID
Pae64Populate (
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
      Pae64PopulatePage (gPae64Cr3, Va, U, W, X, 1);

      Len -= PAGE_CEILING (Va) - Va;
      Va += PAGE_CEILING (Va) - Va;

    }
}

/**
  Transfer control to PAE64 kernel.

  Prepares final environment and transfers control to kernel entry
  point using PAE64 (AMD64) long mode paging.

  @param[in] Entry  Kernel entry point address.
**/
VOID
Pae64Entry (
  IN vaddr_t  Entry
  )
{
  md_entry (ARCH_AMD64, (vaddr_t) (uintptr_t) gPae64Cr3, Entry);
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use ScanPatTable instead **/
static void scan_pat_table (void) {
  ScanPatTable ();
}

/** @deprecated Use SetupPatTable instead **/
static void setup_pat_table (void) {
  SetupPatTable ();
}

/** @deprecated Use MemtypeToFlags instead **/
static unsigned memtype_to_flags (enum memory_type mt, bool small) {
  return MemtypeToFlags (mt, small);
}

/** @deprecated Use CpuIsIntel instead **/
bool cpu_is_intel (void) {
  return CpuIsIntel ();
}

/** @deprecated Use IntelCpuFamily instead **/
unsigned intel_cpu_family (void) {
  return IntelCpuFamily ();
}

/** @deprecated Use IntelCpuModel instead **/
unsigned intel_cpu_model (void) {
  return IntelCpuModel ();
}

/** @deprecated Use CpuSupportsPae instead **/
bool cpu_supports_pae (void) {
  return CpuSupportsPae ();
}

/** @deprecated Use CpuSupportsLongmode instead **/
bool cpu_supports_longmode (void) {
  return CpuSupportsLongmode ();
}

/** @deprecated Use CpuSupports1gbPages instead **/
bool cpu_supports_1gbpages (void) {
  return CpuSupports1gbPages ();
}

/** @deprecated Use CpuSupportsNx instead **/
bool cpu_supports_nx (void) {
  return CpuSupportsNx ();
}

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

/** @deprecated Use PaeVerify instead **/
void pae_verify (vaddr_t va, size64_t size) {
  PaeVerify (va, size);
}

/** @deprecated Use PaeInitialize instead **/
void pae_init (void) {
  PaeInitialize ();
}

/** @deprecated Use PaeGetL2p instead **/
static pte_t *pae_get_l2p (pte_t *cr3, vaddr_t va, int payload) {
  return PaeGetL2p (cr3, va, payload);
}

/** @deprecated Use PaeGetL1p instead **/
static pte_t *pae_get_l1p (pte_t *cr3, vaddr_t va, int payload) {
  return PaeGetL1p (cr3, va, payload);
}

/** @deprecated Use PaeMapPage instead **/
void pae_map_page (void *pt, vaddr_t va, uintptr_t pa, int payload, int w, int x) {
  PaeMapPage (pt, va, pa, payload, w, x);
}

/** @deprecated Use PaePopulatePage instead **/
static uintptr_t pae_populate_page (vaddr_t va, int u, int w, int x) {
  return PaePopulatePage (va, u, w, x);
}

/** @deprecated Use PaeGetPhys instead **/
uintptr_t pae_getphys (vaddr_t va) {
  return PaeGetPhys (va);
}

/** @deprecated Use PaeDirectMap instead **/
void pae_directmap (void *pt, uint64_t pa, vaddr_t va, size64_t size,
		    enum memory_type mt, int payload, int x) {
  PaeDirectMap (pt, pa, va, size, mt, payload, x);
}

/** @deprecated Use PaePhysmap instead **/
void pae_physmap (vaddr_t va, size64_t size, uint64_t pa, enum memory_type mt) {
  PaePhysmap (va, size, pa, mt);
}

/** @deprecated Use PaeTopPtAlloc instead **/
void pae_topptalloc (vaddr_t va, size64_t size) {
  PaeTopPtAlloc (va, size);
}

/** @deprecated Use PaePtAlloc instead **/
void pae_ptalloc (vaddr_t va, size64_t size) {
  PaePtAlloc (va, size);
}

/** @deprecated Use PaeLinear instead **/
void pae_linear (vaddr_t va, size64_t size) {
  PaeLinear (va, size);
}

/** @deprecated Use PaePopulate instead **/
void pae_populate (vaddr_t va, size64_t size, int u, int w, int x) {
  PaePopulate (va, size, u, w, x);
}

/** @deprecated Use PaeEntry instead **/
void pae_entry (vaddr_t entry) {
  PaeEntry (entry);
}

/** @deprecated Use Pae64GetL3p instead **/
static pte_t *pae64_get_l3p (pte_t *cr3, vaddr_t va, int payload) {
  return Pae64GetL3p (cr3, va, payload);
}

/** @deprecated Use Pae64GetL2p instead **/
static pte_t *pae64_get_l2p (pte_t *cr3, vaddr_t va, int payload) {
  return Pae64GetL2p (cr3, va, payload);
}

/** @deprecated Use Pae64GetL1p instead **/
static pte_t *pae64_get_l1p (pte_t *cr3, vaddr_t va, int payload) {
  return Pae64GetL1p (cr3, va, payload);
}

/** @deprecated Use Pae64Verify instead **/
void pae64_verify (vaddr_t va, size64_t size) {
  Pae64Verify (va, size);
}

/** @deprecated Use Pae64Initialize instead **/
void pae64_init (void) {
  Pae64Initialize ();
}

/** @deprecated Use Pae64MapPage instead **/
void pae64_map_page (void *pt, vaddr_t va, uintptr_t pa, int payload, int w, int x) {
  Pae64MapPage (pt, va, pa, payload, w, x);
}

/** @deprecated Use Pae64PopulatePage instead **/
static uintptr_t pae64_populate_page (pte_t *cr3, vaddr_t va, int u, int w, int x, int payload) {
  return Pae64PopulatePage (cr3, va, u, w, x, payload);
}

/** @deprecated Use Pae64GetPhys instead **/
uintptr_t pae64_getphys (vaddr_t va) {
  return Pae64GetPhys (va);
}

/** @deprecated Use Pae64DirectMap instead **/
void pae64_directmap (void *pt, uint64_t pabase, vaddr_t va, size64_t size,
		      enum memory_type mt, int payload, int x) {
  Pae64DirectMap (pt, pabase, va, size, mt, payload, x);
}

/** @deprecated Use Pae64Physmap instead **/
void pae64_physmap (vaddr_t va, size64_t size, uint64_t pa, enum memory_type mt) {
  Pae64Physmap (va, size, pa, mt);
}

/** @deprecated Use Pae64TopPtAlloc instead **/
void pae64_topptalloc (vaddr_t va, size64_t size) {
  Pae64TopPtAlloc (va, size);
}

/** @deprecated Use Pae64PtAlloc instead **/
void pae64_ptalloc (vaddr_t va, size64_t size) {
  Pae64PtAlloc (va, size);
}

/** @deprecated Use Pae64Linear instead **/
void pae64_linear (vaddr_t va, size64_t size) {
  Pae64Linear (va, size);
}

/** @deprecated Use Pae64Populate instead **/
void pae64_populate (vaddr_t va, size64_t size, int u, int w, int x) {
  Pae64Populate (va, size, u, w, x);
}

/** @deprecated Use Pae64Entry instead **/
void pae64_entry (vaddr_t entry) {
  Pae64Entry (entry);
}

// Legacy global variable aliases
static bool nx_enabled __attribute__((alias("gNxEnabled")));
static pte_t *pae_cr3 __attribute__((alias("gPaeCr3")));
static pte_t *l2s[4] __attribute__((alias("gL2s")));
static pte_t *pae64_cr3 __attribute__((alias("gPae64Cr3")));
