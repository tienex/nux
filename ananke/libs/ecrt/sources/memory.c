/** @file
  eCRT - An embedded C runtime library

  NTRTL-based memory function implementations

  These wrappers provide standard C memory functions using optimized
  NTRTL implementations with SIMD support when USE_NTRTL is defined.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ecrt/string.h>
#include <ecrt/stddef.h>

#ifdef USE_NTRTL
#include <ananke/ntrtl/memory.h>

/**
  Copy memory block.

  Uses NTRTL's architecture-specific optimized implementations
  (SSE2, AVX2, AVX-512, RVV, etc.).

  @param[out] dest  Destination buffer
  @param[in]  src   Source buffer
  @param[in]  n     Number of bytes to copy

  @return Destination pointer
**/
void *
memcpy (
    void *dest,
    const void *src,
    size_t n
    )
{
    RtlCopyMemory(dest, src, (UINTN)n);
    return dest;
}

/**
  Move memory block (handles overlapping regions).

  Uses NTRTL's optimized memmove with overlap detection.

  @param[out] dest  Destination buffer
  @param[in]  src   Source buffer
  @param[in]  n     Number of bytes to move

  @return Destination pointer
**/
void *
memmove (
    void *dest,
    const void *src,
    size_t n
    )
{
    RtlMoveMemory(dest, src, (UINTN)n);
    return dest;
}

/**
  Fill memory with constant byte.

  Uses NTRTL's optimized fill operations.

  @param[out] s  Buffer to fill
  @param[in]  c  Byte value to fill
  @param[in]  n  Number of bytes to fill

  @return Buffer pointer
**/
void *
memset (
    void *s,
    int c,
    size_t n
    )
{
    RtlFillMemory(s, (UINTN)n, (UINT8)c);
    return s;
}

/**
  Compare memory blocks.

  Note: NTRTL's RtlCompareMemory returns the number of matching bytes,
  so we need to find the first difference to provide standard memcmp behavior.

  @param[in] s1  First buffer
  @param[in] s2  Second buffer
  @param[in] n   Number of bytes to compare

  @return <0 if s1 < s2, 0 if equal, >0 if s1 > s2
**/
int
memcmp (
    const void *s1,
    const void *s2,
    size_t n
    )
{
    UINTN result = RtlCompareMemory(s1, s2, (UINTN)n);

    if (result == (UINTN)n) {
        return 0;  /* Equal - all bytes matched */
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

/**
  Search for byte in memory.

  @param[in] s  Buffer to search
  @param[in] c  Byte value to find
  @param[in] n  Number of bytes to search

  @return Pointer to first occurrence, or NULL if not found
**/
void *
memchr (
    const void *s,
    int c,
    size_t n
    )
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

#endif /* USE_NTRTL */
