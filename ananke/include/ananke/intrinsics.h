/*++
    Module Name:

        intrinsics.h

    Abstract:

        Unified intrinsic wrappers for bit manipulation, overflow checking,
        varargs, and other compiler-provided operations.
        All exported macros are ANX_-prefixed and ALWAYS defined.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/compiler.h>
#include <ananke/types.h>

/* --------------------------------------------------------------- */
/*  Bit manipulation intrinsics (portable across compilers).       */
/* --------------------------------------------------------------- */

/* Count Trailing Zeros (CTZ) - returns position of first 1-bit from LSB */
#if defined(_MSC_VER)
#   include <intrin.h>
    static INLINE INT32 Anx_Ctz32(UINT32 Value) {
        unsigned long Index;
        if (_BitScanForward(&Index, Value)) return (INT32)Index;
        return 32;
    }
    static INLINE INT32 Anx_Ctz64(UINT64 Value) {
#       if defined(_M_X64) || defined(_M_ARM64)
            unsigned long Index;
            if (_BitScanForward64(&Index, Value)) return (INT32)Index;
            return 64;
#       else
            UINT32 Low = (UINT32)Value;
            if (Low) return Anx_Ctz32(Low);
            UINT32 High = (UINT32)(Value >> 32);
            if (High) return 32 + Anx_Ctz32(High);
            return 64;
#       endif
    }
#   define ANX_CTZ32(x)   Anx_Ctz32(x)
#   define ANX_CTZ64(x)   Anx_Ctz64(x)

#elif defined(__GNUC__) || defined(__clang__)
#   define ANX_CTZ32(x)   ((x) ? __builtin_ctz(x) : 32)
#   define ANX_CTZ64(x)   ((x) ? __builtin_ctzll(x) : 64)

#else
    static INLINE INT32 Anx_Ctz32(UINT32 Value) {
        INT32 Count = 0;
        if (!Value) return 32;
        while (!(Value & 1)) { Value >>= 1; Count++; }
        return Count;
    }
    static INLINE INT32 Anx_Ctz64(UINT64 Value) {
        INT32 Count = 0;
        if (!Value) return 64;
        while (!(Value & 1)) { Value >>= 1; Count++; }
        return Count;
    }
#   define ANX_CTZ32(x)   Anx_Ctz32(x)
#   define ANX_CTZ64(x)   Anx_Ctz64(x)
#endif

/* Count Leading Zeros (CLZ) - returns number of leading zero bits */
#if defined(_MSC_VER)
#   include <intrin.h>
    static INLINE INT32 Anx_Clz32(UINT32 Value) {
        unsigned long Index;
        if (_BitScanReverse(&Index, Value)) return 31 - (INT32)Index;
        return 32;
    }
    static INLINE INT32 Anx_Clz64(UINT64 Value) {
#       if defined(_M_X64) || defined(_M_ARM64)
            unsigned long Index;
            if (_BitScanReverse64(&Index, Value)) return 63 - (INT32)Index;
            return 64;
#       else
            UINT32 High = (UINT32)(Value >> 32);
            if (High) return Anx_Clz32(High);
            UINT32 Low = (UINT32)Value;
            if (Low) return 32 + Anx_Clz32(Low);
            return 64;
#       endif
    }
#   define ANX_CLZ32(x)   Anx_Clz32(x)
#   define ANX_CLZ64(x)   Anx_Clz64(x)

#elif defined(__GNUC__) || defined(__clang__)
#   define ANX_CLZ32(x)   ((x) ? __builtin_clz(x) : 32)
#   define ANX_CLZ64(x)   ((x) ? __builtin_clzll(x) : 64)

#else
    static INLINE INT32 Anx_Clz32(UINT32 Value) {
        INT32 Count = 0;
        if (!Value) return 32;
        while (!(Value & 0x80000000U)) { Value <<= 1; Count++; }
        return Count;
    }
    static INLINE INT32 Anx_Clz64(UINT64 Value) {
        INT32 Count = 0;
        if (!Value) return 64;
        while (!(Value & 0x8000000000000000ULL)) { Value <<= 1; Count++; }
        return Count;
    }
#   define ANX_CLZ32(x)   Anx_Clz32(x)
#   define ANX_CLZ64(x)   Anx_Clz64(x)
#endif

/* Find First Set (FFS) - returns 1-based position of first 1-bit, or 0 if none */
#if defined(_MSC_VER)
    static INLINE INT32 Anx_Ffs32(UINT32 Value) {
        unsigned long Index;
        if (_BitScanForward(&Index, Value)) return (INT32)Index + 1;
        return 0;
    }
    static INLINE INT32 Anx_Ffs64(UINT64 Value) {
#       if defined(_M_X64) || defined(_M_ARM64)
            unsigned long Index;
            if (_BitScanForward64(&Index, Value)) return (INT32)Index + 1;
            return 0;
#       else
            UINT32 Low = (UINT32)Value;
            if (Low) return Anx_Ffs32(Low);
            UINT32 High = (UINT32)(Value >> 32);
            if (High) return 32 + Anx_Ffs32(High);
            return 0;
#       endif
    }
