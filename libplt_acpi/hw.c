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

#define halerror prin
#include <stdint.h>
#include <nux/types.h>
#include <nux/hal.h>
#include <nux/nux.h>

#include "internal.h"

void
hw_cmos_write (uint8_t addr, uint8_t val)
{
  hal_cpu_out (1, 0x70, addr);
  hal_cpu_out (1, 0x71, val);
}

void
hw_delay (void)
{
  int i;
  for (i = 0; i < 1000; i++)
    hal_cpu_relax ();
}
