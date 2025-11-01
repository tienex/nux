/** @file
  eCRT - An embedded C runtime library

  MSVCRT-compatible secure string functions (_s suffix)

  These are safer versions with size checking to prevent buffer overflows.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ecrt/string.h>
#include <stddef.h>
#include <stdint.h>

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

/**
  Secure formatted string output with size checking.

  MSVCRT: sprintf_s

  @param[out] buffer     Destination buffer
  @param[in]  sizeInBytes Size of buffer in bytes
  @param[in]  format     Format string
  @param[in]  ...        Variable arguments

  @return Number of characters written, or -1 on error
**/
int
sprintf_s (
    char *buffer,
    size_t sizeInBytes,
    const char *format,
    ...
    )
{
    va_list args;
    int result;

    if (buffer == NULL || format == NULL || sizeInBytes == 0)
        return -1;

    va_start(args, format);
    result = vsnprintf(buffer, sizeInBytes, format, args);
    va_end(args);

    if (result < 0 || (size_t)result >= sizeInBytes) {
        if (sizeInBytes > 0)
            buffer[0] = '\0';
        return -1;
    }

    return result;
}

/**
  Secure variable-argument formatted string output.

  MSVCRT: vsprintf_s

  @param[out] buffer     Destination buffer
  @param[in]  sizeInBytes Size of buffer in bytes
  @param[in]  format     Format string
  @param[in]  args       Variable argument list

  @return Number of characters written, or -1 on error
**/
int
vsprintf_s (
    char *buffer,
    size_t sizeInBytes,
    const char *format,
    va_list args
    )
{
    int result;

    if (buffer == NULL || format == NULL || sizeInBytes == 0)
        return -1;

    result = vsnprintf(buffer, sizeInBytes, format, args);

    if (result < 0 || (size_t)result >= sizeInBytes) {
        if (sizeInBytes > 0)
            buffer[0] = '\0';
        return -1;
    }

    return result;
}

/**
  Secure memory copy with size checking.

  MSVCRT: memcpy_s

  @param[out] dest      Destination buffer
  @param[in]  destSize  Size of destination buffer
  @param[in]  src       Source buffer
  @param[in]  count     Number of bytes to copy

  @return 0 on success, non-zero on error
**/
int
memcpy_s (
    void *dest,
    size_t destSize,
    const void *src,
    size_t count
    )
{
    if (dest == NULL || src == NULL)
        return 1;

    if (count > destSize)
        return 1;

    memcpy(dest, src, count);
    return 0;
}

/**
  Secure memory move with size checking.

  MSVCRT: memmove_s

  @param[out] dest      Destination buffer
  @param[in]  destSize  Size of destination buffer
  @param[in]  src       Source buffer
  @param[in]  count     Number of bytes to move

  @return 0 on success, non-zero on error
**/
int
memmove_s (
    void *dest,
    size_t destSize,
    const void *src,
    size_t count
    )
{
    if (dest == NULL || src == NULL)
        return 1;

    if (count > destSize)
        return 1;

    memmove(dest, src, count);
    return 0;
}
