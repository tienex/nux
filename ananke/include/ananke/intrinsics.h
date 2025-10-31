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

/* Get thread-local storage pointer (TLS/TIB) */
#if defined(__riscv) && (defined(__GNUC__) || defined(__clang__))
    /* RISC-V: tp register via __builtin_thread_pointer() */
#   define ANX_THREAD_POINTER()  __builtin_thread_pointer()

#elif defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
    /* x86-64: Read from GS segment register offset 0 */
    static INLINE VOID* Anx_GetThreadPointer(VOID) {
        VOID* Ptr;
        __asm__ __volatile__("mov %%gs:0, %0" : "=r"(Ptr));
        return Ptr;
    }
#   define ANX_THREAD_POINTER()  Anx_GetThreadPointer()

#elif (defined(__i386__) || defined(__i686__)) && (defined(__GNUC__) || defined(__clang__))
    /* x86 32-bit: Read from GS segment register offset 0 */
    static INLINE VOID* Anx_GetThreadPointer(VOID) {
        VOID* Ptr;
        __asm__ __volatile__("mov %%gs:0, %0" : "=r"(Ptr));
        return Ptr;
    }
#   define ANX_THREAD_POINTER()  Anx_GetThreadPointer()

#elif defined(_M_X64) && defined(_MSC_VER)
    /* MSVC x86-64: Use __readgsqword intrinsic */
#   include <intrin.h>
    static INLINE VOID* Anx_GetThreadPointer(VOID) {
        return (VOID*)__readgsqword(0);
    }
#   define ANX_THREAD_POINTER()  Anx_GetThreadPointer()

#elif defined(_M_IX86) && defined(_MSC_VER)
    /* MSVC x86 32-bit: Use __readfsdword intrinsic */
#   include <intrin.h>
    static INLINE VOID* Anx_GetThreadPointer(VOID) {
        return (VOID*)__readfsdword(0x18); /* NT_TIB.Self */
    }
#   define ANX_THREAD_POINTER()  Anx_GetThreadPointer()

#else
    /* Unsupported architecture - return NULL */
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

/* --------------------------------------------------------------- */
/*  CPU-specific inline assembly wrappers.                        */
/*  Portable across architectures and compilers.                  */
/* --------------------------------------------------------------- */

/* Compiler barrier - prevents reordering across this point */
#if defined(__GNUC__) || defined(__clang__)
#   define ANX_CPU_BARRIER()  __asm__ __volatile__("" ::: "memory")
#elif defined(_MSC_VER)
#   include <intrin.h>
#   define ANX_CPU_BARRIER()  _ReadWriteBarrier()
#else
#   define ANX_CPU_BARRIER()  ((void)0)
#endif

/* Memory fence - full hardware memory barrier */
#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_MFENCE()  __asm__ __volatile__("mfence" ::: "memory")
#       define ANX_CPU_SFENCE()  __asm__ __volatile__("sfence" ::: "memory")
#       define ANX_CPU_LFENCE()  __asm__ __volatile__("lfence" ::: "memory")
#   elif defined(_MSC_VER)
#       include <intrin.h>
#       define ANX_CPU_MFENCE()  _mm_mfence()
#       define ANX_CPU_SFENCE()  _mm_sfence()
#       define ANX_CPU_LFENCE()  _mm_lfence()
#   endif
#elif defined(__riscv)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_MFENCE()  __asm__ __volatile__("fence rw, rw" ::: "memory")
#       define ANX_CPU_SFENCE()  __asm__ __volatile__("fence w, w" ::: "memory")
#       define ANX_CPU_LFENCE()  __asm__ __volatile__("fence r, r" ::: "memory")
#   endif
#elif defined(__aarch64__) || defined(_M_ARM64)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_MFENCE()  __asm__ __volatile__("dmb sy" ::: "memory")
#       define ANX_CPU_SFENCE()  __asm__ __volatile__("dmb st" ::: "memory")
#       define ANX_CPU_LFENCE()  __asm__ __volatile__("dmb ld" ::: "memory")
#   elif defined(_MSC_VER)
#       include <intrin.h>
#       define ANX_CPU_MFENCE()  __dmb(_ARM64_BARRIER_SY)
#       define ANX_CPU_SFENCE()  __dmb(_ARM64_BARRIER_ST)
#       define ANX_CPU_LFENCE()  __dmb(_ARM64_BARRIER_LD)
#   endif
#endif

#ifndef ANX_CPU_MFENCE
#   define ANX_CPU_MFENCE()  ANX_CPU_BARRIER()
#   define ANX_CPU_SFENCE()  ANX_CPU_BARRIER()
#   define ANX_CPU_LFENCE()  ANX_CPU_BARRIER()
#endif

