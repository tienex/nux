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

// Legacy compatibility
#define ACPI_MADT_TYPE_LAPIC         AcpiMadtTypeLapic
#define ACPI_MADT_TYPE_IOAPIC        AcpiMadtTypeIoapic
#define ACPI_MADT_TYPE_INTOVERRIDE   AcpiMadtTypeIntOverride
#define ACPI_MADT_TYPE_NMISRC        AcpiMadtTypeNmiSrc
#define ACPI_MADT_TYPE_LAPICNMI      AcpiMadtTypeLapicNmi
#define ACPI_MADT_TYPE_LAPICOVERRIDE AcpiMadtTypeLapicOverride
#define ACPI_MADT_TYPE_IOSAPIC       AcpiMadtTypeIoSapic
#define ACPI_MADT_TYPE_LSAPIC        AcpiMadtTypeLSapic
#define ACPI_MADT_TYPE_INTSRC        AcpiMadtTypeIntSrc
#define ACPI_MADT_TYPE_LX2APIC       AcpiMadtTypeLx2Apic
#define ACPI_MADT_TYPE_LX2APICNMI    AcpiMadtTypeLx2ApicNmi
#define ACPI_MADT_TYPE_GENINT        AcpiMadtTypeGenInt
#define ACPI_MADT_TYPE_GENDISTR      AcpiMadtTypeGenDistr

typedef struct _ACPI_RSDP_THDR
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
} __packed ACPI_RSDP_THDR;

// Pointer type
typedef ACPI_RSDP_THDR *PACPI_RSDP_THDR;
typedef CONST ACPI_RSDP_THDR *PCACPI_RSDP_THDR;

// Legacy compatibility
#define acpi_rsdp_thdr ACPI_RSDP_THDR

typedef struct _ACPI_THDR
{
  char signature[4];
  UINT32 length;
  UINT8 revision;
  UINT8 checksum;
  char oemid[6];
  char oemtableid[8];
  UINT32 oemrevision;
  UINT32 creatid;
  UINT32 creatrev;
} __packed ACPI_THDR;

// Pointer type
typedef ACPI_THDR *PACPI_THDR;
typedef CONST ACPI_THDR *PCACPI_THDR;

// Legacy compatibility
#define acpi_thdr ACPI_THDR

typedef struct _ACPI_GENADDR
{
  UINT8 spaceid;
  UINT8 bitwidth;
  UINT8 bitoffset;
  UINT8 accesswidth;
  UINT64 address;
} __packed ACPI_GENADDR;

// Pointer type
typedef ACPI_GENADDR *PACPI_GENADDR;
typedef CONST ACPI_GENADDR *PCACPI_GENADDR;

// Legacy compatibility
#define acpi_genaddr ACPI_GENADDR

typedef struct _ACPI_MADT
{
  ACPI_THDR hdr;
  UINT32 lapic;
  UINT32 flags;
} __packed ACPI_MADT;

// Pointer type
typedef ACPI_MADT *PACPI_MADT;
typedef CONST ACPI_MADT *PCACPI_MADT;

// Legacy compatibility
#define acpi_madt ACPI_MADT

//
// ACPI MADT LAPIC Flags
//
typedef enum _ACPI_MADT_LAPIC_FLAGS {
  AcpiMadtLapicEnabled = 0x01
} ACPI_MADT_LAPIC_FLAGS;

// Legacy compatibility
#define ACPI_MADT_LAPIC_ENABLED AcpiMadtLapicEnabled

typedef struct _ACPI_MADT_LAPIC
{
  UINT8 type;
  UINT8 length;
  UINT8 acpiid;
  UINT8 lapicid;
  UINT32 flags;
} __packed ACPI_MADT_LAPIC;

// Pointer type
typedef ACPI_MADT_LAPIC *PACPI_MADT_LAPIC;
typedef CONST ACPI_MADT_LAPIC *PCACPI_MADT_LAPIC;

