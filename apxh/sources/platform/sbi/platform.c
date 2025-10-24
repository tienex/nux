/** @file
  APXH SBI Machine-Dependent Functions

  Implements machine-dependent functionality for RISC-V SBI (Supervisor
  Binary Interface) platform. Provides device tree parsing, memory region
  management, payload handling, and kernel entry point setup.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
**/

#include "project.h"
#include <string.h>
#include <libfdt.h>
#include <inttypes.h>
#include <apxh.h>

extern long boothid;
extern void *dtbptr;
static UINT64 gMinAddr = -1;
static UINT64 gMaxAddr = 0;
static unsigned gRegions = 0;

#define SBI_MAX_RAM_REGIONS 64
struct bootinfo_region gRamRegions[SBI_MAX_RAM_REGIONS] = { 0, };

static VOID *gpElfKernelPayload, *gpElfUserPayload;
static size_t gElfKernelPayloadSize, gElfUserPayloadSize;

static uintptr_t gBrk;

static struct apxh_pltdesc gPltDesc;

/**
  Get address and size cells from DTB node.

  Retrieves #address-cells and #size-cells properties from parent
  node in device tree.

  @param[in]  pFdt     Pointer to flattened device tree.
  @param[in]  NodOff   Node offset.
  @param[out] pAddrSz  Pointer to receive address cells count.
  @param[out] pSizeSz  Pointer to receive size cells count.
**/
static VOID
DtbAddrSize (
  IN  CONST VOID  *pFdt,
  IN  int         NodOff,
  OUT UINT32      *pAddrSz,
  OUT UINT32      *pSizeSz
  )
{
  int PaOff, Len;
  VOID *p;

  PaOff = fdt_parent_offset (pFdt, NodOff);

  p = (VOID *) fdt_getprop (pFdt, PaOff, "#address-cells", &Len);
  if (!p || Len != 4)
    {
      *pAddrSz = 2;
    }
  else
    {
      *pAddrSz = fdt32_to_cpu (*(UINT32 *) p);
    }

  p = (VOID *) fdt_getprop (pFdt, PaOff, "#size-cells", &Len);
  if (!p || Len != 4)
    {
      *pSizeSz = 2;
    }
  else
    {
      *pSizeSz = fdt32_to_cpu (*(UINT32 *) p);
    }
}

/**
  Iterate RAM regions from device tree.

  Walks device tree nodes to find memory and reserved-memory regions,
  invoking callback for each region found.

  @param[in] Func  Callback function for each region.
  @param[in] pOpq  Opaque pointer passed to callback.
**/
static VOID
RamRegionForeach (
  IN VOID  (*Func) (UINT64, UINT64, bool, VOID *),
  IN VOID  *pOpq
  )
{
  VOID *pFdt = dtbptr;
  int NodOff;

  for (NodOff = fdt_next_node (pFdt, -1, NULL);
       NodOff >= 0; NodOff = fdt_next_node (pFdt, NodOff, NULL))
    {
      CONST CHAR *pName = fdt_get_name (pFdt, NodOff, NULL);
      if (!pName)
	continue;

      // I should probably get the root child only here.
      if (!strncmp (pName, "memory", 6))
	{
	  int Len;
	  UINT32 AddrSz, SizeSz;

	  UINT32 *pReg = (UINT32 *) fdt_getprop (pFdt, NodOff, "reg", &Len);
	  if (!pReg)
	    continue;

	  DtbAddrSize (pFdt, NodOff, &AddrSz, &SizeSz);

	  for (int i = 0; i < Len; i += 4 * (AddrSz + SizeSz))
	    {
	      UINT64 Base, Size;

	      Base = 0;
	      for (int j = 0; j < AddrSz; j++)
		Base = (Base << 32) + fdt32_to_cpu (*pReg++);

	      Size = 0;
	      for (int j = 0; j < SizeSz; j++)
		Size = (Size << 32) + fdt32_to_cpu (*pReg++);

	      Func (Base, Size, false, pOpq);
	    }
	}
      else if (!strncmp (pName, "reserved-memory", 15))
	{
	  int Len, SubOff;
	  UINT32 AddrSz, SizeSz;

	  DtbAddrSize (pFdt, NodOff, &AddrSz, &SizeSz);

	  fdt_for_each_subnode (SubOff, pFdt, NodOff)
	  {
	    UINT32 *pReg =
	      (UINT32 *) fdt_getprop (pFdt, SubOff, "reg", &Len);
	    if (!pReg)
	      continue;

	    for (int i = 0; i < Len; i += 4 * (AddrSz + SizeSz))
	      {
		UINT64 Base, Size;

		Base = 0;
		for (int j = 0; j < AddrSz; j++)
		  Base = (Base << 32) + fdt32_to_cpu (*pReg++);

		Size = 0;
		for (int j = 0; j < SizeSz; j++)
		  Size = (Size << 32) + fdt32_to_cpu (*pReg++);

		Func (Base, Size, true, pOpq);
	      }
	  }
	}
    }
}