/* CPU pause/yield hint */
#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_PAUSE()  __asm__ __volatile__("pause")
#   elif defined(_MSC_VER)
#       include <intrin.h>
#       define ANX_CPU_PAUSE()  _mm_pause()
#   endif
#elif defined(__riscv)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_PAUSE()  __asm__ __volatile__("nop")
#   endif
#elif defined(__aarch64__) || defined(_M_ARM64)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_PAUSE()  __asm__ __volatile__("yield")
#   elif defined(_MSC_VER)
#       define ANX_CPU_PAUSE()  __yield()
#   endif
#endif

#ifndef ANX_CPU_PAUSE
#   define ANX_CPU_PAUSE()  ((void)0)
#endif

/* CPU halt (privileged operation) */
#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_HALT()  __asm__ __volatile__("hlt")
#   endif
#elif defined(__riscv)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_HALT()  __asm__ __volatile__("wfi")
#   endif
#elif defined(__aarch64__) || defined(_M_ARM64)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_HALT()  __asm__ __volatile__("wfi")
#   endif
#endif

#ifndef ANX_CPU_HALT
#   define ANX_CPU_HALT()  do { for(;;); } while(0)
#endif

/* Breakpoint */
#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_BREAKPOINT()  __asm__ __volatile__("int3")
#   elif defined(_MSC_VER)
#       define ANX_CPU_BREAKPOINT()  __debugbreak()
#   endif
#elif defined(__riscv)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_BREAKPOINT()  __asm__ __volatile__("ebreak")
#   endif
#elif defined(__aarch64__) || defined(_M_ARM64)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_BREAKPOINT()  __asm__ __volatile__("brk #0")
#   elif defined(_MSC_VER)
#       define ANX_CPU_BREAKPOINT()  __debugbreak()
#   endif
#endif

#ifndef ANX_CPU_BREAKPOINT
#   define ANX_CPU_BREAKPOINT()  ANX_TRAP()
#endif

/* Cache line flush */
#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_CLFLUSH(addr)  __asm__ __volatile__("clflush %0" : : "m"(*(char*)(addr)))
#   elif defined(_MSC_VER)
#       include <intrin.h>
#       define ANX_CPU_CLFLUSH(addr)  _mm_clflush(addr)
#   endif
#elif defined(__riscv)
#   if defined(__GNUC__) || defined(__clang__)
        /* RISC-V doesn't have cache flush in base ISA, use fence */
#       define ANX_CPU_CLFLUSH(addr)  __asm__ __volatile__("fence rw, rw" ::: "memory")
#   endif
#elif defined(__aarch64__) || defined(_M_ARM64)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_CLFLUSH(addr)  __asm__ __volatile__("dc cvac, %0" : : "r"(addr))
#   endif
#endif

#ifndef ANX_CPU_CLFLUSH
#   define ANX_CPU_CLFLUSH(addr)  ((void)(addr))
#endif

/* Read timestamp counter / cycle counter */
#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
#   if defined(__GNUC__) || defined(__clang__)
        static INLINE UINT64 Anx_ReadTsc(VOID) {
            UINT32 Lo, Hi;
            __asm__ __volatile__("rdtsc" : "=a"(Lo), "=d"(Hi));
            return ((UINT64)Hi << 32) | Lo;
        }
#       define ANX_CPU_RDTSC()  Anx_ReadTsc()
#   elif defined(_MSC_VER)
#       include <intrin.h>
#       define ANX_CPU_RDTSC()  __rdtsc()
#   endif
#elif defined(__riscv)
#   if defined(__GNUC__) || defined(__clang__)
        static INLINE UINT64 Anx_ReadCycle(VOID) {
            UINT64 Cycles;
            __asm__ __volatile__("rdcycle %0" : "=r"(Cycles));
            return Cycles;
        }
#       define ANX_CPU_RDTSC()  Anx_ReadCycle()
#   endif
#elif defined(__aarch64__) || defined(_M_ARM64)
#   if defined(__GNUC__) || defined(__clang__)
        static INLINE UINT64 Anx_ReadCntvct(VOID) {
            UINT64 Val;
            __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(Val));
            return Val;
        }
#       define ANX_CPU_RDTSC()  Anx_ReadCntvct()
#   endif
#endif

#ifndef ANX_CPU_RDTSC
#   define ANX_CPU_RDTSC()  ((UINT64)0)
#endif

/* TLB invalidation */
#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_INVLPG(addr)  __asm__ __volatile__("invlpg (%0)" : : "r"(addr) : "memory")
#   endif
#elif defined(__riscv)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_INVLPG(addr)  __asm__ __volatile__("sfence.vma x0, %0" : : "r"(addr) : "memory")
#   endif
#elif defined(__aarch64__) || defined(_M_ARM64)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_INVLPG(addr)  __asm__ __volatile__("tlbi vae1, %0" : : "r"((UINTN)(addr) >> 12))
#   endif
#endif

#ifndef ANX_CPU_INVLPG
#   define ANX_CPU_INVLPG(addr)  ((void)(addr))
#endif

