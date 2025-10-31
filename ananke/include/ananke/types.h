/*++
    Module Name:

        types.h

    Abstract:

        UEFI-width base types, SAL annotations, and pointer/reference families.

    Environment:

        C89-C23, C++98-C++23 compatible.
--*/

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* --------------------------------------------------------------- */
/*  SAL-lite annotations and BOOLEAN.                              */
/* --------------------------------------------------------------- */
#ifndef IN
#   define IN
#endif
#ifndef OUT
#   define OUT
#endif
#ifndef OPTIONAL
#   define OPTIONAL
#endif
#ifndef CONST
#   define CONST const
#endif
#ifndef VOLATILE
#   define VOLATILE volatile
#endif
#ifndef VOID
#   define VOID void
#endif

typedef bool BOOLEAN;
#ifndef TRUE
#   define TRUE true
#endif
#ifndef FALSE
#   define FALSE false
#endif

/* --------------------------------------------------------------- */
/*  Architecture width detection (32 vs 64-bit).                   */
/* --------------------------------------------------------------- */
#if defined(_M_X64) || defined(__x86_64__) || defined(__aarch64__) || defined(__ppc64__) || defined(__LP64__) || defined(_LP64)
#   define ANX_ARCH_64 1
#else
#   define ANX_ARCH_32 1
#endif

/* --------------------------------------------------------------- */
/*  UEFI-width base types.                                         */
/* --------------------------------------------------------------- */
typedef uint8_t     UINT8;  typedef int8_t   INT8;
typedef uint16_t    UINT16; typedef int16_t  INT16;
typedef uint32_t    UINT32; typedef int32_t  INT32;
typedef uint64_t    UINT64; typedef int64_t  INT64;
#if defined(ANX_ARCH_64)
    typedef uint64_t UINTN; typedef int64_t  INTN;
#else
    typedef uint32_t UINTN; typedef int32_t  INTN;
#endif

typedef UINTN   SIZE_T;  typedef INTN  SSIZE_T;
typedef UINT8   CHAR8;   /* UTF-8 code unit */
typedef UINT16  CHAR16;  /* UTF-16 code unit */
typedef UINT16  WCHAR;   /* Windows/UEFI wide char */
typedef UINT32  CHAR32;  /* UTF-32 code unit */

/* --------------------------------------------------------------- */
/*  NT pointer families: P<Type>, PC<Type>, PV<Type>, PCV<Type>.  */
/* --------------------------------------------------------------- */
#define ANX_DECLARE_TPTRS(T) \
    typedef T*                 P##T;    \
    typedef CONST T*           PC##T;   \
    typedef VOLATILE T*        PV##T;   \
    typedef CONST VOLATILE T*  PCV##T

ANX_DECLARE_TPTRS(UINT8);  ANX_DECLARE_TPTRS(UINT16);  ANX_DECLARE_TPTRS(UINT32);  ANX_DECLARE_TPTRS(UINT64);
ANX_DECLARE_TPTRS(INT8);   ANX_DECLARE_TPTRS(INT16);   ANX_DECLARE_TPTRS(INT32);   ANX_DECLARE_TPTRS(INT64);
ANX_DECLARE_TPTRS(CHAR8);  ANX_DECLARE_TPTRS(CHAR16);  ANX_DECLARE_TPTRS(CHAR32);  ANX_DECLARE_TPTRS(WCHAR);

/* --------------------------------------------------------------- */
/*  C++ reference families: R<Type>, RC<Type>, RV<Type>, RCV<Type>.*/
/* --------------------------------------------------------------- */
#ifdef __cplusplus
#   define ANX_DECLARE_TREFS(T) \
        typedef T&                 R##T;  \
        typedef const T&           RC##T; \
        typedef volatile T&        RV##T; \
        typedef const volatile T&  RCV##T
ANX_DECLARE_TREFS(UINT8);  ANX_DECLARE_TREFS(UINT16);  ANX_DECLARE_TREFS(UINT32);  ANX_DECLARE_TREFS(UINT64);
ANX_DECLARE_TREFS(INT8);   ANX_DECLARE_TREFS(INT16);   ANX_DECLARE_TREFS(INT32);   ANX_DECLARE_TREFS(INT64);
ANX_DECLARE_TREFS(CHAR8);  ANX_DECLARE_TREFS(CHAR16);  ANX_DECLARE_TREFS(CHAR32);  ANX_DECLARE_TREFS(WCHAR);
#endif

/* --------------------------------------------------------------- */
/*  Native-size integer constants (unprefixed).                    */
/* --------------------------------------------------------------- */
#if defined(ANX_ARCH_64)
#   define UINTN_C(x)  UINT64_C(x)
#   define INTN_C(x)   INT64_C(x)
#else
#   define UINTN_C(x)  UINT32_C(x)
#   define INTN_C(x)   INT32_C(x)
#endif
