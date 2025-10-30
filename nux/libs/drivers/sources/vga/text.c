/** @file
  x86 VGA Text Mode Driver

  Provides VGA text mode output for early boot and console display.
  Implements a simple scrolling text buffer at 0xB8000.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <drivers/internal.h>

#include <string.h>

#define VGA_TEXTSIZE (80*25*2)

/**
  Output a character to VGA text mode display.

  Manages a scrolling 80x25 character display with automatic
  line wrapping and scrolling. VGA memory is accessed through
  the physical memory map.

  @param[in] Ch  Character to output.

  @return The character that was output.
**/
INT32
VgaPutChar (
  IN INT32  Ch
  )
{
  extern unsigned char _physmap_start[];
  const unsigned char *pVgaPtr =
    (const unsigned char *) (_physmap_start + 0xb8000);
  static INT32 Initialized = 0;
  static INT32 X = 0;
  static INT32 Y = 0;

  if (!Initialized)
    {
      INT32 i;
      for (i = 0; i < 80 * 25; i++)
	*(unsigned char *) (pVgaPtr + i * 2) = 0;
      Initialized = 1;
    }

  if (Ch == '\n')
    {
      Y += X / 80 + 1;
      X = 0;
      return Ch;
    }

  if (80 * Y + X >= 80 * 25)
    {
      INT32 i;
      memmove ((void *) pVgaPtr, (void *) pVgaPtr + 80 * 2, 80 * 2 * (25 - 1));
      for (i = 0; i < 80; i++)
	*(unsigned char *) (pVgaPtr + 80 * 2 * (25 - 1) + i * 2) = 0;
      Y = 25 - 1;
      X = 0;
    }

  *(unsigned char *) (pVgaPtr + X++ * 2 + Y * 80 * 2) = Ch;
  return Ch;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use VgaPutChar instead **/
int vga_putchar (int c) {
  return VgaPutChar (c);
}
