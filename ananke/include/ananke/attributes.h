/++
    Module Name:

        attributes.h

    Abstract:

        Unified compiler attributes, declspecs, and branch/trap intrinsics.
        All exported macros are ANX_-prefixed and ALWAYS defined.

    Environment:

        C and C++ compatible.
--/

#pragma once

#include <ananke/compiler.h>

/* --------------------------------------------------------------- */
/*  Unified attributes / declspecs / branch & trap intrinsics.     */
/*  All exported macros are ANX_-prefixed and ALWAYS defined.       */
/* --------------------------------------------------------------- */

/* First, undef any prior definitions to avoid conflicts. */
#undef ANX_ATTR_NOINLINE
#undef ANX_ATTR_FORCEINLINE
#undef ANX_ATTR_DEPRECATED
#undef ANX_ATTR_ALIGN
#undef ANX_ATTR_RESTRICT
#undef ANX_ATTR_NODISCARD
#undef ANX_ATTR_NORETURN
#undef ANX_ATTR_PUREFN
#undef ANX_ATTR_CONSTFN
#undef ANX_ATTR_NONNULL
#undef ANX_ATTR_RETNONNULL
#undef ANX_ATTR_FALLTHROUGH
#undef ANX_ATTR_FORMAT
#undef ANX_ATTR_SECTION
#undef ANX_DECL_EXPORT
#undef ANX_DECL_IMPORT
#undef ANX_DECL_THREAD
#undef ANX_ASSUME
#undef ANX_UNREACHABLE
#undef ANX_TRAP
#undef ANX_PREFETCH
#undef ANX_LIKELY
#undef ANX_UNLIKELY

/* Compiler-specific mapping with safe fallbacks. */
#if defined(_MSC_VER)
    /* Attributes / declspecs */
#   define ANX_ATTR_NOINLINE           __declspec(noinline)
#   define ANX_ATTR_FORCEINLINE        __forceinline
#   define ANX_ATTR_DEPRECATED(msg)    __declspec(deprecated(msg))
#   define ANX_ATTR_ALIGN(n)           __declspec(align(n))
#   define ANX_ATTR_RESTRICT           __restrict
#   define ANX_ATTR_NODISCARD          /* no native in older MSVC; use SAL if needed */
#   define ANX_ATTR_NORETURN           __declspec(noreturn)
#   define ANX_ATTR_PUREFN             /* none */
#   define ANX_ATTR_CONSTFN            /* none */
#   define ANX_ATTR_NONNULL(...)       /* none */
#   define ANX_ATTR_RETNONNULL         /* none */
#   define ANX_ATTR_FALLTHROUGH        /* none */
#   define ANX_ATTR_FORMAT(a,f,idx)    /* none */
#   define ANX_ATTR_SECTION(x)         __declspec(allocate(x))
#   define ANX_DECL_EXPORT             __declspec(dllexport)
#   define ANX_DECL_IMPORT             __declspec(dllimport)
#   if defined(_MSC_VER) && (_MSC_VER >= 1900)
#       define ANX_DECL_THREAD         __declspec(thread)
#   else
#       define ANX_DECL_THREAD         __declspec(thread)
#   endif
    /* Branch & trap intrinsics */
#   define ANX_ASSUME(x)               __assume(x)
#   define ANX_UNREACHABLE()           __assume(0)
#   define ANX_TRAP()                  __debugbreak()
#   define ANX_PREFETCH(p, rw, lvl)    /* no portable prefetch on old MSVC */
#   define ANX_LIKELY(x)               (x)
#   define ANX_UNLIKELY(x)             (x)

#elif defined(__clang__) || defined(__GNUC__)
    /* Attributes / declspecs */
