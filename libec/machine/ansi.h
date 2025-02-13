/*
  EC - An embedded non standard C library

  SPDX-License-Identifier:	BSD-2-Clause
*/

#if __i386__
#include <i386/ansi.h>
#elif __amd64__
#include <amd64/ansi.h>
#elif defined(__riscv) && __riscv_xlen == 64
#include <riscv64/ansi.h>
#endif
