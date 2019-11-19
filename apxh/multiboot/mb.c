/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  See COPYING file for the full license.

  SPDX-License-Identifier:	GPL2.0+
*/

#include "project.h"

#include "x86.h"
#include "multiboot.h"

/*
  Physical Layout of boot memory structure.
*/
#define BOOTMEM_MMAP      0x10000  /* Multiboot memory map (256kb Max) */
#define BOOTMEM_MMAPSIZE  0x30000  /* Memory map maximum size */

static void *elf_payload;
static size_t elf_payload_size;

static uintptr_t brk;
static unsigned bootinfo_regions;
static uint64_t bootinfo_maxpfn;

static struct fbdesc fbdesc = {.type = FB_INVALID };

uint64_t rsdp_find (void);

static void
parse_multiboot_framebuffer (struct multiboot_info *info)
{
  if (info->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT)
    {
      fbdesc.type = FB_INVALID;
      return;
    }
  
  /* Warning: we're current setting INDEXED as RGB. */
  fbdesc.type = FB_RGB;
  fbdesc.addr = info->framebuffer_addr;
  fbdesc.size = (uint64_t)info->framebuffer_pitch * info->framebuffer_height;

  fbdesc.pitch = info->framebuffer_pitch;
  fbdesc.width = info->framebuffer_width;
  fbdesc.height = info->framebuffer_height;
  fbdesc.bpp =info->framebuffer_bpp;

  /* XXX: Set mask. */
}

static void
parse_multiboot_mmap (struct multiboot_info *info)
{
  size_t mmap_length;

  mmap_length = info->mmap_length;

  /*
    Step 1: check that we'll fit in the area allocated for the memory
    map. You never know with EFI shitty firmwares of today's hardware.
  */
  assert (mmap_length < BOOTMEM_MMAPSIZE);

  /*
    Step 2: Move multiboot structures to preallocated space.

    We depend here on the fact that we are under the temporary
    boot-time page table, and physical address is mapped 1:1 both at
    the kernel offset and the beginning of virtual memory: we can
    dereference multiboot pointers safely.
  */
  memmove ((void *)BOOTMEM_MMAP, (void *)info->mmap_addr, mmap_length);
  /* Unsafe to use multiboot info after this. */

  /*
    Step 3: Transform in place multiboot mmap entry into
    bootinfo_region format.

    This is tricky. Requires that the bootinfo_region structure keeps
    smaller than the multiboot mmap entry. This is done this way
    because we have no control over where GRUB will put the memory, we
    don't have an early allocator and can't move it really anywhere.
  */
  assert (sizeof (struct bootinfo_region) <= sizeof (struct multiboot_mmap_entry));

  uint64_t maxpfn = 0;
  unsigned regions = 0;
  size_t cur;
  volatile struct multiboot_mmap_entry *mbptr = (struct multiboot_mmap_entry *)BOOTMEM_MMAP;
  volatile struct bootinfo_region *hrptr = (struct bootinfo_region *)BOOTMEM_MMAP;
  printf ("Multiboot memory map:\n");
  for (cur = 0; cur < mmap_length;)
    {
      size_t mbsize;
      struct bootinfo_region hreg;

      printf("%016llx:%016llx:%d\n", mbptr->addr, mbptr->len, mbptr->type);
      if (mbptr->type == MULTIBOOT_MEMORY_AVAILABLE)
	hreg.type = BOOTINFO_REGION_RAM;
      else
	hreg.type = BOOTINFO_REGION_OTHER;

      hreg.pfn = mbptr->addr >> PAGE_SHIFT;
      hreg.len = (mbptr->len + PAGE_SIZE -1) >> PAGE_SHIFT;
      mbsize = mbptr->size + sizeof (mbptr->size);

      /* Count RAM max pfn */
      if ((hreg.type == BOOTINFO_REGION_RAM)
	  && (maxpfn < hreg.pfn + hreg.len))
	maxpfn = hreg.pfn + hreg.len;

      /* We consumed this entry. Can write the hreg region. */
      *hrptr = hreg;

      mbptr = (void *)mbptr + mbsize;
      hrptr++;
      regions++;
      cur += mbsize;
    }

  bootinfo_regions = regions;
  bootinfo_maxpfn = maxpfn;
}


void
parse_multiboot (struct multiboot_info *info)
{
  if (info->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)
    parse_multiboot_framebuffer (info);

  assert (info->flags & MULTIBOOT_INFO_MEM_MAP);
  parse_multiboot_mmap (info);
}


void *
get_payload_start (int argc, char *argv[])
{
  return elf_payload;
}

size_t
get_payload_size (void)
{
  return elf_payload_size;
}

uintptr_t
get_page (void)
{
  uintptr_t m;

  m = PAGE_ROUND(brk);

  brk = m + PAGE_SIZE;

  memset ((void *)m, 0, PAGE_SIZE);

  return m;
}

void
md_init(void)
{
  elf_payload = payload_get(0, &elf_payload_size);

  uintptr_t ptr = (uintptr_t)elf_payload + elf_payload_size;
  brk = PAGE_ROUND(ptr);

  printf("multiboot initialised: brk at %08x\n", brk);
}

uint64_t
md_maxpfn (void)
{
  return bootinfo_maxpfn;
}

unsigned
md_memregions (void)
{
  return bootinfo_regions;
}

struct bootinfo_region *
md_getmemregion (unsigned i)
{
  struct bootinfo_region *hrptr = (struct bootinfo_region *)BOOTMEM_MMAP;  

  assert (i < bootinfo_regions);
  return hrptr + i;
}

