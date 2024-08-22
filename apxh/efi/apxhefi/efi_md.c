#include "project.h"

#define BOOTINFO_REGIONS_MAX 1024

static void *elf_kernel_payload, *elf_user_payload;
static size_t elf_kernel_payload_size, elf_user_payload_size;
static unsigned long maxpfn;
static unsigned long maxrampfn;
static unsigned numregions;
static void *efi_rsdp;
static struct fbdesc fbdesc = {.type = FB_INVALID, };

static struct bootinfo_region memregions[BOOTINFO_REGIONS_MAX];

#include "internal.h"

void __dead
exit (int st)
{
  printf ("EXIT CALLED!\n");

  efi_exit (st);
  while (1);
  //  Exit(_image_handle, EFI_SUCCESS, 0, NULL);
}

uintptr_t
get_page (void)
{
  return (uintptr_t) efi_allocate_maxaddr ((unsigned long) BOOTMEM);
}

void
md_init (void)
{
}

void
md_verify (unsigned long va, uint64_t size)
{
  /* Nothing to verify. */
}

void
md_entry (arch_t arch, vaddr_t pt, vaddr_t entry)
{
  void *trampcr3;
  void *tramp;
  vaddr_t tramp_entry;
  uint64_t tramp_code = 0xe7ffd9220fL;	/* mov %rcx, %cr3; jmp *%rdi */

  assert (arch == ARCH_AMD64);

  printf ("Entry called.\n");

  /* Allocate trampoline pagetable. */
  trampcr3 = (void *) get_page ();

  /* Setup trampoline. */
  tramp = (void *) get_page ();
  *(uint64_t *) tramp = tramp_code;
  tramp_entry = (vaddr_t) (uintptr_t) tramp;

  /* Setup Direct map at 0->1Gb */
  pae64_directmap (trampcr3, 0, 0, 64L << 30, MEMTYPE_WB, 0, 1);

  /* Map Entry page in transitional pagetable VA. */
  pae64_map_page (trampcr3, (vaddr_t) entry, pae64_getphys (entry), 0, 0, 1);

  efi_exitbs ();

  /* Assume physical mapping mode, 1:1 on lower addresses. */
  asm volatile
    ("mov %0, %%rax\n"
     "mov %1, %%rdi\n"
     "mov %2, %%rsi\n"
     "mov %3, %%rcx\n"
     "cli\n"
     "jmp *%%rax\n"::"m" (tramp_entry), "m" (entry), "m" (pt),
     "m" (trampcr3));
}

unsigned long
md_maxpfn (void)
{
  return maxpfn;
}

unsigned long
md_maxrampfn (void)
{
  return maxrampfn;
}

struct bootinfo_region *
md_getmemregion (unsigned i)
{
  assert (i < BOOTINFO_REGIONS_MAX);

  return memregions + i;
}

unsigned
md_memregions (void)
{

  return numregions;
}

struct fbdesc *
md_getframebuffer (void)
{
  return &fbdesc;
}

unsigned long
md_acpi_rsdp (void)
{
  return (unsigned long) efi_rsdp;
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

unsigned long
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

void
apxhefi_add_framebuffer (uint64_t addr, uint64_t size,
			 uint32_t width, uint32_t height,
			 uint32_t pitch, uint32_t bpp,
			 uint32_t rm, uint32_t gm, uint32_t bm)
{
  fbdesc.type = FB_RGB;
  fbdesc.addr = addr;
  fbdesc.size = size;

  fbdesc.pitch = pitch;
  fbdesc.width = width;
  fbdesc.height = height;
  fbdesc.bpp = bpp;

  fbdesc.r_mask = rm;
  fbdesc.g_mask = gm;
  fbdesc.b_mask = bm;
}


void
apxhefi_add_memregion (int ram, int bsy, unsigned long pfn, unsigned len)
{
  unsigned cur = numregions;

  if (cur >= BOOTINFO_REGIONS_MAX)
    {
      printf ("Exceeded maximum number of memory regions. (%d >= %d\n",
	      cur, BOOTINFO_REGIONS_MAX);
      return;
    }

  memregions[cur].pfn = pfn;
  memregions[cur].len = len;

  if (pfn + len > maxpfn)
    maxpfn = pfn + len;

  if (ram && !bsy)
    {
      memregions[cur].type = BOOTINFO_REGION_RAM;
      if (pfn + len > maxrampfn)
	maxrampfn = pfn + len;
    }
  else if (ram && bsy)
    memregions[cur].type = BOOTINFO_REGION_BSY;
  else
    memregions[cur].type = BOOTINFO_REGION_OTHER;


  numregions++;
}

void
apxhefi_add_kernel_payload (void *start, size_t size)
{
  elf_kernel_payload = start;
  elf_kernel_payload_size = size;
}

void
apxhefi_add_user_payload (void *start, size_t size)
{
  elf_user_payload = start;
  elf_user_payload_size = size;
}

void
apxhefi_add_rsdp (void *rsdp)
{
  efi_rsdp = rsdp;
}
