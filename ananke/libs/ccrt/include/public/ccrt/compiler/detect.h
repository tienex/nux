/** @file
  cCRT - Compiler Runtime Library

  Compiler Detection and Feature Macros

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __CCRT_COMPILER_DETECT_H__
#define __CCRT_COMPILER_DETECT_H__

/* Compiler Detection */
#if defined(_MSC_VER) && !defined(__clang__)
  #define CCRT_COMPILER_MSVC 1
  #define CCRT_COMPILER_VERSION _MSC_VER
#elif defined(__clang__)
  #define CCRT_COMPILER_CLANG 1
  #define CCRT_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
  #define CCRT_COMPILER_GCC 1
  #define CCRT_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(__WATCOMC__)
  #define CCRT_COMPILER_WATCOM 1
  #define CCRT_COMPILER_VERSION __WATCOMC__
#else
  #define CCRT_COMPILER_UNKNOWN 1
  #define CCRT_COMPILER_VERSION 0
#endif

/* Architecture Detection */
#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
  #define CCRT_ARCH_AMD64 1
  #define CCRT_ARCH_BITS 64
#elif defined(__i386__) || defined(_M_IX86)
  #define CCRT_ARCH_I386 1
  #define CCRT_ARCH_BITS 32
#elif defined(__riscv) && (__riscv_xlen == 64)
  #define CCRT_ARCH_RISCV64 1
  #define CCRT_ARCH_BITS 64
#elif defined(__aarch64__) || defined(_M_ARM64)
  #define CCRT_ARCH_ARM64 1
  #define CCRT_ARCH_BITS 64
#endif

/* Calling Conventions */
#if defined(CCRT_COMPILER_MSVC) || defined(CCRT_COMPILER_WATCOM)
  #define CCRT_CDECL __cdecl
  #define CCRT_STDCALL __stdcall
  #define CCRT_FASTCALL __fastcall
#elif defined(CCRT_COMPILER_GCC) || defined(CCRT_COMPILER_CLANG)
  #define CCRT_CDECL __attribute__((cdecl))
  #define CCRT_STDCALL __attribute__((stdcall))
  #define CCRT_FASTCALL __attribute__((fastcall))
#else
  #define CCRT_CDECL
  #define CCRT_STDCALL
  #define CCRT_FASTCALL
#endif

/* Function Attributes */
#if defined(CCRT_COMPILER_GCC) || defined(CCRT_COMPILER_CLANG)
  #define CCRT_INLINE static inline __attribute__((always_inline))
  #define CCRT_NOINLINE __attribute__((noinline))
  #define CCRT_NORETURN __attribute__((noreturn))
  #define CCRT_CONST __attribute__((const))
  #define CCRT_PURE __attribute__((pure))
  #define CCRT_USED __attribute__((used))
  #define CCRT_UNUSED __attribute__((unused))
  #define CCRT_PACKED __attribute__((packed))
  #define CCRT_ALIGNED(x) __attribute__((aligned(x)))
#elif defined(CCRT_COMPILER_MSVC)
  #define CCRT_INLINE static __forceinline
  #define CCRT_NOINLINE __declspec(noinline)
  #define CCRT_NORETURN __declspec(noreturn)
  #define CCRT_CONST
  #define CCRT_PURE
  #define CCRT_USED
  #define CCRT_UNUSED
  #define CCRT_PACKED
  #define CCRT_ALIGNED(x) __declspec(align(x))
#elif defined(CCRT_COMPILER_WATCOM)
  #define CCRT_INLINE static inline
  #define CCRT_NOINLINE
  #define CCRT_NORETURN
  #define CCRT_CONST
  #define CCRT_PURE
  #define CCRT_USED
  #define CCRT_UNUSED
  #define CCRT_PACKED
  #define CCRT_ALIGNED(x)
#else
  #define CCRT_INLINE static inline
  #define CCRT_NOINLINE
  #define CCRT_NORETURN
  #define CCRT_CONST
  #define CCRT_PURE
  #define CCRT_USED
  #define CCRT_UNUSED
  #define CCRT_PACKED
  #define CCRT_ALIGNED(x)
#endif

/* Compiler RT ABI */
#if defined(CCRT_COMPILER_MSVC)
  #define COMPILER_RT_ABI
#else
  #define COMPILER_RT_ABI __attribute__((visibility("hidden")))
#endif

/* Builtin Availability */
#if defined(CCRT_COMPILER_GCC) || defined(CCRT_COMPILER_CLANG)
  #define CCRT_HAS_BUILTIN_CLZ 1
  #define CCRT_HAS_BUILTIN_CTZ 1
  #define CCRT_HAS_BUILTIN_POPCOUNT 1
  #define CCRT_HAS_BUILTIN_BSWAP 1
#endif

#if defined(CCRT_COMPILER_CLANG)
  #if __has_builtin(__builtin_clz)
    #define CCRT_HAS_BUILTIN_CLZ 1
  #endif
  #if __has_builtin(__builtin_ctz)
    #define CCRT_HAS_BUILTIN_CTZ 1
  #endif
  #if __has_builtin(__builtin_popcount)
    #define CCRT_HAS_BUILTIN_POPCOUNT 1
  #endif
#endif

#endif /* __CCRT_COMPILER_DETECT_H__ */
