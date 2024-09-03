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

#include "internal.h"

#define SERIAL_PORT 0x3f8

void
serial_init (void)
{
  outb (SERIAL_PORT + 1, 0);
  outb (SERIAL_PORT + 3, 0x80);
  outb (SERIAL_PORT + 0, 3);
  outb (SERIAL_PORT + 1, 0);
  outb (SERIAL_PORT + 3, 3);
  outb (SERIAL_PORT + 2, 0xc7);
  outb (SERIAL_PORT + 4, 0xb);
}

void
serial_putchar (int c)
{
  while (!(inb (SERIAL_PORT + 5) & 0x20));
  outb (SERIAL_PORT, c);
}
