/** @file
  cxxCRT - C++ Runtime Library

  Watcom C++ runtime support

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>

#if defined(__WATCOMC__)

/*
 * Watcom Pure Virtual Handling
 *
 * Watcom uses its own conventions for C++ runtime support.
 * We provide compatibility with both Watcom-specific and
 * Itanium C++ ABI conventions.
 */

/*
 * __pure_virtual_called
 *
 * Watcom's handler for pure virtual function calls.
 * This is the Watcom equivalent of __cxa_pure_virtual.
 */
void __cdecl __pure_virtual_called(void)
{
    /*
     * Pure virtual function called
     *
     * Loop forever - application should override this function.
     */
    while (1) {
        /* Infinite loop */
    }
}

/*
 * Provide Itanium C++ ABI compatibility
 *
 * Forward declarations of GNU implementations
 */
extern void __cxa_pure_virtual(void);
extern void __cxa_deleted_virtual(void);
extern int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle);
extern void __cxa_finalize(void *dso_handle);
extern void *__dso_handle;

/*
 * Watcom wrappers for Itanium C++ ABI functions
 */

void __watcom_cxa_pure_virtual(void)
{
    __cxa_pure_virtual();
}

void __watcom_cxa_deleted_virtual(void)
{
    __cxa_deleted_virtual();
}

int __watcom_cxa_atexit(void (*func)(void *), void *arg, void *dso_handle)
{
    return __cxa_atexit(func, arg, dso_handle);
}

void __watcom_cxa_finalize(void *dso_handle)
{
    __cxa_finalize(dso_handle);
}

/*
 * Watcom pragma aux for function aliasing
 */
#pragma aux __watcom_cxa_pure_virtual "__cxa_pure_virtual"
#pragma aux __watcom_cxa_deleted_virtual "__cxa_deleted_virtual"
#pragma aux __watcom_cxa_atexit "__cxa_atexit"
#pragma aux __watcom_cxa_finalize "__cxa_finalize"

extern void *__watcom_dso_handle;
#pragma aux __watcom_dso_handle "__dso_handle"

#endif /* __WATCOMC__ */