#   define ANX_ATTR_NOINLINE           __attribute__((noinline))
#   define ANX_ATTR_FORCEINLINE        INLINE __attribute__((always_inline))
#   define ANX_ATTR_DEPRECATED(msg)    __attribute__((deprecated(msg)))
#   define ANX_ATTR_ALIGN(n)           __attribute__((aligned(n)))
#   define ANX_ATTR_RESTRICT           __restrict__
#   define ANX_ATTR_NODISCARD          __attribute__((warn_unused_result))
#   define ANX_ATTR_NORETURN           __attribute__((noreturn))
#   define ANX_ATTR_PUREFN             __attribute__((pure))
#   define ANX_ATTR_CONSTFN            __attribute__((const))
#   define ANX_ATTR_NONNULL(...)       __attribute__((nonnull(__VA_ARGS__)))
#   define ANX_ATTR_RETNONNULL         __attribute__((returns_nonnull))
#   define ANX_ATTR_FALLTHROUGH        __attribute__((fallthrough))
#   define ANX_ATTR_FORMAT(a,f,idx)    __attribute__((format(a,f,idx)))
#   define ANX_ATTR_SECTION(x)         __attribute__((section(x)))
#   define ANX_DECL_EXPORT             __attribute__((visibility("default")))
#   define ANX_DECL_IMPORT             /* same visibility */
#   if defined(__STDC_NO_THREADS__)
#       define ANX_DECL_THREAD         /* none */
#   else
#       define ANX_DECL_THREAD         _Thread_local
#   endif
    /* Branch & trap intrinsics */
#   define ANX_ASSUME(x)               do{ if(!(x)) __builtin_unreachable(); }while(0)
#   define ANX_UNREACHABLE()           __builtin_unreachable()
#   define ANX_TRAP()                  __builtin_trap()
#   define ANX_PREFETCH(p, rw, lvl)    __builtin_prefetch((p),(rw),(lvl))
#   define ANX_LIKELY(x)               __builtin_expect(!!(x),1)
#   define ANX_UNLIKELY(x)             __builtin_expect(!!(x),0)

#elif defined(__WATCOMC__)
    /* Attributes / declspecs (limited) */
#   define ANX_ATTR_NOINLINE           /* use pragmas if needed */
#   define ANX_ATTR_FORCEINLINE        INLINE
#   define ANX_ATTR_DEPRECATED(msg)    /* none */
#   define ANX_ATTR_ALIGN(n)           /* use pragma pack */
#   define ANX_ATTR_RESTRICT           /* none */
#   define ANX_ATTR_NODISCARD          /* none */
#   define ANX_ATTR_NORETURN           _Noreturn
#   define ANX_ATTR_PUREFN             /* none */
#   define ANX_ATTR_CONSTFN            /* none */
#   define ANX_ATTR_NONNULL(...)       /* none */
#   define ANX_ATTR_RETNONNULL         /* none */
#   define ANX_ATTR_FALLTHROUGH        /* none */
#   define ANX_ATTR_FORMAT(a,f,idx)    /* none */
#   define ANX_ATTR_SECTION(x)         /* none */
#   define ANX_DECL_EXPORT             /* none */
#   define ANX_DECL_IMPORT             /* none */
#   define ANX_DECL_THREAD             /* none */
    /* Branch & trap intrinsics */
#   define ANX_ASSUME(x)               ((void)0)
#   define ANX_UNREACHABLE()           for(;;){}
#   define ANX_TRAP()                  do{ volatile int* _p=0; *_p=1; }while(0)
#   define ANX_PREFETCH(p, rw, lvl)    ((void)0)
#   define ANX_LIKELY(x)               (x)
#   define ANX_UNLIKELY(x)             (x)

#else
    /* Generic safe fallbacks */
#   define ANX_ATTR_NOINLINE
#   define ANX_ATTR_FORCEINLINE        INLINE
#   define ANX_ATTR_DEPRECATED(msg)
#   define ANX_ATTR_ALIGN(n)
#   define ANX_ATTR_RESTRICT
#   define ANX_ATTR_NODISCARD
#   define ANX_ATTR_NORETURN
#   define ANX_ATTR_PUREFN
#   define ANX_ATTR_CONSTFN
#   define ANX_ATTR_NONNULL(...)
#   define ANX_ATTR_RETNONNULL
#   define ANX_ATTR_FALLTHROUGH
#   define ANX_ATTR_FORMAT(a,f,idx)
#   define ANX_ATTR_SECTION(x)
#   define ANX_DECL_EXPORT
#   define ANX_DECL_IMPORT
#   define ANX_DECL_THREAD
#   define ANX_ASSUME(x)               ((void)0)
#   define ANX_UNREACHABLE()           do{}while(0)
#   define ANX_TRAP()                  do{ volatile int* _p=0; *_p=1; }while(0)
#   define ANX_PREFETCH(p, rw, lvl)    ((void)0)
#   define ANX_LIKELY(x)               (x)
#   define ANX_UNLIKELY(x)             (x)
#endif