/* Full TLB flush */
#if defined(__x86_64__) || defined(__i386__)
#   if defined(__GNUC__) || defined(__clang__)
        static INLINE VOID Anx_FlushTlb(VOID) {
            UINTN Cr3;
            __asm__ __volatile__("mov %%cr3, %0" : "=r"(Cr3));
            __asm__ __volatile__("mov %0, %%cr3" : : "r"(Cr3) : "memory");
        }
#       define ANX_CPU_FLUSHTLB()  Anx_FlushTlb()
#   endif
#elif defined(__riscv)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_FLUSHTLB()  __asm__ __volatile__("sfence.vma x0, x0" ::: "memory")
#   endif
#elif defined(__aarch64__)
#   if defined(__GNUC__) || defined(__clang__)
#       define ANX_CPU_FLUSHTLB()  __asm__ __volatile__("tlbi vmalle1" ::: "memory")
#   endif
#endif

#ifndef ANX_CPU_FLUSHTLB
#   define ANX_CPU_FLUSHTLB()  ((void)0)
#endif

/* ---------------------------------------------------------------
 *  X86-Specific CPU Operations
 * --------------------------------------------------------------- */

#if defined(__x86_64__) || defined(__i386__)
#   if defined(__GNUC__) || defined(__clang__)

/* Control Register Access */
static INLINE UINTN Anx_ReadCr0(VOID) {
    UINTN Val;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(Val));
    return Val;
}

static INLINE VOID Anx_WriteCr0(UINTN Val) {
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(Val));
}

static INLINE UINTN Anx_ReadCr3(VOID) {
    UINTN Val;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(Val));
    return Val;
}

static INLINE VOID Anx_WriteCr3(UINTN Val) {
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(Val) : "memory");
}

static INLINE UINTN Anx_ReadCr4(VOID) {
    UINTN Val;
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(Val));
    return Val;
}

static INLINE VOID Anx_WriteCr4(UINTN Val) {
    __asm__ __volatile__("mov %0, %%cr4" : : "r"(Val));
}

#       define ANX_CPU_READ_CR0()       Anx_ReadCr0()
#       define ANX_CPU_WRITE_CR0(val)   Anx_WriteCr0(val)
#       define ANX_CPU_READ_CR3()       Anx_ReadCr3()
#       define ANX_CPU_WRITE_CR3(val)   Anx_WriteCr3(val)
#       define ANX_CPU_READ_CR4()       Anx_ReadCr4()
#       define ANX_CPU_WRITE_CR4(val)   Anx_WriteCr4(val)

/* MSR Access */
static INLINE UINT64 Anx_ReadMsr(UINT32 Msr) {
    UINT32 Lo, Hi;
    __asm__ __volatile__("rdmsr" : "=a"(Lo), "=d"(Hi) : "c"(Msr));
    return ((UINT64)Hi << 32) | Lo;
}

static INLINE VOID Anx_WriteMsr(UINT32 Msr, UINT64 Val) {
    UINT32 Lo = (UINT32)Val;
    UINT32 Hi = (UINT32)(Val >> 32);
    __asm__ __volatile__("wrmsr" : : "c"(Msr), "a"(Lo), "d"(Hi));
}

#       define ANX_CPU_RDMSR(msr)       Anx_ReadMsr(msr)
#       define ANX_CPU_WRMSR(msr, val)  Anx_WriteMsr(msr, val)

/* I/O Port Access */
static INLINE UINT8 Anx_Inb(UINT16 Port) {
    UINT8 Val;
    __asm__ __volatile__("xor %%eax, %%eax; inb %%dx, %%al" : "=a"(Val) : "d"(Port));
    return Val;
}

static INLINE VOID Anx_Outb(UINT16 Port, UINT8 Val) {
    __asm__ __volatile__("outb %%al, %%dx" : : "d"(Port), "a"(Val));
}

static INLINE UINT16 Anx_Inw(UINT16 Port) {
    UINT16 Val;
    __asm__ __volatile__("xor %%eax, %%eax; inw %%dx, %%ax" : "=a"(Val) : "d"(Port));
    return Val;
}

static INLINE VOID Anx_Outw(UINT16 Port, UINT16 Val) {
    __asm__ __volatile__("outw %%ax, %%dx" : : "d"(Port), "a"(Val));
}

static INLINE UINT32 Anx_Inl(UINT16 Port) {
    UINT32 Val;
    __asm__ __volatile__("inl %%dx, %%eax" : "=a"(Val) : "d"(Port));
    return Val;
}

static INLINE VOID Anx_Outl(UINT16 Port, UINT32 Val) {
    __asm__ __volatile__("outl %%eax, %%dx" : : "d"(Port), "a"(Val));
}