struct fbdesc *
md_getframebuffer (void)
{
  return &fbdesc;
}

uint64_t
md_acpi_rsdp (void)
{
  return rsdp_find ();
}

void
md_verify(vaddr_t va, size64_t size)
{
  /* Check that we're not overwriting something we'll need */

  if (va < BOOTMEM_MMAP + BOOTMEM_MMAPSIZE)
    {
      printf ("ELF would overwrite memory map");
      exit(-1);
    }
}

static uint64_t pae64_gdt[3] __attribute__((aligned(64))) = {
  0,
  0x00a09a0000000000LL,
};

static struct gdtreg {
  uint16_t size;
  uint32_t base;
} __attribute__((aligned(64))) __packed gdtreg = {
  .size = 15,
  .base = (uint32_t)((uintptr_t)&pae64_gdt & 0xffffffff),
  
};

void mb_amd64_entry (vaddr_t pt, vaddr_t entry)
{
  void *trampcr3;
  void *tramp;
  uint16_t tramp_code = 0xe7ff; /* jmp *%rdi */
  unsigned long cr0, cr3, cr4;
  uint64_t efer;
  vaddr_t tramp_entry;

  /* Allocate trampoline pagetable. */
  trampcr3 = (void *)get_page ();

  /* Setup trampoline. */
  tramp = (void *)get_page ();
  *(uint16_t *)tramp = tramp_code;
  tramp_entry = (vaddr_t)(uintptr_t)tramp;

  printf ("tramp is %lx (%x)\n", tramp, *(uint64_t *)tramp);

  /* Setup Direct map at 0->1Gb */
  pae64_directmap (trampcr3, 0, 0, 1L << 30, 0, 1);

  /* Map Entry page in transitional pagetable VA. */
  pae64_map_page (trampcr3, (vaddr_t)entry, pae64_getphys(entry), 0, 0, 1);
  printf ("mapping in %lx %llx at %lx\n", trampcr3, entry, pae64_getphys(entry));

  cr4 = read_cr4();
  write_cr4 (cr4 | CR4_PAE);
  printf("CR4: %08lx -> %08lx.\n", cr4, read_cr4());

  cr3 = read_cr3();
  write_cr3 ((unsigned long)trampcr3);
  printf("CR3: %08lx -> %08lx.\n", cr3, read_cr3());

  efer = rdmsr(MSR_IA32_EFER);
  wrmsr(MSR_IA32_EFER, efer | _MSR_IA32_EFER_LME);
  printf("EFER: %016llx -> %016llx.\n", efer, rdmsr(MSR_IA32_EFER));

  cr0 = read_cr0();
  write_cr0 (cr0 | CR0_PG | CR0_WP);
  printf("CR0: %08lx -> %08lx.\n", cr0, read_cr0());

  lgdt ((uintptr_t)&gdtreg);

  asm volatile
    ("ljmp $8,$1f\n"
     ".code64\n"
     "1:\n"
     "mov %0, %%rax\n"
     "mov %1, %%rdi\n"
     "mov %2, %%rsi\n"
     "jmp *%%rax\n"
     ".code32"
     :: "m"(tramp_entry), "m"(entry), "m"(pt));

  exit (-1);
}

void mb_386_entry (vaddr_t pt, vaddr_t entry)
{
  void *trampcr3;
  void *tramp;
  vaddr_t tramp_entry;
  uint16_t tramp_code = 0xe7ff; /* jmp *%edi */
  unsigned long cr4 = read_cr4();
  unsigned long cr3 = read_cr3();
  unsigned long cr0 = read_cr0();

  /* Allocate trampoline pagetable. */
  trampcr3 = (void *)get_page ();

  /* Setup trampoline. */
  tramp = (void *)get_page ();
  *(uint16_t *)tramp = tramp_code;
  tramp_entry = (vaddr_t)(uintptr_t)tramp;

  printf ("tramp is %lx (%x)\n", tramp, *(uint64_t *)tramp);

  /* Setup Direct map at 0->1Gb */
  pae_directmap (trampcr3, 0, 0, 1L << 30, 0, 1);

  /* Map Entry page in transitional pagetable VA. */
  pae_map_page (trampcr3, (vaddr_t)entry, pae_getphys(entry), 0, 0, 1);
  printf ("mapping in %lx %llx at %lx\n", trampcr3, entry, pae_getphys(entry));

  write_cr4 (cr4 | CR4_PAE);
  printf("CR4: %08lx -> %08lx.\n", cr4, read_cr4());

  write_cr3 ((unsigned long)trampcr3);
  printf("CR3: %08lx -> %08lx.\n", cr3, read_cr3());

  write_cr0 (cr0 | CR0_PG | CR0_WP);
  printf("CR0: %08lx -> %08lx.\n", cr0, read_cr0());

  asm volatile   
    ("mov %0, %%eax\n"
     "mov %1, %%edi\n"
     "mov %2, %%esi\n"
     "jmp *%%eax\n"
     :: "m"(tramp_entry), "m"(entry), "m"(pt));
}

void
md_entry(arch_t arch, vaddr_t pt, vaddr_t entry)
{
  switch (arch)
    {
    case ARCH_386:
      mb_386_entry (pt, entry);
      break;
    case ARCH_AMD64:
      mb_amd64_entry (pt, entry);
      break;
    default:
      printf ("Architecture not supported by multiboot!\n");
      exit(-1);
      break;
    }
}
