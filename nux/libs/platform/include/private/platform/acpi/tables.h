/** @file
  ACPI Table Structures

  Definitions for ACPI table structures including RSDP, MADT,
  and HPET table formats.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __platform_acpi_tables_h__
#define __platform_acpi_tables_h__

#include <cdefs.h>
#include <stdint.h>

//
// ACPI MADT Entry Types
//
typedef enum _ACPI_MADT_ENTRY_TYPE {
  AcpiMadtTypeLapic         = 0,
  AcpiMadtTypeIoapic        = 1,
  AcpiMadtTypeIntOverride   = 2,
  AcpiMadtTypeNmiSrc        = 3,
  AcpiMadtTypeLapicNmi      = 4,
  AcpiMadtTypeLapicOverride = 5,
  AcpiMadtTypeIoSapic       = 6,
  AcpiMadtTypeLSapic        = 7,
  AcpiMadtTypeIntSrc        = 8,
  AcpiMadtTypeLx2Apic       = 9,
  AcpiMadtTypeLx2ApicNmi    = 10,
  AcpiMadtTypeGenInt        = 11,
  AcpiMadtTypeGenDistr      = 12
} ACPI_MADT_ENTRY_TYPE;


typedef struct _ACPI_THDR
{
  CHAR8 Signature[4];
  UINT32 Length;
  UINT8 Revision;
  UINT8 Checksum;
  CHAR8 OemId[6];
  CHAR8 OemTableId[8];
  UINT32 OemRevision;
  UINT32 CreatorId;
  UINT32 CreatorRevision;
} __packed ACPI_THDR;

// Pointer type
typedef ACPI_THDR *PACPI_THDR;
typedef CONST ACPI_THDR *PCACPI_THDR;


typedef struct _ACPI_GENADDR
{
  UINT8 SpaceId;
  UINT8 BitWidth;
  UINT8 BitOffset;
  UINT8 AccessWidth;
  UINT64 Address;
} __packed ACPI_GENADDR;

// Pointer type
typedef ACPI_GENADDR *PACPI_GENADDR;
typedef CONST ACPI_GENADDR *PCACPI_GENADDR;

typedef struct _ACPI_MADT
{
  ACPI_THDR Hdr;
  UINT32 Lapic;
  UINT32 Flags;
} __packed ACPI_MADT;

// Pointer type
typedef ACPI_MADT *PACPI_MADT;
typedef CONST ACPI_MADT *PCACPI_MADT;


//
// ACPI MADT LAPIC Flags
//
typedef enum _ACPI_MADT_LAPIC_FLAGS {
  AcpiMadtLapicEnabled = 0x01
} ACPI_MADT_LAPIC_FLAGS;


typedef struct _ACPI_MADT_IOAPIC
{
  UINT8 Type;
  UINT8 Length;
  UINT8 IoApicId;
  UINT8 Reserved;
  UINT32 Address;
  UINT32 GsiBase;
} __packed ACPI_MADT_IOAPIC;

// Pointer type
typedef ACPI_MADT_IOAPIC *PACPI_MADT_IOAPIC;
typedef CONST ACPI_MADT_IOAPIC *PCACPI_MADT_IOAPIC;


typedef struct _ACPI_MADT_LAPICOVERRIDE
{
  UINT8 Type;
  UINT8 Length;
  UINT16 Reserved;
  UINT64 Address;
} __packed ACPI_MADT_LAPICOVERRIDE;

// Pointer type
typedef ACPI_MADT_LAPICOVERRIDE *PACPI_MADT_LAPICOVERRIDE;
typedef CONST ACPI_MADT_LAPICOVERRIDE *PCACPI_MADT_LAPICOVERRIDE;


typedef struct _ACPI_MADT_LAPICNMI
{
  UINT8 Type;
  UINT8 Length;
  UINT8 AcpiId;
  UINT16 Flags;
  UINT8 Lint;
} __packed ACPI_MADT_LAPICNMI;

// Pointer type
typedef ACPI_MADT_LAPICNMI *PACPI_MADT_LAPICNMI;
typedef CONST ACPI_MADT_LAPICNMI *PCACPI_MADT_LAPICNMI;


//
// ACPI MADT Interrupt Override Trigger Modes
//
typedef enum _ACPI_MADT_TRIGGER_MODE {
  AcpiMadtTriggerConforms = 0x00,
  AcpiMadtTriggerEdge     = 0x04,
  AcpiMadtTriggerReserved = 0x08,
  AcpiMadtTriggerLevel    = 0x0C,
  AcpiMadtTriggerMask     = 0x0C
} ACPI_MADT_TRIGGER_MODE;

//
// ACPI MADT Interrupt Override Polarity
//
typedef enum _ACPI_MADT_POLARITY {
  AcpiMadtPolarityConforms    = 0x00,
  AcpiMadtPolarityActiveHigh  = 0x01,
  AcpiMadtPolarityReserved    = 0x02,
  AcpiMadtPolarityActiveLow   = 0x03,
  AcpiMadtPolarityMask        = 0x03
} ACPI_MADT_POLARITY;


typedef struct _ACPI_HPET
{
  ACPI_THDR Hdr;
  UINT32 Id;
  ACPI_GENADDR Address;
  UINT8 Sequence;
  UINT8 MinTick;
  UINT8 Flags;
} __packed ACPI_HPET;

// Pointer type
typedef ACPI_HPET *PACPI_HPET;
typedef CONST ACPI_HPET *PCACPI_HPET;


#endif
