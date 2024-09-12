/*
  EC - An embedded non standard C library

  SPDX-License-Identifier:	BSD-2-Clause
*/

#if __i386__
#include <i386/setjmp.h>
#elif __amd64__
#include <amd64/setjmp.h>
#elif defined(__riscv) && __riscv_xlen == 64
#include <riscv64/setjmp.h>
#endif
