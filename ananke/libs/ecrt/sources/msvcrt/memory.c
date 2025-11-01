/** @file
  eCRT - An embedded C runtime library

  MSVCRT-compatible memory functions

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ecrt/string.h>
#include <stddef.h>
#include <stdint.h>

/**
  Copy memory until character found or count reached.

  MSVCRT: _memccpy

  @param[out] dest   Destination buffer
  @param[in]  src    Source buffer
  @param[in]  c      Character to stop at
  @param[in]  count  Maximum bytes to copy

  @return Pointer to byte after 'c' in dest, or NULL if 'c' not found
**/
void *
_memccpy (
    void *dest,
    const void *src,
    int c,
    size_t count
    )
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    unsigned char ch = (unsigned char)c;

    while (count--) {
        if ((*d++ = *s++) == ch) {
            return d;
        }
    }

    return NULL;
}

/**
  Case-insensitive memory comparison.

  MSVCRT: _memicmp

  @param[in] buf1   First buffer
  @param[in] buf2   Second buffer
  @param[in] count  Number of bytes to compare

  @return <0 if buf1 < buf2, 0 if equal, >0 if buf1 > buf2
**/
int
_memicmp (
    const void *buf1,
    const void *buf2,
    size_t count
    )
{
    const unsigned char *p1 = (const unsigned char *)buf1;
    const unsigned char *p2 = (const unsigned char *)buf2;
    int diff;

    while (count--) {
        unsigned char c1 = *p1++;
        unsigned char c2 = *p2++;

        /* Convert to lowercase for comparison */
        if (c1 >= 'A' && c1 <= 'Z')
            c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z')
            c2 += ('a' - 'A');

        diff = c1 - c2;
        if (diff != 0)
            return diff;
    }

    return 0;
}
