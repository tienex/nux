/** @file
  APXH Main Bootloader

  Main entry point and coordination logic for APXH ELF bootloader.
  Manages ELF loading, virtual address space setup, and boot information
  structures for both kernel and user-space payloads.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
**/

#include <apxh/internal.h>

static ARCH gElfArch;
static UINT8 gBootPagemap[PAGEMAP_SZ (BOOTMEM)]
  ANX_ATTR_ALIGN(4096);
static VIRTUAL_ADDRESS gReqPfnmapVa, gReqInfoVa, gReqBatreeVa, gReqRegionVa,
  gKtlsVa, gUtlsVa;
static SIZE64 gReqPfnmapSize, gReqInfoSize, gReqBatreeSize,
  gReqRegionSize, gKtlsInitsize, gKtlsSize, gUtlsInitsize, gUtlsSize;
static UINT32 gReqBatreeOrder, gReqRegionNum;
static BOOLEAN gStopPayloadAllocation = FALSE;
static UINT64 gMinRamAddr = 0;


/**
  Allocate page from payload area.

  Allocates a physical page from the bootloader's payload region
  and tracks it in the boot pagemap for later marking as busy.

  @return Physical address of allocated page.
**/
UINTN
GetPayloadPage (
  VOID
  )
{
  UINT32 Pfn;
  UINTN Page;
  UINTN Base = gMinRamAddr;

  assert (!gStopPayloadAllocation);

  Page = GetPage ();
  assert (Page - Base < BOOTMEM);
  memset ((VOID *) Page, 0, PAGE_SIZE);

  Pfn = Page >> PAGE_SHIFT;

  gBootPagemap[(Page - Base) >> (PAGE_SHIFT + 3)] |= 1 << (Pfn & 7);

  return Page;
}

/**
  Check if payload page is allocated.

  Determines whether a physical page has been allocated from the
  payload region.

  @param[in] Addr  Physical address to check.

  @return TRUE if page is allocated, FALSE otherwise.
**/
UINT32
CheckPayloadPage (
  IN UINT32 Addr
  )
{
  UINT32 PageIndex = (Addr - gMinRamAddr) >> PAGE_SHIFT;
  UINT32 ByteIndex = PageIndex >> 3;
  UINT32 BitMask = (1 << (PageIndex & 7));

  assert (ByteIndex <= PAGEMAP_SZ (BOOTMEM));

  return !!(gBootPagemap[ByteIndex] & BitMask);
}

/**
  Initialize bootloader.

  Performs early bootloader initialization including machine-specific
  setup and determining minimum RAM address.
**/
VOID
Initialize (
  VOID
  )
{
  PlatformInit ();
  gMinRamAddr = PlatformGetMinRamPageFrameNumber () << PAGE_SHIFT;
}

/**
  Get architecture name.

  Returns human-readable architecture name for display.

  @param[in] Arch  Architecture enumeration value.

  @return Pointer to architecture name string.
**/
CONST CHAR *
GetArchName (
  IN ARCH  Arch
  )
{
  switch (Arch)
    {
    case ArchInvalid:
      return "invalid";
    case ArchUnsupported:
      return "unsupported";
    case Arch386:
      return "i386";
    case ArchAmd64:
      return "AMD64";
    case ArchRiscV64:
      return "RISCV64";
    default:
      return "unknown";
    }
}

