/** @file
  NUX Emergency Condition Handling

  Provides panic, abort, and exit handling for emergency conditions.
  Includes basic console output and CPU halt/idle operations.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <stdio.h>
#include <stdlib.h>
#include <hal/hal.h>
#include <nux/nux.h>

#include "internal.h"

/**
  Output single character to console.

  Standard C library function for character output via HAL.

  @param[in] Char  Character to output.
**/
void
putchar (
  IN INT32  Char
  )
{
  hal_putchar (Char);
}

/**
  Panic kernel with message and frame.

  Stops all other CPUs via NMI and enters HAL panic handler.
  This function never returns. If system was already in panic
  state, simply halts the CPU.

  @param[in] pMessage  Panic message string.
  @param[in] pFrame    HAL frame at time of panic.
**/
VOID __dead
NuxPanic (
  IN CONST CHAR8       *pMessage,
  IN struct hal_frame  *pFrame
  )
{
  if (NuxStatusSetFlags (NUXST_PANIC) & NUXST_PANIC)
    {
      /* System was already in panic. Just halt the CPU. */
      hal_cpu_halt ();
      /* Unreachable. */
    }

  /* STOP all CPUs except this one. */
  CpuSendNmiAllButSelf ();

  hal_panic (CpuTryGetId (), pMessage, pFrame);
}

/**
  Abort execution.

  Standard C library function. Enters debug mode in infinite loop.
  This function never returns.
**/
VOID __dead
abort (
  VOID
  )
{
  while (1)
    hal_debug ();
}

/**
  Exit execution.

  Standard C library function. Handles special exit codes for
  CPU halt (EXIT_HALT) and idle (EXIT_IDLE). Otherwise aborts.
  This function never returns.

  @param[in] Status  Exit status code.
**/
VOID __dead
exit (
  IN INT32  Status
  )
{
  if (Status == EXIT_HALT)
    {
      hal_cpu_halt ();
    }
  else if (Status == EXIT_IDLE)
    {
      CpuIdle ();
    }


  abort ();
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use NuxPanic instead **/
void __dead nux_panic (const char *message, struct hal_frame *f) {
  NuxPanic (message, f);
}
