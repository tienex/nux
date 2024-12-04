/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <stdio.h>
#include <nux/nux.h>
#include <nux/hal.h>
#include <nux/plt.h>
#include "internal.h"

struct hal_frame *
hal_entry_syscall (struct hal_frame *f,
		   unsigned long a1, unsigned long a2, unsigned long a3,
		   unsigned long a4, unsigned long a5, unsigned long a6,
		   unsigned long a7)
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
  uctxt = entry_sysc (uctxt, a1, a2, a3, a4, a5, a6, a7);
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
hal_entry_timer (struct hal_frame *f)
{
  uctxt_t *uctxt = uctxt_getuser (f);
  uctxt = entry_alarm (uctxt);
  plt_eoi_timer ();
  return uctxt_frame (uctxt);
}

struct hal_frame *
hal_entry_irq (struct hal_frame *f, unsigned irq, bool islevel)
{
  uctxt_t *uctxt = uctxt_getuser (f);

  uctxt = entry_irq (uctxt, irq, islevel);
  plt_eoi_irq (irq);
  return uctxt_frame (uctxt);
}

struct hal_frame *
hal_entry_ipi (struct hal_frame *f)
{
  uctxt_t *uctxt = uctxt_getuser (f);
  uctxt = entry_ipi (uctxt);
  plt_eoi_ipi ();
  return uctxt_frame (uctxt);
}
