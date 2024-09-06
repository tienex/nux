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
#include <nux/plt.h>
#include "internal.h"

struct hal_frame *
hal_entry_syscall (struct hal_frame *f,
		   unsigned long a1, unsigned long a2, unsigned long a3,
		   unsigned long a4, unsigned long a5, unsigned long a6)
{
  uctxt_t *uctxt = uctxt_get (f);
  switch ((uintptr_t) uctxt)
    {
    case (uintptr_t) UCTXT_INVALID:
    case (uintptr_t) UCTXT_IDLE:
      /* Syscall in kernel? */
      nux_panic ("Unexpected Kernel Exception -- Syscall(!)", f);
      /* Unreached. */
    default:
      break;
    }

  /* Process syscall */
  uctxt = entry_sysc (uctxt, a1, a2, a3, a4, a5, a6);
  return uctxt_frame (uctxt);
}

struct hal_frame *
hal_entry_pf (struct hal_frame *f, unsigned long va, hal_pfinfo_t info)
{
  if (!nux_status_okcpu ())
    {
      nux_panic ("Early Kernel Page Fault", f);
      /* Unreachable */
    }

  uctxt_t *uctxt = uctxt_get (f);
  switch ((uintptr_t) uctxt)
    {
    case (uintptr_t) UCTXT_INVALID:
      if (uaddr_valid (va))
	{
	  /*
	     This could be a PF due to kernel user access.

	     In this case we would longjmp to the user pagefault jmp_buf and
	     the next function won't return.
	   */
	  cpu_useraccess_checkpf (va, info);
	}
      /* PASS-THROUGH */
    case (uintptr_t) UCTXT_IDLE:
      nux_panic ("Unexpected Kernel Page Fault", f);
      /* Unreached. */
    default:
      break;
    }

  /* User page fault. */
  uctxt = entry_pf (uctxt, va, info);
  return uctxt_frame (uctxt);
}

struct hal_frame *
hal_entry_debug (struct hal_frame *f, unsigned xcpt)
{
  nux_panic ("Kernel Panic", f);
  /* Unreachable */
}

struct hal_frame *
hal_entry_xcpt (struct hal_frame *f, unsigned xcpt)
{
  if (!nux_status_okcpu ())
    {
      nux_panic ("Early Kernel Exception", f);
      /* Unreachable */
    }

  uctxt_t *uctxt = uctxt_get (f);
  switch ((uintptr_t) uctxt)
    {
    case (uintptr_t) UCTXT_INVALID:
    case (uintptr_t) UCTXT_IDLE:
      /* Kernel exception. */
      nux_panic ("Unexpected Kernel Exception", f);
      /* Unreached. */
    default:
      break;
    }

  /* User exception. */
  uctxt = entry_ex (uctxt, xcpt);
  return uctxt_frame (uctxt);
}

void
hal_entry_nmi (struct hal_frame *f)
{
  printf ("NMI!\n");
  hal_frame_print (f);
  if (__predict_false (nux_status () & NUXST_PANIC))
    {
      hal_cpu_halt ();
      /* Unreachable */
    }

  if (!nux_status_okcpu ())
    {
      return;
    }

  /* NMI are handled internally in NUX. */
  cpu_nmiop ();
}

struct hal_frame *
hal_entry_vect (struct hal_frame *f, unsigned vect)
{
  uctxt_t *uctxt = uctxt_getuser (f);
  struct plt_vect_desc d;;

  plt_vect_translate (vect, &d);
  switch (d.type)
    {
    case PLT_VECT_IRQ:
      uctxt = entry_irq (uctxt, d.no, plt_irq_islevel (d.no));
      plt_irq_eoi ();
      break;
    case PLT_VECT_TMR:
      uctxt = entry_alarm (uctxt);
      plt_irq_eoi ();
      break;
    case PLT_VECT_IPI:
      uctxt = entry_ipi (uctxt);
      plt_irq_eoi ();
      break;
    default:
    case PLT_VECT_IGN:
      warn ("Ignoring vector %d", vect);
      /* UCTXT unchanged. */
      break;
    }

  return uctxt_frame (uctxt);
}

struct hal_frame *
hal_entry_timer (struct hal_frame *f)
{
  uctxt_t *uctxt = uctxt_getuser (f);
  uctxt = entry_alarm (uctxt);
  return uctxt_frame (uctxt);
}

struct hal_frame *
hal_entry_irq (struct hal_frame *f, unsigned irq)
{
  uctxt_t *uctxt = uctxt_getuser (f);

  uctxt = entry_irq (uctxt, irq, plt_irq_islevel (irq));
  plt_irq_eoi ();

  return uctxt_frame (uctxt);
}

struct hal_frame *
hal_entry_ipi (struct hal_frame *f)
{
  uctxt_t *uctxt = uctxt_getuser (f);
  uctxt = entry_ipi (uctxt);
  plt_irq_eoi ();
  return uctxt_frame (uctxt);
}
