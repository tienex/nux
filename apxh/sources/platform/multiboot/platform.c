/** @file
  APXH Multiboot Platform Support

  Implements machine-dependent functionality for Multiboot platform.
  Provides Multiboot info parsing, memory map handling, framebuffer
  setup, and architecture-specific kernel entry points.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
**/

#include <apxh/internal.h>

#include <apxh/x86.h>
#include <apxh/multiboot.h>

/*
  Physical Layout of boot memory structure.
*/
#define BOOTMEM_MMAP      0x10000	/* Multiboot memory map (256kb Max) */
#define BOOTMEM_MMAPSIZE  0x30000	/* Memory map maximum size */

static VOID *gpElfKernelPayload, *gpElfUserPayload;
static size_t gElfKernelPayloadSize, gElfUserPayloadSize;

static uintptr_t gBrk;
static unsigned gBootinfoRegions;
static UINT64 gBootinfoMaxPfn;
static UINT64 gBootinfoMaxRamPfn;

static struct fbdesc gFbDesc = {.type = FB_INVALID };

static struct apxh_pltdesc gPltDesc;

UINT64 RsdpFind (VOID);

/**
  Parse Multiboot framebuffer information.

  Extracts framebuffer descriptor from Multiboot info structure,
  supporting RGB and indexed modes.

  @param[in] pInfo  Pointer to Multiboot info structure.
**/
static VOID
ParseMultibootFramebuffer (
  IN struct multiboot_info  *pInfo
  )
{
  if (pInfo->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB)
    {
      gFbDesc.type = FB_RGB;
      gFbDesc.addr = pInfo->framebuffer_addr;
      gFbDesc.size =
	(UINT64) pInfo->framebuffer_pitch * pInfo->framebuffer_height;

      gFbDesc.pitch = pInfo->framebuffer_pitch;
      gFbDesc.width = pInfo->framebuffer_width;
      gFbDesc.height = pInfo->framebuffer_height;
      gFbDesc.bpp = pInfo->framebuffer_bpp;

#define MB2MASK(_p, _s)  (((1 << (_s)) - 1) << (1 << (_p)))
      gFbDesc.r_mask = MB2MASK (pInfo->rpos, pInfo->rsize);
      gFbDesc.g_mask = MB2MASK (pInfo->gpos, pInfo->gsize);
      gFbDesc.b_mask = MB2MASK (pInfo->bpos, pInfo->bsize);
#undef MB2MASK
    }
  else if (pInfo->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED)
    {
      /*
         Indexed Frame Buffer not supported!

         Unfortunately not much can be done here as GRUB will have switched
         to frame buffer and we have no way (yet!) to handle it.
       */
      gFbDesc.type = FB_RGB;
      gFbDesc.addr = pInfo->framebuffer_addr;
      gFbDesc.size =
	(UINT64) pInfo->framebuffer_pitch * pInfo->framebuffer_height;

      gFbDesc.pitch = pInfo->framebuffer_pitch;
      gFbDesc.width = pInfo->framebuffer_width;
      gFbDesc.height = pInfo->framebuffer_height;
      gFbDesc.bpp = pInfo->framebuffer_bpp;

      gFbDesc.r_mask = 0xff;
      gFbDesc.g_mask = 0xff;
      gFbDesc.b_mask = 0xff;
    }
  else
    {
      gFbDesc.type = FB_INVALID;
      return;
    }
}

