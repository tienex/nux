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
#include <stdint.h>
#include <nux/hal.h>

extern uint32_t *_gdt;

void
set_tss (struct amd64_tss *tss)
{
  uintptr_t ptr = (uintptr_t)tss;
  uint16_t lo16 = (uint16_t)ptr;
  uint8_t ml8 = (uint8_t)(ptr >> 16);
  uint8_t mh8 = (uint8_t)(ptr >> 24);
  uint32_t hi32 = (uint32_t)(ptr >> 32);
  uint16_t limit = sizeof (*tss);

  _gdt[TSS_GDTIDX + 0] = limit | (lo16 << 16);
  _gdt[TSS_GDTIDX + 1] = ml8 | (0x0089 << 8) | (mh8 << 24);
  _gdt[TSS_GDTIDX + 2] = hi32;
  _gdt[TSS_GDTIDX + 3] = 0;
}

void
amd64_init (void)
{
  
}

int
hal_putchar (int c)
{
  return vga_putchar (c);
}
