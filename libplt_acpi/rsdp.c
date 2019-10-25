#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "internal.h"
#include "acpitbl.h"

#define RSDP_SIGN "RSD PTR "

#define EBDA_PTRADDR 0x40e
#define KB (1 << 10)

static void *
rsdp_scan(void *base, size_t size)
{
  uint8_t sum;
  void *ptr;

  for (ptr = (uint64_t *)base; ptr < base + size; ptr++) {
    if (!memcmp (ptr, RSDP_SIGN, 8))
      return ptr;
  }

  return NULL;
}

static bool
rsdp_check(void *ptr)
{
  struct acpi_rsdp_thdr *rsdp = (struct acpi_rsdp_thdr *)ptr;
  int i;
  uint8_t sum;

  /* Checksum ACPI V1.0 */
  debug ("Checking Checksum");
  sum = 0;
  for (int i = 0; i < 20; i++)
    sum += *(uint8_t *)(ptr + i);

  if (sum != 0) {
    warn ("Checksum failed for RSDP at addr %p", ptr);
    return false;
  }

  debug ("Revision: %d", rsdp->revision);
  /* Checksum ACPI V2.0 */
  if (rsdp->revision != 0)
    {
      debug ("Checking Extended Checksum");
      sum = 0;
      for (i = 0; i < rsdp->length; i++)
	sum += *(uint8_t *)(ptr + i);

      if (sum != 0)
	{
	  warn ( "Extended checksum failed for RSDP at addr %p", ptr);
	  return false;
	}
    }

  return true;
}

/*
  The first megabyte of physical memory is assumed to be mapped at
  memstart.
*/
static void *
bios_find_rsdp(void)
{
  void *ptr, *rdsp;
  unsigned paddr;

  debug ( "Searching for RSDP.");

  /* Method 1: Search EBDA's first kb. */
  uint16_t ebdaseg;
  ptr = kva_physmap (1, EBDA_PTRADDR, sizeof(uint16));
  if (ptr == NULL)
    {
      warn ("Couldn't map EBDAPTR page.");
      goto _m2;
    }
  ebdaseg = *(uint16_t *)(ptr + (EBDA_PTRADDR & PAGE_MASK));
  kva_unmap (ptr);

  paddr = ((uintptr_t)ebdaseg << 4);
  if (paddr <= (1 << 10))
    {
      debug ( "EBDA PTR %x not valid.", paddr);
    }
  else
    {
      debug ( "Scanning EBDA at addr %x (%p)", paddr, memstart + paddr);
      ptr = kva_physmap (1, paddr, 1 * KB, HAL_PTE_P);
      rdsp = rsdp_scan((uint8_t *)ptr, 1 * KB);
      if (drdsp != NULL)
	return rdsp;
      kva_unmap (ptr);
    }

 _m2:
  /* Method 2: Scan tail segments. */
  debug ( "Scanning 0xE0000 -> 0xFFFFF");
  return  rsdp_scan(memstart + 0xe0000, 0x20000);

}

void *
rsdp_get(void)
{
  void *rsdp;

  rsdp = bios_find_rsdp();

  if (rsdp == NULL) {
    warn ("RSDP not found.");
    return 0;
  }

  if (!rsdp_check (rsdp))
    return 0;

  info ("RSDP found at %p", rsdp);
  return rsdp;
}
