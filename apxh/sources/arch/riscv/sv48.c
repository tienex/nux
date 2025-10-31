/** @file
  APXH RISC-V SV48 Paging Support

  Implements SV48 (Supervisor Virtual Memory with 48-bit addresses)
  page table management for RISC-V 64-bit systems. Provides 4-level
  page tables with support for 1GB, 2MB, and 4KB mappings.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>

#define PTE_V (1LL << 0)
#define PTE_R (1LL << 1)
#define PTE_W (1LL << 2)
#define PTE_X (1LL << 3)
#define PTE_U (1LL << 4)

typedef UINT64 PTE;

static PTE *gSv48Root;

/**
  Set page table entry.

  Constructs and writes a PTE with the given PFN and flags.

  @param[out] Ptep  Pointer to PTE.
  @param[in]  Pfn    Page frame number.
  @param[in]  Flags  PTE flags (PTE_V, PTE_R, PTE_W, PTE_X, PTE_U).
**/
static VOID
SetPte (
  OUT PTE   *Ptep,
  IN UINT64   Pfn,
  IN UINT64   Flags
  )
{
  *Ptep = (Pfn << 10) | Flags;
}

/**
  Get physical address from PTE.

  Extracts the physical address from a page table entry.

  @param[in] Ptep  Pointer to PTE.

  @return Physical address, or NULL if entry not valid.
**/
static VOID *
PteGetAddr (
  IN PTE  *Ptep
  )
{
  PTE Pte = *Ptep;

  if (!(Pte & PTE_V))
    return NULL;

  return (VOID *) (UINTN) ((Pte & 0x7ffffffffffffc00LL) << 2);
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

  @param[in] Root    Root page table.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE if allocating from payload pages.

  @return Pointer to L3 PTE.
**/
static PTE *
Sv48GetL3p (
  IN PTE   *Root,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN INT32    IsPayload
  )
{
  PTE *pL4p, *pL3p;
  UINT32 L4off = L4OFF64 (VirtualAddress);
  UINT32 L3off = L3OFF64 (VirtualAddress);

  pL4p = Root + L4off;

  pL3p = (PTE *) PteGetAddr (pL4p);
  if (pL3p == NULL)
    {
      UINTN L3page;

      /* Populating L3. */
      L3page = IsPayload ? GetPayloadPage () : GetPage ();

      SetPte (pL4p, L3page >> PAGE_SHIFT, PTE_V);
      pL3p = (PTE *) L3page;
    }

  return pL3p + L3off;
}

/**
  Get L2 page table pointer.

  Retrieves or allocates L2 page table for the given virtual address.

  @param[in] Root    Root page table.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE if allocating from payload pages.

  @return Pointer to L2 PTE.
**/
static PTE *
Sv48GetL2p (
  IN PTE   *Root,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN INT32    IsPayload
  )
{
  PTE *pL3p, *pL2p;

  UINT32 L2off = L2OFF64 (VirtualAddress);

  pL3p = Sv48GetL3p (Root, VirtualAddress, IsPayload);

  pL2p = (PTE *) PteGetAddr (pL3p);
  if (pL2p == NULL)
    {
      UINTN L2page;

      /* Populating L2. */
      L2page = IsPayload ? GetPayloadPage () : GetPage ();

      SetPte (pL3p, L2page >> PAGE_SHIFT, PTE_V);
      pL2p = (PTE *) L2page;
    }

  return pL2p + L2off;
}

/**
  Get L1 page table pointer.

  Retrieves or allocates L1 page table for the given virtual address.

  @param[in] Root    Root page table.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE if allocating from payload pages.

  @return Pointer to L1 PTE.
**/
static PTE *
Sv48GetL1p (
  IN PTE   *Root,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN INT32    IsPayload
  )
{
  PTE *pL2p, *pL1p;
  UINT32 L1off = L1OFF64 (VirtualAddress);

  pL2p = Sv48GetL2p (Root, VirtualAddress, IsPayload);

  pL1p = (PTE *) PteGetAddr (pL2p);
  if (pL1p == NULL)
    {
      UINTN L1page;

      /* Populating L1. */
      L1page = IsPayload ? GetPayloadPage () : GetPage ();

      SetPte (pL2p, L1page >> PAGE_SHIFT, PTE_V);
      pL1p = (PTE *) L1page;
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
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
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
  gSv48Root = (PTE *) GetPayloadPage ();

  printf ("Using SV48 paging (root: %08lx).\n", gSv48Root);
}

/**
  Map single page.

  Creates a page mapping at the specified virtual address.

  @param[in] Pt      Page table root.
  @param[in] Va       Virtual address.
  @param[in] Pa       Physical address.
  @param[in] Payload  TRUE if allocating from payload pages.
  @param[in] W        TRUE if writable.
  @param[in] X        TRUE if executable.
**/
VOID
Sv48MapPage (
  IN VOID     *PageTable,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN UINTN    PhysicalAddress,
  IN INT32    IsPayload,
  IN INT32    IsWritable,
  IN INT32    IsExecutable
  )
{
  PTE *pL1p, *Root;
  UINT64 L1f;
  UINTN Page;

  Root = (PTE *) PageTable;

  printf ("Mapping at va %llx PA %lx (p:%d, w:%d, x:%d)\n", VirtualAddress, PhysicalAddress, IsPayload,
	  IsWritable, IsExecutable);

  pL1p = Sv48GetL1p (Root, VirtualAddress, IsPayload);
  L1f = (IsWritable ? PTE_W : 0) | (IsExecutable ? PTE_X : 0) | PTE_R | PTE_V;

  Page = (UINTN) PteGetAddr (pL1p);
  assert (Page == 0);
  Page = PhysicalAddress >> PAGE_SHIFT;
  SetPte (pL1p, Page, L1f);
}

/**
  Populate page with flags.

  Allocates and maps a page with the specified attributes, merging
  flags if page already exists.

  @param[in] Root    Root page table.
  @param[in] Va       Virtual address.
  @param[in] U        TRUE if user-accessible.
  @param[in] W        TRUE if writable.
  @param[in] X        TRUE if executable.
  @param[in] Payload  TRUE if allocating from payload pages.

  @return Physical address of page.
**/
static UINTN
Sv48PopulatePage (
  IN PTE   *Root,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN INT32    IsUserMode,
  IN INT32    IsWritable,
  IN INT32    IsExecutable,
  IN INT32    IsPayload
  )
{
  PTE *pL1p;
  UINT64 L1f;
  UINTN Page;

  pL1p = Sv48GetL1p (Root, VirtualAddress, IsPayload);
  L1f = (IsUserMode ? PTE_U : 0) | (IsWritable ? PTE_W : 0) | (IsExecutable ? PTE_X : 0) | PTE_R | PTE_V;

  Page = (UINTN) PteGetAddr (pL1p);
  if (Page == 0)
    {
      Page = IsPayload ? GetPayloadPage () : GetPage ();
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
Sv48GetPhysical (
  IN VIRTUAL_ADDRESS  VirtualAddress
  )
{
  UINTN Page;
  PTE *pL4p, *pL3p, *pL2p, *pL1p;
  UINT32 L4off = L4OFF64 (VirtualAddress);
  UINT32 L3off = L3OFF64 (VirtualAddress);
  UINT32 L2off = L2OFF64 (VirtualAddress);
  UINT32 L1off = L1OFF64 (VirtualAddress);

  pL4p = gSv48Root + L4off;

  pL3p = (PTE *) PteGetAddr (pL4p);
  assert (pL3p != NULL);
  pL3p += L3off;

  pL2p = (PTE *) PteGetAddr (pL3p);
  assert (pL2p != NULL);
  pL2p += L2off;

  pL1p = (PTE *) PteGetAddr (pL2p);
  assert (pL1p != NULL);
  pL1p += L1off;

  Page = (UINTN) PteGetAddr (pL1p);
  assert (Page != 0);

  return Page |= (VirtualAddress & ~(PAGE_MASK));
}

/**
  Direct map physical region.

  Creates direct (1:1) physical to virtual mappings using large pages
  where possible (1GB or 2MB).

  @param[in] Pt      Page table root.
  @param[in] PaBase   Physical base address.
  @param[in] Va       Virtual address.
  @param[in] Size     Size of region.
  @param[in] Mt       Memory type.
  @param[in] Payload  TRUE if allocating from payload pages.
  @param[in] X        TRUE if executable.
**/
VOID
Sv48DirectMap (
  IN VOID              *PageTable,
  IN UINT64            PaBase,
  IN VIRTUAL_ADDRESS           VirtualAddress,
  IN SIZE64          Size,
  IN MEMORY_TYPE  Mt,
  IN INT32             IsPayload,
  IN INT32             IsExecutable
  )
{
  SSIZE64 Len;
  UINT64 Pa;
  PTE *Root = (PTE *) PageTable;
  UINT64 L3cnt = 0, L2cnt = 0, L1cnt = 0;

#define GB1ALIGNED(_a) (((_a) & ((1L << 30) - 1)) == 0)
#define MB2ALIGNED(_a) (((_a) & ((1L << 21) - 1)) == 0)

  /* Signed to unsigned: no one will ask us a 1<<64 bytes physmap. */
  Len = (SSIZE64) Size;
  Pa = PaBase;

  while (Len > 0)
    {

      if (GB1ALIGNED (Pa) && GB1ALIGNED (VirtualAddress) && Len >= (1L << 30))
	{
	  PTE *pL3p = Sv48GetL3p (Root, VirtualAddress, IsPayload);

	  SetPte (pL3p, Pa >> PAGE_SHIFT,
		   (IsExecutable ? PTE_X : 0) | PTE_W | PTE_R | PTE_V);
	  VirtualAddress += (1L << 30);
	  Pa += (1L << 30);
	  Len -= (1L << 30);
	  L3cnt++;
	}
      else if (MB2ALIGNED (Pa) && MB2ALIGNED (VirtualAddress) && Len >= (1 << 21))
	{
	  PTE *pL2p = Sv48GetL2p (Root, VirtualAddress, IsPayload);

	  SetPte (pL2p, Pa >> PAGE_SHIFT,
		   (IsExecutable ? PTE_X : 0) | PTE_W | PTE_R | PTE_V);
	  VirtualAddress += (1L << 21);
	  Pa += (1L << 21);
	  Len -= (1L << 21);
	  L2cnt++;
	}
      else
	{
	  PTE *pL1p = Sv48GetL1p (Root, VirtualAddress, IsPayload);

	  SetPte (pL1p, Pa >> PAGE_SHIFT,
		   (IsExecutable ? PTE_X : 0) | PTE_W | PTE_R | PTE_V);
	  VirtualAddress += (1L << PAGE_SHIFT);
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
Sv48MapPhysical (
  IN VIRTUAL_ADDRESS           VirtualAddress,
  IN SIZE64          Size,
  IN UINT64            Pa,
  IN MEMORY_TYPE  Mt
  )
{
  Sv48DirectMap (gSv48Root, Pa, VirtualAddress, Size, Mt, 1, 0);
}

/**
  Allocate top-level page tables.

  Pre-allocates L3 page tables for a virtual address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
Sv48AllocateTopPageTable (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  UINT32 i, n;

  n = (Size + (1 << 30) - 1) >> 30;

  for (i = 0; i < n; i++)
    (VOID) Sv48GetL3p (gSv48Root, VirtualAddress + (i << PAGE_SHIFT), 1);
}

/**
  Allocate page tables.

  Pre-allocates L1 page tables for a virtual address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
Sv48AllocatePageTable (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  UINT32 i, n;

  n = Size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    (VOID) Sv48GetL1p (gSv48Root, VirtualAddress + (i << PAGE_SHIFT), 1);
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
Sv48MapLinear (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  UINT32 L4off = L4OFF64 (VirtualAddress);
  PTE *pL4p;

  if (VirtualAddress & SV48_LINEAR_ALIGN)
    {
      printf ("PAE Linear VA %llx not aligned (align mask: %llx).\n",
	      VirtualAddress, SV48_LINEAR_ALIGN);
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
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size,
  IN INT32     IsUserMode,
  IN INT32     IsWritable,
  IN INT32     IsExecutable
  )
{
  SSIZE64 Len = Size;

  while (Len > 0)
    {
      Sv48PopulatePage (gSv48Root, VirtualAddress, IsUserMode, IsWritable, IsExecutable, 1);

      Len -= PAGE_CEILING (VirtualAddress) - VirtualAddress;
      VirtualAddress += PAGE_CEILING (VirtualAddress) - VirtualAddress;

    }
}

/**
  Jump to kernel entry point.

  Transfers control to the loaded kernel with SV48 paging enabled.

  @param[in] Entry  Kernel entry point virtual address.
**/
VOID
Sv48Entry (
  IN VIRTUAL_ADDRESS  Entry
  )
{
  PlatformEntry (ARCH_RISCV64, (VIRTUAL_ADDRESS) (UINTN) gSv48Root, Entry);
}
