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

#include <stdio.h>
#include <nux/nux.h>
#include <nux/hal.h>

struct hal_frame *hal_entry_pf (struct hal_frame *f, unsigned long va,
				hal_pfinfo_t info)
{
  printf ("CPU%d: Pagefault at %lx\n", 0x1234, va);
  hal_frame_print (f);
  hal_cpu_halt ();
}

struct hal_frame *hal_entry_xcpt (struct hal_frame *f, unsigned xcpt)
{
  printf ("Exception %lx\n", xcpt);
  hal_frame_print (f);
  hal_cpu_halt ();
}

struct hal_frame *hal_entry_nmi (struct hal_frame *f)
{
  printf ("Hm. NMI received!\n");
  hal_cpu_halt ();
}

struct hal_frame *hal_entry_vect (struct hal_frame *f, unsigned irq)
{
  printf ("Hm. IRQ %d received\n");
  hal_cpu_halt ();
}

struct hal_frame *hal_entry_syscall (struct hal_frame *f,
				     unsigned long a0, unsigned long a1,
				     unsigned long a2, unsigned long a3,
				     unsigned long a4, unsigned long a5)
{
  printf ("Oh! Syscalls!\n");
  hal_cpu_halt ();
}
