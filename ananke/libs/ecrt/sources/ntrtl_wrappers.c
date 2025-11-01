/** @file
  eCRT - An embedded C runtime library

  Standard C library wrappers for NTRTL functions

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ecrt/string.h>
#include <ecrt/stddef.h>

#ifdef USE_NTRTL
#include <ananke/ntrtl/memory.h>
#include <ananke/ntrtl/string.h>

/* Memory functions - wrap NTRTL */

void *memcpy(void *dest, const void *src, size_t n)
{
    RtlCopyMemory(dest, src, (UINTN)n);
    return dest;
}

void *memmove(void *dest, const void *src, size_t n)
{
    RtlMoveMemory(dest, src, (UINTN)n);
    return dest;
}

void *memset(void *s, int c, size_t n)
{
    RtlFillMemory(s, (UINTN)n, (UINT8)c);
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    UINTN result = RtlCompareMemory(s1, s2, (UINTN)n);
    if (result == (UINTN)n) {
        return 0;  /* Equal */
    }
    /* Find first differing byte */
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

void *memchr(const void *s, int c, size_t n)
{
    const unsigned char *p = s;
    unsigned char ch = (unsigned char)c;

    for (size_t i = 0; i < n; i++) {
        if (p[i] == ch) {
            return (void *)(p + i);
        }
    }
    return NULL;
}

/* String functions - these use the raw memory/char operations */
/* NTRTL string functions work with STRING structures, so we keep simple helpers */

size_t strlen(const char *s)
{
    const char *p = s;
    while (*p) p++;
    return p - s;
}

char *strchr(const char *s, int c)
{
    char ch = (char)c;
    while (*s) {
        if (*s == ch) return (char *)s;
        s++;
    }
    return (ch == '\0') ? (char *)s : NULL;
}

char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    char ch = (char)c;

    while (*s) {
        if (*s == ch) last = s;
        s++;
    }
    if (ch == '\0') return (char *)s;
    return (char *)last;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

#endif /* USE_NTRTL */
