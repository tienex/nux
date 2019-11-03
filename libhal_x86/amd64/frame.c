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
#include <string.h>
#include <nux/hal.h>

#include "amd64.h"
#include "../internal.h"

struct hal_frame *
do_nmi (uint32_t vect, struct hal_frame *f)
{
  return hal_entry_nmi (f);
}

struct hal_frame *
do_xcpt (uint64_t vect, struct hal_frame *f)
{
  struct hal_frame *rf;

  if (vect == 14)
    {
      unsigned xcpterr;

      /* Page Fault. */

      if (f->err & 1)
	{
	  xcpterr = HAL_PF_REASON_PROT;
	}
      else
	{
	  xcpterr = HAL_PF_REASON_NOTP;
	}

      if (f->err & 2)
	xcpterr |= HAL_PF_INFO_WRITE;

      if (f->err & 4)
	xcpterr |= HAL_PF_INFO_USER;

      rf = hal_entry_pf (f, f->cr2, xcpterr);
    }
  else
    {
      rf = hal_entry_xcpt (f, vect);
    }

  return rf;
}

struct hal_frame *
do_intr (uint32_t vect, struct hal_frame *f)
{

  return hal_entry_vect (f, vect);
}

void
hal_frame_init (struct hal_frame *f)
{
  memset (f, 0, sizeof (*f));
  f->rflags = 0x202;
  f->ss = UDS;
}

bool
hal_frame_isuser (struct hal_frame *f)
{
  return f->cs == UCS;
}

void
hal_frame_setip (struct hal_frame *f, unsigned long ip)
{
  f->rip = ip;
}

void
hal_frame_setsp (struct hal_frame *f, vaddr_t sp)
{
  f->rsp = sp;
}

void
hal_frame_seta0 (struct hal_frame *f, unsigned long a0)
{
  f->rdi = a0;
}

void
hal_frame_seta1 (struct hal_frame *f, unsigned long a1)
{
  f->rsi = a1;
}

void
hal_frame_seta2 (struct hal_frame *f, unsigned long a2)
{
  f->rdx = a2;
}

void
hal_frame_setret (struct hal_frame *f, unsigned long r)
{
  f->rax = r;
}

bool
hal_frame_signal (struct hal_frame *f, unsigned long ip, unsigned long arg)
{
#warning Implement AMD64 signals
  assert (0);
}

void
hal_frame_print (struct hal_frame *f)
{
  hallog ("RAX: %016lx RBX: %016lx\nRCX: %016lx RDX: %016lx",
	  f->rax, f->rbx, f->rcx, f->rdx);
  hallog ("RDI: %016lx RSI: %016lx\nRBP: %016lx RSP: %016lx",
	  f->rdi, f->rsi, f->rbp, f->rsp);
  hallog ("R8 : %016lx R9 : %016lx\nR10: %016lx R11: %016lx",
	  f->r8, f->r9, f->r10, f->r11);
  hallog ("R12: %016lx R13: %016lx\nR14: %016lx R15: %016lx",
	  f->r12, f->r13, f->r14, f->r15);
  hallog (" CS: %04x     EIP: %016lx EFL: %016lx",
	  (int) f->cs, f->rip, f->rflags);
  hallog (" SS: %04x     RSP: %016lx",
	  (int) f->ss, f->rsp);
  hallog ("CR3: %016lx CR2: %016lx err: %08lx", f->cr3, f->cr2, f->err);
}
