/** @file
  APXH x86 PAE32 Paging Implementation

  32-bit Physical Address Extension (PAE) paging implementation using
  3-level page tables (PDPT, PD, PT). Supports 36-bit physical addressing
  with 4KB pages on 32-bit x86 processors.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/x86/pae.h>
#include <apxh/arch.h>

/* 1 Gb direct map in the Payload Page Table. */
#define PAE_DIRECTMAP_START 0
#define PAE_DIRECTMAP_END   (1LL << 30)

#define L3OFF(_va) (((_va) >> 30) & 0x3)
#define L2OFF(_va) (((_va) >> 21) & 0x1ff)
#define L1OFF(_va) (((_va) >> 12) & 0x1ff)

static PTE *gPaeCr3;
static PTE *gL2s[4];

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
  OUT PTE   *Ptep,
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
  IN PTE  *Ptep
  )
{
  PTE Pte = *Ptep;

  if (!(Pte & PTE_P))
    return NULL;

  return (VOID *) (UINTN) (Pte & 0x7ffffffffffff000LL);
}

/**
  Get flags from PTE.

  Extracts the flag bits from a page table entry.

  @param[in] Ptep  Pointer to PTE.

  @return PTE flags.
**/
static UINT64
PteGetFlags (
  IN PTE  *Ptep
  )
{
  PTE Pte = *Ptep;

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
static VOID
PaeVerify (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
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
static VOID
PaeInitialize (
  VOID
  )
{
INT32 i;

  /* Enable NX */
  assert (CpuSupportsPae ());

  gNxEnabled = CpuSupportsNx ();

  /* In PAE is only 64 bytes, but we allocate a full page for it. */
  gPaeCr3 = (PTE *) GetPayloadPage ();

  /* Set PDPTEs */
  for (i = 0; i < 4; i++)
    {
      UINTN L2Page = GetPayloadPage ();

      SetPte (gPaeCr3 + i, L2Page >> PAGE_SHIFT, PTE_P);
      gL2s[i] = (PTE *) L2Page;
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
static PTE *
PaeGetL2p (
  IN PTE    *Cr3,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN INT32 IsPayload
  )
{
  PTE *L3Entry, *L2Entry;
  UINT32 L3Off = L3OFF (VirtualAddress);
  UINT32 L2Off = L2OFF (VirtualAddress);

  L3Entry = Cr3 + L3Off;

  L2Entry = (PTE *) PteGetAddr (L3Entry);
  if (L2Entry == NULL)
    {
      UINTN L2Page;

      /* Populating L2. */
      L2Page = IsPayload ? GetPayloadPage () : GetPage ();

      SetPte (L3Entry, L2Page >> PAGE_SHIFT, PTE_P);
      L2Entry = (PTE *) L2Page;
    }

  return L2Entry + L2Off;
}

/**
  Get L1 page table entry pointer (PAE).

  Walks PAE page tables to L1 level, allocating missing levels.

  @param[in] pCr3     Page table root.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE to allocate from payload pages.

  @return Pointer to L1 PTE.
**/
static PTE *
PaeGetL1p (
  IN PTE    *Cr3,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN INT32 IsPayload
  )
{
  PTE *L2Entry, *L1Entry;
  UINT32 L1Off = L1OFF (VirtualAddress);

  L2Entry = PaeGetL2p (Cr3, VirtualAddress, IsPayload);

  L1Entry = (PTE *) PteGetAddr (L2Entry);
  if (L1Entry == NULL)
    {
      UINTN L1Page;

      /* Populating L1. */
      L1Page = IsPayload ? GetPayloadPage () : GetPage ();

      SetPte (L2Entry, L1Page >> PAGE_SHIFT, PTE_U | PTE_W | PTE_P);
      L1Entry = (PTE *) L1Page;
    }

  return L1Entry + L1Off;
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
  IN VOID      *PageTable,
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN UINTN PhysicalAddress,
  IN INT32 IsPayload,
  IN INT32 IsWritable,
  IN INT32 IsExecutable
  )
{
  PTE *L1Entry, *Cr3;
  UINT64 L1F;
  UINTN Page;

  Cr3 = (PTE *) PageTable;

  L1Entry = PaeGetL1p (Cr3, VirtualAddress, IsPayload);
  L1F = (IsWritable ? PTE_W : 0) | (IsExecutable ? 0 : PTE_NX) | PTE_P;

  Page = (UINTN) PteGetAddr (L1Entry);
  assert (Page == 0);
  Page = PhysicalAddress >> PAGE_SHIFT;
  SetPte (L1Entry, Page, L1F);
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
static UINTN
PaePopulatePage (
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN INT32 IsUserMode,
  IN INT32 IsWritable,
  IN INT32 IsExecutable
  )
{
  PTE *L1Entry;
  UINT64 L1F;
  UINTN Page;

  L1Entry = PaeGetL1p (gPaeCr3, VirtualAddress, 1);

  L1F = (IsUserMode ? PTE_U : 0) | (IsWritable ? PTE_W : 0) | (IsExecutable ? 0 : PTE_NX) | PTE_P;

  Page = (UINTN) PteGetAddr (L1Entry);
  if (PteGetAddr (L1Entry) == NULL)
    {
      Page = GetPayloadPage ();
      SetPte (L1Entry, Page >> PAGE_SHIFT, L1F);
    }
  else
    {
      UINT64 NewF;
      UINT64 OldF = PteGetFlags (L1Entry);

      NewF = PteMergeFlags (L1F, OldF);
      if (NewF != OldF)
	{
	  printf ("Flags changed from %llx to %llx\n", OldF, NewF);
	  SetPte (L1Entry, Page >> PAGE_SHIFT, NewF);
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
static UINTN
PaeGetPhysical (
  IN VIRTUAL_ADDRESS  VirtualAddress
  )
{
  UINTN Page;
  PTE *L2Entry, *L1Table, *L1Entry;
  UINT32 L3Off = L3OFF (VirtualAddress);
  UINT32 L2Off = L2OFF (VirtualAddress);
  UINT32 L1Off = L1OFF (VirtualAddress);

  L2Entry = gL2s[L3Off] + L2Off;

  L1Table = (PTE *) PteGetAddr (L2Entry);
  assert (L1Table != NULL);

  L1Entry = L1Table + L1Off;

  Page = (UINTN) PteGetAddr (L1Entry);
  assert (Page != 0);

  return Page | (VirtualAddress & ~(PAGE_MASK));
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
  IN VOID              *PageTable,
  IN UINT64            PhysicalAddress,
  IN VIRTUAL_ADDRESS           VirtualAddress,
  IN SIZE64          Size,
  IN MEMORY_TYPE  Mt,
  IN INT32 IsPayload,
  IN INT32 IsExecutable
  )
{
  UINT64 PaPfn = PhysicalAddress >> PAGE_SHIFT;
  UINT32 i, n;
  PTE *Entry;

  n = Size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    {
      Entry = PaeGetL1p (PageTable, VirtualAddress + (i << PAGE_SHIFT), IsPayload);
      SetPte (Entry, PaPfn + i,
	       MemtypeToFlags (Mt, TRUE /*4k */ ) | PTE_P | PTE_W | (IsExecutable ? 0 :
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
static VOID
PaeMapPhysical (
  IN VIRTUAL_ADDRESS           VirtualAddress,
  IN SIZE64          Size,
  IN UINT64            Pa,
  IN MEMORY_TYPE  Mt
  )
{
  PaeDirectMap (gPaeCr3, Pa, VirtualAddress, Size, Mt, 1, 0);
}

/**
  Allocate top-level page tables (PAE).

  Pre-allocates page table structures for address range.
  In PAE, equivalent to PTALLOC since there's no L4 level.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
static VOID
PaeAllocateTopPageTable (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  /* In PAE, TOPPTALLOC is equivalent to PTALLOC. */

  PaeAllocatePageTable (VirtualAddress, Size);
}

/**
  Allocate page tables (PAE).

  Pre-allocates L1 page tables for address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
static VOID
PaeAllocatePageTable (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  UINT32 i, n;

  n = Size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    (VOID) PaeGetL1p (gPaeCr3, VirtualAddress + (i << PAGE_SHIFT), 1);
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
static VOID
PaeMapLinear (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
INT32 i;
  UINT32 L3Off = L3OFF (VirtualAddress);
  UINT32 L2Off = L2OFF (VirtualAddress);

  if (VirtualAddress & PAE_LINEAR_ALIGN)
    {
      printf ("PAE Linear VA %llx not aligned (align mask: %lx).\n", VirtualAddress,
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
      PTE *L2Entry;

      L2Entry = gL2s[L3Off] + L2Off + i;
      SetPte (L2Entry, (UINT64) (UINTN) gL2s[i] >> PAGE_SHIFT,
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
static VOID
PaePopulate (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size,
  IN INT32 IsUserMode,
  IN INT32 IsWritable,
  IN INT32 IsExecutable
  )
{
  SSIZE64 Len = Size;

  while (Len > 0)
    {
      PaePopulatePage (VirtualAddress, IsUserMode, IsWritable, IsExecutable);

      Len -= PAGE_CEILING (VirtualAddress) - VirtualAddress;
      VirtualAddress += PAGE_CEILING (VirtualAddress) - VirtualAddress;

    }
}

/**
  Transfer control to PAE kernel.

  Prepares final environment and transfers control to kernel entry
  point using PAE paging mode.

  @param[in] Entry  Kernel entry point address.
**/
static VOID
PaeEntry (
  IN VIRTUAL_ADDRESS  Entry
  )
{
  PlatformEntry (Arch386, (VIRTUAL_ADDRESS) (UINTN) gPaeCr3, Entry);
}

//
// IVirtualAddressSpace COM Interface Implementation
//

static HRESULT STDMETHODCALLTYPE
PaeQueryInterface (
  IN  IVirtualAddressSpace  *This,
  IN  CONST GUID     *Iid,
  OUT VOID           **Object
  )
{
  if (memcmp (Iid, &IID_IVirtualAddressSpace, sizeof (GUID)) == 0 ||
      memcmp (Iid, &IID_IUnknown, sizeof (GUID)) == 0)
    {
      *Object = This;
      return S_OK;
    }

  *Object = NULL;
  return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
PaeAddRef (
  IN IVirtualAddressSpace  *This
  )
{
  return 1;  // Static instance, no reference counting
}

static UINT32 STDMETHODCALLTYPE
PaeRelease (
  IN IVirtualAddressSpace  *This
  )
{
  return 1;  // Static instance, no reference counting
}

static HRESULT STDMETHODCALLTYPE
PaeIInitialize (
  IN IVirtualAddressSpace  *This
  )
{
  PaeInitialize ();
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PaeIGetPhysical (
  IN  IVirtualAddressSpace    *This,
  IN  VIRTUAL_ADDRESS  VirtualAddress,
  OUT UINTN            *PhysicalAddress
  )
{
  if (PhysicalAddress == NULL) {
    return E_POINTER;
  }

  *PhysicalAddress = PaeGetPhysical (VirtualAddress);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PaeIVerify (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN SIZE64           Size
  )
{
  PaeVerify (VirtualAddress, Size);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PaeIPopulate (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN SIZE64           Size,
  IN INT32            IsUserMode,
  IN INT32            IsWritable,
  IN INT32            IsExecutable
  )
{
  PaePopulate (VirtualAddress, Size, IsUserMode, IsWritable, IsExecutable);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PaeIMapPhysical (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN SIZE64           Size,
  IN UINT64           PhysicalAddress,
  IN MEMORY_TYPE      Type
  )
{
  PaeMapPhysical (VirtualAddress, Size, PhysicalAddress, Type);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PaeIAllocatePageTable (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN SIZE64           Size
  )
{
  PaeAllocatePageTable (VirtualAddress, Size);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PaeIAllocateTopPageTable (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN SIZE64           Size
  )
{
  PaeAllocateTopPageTable (VirtualAddress, Size);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PaeIMapLinear (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN SIZE64           Size
  )
{
  PaeMapLinear (VirtualAddress, Size);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PaeIEntry (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  EntryPoint
  )
{
  PaeEntry (EntryPoint);
  return S_OK;  // Never returns
}

//
// PAE Architecture VTable
//

static CONST IVirtualAddressSpaceVtbl gPaeVtbl = {
  PaeQueryInterface,
  PaeAddRef,
  PaeRelease,
  PaeIInitialize,
  PaeIGetPhysical,
  PaeIVerify,
  PaeIPopulate,
  PaeIMapPhysical,
  PaeIAllocatePageTable,
  PaeIAllocateTopPageTable,
  PaeIMapLinear,
  PaeIEntry
};

//
// PAE Architecture Instance
//

IVirtualAddressSpace gPaeArch = {
  &gPaeVtbl
};

// Auto-register this architecture
APXH_REGISTER_ARCH(gPaeArch, Arch386);


/*
  AMD64 PAE support.
*/

