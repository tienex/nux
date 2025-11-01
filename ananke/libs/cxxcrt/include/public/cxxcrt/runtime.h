/** @file
  cxxCRT - C++ Runtime Library

  Public header for C++ runtime support

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef CXXCRT_RUNTIME_H_
#define CXXCRT_RUNTIME_H_

#include <ananke/base.h>

ANX_EXTERN_C_BEGIN

/*
 * Itanium C++ ABI Functions
 * Used by GCC, Clang, and compatible compilers
 */

/**
 * Called when a pure virtual function is invoked.
 * Applications can override this weak symbol to provide custom handling.
 */
ANX_ATTR_WEAK
ANX_ATTR_NORETURN
void __cxa_pure_virtual(void);

/**
 * Called when a deleted virtual function is invoked.
 * Applications can override this weak symbol to provide custom handling.
 */
ANX_ATTR_WEAK
ANX_ATTR_NORETURN
void __cxa_deleted_virtual(void);

/**
 * Register a destructor to be called at exit.
 *
 * @param func       Destructor function
 * @param arg        Argument to pass to destructor
 * @param dso_handle Handle to the DSO (dynamic shared object)
 * @return           0 on success, non-zero on failure
 */
int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle);

/**
 * Call all registered destructors for a given DSO.
 * If dso_handle is NULL, call all destructors.
 *
 * @param dso_handle Handle to the DSO, or NULL for all
 */
void __cxa_finalize(void *dso_handle);

/**
 * DSO handle for the main executable.
 * Referenced by global/static object constructors.
 */
extern void *__dso_handle;

#if defined(_MSC_VER)
/*
 * MSVC-specific Functions
 */

typedef void (__cdecl *_purecall_handler_t)(void);

/**
 * MSVC pure virtual function handler.
 * Called when a pure virtual function is invoked.
 */
int __cdecl _purecall(void);

/**
 * Set a custom pure call handler.
 *
 * @param handler New handler function, or NULL to use default
 * @return        Previous handler
 */
_purecall_handler_t __cdecl _set_purecall_handler(_purecall_handler_t handler);

/**
 * Get the current pure call handler.
 *
 * @return Current handler, or NULL if using default
 */
_purecall_handler_t __cdecl _get_purecall_handler(void);

#endif /* _MSC_VER */

#if defined(__WATCOMC__)
/*
 * Watcom-specific Functions
 */

/**
 * Watcom pure virtual function handler.
 */
void __cdecl __pure_virtual_called(void);

#endif /* __WATCOMC__ */

ANX_EXTERN_C_END

#endif /* CXXCRT_RUNTIME_H_ */
