/** @file
  APXH SBI Environment Functions

  Provides basic environment functions for RISC-V SBI (Supervisor Binary
  Interface) platform, including character output and exit handling.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
**/

#include <stdio.h>

/**
  Output character via SBI.

  Outputs a single character using RISC-V SBI console putchar call
  (EID=1, FID=0).

  @param[in] Ch  Character to output.
**/
VOID
Putchar (
  IN int  Ch
  )
{
  asm volatile ("mv a0, %0\n" "li a7, 1\n" "ecall\n"::"r" (Ch):"a0", "a7");
}

/**
  Exit bootloader.

  Terminates bootloader execution. On SBI platform, enters infinite
  loop as SBI exit mechanism is not implemented.

  @param[in] Status  Exit status code.
**/
VOID
Exit (
  IN int  Status
  )
{
  printf ("Exit %d\n", Status);
  /* Should definitely tell SBI about this. */
  while (1);
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use Putchar instead **/
VOID putchar (INT32 ch) {
  Putchar (ch);
}

/** @deprecated Use Exit instead **/
VOID exit (INT32 status) {
  Exit (status);
}