/**
  Get payload start address.

  Returns pointer to ELF payload in memory.

  @param[in] Argc  Argument count (unused on SBI).
  @param[in] Argv  Argument vector (unused on SBI).
  @param[in] Id    Payload identifier (kernel or user).

  @return Pointer to ELF payload, or NULL if not found.
**/
VOID *
GetPayloadStart (
  IN int     Argc,
  IN char    *Argv[],
  IN plid_t  Id
  )
{
  VOID *pElfPayload;

  switch (Id)
    {
    case PAYLOAD_KERNEL:
      pElfPayload = gpElfKernelPayload;
      break;
    case PAYLOAD_USER:
      pElfPayload = gpElfUserPayload;
      break;
    default:
      printf ("Unsupported payload ID %d\n", Id);
      pElfPayload = NULL;
      break;
    }

  return pElfPayload;
}

/**
  Get payload size.

  Returns size of ELF payload in bytes.

  @param[in] Id  Payload identifier (kernel or user).

  @return Size of payload in bytes, or 0 if not found.
**/
size_t
GetPayloadSize (
  IN plid_t  Id
  )
{
  size_t ElfPayloadSize;

  switch (Id)
    {
    case PAYLOAD_KERNEL:
      ElfPayloadSize = gElfKernelPayloadSize;
      break;
    case PAYLOAD_USER:
      ElfPayloadSize = gElfUserPayloadSize;
      break;
    default:
      printf ("Unsupported payload ID %d\n", Id);
      ElfPayloadSize = 0;
      break;
    }

  return ElfPayloadSize;
}

/**
  Allocate physical page.

  Allocates next available physical page from heap.

  @return Physical address of allocated page.
**/
uintptr_t
GetPage (
  VOID
  )
{
  uintptr_t m;

  m = PAGE_ROUND (gBrk);

  gBrk = m + PAGE_SIZE;

  memset ((VOID *) m, 0, PAGE_SIZE);

  return m;
}

/**
  Add RAM region to table.

  Callback function to add memory or reserved region to RAM regions
  table. Tracks minimum and maximum addresses.

  @param[in] Base  Base physical address.
  @param[in] Len   Length in bytes.
  @param[in] Busy  TRUE if region is reserved/busy.
  @param[in] pOpq  Opaque pointer (unused).
**/
static VOID
AddRamRegion (
  IN UINT64  Base,
  IN UINT64  Len,
  IN bool    Busy,
  IN VOID    *pOpq
  )
{
  struct bootinfo_region *pR;

  if (gRegions >= SBI_MAX_RAM_REGIONS)
    {
      printf ("Too many regions. Ignoring %016lx:%016lx (%s)\n", Base,
	      Base + Len, Busy ? "SYS" : "RAM");
      return;
    }

  if (Base <= gMinAddr)
    gMinAddr = Base;

  if (Base + Len >= gMaxAddr)
    gMaxAddr = Base + Len;

  pR = gRamRegions + gRegions++;
  pR->type = Busy ? BOOTINFO_REGION_BSY : BOOTINFO_REGION_RAM;
  pR->pfn = Base >> PAGE_SHIFT;
  pR->len = Len >> PAGE_SHIFT;

  printf ("\t%016lx:%016lx (%s)\n", Base, Base + Len, Busy ? "SYS" : "RAM");
}

