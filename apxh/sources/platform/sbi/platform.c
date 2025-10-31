/** @file
  APXH SBI Machine-Dependent Functions

  Implements machine-dependent functionality for RISC-V SBI (Supervisor
  Binary Interface) platform. Provides device tree parsing, memory region
  management, payload handling, and kernel entry point setup.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
**/

#include <apxh/internal.h>
#include <string.h>
#include <libfdt.h>
#include <inttypes.h>
#include <apxh/apxh.h>

extern long boothid;
extern VOID *dtbptr;
static UINT64 gMinAddr = -1;
static UINT64 gMaxAddr = 0;
static UINT32 gRegions = 0;

#define SBI_MAX_RAM_REGIONS 64
BOOTINFO_REGION gRamRegions[SBI_MAX_RAM_REGIONS] = { 0, };

static VOID *gpElfKernelPayload, *gpElfUserPayload;
static UINTN gElfKernelPayloadSize, gElfUserPayloadSize;

static UINTN gBrk;

static APXH_PLATFORM_DESCRIPTOR gPlatformDesc;

/**
  Get address and size cells from DTB node.

  Retrieves #address-cells and #size-cells properties from parent
  node in device tree.

  @param[in]  Fdt     Pointer to flattened device tree.
  @param[in]  NodOff   Node offset.
  @param[out] AddrSz  Pointer to receive address cells count.
  @param[out] SizeSz  Pointer to receive size cells count.
**/
static VOID
DtbAddrSize (
  IN  CONST VOID  *Fdt,
  IN INT32 NodeOffset,
  OUT UINT32      *AddrSz,
  OUT UINT32      *SizeSz
  )
{
  INT32 ParentOffset, Length;
  VOID *p;

  ParentOffset = fdt_parent_offset (Fdt, NodeOffset);

  p = (VOID *) fdt_getprop (Fdt, ParentOffset, "#address-cells", &Length);
  if (!p || Length != 4)
    {
      *AddrSz = 2;
    }
  else
    {
      *AddrSz = fdt32_to_cpu (*(UINT32 *) p);
    }

  p = (VOID *) fdt_getprop (Fdt, ParentOffset, "#size-cells", &Length);
  if (!p || Length != 4)
    {
      *SizeSz = 2;
    }
  else
    {
      *SizeSz = fdt32_to_cpu (*(UINT32 *) p);
    }
}

