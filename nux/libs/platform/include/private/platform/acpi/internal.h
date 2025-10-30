/** @file
  Platform ACPI Internal Definitions

  Internal function declarations and definitions for ACPI platform
  layer including LAPIC, IOAPIC, GSI, HPET, and hardware control.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __platform_acpi_internal_h__
#define __platform_acpi_internal_h__

#include <stdint.h>
#include <platform/platform.h>

// Platform ACPI HPET IRQ
extern UINT32 gPlatformAcpiHpetIrq;

// Legacy compatibility alias
extern unsigned pltacpi_hpet_irq;

//
// LAPIC (Local APIC) Functions
//

VOID LapicInit (IN UINT64 Base, IN UINT32 Count);
VOID LapicAdd (IN UINT16 PlatformId, IN UINT16 PhysId);
VOID LapicAddNmi (IN UINT8 Cpu, IN INT32 Lint);
VOID LapicEoi (VOID);

//
// IOAPIC Functions
//

VOID IoapicInit (IN UINT32 Count);
VOID IoapicAdd (IN UINT32 Num, IN UINT64 Base, IN UINT32 IrqBase);

//
// GSI (Global System Interrupt) Functions
//

VOID GsiInit (VOID);
VOID GsiSetup (IN UINT32 Gsi, IN UINT32 Irq, IN PLATFORM_IRQ_TYPE Mode);
VOID GsiStart (VOID);

//
// ACPI Functions
//

VOID AcpiInitialize (IN PHYSICAL_ADDRESS Rsdp);
VOID AcpiMadtScan (VOID);

//
// Hardware Control Functions
//

VOID HwCmosWrite (IN UINT8 Addr, IN UINT8 Val);
VOID HwDelay (VOID);
VOID HwResetVector (IN UINT32 Start);

//
// HPET Functions
//

BOOLEAN HpetInit (IN PHYSICAL_ADDRESS HpetPa);
VOID HpetDoIrq (VOID);
BOOLEAN AcpiHpetScan (VOID);

#endif
