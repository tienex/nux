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

extern uint32_t *_gdt;
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

  _gdt[TSS_GDTIDX + 0] = limit | (lo16 << 16);
  _gdt[TSS_GDTIDX + 1] = ml8 | (0x0089 << 8) | (mh8 << 24);
  _gdt[TSS_GDTIDX + 2] = hi32;
  _gdt[TSS_GDTIDX + 3] = 0;
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
hal_frame_init (struct hal_frame *f)
{
}

bool
hal_frame_isuser (struct hal_frame *f)
{
}

void
hal_frame_setip (struct hal_frame *f, unsigned long ip)
{
}

unsigned long
hal_frame_getip (struct hal_frame *f)
{
}

void
hal_frame_setsp (struct hal_frame *f, unsigned long ip)
{
}

unsigned long
hal_frame_getsp (struct hal_frame *f)
{
}

void
hal_frame_seta0 (struct hal_frame *f, unsigned long a0)
{
}

void
hal_frame_seta1 (struct hal_frame *f, unsigned long a1)
{
}

void
hal_frame_seta2 (struct hal_frame *f, unsigned long a2)
{
}

void
hal_frame_setret (struct hal_frame *f, unsigned long r)
{
}

void
hal_frame_print (struct hal_frame *f)
{
}

bool
hal_frame_signal (struct hal_frame *f, unsigned long ip, unsigned long arg)
{
}

void
amd64_init (void)
{

}
