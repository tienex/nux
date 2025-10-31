/** @file
  APXH Main Bootloader

  Main entry point and coordination logic for APXH bootloader.
  Manages executable image loading (ELF, PE, LE/LX, Mach-O, etc.),
  virtual address space setup, and boot information structures for
  both kernel and user-space payloads.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
**/

#include <apxh/internal.h>

static ARCH gImageArch;
static ARCH gKernelArch = ArchInvalid;  ///< Kernel architecture
static ARCH gUserArch = ArchInvalid;    ///< User architecture
static ARCH gHostArch = ArchInvalid;    ///< Host/CPU architecture
static BOOLEAN gMixedMode = FALSE;      ///< TRUE if kernel/user have different bitness
static BOOLEAN g32on64Mode = FALSE;     ///< TRUE if 32-bit on 64-bit CPU
static BOOLEAN g64Uon32KMode = FALSE;   ///< TRUE if 64-bit user on 32-bit kernel

/**
  Get kernel architecture.

  @return Kernel architecture.
**/
ARCH
GetKernelArchitecture (
  VOID
  )
{
  return gKernelArch;
}

/**
  Get user architecture.

  @return User architecture (ArchInvalid if no user).
**/
ARCH
GetUserArchitecture (
  VOID
  )
{
  return gUserArch;
}

/**
  Get host CPU architecture.

  @return Host CPU architecture.
**/
ARCH
GetHostArchitecture (
  VOID
  )
{
  return gHostArch;
}

/**
  Get mixed-mode flags.

  @return Flags indicating mixed-mode execution scenarios.
**/
UINT32
GetMixedModeFlags (
  VOID
  )
{
  UINT32 Flags = 0;

  if (g32on64Mode)
    Flags |= APXH_MIXEDMODE_32ON64;
  if (g64Uon32KMode)
    Flags |= APXH_MIXEDMODE_64UON32K;
  if (gMixedMode)
    Flags |= APXH_MIXEDMODE_MIXED;

  // Check for 32U-on-64K (32-bit user on 64-bit kernel)
  if (Is64BitArch(gKernelArch) && !Is64BitArch(gUserArch) && gUserArch != ArchInvalid)
    Flags |= APXH_MIXEDMODE_32UON64K;

  return Flags;
}
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

  // Detect host CPU architecture
  gHostArch = ArchitectureGetNative();
}

/**
  Check if architecture is 64-bit.

  @param[in] Arch  Architecture to check.

  @retval TRUE   Architecture is 64-bit.
  @retval FALSE  Architecture is 32-bit or invalid.
**/
static BOOLEAN
Is64BitArch (
  IN ARCH  Arch
  )
{
  switch (Arch) {
    case ArchAmd64:
    case ArchArm64:
    case ArchRiscV64:
    case ArchPpc64:
    case ArchMips64:
    case ArchIa64:
    case ArchSparc64:
    case ArchS390x:
    case ArchPaRisc64:
    case ArchLoongArch64:
      return TRUE;
    default:
      return FALSE;
  }
}

