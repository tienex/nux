/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "project.h"

#define RSDP_SIGN "RSD PTR "

#define EBDA_PTRADDR 0x40e
#define KB (1 << 10)

struct acpi_rsdp_thdr
{
  char signature[8];
  uint8_t checksum;
  char oemid[6];
  uint8_t revision;
  uint32_t rsdt;

  /* ACPI >= 2.0 (revision != 0) */
  uint32_t length;
  uint64_t xsdt;
  uint8_t xchecksum;
  uint8_t reserved[3];
} __packed;


static void *
rsdp_scan (void *base, size_t size)
{
  void *ptr;

  for (ptr = (uint64_t *) base; ptr < base + size; ptr++)
    {
      if (!memcmp (ptr, RSDP_SIGN, 8))
	return ptr;
    }

  return NULL;
}

static bool
rsdp_check (void *ptr)
{
  struct acpi_rsdp_thdr *rsdp = (struct acpi_rsdp_thdr *) ptr;
  int i;
  uint8_t sum;

  /* Checksum ACPI V1.0 */
  debug ("Checking Checksum");
  sum = 0;
  for (i = 0; i < 20; i++)
    sum += *(uint8_t *) (ptr + i);

  if (sum != 0)
    {
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
	sum += *(uint8_t *) (ptr + i);

      if (sum != 0)
	{
	  warn ("Extended checksum failed for RSDP at addr %p", ptr);
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
bios_find_rsdp (void)
{
  unsigned paddr;
  void *memstart = (void *) 0;

  debug ("Searching for RSDP.");

  /* Method 1: Search EBDA's first kb. */
  uint16_t ebdaseg = *(uint16_t *) (memstart + EBDA_PTRADDR);
  paddr = ((uintptr_t) ebdaseg << 4);

  if (paddr <= (1 << 10))
    {
      debug ("EBDA PTR %x not valid.", paddr);
    }
  else
    {
      void *ptr;

      debug ("Scanning EBDA at addr %x (%p)", paddr, memstart + paddr);
      ptr = rsdp_scan ((uint8_t *) memstart + paddr, 1 * KB);
      if (ptr != NULL)
	return ptr;
    }

  debug ("Scanning 0xE0000 -> 0xFFFFF");
  return rsdp_scan (memstart + 0xe0000, 0x20000);

}

uint64_t
rsdp_find (void)
{
  void *rsdp;

  rsdp = bios_find_rsdp ();
  if (rsdp == NULL)
    {
      warn ("RSDP not found.");
      return 0;
    }

  if (!rsdp_check (rsdp))
    return 0;

  info ("RSDP found at %p", rsdp);
  return (uint64_t) (uintptr_t) rsdp;
}
