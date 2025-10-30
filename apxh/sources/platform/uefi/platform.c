/** @file
  APXH EFI Machine-Dependent Support

  Provides platform-specific functions for UEFI firmware boot environment.
  Handles memory allocation, payload loading, framebuffer setup, and
  architecture-specific kernel entry trampolines for AMD64 and RISC-V64.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
**/

#include <apxh/internal.h>

#define BOOTINFO_REGIONS_MAX 1024

static VOID *gpElfKernelPayload, *gpElfUserPayload;
static UINTN gElfKernelPayloadSize, gElfUserPayloadSize;
static UINTN gMaxPfn;
static UINTN gMaxRamPfn;
static UINTN gMinRamPfn = -1;
static UINT32 gNumRegions;
static VOID *gpEfiRsdp;
static FRAMEBUFFER_DESC gFbDesc = {.Type = FB_INVALID, };

static APXH_PLATFORM_DESCRIPTOR gPlatformDesc;

static BOOTINFO_REGION gMemRegions[BOOTINFO_REGIONS_MAX];

#include <apxh/uefi/internal.h>

/**
  Exit bootloader.

  Exits the bootloader with the specified status code. Calls EFI
  exit and then loops indefinitely.

  @param[in] Status  Exit status code.
**/
VOID __dead
Exit (
  IN INT32 Status
  )
{
  printf ("EXIT CALLED!\n");

  EfiExit (Status);
  while (1);
}

/**
  Allocate physical page.

  Allocates a physical page below boot memory limit for bootloader use.

  @return Physical address of allocated page.
**/
UINTN
GetPage (
  VOID
  )
{
  return (UINTN) EfiAllocateMaxAddr ((gMinRamPfn << PAGE_SHIFT) +
					   (UINTN) BOOTMEM);
}

/**
  Initialize machine-dependent platform.

  Performs platform-specific initialization for EFI boot. Currently empty
  as EFI initialization is handled by efi-main.c.
**/
VOID
MdInitialize (
  VOID
  )
{
}

/**
  Verify virtual address range.

  Verifies that a virtual address range is valid. Currently a no-op for
  EFI platform as validation is handled by firmware.

  @param[in] Va    Virtual address to verify.
  @param[in] Size  Size of range to verify.
**/
VOID
MdVerify (
  IN VIRTUAL_ADDRESS  Va,
  IN UINT64   Size
  )
{
  /* Nothing to verify. */
}

