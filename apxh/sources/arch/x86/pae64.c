/** @file
  APXH x86 PAE64 Paging Implementation

  64-bit Physical Address Extension (PAE64/AMD64) paging implementation
  using 4-level page tables (PML4, PDPT, PD, PT). Supports 48-bit virtual
  addressing on AMD64/x86-64 processors.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/x86/pae.h>
#include <apxh/arch.h>

#define PAE64_DIRECTMAP_START 0
#define PAE64_DIRECTMAP_END   (1LL << 30)

#define L4OFF64(_va) (((_va) >> 39) & 0x1ff)
#define L3OFF64(_va) (((_va) >> 30) & 0x1ff)
#define L2OFF64(_va) (((_va) >> 21) & 0x1ff)
#define L1OFF64(_va) (((_va) >> 12) & 0x1ff)

static PTE *gPae64Cr3;

/**
  Get L3 page directory pointer entry (PAE64).

  Walks PAE64 (AMD64) page tables to L3 level, allocating missing
  levels.

  @param[in] pCr3     Page table root.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE to allocate from payload pages.

  @return Pointer to L3 PTE.
**/
static PTE *
Pae64GetL3p (
  IN PTE    *Cr3,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN INT32 IsPayload
  )
{
  PTE *L4Entry, *L3Entry;
  UINT32 L4Off = L4OFF64 (VirtualAddress);
  UINT32 L3Off = L3OFF64 (VirtualAddress);

  L4Entry = Cr3 + L4Off;

  L3Entry = (PTE *) PteGetAddr (L4Entry);
  if (L3Entry == NULL)
    {
      UINTN L3Page;

      /* Populating L3. */
      L3Page = IsPayload ? GetPayloadPage () : GetPage ();

      SetPte (L4Entry, L3Page >> PAGE_SHIFT, PTE_U | PTE_W | PTE_P);
      L3Entry = (PTE *) L3Page;
    }

  return L3Entry + L3Off;
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
static PTE *
Pae64GetL2p (
  IN PTE    *Cr3,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN INT32 IsPayload
  )
{
  PTE *L3Entry, *L2Entry;

  UINT32 L2Off = L2OFF64 (VirtualAddress);

  L3Entry = Pae64GetL3p (Cr3, VirtualAddress, IsPayload);

  L2Entry = (PTE *) PteGetAddr (L3Entry);
  if (L2Entry == NULL)
    {
      UINTN L2Page;

      /* Populating L2. */
      L2Page = IsPayload ? GetPayloadPage () : GetPage ();

      SetPte (L3Entry, L2Page >> PAGE_SHIFT, PTE_U | PTE_W | PTE_P);
      L2Entry = (PTE *) L2Page;
    }

  return L2Entry + L2Off;
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
static PTE *
Pae64GetL1p (
  IN PTE    *Cr3,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN INT32 IsPayload
  )
{
  PTE *L2Entry, *L1Entry;
  UINT32 L1Off = L1OFF64 (VirtualAddress);

  L2Entry = Pae64GetL2p (Cr3, VirtualAddress, IsPayload);

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
  Verify PAE64 address range.

  Validates that a virtual address range is suitable for PAE64
  paging. Currently performs no checks.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
static VOID
Pae64Verify (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  /* Nothing to check. */
}

/**
  Initialize PAE64 paging.

  Sets up PAE64 (AMD64) paging with 4-level page tables. Enables
  NX if supported and configures PAT table.
**/
static VOID
Pae64Initialize (
  VOID
  )
{
  assert (CpuSupportsLongmode ());

  gNxEnabled = CpuSupportsNx ();

  SetupPatTable ();

  gPae64Cr3 = (PTE *) GetPayloadPage ();

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

  printf ("Mapping at va %llx PA %lx (p:%d, w:%d, x:%d)\n", VirtualAddress, PhysicalAddress, IsPayload,
	  IsWritable, IsExecutable);

  L1Entry = Pae64GetL1p (Cr3, VirtualAddress, IsPayload);
  L1F = (IsWritable ? PTE_W : 0) | (IsExecutable ? 0 : PTE_NX) | PTE_P;

  Page = (UINTN) PteGetAddr (L1Entry);
  assert (Page == 0);
  Page = PhysicalAddress >> PAGE_SHIFT;
  SetPte (L1Entry, Page, L1F);
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
static UINTN
Pae64PopulatePage (
  IN PTE    *Cr3,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN INT32 IsUserMode,
  IN INT32 IsWritable,
  IN INT32 IsExecutable,
  IN INT32 IsPayload
  )
{
  PTE *L1Entry;
  UINT64 L1F;
  UINTN Page;

  L1Entry = Pae64GetL1p (Cr3, VirtualAddress, IsPayload);
  L1F = (IsUserMode ? PTE_U : 0) | (IsWritable ? PTE_W : 0) | (IsExecutable ? 0 : PTE_NX) | PTE_P;

  Page = (UINTN) PteGetAddr (L1Entry);
  if (Page == 0)
    {
      Page = IsPayload ? GetPayloadPage () : GetPage ();
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
  Get physical address (PAE64).

  Translates virtual address to physical address using current
  PAE64 page tables.

  @param[in] Va  Virtual address.

  @return Physical address.
**/
static UINTN
Pae64GetPhysical (
  IN VIRTUAL_ADDRESS  VirtualAddress
  )
{
  UINTN Page;
  PTE *L4Entry, *L3Entry, *L2Entry, *L1Entry;
  UINT32 L4Off = L4OFF64 (VirtualAddress);
  UINT32 L3Off = L3OFF64 (VirtualAddress);
  UINT32 L2Off = L2OFF64 (VirtualAddress);
  UINT32 L1Off = L1OFF64 (VirtualAddress);

  L4Entry = gPae64Cr3 + L4Off;

  L3Entry = (PTE *) PteGetAddr (L4Entry);
  assert (L3Entry != NULL);
  L3Entry += L3Off;

  L2Entry = (PTE *) PteGetAddr (L3Entry);
  assert (L2Entry != NULL);
  L2Entry += L2Off;

  L1Entry = (PTE *) PteGetAddr (L2Entry);
  assert (L1Entry != NULL);
  L1Entry += L1Off;

  Page = (UINTN) PteGetAddr (L1Entry);
  assert (Page != 0);

  return Page |= (VirtualAddress & ~(PAGE_MASK));
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
  IN VOID              *PageTable,
  IN UINT64            PaBase,
  IN VIRTUAL_ADDRESS           VirtualAddress,
  IN SIZE64          Size,
  IN MEMORY_TYPE  Mt,
  IN INT32 IsPayload,
  IN INT32 IsExecutable
  )
{
  SSIZE64 Len;
  UINT64 Pa;
  PTE *Cr3 = (PTE *) PageTable;
INT32 P1G = CpuSupports1gbPages ();
  UINTN L3Cnt = 0, L2Cnt = 0, L1Cnt = 0;

#define GB1ALIGNED(_a) (((_a) & ((1L << 30) - 1)) == 0)
#define MB2ALIGNED(_a) (((_a) & ((1L << 21) - 1)) == 0)


  /* Signed to unsigned: no one will ask us a 1<<64 bytes physmap. */
  Len = (SSIZE64) Size;
  Pa = PaBase;

  while (Len > 0)
    {

      if (P1G && GB1ALIGNED (Pa) && GB1ALIGNED (VirtualAddress) && Len >= (1L << 30))
	{
	  PTE *L3Entry = Pae64GetL3p (Cr3, VirtualAddress, IsPayload);

	  SetPte (L3Entry, Pa >> PAGE_SHIFT,
		   MemtypeToFlags (Mt, FALSE /*1GB */ ) |
		   PTE_PS | PTE_W | PTE_P | (IsExecutable ? 0 : PTE_NX));
	  VirtualAddress += (1L << 30);
	  Pa += (1L << 30);
	  Len -= (1L << 30);
	  L3Cnt++;
	}
      else if (MB2ALIGNED (Pa) && MB2ALIGNED (VirtualAddress) && Len >= (1 << 21))
	{
	  PTE *L2Entry = Pae64GetL2p (Cr3, VirtualAddress, IsPayload);

	  SetPte (L2Entry, Pa >> PAGE_SHIFT,
		   MemtypeToFlags (Mt, FALSE /* 2MB */ ) |
		   PTE_PS | PTE_W | PTE_P | (IsExecutable ? 0 : PTE_NX));
	  VirtualAddress += (1L << 21);
	  Pa += (1L << 21);
	  Len -= (1L << 21);
	  L2Cnt++;
	}
      else
	{
	  PTE *L1Entry = Pae64GetL1p (Cr3, VirtualAddress, IsPayload);

	  SetPte (L1Entry, Pa >> PAGE_SHIFT,
		   MemtypeToFlags (Mt,
				     TRUE /*4kB */ ) | PTE_W | PTE_P | (IsExecutable ? 0
									:
									PTE_NX));
	  VirtualAddress += (1L << PAGE_SHIFT);
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
static VOID
Pae64MapPhysical (
  IN VIRTUAL_ADDRESS           VirtualAddress,
  IN SIZE64          Size,
  IN UINT64            Pa,
  IN MEMORY_TYPE  Mt
  )
{
  Pae64DirectMap (gPae64Cr3, Pa, VirtualAddress, Size, Mt, 1, 0);
}

/**
  Allocate top-level page tables (PAE64).

  Pre-allocates L3 page directory pointer tables for address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
static VOID
Pae64AllocateTopPageTable (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  UINT32 i, n;

  n = (Size + (1 << 30) - 1) >> 30;

  for (i = 0; i < n; i++)
    (VOID) Pae64GetL3p (gPae64Cr3, VirtualAddress + (i << PAGE_SHIFT), 1);
}

/**
  Allocate page tables (PAE64).

  Pre-allocates L1 page tables for address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
static VOID
Pae64AllocatePageTable (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  UINT32 i, n;

  n = Size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    (VOID) Pae64GetL1p (gPae64Cr3, VirtualAddress + (i << PAGE_SHIFT), 1);
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
static VOID
Pae64MapLinear (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  UINT32 L4Off = L4OFF64 (VirtualAddress);
  PTE *L4Entry;

  if (VirtualAddress & PAE64_LINEAR_ALIGN)
    {
      printf ("PAE Linear VA %llx not aligned (align mask: %llx).\n",
	      VirtualAddress, PAE64_LINEAR_ALIGN);
      exit (-1);
    }

  if (Size < PAE64_LINEAR_SIZE)
    {
      printf ("PAE Linear size %llx too small.\n", Size);
      exit (-1);
    }

  L4Entry = gPae64Cr3 + L4Off;
  SetPte (L4Entry, (UINTN) gPae64Cr3 >> PAGE_SHIFT, PTE_W | PTE_P);
  printf ("Wrote %llx at %p\n", *L4Entry, L4Entry);
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
static VOID
Pae64Populate (
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
      Pae64PopulatePage (gPae64Cr3, VirtualAddress, IsUserMode, IsWritable, IsExecutable, 1);

      Len -= PAGE_CEILING (VirtualAddress) - VirtualAddress;
      VirtualAddress += PAGE_CEILING (VirtualAddress) - VirtualAddress;

    }
}

/**
  Transfer control to PAE64 kernel.

  Prepares final environment and transfers control to kernel entry
  point using PAE64 (AMD64) long mode paging.

  @param[in] Entry  Kernel entry point address.
**/
static VOID
Pae64Entry (
  IN VIRTUAL_ADDRESS  Entry
  )
{
  PlatformEntry (ArchAmd64, (VIRTUAL_ADDRESS) (UINTN) gPae64Cr3, Entry);
}

//
// IVirtualAddressSpace COM Interface Implementation
//

static HRESULT STDMETHODCALLTYPE
Pae64QueryInterface (
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
Pae64AddRef (
  IN IVirtualAddressSpace  *This
  )
{
  return 1;  // Static instance, no reference counting
}

static UINT32 STDMETHODCALLTYPE
Pae64Release (
  IN IVirtualAddressSpace  *This
  )
{
  return 1;  // Static instance, no reference counting
}

static HRESULT STDMETHODCALLTYPE
Pae64IInitialize (
  IN IVirtualAddressSpace  *This
  )
{
  Pae64Initialize ();
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Pae64IGetPhysical (
  IN  IVirtualAddressSpace    *This,
  IN  VIRTUAL_ADDRESS  VirtualAddress,
  OUT UINTN            *PhysicalAddress
  )
{
  if (PhysicalAddress == NULL) {
    return E_POINTER;
  }

  *PhysicalAddress = Pae64GetPhysical (VirtualAddress);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Pae64IVerify (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN SIZE64           Size
  )
{
  Pae64Verify (VirtualAddress, Size);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Pae64IPopulate (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN SIZE64           Size,
  IN INT32            IsUserMode,
  IN INT32            IsWritable,
  IN INT32            IsExecutable
  )
{
  Pae64Populate (VirtualAddress, Size, IsUserMode, IsWritable, IsExecutable);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Pae64IMapPhysical (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN SIZE64           Size,
  IN UINT64           PhysicalAddress,
  IN MEMORY_TYPE      Type
  )
{
  Pae64MapPhysical (VirtualAddress, Size, PhysicalAddress, Type);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Pae64IAllocatePageTable (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN SIZE64           Size
  )
{
  Pae64AllocatePageTable (VirtualAddress, Size);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Pae64IAllocateTopPageTable (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN SIZE64           Size
  )
{
  Pae64AllocateTopPageTable (VirtualAddress, Size);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Pae64IMapLinear (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN SIZE64           Size
  )
{
  Pae64MapLinear (VirtualAddress, Size);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Pae64IEntry (
  IN IVirtualAddressSpace    *This,
  IN VIRTUAL_ADDRESS  EntryPoint
  )
{
  Pae64Entry (EntryPoint);
  return S_OK;  // Never returns
}

//
// PAE64 Architecture VTable
//

static CONST IVirtualAddressSpaceVtbl gPae64Vtbl = {
  Pae64QueryInterface,
  Pae64AddRef,
  Pae64Release,
  Pae64IInitialize,
  Pae64IGetPhysical,
  Pae64IVerify,
  Pae64IPopulate,
  Pae64IMapPhysical,
  Pae64IAllocatePageTable,
  Pae64IAllocateTopPageTable,
  Pae64IMapLinear,
  Pae64IEntry
};

//
// PAE64 Architecture Instance
//

IVirtualAddressSpace gPae64Arch = {
  &gPae64Vtbl
};

// Auto-register this architecture
APXH_REGISTER_ARCH(gPae64Arch, ArchAmd64);

