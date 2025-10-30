/++
    Module Name:

        compiler.h

    Abstract:

        Compiler detection and INLINE macro definitions.

    Environment:

        Compiler-agnostic: MSVC, Clang, GCC, Watcom.
--/

#pragma once

/* --------------------------------------------------------------- */
/*  Compiler detection.                                            */
/* --------------------------------------------------------------- */
#if defined(_MSC_VER)
#   define ANX_CC_MSVC 1
#elif defined(__clang__)
#   define ANX_CC_CLANG 1
#elif defined(__GNUC__)
#   define ANX_CC_GNU 1
#elif defined(__WATCOMC__)
#   define ANX_CC_WATCOM 1
#else
#   define ANX_CC_UNKNOWN 1
#endif

/* --------------------------------------------------------------- */
/*  INLINE selection.                                              */
/* --------------------------------------------------------------- */
#if defined(__cplusplus)
#   define INLINE inline
#else
#   if defined(ANX_CC_MSVC)
#       define INLINE __inline
#   elif defined(ANX_CC_GNU) || defined(ANX_CC_CLANG)
#       define INLINE __inline__
#   else
#       define INLINE /* no inline */
#   endif
#endif

/* --------------------------------------------------------------- */
/*  Structure packing control (portable).                          */
/* --------------------------------------------------------------- */
#if defined(_MSC_VER)
#   define ANX_PACK_PUSH(n)     __pragma(pack(push, n))
#   define ANX_PACK_POP()       __pragma(pack(pop))
#   define ANX_PACKED           /* use pragma pack instead */
#elif defined(__GNUC__) || defined(__clang__)
#   define ANX_PACK_PUSH(n)     _Pragma("pack(push)") _Pragma("pack(1)")
#   define ANX_PACK_POP()       _Pragma("pack(pop)")
#   define ANX_PACKED           __attribute__((packed))
#elif defined(__WATCOMC__)
#   define ANX_PACK_PUSH(n)     /* use explicit #pragma pack */
#   define ANX_PACK_POP()       /* use explicit #pragma pack */
#   define ANX_PACKED           /* use pragma pack instead */
#else
#   define ANX_PACK_PUSH(n)
#   define ANX_PACK_POP()
#   define ANX_PACKED
#endif
