/** @file
  APXH RISC-V SV32 Paging Support

  Implements SV32 (Supervisor Virtual Memory with 32-bit addresses)
  page table management for RISC-V 32-bit systems. Provides 2-level
  page tables with support for 4MB and 4KB mappings.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>

#define PTE_V (1 << 0)
#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_U (1 << 4)

typedef UINT32 PTE;

static PTE *gSv32Root;

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
  IN UINT32   Pfn,
  IN UINT32   Flags
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

  return (VOID *) (UINTN) ((Pte & 0xfffffc00) << 2);
}

/**
  Get flags from PTE.

  Extracts the flag bits from a page table entry.

  @param[in] Ptep  Pointer to PTE.

  @return PTE flags.
**/
static UINT32
PteGetFlags (
  IN PTE  *Ptep
  )
{
  PTE Pte = *Ptep;

  return Pte & ~0xfffffc00;
}

/**
  Merge PTE flags.

  Combines two sets of PTE flags according to RISC-V rules.
  Validates consistency of user/kernel bits.

  @param[in] Fl1  First set of flags.
  @param[in] Fl2  Second set of flags.

  @return Merged flags.
**/
static UINT32
PteMergeFlags (
  IN UINT32  Fl1,
  IN UINT32  Fl2
  )
{
  UINT32 NewF;

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
  printf ("Merging: %x (+) %x = %x\n", Fl1, Fl2, NewF);
#endif
  return NewF;
}

/*
  RISC-V SV32 Support.
*/

#define SV32_DIRECTMAP_START 0
#define SV32_DIRECTMAP_END (1 << 30)

#define L2OFF32(_va) (((_va) >> 22) & 0x3ff)
#define L1OFF32(_va) (((_va) >> 12) & 0x3ff)

/**
  Get L1 page table pointer.

  Retrieves or allocates L1 page table for the given virtual address.

  @param[in] Root    Root page table.
  @param[in] Va       Virtual address.
  @param[in] Payload  TRUE if allocating from payload pages.

  @return Pointer to L1 PTE.
**/
static PTE *
Sv32GetL1p (
  IN PTE   *Root,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN INT32    IsPayload
  )
{
  PTE *L2Entry, *L1Entry;
  UINT32 L2off = L2OFF32 (VirtualAddress);
  UINT32 L1off = L1OFF32 (VirtualAddress);

  L2Entry = Root + L2off;

  L1Entry = (PTE *) PteGetAddr (L2Entry);
  if (L1Entry == NULL)
    {
      UINTN L1page;

      /* Populating L1. */
      L1page = IsPayload ? GetPayloadPage () : GetPage ();

      SetPte (L2Entry, L1page >> PAGE_SHIFT, PTE_V);
      L1Entry = (PTE *) L1page;
    }

  return L1Entry + L1off;
}

/**
  Verify virtual address range.

  Validates that a virtual address range is appropriate for SV32.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
Sv32Verify (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  /* Nothing to check. */
}

