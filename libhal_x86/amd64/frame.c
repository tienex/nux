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
do_nmi (struct hal_frame *f)
{
  hal_entry_nmi (f);
  /* NMI cannot switch current frame. */
  return f;
}

struct hal_frame *
do_xcpt (uint64_t vect, struct hal_frame *f)
{
  struct hal_frame *rf;

  if (vect == 14)
    {
      unsigned xcpterr;

      /* Page Fault. */

      if (f->intr.err & 1)
	{
	  xcpterr = HAL_PF_REASON_PROT;
	}
      else
	{
	  xcpterr = HAL_PF_REASON_NOTP;
	}

      if (f->intr.err & 2)
	xcpterr |= HAL_PF_INFO_WRITE;

      if (f->intr.err & 4)
	xcpterr |= HAL_PF_INFO_USER;

      rf = hal_entry_pf (f, f->intr.cr2, xcpterr);
    }
  else
    {
      rf = hal_entry_xcpt (f, vect);
    }

  return rf;
}

struct hal_frame *
do_intr (uint64_t vect, struct hal_frame *f)
{
  return hal_entry_vect (f, vect);
}

struct hal_frame *
do_intr_entry (struct hal_frame *f)
{
  uint64_t vect;
  struct hal_frame *rf;

  assert (f->type == FRAMETYPE_INTR);

  vect = f->intr.vect;

  if (vect > 32)
    rf = do_intr (vect, f);
  else if (vect == 2)
    rf = do_nmi (f);
  else
    rf = do_xcpt (vect, f);

  return rf;
}

void
hal_frame_init (struct hal_frame *f)
{
  memset (f, 0, sizeof (*f));
  f->type = FRAMETYPE_INTR;
  f->intr.cs = UCS;
  f->intr.rflags = 0x82;
  f->intr.ss = UDS;
}

bool
hal_frame_isuser (struct hal_frame *f)
{
  assert (f->type == FRAMETYPE_INTR);
  return f->intr.cs == UCS;
}

void
hal_frame_setip (struct hal_frame *f, unsigned long ip)
{
  assert (f->type == FRAMETYPE_INTR);
  f->intr.rip = ip;
}

void
hal_frame_setsp (struct hal_frame *f, vaddr_t sp)
{
  assert (f->type == FRAMETYPE_INTR);
  f->intr.rsp = sp;
}

void
hal_frame_seta0 (struct hal_frame *f, unsigned long a0)
{
  assert (f->type == FRAMETYPE_INTR);
  f->intr.rdi = a0;
}

void
hal_frame_seta1 (struct hal_frame *f, unsigned long a1)
{
  assert (f->type == FRAMETYPE_INTR);
  f->intr.rsi = a1;
}

void
hal_frame_seta2 (struct hal_frame *f, unsigned long a2)
{
  assert (f->type == FRAMETYPE_INTR);
  f->intr.rdx = a2;
}

void
hal_frame_setret (struct hal_frame *f, unsigned long r)
{
  assert (f->type == FRAMETYPE_INTR);
  f->intr.rax = r;
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
  assert (f->type == FRAMETYPE_INTR);

  hallog ("RAX: %016lx RBX: %016lx\nRCX: %016lx RDX: %016lx",
	  f->intr.rax, f->intr.rbx, f->intr.rcx, f->intr.rdx);
  hallog ("RDI: %016lx RSI: %016lx\nRBP: %016lx RSP: %016lx",
	  f->intr.rdi, f->intr.rsi, f->intr.rbp, f->intr.rsp);
  hallog ("R8 : %016lx R9 : %016lx\nR10: %016lx R11: %016lx",
	  f->intr.r8, f->intr.r9, f->intr.r10, f->intr.r11);
  hallog ("R12: %016lx R13: %016lx\nR14: %016lx R15: %016lx",
	  f->intr.r12, f->intr.r13, f->intr.r14, f->intr.r15);
  hallog ("GSBASE: %016lx\n", f->intr.gsbase);
  hallog (" CS: %04x     RIP: %016lx RFL: %016lx",
	  (int) f->intr.cs, f->intr.rip, f->intr.rflags);
  hallog (" SS: %04x     RSP: %016lx", (int) f->intr.ss, f->intr.rsp);
  hallog ("CR2: %016lx err: %08lx", f->intr.cr2, f->intr.err);
}
