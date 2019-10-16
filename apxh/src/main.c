
#include "project.h"

#define BOOTMEM MB(512) /* We won't be using more than 512Mb to boot. Promise. */

static arch_t elf_arch;
static uint8_t boot_pagemap[PAGEMAP_SZ(BOOTMEM)] __attribute__((aligned(4096)));
static unsigned long req_pfnmap_va, req_info_va;
static size_t req_pfnmap_size, req_info_size;
static bool stop_payload_allocation = false;


struct apxh_bootinfo
{
#define APXH_BOOTINFO_MAGIC 0xAF10B007
  uint64_t magic;
  uint64_t maxpfn;
  uint64_t pagemap_size;
  uint8_t  pagemap[0];
} __attribute__((packed));


uintptr_t
get_payload_page(void)
{
  unsigned pfn;
  uintptr_t page;

  assert (!stop_payload_allocation);

  page = get_page();
  memset ((void *)page, 0, PAGE_SIZE);

  pfn = page >> PAGE_SHIFT;

  boot_pagemap[pfn >> 3] |= 1 << (pfn & 7);

  return page;
}

unsigned
check_payload_page (unsigned addr)
{
  unsigned i = addr >> PAGE_SHIFT;
  unsigned by = i >> 3;
  unsigned bi = i & 7;

  assert (by <= PAGEMAP_SZ(BOOTMEM));

  return !!(boot_pagemap[by] & bi);
}

void init (void)
{
  md_init ();
}

const char *
get_arch_name (arch_t arch)
{
  switch (arch) {
  case ARCH_INVALID:
    return "invalid";
  case ARCH_UNSUPPORTED:
    return "unsupported";
  case ARCH_386:
    return "i386";
  default:
    return "unknown";
  }
}

void
va_init (void)
{
  switch (elf_arch) {
  case ARCH_386:
    pae_init();
    break;
  default:
    printf("Unsupported VM architecture.\n");
    exit (-1);
  }
}

void
va_populate (unsigned long va, size_t size, int w, int x)
{
  md_verify (va, size);
  va_verify (va, size);

  switch (elf_arch) {
  case ARCH_386:
    pae_populate(va, size, w, x);
    break;
  default:
    printf("Unsupported VM architecture.\n");
    exit (-1);
  }
}

void
va_copy (unsigned long va, void *addr, size_t size, int w, int x)
{
  ssize_t len = size;

  md_verify(va, size);
  va_verify(va, size);

  printf("Copying %08lx <- %p (w:%d, x:%d, %d bytes)\n", va, addr, w, x, size);
  va_populate(va, size, w, x);

  while (len > 0)
    {
      uintptr_t paddr;
      size_t clen = PAGE_CEILING(va) - va;

      paddr = va_getphys (va);

      memcpy ((void *)paddr, addr, clen);

      len -= clen;
      va += clen;
      addr += clen;
    }
}

void
va_memset (unsigned long va, int c, size_t size, int w, int x)
{
  ssize_t len = size;

  md_verify(va, size);
  va_verify(va, size);

  printf("Setting %08lx <- %d (w:%d, x: %d, %d bytes)\n", va, c, w, x, size);
  va_populate(va, size, w, x);

  while (len > 0)
    {
      uintptr_t paddr;
      size_t clen = PAGE_CEILING(va) - va;

      paddr = va_getphys (va);

      memset ((void *)paddr, 0, clen);

      len -= clen;
      va += clen;
    }
}

void
va_physmap (unsigned long va, size_t size)
{
  md_verify (va, size);
  va_verify (va, size);

  switch (elf_arch) {
  case ARCH_386:
    pae_physmap (va, size);
    break;
  default:
    printf("Unsupported VM architecture.\n");
    exit (-1);
  }
}

void
va_info (unsigned long va, size_t size)
{
  md_verify (va, size);
  va_verify (va, size);

  va_populate (va, size, 0, 0);

  /* Only save the va and size, we'll have to finish all allocations
     before we can return the proper data. */
  req_info_va = va;
  req_info_size = size;
}

