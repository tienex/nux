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

#include <stdio.h>
#include <stdlib.h>
#include <nux/hal.h>
#include <nux/nux.h>

#include "internal.h"

int
putchar (int c)
{
  return hal_putchar (c);
}

void __dead
nux_panic (const char *message, struct hal_frame *f)
{
  if (nux_status_setfl (NUXST_PANIC) & NUXST_PANIC)
    {
      /* System was already in panic. Just halt the CPU. */
      hal_cpu_halt ();
      /* Unreachable. */
    }

  /* STOP all CPUs except this one. */
  cpu_nmi_allbutself ();

  hal_panic (cpu_try_id (), message, f);
}

void __dead
abort (void)
{
  while (1)
    hal_debug ();
}

void __dead
exit (int status)
{
  if (status == EXIT_HALT)
    {
      hal_cpu_halt ();
    }
  else if (status == EXIT_IDLE)
    {
      cpu_idle ();
    }


  abort ();
}