/**
  Initialize virtual address subsystem.

  Sets up paging structures for the target architecture.
**/
VOID
VasInitialize (
  VOID
  )
{
  switch (gElfArch)
    {
#if (EC_MACHINE_I386) || (EC_MACHINE_AMD64)
    case Arch386:
      PaeInitialize ();
      break;
    case ArchAmd64:
      Pae64Initialize ();
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48Initialize ();
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Populate virtual address range.

  Allocates and maps pages for the specified virtual address range
  with given permissions.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
  @param[in] U     TRUE for user-accessible.
  @param[in] W     TRUE for writable.
  @param[in] X     TRUE for executable.
**/
VOID
VasPopulate (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size,
  IN INT32       U,
  IN INT32       W,
  IN INT32       X
  )
{
  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaePopulate (Va, Size, U, W, X);
      break;
    case ArchAmd64:
      Pae64Populate (Va, Size, U, W, X);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48Populate (Va, Size, U, W, X);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Copy data to virtual address.

  Copies data from physical address to virtual address range,
  allocating and mapping pages as needed.

  @param[in] Va    Virtual address destination.
  @param[in] Addr Physical address source.
  @param[in] Size  Size to copy.
  @param[in] U     TRUE for user-accessible.
  @param[in] W     TRUE for writable.
  @param[in] X     TRUE for executable.
**/
VOID
VasCopy (
  IN VIRTUAL_ADDRESS   Va,
  IN VOID      *Addr,
  IN SIZE64  Size,
  IN INT32       U,
  IN INT32       W,
  IN INT32       X
  )
{
  SSIZE64 Len = Size;

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

#if 0
  printf ("Copying %08llx <- %p (u: %d, w:%d, x:%d, %d bytes)\n", Va, Addr, U,
	  W, X, Size);
#endif
  VasPopulate (Va, Size, U, W, X);

  while (Len > 0)
    {
      UINTN PAddr;
      SIZE64 CLen = PAGE_CEILING (Va) - Va;

      if (CLen > Len)
	CLen = Len;

      PAddr = VasGetPhysical (Va);

      memcpy ((VOID *) PAddr, Addr, CLen);

      Len -= CLen;
      Va += CLen;
      Addr += CLen;
    }
}

/**
  Set virtual address range to value.

  Fills virtual address range with specified byte value,
  allocating and mapping pages as needed.

  @param[in] Va    Virtual address.
  @param[in] C     Byte value to set.
  @param[in] Size  Size of region.
  @param[in] U     TRUE for user-accessible.
  @param[in] W     TRUE for writable.
  @param[in] X     TRUE for executable.
**/
VOID
VasFill (
  IN VIRTUAL_ADDRESS   Va,
  IN INT32       FillChar,
  IN SIZE64  Size,
  IN INT32       U,
  IN INT32       W,
  IN INT32       X
  )
{
  SSIZE64 Len = Size;

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  printf ("Setting %08llx <- %d (u:%d, w:%d, x: %d, %d bytes)\n", Va, FillChar, U, W,
	  X, Size);
  VasPopulate (Va, Size, U, W, X);

  while (Len > 0)
    {
      UINTN PAddr;
      SIZE64 CLen = PAGE_CEILING (Va) - Va;

      PAddr = VasGetPhysical (Va);

      memset ((VOID *) PAddr, 0, CLen);

      Len -= CLen;
      Va += CLen;
    }
}

/**
  Map physical memory at virtual address.

  Creates identity mapping of physical memory starting at PA 0.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
  @param[in] Mt    Memory type (WC, WB, UC).
**/
VOID
VasMapPhysical (
  IN VIRTUAL_ADDRESS           Va,
  IN SIZE64          Size,
  IN MEMORY_TYPE  Mt
  )
{
  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeMapPhysical (Va, Size, 0, Mt);
      break;
    case ArchAmd64:
      Pae64MapPhysical (Va, Size, 0, Mt);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48MapPhysical (Va, Size, 0, Mt);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Map framebuffer at virtual address.

  Maps platform framebuffer memory to specified virtual address
  with appropriate memory type.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
  @param[in] Mt    Memory type (typically WC for framebuffer).
**/
VOID
VasMapFramebuffer (
  IN VIRTUAL_ADDRESS           Va,
  IN SIZE64          Size,
  IN MEMORY_TYPE  Mt
  )
{
  UINT64 Pa;
  FRAMEBUFFER_DESC *FbPtr;

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  FbPtr = PlatformGetFramebuffer ();
  if (FbPtr == NULL || FbPtr->Type == FB_INVALID)
    return;

  if (FbPtr->Size > Size)
    {
      printf ("ERROR: framebuffer too big. Shrinking int from %lx to %lx\n",
	      FbPtr->Size, Size);
      FbPtr->Size = Size;
    }

  Pa = FbPtr->Addr;

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeMapPhysical (Va, Size, Pa, Mt);
      break;
    case ArchAmd64:
      Pae64MapPhysical (Va, Size, Pa, Mt);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48MapPhysical (Va, Size, Pa, Mt);
      break;
#endif
    default:
      (VOID) Pa;
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Set up linear (recursive) mapping.

  Creates recursive page table mapping allowing page tables to be
  accessed as regular memory.

  @param[in] Va    Virtual address for linear mapping.
  @param[in] Size  Size of region.
**/
VOID
VasMapLinear (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeMapLinear (Va, Size);
      break;
    case ArchAmd64:
      Pae64MapLinear (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48MapLinear (Va, Size);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Allocate top-level page tables.

  Pre-allocates top-level page table structures for address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VasAllocTopPageTable (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeAllocateTopPageTable (Va, Size);
      break;
    case ArchAmd64:
      Pae64AllocateTopPageTable (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48AllocateTopPageTable (Va, Size);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Allocate page tables.

  Pre-allocates page table structures for address range.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VasAllocPageTable (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeAllocatePageTable (Va, Size);
      break;
    case ArchAmd64:
      Pae64AllocatePageTable (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48AllocatePageTable (Va, Size);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Set up boot info structure.

  Allocates space for boot information that will be filled after
  all memory allocations complete.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VasMapInfo (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  VasPopulate (Va, Size, 0, 0, 0);

  /* Only save the va and size, we'll have to finish all allocations
     before we can return the proper data. */
  gReqInfoVa = Va;
  gReqInfoSize = Size;
}

/**
  Copy boot info data.

  Fills boot information structure with final system configuration
  including memory layout, TLS, framebuffer, and platform data.

  @param[in] UEntry      User-space entry point (0 if none).
  @param[in] NumRegions  Number of memory regions.
**/
static VOID
VasMapInfoCopy (
  IN UINT64   UEntry,
  IN UINT64   NumRegions
  )
{
  VIRTUAL_ADDRESS Va = gReqInfoVa;
  SIZE64 Size = gReqInfoSize;
#define MIN(x,y) ((x < y) ? x : y)
  APXH_BOOT_INFO BootInfo;
  FRAMEBUFFER_DESC *FbPtr;
  APXH_PLATFORM_DESCRIPTOR *PlatformDesc;

  if (Va == 0)
    {
      /* No INFO. Skip. */
      return;
    }

  BootInfo.Magic = APXH_BOOTINFO_MAGIC;
  BootInfo.MaxPfn = PlatformGetMaxPageFrameNumber ();
  BootInfo.MaxRamPfn = PlatformGetMaxRamPageFrameNumber ();
  BootInfo.NumRegions = NumRegions;
  BootInfo.UserEntry = UEntry;

  PlatformDesc = PlatformGetDescriptor ();
  if (PlatformDesc != NULL)
    BootInfo.PlatformDesc = *PlatformDesc;
  else
    BootInfo.PlatformDesc = (APXH_PLATFORM_DESCRIPTOR)
    {.Type = ApxhPlatformUnknown,.PlatformPointer = 0 };

  FbPtr = PlatformGetFramebuffer ();
  if (FbPtr != NULL)
    BootInfo.FramebufferDesc = *FbPtr;
  else
    BootInfo.FramebufferDesc.Type = FB_INVALID;

  BootInfo.KernelTls.InitializedDataVaddr = gKtlsVa;
  BootInfo.KernelTls.InitializedDataSize = gKtlsInitsize;
  BootInfo.KernelTls.TotalSize = gKtlsSize;

  BootInfo.UserTls.InitializedDataVaddr = gUtlsVa;
  BootInfo.UserTls.InitializedDataSize = gUtlsInitsize;
  BootInfo.UserTls.TotalSize = gUtlsSize;

  VasCopy (Va, &BootInfo, MIN (Size, sizeof (APXH_BOOT_INFO)), 0, 0, 0);
#undef MIN
}


#define OR_WORD(p, x) ((*(UINT64 *)VasGetPhysical(gReqBatreeVa + (VIRTUAL_ADDRESS)(UINTN)(p))) |= (x))
#define MASK_WORD(p,x) ((*(UINT64 *)VasGetPhysical(gReqBatreeVa + (VIRTUAL_ADDRESS)(UINTN)(p))) &= (x))
#define GET_WORD(p) (*(UINT64 *)VasGetPhysical(gReqBatreeVa + (VIRTUAL_ADDRESS)(UINTN)(p)))
#define SET_WORD(p,x) (*(UINT64 *)VasGetPhysical(gReqBatreeVa + (VIRTUAL_ADDRESS)(UINTN)(p)) = x)
#include <nux/batree.h>

/**
  Set up BAtree structure.

  Creates buddy allocator BAtree for tracking free page frames.
  Marks all RAM regions as free and non-RAM regions as busy.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VasMapBatree (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  SIZE64 RequiredSize;
  INT32 RegionIndex, Order;
  APXH_BATREE BatreeHeader;
  BOOTINFO_REGION *Region;
  UINT32 Regions = PlatformGetMemoryRegionCount ();
  UINT32 MaxPageFrameNumber = PlatformGetMaxRamPageFrameNumber ();

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  Order = BatreeOrder (MaxPageFrameNumber);
  RequiredSize = 8 * BATREE_SIZE (Order);
  RequiredSize += sizeof (APXH_BATREE);

  if (RequiredSize > Size)
    {
      printf ("Can't create PFN S-Tree of order %d: "
	      "required %d bytes, %d available.\n", Order, RequiredSize, Size);
    }

  Size = RequiredSize;
  printf ("Populating size %d (order: %d)\n", Size, Order);
  VasPopulate (Va, Size, 0, 1, 0);

  /* Copy the header. */
  BatreeHeader.Magic = APXH_BATREE_MAGIC;
  BatreeHeader.Version = APXH_BATREE_VERSION;
  BatreeHeader.Order = Order;
  BatreeHeader.Offset = sizeof (BatreeHeader);
  BatreeHeader.Size = 8 * BATREE_SIZE (Order);
  VasCopy (Va, &BatreeHeader, sizeof (BatreeHeader), 0, 1, 0);

  /* Fill the S-Tree with all RAM regions. */
  gReqBatreeVa = Va + sizeof (BatreeHeader);

  for (RegionIndex = 0; RegionIndex < Regions; RegionIndex++)
    {
      UINT32 j;

      Region = PlatformGetMemoryRegion (RegionIndex);

      if (Region->Type != BootInfoRegionRam)
	continue;


      for (j = 0; j < Region->Length; j++)
	{
	  UINT32 Frame = Region->PageFrameNumber + j;

	  if (Frame > MaxPageFrameNumber)
	    {
	      printf ("Maximum reached.\n");
	      break;
	    }

	  BatreeSetBit ((WORD_T *) 0, Order, Frame);
	}
    }

  /* Clear in case of overlapping non-ram regions. */
  for (RegionIndex = 0; RegionIndex < Regions; RegionIndex++)
    {
      UINT32 j;

      Region = PlatformGetMemoryRegion (RegionIndex);

      if (Region->Type == BootInfoRegionRam)
	continue;


      for (j = 0; j < Region->Length; j++)
	{
	  UINT32 Frame = Region->PageFrameNumber + j;

	  if (Frame > MaxPageFrameNumber)
	    {
	      printf ("Maximum reached.\n");
	      break;
	    }

	  BatreeClrBit ((WORD_T *) 0, Order, Frame);
	}
    }


  /* We'll need to continue to update allocated pages. */
  gReqBatreeOrder = Order;
  gReqBatreeSize = Size;
}

/**
  Update BAtree with allocations.

  Marks all pages allocated by bootloader as busy in the BAtree.
**/
VOID
VasMapBatreeCopy (
  VOID
  )
{
  VIRTUAL_ADDRESS Va = gReqBatreeVa;
  UINT32 Order = gReqBatreeOrder;
  UINT64 Pa;
  VIRTUAL_ADDRESS MaxPageFrameNumber;

  if (Va == 0)
    {
      /* No STREE. Skip. */
      return;
    }

  MaxPageFrameNumber = PlatformGetMaxRamPageFrameNumber ();

  for (Pa = gMinRamAddr; Pa < BOOTMEM + gMinRamAddr; Pa += PAGE_SIZE)
    {
      UINT32 Frame = Pa >> PAGE_SHIFT;

      if (Frame > MaxPageFrameNumber)
	break;

      if (CheckPayloadPage (Pa))
	{
	  /* Page is allocated. Mark as BSY. */
	  BatreeClrBit ((WORD_T *) 0, Order, Frame);
	}
    }
}

/**
  Set up memory regions array.

  Allocates space for memory regions array that will be filled
  after all allocations complete.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VasMapRegions (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  UINT32 MaxRegion;
  UINT32 Regions = PlatformGetMemoryRegionCount ();

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  MaxRegion = Size / sizeof (APXH_REGION);

  if (MaxRegion > Regions)
    MaxRegion = Regions;

  Size = Regions * sizeof (APXH_REGION);

  printf ("Size of area: %lld = %ld * %d\n", Size, Regions,
	  sizeof (APXH_REGION));
  VasPopulate (Va, Size, 0, 0, 0);

  gReqRegionVa = Va;
  gReqRegionSize = Size;
  gReqRegionNum = MaxRegion;
}

/**
  Copy memory regions data.

  Fills memory regions array with platform memory map.
**/
static VOID
VasMapRegionsCopy (
  VOID
  )
{
  VIRTUAL_ADDRESS Va = gReqRegionVa;
  UINTN Size = gReqRegionSize;
  UINT32 i, Regions;
  APXH_REGION ApxhReg;
  BOOTINFO_REGION *Reg;

  if (Va == 0)
    {
      /* No REGIONS. Skip. */
      return;
    }

  Regions = Size / sizeof (APXH_REGION);

  for (i = 0; i < Regions; i++)
    {
      Reg = PlatformGetMemoryRegion (i);
      ApxhReg.Type = Reg->Type;
      ApxhReg.Pfn = Reg->PageFrameNumber;
      ApxhReg.Length = Reg->Length;
#if 0
      printf ("Copying %d %d %d\n", ApxhReg.Type, ApxhReg.Pfn, ApxhReg.Length);
#endif
      VasCopy (Va + i * sizeof (APXH_REGION), &ApxhReg,
	       sizeof (APXH_REGION), 0, 0, 0);
    }
}

/**
  Set up page frame number map.

  Creates a map of all page frames with their types (RAM, ACPI, etc).

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VasMapPageFrameNumbers (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  UINT32 RegionIndex, MaxPageFrameNumber;
  BOOTINFO_REGION *Region;
  UINT32 Regions = PlatformGetMemoryRegionCount ();

  PlatformVerify (Va, Size);
  VasVerify (Va, Size);

  MaxPageFrameNumber = Size / PFNMAP_ENTRY_SIZE;

  if (MaxPageFrameNumber > PlatformGetMaxPageFrameNumber ())
    {
      MaxPageFrameNumber = PlatformGetMaxPageFrameNumber ();
      Size = MaxPageFrameNumber * PFNMAP_ENTRY_SIZE;
    }

  VasPopulate (Va, Size, 0, 1, 0);

  for (RegionIndex = 0; RegionIndex < Regions; RegionIndex++)
    {
      UINT32 j;

      Region = PlatformGetMemoryRegion (RegionIndex);

      printf ("Reg: %d Type %02d, PA: %016llx (%ld)\n", RegionIndex, Region->Type,
	      (UINT64) Region->PageFrameNumber << PAGE_SHIFT, Region->Length);


      for (j = 0; j < Region->Length; j++)
	{
	  UINT32 Frame = Region->PageFrameNumber + j;
	  UINT8 *Ptr;

	  if (Frame > MaxPageFrameNumber)
	    {
	      printf ("Maximum reached.\n");
	      break;
	    }

	  Ptr = (UINT8 *) VasGetPhysical (Va + Frame * PFNMAP_ENTRY_SIZE);
	  assert (Ptr != NULL);

	  /* There's  a  priority  in  numbering of  regions.  RAM  is
	     lowest, overwritten most easily. */
	  if (*Ptr < Region->Type)
	    *Ptr = Region->Type;
	}
    }

  gReqPfnmapVa = Va;
  gReqPfnmapSize = Size;
}

/**
  Update PFN map with allocations.

  Marks all pages allocated by bootloader as busy in the PFN map.
**/
static VOID
VasMapPageFrameNumbersCopy (
  VOID
  )
{
  VIRTUAL_ADDRESS Va = gReqPfnmapVa;
  UINTN Size = gReqPfnmapSize;
  UINT32 MaxFrame = Size / PFNMAP_ENTRY_SIZE;
#define MIN(x,y) ((x < y) ? x : y)
  UINTN Pa;

  if (Va == 0)
    {
      /* No PFNMAP. Skip. */
      return;
    }

  for (Pa = gMinRamAddr; Pa < BOOTMEM + gMinRamAddr; Pa += PAGE_SIZE)
    {
      UINT32 Frame = Pa >> PAGE_SHIFT;

      if (Frame > MaxFrame)
	break;

      if (CheckPayloadPage (Pa))
	{
	  /* Page is allocated. Mark as BSY. */

	  UINT8 *Ptr =
	    (UINT8 *) VasGetPhysical (Va + Frame * PFNMAP_ENTRY_SIZE);
	  assert (Ptr != NULL);

	  *Ptr = BootInfoRegionBusy;
	}
    }
#undef MIN
}

/**
  Set kernel TLS information.

  Records kernel Thread Local Storage configuration for boot info.

  @param[in] Va        Virtual address of TLS template.
  @param[in] InitSize  Size of initialized TLS data.
  @param[in] Size      Total TLS size including BSS.
**/
VOID
VasMapKernelTls (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  InitSize,
  IN SIZE64  Size
  )
{
  gKtlsVa = Va;
  gKtlsInitsize = InitSize;
  gKtlsSize = Size;
}

/**
  Set user TLS information.

  Records user-space Thread Local Storage configuration for boot info.

  @param[in] Va        Virtual address of TLS template.
  @param[in] InitSize  Size of initialized TLS data.
  @param[in] Size      Total TLS size including BSS.
**/
VOID
VasMapUserTls (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  InitSize,
  IN SIZE64  Size
  )
{
  gUtlsVa = Va;
  gUtlsInitsize = InitSize;
  gUtlsSize = Size;
}

/**
  Verify virtual address range.

  Validates that virtual address range is suitable for target
  architecture.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VasVerify (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeVerify (Va, Size);
      break;
    case ArchAmd64:
      Pae64Verify (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48Verify (Va, Size);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Get physical address from virtual.

  Translates virtual address to physical address using current
  page tables.

  @param[in] Va  Virtual address.

  @return Physical address.
**/
UINTN
VasGetPhysical (
  IN VIRTUAL_ADDRESS  Va
  )
{
  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      return PaeGetPhysical (Va);
      break;
    case ArchAmd64:
      return Pae64GetPhysical (Va);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      return Sv48GetPhysical (Va);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }
}

/**
  Transfer control to kernel.

  Performs final setup and transfers control to loaded kernel
  entry point. Does not return.

  @param[in] Entry  Kernel entry point address.
**/
VOID
VasSetEntry (
  IN VIRTUAL_ADDRESS  Entry
  )
{
  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
      PaeEntry (Entry);
      break;
    case ArchAmd64:
      Pae64Entry (Entry);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      Sv48Entry (Entry);
      break;
#endif
    default:
      printf ("Unsupported VM architecture.\n");
      exit (-1);
    }

  printf ("Returned from entry!");
  exit (-1);
}

/**
  Main bootloader entry point.

  Loads kernel and optional user-space payloads, sets up virtual
  address space and boot information structures, then transfers
  control to kernel.

  @param[in] Argc  Argument count.
  @param[in] Argv  Argument vector.

  @return Exit status (only on error).
**/
int
main (
  IN INT32   ArgumentCount,
  IN char  *ArgumentVector[]
  )
{
  VOID *ElfStart;
  SIZE64 ElfSize;
  UINT64 KEntry, UEntry;

  printf ("\nAPXH started.\n\n");

  Initialize ();

  /*
     Load kernel.
   */
  ElfStart = GetPayloadStart (ArgumentCount, ArgumentVector, PayloadKernel);
  ElfSize = GetPayloadSize (PayloadKernel);
  gElfArch = GetImageArch (ElfStart);
  printf ("Kernel payload %s at addr %p (%d bytes)\n",
	  GetArchName (gElfArch), ElfStart, ElfSize);

  // Verify architecture is supported before initializing VAS
  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case Arch386:
    case ArchAmd64:
      break;
#endif
#if EC_MACHINE_RISCV64
    case ArchRiscV64:
      break;
#endif
    default:
      printf ("Unsupported architecture for this platform");
      exit (-1);
    }

  VasInitialize ();

  // Load kernel image (format and bitness detected automatically)
  KEntry = LoadExecutable (ElfStart, FALSE);
  if (KEntry == (VIRTUAL_ADDRESS)-1) {
    printf ("Failed to load kernel image");
    exit (-1);
  }

  printf ("Kernel entry: %llx\n", KEntry);

  /*
     Load user if it exists.
   */
  ElfStart = GetPayloadStart (ArgumentCount, ArgumentVector, PayloadUser);
  ElfSize = GetPayloadSize (PayloadUser);
  if (ElfStart != NULL && ElfSize != 0)
    {
      gElfArch = GetImageArch (ElfStart);
      printf ("User payload %s at addr %p (%d bytes)\n",
	      GetArchName (gElfArch), ElfStart, ElfSize);

      // Load user image (format and bitness detected automatically)
      UEntry = LoadExecutable (ElfStart, TRUE);
      if (UEntry == (VIRTUAL_ADDRESS)-1) {
        printf ("Failed to load user image");
        exit (-1);
      }
      printf ("User entry: %llx\n", UEntry);
    }
  else
    {
      UEntry = 0;
    }

  /* Stop allocations as we're copying boot-time allocation. */
  gStopPayloadAllocation = TRUE;
  VasMapInfoCopy (UEntry, gReqRegionNum);
  VasMapPageFrameNumbersCopy ();
  VasMapBatreeCopy ();
  VasMapRegionsCopy ();

  VasSetEntry (KEntry);
  return 0;
}
