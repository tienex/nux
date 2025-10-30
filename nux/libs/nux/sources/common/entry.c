/** @file
  NUX HAL Entry Points

  Provides Hardware Abstraction Layer entry points for system calls,
  exceptions, page faults, interrupts (timer, IRQ, IPI), and NMI.
  Routes hardware events to appropriate user context handlers.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <stdio.h>
#include <nux/nux.h>
#include <hal/hal.h>
#include <platform/platform.h>
#include <nux/internal.h>

/**
  Handle system call entry from HAL.

  Routes system call to user context handler. Panics if called
  from kernel context.

  @param[in] Frame  Exception frame.
  @param[in] A1      First system call argument.
  @param[in] A2      Second system call argument.
  @param[in] A3      Third system call argument.
  @param[in] A4      Fourth system call argument.
  @param[in] A5      Fifth system call argument.
  @param[in] A6      Sixth system call argument.
  @param[in] A7      Seventh system call argument.

  @return Modified frame pointer after system call processing.
**/
struct hal_frame *
hal_entry_syscall (
  IN struct hal_frame  *Frame,
  IN UINTN             A1,
  IN UINTN             A2,
  IN UINTN             A3,
  IN UINTN             A4,
  IN UINTN             A5,
  IN UINTN             A6,
  IN UINTN             A7
  )
{
  uctxt_t *Uctxt;

  nuxperf_inc (&pnux_entry_syscall);
  Uctxt = uctxt_get (Frame);

  switch ((uintptr_t) Uctxt)
    {
    case (uintptr_t) UCTXT_INVALID:
    case (uintptr_t) UCTXT_IDLE:
      /* Syscall in kernel? */
      nux_panic ("Unexpected Kernel Exception -- Syscall(!)", Frame);
      /* Unreached. */
    default:
      break;
    }

  /* Process syscall */
  Uctxt = entry_sysc (Uctxt, A1, A2, A3, A4, A5, A6, A7);
  return uctxt_frame (Uctxt);
}

/**
  Handle page fault entry from HAL.

  Routes page fault to user context handler. Handles kernel user access
  faults via longjmp. Panics on unexpected kernel page faults or early
  kernel faults.

  @param[in] Frame  Exception frame.
  @param[in] Va      Faulting virtual address.
  @param[in] Info    Page fault information flags.

  @return Modified frame pointer after page fault processing.
**/
struct hal_frame *
hal_entry_pf (
  IN struct hal_frame  *Frame,
  IN UINTN             Va,
  IN hal_pfinfo_t      Info
  )
{
  uctxt_t *Uctxt;

  nuxperf_inc (&pnux_entry_pagefault);

  if (!NuxStatusOkCpu ())
    {
      nux_panic ("Early Kernel Page Fault", Frame);
      /* Unreachable */
    }

  Uctxt = uctxt_get (Frame);

  switch ((uintptr_t) Uctxt)
    {
    case (uintptr_t) UCTXT_INVALID:
      if (uaddr_valid (Va))
        {
          /*
             This could be a PF due to kernel user access.

             In this case we would longjmp to the user pagefault jmp_buf and
             the next function won't return.
           */
          CpuUserAccessCheckPageFault (Va, Info);
        }

      /* PASS-THROUGH */
    case (uintptr_t) UCTXT_IDLE:
      nux_panic ("Unexpected Kernel Page Fault", Frame);
      /* Unreached. */
    default:
      break;
    }

  /* User page fault. */
  Uctxt = entry_pf (Uctxt, Va, Info);
  return uctxt_frame (Uctxt);
}

/**
  Handle debug exception entry from HAL.

  Currently triggers kernel panic on any debug exception.

  @param[in] Frame  Exception frame.
  @param[in] Xcpt    Exception number.

  @return Does not return (panic).
**/
struct hal_frame *
hal_entry_debug (
  IN struct hal_frame  *Frame,
  IN UINT32            Xcpt
  )
{
  nux_panic ("Kernel Panic", Frame);
  /* Unreachable */
}

