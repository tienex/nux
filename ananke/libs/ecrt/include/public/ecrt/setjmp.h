/** @file
  eCRT - An embedded C runtime library

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

/*
  eCRT - An embedded non standard C library

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __ecrt_setjmp_h__
#define __ecrt_setjmp_h__

#include <cdefs.h>
#include <machine/setjmp.h>

typedef long jmp_buf[_JBLEN];

#define setjmp(j) _setjmp(j)
#define longjmp(j,i) _longjmp(j,i)

int _setjmp (jmp_buf);
__dead void _longjmp (jmp_buf, int);
void _setupjmp (jmp_buf, void (*)(void), void *);

#endif /* eCRT_SETJMP_H */