/**
  Initialize machine-dependent subsystem.

  Parses device tree to discover memory regions, locates ELF payloads,
  and initializes heap allocator.
**/
VOID
MdInitialize (
  VOID
  )
{
  uintptr_t Ptr;
  struct fdt_header *pFdtH;

  printf ("Booting from HART %lx\n", boothid);
  printf ("DTB Pointer at %lx\n", dtbptr);

  pFdtH = (struct fdt_header *) dtbptr;
  if (fdt_check_header (pFdtH) != 0)
    {
      printf ("Invalid DTB header.\n");
      exit (-1);
    }

  printf ("Total size: %lx\n", fdt32_to_cpu (pFdtH->totalsize));

  printf ("Device Tree Memory Regions:\n");
  RamRegionForeach (AddRamRegion, NULL);
  /* Add DTB as busy. */
  AddRamRegion ((UINT64) dtbptr,
		 PAGE_ROUND (fdt32_to_cpu (pFdtH->totalsize)), true, NULL);
  printf ("\n");

  gpElfKernelPayload = payload_get (0, &gElfKernelPayloadSize);
  Ptr = (uintptr_t) gpElfKernelPayload + gElfKernelPayloadSize;
  gBrk = PAGE_ROUND (Ptr);

  gpElfUserPayload = payload_get (1, &gElfUserPayloadSize);
  Ptr = (uintptr_t) gpElfUserPayload + gElfUserPayloadSize;
  gBrk = PAGE_ROUND (Ptr);

  printf ("OpenSBI (device tree) boot initialised: brk at %08x\n", gBrk);
}

/**
  Verify virtual address range.

  Validates virtual address range for SBI platform. Currently
  performs no checks.

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
MdVerify (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  /* Nothing to verify. */
}

/**
  Get memory region count.

  Returns number of memory regions discovered from device tree.

  @return Number of memory regions.
**/
unsigned
MdMemRegions (
  VOID
  )
{
  return gRegions;
}

/**
  Get memory region descriptor.

  Returns pointer to memory region descriptor.

  @param[in] Index  Region index.

  @return Pointer to region descriptor, or NULL if index out of range.
**/
struct bootinfo_region *
MdGetMemRegion (
  IN unsigned  Index
  )
{
  if (Index >= SBI_MAX_RAM_REGIONS)
    return NULL;

  return gRamRegions + Index;
}

/**
  Get minimum RAM page frame number.

  Returns lowest RAM address as page frame number.

  @return Minimum RAM PFN.
**/
UINT64
MdMinRamPfn (
  VOID
  )
{
  return gMinAddr >> PAGE_SHIFT;
}

/**
  Get maximum RAM page frame number.

  Returns highest RAM address as page frame number.

  @return Maximum RAM PFN.
**/
UINT64
MdMaxRamPfn (
  VOID
  )
{

  return gMaxAddr >> PAGE_SHIFT;
}

/**
  Get framebuffer descriptor.

  Returns framebuffer descriptor. SBI platform has no framebuffer.

  @return NULL (no framebuffer on SBI).
**/
struct fbdesc *
MdGetFramebuffer (
  VOID
  )
{
  return NULL;
}

/**
  Get maximum page frame number.

  Returns maximum physical page frame number. For SBI, this is
  the same as maximum RAM PFN.

  @return Maximum PFN.
**/
UINT64
MdMaxPfn (
  VOID
  )
{
  /*
     Scanning device memory usage in Device Tree is dependent from the
     board. Return the higest RAM address.
   */
  return MdMaxRamPfn ();
}

/**
  Get platform descriptor.

  Returns platform descriptor containing device tree pointer.

  @return Pointer to platform descriptor.
**/
struct apxh_pltdesc *
MdGetPltDesc (
  VOID
  )
{
  /* Only DTB supported. */
  gPltDesc.type = PLT_DTB;
  gPltDesc.pltptr = (UINT64) (uintptr_t) dtbptr;
  return &gPltDesc;
}

