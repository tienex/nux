/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"

#include <string.h>

#define VGA_TEXTSIZE (80*25*2)

int
vga_putchar (int c)
{
  extern unsigned char _physmap_start[];
  const unsigned char *vptr =
    (const unsigned char *) (_physmap_start + 0xb8000);
  static int init = 0;
  static int x = 0;
  static int y = 0;

  if (!init)
    {
      int i;
      for (i = 0; i < 80 * 25; i++)
	*(unsigned char *) (vptr + i * 2) = 0;
      init = 1;
    }

  if (c == '\n')
    {
      y += x / 80 + 1;
      x = 0;
      return c;
    }

  if (80 * y + x >= 80 * 25)
    {
      int i;
      memmove ((void *) vptr, (void *) vptr + 80 * 2, 80 * 2 * (25 - 1));
      for (i = 0; i < 80; i++)
	*(unsigned char *) (vptr + 80 * 2 * (25 - 1) + i * 2) = 0;
      y = 25 - 1;
      x = 0;
    }

  *(unsigned char *) (vptr + x++ * 2 + y * 80 * 2) = c;
  return c;
}