/**
  Parse Multiboot memory map.

  Converts Multiboot memory map to APXH bootinfo_region format
  in-place. Tracks maximum PFN and maximum RAM PFN.

  @param[in] pInfo  Pointer to Multiboot info structure.
**/
static VOID
ParseMultibootMmap (
  IN struct multiboot_info  *pInfo
  )
{
  size_t MmapLength;

  MmapLength = pInfo->mmap_length;

  /*
     Step 1: check that we'll fit in the area allocated for the memory
     map.
   */
  assert (MmapLength < BOOTMEM_MMAPSIZE);

  /*
     Step 2: Move multiboot structures to preallocated space.

     We depend here on the fact that we are under the temporary
     boot-time page table, and physical address is mapped 1:1 both at
     the kernel offset and the beginning of virtual memory: we can
     dereference multiboot pointers safely.
   */
  memmove ((VOID *) BOOTMEM_MMAP, (VOID *) (uintptr_t) pInfo->mmap_addr,
	   MmapLength);
  /* Unsafe to use multiboot info after this. */

  /*
     Step 3: Transform in place multiboot mmap entry into
     bootinfo_region format.

     This is tricky. Requires that the bootinfo_region structure keeps
     smaller than the multiboot mmap entry. This is done this way
     because we have no control over where GRUB will put the memory, we
     don't have an early allocator and can't move it really anywhere.
   */
  assert (sizeof (struct bootinfo_region) <=
	  sizeof (struct multiboot_mmap_entry));

  UINT64 MaxPfn = 0;
  UINT64 MaxRamPfn = 0;
  unsigned Regions = 0;
  size_t Cur;
  volatile struct multiboot_mmap_entry *pMbPtr =
    (struct multiboot_mmap_entry *) BOOTMEM_MMAP;
  volatile struct bootinfo_region *pHrPtr =
    (struct bootinfo_region *) BOOTMEM_MMAP;
  printf ("Multiboot memory map:\n");
  for (Cur = 0; Cur < MmapLength;)
    {
      size_t MbSize;
      struct bootinfo_region HReg;

      printf ("%016llx:%016llx:%d\n", pMbPtr->addr, pMbPtr->len, pMbPtr->type);
      if (pMbPtr->type == MULTIBOOT_MEMORY_AVAILABLE)
	HReg.type = BOOTINFO_REGION_RAM;
      else
	HReg.type = BOOTINFO_REGION_OTHER;

      HReg.pfn = pMbPtr->addr >> PAGE_SHIFT;
      HReg.len = (pMbPtr->len + PAGE_SIZE - 1) >> PAGE_SHIFT;
      MbSize = pMbPtr->size + sizeof (pMbPtr->size);

      /* Count all memory as maxpfn */
      if (MaxPfn < HReg.pfn + HReg.len)
	MaxPfn = HReg.pfn + HReg.len;

      /* Count RAM maxrampfn */
      if ((HReg.type == BOOTINFO_REGION_RAM)
	  && (MaxRamPfn < HReg.pfn + HReg.len))
	MaxRamPfn = HReg.pfn + HReg.len;

      /* We consumed this entry. Can write the hreg region. */
      *pHrPtr = HReg;

      pMbPtr = (VOID *) pMbPtr + MbSize;
      pHrPtr++;
      Regions++;
      Cur += MbSize;
    }

  gBootinfoRegions = Regions;
  gBootinfoMaxPfn = MaxPfn;
  gBootinfoMaxRamPfn = MaxRamPfn;
}


/**
  Parse Multiboot information.

  Extracts framebuffer and memory map information from Multiboot
  info structure passed by bootloader.

  @param[in] pInfo  Pointer to Multiboot info structure.
**/
VOID
ParseMultiboot (
  IN struct multiboot_info  *pInfo
  )
{
  if (pInfo->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)
    ParseMultibootFramebuffer (pInfo);

  assert (pInfo->flags & MULTIBOOT_INFO_MEM_MAP);
  ParseMultibootMmap (pInfo);
}


/**
  Get payload start address.

  Returns pointer to ELF payload in memory.

  @param[in] Argc  Argument count (unused on Multiboot).
  @param[in] Argv  Argument vector (unused on Multiboot).
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
  Initialize machine-dependent subsystem.

  Locates ELF payloads and initializes heap allocator.
**/
VOID
MdInitialize (
  VOID
  )
{
  uintptr_t Ptr;

  gpElfKernelPayload = payload_get (0, &gElfKernelPayloadSize);
  Ptr = (uintptr_t) gpElfKernelPayload + gElfKernelPayloadSize;
  gBrk = PAGE_ROUND (Ptr);

  gpElfUserPayload = payload_get (1, &gElfUserPayloadSize);
  Ptr = (uintptr_t) gpElfUserPayload + gElfUserPayloadSize;
  gBrk = PAGE_ROUND (Ptr);

  printf ("multiboot initialised: brk at %08x\n", gBrk);
}