#       define ANX_CPU_INB(port)        Anx_Inb(port)
#       define ANX_CPU_OUTB(port, val)  Anx_Outb(port, val)
#       define ANX_CPU_INW(port)        Anx_Inw(port)
#       define ANX_CPU_OUTW(port, val)  Anx_Outw(port, val)
#       define ANX_CPU_INL(port)        Anx_Inl(port)
#       define ANX_CPU_OUTL(port, val)  Anx_Outl(port, val)

/* CPUID */
static INLINE VOID Anx_Cpuid(UINT32* Eax, UINT32* Ebx, UINT32* Ecx, UINT32* Edx) {
    __asm__ __volatile__("cpuid" : "+a"(*Eax), "=b"(*Ebx), "+c"(*Ecx), "=d"(*Edx));
}

#       define ANX_CPU_CPUID(eax, ebx, ecx, edx)  Anx_Cpuid(eax, ebx, ecx, edx)

/* Segment Register Operations */
#       if defined(__x86_64__)
static INLINE VOID Anx_WriteGsBase(UINTN Val) {
    __asm__ __volatile__("movq %0, %%gs:0" : : "r"(Val));
}

static INLINE UINTN Anx_ReadGsBase(VOID) {
    UINTN Val;
    __asm__ __volatile__("movq %%gs:0, %0" : "=r"(Val));
    return Val;
}

#           define ANX_CPU_WRITE_GSBASE(val)  Anx_WriteGsBase(val)
#           define ANX_CPU_READ_GSBASE()      Anx_ReadGsBase()
#       endif

#       if defined(__i386__)
static INLINE VOID Anx_WriteFsBase(UINT32 Val) {
    __asm__ __volatile__("movl %0, %%fs:0" : : "r"(Val));
}

static INLINE UINT32 Anx_ReadFsBase(VOID) {
    UINT32 Val;
    __asm__ __volatile__("movl %%fs:0, %0" : "=r"(Val));
    return Val;
}

static INLINE VOID Anx_LoadFs(UINT16 Selector) {
    __asm__ __volatile__("mov %%ax, %%fs" : : "a"(Selector));
}

#           define ANX_CPU_WRITE_FSBASE(val)  Anx_WriteFsBase(val)
#           define ANX_CPU_READ_FSBASE()      Anx_ReadFsBase()
#           define ANX_CPU_LOAD_FS(sel)       Anx_LoadFs(sel)
#       endif

/* Task Register and GDT */
static INLINE VOID Anx_LoadTr(UINT16 Selector) {
    __asm__ __volatile__("ltr %%ax" : : "a"(Selector));
}

#       define ANX_CPU_LOAD_TR(sel)  Anx_LoadTr(sel)

#       if defined(__i386__)
static INLINE VOID Anx_LoadGdt(VOID* Ptr) {
    __asm__ __volatile__("lgdtl (%0)" : : "r"(Ptr));
}

#           define ANX_CPU_LOAD_GDT(ptr)  Anx_LoadGdt(ptr)
#       endif

/* Special Instructions */
#       define ANX_CPU_CLI_HLT()  __asm__ __volatile__("cli; hlt")
#       define ANX_CPU_STI_HLT()  __asm__ __volatile__("sti; hlt")
#       define ANX_CPU_UD2()      __asm__ __volatile__("ud2")

#   elif defined(__WATCOMC__)
/* Watcom C/C++ Compiler Support for x86 */

/* Control Register Access - Watcom uses auxiliary pragmas */
UINTN Anx_Watcom_ReadCr0(VOID);
VOID Anx_Watcom_WriteCr0(UINTN Val);
UINTN Anx_Watcom_ReadCr3(VOID);
VOID Anx_Watcom_WriteCr3(UINTN Val);
UINTN Anx_Watcom_ReadCr4(VOID);
VOID Anx_Watcom_WriteCr4(UINTN Val);

#       pragma aux Anx_Watcom_ReadCr0 = \
            "mov eax, cr0"              \
            value [eax]                 \
            modify exact [eax];

#       pragma aux Anx_Watcom_WriteCr0 = \
            "mov cr0, eax"                \
            parm [eax]                    \
            modify exact [];

#       pragma aux Anx_Watcom_ReadCr3 = \
            "mov eax, cr3"              \
            value [eax]                 \
            modify exact [eax];

#       pragma aux Anx_Watcom_WriteCr3 = \
            "mov cr3, eax"                \
            parm [eax]                    \
            modify exact [];

#       pragma aux Anx_Watcom_ReadCr4 = \
            "mov eax, cr4"              \
            value [eax]                 \
            modify exact [eax];

#       pragma aux Anx_Watcom_WriteCr4 = \
            "mov cr4, eax"                \
            parm [eax]                    \
            modify exact [];

