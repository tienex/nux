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
static size_t gElfKernelPayloadSize, gElfUserPayloadSize;
static unsigned long gMaxPfn;
static unsigned long gMaxRamPfn;
static unsigned long gMinRamPfn = -1;
static unsigned gNumRegions;
static VOID *gpEfiRsdp;
static struct fbdesc gFbDesc = {.type = FB_INVALID, };

static struct apxh_platformdesc gPlatformDesc;

static struct bootinfo_region gMemRegions[BOOTINFO_REGIONS_MAX];

#include <apxh/uefi/internal.h>

/**
  Exit bootloader.

  Exits the bootloader with the specified status code. Calls EFI
  exit and then loops indefinitely.

  @param[in] Status  Exit status code.
**/
VOID __dead
Exit (
  IN int  Status
  )
{
  printf ("EXIT CALLED!\n");

  efi_exit (Status);
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
  return (UINTN) efi_allocate_maxaddr ((gMinRamPfn << PAGE_SHIFT) +
					   (unsigned long) BOOTMEM);
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
  IN arch_t   Arch,
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
  pae64_directmap (pTrampCr3, 0, 0, 64L << 30, MEMTYPE_WB, 0, 1);

  /* Map Entry page in transitional pagetable VA. */
  pae64_map_page (pTrampCr3, (VIRTUAL_ADDRESS) Entry, pae64_getphys (Entry), 0, 0, 1);

  efi_exitbs ();

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
  IN arch_t   Arch,
  IN VIRTUAL_ADDRESS  Pt,
  IN VIRTUAL_ADDRESS  Entry
  )
{
  VOID *TrampRoot;
  unsigned long TrampSatp, Satp;
  extern char trampoline_start asm ("__rv64_tstart");
  extern char trampoline_end asm ("__rv64_tend");

  assert (Arch == ARCH_RISCV64);

  printf ("Entry called.\n");

  /* Setup trampoline. */
  TrampRoot = (VOID *) GetPage ();
  /* Map trampoline page */
  sv48_directmap (TrampRoot, (UINTN) & trampoline_start,
		  (UINTN) & trampoline_start,
		  (UINTN) (&trampoline_end - &trampoline_start),
		  MEMTYPE_WB, 0, 1);
  /* Map start page */
  sv48_directmap (TrampRoot, sv48_getphys (Entry), Entry, 4096, MEMTYPE_WB,
		  0, 1);

  TrampSatp = 0x9L << 60 | (UINTN) TrampRoot >> PAGE_SHIFT;
  Satp = 0x9L << 60 | Pt >> PAGE_SHIFT;

  printf ("%lx %lx %lx\n", TrampSatp, Entry, Satp);

  efi_exitbs ();

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
struct bootinfo_region *
MdGetMemRegion (
  IN unsigned  Index
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
unsigned
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
struct fbdesc *
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
struct apxh_platformdesc *
MdGetPlatformDesc (
  VOID
  )
{
  /* Only ACPI supported. */
  gPlatformDesc.type = PLATFORM_ACPI;
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
  IN int     Argc,
  IN char    *Argv[],
  IN plid_t  Id
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
unsigned long
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
  gFbDesc.type = FB_RGB;
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
  IN int            Ram,
  IN int            Bsy,
  IN unsigned long  Pfn,
  IN unsigned       Len
  )
{
  unsigned Cur = gNumRegions;

  if (Cur >= BOOTINFO_REGIONS_MAX)
    {
      printf ("Exceeded maximum number of memory regions. (%d >= %d\n",
	      Cur, BOOTINFO_REGIONS_MAX);
      return;
    }

  gMemRegions[Cur].pfn = Pfn;
  gMemRegions[Cur].len = Len;

  if (Pfn + Len > gMaxPfn)
    gMaxPfn = Pfn + Len;

  if (Ram && !Bsy)
    {
      gMemRegions[Cur].type = BOOTINFO_REGION_RAM;
      if (Pfn + Len > gMaxRamPfn)
	gMaxRamPfn = Pfn + Len;
      if (Pfn < gMinRamPfn)
	gMinRamPfn = Pfn;
    }
  else if (Ram && Bsy)
    gMemRegions[Cur].type = BOOTINFO_REGION_BSY;
  else
    gMemRegions[Cur].type = BOOTINFO_REGION_OTHER;


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
  IN size_t  Size
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
  IN size_t  Size
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

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use Exit instead **/
void __dead exit (int st) {
  Exit (st);
}

/** @deprecated Use GetPage instead **/
UINTN get_page (void) {
  return GetPage ();
}

/** @deprecated Use MdInitialize instead **/
void md_init (void) {
  MdInitialize ();
}

/** @deprecated Use MdVerify instead **/
void md_verify (VIRTUAL_ADDRESS va, UINT64 size) {
  MdVerify (va, size);
}

/** @deprecated Use MdEntry instead **/
void md_entry (arch_t arch, VIRTUAL_ADDRESS pt, VIRTUAL_ADDRESS entry) {
  MdEntry (arch, pt, entry);
}

/** @deprecated Use MdMaxPfn instead **/
UINT64 md_maxpfn (void) {
  return MdMaxPfn ();
}

/** @deprecated Use MdMinRamPfn instead **/
UINT64 md_minrampfn (void) {
  return MdMinRamPfn ();
}

/** @deprecated Use MdMaxRamPfn instead **/
UINT64 md_maxrampfn (void) {
  return MdMaxRamPfn ();
}

/** @deprecated Use MdGetMemRegion instead **/
struct bootinfo_region *md_getmemregion (unsigned i) {
  return MdGetMemRegion (i);
}

/** @deprecated Use MdMemRegions instead **/
unsigned md_memregions (void) {
  return MdMemRegions ();
}

/** @deprecated Use MdGetFramebuffer instead **/
struct fbdesc *md_getframebuffer (void) {
  return MdGetFramebuffer ();
}

/** @deprecated Use MdGetPlatformDesc instead **/
struct apxh_platformdesc *md_getplatformdesc (void) {
  return MdGetPlatformDesc ();
}

/** @deprecated Use GetPayloadStart instead **/
void *get_payload_start (int argc, char *argv[], plid_t id) {
  return GetPayloadStart (argc, argv, id);
}

/** @deprecated Use GetPayloadSize instead **/
unsigned long get_payload_size (plid_t id) {
  return GetPayloadSize (id);
}

/** @deprecated Use ApxhEfiAddFramebuffer instead **/
void apxhefi_add_framebuffer (UINT64 addr, UINT64 size,
			      UINT32 width, UINT32 height,
			      UINT32 pitch, UINT32 bpp,
			      UINT32 rm, UINT32 gm, UINT32 bm) {
  ApxhEfiAddFramebuffer (addr, size, width, height, pitch, bpp, rm, gm, bm);
}

/** @deprecated Use ApxhEfiAddMemRegion instead **/
void apxhefi_add_memregion (int ram, int bsy, unsigned long pfn, unsigned len) {
  ApxhEfiAddMemRegion (ram, bsy, pfn, len);
}

/** @deprecated Use ApxhEfiAddKernelPayload instead **/
void apxhefi_add_kernel_payload (void *start, size_t size) {
  ApxhEfiAddKernelPayload (start, size);
}

/** @deprecated Use ApxhEfiAddUserPayload instead **/
void apxhefi_add_user_payload (void *start, size_t size) {
  ApxhEfiAddUserPayload (start, size);
}

/** @deprecated Use ApxhEfiAddRsdp instead **/
void apxhefi_add_rsdp (void *rsdp) {
  ApxhEfiAddRsdp (rsdp);
}