/**
  Transfer control to kernel.

  Sets up trampoline page table and transfers control to kernel
  entry point using RISC-V SV48 paging.

  @param[in] Arch   Architecture (must be ARCH_RISCV64).
  @param[in] Pt     Page table root physical address.
  @param[in] Entry  Kernel entry point virtual address.
**/
VOID
MdEntry (
  IN arch_t   Arch,
  IN vaddr_t  Pt,
  IN vaddr_t  Entry
  )
{
  VOID *pTrampRoot;
  unsigned long TrampSatp, Satp;
  extern char trampoline_start asm ("__rv64_tstart");
  extern char trampoline_end asm ("__rv64_tend");

  assert (Arch == ARCH_RISCV64);

  printf ("Entry called.\n");

  /* Setup trampoline. */
  pTrampRoot = (VOID *) GetPage ();
  /* Map trampoline page */
  sv48_directmap (pTrampRoot, (uintptr_t) & trampoline_start,
		  (uintptr_t) & trampoline_start,
		  (uintptr_t) (&trampoline_end - &trampoline_start),
		  MEMTYPE_WB, 0, 1);
  /* Map start page */
  sv48_directmap (pTrampRoot, sv48_getphys (Entry), Entry, 4096, MEMTYPE_WB,
		  0, 1);

  TrampSatp = 0x9L << 60 | (uintptr_t) pTrampRoot >> PAGE_SHIFT;
  Satp = 0x9L << 60 | Pt >> PAGE_SHIFT;

  asm volatile
    (".globl __rv64_tstart, __rv64_tend\n"
     "mv t0, %0\n"
     "mv t1, %1\n"
     "mv a0, %2\n"
     "csrci sstatus, 0x2\n"
     "__rv64_tstart:\n"
     "csrw satp, t0\n"
     "sfence.vma x0, x0\n"
     "jalr x0, t1, 0\n"
     "__rv64_tend:\n"::"r" (TrampSatp), "r" (Entry), "r" (Satp):"t0", "t1",
     "a0");
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use DtbAddrSize instead **/
static void dtb_addrsize (const void *fdt, int noff, uint32_t *addrsz, uint32_t *sizesz) {
  DtbAddrSize (fdt, noff, addrsz, sizesz);
}

/** @deprecated Use RamRegionForeach instead **/
static void ramregion_foreach (void (*_f) (uint64_t, uint64_t, bool, void *), void *opq) {
  RamRegionForeach (_f, opq);
}

/** @deprecated Use GetPayloadStart instead **/
void *get_payload_start (int argc, char *argv[], plid_t id) {
  return GetPayloadStart (argc, argv, id);
}

/** @deprecated Use GetPayloadSize instead **/
size_t get_payload_size (plid_t id) {
  return GetPayloadSize (id);
}

/** @deprecated Use GetPage instead **/
uintptr_t get_page (void) {
  return GetPage ();
}

/** @deprecated Use AddRamRegion instead **/
static void add_ramregion (uint64_t base, uint64_t len, bool busy, void *opq) {
  AddRamRegion (base, len, busy, opq);
}

/** @deprecated Use MdInitialize instead **/
void md_init (void) {
  MdInitialize ();
}

/** @deprecated Use MdVerify instead **/
void md_verify (vaddr_t va, size64_t size) {
  MdVerify (va, size);
}

/** @deprecated Use MdMemRegions instead **/
unsigned md_memregions (void) {
  return MdMemRegions ();
}

/** @deprecated Use MdGetMemRegion instead **/
struct bootinfo_region *md_getmemregion (unsigned i) {
  return MdGetMemRegion (i);
}

/** @deprecated Use MdMinRamPfn instead **/
uint64_t md_minrampfn (void) {
  return MdMinRamPfn ();
}

/** @deprecated Use MdMaxRamPfn instead **/
uint64_t md_maxrampfn (void) {
  return MdMaxRamPfn ();
}

/** @deprecated Use MdGetFramebuffer instead **/
struct fbdesc *md_getframebuffer (void) {
  return MdGetFramebuffer ();
}

/** @deprecated Use MdMaxPfn instead **/
uint64_t md_maxpfn (void) {
  return MdMaxPfn ();
}

/** @deprecated Use MdGetPltDesc instead **/
struct apxh_pltdesc *md_getpltdesc (void) {
  return MdGetPltDesc ();
}

/** @deprecated Use MdEntry instead **/
void md_entry (arch_t arch, vaddr_t pt, vaddr_t entry) {
  MdEntry (arch, pt, entry);
}

// Legacy global variable aliases
static uint64_t minaddr __attribute__((alias("gMinAddr")));
static uint64_t maxaddr __attribute__((alias("gMaxAddr")));
static unsigned regions __attribute__((alias("gRegions")));
static struct bootinfo_region ram_regions[SBI_MAX_RAM_REGIONS] __attribute__((alias("gRamRegions")));
static void *elf_kernel_payload __attribute__((alias("gpElfKernelPayload")));
static void *elf_user_payload __attribute__((alias("gpElfUserPayload")));
static size_t elf_kernel_payload_size __attribute__((alias("gElfKernelPayloadSize")));
static size_t elf_user_payload_size __attribute__((alias("gElfUserPayloadSize")));
static uintptr_t brk __attribute__((alias("gBrk")));
static struct apxh_pltdesc pltdesc __attribute__((alias("gPltDesc")));
