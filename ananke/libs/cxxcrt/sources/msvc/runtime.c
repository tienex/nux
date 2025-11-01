/** @file
  cxxCRT - C++ Runtime Library

  MSVC C++ runtime support

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>

#if defined(_MSC_VER)

/*
 * MSVC Pure Call Handling
 *
 * MSVC uses _purecall instead of __cxa_pure_virtual.
 * The _purecall_handler allows applications to set a custom handler.
 */

/* Function pointer type for pure call handler */
typedef void (__cdecl *_purecall_handler_t)(void);

/* Global handler pointer - can be set by application */
static _purecall_handler_t _purecall_handler_func = NULL;

/*
 * Default pure call handler
 */
static void __cdecl _default_purecall_handler(void)
{
    /*
     * Pure virtual function called
     *
     * In a hosted environment, this would call abort().
     * In embedded/kernel environment, loop forever.
     */
    while (1) {
        /* Infinite loop */
    }
}

/*
 * _purecall
 *
 * Called when a pure virtual function is invoked.
 * This is the MSVC equivalent of __cxa_pure_virtual.
 */
int __cdecl _purecall(void)
{
    if (_purecall_handler_func != NULL) {
        _purecall_handler_func();
    } else {
        _default_purecall_handler();
    }

    /* Should never reach here */
    return 0;
}

/*
 * _set_purecall_handler
 *
 * Allows applications to set a custom pure call handler.
 *
 * Parameters:
 *   handler - New handler function, or NULL to use default
 *
 * Returns:
 *   Previous handler
 */
_purecall_handler_t __cdecl _set_purecall_handler(_purecall_handler_t handler)
{
    _purecall_handler_t old_handler = _purecall_handler_func;
    _purecall_handler_func = handler;
    return old_handler;
}

/*
 * _get_purecall_handler
 *
 * Get the current pure call handler.
 *
 * Returns:
 *   Current handler, or NULL if using default
 */
_purecall_handler_t __cdecl _get_purecall_handler(void)
{
    return _purecall_handler_func;
}

/*
 * MSVC Compatibility with Itanium C++ ABI
 *
 * MSVC doesn't use the Itanium C++ ABI (__cxa_atexit, etc.) but
 * we provide these for compatibility when using Clang-CL or when
 * linking MSVC-compiled code with GNU-compiled code.
 */

/* Forward declarations of GNU implementations */
extern int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle);
extern void __cxa_finalize(void *dso_handle);
extern void *__dso_handle;

/*
 * Wrapper to call __cxa_atexit from MSVC code
 */
int _cxa_atexit_msvc(void (*func)(void *), void *arg, void *dso_handle)
{
    return __cxa_atexit(func, arg, dso_handle);
}

/*
 * Wrapper to call __cxa_finalize from MSVC code
 */
void _cxa_finalize_msvc(void *dso_handle)
{
    __cxa_finalize(dso_handle);
}

/*
 * MSVC doesn't have __dso_handle, but provide it for compatibility
 */
#pragma comment(linker, "/alternatename:___dso_handle_msvc=___dso_handle")
extern void *__dso_handle_msvc;

#endif /* _MSC_VER */
