/** @file
  eCRT - An embedded C runtime library

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

/*
  eCRT - An embedded non standard C library
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __ecrt_string_h__
#define __ecrt_string_h__

#include <stddef.h>


size_t strlen (const char *s);
size_t strnlen (const char *s, size_t maxlen);
char *strchr (const char *p, int ch);
char *strrchr (const char *p, int ch);
size_t strcspn (const char *s, const char *charset);
size_t strlcpy (char *dst, const char *src, size_t siz);
char * strncpy(char *dst, const char *src, size_t n);
int strncmp (const char *s1, const char *s2, size_t n);

void *memset (void *b, int c, size_t len);
void *memcpy (void *d, const void *s, size_t len);
int memcmp (const void *s1, const void *s2, size_t len);
void *memmove (void *d, const void *s, size_t len);
void *memchr (const void *s, int c, size_t n);

unsigned long fls (unsigned long);
unsigned long ffs (unsigned long);

/* Wide character string functions (16-bit wchar_t with -fshort-wchar) */
#ifndef _KERNEL
typedef unsigned short wchar_t;

size_t wcslen (const wchar_t *s);
wchar_t *wcscpy (wchar_t *dest, const wchar_t *src);
wchar_t *wcsncpy (wchar_t *dest, const wchar_t *src, size_t n);
int wcscmp (const wchar_t *s1, const wchar_t *s2);
int wcsncmp (const wchar_t *s1, const wchar_t *s2, size_t n);
wchar_t *wcschr (const wchar_t *s, wchar_t c);
wchar_t *wcsrchr (const wchar_t *s, wchar_t c);

/* Optional: Integration with NT RTL if available */
#ifdef USE_NTRTL
#include <ananke/ntrtl/string.h>
#endif

#endif /* _KERNEL */

#endif /* eCRT_STRING_H */
