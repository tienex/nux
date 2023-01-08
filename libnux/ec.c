/*
 * NUX: A kernel Library. Copyright (C) 2019 Gianluca Guida,
 * glguida@tlbflush.org
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * See COPYING file for the full license.
 * 
 * SPDX-License-Identifier:	GPL2.0+
 */

#include <stdio.h>
#include <stdlib.h>
#include <nux/hal.h>
#include <nux/nux.h>

int
putchar (int c)
{
  return hal_putchar (c);
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
  else
    {
      hal_cpu_halt ();
      /* Full-blown panic */
    }


  hal_cpu_halt ();
}