/**
  Get maximum page frame number.

  Returns maximum physical page frame number.

  @return Maximum PFN.
**/
UINT64
MdMaxPfn (
  VOID
  )
{
  return gBootinfoMaxPfn;
}

/**
  Get minimum RAM page frame number.

  Returns lowest RAM address as page frame number. Always 0 for
  Multiboot.

  @return Minimum RAM PFN (0).
**/
UINT64
MdMinRamPfn (
  VOID
  )
{
  return 0;
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
  return gBootinfoMaxRamPfn;
}

/**
  Get memory region count.

  Returns number of memory regions from Multiboot memory map.

  @return Number of memory regions.
**/
unsigned
MdMemRegions (
  VOID
  )
{
  return gBootinfoRegions;
}

/**
  Get memory region descriptor.

  Returns pointer to memory region descriptor.

  @param[in] Index  Region index.

  @return Pointer to region descriptor.
**/
struct bootinfo_region *
MdGetMemRegion (
  IN unsigned  Index
  )
{
  struct bootinfo_region *pHrPtr = (struct bootinfo_region *) BOOTMEM_MMAP;

  assert (Index < gBootinfoRegions);
  return pHrPtr + Index;
}

/**
  Get framebuffer descriptor.

  Returns framebuffer descriptor from Multiboot info.

  @return Pointer to framebuffer descriptor.
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

  Returns platform descriptor with ACPI RSDP pointer.

  @return Pointer to platform descriptor.
**/
struct apxh_pltdesc *
MdGetPltDesc (
  VOID
  )
{
  /* Only ACPI supported. */
  gPltDesc.type = PLT_ACPI;
  gPltDesc.pltptr = RsdpFind ();
  return &gPltDesc;
}

/**
  Verify virtual address range.

  Validates that virtual address range won't overwrite critical
  boot structures (memory map).

  @param[in] Va    Virtual address.
  @param[in] Size  Size of region.
**/
VOID
MdVerify (
  IN vaddr_t   Va,
  IN size64_t  Size
  )
{
  /* Check that we're not overwriting something we'll need */

  if (Va < BOOTMEM_MMAP + BOOTMEM_MMAPSIZE)
    {
      printf ("ELF would overwrite memory map");
      exit (-1);
    }
}

static UINT64 gPae64Gdt[3] __attribute__((aligned (64))) = {
  0,
  0x00a09a0000000000LL,
};

static struct gdtreg
{
  UINT16 Size;
  UINT32 Base;
} __attribute__((aligned (64)))
     __packed gGdtReg = {
       .Size = 15,
       .Base = 0,

     };

