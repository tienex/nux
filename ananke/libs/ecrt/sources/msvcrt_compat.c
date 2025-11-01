/** @file
  eCRT - An embedded C runtime library

  MSVCRT compatibility functions

  Provides MSVCRT-specific string and memory functions not in standard C.
  These functions use non-standard names (prefixed with _ or suffixed with _s).

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ecrt/string.h>
#include <ecrt/stdlib.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Memory functions
 */

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

/*
 * String functions
 */

/**
  Case-insensitive string comparison.

  MSVCRT: _stricmp, _strcmpi

  @param[in] str1  First string
  @param[in] str2  Second string

  @return <0 if str1 < str2, 0 if equal, >0 if str1 > str2
**/
int
_stricmp (
    const char *str1,
    const char *str2
    )
{
    unsigned char c1, c2;

    do {
        c1 = (unsigned char)*str1++;
        c2 = (unsigned char)*str2++;

        /* Convert to lowercase */
        if (c1 >= 'A' && c1 <= 'Z')
            c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z')
            c2 += ('a' - 'A');

        if (c1 != c2)
            return c1 - c2;
    } while (c1 != '\0');

    return 0;
}

/* Alias */
int
_strcmpi (
    const char *str1,
    const char *str2
    )
{
    return _stricmp(str1, str2);
}

/**
  Case-insensitive string comparison with length limit.

  MSVCRT: _strnicmp, _strncmpi

  @param[in] str1   First string
  @param[in] str2   Second string
  @param[in] count  Maximum characters to compare

  @return <0 if str1 < str2, 0 if equal, >0 if str1 > str2
**/
int
_strnicmp (
    const char *str1,
    const char *str2,
    size_t count
    )
{
    unsigned char c1, c2;

    while (count--) {
        c1 = (unsigned char)*str1++;
        c2 = (unsigned char)*str2++;

        /* Convert to lowercase */
        if (c1 >= 'A' && c1 <= 'Z')
            c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z')
            c2 += ('a' - 'A');

        if (c1 != c2)
            return c1 - c2;
        if (c1 == '\0')
            break;
    }

    return 0;
}

/* Alias */
int
_strncmpi (
    const char *str1,
    const char *str2,
    size_t count
    )
{
    return _strnicmp(str1, str2, count);
}

/**
  Convert string to lowercase in place.

  MSVCRT: _strlwr

  @param[in,out] str  String to convert

  @return Pointer to converted string
**/
char *
_strlwr (
    char *str
    )
{
    char *p = str;

    while (*p != '\0') {
        if (*p >= 'A' && *p <= 'Z')
            *p += ('a' - 'A');
        p++;
    }

    return str;
}

/**
  Convert string to uppercase in place.

  MSVCRT: _strupr

  @param[in,out] str  String to convert

  @return Pointer to converted string
**/
char *
_strupr (
    char *str
    )
{
    char *p = str;

    while (*p != '\0') {
        if (*p >= 'a' && *p <= 'z')
            *p -= ('a' - 'A');
        p++;
    }

    return str;
}

/**
  Reverse string in place.

  MSVCRT: _strrev

  @param[in,out] str  String to reverse

  @return Pointer to reversed string
**/
char *
_strrev (
    char *str
    )
{
    char *start = str;
    char *end = str;
    char temp;

    if (str == NULL || *str == '\0')
        return str;

    /* Find end */
    while (*end != '\0')
        end++;
    end--;

    /* Swap characters */
    while (start < end) {
        temp = *start;
        *start++ = *end;
        *end-- = temp;
    }

    return str;
}

/**
  Set all characters in string to specified value.

  MSVCRT: _strset

  @param[in,out] str  String to modify
  @param[in]     c    Character value

  @return Pointer to modified string
**/
char *
_strset (
    char *str,
    int c
    )
{
    char *p = str;

    while (*p != '\0')
        *p++ = (char)c;

    return str;
}

/**
  Set up to N characters in string to specified value.

  MSVCRT: _strnset

  @param[in,out] str    String to modify
  @param[in]     c      Character value
  @param[in]     count  Maximum characters to set

  @return Pointer to modified string
**/
char *
_strnset (
    char *str,
    int c,
    size_t count
    )
{
    char *p = str;

    while (count-- && *p != '\0')
        *p++ = (char)c;

    return str;
}

/**
  Duplicate string (allocates memory).

  MSVCRT: _strdup

  Note: This requires a memory allocator. Returns NULL if allocation fails.

  @param[in] str  String to duplicate

  @return Pointer to duplicated string, or NULL on failure
**/
char *
_strdup (
    const char *str
    )
{
    size_t len;
    char *dup;

    if (str == NULL)
        return NULL;

    len = strlen(str) + UINT32_C(1);

#ifdef USE_NTRTL
    dup = (char *)RtlAllocateMemory(0, len, 'PURD');
#else
    /* Without allocator, return NULL */
    dup = NULL;
#endif

    if (dup != NULL) {
        memcpy(dup, str, len);
    }

    return dup;
}

/*
 * Secure string functions (_s suffix)
 * These are safer versions that check buffer sizes
 */

/**
  Secure string copy with size checking.

  MSVCRT: strcpy_s

  @param[out] dest      Destination buffer
  @param[in]  destSize  Size of destination buffer
  @param[in]  src       Source string

  @return 0 on success, non-zero on error
**/
int
strcpy_s (
    char *dest,
    size_t destSize,
    const char *src
    )
{
    size_t len;

    if (dest == NULL || src == NULL || destSize == 0)
        return 1;  /* Invalid parameter */

    len = strlen(src);
    if (len >= destSize)
        return 1;  /* Buffer too small */

    memcpy(dest, src, len + UINT32_C(1));
    return 0;
}

/**
  Secure string concatenation with size checking.

  MSVCRT: strcat_s

  @param[in,out] dest      Destination buffer
  @param[in]     destSize  Size of destination buffer
  @param[in]     src       Source string to append

  @return 0 on success, non-zero on error
**/
int
strcat_s (
    char *dest,
    size_t destSize,
    const char *src
    )
{
    size_t destLen, srcLen;

    if (dest == NULL || src == NULL || destSize == 0)
        return 1;

    destLen = strnlen(dest, destSize);
    if (destLen >= destSize)
        return 1;  /* Destination not null-terminated */

    srcLen = strlen(src);
    if (destLen + srcLen >= destSize)
        return 1;  /* Buffer too small */

    memcpy(dest + destLen, src, srcLen + UINT32_C(1));
    return 0;
}

/**
  Secure string copy with length limit.

  MSVCRT: strncpy_s

  @param[out] dest       Destination buffer
  @param[in]  destSize   Size of destination buffer
  @param[in]  src        Source string
  @param[in]  count      Maximum characters to copy

  @return 0 on success, non-zero on error
**/
int
strncpy_s (
    char *dest,
    size_t destSize,
    const char *src,
    size_t count
    )
{
    size_t i;

    if (dest == NULL || src == NULL || destSize == 0)
        return 1;

    if (count >= destSize)
        count = destSize - UINT32_C(1);

    for (i = 0; i < count && src[i] != '\0'; i++)
        dest[i] = src[i];

    dest[i] = '\0';
    return 0;
}
