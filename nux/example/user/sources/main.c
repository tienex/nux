/** @file
  NUX Userspace Example

  Simple userspace program demonstrating system call interface.
  Implements basic I/O functions (putchar, puts, exit) using syscalls
  and tests all syscall argument passing conventions (0-6 arguments).

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <syscalls.h>
#include <stdio.h>

/**
  Output character to console.

  Uses syscall 4096 to output a single character.

  @param[in] C  Character to output.
**/
void
putchar (
  int  C
  )
{
  (void) syscall1 (4096, C);
}

/**
  Exit userspace process.

  Uses syscall 4097 to exit with specified status code.

  @param[in] Status  Exit status code.
**/
void
exit (
  int  Status
  )
{
  syscall1 (4097, Status);
}

/**
  Test system call interface.

  Tests all syscall argument passing conventions from 0 to 6 arguments.
  Each syscall validates that arguments are passed correctly.
**/
void
test (
  void
  )
{
  syscall0 (0);
  syscall1 (1, 1);
  syscall2 (2, 1, 2);
  syscall3 (3, 1, 2, 3);
  syscall4 (4, 1, 2, 3, 4);
  syscall5 (5, 1, 2, 3, 4, 5);
  syscall6 (6, 1, 2, 3, 4, 5, 6);
}

/**
  Output string to console.

  Outputs null-terminated string using putchar for each character.

  @param[in] pS  Pointer to null-terminated string.

  @return Always returns 0.
**/
int
puts (
  const char  *pS
  )
{
  char C;

  while ((C = *pS++) != '\0')
    putchar (C);

  return 0;
}

/**
  Main userspace entry point.

  Prints greeting message, runs syscall tests, and exits.

  @return Exit status 42.
**/
int
main (
  void
  )
{
  puts ("Hello from userspace, NUX!\n");

  test ();

  return 42;
}