/**
  Transfer control to AMD64 kernel.

  Sets up long mode (AMD64) with PAE64 paging and transfers control
  to kernel entry point. Creates trampoline page table and switches
  CPU to 64-bit mode.

  @param[in] Pt     Page table root physical address.
  @param[in] Entry  Kernel entry point virtual address.
**/
VOID
MbAmd64Entry (
  IN vaddr_t  Pt,
  IN vaddr_t  Entry
  )
{
  VOID *pTrampCr3;
  VOID *pTramp;
  UINT16 TrampCode = 0xe7ff;	/* jmp *%rdi */
  unsigned long Cr0, Cr3, Cr4;
  UINT64 Efer;
  vaddr_t TrampEntry;

  /* Allocate trampoline pagetable. */
  pTrampCr3 = (VOID *) GetPage ();

  /* Setup trampoline. */
  pTramp = (VOID *) GetPage ();
  *(UINT16 *) pTramp = TrampCode;
  TrampEntry = (vaddr_t) (uintptr_t) pTramp;

  printf ("tramp is %lx (%x)\n", pTramp, *(UINT64 *) pTramp);

  /* Setup Direct map at 0->1Gb */
  pae64_directmap (pTrampCr3, 0, 0, 1L << 30, MEMTYPE_WB, 0, 1);

  /* Map Entry page in transitional pagetable VA. */
  pae64_map_page (pTrampCr3, (vaddr_t) Entry, pae64_getphys (Entry), 0, 0, 1);
  printf ("mapping in %lx %llx at %lx\n", pTrampCr3, Entry,
	  pae64_getphys (Entry));

  Cr4 = read_cr4 ();
  write_cr4 (Cr4 | CR4_PAE);
  printf ("CR4: %08lx -> %08lx.\n", Cr4, read_cr4 ());

  Cr3 = read_cr3 ();
  write_cr3 ((unsigned long) pTrampCr3);
  printf ("CR3: %08lx -> %08lx.\n", Cr3, read_cr3 ());

  Efer = rdmsr (MSR_IA32_EFER);
  wrmsr (MSR_IA32_EFER, Efer | _MSR_IA32_EFER_LME);
  printf ("EFER: %016llx -> %016llx.\n", Efer, rdmsr (MSR_IA32_EFER));

  Cr0 = read_cr0 ();
  write_cr0 (Cr0 | CR0_PG | CR0_WP);
  printf ("CR0: %08lx -> %08lx.\n", Cr0, read_cr0 ());

  gGdtReg.Base = (UINT32) ((uintptr_t) & gPae64Gdt & 0xffffffff),
    lgdt ((uintptr_t) & gGdtReg);

  asm volatile
    ("ljmp $8,$1f\n"
     ".code64\n"
     "1:\n"
     "mov %0, %%rax\n"
     "mov %1, %%rdi\n"
     "mov %2, %%rsi\n"
     "jmp *%%rax\n" ".code32"::"m" (TrampEntry), "m" (Entry), "m" (Pt));

  exit (-1);
}

/**
  Transfer control to i386 kernel.

  Sets up PAE paging and transfers control to 32-bit kernel entry
  point. Creates trampoline page table.

  @param[in] Pt     Page table root physical address.
  @param[in] Entry  Kernel entry point virtual address.
**/
VOID
Mb386Entry (
  IN vaddr_t  Pt,
  IN vaddr_t  Entry
  )
{
  VOID *pTrampCr3;
  VOID *pTramp;
  vaddr_t TrampEntry;
  UINT16 TrampCode = 0xe7ff;	/* jmp *%edi */
  unsigned long Cr4 = read_cr4 ();
  unsigned long Cr3 = read_cr3 ();
  unsigned long Cr0 = read_cr0 ();

  /* Allocate trampoline pagetable. */
  pTrampCr3 = (VOID *) GetPage ();

  /* Setup trampoline. */
  pTramp = (VOID *) GetPage ();
  *(UINT16 *) pTramp = TrampCode;
  TrampEntry = (vaddr_t) (uintptr_t) pTramp;

  printf ("tramp is %lx (%x)\n", pTramp, *(UINT64 *) pTramp);

  /* Setup Direct map at 0->1Gb */
  pae_directmap (pTrampCr3, 0, 0, 1L << 30, MEMTYPE_WB, 0, 1);

  /* Map Entry page in transitional pagetable VA. */
  pae_map_page (pTrampCr3, (vaddr_t) Entry, pae_getphys (Entry), 0, 0, 1);
  printf ("mapping in %lx %llx at %lx\n", pTrampCr3, Entry,
	  pae_getphys (Entry));

  write_cr4 (Cr4 | CR4_PAE);
  printf ("CR4: %08lx -> %08lx.\n", Cr4, read_cr4 ());

  write_cr3 ((unsigned long) pTrampCr3);
  printf ("CR3: %08lx -> %08lx.\n", Cr3, read_cr3 ());

  write_cr0 (Cr0 | CR0_PG | CR0_WP);
  printf ("CR0: %08lx -> %08lx.\n", Cr0, read_cr0 ());

  asm volatile
    ("mov %0, %%eax\n"
     "mov %1, %%edi\n"
     "mov %2, %%esi\n"
     "jmp *%%eax\n"::"m" (TrampEntry), "m" (Entry), "m" (Pt));
}