/**
  Handle general exception entry from HAL.

  Routes exception to user context handler. Panics on kernel exceptions
  or early boot exceptions.

  @param[in] Frame  Exception frame.
  @param[in] Xcpt    Exception number.

  @return Modified frame pointer after exception processing.
**/
struct hal_frame *
hal_entry_xcpt (
  IN struct hal_frame  *Frame,
  IN UINT32            Xcpt
  )
{
  uctxt_t *Uctxt;

  nuxperf_inc (&pnux_entry_exception);

  if (!NuxStatusOkCpu ())
    {
      nux_panic ("Early Kernel Exception", Frame);
      /* Unreachable */
    }

  Uctxt = uctxt_get (Frame);

  switch ((uintptr_t) Uctxt)
    {
    case (uintptr_t) UCTXT_INVALID:
    case (uintptr_t) UCTXT_IDLE:
      /* Kernel exception. */
      nux_panic ("Unexpected Kernel Exception", Frame);
      /* Unreached. */
    default:
      break;
    }

  /* User exception. */
  Uctxt = entry_ex (Uctxt, Xcpt);
  return uctxt_frame (Uctxt);
}

/**
  Handle Non-Maskable Interrupt (NMI) entry from HAL.

  Processes NMI operations for CPU coordination. Halts CPU if system
  is in panic state. Ignored during early boot.

  @param[in] Frame  Exception frame.
**/
VOID
hal_entry_nmi (
  IN struct hal_frame  *Frame
  )
{
  nuxperf_inc (&pnux_entry_nmi);

  if (__predict_false (NuxStatus () & NUXST_PANIC))
    {
      hal_cpu_halt ();
      /* Unreachable */
    }

  if (!NuxStatusOkCpu ())
    {
      return;
    }

  /* NMI are handled internally in NUX. */
  CpuNmiOperation ();
}

/**
  Handle timer interrupt entry from HAL.

  Routes timer event to user context handler and sends End-Of-Interrupt
  to platform.

  @param[in] Frame  Exception frame.

  @return Modified frame pointer after timer processing.
**/
struct hal_frame *
hal_entry_timer (
  IN struct hal_frame  *Frame
  )
{
  uctxt_t *Uctxt;

  nuxperf_inc (&pnux_entry_timer);
  Uctxt = uctxt_getuser (Frame);
  Uctxt = entry_alarm (Uctxt);
  plt_eoi_timer ();
  return uctxt_frame (Uctxt);
}

/**
  Handle IRQ entry from HAL.

  Routes IRQ to user context handler and sends End-Of-Interrupt to platform.

  @param[in] Frame    Exception frame.
  @param[in] Irq       IRQ number.
  @param[in] IsLevel   TRUE if level-triggered, FALSE if edge-triggered.

  @return Modified frame pointer after IRQ processing.
**/
struct hal_frame *
hal_entry_irq (
  IN struct hal_frame  *Frame,
  IN UINT32            Irq,
  IN BOOLEAN           IsLevel
  )
{
  uctxt_t *Uctxt;

  nuxperf_inc (&pnux_entry_irq);
  Uctxt = uctxt_getuser (Frame);
  Uctxt = entry_irq (Uctxt, Irq, IsLevel);
  plt_eoi_irq (Irq);
  return uctxt_frame (Uctxt);
}

/**
  Handle Inter-Processor Interrupt (IPI) entry from HAL.

  Routes IPI to user context handler and sends End-Of-Interrupt to platform.

  @param[in] Frame  Exception frame.

  @return Modified frame pointer after IPI processing.
**/
struct hal_frame *
hal_entry_ipi (
  IN struct hal_frame  *Frame
  )
{
  uctxt_t *Uctxt;

  nuxperf_inc (&pnux_entry_ipi);
  Uctxt = uctxt_getuser (Frame);
  Uctxt = entry_ipi (Uctxt);
  plt_eoi_ipi ();
  return uctxt_frame (Uctxt);
}