#if EC_MACHINE_AMD64
/**
  Enter kernel (AMD64).

  Transfers control to kernel with AMD64 long mode enabled. Sets up
  trampoline page tables, exits EFI boot services, and switches to
  long mode before jumping to kernel entry point.

  @param[in] Arch   Target architecture (must be ARCH_AMD64).
  @param[in] Pt     Root page table physical address.
  @param[in] Entry  Kernel entry point virtual address.
**/
VOID
MdEntry (
  IN ARCH   Arch,
  IN VIRTUAL_ADDRESS  Pt,
  IN VIRTUAL_ADDRESS  Entry
  )
{
  VOID *pTrampCr3;
  VOID *Tramp;
  VIRTUAL_ADDRESS TrampEntry;
  UINT64 TrampCode = 0xe7ffd9220fL;	/* mov %rcx, %cr3; jmp *%rdi */

  assert (Arch == ARCH_AMD64);

  printf ("Entry called.\n");

  /* Allocate trampoline pagetable. */
  pTrampCr3 = (VOID *) GetPage ();

  /* Setup trampoline. */
  Tramp = (VOID *) GetPage ();
  *(UINT64 *) Tramp = TrampCode;
  TrampEntry = (VIRTUAL_ADDRESS) (UINTN) Tramp;

  /* Setup Direct map at 0->1Gb */
  Pae64DirectMap (pTrampCr3, 0, 0, 64L << 30, MEMTYPE_WB, 0, 1);

  /* Map Entry page in transitional pagetable VA. */
  Pae64MapPage (pTrampCr3, (VIRTUAL_ADDRESS) Entry, Pae64GetPhys (Entry), 0, 0, 1);

  EfiExitBs ();

  /* Assume physical mapping mode, 1:1 on lower addresses. */
  asm volatile
    ("mov %0, %%rax\n"
     "mov %1, %%rdi\n"
     "mov %2, %%rsi\n"
     "mov %3, %%rcx\n"
     "cli\n"
     "jmp *%%rax\n"::"m" (TrampEntry), "m" (Entry), "m" (Pt),
     "m" (pTrampCr3));
}
#elif EC_MACHINE_RISCV64
/**
  Enter kernel (RISC-V64).

  Transfers control to kernel with RISC-V SV48 paging enabled. Sets up
  trampoline page tables, exits EFI boot services, and switches SATP
  register before jumping to kernel entry point.

  @param[in] Arch   Target architecture (must be ARCH_RISCV64).
  @param[in] Pt     Root page table physical address.
  @param[in] Entry  Kernel entry point virtual address.
**/
VOID
MdEntry (
  IN ARCH   Arch,
  IN VIRTUAL_ADDRESS  Pt,
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
  Sv48DirectMap (TrampRoot, Sv48GetPhys (Entry), Entry, 4096, MEMTYPE_WB,
		  0, 1);

  TrampSatp = 0x9L << 60 | (UINTN) TrampRoot >> PAGE_SHIFT;
  Satp = 0x9L << 60 | Pt >> PAGE_SHIFT;

  printf ("%lx %lx %lx\n", TrampSatp, Entry, Satp);

  EfiExitBs ();

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
#endif

/**
  Get maximum page frame number.

  Returns the highest page frame number in the system memory map.

  @return Maximum PFN across all memory regions.
**/
UINT64
MdMaxPfn (
  VOID
  )
{
  return gMaxPfn;
}

/**
  Get minimum RAM page frame number.

  Returns the lowest page frame number in usable RAM.

  @return Minimum RAM PFN.
**/
UINT64
MdMinRamPfn (
  VOID
  )
{
  return gMinRamPfn;
}

/**
  Get maximum RAM page frame number.

  Returns the highest page frame number in usable RAM.

  @return Maximum RAM PFN.
**/
UINT64
MdMaxRamPfn (
  VOID
  )
{
  return gMaxRamPfn;
}

/**
  Get memory region by index.

  Returns pointer to memory region descriptor at specified index.

  @param[in] Index  Region index (must be < BOOTINFO_REGIONS_MAX).

  @return Pointer to bootinfo_region structure.
**/
BOOTINFO_REGION *
MdGetMemRegion (
  IN UINT32 Index
  )
{
  assert (Index < BOOTINFO_REGIONS_MAX);

  return gMemRegions + Index;
}

/**
  Get number of memory regions.

  Returns the count of memory regions discovered in EFI memory map.

  @return Number of memory regions.
**/
UINT32
MdMemRegions (
  VOID
  )
{

  return gNumRegions;
}

/**
  Get framebuffer descriptor.

  Returns pointer to framebuffer descriptor populated from EFI GOP.

  @return Pointer to fbdesc structure.
**/
FRAMEBUFFER_DESC *
MdGetFramebuffer (
  VOID
  )
{
  return &gFbDesc;
}

/**
  Get platform descriptor.

  Returns platform descriptor with ACPI RSDP pointer from EFI system table.

  @return Pointer to apxh_platformdesc structure.
**/
APXH_PLATFORM_DESCRIPTOR *
MdGetPlatformDesc (
  VOID
  )
{
  /* Only ACPI supported. */
  gPlatformDesc.Type = PLATFORM_ACPI;
  gPlatformDesc.PlatformPointer = (UINT64) (UINTN) gpEfiRsdp;
  return &gPlatformDesc;
}

/**
  Get payload start address.

  Returns pointer to ELF payload loaded from EFI file system.

  @param[in] Argc  Unused (for compatibility).
  @param[in] Argv  Unused (for compatibility).
  @param[in] Id    Payload ID (PAYLOAD_KERNEL or PAYLOAD_USER).

  @return Pointer to ELF payload, or NULL if not loaded.
**/
VOID *
GetPayloadStart (
  IN INT32 Argc,
  IN char    *Argv[],
  IN PAYLOAD_ID  Id
  )
{
  VOID *ElfPayload;

  switch (Id)
    {
    case PAYLOAD_KERNEL:
      ElfPayload = gpElfKernelPayload;
      break;
    case PAYLOAD_USER:
      ElfPayload = gpElfUserPayload;
      break;
    default:
      printf ("Unsupported payload ID %d\n", Id);
      ElfPayload = NULL;
      break;
    }

  return ElfPayload;
}

/**
  Get payload size.

  Returns size of ELF payload loaded from EFI file system.

  @param[in] Id  Payload ID (PAYLOAD_KERNEL or PAYLOAD_USER).

  @return Payload size in bytes, or 0 if not loaded.
**/
UINTN
GetPayloadSize (
  IN PAYLOAD_ID  Id
  )
{
  UINTN ElfPayloadSize;

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
  Add framebuffer descriptor.

  Stores framebuffer information from EFI Graphics Output Protocol.

  @param[in] Addr    Physical address of framebuffer.
  @param[in] Size    Size of framebuffer in bytes.
  @param[in] Width   Width in pixels.
  @param[in] Height  Height in pixels.
  @param[in] Pitch   Bytes per scanline.
  @param[in] Bpp     Bits per pixel.
  @param[in] Rm      Red color mask.
  @param[in] Gm      Green color mask.
  @param[in] Bm      Blue color mask.
**/
VOID
ApxhEfiAddFramebuffer (
  IN UINT64   Addr,
  IN UINT64   Size,
  IN UINT32   Width,
  IN UINT32   Height,
  IN UINT32   Pitch,
  IN UINT32   Bpp,
  IN UINT32   Rm,
  IN UINT32   Gm,
  IN UINT32   Bm
  )
{
  gFbDesc.Type = FB_RGB;
  gFbDesc.addr = Addr;
  gFbDesc.size = Size;

  gFbDesc.pitch = Pitch;
  gFbDesc.width = Width;
  gFbDesc.height = Height;
  gFbDesc.bpp = Bpp;

  gFbDesc.r_mask = Rm;
  gFbDesc.g_mask = Gm;
  gFbDesc.b_mask = Bm;
}


/**
  Add memory region.

  Adds a memory region from EFI memory map to boot info structures.
  Updates maxpfn, maxrampfn, and minrampfn as needed.

  @param[in] Ram  TRUE if region is RAM, FALSE otherwise.
  @param[in] Bsy  TRUE if region is busy (reserved), FALSE if available.
  @param[in] Pfn  Starting page frame number.
  @param[in] Len  Length in pages.
**/
VOID
ApxhEfiAddMemRegion (
  IN INT32 Ram,
  IN INT32 Bsy,
  IN UINTN  Pfn,
  IN UINT32 Len
  )
{
  UINT32 Cur = gNumRegions;

  if (Cur >= BOOTINFO_REGIONS_MAX)
    {
      printf ("Exceeded maximum number of memory regions. (%d >= %d\n",
	      Cur, BOOTINFO_REGIONS_MAX);
      return;
    }

  gMemRegions[Cur].PageFrameNumber = Pfn;
  gMemRegions[Cur].Length = Len;

  if (Pfn + Len > gMaxPfn)
    gMaxPfn = Pfn + Len;

  if (Ram && !Bsy)
    {
      gMemRegions[Cur].Type = BOOTINFO_REGION_RAM;
      if (Pfn + Len > gMaxRamPfn)
	gMaxRamPfn = Pfn + Len;
      if (Pfn < gMinRamPfn)
	gMinRamPfn = Pfn;
    }
  else if (Ram && Bsy)
    gMemRegions[Cur].Type = BOOTINFO_REGION_BSY;
  else
    gMemRegions[Cur].Type = BOOTINFO_REGION_OTHER;


  gNumRegions++;
}

/**
  Add kernel payload.

  Stores pointer and size of kernel ELF payload loaded from EFI filesystem.

  @param[in] Start  Pointer to kernel ELF image.
  @param[in] Size    Size of kernel ELF image in bytes.
**/
VOID
ApxhEfiAddKernelPayload (
  IN VOID    *Start,
  IN UINTN  Size
  )
{
  gpElfKernelPayload = Start;
  gElfKernelPayloadSize = Size;
}

/**
  Add user payload.

  Stores pointer and size of optional user ELF payload loaded from EFI
  filesystem.

  @param[in] Start  Pointer to user ELF image.
  @param[in] Size    Size of user ELF image in bytes.
**/
VOID
ApxhEfiAddUserPayload (
  IN VOID    *Start,
  IN UINTN  Size
  )
{
  gpElfUserPayload = Start;
  gElfUserPayloadSize = Size;
}

/**
  Add ACPI RSDP pointer.

  Stores ACPI Root System Description Pointer from EFI system table.

  @param[in] Rsdp  Pointer to ACPI RSDP structure.
**/
VOID
ApxhEfiAddRsdp (
  IN VOID  *Rsdp
  )
{
  gpEfiRsdp = Rsdp;
}