/**
  Analyze kernel/user architecture combination.

  Detects mixed-mode scenarios:
  - 32-bit kernel/user on 64-bit CPU (32-on-64)
  - 64-bit user on 32-bit kernel (64U-on-32K)
  - 32-bit user on 64-bit kernel (32U-on-64K)

  Sets global flags accordingly.
**/
static VOID
AnalyzeArchitectureCombination (
  VOID
  )
{
  BOOLEAN KernelIs64 = Is64BitArch(gKernelArch);
  BOOLEAN UserIs64 = Is64BitArch(gUserArch);
  BOOLEAN HostIs64 = Is64BitArch(gHostArch);

  // Reset flags
  gMixedMode = FALSE;
  g32on64Mode = FALSE;
  g64Uon32KMode = FALSE;

  // Check for 64-bit user on 32-bit kernel
  if (!KernelIs64 && UserIs64) {
    g64Uon32KMode = TRUE;
    gMixedMode = TRUE;
    info("Detected 64-bit user on 32-bit kernel (64U-on-32K mode)");
    info("  Kernel: %s (32-bit)", ArchitectureGetName(gKernelArch));
    info("  User:   %s (64-bit)", ArchitectureGetName(gUserArch));

    if (!HostIs64) {
      warn("64U-on-32K requires 64-bit capable CPU!");
      warn("Host CPU is: %s", ArchitectureGetName(gHostArch));
    }
  }
  // Check for 32-bit kernel on 64-bit host
  else if (!KernelIs64 && HostIs64) {
    g32on64Mode = TRUE;
    info("Detected 32-bit kernel on 64-bit CPU (32-on-64 mode)");
    info("  Kernel: %s (32-bit)", ArchitectureGetName(gKernelArch));
    info("  Host:   %s (64-bit)", ArchitectureGetName(gHostArch));
  }
  // Check for 32-bit user on 64-bit kernel
  else if (KernelIs64 && !UserIs64 && gUserArch != ArchInvalid) {
    gMixedMode = TRUE;
    info("Detected 32-bit user on 64-bit kernel (32U-on-64K mode)");
    info("  Kernel: %s (64-bit)", ArchitectureGetName(gKernelArch));
    info("  User:   %s (32-bit)", ArchitectureGetName(gUserArch));
  }
  // Check if both are same bitness but different architectures
  else if (gKernelArch != gUserArch && gUserArch != ArchInvalid) {
    info("Kernel and user have different architectures:");
    info("  Kernel: %s", ArchitectureGetName(gKernelArch));
    info("  User:   %s", ArchitectureGetName(gUserArch));
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

  // Architecture and mixed-mode information
  BootInfo.KernelArchitecture = (UINT32)gKernelArch;
  BootInfo.UserArchitecture = (UINT32)gUserArch;
  BootInfo.HostArchitecture = (UINT32)gHostArch;
  BootInfo.MixedModeFlags = GetMixedModeFlags();

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
  VOID *KernelImageStart, *UserImageStart;
  SIZE64 KernelImageSize, UserImageSize;
  UINT64 KEntry, UEntry;
  ARCH VasArch;

  printf ("\nAPXH started.\n\n");

  Initialize ();

  /*
     Peek at both payloads to determine architecture before VAS initialization.
     This is critical for mixed-mode scenarios like 64U-on-32K where we need
     64-bit page tables even though the kernel is 32-bit.
   */
  KernelImageStart = GetPayloadStart (ArgumentCount, ArgumentVector, PayloadKernel);
  KernelImageSize = GetPayloadSize (PayloadKernel);
  gKernelArch = GetImageArch (KernelImageStart);

  UserImageStart = GetPayloadStart (ArgumentCount, ArgumentVector, PayloadUser);
  UserImageSize = GetPayloadSize (PayloadUser);
  if (UserImageStart != NULL && UserImageSize != 0) {
    gUserArch = GetImageArch (UserImageStart);
  } else {
    gUserArch = ArchInvalid;
  }

  /*
     Determine VAS architecture based on mixed-mode requirements:
     - 64U-on-32K: Use 64-bit page tables (user arch) to map 64-bit user space
     - Otherwise: Use kernel architecture (kernel controls paging)
   */
  VasArch = gKernelArch;
  if (!Is64BitArch(gKernelArch) && Is64BitArch(gUserArch)) {
    // 64-bit user on 32-bit kernel - requires 64-bit page tables
    VasArch = gUserArch;
    info("64U-on-32K detected: Using 64-bit page tables for 32-bit kernel");
    info("  Kernel: %s (32-bit)", ArchitectureGetName(gKernelArch));
    info("  User:   %s (64-bit)", ArchitectureGetName(gUserArch));
    info("  VAS:    %s (64-bit page tables)", ArchitectureGetName(VasArch));
  } else if (Is64BitArch(gKernelArch) && !Is64BitArch(gUserArch) && gUserArch != ArchInvalid) {
    // 32-bit user on 64-bit kernel - use 64-bit page tables
    info("32U-on-64K detected: Using 64-bit page tables");
    info("  Kernel: %s (64-bit)", ArchitectureGetName(gKernelArch));
    info("  User:   %s (32-bit)", ArchitectureGetName(gUserArch));
  } else if (gKernelArch != gHostArch) {
    // 32-on-64 or other compatibility mode
    info("Compatibility mode: Kernel %s on %s CPU",
         ArchitectureGetName(gKernelArch),
         ArchitectureGetName(gHostArch));
  }

  // Set gImageArch for VAS initialization
  gImageArch = VasArch;

  printf ("Kernel payload %s at addr %p (%d bytes)\n",
	  GetArchName (gKernelArch), KernelImageStart, KernelImageSize);
  if (gUserArch != ArchInvalid) {
    printf ("User payload %s at addr %p (%d bytes)\n",
	    GetArchName (gUserArch), UserImageStart, UserImageSize);
  }

  // Initialize architecture handlers and VAS with selected architecture
  ArchitecturesInit ();
  VasInitialize ();

  /*
     Load kernel.
   */
  KEntry = LoadExecutable (KernelImageStart, FALSE);
  if (KEntry == (VIRTUAL_ADDRESS)-1) {
    printf ("Failed to load kernel image");
    exit (-1);
  }
  printf ("Kernel entry: %llx\n", KEntry);

  /*
     Load user if it exists.
   */
  if (gUserArch != ArchInvalid)
    {
      // Load user image (format and bitness detected automatically)
      UEntry = LoadExecutable (UserImageStart, TRUE);
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

  // Analyze kernel/user architecture combination
  // Detects 32-on-64, 64U-on-32K, and other mixed modes
  AnalyzeArchitectureCombination();

  /* Stop allocations as we're copying boot-time allocation. */
  gStopPayloadAllocation = TRUE;
  VasMapInfoCopy (UEntry, gReqRegionNum);
  VasMapPageFrameNumbersCopy ();
  VasMapBatreeCopy ();
  VasMapRegionsCopy ();

  // Transfer control to kernel using kernel's architecture
  // For mixed-mode (e.g., 64U-on-32K), this ensures we use the correct entry mechanism
  VasSetEntry (KEntry, gKernelArch);
  return 0;
}
