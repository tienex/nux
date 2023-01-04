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
      error ("Unexpected Kernel Exception -- Syscall(!)");
      hal_frame_print (f);
      exit (-1);
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
    case (uintptr_t) UCTXT_IDLE:
      error ("Unexpected Kernel Page Fault");
      hal_frame_print (f);
      exit (-1);
    default:
      break;
    }

  /* User page fault. */
  uctxt = entry_pf (uctxt, va, info);
  return uctxt_frame (uctxt);
}

struct hal_frame *
hal_entry_xcpt (struct hal_frame *f, unsigned xcpt)
{
  uctxt_t *uctxt = uctxt_get (f);

  switch ((uintptr_t) uctxt)
    {
    case (uintptr_t) UCTXT_INVALID:
    case (uintptr_t) UCTXT_IDLE:
      /* Kernel exception. */
      error ("Unexpected Kernel Exception %d", xcpt);
      hal_frame_print (f);
      exit (-1);
    default:
      break;
    }

  /* User exception. */
  uctxt = entry_ex (uctxt, xcpt);
  return uctxt_frame (uctxt);
}

struct hal_frame *
hal_entry_nmi (struct hal_frame *f)
{
  /* NMI are handled internally in NUX. */
  cpu_tlbnmi ();
  return f;
}

struct hal_frame *
hal_entry_vect (struct hal_frame *f, unsigned vect)
{
  uctxt_t *uctxt = uctxt_getuser (f);

  if (plt_vect_process (vect))
    {
      uctxt = entry_alarm (uctxt);
    }
  else
    {
      unsigned irqbase = hal_vect_irqbase ();
      unsigned ipibase = hal_vect_ipibase ();
      unsigned vectmax = hal_vect_max ();
      unsigned irqlimit = irqbase < ipibase ? ipibase : vectmax;
      unsigned ipilimit = ipibase < irqbase ? irqbase : vectmax;

      if (vect >= irqbase && vect < irqlimit)
	{
	  unsigned irq = vect - irqbase;
	  uctxt = entry_irq (uctxt, irq, plt_irq_islevel (irq));
	}
      else if (vect >= ipibase && vect < ipilimit)
	{
	  unsigned ipi = vect - ipibase;
	  uctxt = entry_ipi (uctxt, ipi);
	}
#undef MIN
    }
  plt_irq_eoi ();

  return uctxt_frame (uctxt);
}

