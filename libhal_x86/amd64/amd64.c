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


#include <assert.h>
#include <stdint.h>
#include <nux/hal.h>
#include "amd64.h"
#include "../internal.h"

extern uint64_t *_gdt;
extern int _physmap_start;
extern int _physmap_end;

void
set_tss (struct amd64_tss *tss)
{
  uintptr_t ptr = (uintptr_t)tss;
  uint16_t lo16 = (uint16_t)ptr;
  uint8_t ml8 = (uint8_t)(ptr >> 16);
  uint8_t mh8 = (uint8_t)(ptr >> 24);
  uint32_t hi32 = (uint32_t)(ptr >> 32);
  uint16_t limit = sizeof (*tss);
  uint32_t *ptr32 = (uint32_t *)(_gdt + TSS_GDTIDX);

  ptr32[0] = limit | (lo16 << 16);
  ptr32[1] = ml8 | (0x0089 << 8) | (mh8 << 24);
  ptr32[2] = hi32;
  ptr32[3] = 0;
}


void
hal_cpu_setdata (void *data)
{
}

void *
hal_cpu_getdata (void)
{
  return NULL;
}

unsigned
hal_vect_irqbase (void)
{
  return 33;
}

unsigned
hal_vect_ipibase (void)
{
  return 34;
}

unsigned
hal_vect_max (void)
{
  return 255;
}

void
hal_pcpu_init (unsigned pcpuid, struct hal_cpu *haldata)
{
}

void
hal_pcpu_enter (unsigned pcpuid)
{
}

uint64_t
hal_pcpu_prepare (unsigned pcpu)
{
}

void
amd64_init (void)
{

}