#       define ANX_CPU_READ_CR0()       Anx_Watcom_ReadCr0()
#       define ANX_CPU_WRITE_CR0(val)   Anx_Watcom_WriteCr0(val)
#       define ANX_CPU_READ_CR3()       Anx_Watcom_ReadCr3()
#       define ANX_CPU_WRITE_CR3(val)   Anx_Watcom_WriteCr3(val)
#       define ANX_CPU_READ_CR4()       Anx_Watcom_ReadCr4()
#       define ANX_CPU_WRITE_CR4(val)   Anx_Watcom_WriteCr4(val)

/* MSR Access */
UINT64 Anx_Watcom_ReadMsr(UINT32 Msr);
VOID Anx_Watcom_WriteMsr(UINT32 Msr, UINT32 Lo, UINT32 Hi);

#       pragma aux Anx_Watcom_ReadMsr = \
            "rdmsr"                      \
            parm [ecx]                   \
            value [edx eax]              \
            modify exact [eax edx];

#       pragma aux Anx_Watcom_WriteMsr = \
            "wrmsr"                       \
            parm [ecx] [eax] [edx]        \
            modify exact [];

static INLINE UINT64 Anx_Watcom_ReadMsrWrapper(UINT32 Msr) {
    return Anx_Watcom_ReadMsr(Msr);
}

static INLINE VOID Anx_Watcom_WriteMsrWrapper(UINT32 Msr, UINT64 Val) {
    Anx_Watcom_WriteMsr(Msr, (UINT32)Val, (UINT32)(Val >> 32));
}

#       define ANX_CPU_RDMSR(msr)       Anx_Watcom_ReadMsrWrapper(msr)
#       define ANX_CPU_WRMSR(msr, val)  Anx_Watcom_WriteMsrWrapper(msr, val)

/* I/O Port Access */
UINT8 Anx_Watcom_Inb(UINT16 Port);
VOID Anx_Watcom_Outb(UINT16 Port, UINT8 Val);
UINT16 Anx_Watcom_Inw(UINT16 Port);
VOID Anx_Watcom_Outw(UINT16 Port, UINT16 Val);
UINT32 Anx_Watcom_Inl(UINT16 Port);
VOID Anx_Watcom_Outl(UINT16 Port, UINT32 Val);

#       pragma aux Anx_Watcom_Inb = \
            "in al, dx"             \
            parm [dx]               \
            value [al]              \
            modify exact [al];

#       pragma aux Anx_Watcom_Outb = \
            "out dx, al"              \
            parm [dx] [al]            \
            modify exact [];

#       pragma aux Anx_Watcom_Inw = \
            "in ax, dx"             \
            parm [dx]               \
            value [ax]              \
            modify exact [ax];

#       pragma aux Anx_Watcom_Outw = \
            "out dx, ax"              \
            parm [dx] [ax]            \
            modify exact [];

#       pragma aux Anx_Watcom_Inl = \
            "in eax, dx"            \
            parm [dx]               \
            value [eax]             \
            modify exact [eax];

#       pragma aux Anx_Watcom_Outl = \
            "out dx, eax"             \
            parm [dx] [eax]           \
            modify exact [];

#       define ANX_CPU_INB(port)        Anx_Watcom_Inb(port)
#       define ANX_CPU_OUTB(port, val)  Anx_Watcom_Outb(port, val)
#       define ANX_CPU_INW(port)        Anx_Watcom_Inw(port)
#       define ANX_CPU_OUTW(port, val)  Anx_Watcom_Outw(port, val)
#       define ANX_CPU_INL(port)        Anx_Watcom_Inl(port)
#       define ANX_CPU_OUTL(port, val)  Anx_Watcom_Outl(port, val)

/* CPUID */
VOID Anx_Watcom_Cpuid(UINT32 Leaf, UINT32 SubLeaf, UINT32* Eax, UINT32* Ebx, UINT32* Ecx, UINT32* Edx);

#       pragma aux Anx_Watcom_Cpuid =   \
            "cpuid"                      \
            parm [eax] [ecx] [esi] [edi] [ebx] [edx] \
            modify exact [eax ebx ecx edx];

static INLINE VOID Anx_Watcom_CpuidWrapper(UINT32* Eax, UINT32* Ebx, UINT32* Ecx, UINT32* Edx) {
    UINT32 InEax = *Eax;
    UINT32 InEcx = *Ecx;
    Anx_Watcom_Cpuid(InEax, InEcx, Eax, Ebx, Ecx, Edx);
}

#       define ANX_CPU_CPUID(eax, ebx, ecx, edx)  Anx_Watcom_CpuidWrapper(eax, ebx, ecx, edx)

/* Special Instructions */
#       define ANX_CPU_CLI_HLT()  do { _disable(); _asm { hlt } } while(0)
#       define ANX_CPU_STI_HLT()  do { _enable(); _asm { hlt } } while(0)
#       define ANX_CPU_UD2()      _asm { ud2 }

/* Load TR */
VOID Anx_Watcom_LoadTr(UINT16 Selector);

#       pragma aux Anx_Watcom_LoadTr = \
            "ltr ax"                    \
            parm [ax]                   \
            modify exact [];

