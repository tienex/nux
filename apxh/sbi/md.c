#include "project.h"
#include <string.h>
#include <libfdt.h>
#include <inttypes.h>

extern long boothid;
extern void *dtbptr;
static uint64_t maxaddr = 0;
static unsigned regions = 0;

#define SBI_MAX_RAM_REGIONS 64
struct bootinfo_region ram_regions[SBI_MAX_RAM_REGIONS] = { 0, };

static void *elf_kernel_payload, *elf_user_payload;
static size_t elf_kernel_payload_size, elf_user_payload_size;

static uintptr_t brk;

static void
dtb_addrsize(const void *fdt, int noff, uint32_t *addrsz, uint32_t *sizesz)
{
  int poff, len;
  void *p;

  poff = fdt_parent_offset(fdt, noff);

  p = (void *)fdt_getprop(fdt, poff, "#address-cells", &len);
  if (!p || len != 4)
    {
      *addrsz = 2;
    }
  else
    {
      *addrsz = fdt32_to_cpu(*(uint32_t *)p);
    }

  p = (void *)fdt_getprop(fdt, poff, "#size-cells", &len);
  if (!p || len != 4)
    {
      *sizesz = 2;
    }
  else
    {
      *sizesz = fdt32_to_cpu(*(uint32_t *)p);
    }
}

static void ramregion_foreach(void (*_f)(uint64_t, uint64_t, bool, void *), void *opq)
{
  void *fdt = dtbptr;
  int noff;

  for (noff = fdt_next_node(fdt, -1, NULL);
       noff >= 0;
       noff = fdt_next_node(fdt, noff, NULL))
    {
      const char *name = fdt_get_name(fdt, noff, NULL);
      if (!name)
	continue;

      // I should probably get the root child only here. 
      if (!strncmp(name, "memory", 6))
	{
	  int len;
	  uint32_t addrsz, sizesz;

	  uint32_t *reg = (uint32_t *)fdt_getprop(fdt, noff, "reg", &len);
	  if (!reg)
	    continue;

	  dtb_addrsize(fdt, noff, &addrsz, &sizesz);

	  for (int i = 0; i < len; i += 4 * (addrsz + sizesz))
	    {
	      uint64_t base, size;

	      base = 0;
	      for (int j = 0; j < addrsz; j++)
		base = (base << 32) + fdt32_to_cpu(*reg++);

	      size = 0;
	      for (int j = 0; j < sizesz; j++)
		size = (size << 32) + fdt32_to_cpu(*reg++);

	      _f(base, size, false, opq);
	    }
	}
      else if (!strncmp(name, "reserved-memory", 15))
	{
	  int len, suboff;
	  uint32_t addrsz, sizesz;

	  dtb_addrsize(fdt, noff, &addrsz, &sizesz);

	  fdt_for_each_subnode(suboff, fdt, noff)
	    {
	      uint32_t *reg = (uint32_t *)fdt_getprop(fdt, suboff, "reg", &len);
	      if (!reg)
		continue;
	      
	      for (int i = 0; i < len; i += 4 * (addrsz + sizesz))
		{
		  uint64_t base, size;

		  base = 0;
		  for (int j = 0; j < addrsz; j++)
		    base = (base << 32) + fdt32_to_cpu(*reg++);

		  size = 0;
		  for (int j = 0; j < sizesz; j++)
		    size = (size << 32) + fdt32_to_cpu(*reg++);

		  _f(base, size, true, opq);
		}
	    }
	}
    }
}

void *
get_payload_start (int argc, char *argv[], plid_t id)
{
  void *elf_payload;

  switch (id)
    {
    case PAYLOAD_KERNEL:
      elf_payload = elf_kernel_payload;
      break;
    case PAYLOAD_USER:
      elf_payload = elf_user_payload;
      break;
    default:
      printf ("Unsupported payload ID %d\n", id);
      elf_payload = NULL;
      break;
    }

  return elf_payload;
}

size_t
get_payload_size (plid_t id)
{
  size_t elf_payload_size;

  switch (id)
    {
    case PAYLOAD_KERNEL:
      elf_payload_size = elf_kernel_payload_size;
      break;
    case PAYLOAD_USER:
      elf_payload_size = elf_user_payload_size;
      break;
    default:
      printf ("Unsupported payload ID %d\n", id);
      elf_payload_size = 0;
      break;
    }

  return elf_payload_size;
}

uintptr_t
get_page (void)
{
  uintptr_t m;

  m = PAGE_ROUND (brk);

  brk = m + PAGE_SIZE;

  memset ((void *) m, 0, PAGE_SIZE);

  return m;
}

static void
add_ramregion(uint64_t base, uint64_t len, bool busy, void *opq)
{
  struct bootinfo_region *r;

  if (regions >= SBI_MAX_RAM_REGIONS)
    {
      printf("Too many regions. Ignoring %016lx:%016lx (%s)\n", base, base+len, busy ? "SYS" : "RAM");
      return;
    }

  if (base + len >= maxaddr)
    maxaddr = base+len;

  r = ram_regions + regions++;
  r->type = busy ? BOOTINFO_REGION_BSY : BOOTINFO_REGION_RAM;
  r->pfn = base >> PAGE_SHIFT;
  r->len = len >> PAGE_SHIFT;

  printf("\t%016lx:%016lx (%s)\n", base, base+len, busy? "SYS" : "RAM");
}

void
md_init (void)
{
  uintptr_t ptr;

  printf("Booting from HART %lx\n", boothid);
  printf("DTB Pointer at %lx\n", dtbptr);
  
  if (fdt_check_header((const void *)dtbptr) != 0) {
    printf("Invalid DTB header.\n");
    exit(-1);
  }

  printf("Device Tree Memory Regions:\n");
  ramregion_foreach(add_ramregion, NULL);
  printf("\n");

  elf_kernel_payload = payload_get (0, &elf_kernel_payload_size);
  ptr = (uintptr_t) elf_kernel_payload + elf_kernel_payload_size;
  brk = PAGE_ROUND (ptr);

  elf_user_payload = payload_get (1, &elf_user_payload_size);
  ptr = (uintptr_t) elf_user_payload + elf_user_payload_size;
  brk = PAGE_ROUND (ptr);

  printf ("multiboot initialised: brk at %08x\n", brk);
}

void
md_verify (vaddr_t va, size64_t size)
{
  /* Nothing to verify. */
}

unsigned
md_memregions (void)
{
  return regions;
}

struct bootinfo_region *
md_getmemregion (unsigned i)
{
  if (i >= SBI_MAX_RAM_REGIONS)
    return NULL;

  return ram_regions + i;
}

uint64_t
md_maxrampfn (void)
{
  
  return maxaddr >> PAGE_SHIFT;
}
