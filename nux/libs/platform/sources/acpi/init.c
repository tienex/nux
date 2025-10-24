/** @file
  ACPI Platform Initialization and Interrupt Handling

  Provides platform initialization using ACPI tables and interrupt
  routing through APIC. Initializes ACPI, MADT, GSI, and HPET.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <stddef.h>
#include <hal.h>
#include <nux.h>
#include <apxh.h>

#include "platform/acpi/internal.h"

#define PLTACPI_INVALID_IRQ ((UINT32)-1)
UINT32 gPltAcpiHpetIrq = PLTACPI_INVALID_IRQ;

/**
  Initialize ACPI platform.

  Obtains ACPI RSDP pointer from bootloader, initializes ACPI
  subsystem, scans MADT for interrupt configuration, and
  initializes HPET timer if available.
**/
VOID
PltInitialize (
  VOID
  )
{
  CONST struct apxh_pltdesc *pDesc;

  pDesc = hal_pltinfo ();
  if (pDesc == NULL)
    fatal ("Invalid PLT Boot Table.");

  if (pDesc->type != PLT_ACPI)
    fatal ("No ACPI RSDP found.");

  printf ("RSDP: %llx\n", pDesc->pltptr);

  AcpiInitialize (pDesc->pltptr);
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
PltHwPutChar (
  IN INT32  Char
  )
{
  /* Only HAL putc for now. */
  hal_putchar (Char);
}


#include "apic.h"

/**
  Handle platform interrupt.

  Routes interrupts to appropriate handlers based on vector number.
  Handles IPIs, IRQs, and HPET timer interrupts.

  @param[in] Vector  Interrupt vector number.
  @param[in] pFrame  HAL frame at interrupt.

  @return Updated frame pointer.
**/
struct hal_frame *
PltInterrupt (
  IN UINT32            Vector,
  IN struct hal_frame  *pFrame
  )
{
  struct hal_frame *pResult;

  if (Vector >= APIC_VECT_MAX)
    {
      /* Something wrong here. */
      warn ("HAL vector %d outside of bounds.", Vector);
      pResult = pFrame;
    }
  else if (Vector >= APIC_VECT_IPIBASE)
    {
      pResult = hal_entry_ipi (pFrame);
    }
  else if (Vector >= APIC_VECT_IRQBASE)
    {
      UINT32 Irq = Vector - APIC_VECT_IRQBASE;
      if (Irq == gPltAcpiHpetIrq)
	{
	  HpetDoIrq ();
	  pResult = hal_entry_timer (pFrame);
	}
      else
	{
	  pResult = hal_entry_irq (pFrame, Irq, PltIrqIsLevel (Irq));
	}
    }
  else
    {
      /* Something wrong here. */
      warn ("HAL vector %d outside of bounds", Vector);
      pResult = pFrame;
    }

  return pResult;
}

/**
  Send end-of-interrupt for IPI.

  Signals completion of IPI handling to local APIC.
**/
VOID
PltEoiIpi (
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
PltEoiIrq (
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
PltEoiTimer (
  VOID
  )
{
  LapicEoi ();
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use PltInitialize instead **/
void plt_init (void) {
  PltInitialize ();
}

/** @deprecated Use PltHwPutChar instead **/
void plt_hw_putc (int c) {
  PltHwPutChar (c);
}

/** @deprecated Use PltInterrupt instead **/
struct hal_frame *plt_interrupt (unsigned vect, struct hal_frame *f) {
  return PltInterrupt (vect, f);
}

/** @deprecated Use PltEoiIpi instead **/
void plt_eoi_ipi (void) {
  PltEoiIpi ();
}

/** @deprecated Use PltEoiIrq instead **/
void plt_eoi_irq (unsigned irq) {
  PltEoiIrq (irq);
}

/** @deprecated Use PltEoiTimer instead **/
void plt_eoi_timer (void) {
  PltEoiTimer ();
}

// Legacy global variable alias
unsigned pltacpi_hpet_irq __attribute__((alias("gPltAcpiHpetIrq")));
