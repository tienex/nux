/** @file
  cxxCRT - C++ Runtime Library

  GNU/Clang demangling wrappers

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>

/* NTRTL demanglers */
extern UINTN EFIAPI RtlDemangleName(const CHAR8 *, CHAR8 *, UINTN);
extern UINTN EFIAPI RtlDemangleNameItanium(const CHAR8 *, CHAR8 *, UINTN);

/**
 * __cxa_demangle - Itanium C++ ABI demangling function
 *
 * This is the standard function used by GCC/Clang for demangling.
 *
 * @param mangled_name  - Mangled symbol name
 * @param output_buffer - Output buffer (if NULL, allocate)
 * @param length        - Output buffer size (if buffer provided)
 * @param status        - Status code output
 *
 * @return Demangled name (caller must free if allocated)
 *
 * Note: This simplified implementation does not allocate memory.
 *       Caller must provide output_buffer.
 */
char *
__cxa_demangle(
    const char *mangled_name,
    char *output_buffer,
    size_t *length,
    int *status
    )
{
    UINTN Result;

    if (mangled_name == NULL) {
        if (status) *status = -1; /* Invalid argument */
        return NULL;
    }

    if (output_buffer == NULL) {
        /* Memory allocation not supported in embedded environment */
        if (status) *status = -1; /* Memory allocation failure */
        return NULL;
    }

    if (length == NULL || *length == 0) {
        if (status) *status = -1; /* Invalid argument */
        return NULL;
    }

    /* Attempt demangling */
    Result = RtlDemangleNameItanium(
        (const CHAR8 *)mangled_name,
        (CHAR8 *)output_buffer,
        *length
    );

    if (Result > 0) {
        if (status) *status = 0; /* Success */
        return output_buffer;
    } else {
        if (status) *status = -2; /* Invalid mangled name */
        return NULL;
    }
}