#   define ANX_FFS32(x)   Anx_Ffs32(x)
#   define ANX_FFS64(x)   Anx_Ffs64(x)

#elif defined(__GNUC__) || defined(__clang__)
#   define ANX_FFS32(x)   __builtin_ffs((int)(x))
#   define ANX_FFS64(x)   __builtin_ffsll((long long)(x))

#else
    static INLINE INT32 Anx_Ffs32(UINT32 Value) {
        if (!Value) return 0;
        return Anx_Ctz32(Value) + 1;
    }
    static INLINE INT32 Anx_Ffs64(UINT64 Value) {
        if (!Value) return 0;
        return Anx_Ctz64(Value) + 1;
    }
#   define ANX_FFS32(x)   Anx_Ffs32(x)
#   define ANX_FFS64(x)   Anx_Ffs64(x)
#endif

/* Population Count (POPCOUNT) - counts number of 1-bits */
#if defined(_MSC_VER)
#   include <intrin.h>
#   if defined(_M_X64) || defined(_M_ARM64)
#       define ANX_POPCOUNT32(x)  ((INT32)__popcnt(x))
#       define ANX_POPCOUNT64(x)  ((INT32)__popcnt64(x))
#   else
        static INLINE INT32 Anx_Popcount32(UINT32 Value) {
            Value = Value - ((Value >> 1) & 0x55555555U);
            Value = (Value & 0x33333333U) + ((Value >> 2) & 0x33333333U);
            Value = (Value + (Value >> 4)) & 0x0F0F0F0FU;
            return (INT32)((Value * 0x01010101U) >> 24);
        }
        static INLINE INT32 Anx_Popcount64(UINT64 Value) {
            return Anx_Popcount32((UINT32)Value) + Anx_Popcount32((UINT32)(Value >> 32));
        }
#       define ANX_POPCOUNT32(x)  Anx_Popcount32(x)
#       define ANX_POPCOUNT64(x)  Anx_Popcount64(x)
#   endif

#elif defined(__GNUC__) || defined(__clang__)
#   define ANX_POPCOUNT32(x)  __builtin_popcount(x)
#   define ANX_POPCOUNT64(x)  __builtin_popcountll(x)

#else
    static INLINE INT32 Anx_Popcount32(UINT32 Value) {
        Value = Value - ((Value >> 1) & 0x55555555U);
        Value = (Value & 0x33333333U) + ((Value >> 2) & 0x33333333U);
        Value = (Value + (Value >> 4)) & 0x0F0F0F0FU;
        return (INT32)((Value * 0x01010101U) >> 24);
    }
    static INLINE INT32 Anx_Popcount64(UINT64 Value) {
        return Anx_Popcount32((UINT32)Value) + Anx_Popcount32((UINT32)(Value >> 32));
    }
#   define ANX_POPCOUNT32(x)  Anx_Popcount32(x)
#   define ANX_POPCOUNT64(x)  Anx_Popcount64(x)
#endif

/* Width-based variants for convenience */
#if defined(ANX_ARCH_64)
#   define ANX_CTZN(x)      ANX_CTZ64(x)
#   define ANX_CLZN(x)      ANX_CLZ64(x)
#   define ANX_FFSN(x)      ANX_FFS64(x)
#   define ANX_POPCOUNTN(x) ANX_POPCOUNT64(x)
#else
#   define ANX_CTZN(x)      ANX_CTZ32(x)
#   define ANX_CLZN(x)      ANX_CLZ32(x)
#   define ANX_FFSN(x)      ANX_FFS32(x)
#   define ANX_POPCOUNTN(x) ANX_POPCOUNT32(x)
#endif

/* Long variants (map to native width) */
#define ANX_CTZL(x)      ANX_CTZN((UINTN)(x))
#define ANX_CLZL(x)      ANX_CLZN((UINTN)(x))
#define ANX_FFSL(x)      ANX_FFSN((UINTN)(x))
#define ANX_POPCOUNTL(x) ANX_POPCOUNTN((UINTN)(x))

/* --------------------------------------------------------------- */
/*  Varargs intrinsics - portable across all compilers.            */
/* --------------------------------------------------------------- */

#if defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)
    typedef __builtin_va_list ANX_VA_LIST;
#   define ANX_VA_START(ap, last)  __builtin_va_start((ap), (last))
#   define ANX_VA_ARG(ap, type)    __builtin_va_arg((ap), type)
#   define ANX_VA_END(ap)          __builtin_va_end(ap)
#   define ANX_VA_COPY(dest, src)  __builtin_va_copy((dest), (src))
#else
#   include <stdarg.h>
    typedef va_list ANX_VA_LIST;
