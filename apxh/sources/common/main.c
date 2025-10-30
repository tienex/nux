/** @file
  APXH Main Bootloader

  Main entry point and coordination logic for APXH ELF bootloader.
  Manages ELF loading, virtual address space setup, and boot information
  structures for both kernel and user-space payloads.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
**/

#include <apxh/internal.h>

static arch_t gElfArch;
static UINT8 gBootPagemap[PAGEMAP_SZ (BOOTMEM)]
  __attribute__((aligned (4096)));
static VIRTUAL_ADDRESS gReqPfnmapVa, gReqInfoVa, gReqStreeVa, gReqRegionVa,
  gKtlsVa, gUtlsVa;
static size64_t gReqPfnmapSize, gReqInfoSize, gReqStreeSize,
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

  Page = get_page ();
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
  md_init ();
  gMinRamAddr = md_minrampfn () << PAGE_SHIFT;
}

/**
  Get architecture name.

  Returns human-readable architecture name for display.

  @param[in] Arch  Architecture enumeration value.

  @return Pointer to architecture name string.
**/
CONST CHAR *
GetArchName (
  IN arch_t  Arch
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
      pae_init ();
      break;
    case ARCH_AMD64:
      pae64_init ();
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      sv48_init ();
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
  IN size64_t  Size,
  IN int       U,
  IN int       W,
  IN int       X
  )
{
  md_verify (Va, Size);
  VaVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      pae_populate (Va, Size, U, W, X);
      break;
    case ARCH_AMD64:
      pae64_populate (Va, Size, U, W, X);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      sv48_populate (Va, Size, U, W, X);
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
  IN size64_t  Size,
  IN int       U,
  IN int       W,
  IN int       X
  )
{
  ssize64_t Len = Size;

  md_verify (Va, Size);
  VaVerify (Va, Size);

#if 0
  printf ("Copying %08llx <- %p (u: %d, w:%d, x:%d, %d bytes)\n", Va, Addr, U,
	  W, X, Size);
#endif
  VaPopulate (Va, Size, U, W, X);

  while (Len > 0)
    {
      UINTN PAddr;
      size64_t CLen = PAGE_CEILING (Va) - Va;

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
  IN size64_t  Size,
  IN int       U,
  IN int       W,
  IN int       X
  )
{
  ssize64_t Len = Size;

  md_verify (Va, Size);
  VaVerify (Va, Size);

  printf ("Setting %08llx <- %d (u:%d, w:%d, x: %d, %d bytes)\n", Va, C, U, W,
	  X, Size);
  VaPopulate (Va, Size, U, W, X);

  while (Len > 0)
    {
      UINTN PAddr;
      size64_t CLen = PAGE_CEILING (Va) - Va;

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
  IN size64_t          Size,
  IN enum memory_type  Mt
  )
{
  md_verify (Va, Size);
  VaVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      pae_physmap (Va, Size, 0, Mt);
      break;
    case ARCH_AMD64:
      pae64_physmap (Va, Size, 0, Mt);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      sv48_physmap (Va, Size, 0, Mt);
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
  IN size64_t          Size,
  IN enum memory_type  Mt
  )
{
  UINT64 Pa;
  struct fbdesc *FbPtr;

  md_verify (Va, Size);
  VaVerify (Va, Size);

  FbPtr = md_getframebuffer ();
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
      pae_physmap (Va, Size, Pa, Mt);
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
  IN size64_t  Size
  )
{
  md_verify (Va, Size);
  VaVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      pae_linear (Va, Size);
      break;
    case ARCH_AMD64:
      pae64_linear (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      sv48_linear (Va, Size);
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
  IN size64_t  Size
  )
{
  md_verify (Va, Size);
  VaVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      pae_topptalloc (Va, Size);
      break;
    case ARCH_AMD64:
      pae64_topptalloc (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      sv48_topptalloc (Va, Size);
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
  IN size64_t  Size
  )
{
  md_verify (Va, Size);
  VaVerify (Va, Size);

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      pae_ptalloc (Va, Size);
      break;
    case ARCH_AMD64:
      pae64_ptalloc (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      sv48_ptalloc (Va, Size);
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
  IN size64_t  Size
  )
{
  md_verify (Va, Size);
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
  size64_t Size = gReqInfoSize;
#define MIN(x,y) ((x < y) ? x : y)
  struct apxh_bootinfo i;
  struct fbdesc *FbPtr;
  struct apxh_platformdesc *PlatformDesc;

  if (Va == 0)
    {
      /* No INFO. Skip. */
      return;
    }

  i.magic = APXH_BOOTINFO_MAGIC;
  i.maxpfn = md_maxpfn ();
  i.maxrampfn = md_maxrampfn ();
  i.numregions = NumRegions;
  i.uentry = UEntry;

  PlatformDesc = md_getplatformdesc ();
  if (PlatformDesc != NULL)
    i.pltdesc = *PlatformDesc;
  else
    i.pltdesc = (struct apxh_platformdesc)
    {.Type = PLATFORM_UNKNOWN,.PlatformPointer = 0 };

  FbPtr = md_getframebuffer ();
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
#include <stree.h>

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
  IN size64_t  Size
  )
{
  size64_t s;
  int i, Order;
  struct apxh_stree Hdr;
  struct bootinfo_region *Reg;
  unsigned Regions = md_memregions ();
  unsigned MaxFrame = md_maxrampfn ();

  md_verify (Va, Size);
  VaVerify (Va, Size);

  Order = stree_order (MaxFrame);
  s = 8 * STREE_SIZE (Order);
  s += sizeof (struct apxh_stree);

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
  Hdr.size = 8 * STREE_SIZE (Order);
  VaCopy (Va, &Hdr, sizeof (Hdr), 0, 1, 0);

  /* Fill the S-Tree with all RAM regions. */
  gReqStreeVa = Va + sizeof (Hdr);

  for (i = 0; i < Regions; i++)
    {
      unsigned j;

      Reg = md_getmemregion (i);

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

	  stree_setbit ((WORD_T *) 0, Order, Frame);
	}
    }

  /* Clear in case of overlapping non-ram regions. */
  for (i = 0; i < Regions; i++)
    {
      unsigned j;

      Reg = md_getmemregion (i);

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

	  stree_clrbit ((WORD_T *) 0, Order, Frame);
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

  MaxFrame = md_maxrampfn ();

  for (Pa = gMinRamAddr; Pa < BOOTMEM + gMinRamAddr; Pa += PAGE_SIZE)
    {
      unsigned Frame = Pa >> PAGE_SHIFT;

      if (Frame > MaxFrame)
	break;

      if (CheckPayloadPage (Pa))
	{
	  /* Page is allocated. Mark as BSY. */
	  stree_clrbit ((WORD_T *) 0, Order, Frame);
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
  IN size64_t  Size
  )
{
  unsigned MaxRegion;
  unsigned Regions = md_memregions ();

  md_verify (Va, Size);
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
  struct bootinfo_region *Reg;

  if (Va == 0)
    {
      /* No REGIONS. Skip. */
      return;
    }

  Regions = Size / sizeof (struct apxh_region);

  for (i = 0; i < Regions; i++)
    {
      Reg = md_getmemregion (i);
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
  IN size64_t  Size
  )
{
  unsigned i, MaxFrame;
  struct bootinfo_region *Reg;
  unsigned Regions = md_memregions ();

  md_verify (Va, Size);
  VaVerify (Va, Size);

  MaxFrame = Size / PFNMAP_ENTRY_SIZE;

  if (MaxFrame > md_maxpfn ())
    {
      MaxFrame = md_maxpfn ();
      Size = MaxFrame * PFNMAP_ENTRY_SIZE;
    }

  VaPopulate (Va, Size, 0, 1, 0);

  for (i = 0; i < Regions; i++)
    {
      unsigned j;

      Reg = md_getmemregion (i);

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
  IN size64_t  InitSize,
  IN size64_t  Size
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
  IN size64_t  InitSize,
  IN size64_t  Size
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
  IN size64_t  Size
  )
{
  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      pae_verify (Va, Size);
      break;
    case ARCH_AMD64:
      pae64_verify (Va, Size);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      sv48_verify (Va, Size);
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
      return pae_getphys (Va);
      break;
    case ARCH_AMD64:
      return pae64_getphys (Va);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      return sv48_getphys (Va);
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
      pae_entry (Entry);
      break;
    case ARCH_AMD64:
      pae64_entry (Entry);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      sv48_entry (Entry);
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
  size64_t ElfSize;
  UINT64 KEntry, UEntry;

  printf ("\nAPXH started.\n\n");

  Initialize ();

  /*
     Load kernel.
   */
  ElfStart = get_payload_start (Argc, Argv, PAYLOAD_KERNEL);
  ElfSize = get_payload_size (PAYLOAD_KERNEL);
  gElfArch = get_elf_arch (ElfStart);
  printf ("Kernel payload %s ELF at addr %p (%d bytes)\n",
	  GetArchName (gElfArch), ElfStart, ElfSize);

  VaInitialize ();

  switch (gElfArch)
    {
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
    case ARCH_386:
      KEntry = load_elf32 (ElfStart, 0);
      break;
    case ARCH_AMD64:
      KEntry = load_elf64 (ElfStart, 0);
      break;
#endif
#if EC_MACHINE_RISCV64
    case ARCH_RISCV64:
      KEntry = load_elf64 (ElfStart, 0);
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
  ElfStart = get_payload_start (Argc, Argv, PAYLOAD_USER);
  ElfSize = get_payload_size (PAYLOAD_USER);
  if (ElfStart != NULL && ElfSize != 0)
    {
      gElfArch = get_elf_arch (ElfStart);
      printf ("User payload %s ELF at addr %p (%d bytes)\n",
	      GetArchName (gElfArch), ElfStart, ElfSize);

      switch (gElfArch)
	{
#if EC_MACHINE_I386 || EC_MACHINE_AMD64
	case ARCH_386:
	  UEntry = load_elf32 (ElfStart, 1);
	  break;
	case ARCH_AMD64:
	  UEntry = load_elf64 (ElfStart, 1);
	  break;
#endif
#if EC_MACHINE_RISCV64
	case ARCH_RISCV64:
	  UEntry = load_elf64 (ElfStart, 1);
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

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use GetPayloadPage instead **/
UINTN get_payload_page (void) {
  return GetPayloadPage ();
}

/** @deprecated Use CheckPayloadPage instead **/
unsigned check_payload_page (unsigned addr) {
  return CheckPayloadPage (addr);
}

/** @deprecated Use Initialize instead **/
void init (void) {
  Initialize ();
}

/** @deprecated Use GetArchName instead **/
const char *get_arch_name (arch_t arch) {
  return GetArchName (arch);
}

/** @deprecated Use VaInitialize instead **/
void va_init (void) {
  VaInitialize ();
}

/** @deprecated Use VaPopulate instead **/
void va_populate (VIRTUAL_ADDRESS va, size64_t size, int u, int w, int x) {
  VaPopulate (va, size, u, w, x);
}

/** @deprecated Use VaCopy instead **/
void va_copy (VIRTUAL_ADDRESS va, void *addr, size64_t size, int u, int w, int x) {
  VaCopy (va, addr, size, u, w, x);
}

/** @deprecated Use VaMemset instead **/
void va_memset (VIRTUAL_ADDRESS va, int c, size64_t size, int u, int w, int x) {
  VaMemset (va, c, size, u, w, x);
}

/** @deprecated Use VaPhysmap instead **/
void va_physmap (VIRTUAL_ADDRESS va, size64_t size, enum memory_type mt) {
  VaPhysmap (va, size, mt);
}

/** @deprecated Use VaFramebuf instead **/
void va_framebuf (VIRTUAL_ADDRESS va, size64_t size, enum memory_type mt) {
  VaFramebuf (va, size, mt);
}

/** @deprecated Use VaLinear instead **/
void va_linear (VIRTUAL_ADDRESS va, size64_t size) {
  VaLinear (va, size);
}

/** @deprecated Use VaTopPtAlloc instead **/
void va_topptalloc (VIRTUAL_ADDRESS va, size64_t size) {
  VaTopPtAlloc (va, size);
}

/** @deprecated Use VaPtAlloc instead **/
void va_ptalloc (VIRTUAL_ADDRESS va, size64_t size) {
  VaPtAlloc (va, size);
}

/** @deprecated Use VaInfo instead **/
void va_info (VIRTUAL_ADDRESS va, size64_t size) {
  VaInfo (va, size);
}

/** @deprecated Use VaInfoCopy instead **/
static void va_info_copy (UINT64 uentry, UINT64 num_regions) {
  VaInfoCopy (uentry, num_regions);
}

/** @deprecated Use VaStree instead **/
void va_stree (VIRTUAL_ADDRESS va, size64_t size) {
  VaStree (va, size);
}

/** @deprecated Use VaStreeCopy instead **/
void va_stree_copy (void) {
  VaStreeCopy ();
}

/** @deprecated Use VaRegions instead **/
void va_regions (VIRTUAL_ADDRESS va, size64_t size) {
  VaRegions (va, size);
}

/** @deprecated Use VaRegionsCopy instead **/
static void va_regions_copy (void) {
  VaRegionsCopy ();
}

/** @deprecated Use VaPfnmap instead **/
void va_pfnmap (VIRTUAL_ADDRESS va, size64_t size) {
  VaPfnmap (va, size);
}

/** @deprecated Use VaPfnmapCopy instead **/
static void va_pfnmap_copy (void) {
  VaPfnmapCopy ();
}

/** @deprecated Use VaKtls instead **/
void va_ktls (VIRTUAL_ADDRESS va, size64_t initsize, size64_t size) {
  VaKtls (va, initsize, size);
}

/** @deprecated Use VaUtls instead **/
void va_utls (VIRTUAL_ADDRESS va, size64_t initsize, size64_t size) {
  VaUtls (va, initsize, size);
}

/** @deprecated Use VaVerify instead **/
void va_verify (VIRTUAL_ADDRESS va, size64_t size) {
  VaVerify (va, size);
}

/** @deprecated Use VaGetPhys instead **/
UINTN va_getphys (VIRTUAL_ADDRESS va) {
  return VaGetPhys (va);
}

/** @deprecated Use VaEntry instead **/
void va_entry (VIRTUAL_ADDRESS entry) {
  VaEntry (entry);
}

// Legacy global variable aliases
static arch_t elf_arch __attribute__((alias("gElfArch")));
static UINT8 boot_pagemap[PAGEMAP_SZ (BOOTMEM)]
  __attribute__((alias("gBootPagemap")));
static VIRTUAL_ADDRESS req_pfnmap_va __attribute__((alias("gReqPfnmapVa")));
static VIRTUAL_ADDRESS req_info_va __attribute__((alias("gReqInfoVa")));
static VIRTUAL_ADDRESS req_stree_va __attribute__((alias("gReqStreeVa")));
static VIRTUAL_ADDRESS req_region_va __attribute__((alias("gReqRegionVa")));
static VIRTUAL_ADDRESS ktls_va __attribute__((alias("gKtlsVa")));
static VIRTUAL_ADDRESS utls_va __attribute__((alias("gUtlsVa")));
static size64_t req_pfnmap_size __attribute__((alias("gReqPfnmapSize")));
static size64_t req_info_size __attribute__((alias("gReqInfoSize")));
static size64_t req_stree_size __attribute__((alias("gReqStreeSize")));
static size64_t req_region_size __attribute__((alias("gReqRegionSize")));
static size64_t ktls_initsize __attribute__((alias("gKtlsInitsize")));
static size64_t ktls_size __attribute__((alias("gKtlsSize")));
static size64_t utls_initsize __attribute__((alias("gUtlsInitsize")));
static size64_t utls_size __attribute__((alias("gUtlsSize")));
static unsigned req_stree_order __attribute__((alias("gReqStreeOrder")));
static unsigned req_region_num __attribute__((alias("gReqRegionNum")));
static bool stop_payload_allocation __attribute__((alias("gStopPayloadAllocation")));
static UINT64 minramaddr __attribute__((alias("gMinRamAddr")));
