/** @file
  eCRT - An embedded C runtime library

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

/*
  eCRT - An embedded non standard C library

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __ecrt_stdlib_h__
#define __ecrt_stdlib_h__

#include <cdefs.h>

unsigned long strtoul (const char *str, char **endptr, int base);
int atexit (void (*func) (void));
void __dead exit (int status);	/* EXTERNAL */
void __dead abort (void);	/* EXTERNAL */

#endif
