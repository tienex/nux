/** @file
  eCRT - An embedded C runtime library

  MSVCRT-compatible string functions

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ecrt/string.h>
#include <ecrt/stdlib.h>
#include <stddef.h>
#include <stdint.h>

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
