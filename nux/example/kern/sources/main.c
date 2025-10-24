/** @file
  NUX Kernel Example

  Demonstrates NUX kernel library usage including memory allocation,
  timer operations, user context management, HAL operations, and
  kernel entry points for system calls, IPIs, alarms, exceptions,
  page faults, and interrupts.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <assert.h>
#include <stdio.h>
#include <nux/nux.h>
#include <nux/nuxperf.h>

#include <hal/hal.h>

uctxt_t gUserInit;
struct hal_umap gUserMap;

DEFINE_MEASURE (syscalls_cycles);
DEFINE_MEASURE (syscalls_nsecs);

/**
  Main kernel entry point.

  Demonstrates timer operations, memory allocation/deallocation,
  user context bootstrapping, and user address space enumeration.

  @param[in] Argc  Argument count.
  @param[in] Argv  Argument vector.

  @return EXIT_IDLE to indicate kernel idle state.
**/
int
main (
  int   Argc,
  char  *Argv[]
  )
{
  printf ("Hello, %s (%" PRIx64 ")!", Argv[1], timer_gettime ());

  timer_alarm (1 * 1000 * 1000 * 1000);

  kmem_trim_setmode (TRIM_BRK);

  for (int i = 0; i < 2; i++)
    {
      uintptr_t X1, Y1, X2, Y2;

      X1 = kmem_alloc (0, 61234);
      Y1 = kmem_alloc (1, 5123);
      X2 = kmem_alloc (0, 61234);
      Y2 = kmem_alloc (1, 5123);

      kmem_free (1, Y2, 5123);
      kmem_free (0, X2, 61234);
      kmem_free (1, Y1, 5123);
      kmem_free (0, X1, 61234);

      kmem_trim_one (TRIM_BRK);
    }

  if (!uctxt_bootstrap (&gUserInit))
    {
      printf ("NO USER PROCESS.");
    }
  else
    {
      cpu_ipi (cpu_id ());
    }


  hal_l1p_t L1p;
  hal_l1e_t L1e;
  hal_umap_bootstrap (&gUserMap);
  uctxt_print (&gUserInit);

  for (uint64_t i = 0;; i += (1 << 12))
    {
      uint64_t X = hal_umap_next (&gUserMap, i, &L1p, &L1e);
      if (X == UADDR_INVALID)
	break;
      printf ("%lx - %lx(%lx)\n", X, L1p, L1e);
      i = X;
    }

  return EXIT_IDLE;
}

/**
  Application processor entry point.

  Entry point for secondary CPUs. Prints CPU ID and current time.

  @return EXIT_IDLE to indicate processor idle state.
**/
int
main_ap (
  void
  )
{
  printf ("%d: %" PRIx64 "\n", cpu_id (), timer_gettime ());
  return EXIT_IDLE;
}

