/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  See COPYING file for the full license.

  SPDX-License-Identifier:	GPL2.0+
*/

#include <stddef.h>
#include <nux/hal.h>
#include <nux/nux.h>
#include <nux/apxh.h>

#include "internal.h"

#define PLTACPI_INVALID_IRQ ((unsigned)-1)
unsigned pltacpi_hpet_irq = PLTACPI_INVALID_IRQ;

void
plt_init (void)
{
  const struct apxh_pltdesc *desc;

  desc = hal_pltinfo ();
  if (desc == NULL)
    fatal ("Invalid PLT Boot Table.");

  if (desc->type != PLT_ACPI)
    fatal ("No ACPI RSDP found.");

  printf ("RSDP: %llx\n", desc->pltptr);

  acpi_init (desc->pltptr);
  acpi_madt_scan ();
  gsi_start ();

  acpi_hpet_scan ();
}

void
plt_hw_putc (int c)
{
  /* Only HAL putc for now. */
  hal_putchar (c);
}

/*
  Translate an external vector into an IRQ or entry type.
*/

void
plt_vect_translate (unsigned vect, struct plt_vect_desc *desc)
{
  gsi_translate (vect, desc);

  if ((desc->type == PLT_VECT_IRQ) && (desc->no == pltacpi_hpet_irq))
    {
      /* Is an HPET IRQ. */
      hpet_doirq ();
      /* Make it a timer interrupt. */
      desc->type = PLT_VECT_TMR;
      desc->no = 0;
    }
}