/**
  Initialize SV32 paging.

  Allocates and initializes the root page table for SV32.
**/
VOID
Sv32Initialize (
  VOID
  )
{
  gSv32Root = (PTE *) GetPayloadPage ();

  printf ("Using SV32 paging (root: %08lx).\n", gSv32Root);
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
Sv32MapPage (
  IN VOID     *PageTable,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN UINTN    PhysicalAddress,
  IN INT32    IsPayload,
  IN INT32    IsWritable,
  IN INT32    IsExecutable
  )
{
  PTE *L1Entry, *Root;
  UINT32 L1f;
  UINTN Page;

  Root = (PTE *) PageTable;

  printf ("Mapping at va %x PA %lx (p:%d, w:%d, x:%d)\n", (UINT32)VirtualAddress, PhysicalAddress, IsPayload,
	  IsWritable, IsExecutable);

  L1Entry = Sv32GetL1p (Root, VirtualAddress, IsPayload);
  L1f = (IsWritable ? PTE_W : 0) | (IsExecutable ? PTE_X : 0) | PTE_R | PTE_V;

  Page = (UINTN) PteGetAddr (L1Entry);
  assert (Page == 0);
  Page = PhysicalAddress >> PAGE_SHIFT;
  SetPte (L1Entry, Page, L1f);
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
Sv32PopulatePage (
  IN PTE   *Root,
  IN VIRTUAL_ADDRESS  VirtualAddress,
  IN INT32    IsUserMode,
  IN INT32    IsWritable,
  IN INT32    IsExecutable,
  IN INT32    IsPayload
  )
{
  PTE *L1Entry;
  UINT32 L1f;
  UINTN Page;

  L1Entry = Sv32GetL1p (Root, VirtualAddress, IsPayload);
  L1f = (IsUserMode ? PTE_U : 0) | (IsWritable ? PTE_W : 0) | (IsExecutable ? PTE_X : 0) | PTE_R | PTE_V;

  Page = (UINTN) PteGetAddr (L1Entry);
  if (Page == 0)
    {
      Page = IsPayload ? GetPayloadPage () : GetPage ();
      SetPte (L1Entry, Page >> PAGE_SHIFT, L1f);
    }
  else
    {
      UINT32 NewF;
      UINT32 OldF = PteGetFlags (L1Entry);

      NewF = PteMergeFlags (L1f, OldF);
      if (NewF != OldF)
	{
	  printf ("Flags changed from %x to %x\n", OldF, NewF);
	  SetPte (L1Entry, Page >> PAGE_SHIFT, NewF);
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
Sv32GetPhysical (
  IN VIRTUAL_ADDRESS  VirtualAddress
  )
{
  UINTN Page;
  PTE *L2Entry, *L1Entry;
  UINT32 L2off = L2OFF32 (VirtualAddress);
  UINT32 L1off = L1OFF32 (VirtualAddress);

  L2Entry = gSv32Root + L2off;

  L1Entry = (PTE *) PteGetAddr (L2Entry);
  assert (L1Entry != NULL);
  L1Entry += L1off;

  Page = (UINTN) PteGetAddr (L1Entry);
  assert (Page != 0);

  return Page |= (VirtualAddress & ~(PAGE_MASK));
}

/**
  Direct map physical region.

  Creates direct (1:1) physical to virtual mappings using large pages
  where possible (4MB).

  @param[in] Pt      Page table root.
  @param[in] PaBase   Physical base address.
  @param[in] Va       Virtual address.
  @param[in] Size     Size of region.
  @param[in] Mt       Memory type.
  @param[in] Payload  TRUE if allocating from payload pages.
  @param[in] X        TRUE if executable.
**/
VOID
Sv32DirectMap (
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
  UINT64 L2cnt = 0, L1cnt = 0;

#define MB4ALIGNED(_a) (((_a) & ((1 << 22) - 1)) == 0)

  /* Signed to unsigned: no one will ask us a 1<<32 bytes physmap. */
  Len = (SSIZE64) Size;
  Pa = PaBase;

  while (Len > 0)
    {

      if (MB4ALIGNED (Pa) && MB4ALIGNED (VirtualAddress) && Len >= (1 << 22))
	{
	  PTE *L2Entry = Root + L2OFF32(VirtualAddress);

	  SetPte (L2Entry, Pa >> PAGE_SHIFT,
		   (IsExecutable ? PTE_X : 0) | PTE_W | PTE_R | PTE_V);
	  VirtualAddress += (1 << 22);
	  Pa += (1 << 22);
	  Len -= (1 << 22);
	  L2cnt++;
	}
      else
	{
	  PTE *L1Entry = Sv32GetL1p (Root, VirtualAddress, IsPayload);

	  SetPte (L1Entry, Pa >> PAGE_SHIFT,
		   (IsExecutable ? PTE_X : 0) | PTE_W | PTE_R | PTE_V);
	  VirtualAddress += (1 << PAGE_SHIFT);
	  Pa += (1 << PAGE_SHIFT);
	  Len -= (1 << PAGE_SHIFT);
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
Sv32MapPhysical (
  IN VIRTUAL_ADDRESS           VirtualAddress,
  IN SIZE64          Size,
  IN UINT64            Pa,
  IN MEMORY_TYPE  Mt
  )
{
  Sv32DirectMap (gSv32Root, Pa, VirtualAddress, Size, Mt, 1, 0);
}

/**
  Allocate top-level page tables.

  Pre-allocates L1 page tables for a virtual address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
Sv32AllocateTopPageTable (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  UINT32 i, n;

  n = (Size + (1 << 22) - 1) >> 22;

  for (i = 0; i < n; i++)
    (VOID) Sv32GetL1p (gSv32Root, VirtualAddress + (i << PAGE_SHIFT), 1);
}

/**
  Allocate page tables.

  Pre-allocates L1 page tables for a virtual address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
Sv32AllocatePageTable (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  UINT32 i, n;

  n = Size >> PAGE_SHIFT;

  for (i = 0; i < n; i++)
    (VOID) Sv32GetL1p (gSv32Root, VirtualAddress + (i << PAGE_SHIFT), 1);
}

#define SV32_LINEAR_SHIFT (PAGE_SHIFT + 10)
#define SV32_LINEAR_SIZE  (1 << SV32_LINEAR_SHIFT)
#define SV32_LINEAR_ALIGN (SV32_LINEAR_SIZE - 1)

/**
  Create linear (recursive) mapping.

  Maps page table recursively for linear address space access.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
Sv32MapLinear (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  UINT32 L2off = L2OFF32 (VirtualAddress);
  PTE *L2Entry;

  if (VirtualAddress & SV32_LINEAR_ALIGN)
    {
      printf ("SV32 Linear VA %x not aligned (align mask: %x).\n",
	      (UINT32)VirtualAddress, SV32_LINEAR_ALIGN);
      exit (-1);
    }

  if (Size < SV32_LINEAR_SIZE)
    {
      printf ("SV32 Linear size %llx too small.\n", Size);
      exit (-1);
    }

  L2Entry = gSv32Root + L2off;
  SetPte (L2Entry, (UINTN) gSv32Root >> PAGE_SHIFT, PTE_V);
  printf ("Wrote %x at %p\n", *L2Entry, L2Entry);
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
Sv32Populate (
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
      Sv32PopulatePage (gSv32Root, VirtualAddress, IsUserMode, IsWritable, IsExecutable, 1);

      Len -= PAGE_CEILING (VirtualAddress) - VirtualAddress;
      VirtualAddress += PAGE_CEILING (VirtualAddress) - VirtualAddress;

    }
}

/**
  Jump to kernel entry point.

  Transfers control to the loaded kernel with SV32 paging enabled.

  @param[in] Entry  Kernel entry point virtual address.
**/
VOID
Sv32Entry (
  IN VIRTUAL_ADDRESS  Entry
  )
{
  PlatformEntry (ArchRiscV32, (VIRTUAL_ADDRESS) (UINTN) gSv32Root, Entry);
}
