/** @file
  cxxCRT - C++ Runtime Library

  C++ Name Demangling API

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef CXXCRT_DEMANGLE_H_
#define CXXCRT_DEMANGLE_H_

#include <ananke/base.h>
#include <stddef.h>

ANX_EXTERN_C_BEGIN

/*
 * Itanium C++ ABI Demangling (GCC/Clang/ICC)
 */

/**
 * __cxa_demangle - Demangle Itanium C++ ABI symbol name
 *
 * @param mangled_name  - Mangled symbol name
 * @param output_buffer - Output buffer (must be provided)
 * @param length        - Pointer to output buffer size
 * @param status        - Status code output (0=success, -1=error, -2=invalid)
 *
 * @return Pointer to output buffer on success, NULL on error
 *
 * Note: Unlike the standard __cxa_demangle, this implementation
 *       does NOT allocate memory. Caller must provide output_buffer.
 */
char *__cxa_demangle(
    const char *mangled_name,
    char *output_buffer,
    size_t *length,
    int *status
);

#if defined(_MSC_VER)
/*
 * MSVC C++ ABI Demangling
 */

/**
 * __unDName - MSVC internal demangling function
 *
 * @param outputString    - Output buffer
 * @param name            - Mangled name
 * @param maxStringLength - Output buffer size
 * @param mallocFnPtr     - Unused (no allocation)
 * @param freeFnPtr       - Unused (no allocation)
 * @param disableFlags    - Unused
 *
 * @return Pointer to output buffer on success, NULL on error
 */
char * __cdecl __unDName(
    char *outputString,
    const char *name,
    int maxStringLength,
    void *mallocFnPtr,
    void *freeFnPtr,
    unsigned short disableFlags
);

/**
 * UnDecorateSymbolName - Public MSVC demangling API
 *
 * @param DecoratedName     - Mangled name
 * @param UnDecoratedName   - Output buffer
 * @param UndecoratedLength - Output buffer size
 * @param Flags             - Unused
 *
 * @return Length of demangled name, or 0 on error
 */
unsigned long __stdcall UnDecorateSymbolName(
    const char *DecoratedName,
    char *UnDecoratedName,
    unsigned long UndecoratedLength,
    unsigned long Flags
);

#endif /* _MSC_VER */

ANX_EXTERN_C_END

#endif /* CXXCRT_DEMANGLE_H_ */
