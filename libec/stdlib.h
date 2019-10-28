/*
  EC - An embedded non standard C library

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef EC_STDLIB_H
#define EC_STDLIB_H

#include <cdefs.h>

int atexit (void (*func) (void));
void __dead exit (int status);	/* EXTERNAL */

#endif