// Legacy compatibility
#define acpi_madt_lapic ACPI_MADT_LAPIC

typedef struct _ACPI_MADT_IOAPIC
{
  UINT8 type;
  UINT8 length;
  UINT8 ioapicid;
  UINT8 reserved;
  UINT32 address;
  UINT32 gsibase;
} __packed ACPI_MADT_IOAPIC;

// Pointer type
typedef ACPI_MADT_IOAPIC *PACPI_MADT_IOAPIC;
typedef CONST ACPI_MADT_IOAPIC *PCACPI_MADT_IOAPIC;

// Legacy compatibility
#define acpi_madt_ioapic ACPI_MADT_IOAPIC

typedef struct _ACPI_MADT_LAPICOVERRIDE
{
  UINT8 type;
  UINT8 length;
  UINT16 reserved;
  UINT64 address;
} __packed ACPI_MADT_LAPICOVERRIDE;

// Pointer type
typedef ACPI_MADT_LAPICOVERRIDE *PACPI_MADT_LAPICOVERRIDE;
typedef CONST ACPI_MADT_LAPICOVERRIDE *PCACPI_MADT_LAPICOVERRIDE;

// Legacy compatibility
#define acpi_madt_lapicoverride ACPI_MADT_LAPICOVERRIDE

typedef struct _ACPI_MADT_LAPICNMI
{
  UINT8 type;
  UINT8 length;
  UINT8 acpiid;
  UINT16 flags;
  UINT8 lint;
} __packed ACPI_MADT_LAPICNMI;

// Pointer type
typedef ACPI_MADT_LAPICNMI *PACPI_MADT_LAPICNMI;
typedef CONST ACPI_MADT_LAPICNMI *PCACPI_MADT_LAPICNMI;

// Legacy compatibility
#define acpi_madt_lapicnmi ACPI_MADT_LAPICNMI

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

// Legacy compatibility
#define ACPI_MADT_TRIGGER_MASK     AcpiMadtTriggerMask
#define ACPI_MADT_TRIGGER_CONFORMS AcpiMadtTriggerConforms
#define ACPI_MADT_TRIGGER_EDGE     AcpiMadtTriggerEdge
#define ACPI_MADT_TRIGGER_RESERVED AcpiMadtTriggerReserved
#define ACPI_MADT_TRIGGER_LEVEL    AcpiMadtTriggerLevel
#define ACPI_MADT_POLARITY_MASK        AcpiMadtPolarityMask
#define ACPI_MADT_POLARITY_CONFORMS    AcpiMadtPolarityConforms
#define ACPI_MADT_POLARITY_ACTIVE_HIGH AcpiMadtPolarityActiveHigh
#define ACPI_MADT_POLARITY_RESERVED    AcpiMadtPolarityReserved
#define ACPI_MADT_POLARITY_ACTIVE_LOW  AcpiMadtPolarityActiveLow

typedef struct _ACPI_MADT_INTOVERRIDE
{
  UINT8 type;
  UINT8 length;
  UINT8 bus;
  UINT8 irq;
  UINT8 gsi;
  UINT16 flags;
} __packed ACPI_MADT_INTOVERRIDE;

// Pointer type
typedef ACPI_MADT_INTOVERRIDE *PACPI_MADT_INTOVERRIDE;
typedef CONST ACPI_MADT_INTOVERRIDE *PCACPI_MADT_INTOVERRIDE;

// Legacy compatibility
#define acpi_madt_intoverride ACPI_MADT_INTOVERRIDE

typedef struct _ACPI_HPET
{
  ACPI_THDR hdr;
  UINT32 id;
  ACPI_GENADDR address;
  UINT8 sequence;
  UINT8 mintick;
  UINT8 flags;
} __packed ACPI_HPET;

// Pointer type
typedef ACPI_HPET *PACPI_HPET;
typedef CONST ACPI_HPET *PCACPI_HPET;

// Legacy compatibility
#define acpi_hpet ACPI_HPET

#endif
