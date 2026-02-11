/** @file
  APXH Intel MultiProcessor Specification Discovery

  Implements Intel MP Floating Pointer Structure discovery for Multiboot
  platform. Searches EBDA, BIOS ROM, and other areas for MP signature.

  The Intel MultiProcessor Specification (MPS) is the legacy standard for
  x86 SMP systems that predates ACPI. It defines MP Configuration Tables
  that describe processors, APICs, buses, and interrupt routing.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>

#define MP_SIGNATURE "_MP_"

#define EBDA_PTRADDR 0x40e
#define KB (1 << 10)

/**
  MP Floating Pointer Structure (Intel MPS 1.4 specification).

  This structure contains a pointer to the MP Configuration Table
  and provides basic information about the multiprocessor system.
**/
typedef struct _MP_FLOATING_POINTER
ANX_PACK_PUSH(1)
{
  CHAR8   Signature[4];     ///< "_MP_" signature
  UINT32  PhysAddr;         ///< Physical address of MP Configuration Table
  UINT8   Length;           ///< Length in 16-byte paragraphs (always 1)
  UINT8   SpecRev;          ///< MP Spec revision (1 or 4)
  UINT8   Checksum;         ///< Checksum (all bytes must sum to 0)
  UINT8   Feature1;         ///< MP feature byte 1
  UINT8   Feature2;         ///< MP feature byte 2
  UINT8   Feature3;         ///< MP feature byte 3
  UINT8   Feature4;         ///< MP feature byte 4
  UINT8   Feature5;         ///< MP feature byte 5
} MP_FLOATING_POINTER;
ANX_PACK_POP()

/**
  Scan memory region for MP Floating Pointer signature.

  Searches memory region for Intel MP signature "_MP_" on 16-byte
  paragraph boundaries.

  @param[in] Base  Base address to start scanning.
  @param[in] Size  Size of region to scan.

  @return Pointer to MP Floating Pointer if found, NULL otherwise.
**/
static VOID *
MpScan (
  IN VOID   *Base,
  IN UINTN  Size
  )
{
  VOID *Ptr;

  // MP Floating Pointer must be on 16-byte paragraph boundary
  for (Ptr = Base; Ptr < Base + Size; Ptr += 16)
    {
      if (!memcmp (Ptr, MP_SIGNATURE, 4))
        return Ptr;
    }

  return NULL;
}

/**
  Verify MP Floating Pointer checksum.

  Validates MP Floating Pointer checksum. All bytes in the structure
  must sum to zero.

  @param[in] Ptr  Pointer to MP Floating Pointer structure.

  @retval TRUE   Checksum is valid.
  @retval FALSE  Checksum is invalid.
**/
static BOOLEAN
MpCheck (
  IN VOID  *Ptr
  )
{
  MP_FLOATING_POINTER *Mp = (MP_FLOATING_POINTER *) Ptr;
  INT32 i;
  UINT8 Sum;

  // Verify length (must be 1 paragraph = 16 bytes)
  if (Mp->Length != 1)
    {
      warn ("MP Floating Pointer at %p has invalid length %d", Ptr, Mp->Length);
      return FALSE;
    }

  // Verify specification revision
  if (Mp->SpecRev != 1 && Mp->SpecRev != 4)
    {
      warn ("MP Floating Pointer at %p has unsupported spec revision %d", Ptr, Mp->SpecRev);
      return FALSE;
    }

  // Checksum all 16 bytes
  debug ("Checking MP Floating Pointer checksum");
  Sum = 0;
  for (i = 0; i < 16; i++)
    Sum += *(UINT8 *) (Ptr + i);

  if (Sum != 0)
    {
      warn ("Checksum failed for MP Floating Pointer at %p", Ptr);
      return FALSE;
    }

  debug ("MP Spec Revision: 1.%d", Mp->SpecRev);
  return TRUE;
}

/**
  Find MP Floating Pointer in BIOS memory.

  Searches for Intel MP Floating Pointer Structure in standard locations
  according to Intel MultiProcessor Specification:
  1. First KB of EBDA (Extended BIOS Data Area)
  2. Last KB of base memory (639-640 KB)
  3. 0xF0000-0xFFFFF (BIOS ROM area)

  The first megabyte of physical memory is assumed to be mapped at
  memstart.

  @return Pointer to MP Floating Pointer if found, NULL otherwise.
**/
static VOID *
BiosFindMp (
  VOID
  )
{
  UINT32 PAddr;
  VOID *MemStart = (VOID *) 0;
  VOID *Ptr;

  debug ("Searching for MP Floating Pointer.");

  // Method 1: Search EBDA's first KB
  UINT16 EbdaSeg = *(UINT16 *) (MemStart + EBDA_PTRADDR);
  PAddr = ((UINTN) EbdaSeg << 4);

  if (PAddr > (1 << 10) && PAddr < (640 * KB))
    {
      debug ("Scanning EBDA at addr %x (%p)", PAddr, MemStart + PAddr);
      Ptr = MpScan ((UINT8 *) MemStart + PAddr, 1 * KB);
      if (Ptr != NULL)
        return Ptr;
    }
  else
    {
      debug ("EBDA PTR %x not valid.", PAddr);
    }

  // Method 2: Search last KB of base memory (639-640 KB)
  debug ("Scanning last KB of base memory (639-640 KB)");
  Ptr = MpScan (MemStart + (639 * KB), 1 * KB);
  if (Ptr != NULL)
    return Ptr;

  // Method 3: Search BIOS ROM area 0xF0000-0xFFFFF
  debug ("Scanning BIOS ROM 0xF0000 -> 0xFFFFF");
  return MpScan (MemStart + 0xf0000, 0x10000);
}

/**
  Find Intel MP Floating Pointer Structure.

  Locates and validates Intel MultiProcessor Specification Floating
  Pointer Structure. This is used on legacy x86 SMP systems that
  predate ACPI.

  @return Physical address of MP Floating Pointer, or 0 if not found.
**/
UINT64
MpsFind (
  VOID
  )
{
  VOID *Mp;

  Mp = BiosFindMp ();
  if (Mp == NULL)
    {
      debug ("MP Floating Pointer not found.");
      return 0;
    }

  if (!MpCheck (Mp))
    return 0;

  info ("MP Floating Pointer found at %p (MPS 1.%d)",
        Mp, ((MP_FLOATING_POINTER *)Mp)->SpecRev);
  return (UINT64) (UINTN) Mp;
}
