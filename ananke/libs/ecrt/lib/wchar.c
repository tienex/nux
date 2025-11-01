/** @file
  eCRT - An embedded C runtime library

  Wide character (wchar_t) string functions
  Maps to NTRTL CHAR16 operations when USE_NTRTL is defined

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ecrt/string.h>
#include <stddef.h>

#ifndef _KERNEL

/*
 * Wide character type is 16-bit with -fshort-wchar
 * Maps directly to CHAR16/WCHAR in NTRTL
 */

#ifdef USE_NTRTL
/*
 * Use optimized NTRTL implementations
 * These call into the architecture-specific SIMD implementations
 */
#include <ananke/ntrtl.h>

size_t
wcslen (const wchar_t *s)
{
    return (size_t)RtlStringLength16((const CHAR16 *)s);
}

wchar_t *
wcscpy (wchar_t *dest, const wchar_t *src)
{
    return (wchar_t *)RtlCopyString16((CHAR16 *)dest, (const CHAR16 *)src);
}

wchar_t *
wcsncpy (wchar_t *dest, const wchar_t *src, size_t n)
{
    CHAR16 *d = (CHAR16 *)dest;
    const CHAR16 *s = (const CHAR16 *)src;

    RtlCopyChars16(d, s, (UINTN)n);

    /* Ensure null termination */
    if (n > 0) {
        d[n - 1] = 0;
    }

    return dest;
}

int
wcscmp (const wchar_t *s1, const wchar_t *s2)
{
    return RtlCompareString16((const CHAR16 *)s1, (const CHAR16 *)s2);
}

int
wcsncmp (const wchar_t *s1, const wchar_t *s2, size_t n)
{
    return RtlCompareChars16((const CHAR16 *)s1, (const CHAR16 *)s2, (UINTN)n);
}

wchar_t *
wcschr (const wchar_t *s, wchar_t c)
{
    return (wchar_t *)RtlFindChar16((const CHAR16 *)s, (CHAR16)c);
}

wchar_t *
wcsrchr (const wchar_t *s, wchar_t c)
{
    return (wchar_t *)RtlFindLastChar16((const CHAR16 *)s, (CHAR16)c);
}

#else
/*
 * Portable C implementations (fallback when NTRTL not available)
 */

size_t
wcslen (const wchar_t *s)
{
    const wchar_t *p = s;

    while (*p != 0) {
        p++;
    }

    return (size_t)(p - s);
}

wchar_t *
wcscpy (wchar_t *dest, const wchar_t *src)
{
    wchar_t *d = dest;

    while ((*d++ = *src++) != 0)
        ;

    return dest;
}

wchar_t *
wcsncpy (wchar_t *dest, const wchar_t *src, size_t n)
{
    wchar_t *d = dest;
    size_t i;

    for (i = 0; i < n && src[i] != 0; i++) {
        d[i] = src[i];
    }

    /* Pad with nulls if needed */
    for (; i < n; i++) {
        d[i] = 0;
    }

    return dest;
}

int
wcscmp (const wchar_t *s1, const wchar_t *s2)
{
    while (*s1 != 0 && *s1 == *s2) {
        s1++;
        s2++;
    }

    return (int)(*s1 - *s2);
}

int
wcsncmp (const wchar_t *s1, const wchar_t *s2, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (int)(s1[i] - s2[i]);
        }
        if (s1[i] == 0) {
            break;
        }
    }

    return 0;
}

wchar_t *
wcschr (const wchar_t *s, wchar_t c)
{
    while (*s != 0) {
        if (*s == c) {
            return (wchar_t *)s;
        }
        s++;
    }

    /* Check if searching for null terminator */
    if (c == 0) {
        return (wchar_t *)s;
    }

    return NULL;
}

wchar_t *
wcsrchr (const wchar_t *s, wchar_t c)
{
    const wchar_t *last = NULL;

    do {
        if (*s == c) {
            last = s;
        }
    } while (*s++ != 0);

    return (wchar_t *)last;
}

#endif /* USE_NTRTL */

#endif /* _KERNEL */
