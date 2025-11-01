/** @file
  cxxCRT - C++ Runtime Library

  MSVC demangling wrappers

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>

#if defined(_MSC_VER)

/* NTRTL demanglers */
extern UINTN EFIAPI RtlDemangleName(const CHAR8 *, CHAR8 *, UINTN);
extern UINTN EFIAPI RtlDemangleNameMSVC(const CHAR8 *, CHAR8 *, UINTN);

/**
 * __unDName - MSVC undecorated name function
 *
 * @param outputString  - Output buffer
 * @param name          - Mangled name
 * @param maxStringLength - Output buffer size
 * @param mallocFnPtr   - Unused (no allocation)
 * @param freeFnPtr     - Unused (no allocation)
 * @param disableFlags  - Unused
 *
 * @return Pointer to output buffer, or NULL on error
 */
char * __cdecl
__unDName(
    char *outputString,
    const char *name,
    int maxStringLength,
    void *mallocFnPtr,
    void *freeFnPtr,
    unsigned short disableFlags
    )
{
    UINTN Result;

    if (name == NULL || outputString == NULL || maxStringLength <= 0) {
        return NULL;
    }

    /* Attempt demangling */
    Result = RtlDemangleNameMSVC(
        (const CHAR8 *)name,
        (CHAR8 *)outputString,
        (UINTN)maxStringLength
    );

    return (Result > 0) ? outputString : NULL;
}

/**
 * UnDecorateSymbolName - Public MSVC demangling API
 *
 * @param DecoratedName - Mangled name
 * @param UnDecoratedName - Output buffer
 * @param UndecoratedLength - Output buffer size
 * @param Flags - Unused
 *
 * @return Length of demangled name, or 0 on error
 */
unsigned long __stdcall
UnDecorateSymbolName(
    const char *DecoratedName,
    char *UnDecoratedName,
    unsigned long UndecoratedLength,
    unsigned long Flags
    )
{
    return (unsigned long)RtlDemangleNameMSVC(
        (const CHAR8 *)DecoratedName,
        (CHAR8 *)UnDecoratedName,
        (UINTN)UndecoratedLength
    );
}

#endif /* _MSC_VER */