#   define ANX_VA_START(ap, last)  va_start((ap), (last))
#   define ANX_VA_ARG(ap, type)    va_arg((ap), type)
#   define ANX_VA_END(ap)          va_end(ap)
#   define ANX_VA_COPY(dest, src)  va_copy((dest), (src))
#endif

/* --------------------------------------------------------------- */
/*  Structure offset intrinsic - standard offsetof.                */
/* --------------------------------------------------------------- */

#if defined(__GNUC__) || defined(__clang__)
#   define ANX_OFFSETOF(type, member)  __builtin_offsetof(type, member)
#elif defined(_MSC_VER)
#   define ANX_OFFSETOF(type, member)  ((UINTN)&(((type*)0)->member))
#else
#   include <stddef.h>
#   define ANX_OFFSETOF(type, member)  offsetof(type, member)
#endif

/* --------------------------------------------------------------- */
/*  Compiler optimization hint intrinsics.                         */
/* --------------------------------------------------------------- */

#if defined(__GNUC__) || defined(__clang__)
#   define ANX_IS_CONSTANT(x)  __builtin_constant_p(x)
#else
#   define ANX_IS_CONSTANT(x)  0
#endif

/* Portable memory operations */
#if defined(__GNUC__) || defined(__clang__)
#   define ANX_MEMSET(dest, val, size)  __builtin_memset((dest), (val), (size))
#   define ANX_MEMCPY(dest, src, size)  __builtin_memcpy((dest), (src), (size))
#   define ANX_MEMMOVE(dest, src, size) __builtin_memmove((dest), (src), (size))
#   define ANX_MEMCMP(s1, s2, size)     __builtin_memcmp((s1), (s2), (size))
#else
#   include <string.h>
#   define ANX_MEMSET(dest, val, size)  memset((dest), (val), (size))
#   define ANX_MEMCPY(dest, src, size)  memcpy((dest), (src), (size))
#   define ANX_MEMMOVE(dest, src, size) memmove((dest), (src), (size))
#   define ANX_MEMCMP(s1, s2, size)     memcmp((s1), (s2), (size))
#endif

/* --------------------------------------------------------------- */
/*  Architecture-specific intrinsics.                              */
/* --------------------------------------------------------------- */

#if defined(__riscv) && (defined(__GNUC__) || defined(__clang__))
#   define ANX_THREAD_POINTER()  __builtin_thread_pointer()
#elif defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
    static INLINE VOID* Anx_GetThreadPointer(VOID) {
        VOID* Ptr;
        __asm__ __volatile__("mov %%fs:0, %0" : "=r"(Ptr));
        return Ptr;
    }
#   define ANX_THREAD_POINTER()  Anx_GetThreadPointer()
#elif defined(_M_X64) && defined(_MSC_VER)
#   include <intrin.h>
    static INLINE VOID* Anx_GetThreadPointer(VOID) {
        return (VOID*)__readgsqword(0);
    }
#   define ANX_THREAD_POINTER()  Anx_GetThreadPointer()
#else
#   define ANX_THREAD_POINTER()  ((VOID*)0)
#endif

/* --------------------------------------------------------------- */
/*  Overflow checking intrinsics.                                  */
/* --------------------------------------------------------------- */

#if defined(__GNUC__) || defined(__clang__)
#   define ANX_ADD_OVERFLOW(a, b, result)  __builtin_add_overflow((a), (b), (result))
#   define ANX_SUB_OVERFLOW(a, b, result)  __builtin_sub_overflow((a), (b), (result))
#   define ANX_MUL_OVERFLOW(a, b, result)  __builtin_mul_overflow((a), (b), (result))
#else
    static INLINE INT32 Anx_AddOverflow_I32(INT32 A, INT32 B, INT32 *Result) {
        *Result = A + B;
        return ((B > 0 && A > INT32_MAX - B) || (B < 0 && A < INT32_MIN - B));
    }
    static INLINE INT32 Anx_SubOverflow_I32(INT32 A, INT32 B, INT32 *Result) {
        *Result = A - B;
        return ((B < 0 && A > INT32_MAX + B) || (B > 0 && A < INT32_MIN + B));
    }
    static INLINE INT32 Anx_MulOverflow_I32(INT32 A, INT32 B, INT32 *Result) {
        INT64 R = (INT64)A * (INT64)B;
        *Result = (INT32)R;
        return (R > INT32_MAX || R < INT32_MIN);
    }
#   define ANX_ADD_OVERFLOW(a, b, result)  Anx_AddOverflow_I32((a), (b), (result))
#   define ANX_SUB_OVERFLOW(a, b, result)  Anx_SubOverflow_I32((a), (b), (result))
#   define ANX_MUL_OVERFLOW(a, b, result)  Anx_MulOverflow_I32((a), (b), (result))
#endif
