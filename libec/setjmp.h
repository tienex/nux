/*
  EC - An embedded non standard C library

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef EC_SETJMP_H
#define EC_SETJMP_H

#include <cdefs.h>
#include <machine/setjmp.h>

typedef long jmp_buf[_JBLEN];

#define setjmp(j) _setjmp(j)
#define longjmp(j,i) _longjmp(j,i)

int _setjmp (jmp_buf);
__dead void _longjmp (jmp_buf, int);
void _setupjmp (jmp_buf, void (*)(void), void *);

#endif /* EC_SETJMP_H */
