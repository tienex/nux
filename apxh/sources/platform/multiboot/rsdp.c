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

typedef struct _ACPI_RSDP_THDR
{
  CHAR8 Signature[8];
  UINT8 Checksum;
  CHAR8 OemId[6];
  UINT8 Revision;
  UINT32 Rsdt;

  /* ACPI >= 2.0 (revision != 0) */
  UINT32 Length;
  UINT64 Xsdt;
  UINT8 XChecksum;
  UINT8 Reserved[3];
} ANX_PACKED ACPI_RSDP_THDR;


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
  IN UINTN  Size
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
static BOOLEAN
RsdpCheck (
  IN VOID  *Ptr
  )
{
  ACPI_RSDP_THDR *Rsdp = (ACPI_RSDP_THDR *) Ptr;
  INT32 i;
  UINT8 Sum;

  /* Checksum ACPI V1.0 */
  debug ("Checking Checksum");
  Sum = 0;
  for (i = 0; i < 20; i++)
    Sum += *(UINT8 *) (Ptr + i);

  if (Sum != 0)
    {
      warn ("Checksum failed for RSDP at addr %p", Ptr);
      return FALSE;
    }

  debug ("Revision: %d", Rsdp->Revision);
  /* Checksum ACPI V2.0 */
  if (Rsdp->Revision != 0)
    {
      debug ("Checking Extended Checksum");
      Sum = 0;
      for (i = 0; i < Rsdp->Length; i++)
	Sum += *(UINT8 *) (Ptr + i);

      if (Sum != 0)
	{
	  warn ("Extended checksum failed for RSDP at addr %p", Ptr);
	  return FALSE;
	}
    }

  return TRUE;
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
  UINT32 PAddr;
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
