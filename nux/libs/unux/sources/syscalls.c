/** @file
  User-Space System Call Wrappers

  Provides architecture-independent system call wrapper functions
  for 0-6 argument system calls. Uses architecture-specific macros
  to implement the actual system call invocation mechanism.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <arch_syscalls.h>

/**
  Invoke system call with no arguments.

  @param[in] Sys  System call number.

  @return System call return value.
**/
long
Syscall0 (
  IN unsigned long  Sys
  )
{
  __SYSCALL0 (Sys);
  return Sys;
}

/**
  Invoke system call with one argument.

  @param[in] Sys   System call number.
  @param[in] Arg1  First argument.

  @return System call return value.
**/
long
Syscall1 (
  IN unsigned long  Sys,
  IN unsigned long  Arg1
  )
{
  __SYSCALL1 (Sys, Arg1);
  return Sys;
}

/**
  Invoke system call with two arguments.

  @param[in] Sys   System call number.
  @param[in] Arg1  First argument.
  @param[in] Arg2  Second argument.

  @return System call return value.
**/
long
Syscall2 (
  IN unsigned long  Sys,
  IN unsigned long  Arg1,
  IN unsigned long  Arg2
  )
{
  __SYSCALL2 (Sys, Arg1, Arg2);
  return Sys;
}

/**
  Invoke system call with three arguments.

  @param[in] Sys   System call number.
  @param[in] Arg1  First argument.
  @param[in] Arg2  Second argument.
  @param[in] Arg3  Third argument.

  @return System call return value.
**/
long
Syscall3 (
  IN unsigned long  Sys,
  IN unsigned long  Arg1,
  IN unsigned long  Arg2,
  IN unsigned long  Arg3
  )
{
  __SYSCALL3 (Sys, Arg1, Arg2, Arg3);
  return Sys;
}

/**
  Invoke system call with four arguments.

  @param[in] Sys   System call number.
  @param[in] Arg1  First argument.
  @param[in] Arg2  Second argument.
  @param[in] Arg3  Third argument.
  @param[in] Arg4  Fourth argument.

  @return System call return value.
**/
long
Syscall4 (
  IN unsigned long  Sys,
  IN unsigned long  Arg1,
  IN unsigned long  Arg2,
  IN unsigned long  Arg3,
  IN unsigned long  Arg4
  )
{
  __SYSCALL4 (Sys, Arg1, Arg2, Arg3, Arg4);
  return Sys;
}

/**
  Invoke system call with five arguments.

  @param[in] Sys   System call number.
  @param[in] Arg1  First argument.
  @param[in] Arg2  Second argument.
  @param[in] Arg3  Third argument.
  @param[in] Arg4  Fourth argument.
  @param[in] Arg5  Fifth argument.

  @return System call return value.
**/
long
Syscall5 (
  IN unsigned long  Sys,
  IN unsigned long  Arg1,
  IN unsigned long  Arg2,
  IN unsigned long  Arg3,
  IN unsigned long  Arg4,
  IN unsigned long  Arg5
  )
{
  __SYSCALL5 (Sys, Arg1, Arg2, Arg3, Arg4, Arg5);
  return Sys;
}

/**
  Invoke system call with six arguments.

  @param[in] Sys   System call number.
  @param[in] Arg1  First argument.
  @param[in] Arg2  Second argument.
  @param[in] Arg3  Third argument.
  @param[in] Arg4  Fourth argument.
  @param[in] Arg5  Fifth argument.
  @param[in] Arg6  Sixth argument.

  @return System call return value.
**/
long
Syscall6 (
  IN unsigned long  Sys,
  IN unsigned long  Arg1,
  IN unsigned long  Arg2,
  IN unsigned long  Arg3,
  IN unsigned long  Arg4,
  IN unsigned long  Arg5,
  IN unsigned long  Arg6
  )
{
  __SYSCALL6 (Sys, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);
  return Sys;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use Syscall0 instead **/
long syscall0 (unsigned long sys) {
  return Syscall0 (sys);
}

/** @deprecated Use Syscall1 instead **/
long syscall1 (unsigned long sys, unsigned long arg1) {
  return Syscall1 (sys, arg1);
}

/** @deprecated Use Syscall2 instead **/
long syscall2 (unsigned long sys, unsigned long arg1, unsigned long arg2) {
  return Syscall2 (sys, arg1, arg2);
}

/** @deprecated Use Syscall3 instead **/
long syscall3 (unsigned long sys, unsigned long arg1, unsigned long arg2,
	       unsigned long arg3) {
  return Syscall3 (sys, arg1, arg2, arg3);
}

/** @deprecated Use Syscall4 instead **/
long syscall4 (unsigned long sys, unsigned long arg1, unsigned long arg2,
	       unsigned long arg3, unsigned long arg4) {
  return Syscall4 (sys, arg1, arg2, arg3, arg4);
}

/** @deprecated Use Syscall5 instead **/
long syscall5 (unsigned long sys, unsigned long arg1, unsigned long arg2,
	       unsigned long arg3, unsigned long arg4, unsigned long arg5) {
  return Syscall5 (sys, arg1, arg2, arg3, arg4, arg5);
}

/** @deprecated Use Syscall6 instead **/
long syscall6 (unsigned long sys, unsigned long arg1, unsigned long arg2,
	       unsigned long arg3, unsigned long arg4, unsigned long arg5,
	       unsigned long arg6) {
  return Syscall6 (sys, arg1, arg2, arg3, arg4, arg5, arg6);
}
