/*
  EC - An embedded non standard C library
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef EC_STRING_H
#define EC_STRING_H

#include <stddef.h>

size_t strlen (const char *s);
void *memset (void *b, int c, size_t len);
void *memcpy (void *d, const void *s, size_t len);
int memcmp (const void *s1, const void *s2, size_t len);
void *memmove (void *d, const void *s, size_t len);

unsigned long fls (unsigned long);
unsigned long ffs (unsigned long);

#endif /* EC_STRING_H */
