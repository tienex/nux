/** @file
  cCRT - Compiler Runtime Library

  Compiler Builtin Functions (GCC, Clang, MSVC, Open Watcom)

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __CCRT_INTRINSICS_BUILTINS_H__
#define __CCRT_INTRINSICS_BUILTINS_H__

#include <ccrt/compiler/detect.h>

/* Count Leading Zeros */
CCRT_INLINE int ccrt_clz32(unsigned int x) {
#if defined(CCRT_COMPILER_GCC) || defined(CCRT_COMPILER_CLANG)
    return __builtin_clz(x);
#elif defined(CCRT_COMPILER_MSVC)
    unsigned long index;
    _BitScanReverse(&index, x);
    return 31 - (int)index;
#elif defined(CCRT_COMPILER_WATCOM)
    int count = 0;
    if (x == 0) return 32;
    while ((x & 0x80000000) == 0) {
        x <<= 1;
        count++;
    }
    return count;
#else
    int count = 0;
    if (x == 0) return 32;
    while ((x & 0x80000000) == 0) {
        x <<= 1;
        count++;
    }
    return count;
#endif
}

CCRT_INLINE int ccrt_clz64(unsigned long long x) {
#if defined(CCRT_COMPILER_GCC) || defined(CCRT_COMPILER_CLANG)
    return __builtin_clzll(x);
#elif defined(CCRT_COMPILER_MSVC) && defined(CCRT_ARCH_AMD64)
    unsigned long index;
    _BitScanReverse64(&index, x);
    return 63 - (int)index;
#else
    if (x == 0) return 64;
    int count = 0;
    while ((x & 0x8000000000000000ULL) == 0) {
        x <<= 1;
        count++;
    }
    return count;
#endif
}

/* Count Trailing Zeros */
CCRT_INLINE int ccrt_ctz32(unsigned int x) {
#if defined(CCRT_COMPILER_GCC) || defined(CCRT_COMPILER_CLANG)
    return __builtin_ctz(x);
#elif defined(CCRT_COMPILER_MSVC)
    unsigned long index;
    _BitScanForward(&index, x);
    return (int)index;
#else
    if (x == 0) return 32;
    int count = 0;
    while ((x & 1) == 0) {
        x >>= 1;
        count++;
    }
    return count;
#endif
}

CCRT_INLINE int ccrt_ctz64(unsigned long long x) {
#if defined(CCRT_COMPILER_GCC) || defined(CCRT_COMPILER_CLANG)
    return __builtin_ctzll(x);
#elif defined(CCRT_COMPILER_MSVC) && defined(CCRT_ARCH_AMD64)
    unsigned long index;
    _BitScanForward64(&index, x);
    return (int)index;
#else
    if (x == 0) return 64;
    int count = 0;
    while ((x & 1) == 0) {
        x >>= 1;
        count++;
    }
    return count;
#endif
}

/* Population Count (number of 1 bits) */
CCRT_INLINE int ccrt_popcount32(unsigned int x) {
#if defined(CCRT_COMPILER_GCC) || defined(CCRT_COMPILER_CLANG)
    return __builtin_popcount(x);
#elif defined(CCRT_COMPILER_MSVC)
    return (int)__popcnt(x);
#else
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0F0F0F0F;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return x & 0x0000003F;
#endif
}

CCRT_INLINE int ccrt_popcount64(unsigned long long x) {
#if defined(CCRT_COMPILER_GCC) || defined(CCRT_COMPILER_CLANG)
    return __builtin_popcountll(x);
#elif defined(CCRT_COMPILER_MSVC) && defined(CCRT_ARCH_AMD64)
    return (int)__popcnt64(x);
#else
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    x = x + (x >> 8);
    x = x + (x >> 16);
    x = x + (x >> 32);
    return x & 0x000000000000007FULL;
#endif
}

/* Byte Swap */
CCRT_INLINE unsigned short ccrt_bswap16(unsigned short x) {
#if defined(CCRT_COMPILER_GCC) || defined(CCRT_COMPILER_CLANG)
    return __builtin_bswap16(x);
#elif defined(CCRT_COMPILER_MSVC)
    return _byteswap_ushort(x);
#else
    return (x << 8) | (x >> 8);
#endif
}

CCRT_INLINE unsigned int ccrt_bswap32(unsigned int x) {
#if defined(CCRT_COMPILER_GCC) || defined(CCRT_COMPILER_CLANG)
    return __builtin_bswap32(x);
#elif defined(CCRT_COMPILER_MSVC)
    return _byteswap_ulong(x);
#else
    return ((x << 24) & 0xFF000000) |
           ((x << 8)  & 0x00FF0000) |
           ((x >> 8)  & 0x0000FF00) |
           ((x >> 24) & 0x000000FF);
#endif
}

CCRT_INLINE unsigned long long ccrt_bswap64(unsigned long long x) {
#if defined(CCRT_COMPILER_GCC) || defined(CCRT_COMPILER_CLANG)
    return __builtin_bswap64(x);
#elif defined(CCRT_COMPILER_MSVC)
    return _byteswap_uint64(x);
#else
    return ((x << 56) & 0xFF00000000000000ULL) |
           ((x << 40) & 0x00FF000000000000ULL) |
           ((x << 24) & 0x0000FF0000000000ULL) |
           ((x << 8)  & 0x000000FF00000000ULL) |
           ((x >> 8)  & 0x00000000FF000000ULL) |
           ((x >> 24) & 0x0000000000FF0000ULL) |
           ((x >> 40) & 0x000000000000FF00ULL) |
           ((x >> 56) & 0x00000000000000FFULL);
#endif
}

#endif /* __CCRT_INTRINSICS_BUILTINS_H__ */
