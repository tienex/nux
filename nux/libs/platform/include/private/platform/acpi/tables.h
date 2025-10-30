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

#define ACPI_MADT_TYPE_LAPIC 0
#define ACPI_MADT_TYPE_IOAPIC 1
#define ACPI_MADT_TYPE_INTOVERRIDE 2
#define ACPI_MADT_TYPE_NMISRC 3
#define ACPI_MADT_TYPE_LAPICNMI 4
#define ACPI_MADT_TYPE_LAPICOVERRIDE 5
#define ACPI_MADT_TYPE_IOSAPIC 6
#define ACPI_MADT_TYPE_LSAPIC 7
#define ACPI_MADT_TYPE_INTSRC 8
#define ACPI_MADT_TYPE_LX2APIC 9
#define ACPI_MADT_TYPE_LX2APICNMI 10
#define ACPI_MADT_TYPE_GENINT 11
#define ACPI_MADT_TYPE_GENDISTR 12

typedef struct _ACPI_RSDP_THDR
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
} __packed ACPI_RSDP_THDR;

// Legacy compatibility
#define acpi_rsdp_thdr ACPI_RSDP_THDR

typedef struct _ACPI_THDR
{
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oemid[6];
  char oemtableid[8];
  uint32_t oemrevision;
  uint32_t creatid;
  uint32_t creatrev;
} __packed ACPI_THDR;

// Legacy compatibility
#define acpi_thdr ACPI_THDR

typedef struct _ACPI_GENADDR
{
  uint8_t spaceid;
  uint8_t bitwidth;
  uint8_t bitoffset;
  uint8_t accesswidth;
  uint64_t address;
} __packed ACPI_GENADDR;

// Legacy compatibility
#define acpi_genaddr ACPI_GENADDR

typedef struct _ACPI_MADT
{
  ACPI_THDR hdr;
  uint32_t lapic;
  uint32_t flags;
} __packed ACPI_MADT;

// Legacy compatibility
#define acpi_madt ACPI_MADT

typedef struct _ACPI_MADT_LAPIC
{
  uint8_t type;
  uint8_t length;
  uint8_t acpiid;
  uint8_t lapicid;
#define ACPI_MADT_LAPIC_ENABLED 1
  uint32_t flags;
} __packed ACPI_MADT_LAPIC;

// Legacy compatibility
#define acpi_madt_lapic ACPI_MADT_LAPIC

typedef struct _ACPI_MADT_IOAPIC
{
  uint8_t type;
  uint8_t length;
  uint8_t ioapicid;
  uint8_t reserved;
  uint32_t address;
  uint32_t gsibase;
} __packed ACPI_MADT_IOAPIC;

// Legacy compatibility
#define acpi_madt_ioapic ACPI_MADT_IOAPIC

typedef struct _ACPI_MADT_LAPICOVERRIDE
{
  uint8_t type;
  uint8_t length;
  uint16_t reserved;
  uint64_t address;
} __packed ACPI_MADT_LAPICOVERRIDE;

// Legacy compatibility
#define acpi_madt_lapicoverride ACPI_MADT_LAPICOVERRIDE

typedef struct _ACPI_MADT_LAPICNMI
{
  uint8_t type;
  uint8_t length;
  uint8_t acpiid;
  uint16_t flags;
  uint8_t lint;
} __packed ACPI_MADT_LAPICNMI;

// Legacy compatibility
#define acpi_madt_lapicnmi ACPI_MADT_LAPICNMI

typedef struct _ACPI_MADT_INTOVERRIDE
{
  uint8_t type;
  uint8_t length;
  uint8_t bus;
  uint8_t irq;
  uint8_t gsi;
#define ACPI_MADT_TRIGGER_MASK     0x0C
#define ACPI_MADT_TRIGGER_CONFORMS 0x00
#define ACPI_MADT_TRIGGER_EDGE     0x04
#define ACPI_MADT_TRIGGER_RESERVED 0x08
#define ACPI_MADT_TRIGGER_LEVEL    0x0C
#define ACPI_MADT_POLARITY_MASK        0x03
#define ACPI_MADT_POLARITY_CONFORMS    0x00
#define ACPI_MADT_POLARITY_ACTIVE_HIGH 0x01
#define ACPI_MADT_POLARITY_RESERVED    0x02
#define ACPI_MADT_POLARITY_ACTIVE_LOW  0x03
  uint16_t flags;
} __packed ACPI_MADT_INTOVERRIDE;

// Legacy compatibility
#define acpi_madt_intoverride ACPI_MADT_INTOVERRIDE

typedef struct _ACPI_HPET
{
  ACPI_THDR hdr;
  uint32_t id;
  ACPI_GENADDR address;
  uint8_t sequence;
  uint8_t mintick;
  uint8_t flags;
} __packed ACPI_HPET;

// Legacy compatibility
#define acpi_hpet ACPI_HPET

#endif
