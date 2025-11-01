/** @file
  NTRTL - NT Runtime Library

  Unified C++ Name Demangling

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

/*
 * Unified Demangler
 *
 * Attempts to demangle using all supported ABI schemes:
 * 1. Itanium C++ ABI (GCC, Clang, ICC) - starts with "_Z"
 * 2. MSVC C++ ABI - starts with "?"
 * 3. Watcom/Borland C++ ABI - starts with "@" or "_"
 */

#include <ananke/base.h>

/* Forward declarations */
extern UINTN EFIAPI RtlDemangleNameItanium(const CHAR8 *, CHAR8 *, UINTN);
extern UINTN EFIAPI RtlDemangleNameMSVC(const CHAR8 *, CHAR8 *, UINTN);
extern UINTN EFIAPI RtlDemangleNameWatcom(const CHAR8 *, CHAR8 *, UINTN);

/**
 * RtlDemangleName - Demangle C++ symbol name (auto-detect ABI)
 *
 * @param MangledName   - Mangled symbol name
 * @param DemangledName - Output buffer
 * @param BufferSize    - Output buffer size
 *
 * @return Length of demangled name, or 0 on error
 */
UINTN
EFIAPI
RtlDemangleName(
    const CHAR8 *MangledName,
    CHAR8 *DemangledName,
    UINTN BufferSize
    )
{
    UINTN Result;

    if (MangledName == NULL || DemangledName == NULL || BufferSize == 0) {
        return 0;
    }

    /* Try Itanium C++ ABI (GCC/Clang/ICC) */
    if (MangledName[0] == '_' && MangledName[1] == 'Z') {
        Result = RtlDemangleNameItanium(MangledName, DemangledName, BufferSize);
        if (Result > 0) {
            return Result;
        }
    }

    /* Try MSVC C++ ABI */
    if (MangledName[0] == '?') {
        Result = RtlDemangleNameMSVC(MangledName, DemangledName, BufferSize);
        if (Result > 0) {
            return Result;
        }
    }

    /* Try Watcom/Borland C++ ABI */
    if (MangledName[0] == '@' ||
        (MangledName[0] == '_' && MangledName[1] != 'Z')) {
        Result = RtlDemangleNameWatcom(MangledName, DemangledName, BufferSize);
        if (Result > 0) {
            return Result;
        }
    }

    /* Not a recognized mangled name, copy as-is */
    Result = 0;
    while (MangledName[Result] && Result < BufferSize - 1) {
        DemangledName[Result] = MangledName[Result];
        Result++;
    }
    DemangledName[Result] = '\0';

    return Result;
}
