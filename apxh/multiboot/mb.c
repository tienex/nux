#include "project.h"

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

      if (maxpfn < hreg.pfn + hreg.len)
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

void
md_verify(unsigned long va, size_t size)
{
  /* Check that we're not overwriting something we'll need */

  if (va < BOOTMEM_MMAP + BOOTMEM_MMAPSIZE)
    {
      printf ("ELF would overwrite memory map");
      exit(-1);
    }
}