#       define ANX_CPU_LOAD_TR(sel)  Anx_Watcom_LoadTr(sel)

#       if defined(_M_IX86) || defined(__386__) || defined(__486__) || defined(__586__) || defined(__686__)
/* 32-bit x86 specific - Load FS and GDT */
VOID Anx_Watcom_LoadFs(UINT16 Selector);
VOID Anx_Watcom_LoadGdt(VOID* Ptr);
VOID Anx_Watcom_WriteFsBase(UINT32 Val);
UINT32 Anx_Watcom_ReadFsBase(VOID);

#           pragma aux Anx_Watcom_LoadFs = \
                "mov fs, ax"                \
                parm [ax]                   \
                modify exact [];

#           pragma aux Anx_Watcom_LoadGdt = \
                "lgdt fword ptr [eax]"       \
                parm [eax]                   \
                modify exact [];

#           pragma aux Anx_Watcom_WriteFsBase = \
                "mov fs:[0], eax"                \
                parm [eax]                       \
                modify exact [];

#           pragma aux Anx_Watcom_ReadFsBase = \
                "mov eax, fs:[0]"               \
                value [eax]                     \
                modify exact [eax];

#           define ANX_CPU_LOAD_FS(sel)       Anx_Watcom_LoadFs(sel)
#           define ANX_CPU_LOAD_GDT(ptr)      Anx_Watcom_LoadGdt(ptr)
#           define ANX_CPU_WRITE_FSBASE(val)  Anx_Watcom_WriteFsBase(val)
#           define ANX_CPU_READ_FSBASE()      Anx_Watcom_ReadFsBase()

/* Additional x86-32 operations */
VOID Anx_Watcom_WriteGs(UINT16 Selector);
UINT16 Anx_Watcom_ReadGs(VOID);

#           pragma aux Anx_Watcom_WriteGs = \
                "mov gs, ax"                 \
                parm [ax]                    \
                modify exact [];

#           pragma aux Anx_Watcom_ReadGs = \
                "mov ax, gs"                \
                value [ax]                  \
                modify exact [ax];

#           define ANX_CPU_WRITE_GS(sel)  Anx_Watcom_WriteGs(sel)
#           define ANX_CPU_READ_GS()      Anx_Watcom_ReadGs()

#       endif /* _M_IX86 */

#       if defined(_M_X64) || defined(__X86_64__)
/* 64-bit x86 specific - GS base access */
VOID Anx_Watcom_WriteGsBase(UINTN Val);
UINTN Anx_Watcom_ReadGsBase(VOID);

#           pragma aux Anx_Watcom_WriteGsBase = \
                "mov gs:[0], rax"                \
                parm [rax]                       \
                modify exact [];

#           pragma aux Anx_Watcom_ReadGsBase = \
                "mov rax, gs:[0]"               \
                value [rax]                     \
                modify exact [rax];

#           define ANX_CPU_WRITE_GSBASE(val)  Anx_Watcom_WriteGsBase(val)
#           define ANX_CPU_READ_GSBASE()      Anx_Watcom_ReadGsBase()
#       endif /* _M_X64 */

#   elif defined(_MSC_VER)
/* Microsoft Visual C++ Compiler Support for x86/x64 */

#       include <intrin.h>

/* Control Register Access - MSVC provides __readcr/__ writecr intrinsics */
#       define ANX_CPU_READ_CR0()       __readcr0()
#       define ANX_CPU_WRITE_CR0(val)   __writecr0(val)
#       define ANX_CPU_READ_CR3()       __readcr3()
#       define ANX_CPU_WRITE_CR3(val)   __writecr3(val)
#       define ANX_CPU_READ_CR4()       __readcr4()
#       define ANX_CPU_WRITE_CR4(val)   __writecr4(val)

/* MSR Access */
#       define ANX_CPU_RDMSR(msr)       __readmsr(msr)
#       define ANX_CPU_WRMSR(msr, val)  __writemsr(msr, val)

/* I/O Port Access */
#       define ANX_CPU_INB(port)        __inbyte(port)
#       define ANX_CPU_OUTB(port, val)  __outbyte(port, val)
#       define ANX_CPU_INW(port)        __inword(port)
#       define ANX_CPU_OUTW(port, val)  __outword(port, val)
#       define ANX_CPU_INL(port)        __indword(port)
#       define ANX_CPU_OUTL(port, val)  __outdword(port, val)

/* CPUID */
static INLINE VOID Anx_Msvc_Cpuid(UINT32* Eax, UINT32* Ebx, UINT32* Ecx, UINT32* Edx) {
    INT32 CpuInfo[4];
    __cpuidex(CpuInfo, *Eax, *Ecx);
    *Eax = CpuInfo[0];
    *Ebx = CpuInfo[1];
    *Ecx = CpuInfo[2];
    *Edx = CpuInfo[3];
}

