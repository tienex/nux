/** @file
  APXH ACPI RSDP Discovery

  Implements ACPI Root System Description Pointer (RSDP) discovery for
  Multiboot platform. Searches EBDA and BIOS ROM areas for RSDP signature.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
**/

#include <apxh/project.h>

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


/**
  Scan memory region for RSDP signature.

  Searches memory region for ACPI RSDP signature "RSD PTR ".

  @param[in] pBase  Base address to start scanning.
  @param[in] Size   Size of region to scan.

  @return Pointer to RSDP if found, NULL otherwise.
**/
static VOID *
RsdpScan (
  IN VOID    *pBase,
  IN size_t  Size
  )
{
  VOID *pPtr;

  for (pPtr = (UINT64 *) pBase; pPtr < pBase + Size; pPtr++)
    {
      if (!memcmp (pPtr, RSDP_SIGN, 8))
	return pPtr;
    }

  return NULL;
}

/**
  Verify RSDP checksum.

  Validates RSDP checksum for both ACPI 1.0 and 2.0+ versions.

  @param[in] pPtr  Pointer to RSDP structure.

  @retval TRUE   Checksum is valid.
  @retval FALSE  Checksum is invalid.
**/
static bool
RsdpCheck (
  IN VOID  *pPtr
  )
{
  struct acpi_rsdp_thdr *pRsdp = (struct acpi_rsdp_thdr *) pPtr;
  int i;
  UINT8 Sum;

  /* Checksum ACPI V1.0 */
  debug ("Checking Checksum");
  Sum = 0;
  for (i = 0; i < 20; i++)
    Sum += *(UINT8 *) (pPtr + i);

  if (Sum != 0)
    {
      warn ("Checksum failed for RSDP at addr %p", pPtr);
      return false;
    }

  debug ("Revision: %d", pRsdp->revision);
  /* Checksum ACPI V2.0 */
  if (pRsdp->revision != 0)
    {
      debug ("Checking Extended Checksum");
      Sum = 0;
      for (i = 0; i < pRsdp->length; i++)
	Sum += *(UINT8 *) (pPtr + i);

      if (Sum != 0)
	{
	  warn ("Extended checksum failed for RSDP at addr %p", pPtr);
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
  VOID *pMemStart = (VOID *) 0;

  debug ("Searching for RSDP.");

  /* Method 1: Search EBDA's first kb. */
  UINT16 EbdaSeg = *(UINT16 *) (pMemStart + EBDA_PTRADDR);
  PAddr = ((uintptr_t) EbdaSeg << 4);

  if (PAddr <= (1 << 10))
    {
      debug ("EBDA PTR %x not valid.", PAddr);
    }
  else
    {
      VOID *pPtr;

      debug ("Scanning EBDA at addr %x (%p)", PAddr, pMemStart + PAddr);
      pPtr = RsdpScan ((UINT8 *) pMemStart + PAddr, 1 * KB);
      if (pPtr != NULL)
	return pPtr;
    }

  debug ("Scanning 0xE0000 -> 0xFFFFF");
  return RsdpScan (pMemStart + 0xe0000, 0x20000);

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
  VOID *pRsdp;

  pRsdp = BiosFindRsdp ();
  if (pRsdp == NULL)
    {
      warn ("RSDP not found.");
      return 0;
    }

  if (!RsdpCheck (pRsdp))
    return 0;

  info ("RSDP found at %p", pRsdp);
  return (UINT64) (uintptr_t) pRsdp;
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
uint64_t rsdp_find (void) {
  return RsdpFind ();
}
