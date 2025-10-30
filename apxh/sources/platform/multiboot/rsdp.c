/** @file
  APXH ACPI RSDP Discovery

  Implements ACPI Root System Description Pointer (RSDP) discovery for
  Multiboot platform. Searches EBDA and BIOS ROM areas for RSDP signature.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
**/

#include <apxh/internal.h>

#define RSDP_SIGN "RSD PTR "

#define EBDA_PTRADDR 0x40e
#define KB (1 << 10)

struct acpi_rsdp_thdr
{
  char signature[8];
  UINT8 checksum;
  char oemid[6];
  UINT8 revision;
  UINT32 rsdt;

  /* ACPI >= 2.0 (revision != 0) */
  UINT32 length;
  UINT64 xsdt;
  UINT8 xchecksum;
  UINT8 reserved[3];
} __packed;


/**
  Scan memory region for RSDP signature.

  Searches memory region for ACPI RSDP signature "RSD PTR ".

  @param[in] Base  Base address to start scanning.
  @param[in] Size   Size of region to scan.

  @return Pointer to RSDP if found, NULL otherwise.
**/
static VOID *
RsdpScan (
  IN VOID    *Base,
  IN size_t  Size
  )
{
  VOID *Ptr;

  for (Ptr = (UINT64 *) Base; Ptr < Base + Size; Ptr++)
    {
      if (!memcmp (Ptr, RSDP_SIGN, 8))
	return Ptr;
    }

  return NULL;
}

/**
  Verify RSDP checksum.

  Validates RSDP checksum for both ACPI 1.0 and 2.0+ versions.

  @param[in] Ptr  Pointer to RSDP structure.

  @retval TRUE   Checksum is valid.
  @retval FALSE  Checksum is invalid.
**/
static bool
RsdpCheck (
  IN VOID  *Ptr
  )
{
  struct acpi_rsdp_thdr *Rsdp = (struct acpi_rsdp_thdr *) Ptr;
  int i;
  UINT8 Sum;

  /* Checksum ACPI V1.0 */
  debug ("Checking Checksum");
  Sum = 0;
  for (i = 0; i < 20; i++)
    Sum += *(UINT8 *) (Ptr + i);

  if (Sum != 0)
    {
      warn ("Checksum failed for RSDP at addr %p", Ptr);
      return false;
    }

  debug ("Revision: %d", Rsdp->revision);
  /* Checksum ACPI V2.0 */
  if (Rsdp->revision != 0)
    {
      debug ("Checking Extended Checksum");
      Sum = 0;
      for (i = 0; i < Rsdp->length; i++)
	Sum += *(UINT8 *) (Ptr + i);

      if (Sum != 0)
	{
	  warn ("Extended checksum failed for RSDP at addr %p", Ptr);
	  return false;
	}
    }

  return true;
}

/**
  Find RSDP in BIOS memory.

  Searches for ACPI RSDP in standard BIOS locations:
  1. First KB of EBDA (Extended BIOS Data Area)
  2. 0xE0000-0xFFFFF (BIOS ROM area)

  The first megabyte of physical memory is assumed to be mapped at
  memstart.

  @return Pointer to RSDP if found, NULL otherwise.
**/
static VOID *
BiosFindRsdp (
  VOID
  )
{
  unsigned PAddr;
  VOID *MemStart = (VOID *) 0;

  debug ("Searching for RSDP.");

  /* Method 1: Search EBDA's first kb. */
  UINT16 EbdaSeg = *(UINT16 *) (MemStart + EBDA_PTRADDR);
  PAddr = ((UINTN) EbdaSeg << 4);

  if (PAddr <= (1 << 10))
    {
      debug ("EBDA PTR %x not valid.", PAddr);
    }
  else
    {
      VOID *Ptr;

      debug ("Scanning EBDA at addr %x (%p)", PAddr, MemStart + PAddr);
      Ptr = RsdpScan ((UINT8 *) MemStart + PAddr, 1 * KB);
      if (Ptr != NULL)
	return Ptr;
    }

  debug ("Scanning 0xE0000 -> 0xFFFFF");
  return RsdpScan (MemStart + 0xe0000, 0x20000);

}

/**
  Find ACPI RSDP.

  Locates and validates ACPI Root System Description Pointer.

  @return Physical address of RSDP, or 0 if not found.
**/
UINT64
RsdpFind (
  VOID
  )
{
  VOID *Rsdp;

  Rsdp = BiosFindRsdp ();
  if (Rsdp == NULL)
    {
      warn ("RSDP not found.");
      return 0;
    }

  if (!RsdpCheck (Rsdp))
    return 0;

  info ("RSDP found at %p", Rsdp);
  return (UINT64) (UINTN) Rsdp;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use RsdpScan instead **/
static void *rsdp_scan (void *base, size_t size) {
  return RsdpScan (base, size);
}

/** @deprecated Use RsdpCheck instead **/
static bool rsdp_check (void *ptr) {
  return RsdpCheck (ptr);
}

/** @deprecated Use BiosFindRsdp instead **/
static void *bios_find_rsdp (void) {
  return BiosFindRsdp ();
}

/** @deprecated Use RsdpFind instead **/
UINT64 rsdp_find (void) {
  return RsdpFind ();
}