#       define ANX_CPU_CPUID(eax, ebx, ecx, edx)  Anx_Msvc_Cpuid(eax, ebx, ecx, edx)

/* Special Instructions */
#       define ANX_CPU_CLI_HLT()  do { _disable(); __halt(); } while(0)
#       define ANX_CPU_STI_HLT()  do { _enable(); __halt(); } while(0)
#       define ANX_CPU_UD2()      __ud2()

/* Load TR */
#       pragma intrinsic(__ltr)
static INLINE VOID Anx_Msvc_LoadTr(UINT16 Selector) {
    __ltr(Selector);
}

#       define ANX_CPU_LOAD_TR(sel)  Anx_Msvc_LoadTr(sel)

#       if defined(_M_IX86)
/* 32-bit specific - Load FS and GDT */
static INLINE VOID Anx_Msvc_LoadFs(UINT16 Selector) {
    __asm mov ax, Selector
    __asm mov fs, ax
}

static INLINE VOID Anx_Msvc_LoadGdt(VOID* Ptr) {
    __asm {
        mov eax, Ptr
        lgdt [eax]
    }
}

#           define ANX_CPU_LOAD_FS(sel)       Anx_Msvc_LoadFs(sel)
#           define ANX_CPU_LOAD_GDT(ptr)      Anx_Msvc_LoadGdt(ptr)
#           define ANX_CPU_WRITE_FSBASE(val)  (__writefsdword(0, val))
#           define ANX_CPU_READ_FSBASE()      (__readfsdword(0))
#       endif

#       if defined(_M_X64)
/* 64-bit specific - GS base access */
#           define ANX_CPU_WRITE_GSBASE(val)  (__writegsqword(0, val))
#           define ANX_CPU_READ_GSBASE()      (__readgsqword(0))
#       endif

#   endif /* _MSC_VER */
#endif /* __x86_64__ || __i386__ */

/* ---------------------------------------------------------------
 *  ARM/ARM64-Specific CPU Operations (MSVC)
 * --------------------------------------------------------------- */

#if (defined(_M_ARM) || defined(_M_ARM64)) && defined(_MSC_VER)
#   include <intrin.h>

/* Memory Barriers */
#   define ANX_CPU_ARM_DMB()  __dmb(_ARM_BARRIER_SY)
#   define ANX_CPU_ARM_DSB()  __dsb(_ARM_BARRIER_SY)
#   define ANX_CPU_ARM_ISB()  __isb(_ARM_BARRIER_SY)

/* Wait for interrupt/event */
#   define ANX_CPU_ARM_WFI()  __wfi()
#   define ANX_CPU_ARM_WFE()  __wfe()
#   define ANX_CPU_ARM_SEV()  __sev()

/* Breakpoint */
#   if defined(_M_ARM64)
#       define ANX_CPU_BREAKPOINT()  __break(0)
#   elif defined(_M_ARM)
#       define ANX_CPU_BREAKPOINT()  __debugbreak()
#   endif

/* Yield */
#   define ANX_CPU_ARM_YIELD()  __yield()

#   if defined(_M_ARM64)
/* ARM64-specific system register access */
#       define ANX_CPU_ARM64_READ_SYSREG(reg)   _ReadStatusReg(reg)
#       define ANX_CPU_ARM64_WRITE_SYSREG(reg, val)  _WriteStatusReg(reg, val)

/* Common ARM64 system registers */
#       define ANX_CPU_ARM64_READ_TPIDR_EL0()   _ReadStatusReg(ARM64_TPIDR_EL0)
#       define ANX_CPU_ARM64_WRITE_TPIDR_EL0(val)  _WriteStatusReg(ARM64_TPIDR_EL0, val)
#   endif

#endif /* (_M_ARM || _M_ARM64) && _MSC_VER */

/* ---------------------------------------------------------------
 *  RISC-V Specific CPU Operations
 * --------------------------------------------------------------- */

#if defined(__riscv)
#   if defined(__GNUC__) || defined(__clang__)

