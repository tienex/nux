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
  __attribute__((aligned (4096)));
static VIRTUAL_ADDRESS gReqPfnmapVa, gReqInfoVa, gReqStreeVa, gReqRegionVa,
  gKtlsVa, gUtlsVa;
static SIZE64 gReqPfnmapSize, gReqInfoSize, gReqStreeSize,
  gReqRegionSize, gKtlsInitsize, gKtlsSize, gUtlsInitsize, gUtlsSize;
static unsigned gReqStreeOrder, gReqRegionNum;
static bool gStopPayloadAllocation = false;
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
  unsigned Pfn;
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
unsigned
CheckPayloadPage (
  IN unsigned Addr
  )
{
  unsigned i = (Addr - gMinRamAddr) >> PAGE_SHIFT;
  unsigned By = i >> 3;
  unsigned Bi = (1 << (i & 7));

  assert (By <= PAGEMAP_SZ (BOOTMEM));

  return !!(gBootPagemap[By] & Bi);
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
  MdInit ();
  gMinRamAddr = MdMinRamPfn () << PAGE_SHIFT;
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
    case ARCH_INVALID:
      return "invalid";
    case ARCH_UNSUPPORTED:
      return "unsupported";
    case ARCH_386:
      return "i386";
    case ARCH_AMD64:
      return "AMD64";
    case ARCH_RISCV64:
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
VaInitialize (
  VOID
  )
{
  switch (gElfArch)
    {
#if (EC_MACHINE_I386) || (EC_MACHINE_AMD64)
    case ARCH_386:
      PaeInit ();
      break;
    case ARCH_AMD64:
      Pae64Init ();
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      Sv48Init ();
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
VaPopulate (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size,
  IN int       U,
  IN int       W,
  IN int       X
  )
{
  MdVerify (Va, Size);
  VaVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      PaePopulate (Va, Size, U, W, X);
      break;
    case ARCH_AMD64:
      Pae64Populate (Va, Size, U, W, X);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
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
VaCopy (
  IN VIRTUAL_ADDRESS   Va,
  IN VOID      *Addr,
  IN SIZE64  Size,
  IN int       U,
  IN int       W,
  IN int       X
  )
{
  SSIZE64 Len = Size;

  MdVerify (Va, Size);
  VaVerify (Va, Size);

#if 0
  printf ("Copying %08llx <- %p (u: %d, w:%d, x:%d, %d bytes)\n", Va, Addr, U,
	  W, X, Size);
#endif
  VaPopulate (Va, Size, U, W, X);

  while (Len > 0)
    {
      UINTN PAddr;
      SIZE64 CLen = PAGE_CEILING (Va) - Va;

      if (CLen > Len)
	CLen = Len;

      PAddr = VaGetPhys (Va);

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
VaMemset (
  IN VIRTUAL_ADDRESS   Va,
  IN int       C,
  IN SIZE64  Size,
  IN int       U,
  IN int       W,
  IN int       X
  )
{
  SSIZE64 Len = Size;

  MdVerify (Va, Size);
  VaVerify (Va, Size);

  printf ("Setting %08llx <- %d (u:%d, w:%d, x: %d, %d bytes)\n", Va, C, U, W,
	  X, Size);
  VaPopulate (Va, Size, U, W, X);

  while (Len > 0)
    {
      UINTN PAddr;
      SIZE64 CLen = PAGE_CEILING (Va) - Va;

      PAddr = VaGetPhys (Va);

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
VaPhysmap (
  IN VIRTUAL_ADDRESS           Va,
  IN SIZE64          Size,
  IN MEMORY_TYPE  Mt
  )
{
  MdVerify (Va, Size);
  VaVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      PaePhysmap (Va, Size, 0, Mt);
      break;
    case ARCH_AMD64:
      Pae64Physmap (Va, Size, 0, Mt);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      Sv48Physmap (Va, Size, 0, Mt);
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
VaFramebuf (
  IN VIRTUAL_ADDRESS           Va,
  IN SIZE64          Size,
  IN MEMORY_TYPE  Mt
  )
{
  UINT64 Pa;
  FRAMEBUFFER_DESC *FbPtr;

  MdVerify (Va, Size);
  VaVerify (Va, Size);

  FbPtr = MdGetFramebuffer ();
  if (FbPtr == NULL || FbPtr->type == FB_INVALID)
    return;

  if (FbPtr->size > Size)
    {
      printf ("ERROR: framebuffer too big. Shrinking int from %lx to %lx\n",
	      FbPtr->size, Size);
      FbPtr->size = Size;
    }

  Pa = FbPtr->addr;

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      PaePhysmap (Va, Size, Pa, Mt);
      break;
    case ARCH_AMD64:
      pae64_physmap (Va, Size, Pa, Mt);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      sv48_physmap (Va, Size, Pa, Mt);
      break;
#endif
    default:
      (void) Pa;
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
VaLinear (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  MdVerify (Va, Size);
  VaVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      PaeLinear (Va, Size);
      break;
    case ARCH_AMD64:
      Pae64Linear (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      Sv48Linear (Va, Size);
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
VaTopPtAlloc (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  MdVerify (Va, Size);
  VaVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      PaeTopPtAlloc (Va, Size);
      break;
    case ARCH_AMD64:
      Pae64TopPtAlloc (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      Sv48TopPtAlloc (Va, Size);
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
VaPtAlloc (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  MdVerify (Va, Size);
  VaVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      PaePtAlloc (Va, Size);
      break;
    case ARCH_AMD64:
      Pae64PtAlloc (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      Sv48PtAlloc (Va, Size);
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
VaInfo (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  MdVerify (Va, Size);
  VaVerify (Va, Size);

  VaPopulate (Va, Size, 0, 0, 0);

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
VaInfoCopy (
  IN UINT64   UEntry,
  IN UINT64   NumRegions
  )
{
  VIRTUAL_ADDRESS Va = gReqInfoVa;
  SIZE64 Size = gReqInfoSize;
#define MIN(x,y) ((x < y) ? x : y)
  struct apxh_bootinfo i;
  FRAMEBUFFER_DESC *FbPtr;
  APXH_PLATFORM_DESCRIPTOR *PlatformDesc;

  if (Va == 0)
    {
      /* No INFO. Skip. */
      return;
    }

  i.magic = APXH_BOOTINFO_MAGIC;
  i.maxpfn = MdMaxPfn ();
  i.maxrampfn = MdMaxRamPfn ();
  i.numregions = NumRegions;
  i.uentry = UEntry;

  PlatformDesc = MdGetPlatformDesc ();
  if (PlatformDesc != NULL)
    i.pltdesc = *PlatformDesc;
  else
    i.pltdesc = (APXH_PLATFORM_DESCRIPTOR)
    {.Type = PLATFORM_UNKNOWN,.PlatformPointer = 0 };

  FbPtr = MdGetFramebuffer ();
  if (FbPtr != NULL)
    i.fbdesc = *FbPtr;
  else
    i.fbdesc.type = FB_INVALID;

  i.ktls.initvaddr = gKtlsVa;
  i.ktls.initsize = gKtlsInitsize;
  i.ktls.size = gKtlsSize;

  i.utls.initvaddr = gUtlsVa;
  i.utls.initsize = gUtlsInitsize;
  i.utls.size = gUtlsSize;

  VaCopy (Va, &i, MIN (Size, sizeof (struct apxh_bootinfo)), 0, 0, 0);
#undef MIN
}


#define OR_WORD(p, x) ((*(UINT64 *)VaGetPhys(gReqStreeVa + (VIRTUAL_ADDRESS)(UINTN)(p))) |= (x))
#define MASK_WORD(p,x) ((*(UINT64 *)VaGetPhys(gReqStreeVa + (VIRTUAL_ADDRESS)(UINTN)(p))) &= (x))
#define GET_WORD(p) (*(UINT64 *)VaGetPhys(gReqStreeVa + (VIRTUAL_ADDRESS)(UINTN)(p)))
#define SET_WORD(p,x) (*(UINT64 *)VaGetPhys(gReqStreeVa + (VIRTUAL_ADDRESS)(UINTN)(p)) = x)
#include <nux/batree.h>

/**
  Set up S-tree structure.

  Creates buddy allocator S-tree for tracking free page frames.
  Marks all RAM regions as free and non-RAM regions as busy.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VaStree (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  SIZE64 s;
  int i, Order;
  APXH_STREE Hdr;
  BOOTINFO_REGION *Reg;
  unsigned Regions = MdMemRegions ();
  unsigned MaxFrame = MdMaxRamPfn ();

  MdVerify (Va, Size);
  VaVerify (Va, Size);

  Order = BatreeOrder (MaxFrame);
  s = 8 * BATREE_SIZE (Order);
  s += sizeof (APXH_STREE);

  if (s > Size)
    {
      printf ("Can't create PFN S-Tree of order %d: "
	      "required %d bytes, %d available.\n", Order, s, Size);
    }

  Size = s;
  printf ("Populating size %d (order: %d)\n", Size, Order);
  VaPopulate (Va, Size, 0, 1, 0);

  /* Copy the header. */
  Hdr.magic = APXH_STREE_MAGIC;
  Hdr.version = APXH_STREE_VERSION;
  Hdr.order = Order;
  Hdr.offset = sizeof (Hdr);
  Hdr.size = 8 * BATREE_SIZE (Order);
  VaCopy (Va, &Hdr, sizeof (Hdr), 0, 1, 0);

  /* Fill the S-Tree with all RAM regions. */
  gReqStreeVa = Va + sizeof (Hdr);

  for (i = 0; i < Regions; i++)
    {
      unsigned j;

      Reg = MdGetMemRegion (i);

      if (Reg->type != BOOTINFO_REGION_RAM)
	continue;


      for (j = 0; j < Reg->len; j++)
	{
	  unsigned Frame = Reg->pfn + j;

	  if (Frame > MaxFrame)
	    {
	      printf ("Maximum reached.\n");
	      break;
	    }

	  BatreeSetBit ((WORD_T *) 0, Order, Frame);
	}
    }

  /* Clear in case of overlapping non-ram regions. */
  for (i = 0; i < Regions; i++)
    {
      unsigned j;

      Reg = MdGetMemRegion (i);

      if (Reg->type == BOOTINFO_REGION_RAM)
	continue;


      for (j = 0; j < Reg->len; j++)
	{
	  unsigned Frame = Reg->pfn + j;

	  if (Frame > MaxFrame)
	    {
	      printf ("Maximum reached.\n");
	      break;
	    }

	  BatreeClrBit ((WORD_T *) 0, Order, Frame);
	}
    }


  /* We'll need to continue to update allocated pages. */
  gReqStreeOrder = Order;
  gReqStreeSize = Size;
}

/**
  Update S-tree with allocations.

  Marks all pages allocated by bootloader as busy in the S-tree.
**/
VOID
VaStreeCopy (
  VOID
  )
{
  VIRTUAL_ADDRESS Va = gReqStreeVa;
  unsigned Order = gReqStreeOrder;
  UINT64 Pa;
  VIRTUAL_ADDRESS MaxFrame;

  if (Va == 0)
    {
      /* No STREE. Skip. */
      return;
    }

  MaxFrame = MdMaxRamPfn ();

  for (Pa = gMinRamAddr; Pa < BOOTMEM + gMinRamAddr; Pa += PAGE_SIZE)
    {
      unsigned Frame = Pa >> PAGE_SHIFT;

      if (Frame > MaxFrame)
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
VaRegions (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  unsigned MaxRegion;
  unsigned Regions = MdMemRegions ();

  MdVerify (Va, Size);
  VaVerify (Va, Size);

  MaxRegion = Size / sizeof (struct apxh_region);

  if (MaxRegion > Regions)
    MaxRegion = Regions;

  Size = Regions * sizeof (struct apxh_region);

  printf ("Size of area: %lld = %ld * %d\n", Size, Regions,
	  sizeof (struct apxh_region));
  VaPopulate (Va, Size, 0, 0, 0);

  gReqRegionVa = Va;
  gReqRegionSize = Size;
  gReqRegionNum = MaxRegion;
}

/**
  Copy memory regions data.

  Fills memory regions array with platform memory map.
**/
static VOID
VaRegionsCopy (
  VOID
  )
{
  VIRTUAL_ADDRESS Va = gReqRegionVa;
  unsigned long Size = gReqRegionSize;
  unsigned i, Regions;
  struct apxh_region ApxhReg;
  BOOTINFO_REGION *Reg;

  if (Va == 0)
    {
      /* No REGIONS. Skip. */
      return;
    }

  Regions = Size / sizeof (struct apxh_region);

  for (i = 0; i < Regions; i++)
    {
      Reg = MdGetMemRegion (i);
      ApxhReg.type = Reg->type;
      ApxhReg.pfn = Reg->pfn;
      ApxhReg.len = Reg->len;
#if 0
      printf ("Copying %d %d %d\n", ApxhReg.type, ApxhReg.pfn, ApxhReg.len);
#endif
      VaCopy (Va + i * sizeof (struct apxh_region), &ApxhReg,
	       sizeof (struct apxh_region), 0, 0, 0);
    }
}

/**
  Set up page frame number map.

  Creates a map of all page frames with their types (RAM, ACPI, etc).

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
VaPfnmap (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  unsigned i, MaxFrame;
  BOOTINFO_REGION *Reg;
  unsigned Regions = MdMemRegions ();

  MdVerify (Va, Size);
  VaVerify (Va, Size);

  MaxFrame = Size / PFNMAP_ENTRY_SIZE;

  if (MaxFrame > MdMaxPfn ())
    {
      MaxFrame = MdMaxPfn ();
      Size = MaxFrame * PFNMAP_ENTRY_SIZE;
    }

  VaPopulate (Va, Size, 0, 1, 0);

  for (i = 0; i < Regions; i++)
    {
      unsigned j;

      Reg = MdGetMemRegion (i);

      printf ("Reg: %d Type %02d, PA: %016llx (%ld)\n", i, Reg->type,
	      (UINT64) Reg->pfn << PAGE_SHIFT, Reg->len);


      for (j = 0; j < Reg->len; j++)
	{
	  unsigned Frame = Reg->pfn + j;
	  UINT8 *Ptr;

	  if (Frame > MaxFrame)
	    {
	      printf ("Maximum reached.\n");
	      break;
	    }

	  Ptr = (UINT8 *) VaGetPhys (Va + Frame * PFNMAP_ENTRY_SIZE);
	  assert (Ptr != NULL);

	  /* There's  a  priority  in  numbering of  regions.  RAM  is
	     lowest, overwritten most easily. */
	  if (*Ptr < Reg->type)
	    *Ptr = Reg->type;
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
VaPfnmapCopy (
  VOID
  )
{
  VIRTUAL_ADDRESS Va = gReqPfnmapVa;
  unsigned long Size = gReqPfnmapSize;
  unsigned MaxFrame = Size / PFNMAP_ENTRY_SIZE;
#define MIN(x,y) ((x < y) ? x : y)
  unsigned long Pa;

  if (Va == 0)
    {
      /* No PFNMAP. Skip. */
      return;
    }

  for (Pa = gMinRamAddr; Pa < BOOTMEM + gMinRamAddr; Pa += PAGE_SIZE)
    {
      unsigned Frame = Pa >> PAGE_SHIFT;

      if (Frame > MaxFrame)
	break;

      if (CheckPayloadPage (Pa))
	{
	  /* Page is allocated. Mark as BSY. */

	  UINT8 *Ptr =
	    (UINT8 *) VaGetPhys (Va + Frame * PFNMAP_ENTRY_SIZE);
	  assert (Ptr != NULL);

	  *Ptr = BOOTINFO_REGION_BSY;
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
VaKtls (
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
VaUtls (
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
VaVerify (
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
  )
{
  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      PaeVerify (Va, Size);
      break;
    case ARCH_AMD64:
      Pae64Verify (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
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
VaGetPhys (
  IN VIRTUAL_ADDRESS  Va
  )
{
  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      return PaeGetPhys (Va);
      break;
    case ARCH_AMD64:
      return Pae64GetPhys (Va);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      return Sv48GetPhys (Va);
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
VaEntry (
  IN VIRTUAL_ADDRESS  Entry
  )
{
  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      PaeEntry (Entry);
      break;
    case ARCH_AMD64:
      Pae64Entry (Entry);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
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
  IN int   Argc,
  IN char  *Argv[]
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
  ElfStart = GetPayloadStart (Argc, Argv, PayloadKernel);
  ElfSize = GetPayloadSize (PayloadKernel);
  gElfArch = GetElfArch (ElfStart);
  printf ("Kernel payload %s ELF at addr %p (%d bytes)\n",
	  GetArchName (gElfArch), ElfStart, ElfSize);

  VaInitialize ();

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      KEntry = LoadElf32 (ElfStart, 0);
      break;
    case ARCH_AMD64:
      KEntry = LoadElf64 (ElfStart, 0);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      KEntry = LoadElf64 (ElfStart, 0);
      break;
#endif
    default:
      printf ("Unsupported ELF architecture");
      exit (-1);
    }

  printf ("Kernel entry: %llx\n", KEntry);

  /*
     Load user if it exists.
   */
  ElfStart = GetPayloadStart (Argc, Argv, PayloadUser);
  ElfSize = GetPayloadSize (PayloadUser);
  if (ElfStart != NULL && ElfSize != 0)
    {
      gElfArch = GetElfArch (ElfStart);
      printf ("User payload %s ELF at addr %p (%d bytes)\n",
	      GetArchName (gElfArch), ElfStart, ElfSize);

      switch (gElfArch)
	{
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
	case ARCH_386:
	  UEntry = LoadElf32 (ElfStart, 1);
	  break;
	case ARCH_AMD64:
	  UEntry = LoadElf64 (ElfStart, 1);
	  break;
#endif
#if EC_MACHINE_RISCV64
	case ARCH_RISCV64:
	  UEntry = LoadElf64 (ElfStart, 1);
	  break;
#endif
	default:
	  printf ("Unsupported ELF architecture");
	  exit (-1);
	}
      printf ("User entry: %llx\n", UEntry);
    }
  else
    {
      UEntry = 0;
    }

  /* Stop allocations as we're copying boot-time allocation. */
  gStopPayloadAllocation = true;
  VaInfoCopy (UEntry, gReqRegionNum);
  VaPfnmapCopy ();
  VaStreeCopy ();
  VaRegionsCopy ();

  VaEntry (KEntry);
  return 0;
}
