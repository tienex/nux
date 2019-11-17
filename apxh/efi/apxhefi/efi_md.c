#include "project.h"

void __dead
exit (int st)
{
  printf ("EXIT CALLED!\n");
  
  efi_exit (st);
  while (1);
  //  Exit(_image_handle, EFI_SUCCESS, 0, NULL);
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
  uint64_t tramp_code = 0xe7ffd9220fL; /* mov %rcx, %cr3; jmp *%rdi */

  assert (arch == ARCH_AMD64);

  printf ("Entry called.\n");

  /* Allocate trampoline pagetable. */
  trampcr3 = (void *)get_page ();

  /* Setup trampoline. */
  tramp = (void *)get_page ();
  *(uint64_t *)tramp = tramp_code;
  tramp_entry = (vaddr_t)(uintptr_t)tramp;

  uint8_t *p8 = (uint8_t *)tramp;

  printf ("tramp is %lx (%lx) [%x %x %x %x %x]\n", tramp, *(uint64_t *)tramp,
	  p8[0], p8[1], p8[2], p8[3], p8[4]);

  /* Setup Direct map at 0->1Gb */
  pae64_directmap (trampcr3, 0, 1L << 30, 0, 1);

  /* Map Entry page in transitional pagetable VA. */
  p8 = (uint8_t *)pae64_getphys(entry);
  printf ("Entry: %02x %02x %02x %02x %02x\n", p8[0], p8[1], p8[2], p8[3], p8[4]);
  
  pae64_map_page (trampcr3, (vaddr_t)entry, pae64_getphys(entry), 0, 0, 1);
  printf ("mapping in %lx %llx at %lx\n", trampcr3, entry, pae64_getphys(entry));

  /* Assume physical mapping mode, 1:1 on lower addresses. */
  asm volatile
    ("mov %0, %%rax\n"
     "mov %1, %%rdi\n"
     "mov %2, %%rsi\n"
     "mov %3, %%rcx\n"
     "jmp *%%rax\n"
     :: "m"(tramp_entry), "m"(entry), "m"(pt), "m" (trampcr3));

  while (1);
  write_cr3 ((unsigned long)trampcr3);
  printf (" CHANGED PT\n");

}

unsigned long md_maxpfn (void)
{
  return 0;
}

unsigned long md_acpi_rsdp (void)
{
  return 0;
}

struct bootinfo_region *
md_getmemregion (unsigned i)
{
  return 0;
}

unsigned
md_memregions (void)
{
  return 0;
}

void *
get_payload_start (int argc, char *argv[])
{
  return efi_payload_start();
}

unsigned long
get_payload_size (void)
{
  return efi_payload_size();
}

