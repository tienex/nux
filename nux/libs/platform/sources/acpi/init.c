/** @file
  ACPI Platform Initialization and Interrupt Handling

  Provides platform initialization using ACPI tables and interrupt
  routing through APIC. Initializes ACPI, MADT, GSI, and HPET.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <stddef.h>
#include <hal/hal.h>
#include <nux/nux.h>
#include <apxh/apxh.h>

#include <platform/acpi/internal.h>

#define PLATFORMACPI_INVALID_IRQ ((UINT32)-1)
UINT32 gPlatformAcpiHpetIrq = PLATFORMACPI_INVALID_IRQ;

/**
  Initialize ACPI platform.

  Obtains ACPI RSDP pointer from bootloader, initializes ACPI
  subsystem, scans MADT for interrupt configuration, and
  initializes HPET timer if available.
**/
VOID
PlatformInitialize (
  VOID
  )
{
  CONST struct apxh_platformdesc *Desc;

  Desc = hal_pltinfo ();
  if (Desc == NULL)
    fatal ("Invalid Platform Boot Table.");

  if (Desc->Type != ApxhPlatformAcpi)
    fatal ("No ACPI RSDP found.");

  printf ("RSDP: %llx\n", Desc->PlatformPointer);

  AcpiInitialize (Desc->PlatformPointer);
  AcpiMadtScan ();
  GsiStart ();

  AcpiHpetScan ();
}

/**
  Output character to platform console.

  Currently delegates to HAL character output.

  @param[in] Char  Character to output.
**/
VOID
PlatformHwPutChar (
  IN INT32  Char
  )
{
  /* Only HAL putc for now. */
  hal_putchar (Char);
}


#include <platform/acpi/apic.h>

/**
  Handle platform interrupt.

  Routes interrupts to appropriate handlers based on vector number.
  Handles IPIs, IRQs, and HPET timer interrupts.

  @param[in] Vector  Interrupt vector number.
  @param[in] Frame  HAL frame at interrupt.

  @return Updated frame pointer.
**/
struct hal_frame *
PlatformInterrupt (
  IN UINT32            Vector,
  IN struct hal_frame  *Frame
  )
{
  struct hal_frame *Result;

  if (Vector >= APIC_VECT_MAX)
    {
      /* Something wrong here. */
      warn ("HAL vector %d outside of bounds.", Vector);
      Result = Frame;
    }
  else if (Vector >= APIC_VECT_IPIBASE)
    {
      Result = hal_entry_ipi (Frame);
    }
  else if (Vector >= APIC_VECT_IRQBASE)
    {
      UINT32 Irq = Vector - APIC_VECT_IRQBASE;
      if (Irq == gPlatformAcpiHpetIrq)
	{
	  HpetDoIrq ();
	  Result = hal_entry_timer (Frame);
	}
      else
	{
	  Result = hal_entry_irq (Frame, Irq, PlatformIrqIsLevel (Irq));
	}
    }
  else
    {
      /* Something wrong here. */
      warn ("HAL vector %d outside of bounds", Vector);
      Result = Frame;
    }

  return Result;
}

/**
  Send end-of-interrupt for IPI.

  Signals completion of IPI handling to local APIC.
**/
VOID
PlatformEoiIpi (
  VOID
  )
{
  LapicEoi ();
}

/**
  Send end-of-interrupt for IRQ.

  Signals completion of IRQ handling to local APIC.

  @param[in] Irq  IRQ number.
**/
VOID
PlatformEoiIrq (
  IN UINT32  Irq
  )
{
  LapicEoi ();
}

/**
  Send end-of-interrupt for timer.

  Signals completion of timer interrupt handling to local APIC.
**/
VOID
PlatformEoiTimer (
  VOID
  )
{
  LapicEoi ();
}
