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
static UINTN gElfKernelPayloadSize, gElfUserPayloadSize;

static UINTN gBrk;
static UINT32 gBootinfoRegions;
static UINT64 gBootinfoMaxPfn;
static UINT64 gBootinfoMaxRamPfn;

static FRAMEBUFFER_DESC gFbDesc = {.Type = FB_INVALID };

static APXH_PLATFORM_DESCRIPTOR gPlatformDesc;

UINT64 RsdpFind (VOID);

/**
  Parse Multiboot framebuffer information.

  Extracts framebuffer descriptor from Multiboot info structure,
  supporting RGB and indexed modes.

  @param[in] Info  Pointer to Multiboot info structure.
**/
static VOID
ParseMultibootFramebuffer (
  IN struct multiboot_info  *Info
  )
{
  if (Info->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB)
    {
      gFbDesc.Type = FB_RGB;
      gFbDesc.addr = Info->framebuffer_addr;
      gFbDesc.size =
	(UINT64) Info->framebuffer_pitch * Info->framebuffer_height;

      gFbDesc.pitch = Info->framebuffer_pitch;
      gFbDesc.width = Info->framebuffer_width;
      gFbDesc.height = Info->framebuffer_height;
      gFbDesc.bpp = Info->framebuffer_bpp;

#define MB2MASK(_p, _s)  (((1 << (_s)) - 1) << (1 << (_p)))
      gFbDesc.r_mask = MB2MASK (Info->rpos, Info->rsize);
      gFbDesc.g_mask = MB2MASK (Info->gpos, Info->gsize);
      gFbDesc.b_mask = MB2MASK (Info->bpos, Info->bsize);
#undef MB2MASK
    }
  else if (Info->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED)
    {
      /*
         Indexed Frame Buffer not supported!

         Unfortunately not much can be done here as GRUB will have switched
         to frame buffer and we have no way (yet!) to handle it.
       */
      gFbDesc.Type = FB_RGB;
      gFbDesc.addr = Info->framebuffer_addr;
      gFbDesc.size =
	(UINT64) Info->framebuffer_pitch * Info->framebuffer_height;

      gFbDesc.pitch = Info->framebuffer_pitch;
      gFbDesc.width = Info->framebuffer_width;
      gFbDesc.height = Info->framebuffer_height;
      gFbDesc.bpp = Info->framebuffer_bpp;

      gFbDesc.r_mask = 0xff;
      gFbDesc.g_mask = 0xff;
      gFbDesc.b_mask = 0xff;
    }
  else
    {
      gFbDesc.Type = FB_INVALID;
      return;
    }
}