/**
  Transfer control to kernel.

  Architecture dispatcher for kernel entry. Supports i386 and AMD64.

  @param[in] Arch   Architecture (ARCH_386 or ARCH_AMD64).
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
  switch (Arch)
    {
    case ARCH_386:
      Mb386Entry (Pt, Entry);
      break;
    case ARCH_AMD64:
      MbAmd64Entry (Pt, Entry);
      break;
    default:
      printf ("Architecture not supported by multiboot!\n");
      exit (-1);
      break;
    }
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use ParseMultibootFramebuffer instead **/
static void parse_multiboot_framebuffer (struct multiboot_info *info) {
  ParseMultibootFramebuffer (info);
}

/** @deprecated Use ParseMultibootMmap instead **/
static void parse_multiboot_mmap (struct multiboot_info *info) {
  ParseMultibootMmap (info);
}

/** @deprecated Use ParseMultiboot instead **/
void parse_multiboot (struct multiboot_info *info) {
  ParseMultiboot (info);
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

/** @deprecated Use MdInitialize instead **/
void md_init (void) {
  MdInitialize ();
}

/** @deprecated Use MdMaxPfn instead **/
uint64_t md_maxpfn (void) {
  return MdMaxPfn ();
}

/** @deprecated Use MdMinRamPfn instead **/
uint64_t md_minrampfn (void) {
  return MdMinRamPfn ();
}

/** @deprecated Use MdMaxRamPfn instead **/
uint64_t md_maxrampfn (void) {
  return MdMaxRamPfn ();
}

/** @deprecated Use MdMemRegions instead **/
unsigned md_memregions (void) {
  return MdMemRegions ();
}

/** @deprecated Use MdGetMemRegion instead **/
struct bootinfo_region *md_getmemregion (unsigned i) {
  return MdGetMemRegion (i);
}

/** @deprecated Use MdGetFramebuffer instead **/
struct fbdesc *md_getframebuffer (void) {
  return MdGetFramebuffer ();
}

/** @deprecated Use MdGetPltDesc instead **/
struct apxh_pltdesc *md_getpltdesc (void) {
  return MdGetPltDesc ();
}

/** @deprecated Use MdVerify instead **/
void md_verify (vaddr_t va, size64_t size) {
  MdVerify (va, size);
}

/** @deprecated Use MbAmd64Entry instead **/
void mb_amd64_entry (vaddr_t pt, vaddr_t entry) {
  MbAmd64Entry (pt, entry);
}

/** @deprecated Use Mb386Entry instead **/
void mb_386_entry (vaddr_t pt, vaddr_t entry) {
  Mb386Entry (pt, entry);
}

/** @deprecated Use MdEntry instead **/
void md_entry (arch_t arch, vaddr_t pt, vaddr_t entry) {
  MdEntry (arch, pt, entry);
}

// Legacy global variable aliases
static void *elf_kernel_payload __attribute__((alias("gpElfKernelPayload")));
static void *elf_user_payload __attribute__((alias("gpElfUserPayload")));
static size_t elf_kernel_payload_size __attribute__((alias("gElfKernelPayloadSize")));
static size_t elf_user_payload_size __attribute__((alias("gElfUserPayloadSize")));
static uintptr_t brk __attribute__((alias("gBrk")));
static unsigned bootinfo_regions __attribute__((alias("gBootinfoRegions")));
static uint64_t bootinfo_maxpfn __attribute__((alias("gBootinfoMaxPfn")));
static uint64_t bootinfo_maxrampfn __attribute__((alias("gBootinfoMaxRamPfn")));
static struct fbdesc fbdesc __attribute__((alias("gFbDesc")));
static struct apxh_pltdesc pltdesc __attribute__((alias("gPltDesc")));
static uint64_t pae64_gdt[3] __attribute__((alias("gPae64Gdt")));
static struct gdtreg gdtreg __attribute__((alias("gGdtReg")));