/* CSR Read/Write Operations */
#       define ANX_CPU_CSR_READ(csr, val) \
            __asm__ __volatile__("csrr %0, " #csr : "=r"(val))

#       define ANX_CPU_CSR_WRITE(csr, val) \
            __asm__ __volatile__("csrw " #csr ", %0" : : "r"(val))

#       define ANX_CPU_CSR_SET(csr, val) \
            __asm__ __volatile__("csrs " #csr ", %0" : : "r"(val) : "memory")

#       define ANX_CPU_CSR_CLEAR(csr, val) \
            __asm__ __volatile__("csrc " #csr ", %0" : : "r"(val) : "memory")

#       define ANX_CPU_CSR_SET_IMM(csr, imm) \
            __asm__ __volatile__("csrsi " #csr ", %0" : : "K"(imm))

#       define ANX_CPU_CSR_CLEAR_IMM(csr, imm) \
            __asm__ __volatile__("csrci " #csr ", %0" : : "K"(imm))

/* CSR Read-Modify-Write with Return */
static INLINE UINTN Anx_CsrReadClearImm(CONST CHAR8* Csr, UINTN Imm) {
    UINTN Old;
    if (Imm == 2) {
        __asm__ __volatile__("csrrci %0, sstatus, 2" : "=r"(Old));
    } else if (Imm == 32) {
        __asm__ __volatile__("csrrci %0, sstatus, 32" : "=r"(Old));
    }
    return Old;
}

static INLINE UINTN Anx_CsrReadSetImm(CONST CHAR8* Csr, UINTN Imm) {
    UINTN Old;
    if (Imm == 2) {
        __asm__ __volatile__("csrrsi %0, sstatus, 2" : "=r"(Old));
    } else if (Imm == 32) {
        __asm__ __volatile__("csrrsi %0, sstatus, 32" : "=r"(Old));
    }
    return Old;
}

static INLINE UINTN Anx_CsrReadWrite(CONST CHAR8* Csr, UINTN Val) {
    UINTN Old;
    /* Note: This is a simplified version - full implementation would need
     * to handle different CSRs. For now, we support the common ones. */
    __asm__ __volatile__("csrrw %0, sie, %1" : "=r"(Old) : "r"(Val));
    return Old;
}

#       define ANX_CPU_CSR_RCI_SSTATUS(imm)  Anx_CsrReadClearImm("sstatus", imm)
#       define ANX_CPU_CSR_RSI_SSTATUS(imm)  Anx_CsrReadSetImm("sstatus", imm)
#       define ANX_CPU_CSR_RW_SIE(val, old)  do { old = Anx_CsrReadWrite("sie", val); } while(0)

/* Thread Pointer (TP register) */
static INLINE VOID Anx_SetTp(UINTN Val) {
    __asm__ __volatile__("mv tp, %0" : : "r"(Val));
}

static INLINE UINTN Anx_GetTp(VOID) {
    UINTN Val;
    __asm__ __volatile__("mv %0, tp" : "=r"(Val));
    return Val;
}

#       define ANX_CPU_SET_TP(val)  Anx_SetTp(val)
#       define ANX_CPU_GET_TP()     Anx_GetTp()

/* Special RISC-V Instructions */
#       define ANX_CPU_WFI()  __asm__ __volatile__("wfi")
#       define ANX_CPU_NOP()  __asm__ __volatile__("nop")

/* SBI Console (for early boot) */
static INLINE VOID Anx_SbiConsolePutchar(CHAR8 Ch) {
    __asm__ __volatile__("mv a0, %0\n"
                         "li a7, 1\n"
                         "ecall"
                         : : "r"(Ch) : "a0", "a7");
}

#       define ANX_CPU_SBI_PUTCHAR(ch)  Anx_SbiConsolePutchar(ch)

/* RISC-V special idle/halt sequences */
#       define ANX_CPU_RISCV_ENABLE_INTR_WFI() \
            __asm__ __volatile__("csrsi sstatus, 0x2; wfi")

#       define ANX_CPU_RISCV_DISABLE_INTR_LOOP() \
            __asm__ __volatile__("csrci sstatus, 0x2; 1: j 1b")

/* CSR sscratch with TP */
static INLINE VOID Anx_WriteSscratchTp(VOID) {
    UINTN Tp = ANX_CPU_GET_TP();
    __asm__ __volatile__("csrw sscratch, %0" : : "r"(Tp));
}

#       define ANX_CPU_WRITE_SSCRATCH_TP()  Anx_WriteSscratchTp()

#   endif /* __GNUC__ || __clang__ */
#endif /* __riscv */

/* ---------------------------------------------------------------
 *  Memory Prefetch Instructions
 * --------------------------------------------------------------- */

/**
  Prefetch memory for reading.

  @param[in] addr  Address to prefetch
**/
#if defined(__GNUC__) || defined(__clang__)
#   define ANX_CPU_PREFETCH_READ(addr)  __builtin_prefetch((addr), 0, 3)
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64))
#   include <intrin.h>
#   define ANX_CPU_PREFETCH_READ(addr)  _mm_prefetch((const char *)(addr), _MM_HINT_T0)
#else
#   define ANX_CPU_PREFETCH_READ(addr)  ((void)(addr))
#endif

/**
  Prefetch memory for writing.

  @param[in] addr  Address to prefetch
**/
#if defined(__GNUC__) || defined(__clang__)
#   define ANX_CPU_PREFETCH_WRITE(addr)  __builtin_prefetch((addr), 1, 3)
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64))
#   include <intrin.h>
#   define ANX_CPU_PREFETCH_WRITE(addr)  _mm_prefetch((const char *)(addr), _MM_HINT_T0)
#else
#   define ANX_CPU_PREFETCH_WRITE(addr)  ((void)(addr))
#endif