/**
  Parse Multiboot memory map.

  Converts Multiboot memory map to APXH bootinfo_region format
  in-place. Tracks maximum PFN and maximum RAM PFN.

  @param[in] Info  Pointer to Multiboot info structure.
**/
static VOID
ParseMultibootMmap (
  IN struct multiboot_info  *Info
  )
{
  UINTN MmapLength;

  MmapLength = Info->mmap_length;

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
  memmove ((VOID *) BOOTMEM_MMAP, (VOID *) (UINTN) Info->mmap_addr,
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
  assert (sizeof (BOOTINFO_REGION) <=
	  sizeof (struct multiboot_mmap_entry));

  UINT64 MaxPfn = 0;
  UINT64 MaxRamPfn = 0;
  UINT32 Regions = 0;
  UINTN Cur;
  VOLATILE struct multiboot_mmap_entry *MbPtr =
    (struct multiboot_mmap_entry *) BOOTMEM_MMAP;
  VOLATILE BOOTINFO_REGION *HrPtr =
    (BOOTINFO_REGION *) BOOTMEM_MMAP;
  printf ("Multiboot memory map:\n");
  for (Cur = 0; Cur < MmapLength;)
    {
      UINTN MbSize;
      BOOTINFO_REGION HReg;

      printf ("%016llx:%016llx:%d\n", MbPtr->addr, MbPtr->Len, MbPtr->Type);
      if (MbPtr->Type == MULTIBOOT_MEMORY_AVAILABLE)
	HReg.Type = BOOTINFO_REGION_RAM;
      else
	HReg.Type = BOOTINFO_REGION_OTHER;

      HReg.Pfn = MbPtr->addr >> PAGE_SHIFT;
      HReg.Len = (MbPtr->Len + PAGE_SIZE - 1) >> PAGE_SHIFT;
      MbSize = MbPtr->size + sizeof (MbPtr->size);

      /* Count all memory as maxpfn */
      if (MaxPfn < HReg.Pfn + HReg.Len)
	MaxPfn = HReg.Pfn + HReg.Len;

      /* Count RAM maxrampfn */
      if ((HReg.Type == BOOTINFO_REGION_RAM)
	  && (MaxRamPfn < HReg.Pfn + HReg.Len))
	MaxRamPfn = HReg.Pfn + HReg.Len;

      /* We consumed this entry. Can write the hreg region. */
      *HrPtr = HReg;

      MbPtr = (VOID *) MbPtr + MbSize;
      HrPtr++;
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

  @param[in] Info  Pointer to Multiboot info structure.
**/
VOID
ParseMultiboot (
  IN struct multiboot_info  *Info
  )
{
  if (Info->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)
    ParseMultibootFramebuffer (Info);

  assert (Info->flags & MULTIBOOT_INFO_MEM_MAP);
  ParseMultibootMmap (Info);
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

  Returns size of ELF payload in bytes.

  @param[in] Id  Payload identifier (kernel or user).

  @return Size of payload in bytes, or 0 if not found.
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
  Initialize machine-dependent subsystem.

  Locates ELF payloads and initializes heap allocator.
**/
VOID
MdInitialize (
  VOID
  )
{
  UINTN Ptr;

  gpElfKernelPayload = PayloadGet (0, &gElfKernelPayloadSize);
  Ptr = (UINTN) gpElfKernelPayload + gElfKernelPayloadSize;
  gBrk = PAGE_ROUND (Ptr);

  gpElfUserPayload = PayloadGet (1, &gElfUserPayloadSize);
  Ptr = (UINTN) gpElfUserPayload + gElfUserPayloadSize;
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
UINT32
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
BOOTINFO_REGION *
MdGetMemRegion (
  IN UINT32 Index
  )
{
  BOOTINFO_REGION *HrPtr = (BOOTINFO_REGION *) BOOTMEM_MMAP;

  assert (Index < gBootinfoRegions);
  return HrPtr + Index;
}

/**
  Get framebuffer descriptor.

  Returns framebuffer descriptor from Multiboot info.

  @return Pointer to framebuffer descriptor.
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

  Returns platform descriptor with ACPI RSDP pointer.

  @return Pointer to platform descriptor.
**/
APXH_PLATFORM_DESCRIPTOR *
MdGetPlatformDesc (
  VOID
  )
{
  /* Only ACPI supported. */
  gPlatformDesc.Type = PLATFORM_ACPI;
  gPlatformDesc.PlatformPointer = RsdpFind ();
  return &gPlatformDesc;
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
  IN VIRTUAL_ADDRESS   Va,
  IN SIZE64  Size
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

typedef struct _GDTREG
{
  UINT16 Size;
  UINT32 Base;
} __attribute__((aligned (64))) __packed GDTREG;

static GDTREG gGdtReg = {
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
  IN VIRTUAL_ADDRESS  Pt,
  IN VIRTUAL_ADDRESS  Entry
  )
{
  VOID *pTrampCr3;
  VOID *Tramp;
  UINT16 TrampCode = 0xe7ff;	/* jmp *%rdi */
  UINTN Cr0, Cr3, Cr4;
  UINT64 Efer;
  VIRTUAL_ADDRESS TrampEntry;

  /* Allocate trampoline pagetable. */
  pTrampCr3 = (VOID *) GetPage ();

  /* Setup trampoline. */
  Tramp = (VOID *) GetPage ();
  *(UINT16 *) Tramp = TrampCode;
  TrampEntry = (VIRTUAL_ADDRESS) (UINTN) Tramp;

  printf ("tramp is %lx (%x)\n", Tramp, *(UINT64 *) Tramp);

  /* Setup Direct map at 0->1Gb */
  Pae64DirectMap (pTrampCr3, 0, 0, 1L << 30, MEMTYPE_WB, 0, 1);

  /* Map Entry page in transitional pagetable VA. */
  Pae64MapPage (pTrampCr3, (VIRTUAL_ADDRESS) Entry, Pae64GetPhys (Entry), 0, 0, 1);
  printf ("mapping in %lx %llx at %lx\n", pTrampCr3, Entry,
	  Pae64GetPhys (Entry));

  Cr4 = ReadCr4 ();
  WriteCr4 (Cr4 | CR4_PAE);
  printf ("CR4: %08lx -> %08lx.\n", Cr4, ReadCr4 ());

  Cr3 = ReadCr3 ();
  WriteCr3 ((UINTN) pTrampCr3);
  printf ("CR3: %08lx -> %08lx.\n", Cr3, ReadCr3 ());

  Efer = Rdmsr (MSR_IA32_EFER);
  Wrmsr (MSR_IA32_EFER, Efer | _MSR_IA32_EFER_LME);
  printf ("EFER: %016llx -> %016llx.\n", Efer, Rdmsr (MSR_IA32_EFER));

  Cr0 = ReadCr0 ();
  WriteCr0 (Cr0 | CR0_PG | CR0_WP);
  printf ("CR0: %08lx -> %08lx.\n", Cr0, ReadCr0 ());

  gGdtReg.Base = (UINT32) ((UINTN) & gPae64Gdt & 0xffffffff),
    Lgdt ((UINTN) & gGdtReg);

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
  IN VIRTUAL_ADDRESS  Pt,
  IN VIRTUAL_ADDRESS  Entry
  )
{
  VOID *pTrampCr3;
  VOID *Tramp;
  VIRTUAL_ADDRESS TrampEntry;
  UINT16 TrampCode = 0xe7ff;	/* jmp *%edi */
  UINTN Cr4 = ReadCr4 ();
  UINTN Cr3 = ReadCr3 ();
  UINTN Cr0 = ReadCr0 ();

  /* Allocate trampoline pagetable. */
  pTrampCr3 = (VOID *) GetPage ();

  /* Setup trampoline. */
  Tramp = (VOID *) GetPage ();
  *(UINT16 *) Tramp = TrampCode;
  TrampEntry = (VIRTUAL_ADDRESS) (UINTN) Tramp;

  printf ("tramp is %lx (%x)\n", Tramp, *(UINT64 *) Tramp);

  /* Setup Direct map at 0->1Gb */
  PaeDirectMap (pTrampCr3, 0, 0, 1L << 30, MEMTYPE_WB, 0, 1);

  /* Map Entry page in transitional pagetable VA. */
  PaeMapPage (pTrampCr3, (VIRTUAL_ADDRESS) Entry, PaeGetPhys (Entry), 0, 0, 1);
  printf ("mapping in %lx %llx at %lx\n", pTrampCr3, Entry,
	  PaeGetPhys (Entry));

  WriteCr4 (Cr4 | CR4_PAE);
  printf ("CR4: %08lx -> %08lx.\n", Cr4, ReadCr4 ());

  WriteCr3 ((UINTN) pTrampCr3);
  printf ("CR3: %08lx -> %08lx.\n", Cr3, ReadCr3 ());

  WriteCr0 (Cr0 | CR0_PG | CR0_WP);
  printf ("CR0: %08lx -> %08lx.\n", Cr0, ReadCr0 ());

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
  IN ARCH   Arch,
  IN VIRTUAL_ADDRESS  Pt,
  IN VIRTUAL_ADDRESS  Entry
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