static void
va_info_copy (void)
{
  unsigned long va = req_info_va;
  size_t size = req_info_size;
#define MIN(x,y) ((x < y) ? x : y)
  struct apxh_bootinfo i;
  uint64_t psize;

  if (va == 0) {
    /* No INFO. Skip. */
    return;
  }

  /* The pagemap size. */
  if (size > sizeof (struct apxh_bootinfo))
    psize = MIN (size - sizeof (struct apxh_bootinfo), sizeof (boot_pagemap));
  else
    psize = 0;

  i.magic = APXH_BOOTINFO_MAGIC;
  i.maxpfn = md_maxpfn ();
  i.pagemap_size = psize;

  va_copy (va, &i, MIN(size, sizeof (struct apxh_bootinfo)), 0, 0);

  va += sizeof (struct apxh_bootinfo);
  va_copy (va, boot_pagemap, psize, 0, 0);
#undef MIN
}

void
va_pfnmap (unsigned long va, size_t size)
{
  unsigned i, maxframe;
  struct bootinfo_region *reg;
  unsigned regions = md_memregions ();

  maxframe = size / PFNMAP_ENTRY_SIZE;

  if (maxframe > md_maxpfn ()) {
    maxframe = md_maxpfn ();
    size = maxframe * PFNMAP_ENTRY_SIZE;
  }

  va_populate (va, size, 1, 0);

  for (i = 0; i < regions; i++)
    {
      unsigned j;

      reg = md_getmemregion (i);

      printf ("Reg: %d Type %02d, PA: %016llx (%ld)\n", i, reg->type, (uint64_t)reg->pfn << PAGE_SHIFT, reg->len);


      for (j = 0; j < reg->len; j++)
	{
	  unsigned frame = reg->pfn + j;
	  uint8_t *ptr;

	  if (frame > maxframe) {
	    printf ("Maximum reached.\n");
	    break;
	  }

	  ptr = (uint8_t *)va_getphys (va + frame * PFNMAP_ENTRY_SIZE);
	  assert (ptr != NULL);

	  /* There's  a  priority  in  numbering of  regions.  RAM  is
	     lowest, overwritten most easily. */
	  if (*ptr < reg->type)
	    *ptr = reg->type;
	}
    }

  req_pfnmap_va = va;
  req_pfnmap_size = size;
}

static void
va_pfnmap_copy (void)
{
  unsigned long va = req_pfnmap_va;
  unsigned long size = req_pfnmap_size;
  unsigned maxframe = size / PFNMAP_ENTRY_SIZE;
#define MIN(x,y) ((x < y) ? x : y)
  unsigned long pa;
  
  if (va == 0)
    {
      /* No PFNMAP. Skip. */
      return;
    }

  for (pa = 0; pa < BOOTMEM; pa += PAGE_SIZE)
    {
      unsigned frame = pa >> PAGE_SHIFT;

      if (frame > maxframe)
	break;

      if (check_payload_page (pa))
	{
	  /* Page is allocated. Mark as BSY. */

	  uint8_t *ptr = (uint8_t *)va_getphys (va + frame * PFNMAP_ENTRY_SIZE);
	  assert (ptr != NULL);

	  *ptr = BOOTINFO_REGION_BSY;
	}
    }
#undef MIN
}

void
va_verify (unsigned long va, size_t size)
{
  switch (elf_arch) {
  case ARCH_386:
    pae_verify(va, size);
    break;
  default:
    printf("Unsupported VM architecture.\n");
    exit (-1);
  }
}

uintptr_t
va_getphys (unsigned long va)
{
  switch (elf_arch) {
  case ARCH_386:
    return pae_getphys(va);
    break;
  default:
    printf("Unsupported VM architecture.\n");
    exit (-1);
  }
}

void
va_entry (unsigned long entry)
{

  switch (elf_arch) {
  case ARCH_386:
    pae_entry(entry);
    break;
  default:
    printf("Unsupported VM architecture.\n");
    exit (-1);
  }

  printf("Returned from entry!");
  exit (-1);
}

int main (int argc, char *argv[])
{
  void *elf_start;
  size_t elf_size;
  uintptr_t entry;

  printf("\nAPXH started.\n\n");

  init();

  elf_start = get_payload_start (argc, argv);
  elf_size = get_payload_size ();
  elf_arch = get_elf_arch(elf_start);
  printf("Payload %s ELF at addr %p (%d bytes)\n", get_arch_name(elf_arch), elf_start, elf_size);

  va_init();

  switch (elf_arch) {
  case ARCH_386:
    entry = load_elf32(elf_start);
    break;
  default:
    printf ("Unsupported ELF architecture");
    exit (-1);
  }

  printf("entry = %lx\n", entry);

  /* Stop allocations as we're copying boot-time allocation. */
  stop_payload_allocation = true;
  va_info_copy ();
  va_pfnmap_copy ();

  va_entry (entry);
  return 0;
}


