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
void *memcpy (void *d, void *s, size_t len);
int memcmp (void *s1, void *s2, size_t len);
void *memmove (void *d, void *s, size_t len);

int fls (int);
int ffs (int);

#endif /* EC_STRING_H */