/**
  System call entry point.

  Handles system calls from user space. Implements test syscalls (0-6),
  putchar (4096), and exit (4097). Measures syscall performance.

  @param[in] pU   User context.
  @param[in] A1   System call number.
  @param[in] A2   Argument 1.
  @param[in] A3   Argument 2.
  @param[in] A4   Argument 3.
  @param[in] A5   Argument 4.
  @param[in] A6   Argument 5.
  @param[in] A7   Argument 6.

  @return Updated user context, or UCTXT_IDLE if user process exits.
**/
uctxt_t *
entry_sysc (
  uctxt_t        *pU,
  unsigned long  A1,
  unsigned long  A2,
  unsigned long  A3,
  unsigned long  A4,
  unsigned long  A5,
  unsigned long  A6,
  unsigned long  A7
  )
{
  uint64_t StartCy = hal_cpu_cycles ();
  uint64_t StartNsecs = timer_gettime ();

  switch (A1)
    {
    case 0:
      info ("SYSC%ld test passed.", A1);
      break;
    case 1:
      assert (A2 == 1);
      info ("SYSC%ld test passed.", A1);
      break;
    case 2:
      assert (A2 == 1);
      assert (A3 == 2);
      info ("SYSC%ld test passed.", A1);
      break;
    case 3:
      assert (A2 == 1);
      assert (A3 == 2);
      assert (A4 == 3);
      info ("SYSC%ld test passed.", A1);
      break;
    case 4:
      assert (A2 == 1);
      assert (A3 == 2);
      assert (A4 == 3);
      assert (A5 == 4);
      info ("SYSC%ld test passed.", A1);
      break;
    case 5:
      assert (A2 == 1);
      assert (A3 == 2);
      assert (A4 == 3);
      assert (A5 == 4);
      assert (A6 == 5);
      info ("SYSC%ld test passed.", A1);
      break;
    case 6:
      assert (A2 == 1);
      assert (A3 == 2);
      assert (A4 == 3);
      assert (A5 == 4);
      assert (A6 == 5);
      assert (A7 == 6);
      info ("SYSC%ld test passed.", A1);
      break;
    case 4096:
      putchar (A2);
      break;
    case 4097:
      info ("User exited with error code: %ld", A2);
      hal_umap_load (NULL);
      hal_umap_free (&gUserMap);
      return UCTXT_IDLE;

    default:
      info ("Received unknown syscall %ld %ld %ld %ld %ld %ld %ld\n",
	    A1, A2, A3, A4, A5, A6, A7);
      break;
    }
  nuxmeasure_add (&syscalls_nsecs, timer_gettime() - StartNsecs);
  nuxmeasure_add (&syscalls_cycles, hal_cpu_cycles() - StartCy);
  return pU;
}

/**
  Inter-processor interrupt entry point.

  Handles IPI (inter-processor interrupt) and returns initial user context.

  @param[in] pUctxt  Current user context.

  @return Initial user context.
**/
uctxt_t *
entry_ipi (
  uctxt_t  *pUctxt
  )
{
  info ("IPI!");
  return &gUserInit;
}


/**
  Timer alarm entry point.

  Handles timer alarms, sets next alarm, prints current time and context,
  and displays performance measurements.

  @param[in] pUctxt  Current user context.

  @return Same user context.
**/
uctxt_t *
entry_alarm (
  uctxt_t  *pUctxt
  )
{
  timer_alarm (1 * 1000 * 1000 * 1000);
  info ("TMR: %" PRIu64 " us", timer_gettime ());
  uctxt_print (pUctxt);

  nuxperf_print ();
  nuxmeasure_print ();

  return pUctxt;
}

/**
  Exception entry point.

  Handles CPU exceptions, prints exception number and context.

  @param[in] pUctxt  Current user context.
  @param[in] Ex      Exception number.

  @return UCTXT_IDLE to indicate idle state.
**/
uctxt_t *
entry_ex (
  uctxt_t   *pUctxt,
  unsigned  Ex
  )
{
  info ("Exception %d", Ex);
  uctxt_print (pUctxt);
  return UCTXT_IDLE;
}

/**
  Page fault entry point.

  Handles page faults, prints faulting address and page fault info.

  @param[in] pUctxt  Current user context.
  @param[in] Va      Faulting virtual address.
  @param[in] Pfi     Page fault information.

  @return UCTXT_IDLE to indicate idle state.
**/
uctxt_t *
entry_pf (
  uctxt_t       *pUctxt,
  vaddr_t       Va,
  hal_pfinfo_t  Pfi
  )
{
  info ("CPU #%d Pagefault at %08lx (%d)", cpu_id (), Va, Pfi);
  uctxt_print (pUctxt);
  return UCTXT_IDLE;
}

/**
  Interrupt request entry point.

  Handles hardware interrupts, prints IRQ number.

  @param[in] pUctxt  Current user context.
  @param[in] Irq     Interrupt request number.
  @param[in] Lvl     Level-triggered flag.

  @return Same user context.
**/
uctxt_t *
entry_irq (
  uctxt_t   *pUctxt,
  unsigned  Irq,
  bool      Lvl
  )
{
  info ("IRQ %d", Irq);
  return pUctxt;

}