/**
  Iterate RAM regions from device tree.

  Walks device tree nodes to find memory and reserved-memory regions,
  invoking callback for each region found.

  @param[in] Func  Callback function for each region.
  @param[in] Opq  Opaque pointer passed to callback.
**/
static VOID
RamRegionForeach (
  IN VOID  (*Func) (UINT64, UINT64, BOOLEAN, VOID *),
  IN VOID  *Opq
  )
{
  VOID *Fdt = dtbptr;
  INT32 NodeOffset;

  for (NodeOffset = fdt_next_node (Fdt, -1, NULL);
       NodeOffset >= 0; NodeOffset = fdt_next_node (Fdt, NodeOffset, NULL))
    {
      CONST CHAR *Name = fdt_get_name (Fdt, NodeOffset, NULL);
      if (!Name)
	continue;

      // I should probably get the root child only here.
      if (!strncmp (Name, "memory", 6))
	{
	  INT32 Length;
	  UINT32 AddrSz, SizeSz;

	  UINT32 *Reg = (UINT32 *) fdt_getprop (Fdt, NodeOffset, "reg", &Length);
	  if (!Reg)
	    continue;

	  DtbAddrSize (Fdt, NodeOffset, &AddrSz, &SizeSz);

	  for (INT32 i = 0; i < Length; i += 4 * (AddrSz + SizeSz))
	    {
	      UINT64 Base, Size;

	      Base = 0;
	      for (INT32 j = 0; j < AddrSz; j++)
		Base = (Base << 32) + fdt32_to_cpu (*Reg++);

	      Size = 0;
	      for (INT32 j = 0; j < SizeSz; j++)
		Size = (Size << 32) + fdt32_to_cpu (*Reg++);

	      Func (Base, Size, FALSE, Opq);
	    }
	}
      else if (!strncmp (Name, "reserved-memory", 15))
	{
	  INT32 Length, SubOff;
	  UINT32 AddrSz, SizeSz;

	  DtbAddrSize (Fdt, NodeOffset, &AddrSz, &SizeSz);

	  fdt_for_each_subnode (SubOff, Fdt, NodeOffset)
	  {
	    UINT32 *Reg =
	      (UINT32 *) fdt_getprop (Fdt, SubOff, "reg", &Length);
	    if (!Reg)
	      continue;

	    for (INT32 i = 0; i < Length; i += 4 * (AddrSz + SizeSz))
	      {
		UINT64 Base, Size;

		Base = 0;
		for (INT32 j = 0; j < AddrSz; j++)
		  Base = (Base << 32) + fdt32_to_cpu (*Reg++);

		Size = 0;
		for (INT32 j = 0; j < SizeSz; j++)
		  Size = (Size << 32) + fdt32_to_cpu (*Reg++);

		Func (Base, Size, TRUE, Opq);
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
  IN INT32 ArgumentCount,
  IN char    *ArgumentVector[],
  IN PAYLOAD_ID  PayloadId
  )
{
  VOID *ElfPayload;

  switch (PayloadId)
    {
    case PAYLOAD_KERNEL:
      ElfPayload = gpElfKernelPayload;
      break;
    case PAYLOAD_USER:
      ElfPayload = gpElfUserPayload;
      break;
    default:
      printf ("Unsupported payload ID %d\n", PayloadId);
      ElfPayload = NULL;
      break;
    }

  return ElfPayload;
}

/**
  Get payload size.

  Returns size of ELF payload in bytes.

  @param[in] Id  Payload identifier (kernel or user).

  @return Size of payload in bytes, or 0 if not found.
**/
UINTN
GetPayloadSize (
  IN PAYLOAD_ID  PayloadId
  )
{
  UINTN ElfPayloadSize;

  switch (PayloadId)
    {
    case PAYLOAD_KERNEL:
      ElfPayloadSize = gElfKernelPayloadSize;
      break;
    case PAYLOAD_USER:
      ElfPayloadSize = gElfUserPayloadSize;
      break;
    default:
      printf ("Unsupported payload ID %d\n", PayloadId);
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
UINTN
GetPage (
  VOID
  )
{
  UINTN m;

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
  @param[in] Opq  Opaque pointer (unused).
**/
static VOID
AddRamRegion (
  IN UINT64  Base,
  IN UINT64  Len,
  IN BOOLEAN    Busy,
  IN VOID    *Opq
  )
{
  BOOTINFO_REGION *Region;

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

  Region = gRamRegions + gRegions++;
  Region->Type = Busy ? BootInfoRegionBusy : BootInfoRegionRam;
  Region->Pfn = Base >> PAGE_SHIFT;
  Region->Len = Len >> PAGE_SHIFT;

  printf ("\t%016lx:%016lx (%s)\n", Base, Base + Len, Busy ? "SYS" : "RAM");
}

/**
  Initialize machine-dependent subsystem.

  Parses device tree to discover memory regions, locates ELF payloads,
  and initializes heap allocator.
**/
VOID
PlatformInitialize (
  VOID
  )
{
  UINTN Ptr;
  struct fdt_header *FdtH;

  printf ("Booting from HART %lx\n", boothid);
  printf ("DTB Pointer at %lx\n", dtbptr);

  FdtH = (struct fdt_header *) dtbptr;
  if (fdt_check_header (FdtH) != 0)
    {
      printf ("Invalid DTB header.\n");
      exit (-1);
    }

  printf ("Total size: %lx\n", fdt32_to_cpu (FdtH->totalsize));

  printf ("Device Tree Memory Regions:\n");
  RamRegionForeach (AddRamRegion, NULL);
  /* Add DTB as busy. */
  AddRamRegion ((UINT64) dtbptr,
		 PAGE_ROUND (fdt32_to_cpu (FdtH->totalsize)), TRUE, NULL);
  printf ("\n");

  gpElfKernelPayload = PayloadGet (0, &gElfKernelPayloadSize);
  Ptr = (UINTN) gpElfKernelPayload + gElfKernelPayloadSize;
  gBrk = PAGE_ROUND (Ptr);

  gpElfUserPayload = PayloadGet (1, &gElfUserPayloadSize);
  Ptr = (UINTN) gpElfUserPayload + gElfUserPayloadSize;
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
PlatformVerify (
  IN VIRTUAL_ADDRESS   VirtualAddress,
  IN SIZE64  Size
  )
{
  /* Nothing to verify. */
}

/**
  Get memory region count.

  Returns number of memory regions discovered from device tree.

  @return Number of memory regions.
**/
UINT32
PlatformGetMemoryRegionCount (
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
BOOTINFO_REGION *
PlatformGetMemoryRegion (
  IN UINT32 Index
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
PlatformGetMinRamPageFrameNumber (
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
PlatformGetMaxRamPageFrameNumber (
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
FRAMEBUFFER_DESC *
PlatformGetFramebuffer (
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
PlatformGetMaxPageFrameNumber (
  VOID
  )
{
  /*
     Scanning device memory usage in Device Tree is dependent from the
     board. Return the higest RAM address.
   */
  return PlatformGetMaxRamPageFrameNumber ();
}

/**
  Get platform descriptor.

  Returns platform descriptor containing device tree pointer.

  @return Pointer to platform descriptor.
**/
APXH_PLATFORM_DESCRIPTOR *
PlatformGetDescriptor (
  VOID
  )
{
  /* Only DTB supported. */
  gPlatformDesc.Type = ApxhPlatformDtb;
  gPlatformDesc.PlatformPointer = (UINT64) (UINTN) dtbptr;
  return &gPlatformDesc;
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
PlatformEntry (
  IN ARCH   Arch,
  IN VIRTUAL_ADDRESS  PageTable,
  IN VIRTUAL_ADDRESS  Entry
  )
{
  VOID *TrampRoot;
  UINTN TrampSatp, Satp;
  extern char trampoline_start asm ("__rv64_tstart");
  extern char trampoline_end asm ("__rv64_tend");

  assert (Arch == ARCH_RISCV64);

  printf ("Entry called.\n");

  /* Setup trampoline. */
  TrampRoot = (VOID *) GetPage ();
  /* Map trampoline page */
  Sv48DirectMap (TrampRoot, (UINTN) & trampoline_start,
		  (UINTN) & trampoline_start,
		  (UINTN) (&trampoline_end - &trampoline_start),
		  MEMTYPE_WB, 0, 1);
  /* Map start page */
  Sv48DirectMap (TrampRoot, Sv48GetPhysical (Entry), Entry, 4096, MEMTYPE_WB,
		  0, 1);

  TrampSatp = 0x9L << 60 | (UINTN) TrampRoot >> PAGE_SHIFT;
  Satp = 0x9L << 60 | PageTable >> PAGE_SHIFT;

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
