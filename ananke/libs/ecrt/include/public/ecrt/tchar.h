/** @file
  eCRT - An embedded C runtime library

  Generic text mapping header (tchar.h)

  Provides generic text mappings for both char (ANSI) and wchar_t (Unicode)
  character types. Based on MSVCRT tchar.h but simplified for embedded use.

  Define _UNICODE or UNICODE to use wide character versions.
  Otherwise, ANSI (char) versions are used by default.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __ecrt_tchar_h__
#define __ecrt_tchar_h__

#include <stddef.h>

/* Determine if we're using Unicode */
#if defined(_UNICODE) || defined(UNICODE)
#   ifndef _UNICODE
#       define _UNICODE
#   endif
#   ifndef UNICODE
#       define UNICODE
#   endif
#   define _TCHAR_DEFINED
#endif

/* ---------------------------------------------------------------
 *  Generic text type definitions
 * --------------------------------------------------------------- */

#ifdef _UNICODE
    /* Wide character (Unicode) mode */
    typedef wchar_t             TCHAR;
    typedef wchar_t             _TCHAR;
    typedef wchar_t             TBYTE;
    typedef wchar_t             _TBYTE;
    typedef wchar_t             TINT;
    typedef wchar_t             _TINT;
    typedef wchar_t             TSCHAR;
    typedef wchar_t             _TSCHAR;
    typedef wchar_t             TUCHAR;
    typedef wchar_t             _TUCHAR;
    typedef wchar_t             TXCHAR;
    typedef wchar_t             _TXCHAR;

#else
    /* ANSI character mode */
    typedef char                TCHAR;
    typedef char                _TCHAR;
    typedef unsigned char       TBYTE;
    typedef unsigned char       _TBYTE;
    typedef int                 TINT;
    typedef int                 _TINT;
    typedef signed char         TSCHAR;
    typedef signed char         _TSCHAR;
    typedef unsigned char       TUCHAR;
    typedef unsigned char       _TUCHAR;
    typedef char                TXCHAR;
    typedef char                _TXCHAR;
#endif

/* Pointer types */
typedef TCHAR *                 PTCHAR;
typedef TCHAR *                 LPTCH;
typedef TCHAR *                 LPTSTR;
typedef const TCHAR *           LPCTSTR;

/* ---------------------------------------------------------------
 *  Generic text macros for string literals
 * --------------------------------------------------------------- */

#ifdef _UNICODE
#   define __T(x)               L##x
#else
#   define __T(x)               x
#endif

#define _T(x)                   __T(x)
#define _TEXT(x)                __T(x)

/* ---------------------------------------------------------------
 *  Generic text function mappings
 * --------------------------------------------------------------- */

#ifdef _UNICODE

    /* String functions */
#   define _tcslen              wcslen
#   define _tcsnlen             wcsnlen
#   define _tcscpy              wcscpy
#   define _tcsncpy             wcsncpy
#   define _tcslcpy             wcslcpy
#   define _tcscat              wcscat
#   define _tcschr              wcschr
#   define _tcsrchr             wcsrchr
#   define _tcscmp              wcscmp
#   define _tcsncmp             wcsncmp
#   define _tcscspn             wcscspn
#   define _tcsdup              _wcsdup

    /* Case conversion */
#   define _tcslwr              _wcslwr
#   define _tcsupr              _wcsupr
#   define _tcsrev              _wcsrev

    /* Case-insensitive comparison */
#   define _tcsicmp             _wcsicmp
#   define _tcsnicmp            _wcsnicmp

    /* Character classification (would need wctype.h) */
#   define _istspace            iswspace
#   define _istdigit            iswdigit
#   define _istalpha            iswalpha
#   define _istalnum            iswalnum

    /* Character operations */
#   define _totupper            towupper
#   define _totlower            towlower

    /* I/O functions (if stdio is available) */
#   define _tprintf             wprintf
#   define _ftprintf            fwprintf
#   define _stprintf            swprintf
#   define _sntprintf           snwprintf
#   define _vtprintf            vwprintf
#   define _vftprintf           vfwprintf
#   define _vstprintf           vswprintf
#   define _vsntprintf          vsnwprintf

#   define _tscanf              wscanf
#   define _ftscanf             fwscanf
#   define _stscanf             swscanf

    /* File operations */
#   define _tfopen              _wfopen
#   define _tfreopen            _wfreopen
#   define _tfsopen             _wfsopen

    /* Character constants */
#   define _TEOF                WEOF

#else  /* _UNICODE */

    /* String functions */
#   define _tcslen              strlen
#   define _tcsnlen             strnlen
#   define _tcscpy              strcpy
#   define _tcsncpy             strncpy
#   define _tcslcpy             strlcpy
#   define _tcscat              strcat
#   define _tcschr              strchr
#   define _tcsrchr             strrchr
#   define _tcscmp              strcmp
#   define _tcsncmp             strncmp
#   define _tcscspn             strcspn
#   define _tcsdup              _strdup

    /* Case conversion */
#   define _tcslwr              _strlwr
#   define _tcsupr              _strupr
#   define _tcsrev              _strrev

    /* Case-insensitive comparison */
#   define _tcsicmp             _stricmp
#   define _tcsnicmp            _strnicmp

    /* Character classification */
#   define _istspace            isspace
#   define _istdigit            isdigit
#   define _istalpha            isalpha
#   define _istalnum            isalnum

    /* Character operations */
#   define _totupper            toupper
#   define _totlower            tolower

    /* I/O functions (if stdio is available) */
#   define _tprintf             printf
#   define _ftprintf            fprintf
#   define _stprintf            sprintf
#   define _sntprintf           snprintf
#   define _vtprintf            vprintf
#   define _vftprintf           vfprintf
#   define _vstprintf           vsprintf
#   define _vsntprintf          vsnprintf

#   define _tscanf              scanf
#   define _ftscanf             fscanf
#   define _stscanf             sscanf

    /* File operations */
#   define _tfopen              fopen
#   define _tfreopen            freopen
#   define _tfsopen             _fsopen

    /* Character constants */
#   define _TEOF                EOF

#endif /* _UNICODE */

/* ---------------------------------------------------------------
 *  Deprecated/legacy names (for compatibility)
 * --------------------------------------------------------------- */

#ifdef _UNICODE
#   define _tcscpy_s            wcscpy_s
#   define _tcscat_s            wcscat_s
#   define _tcsncpy_s           wcsncpy_s
#else
#   define _tcscpy_s            strcpy_s
#   define _tcscat_s            strcat_s
#   define _tcsncpy_s           strncpy_s
#endif

/* ---------------------------------------------------------------
 *  Additional platform-specific mappings
 * --------------------------------------------------------------- */

/* main() function */
#ifdef _UNICODE
#   define _tmain               wmain
#   define _tWinMain            wWinMain
#else
#   define _tmain               main
#   define _tWinMain            WinMain
#endif

/* Command line arguments */
#ifdef _UNICODE
#   define _targv               __wargv
#   define _tenvp               __wenvp
#else
#   define _targv               __argv
#   define _tenvp               __envp
#endif

#endif /* __ecrt_tchar_h__ */
