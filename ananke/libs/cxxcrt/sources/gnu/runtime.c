/** @file
  cxxCRT - C++ Runtime Library

  GNU/Clang C++ runtime support

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>

/*
 * Pure Virtual Function Handlers
 *
 * These functions are called when pure virtual or deleted virtual
 * functions are invoked, which should never happen in correct code.
 */

/*
 * __cxa_pure_virtual
 *
 * This function is called when a pure virtual function is invoked.
 * This typically happens when:
 * 1. A pure virtual function is called from a constructor or destructor
 * 2. A vtable entry for a pure virtual function is somehow invoked
 *
 * In a hosted environment, this would typically terminate the program.
 * In an embedded/kernel environment, we provide a weak implementation
 * that can be overridden by the application.
 */
ANX_ATTR_WEAK
ANX_ATTR_NORETURN
void __cxa_pure_virtual(void)
{
    /*
     * Pure virtual function called
     *
     * In a proper implementation, this should:
     * - Log an error message
     * - Terminate the program or kernel panic
     *
     * Since we're in a minimal runtime, we just loop forever.
     * The weak attribute allows applications to override this.
     */

    while (1) {
        /* Infinite loop - application should override this function */
    }

    ANX_UNREACHABLE();
}

/*
 * __cxa_deleted_virtual
 *
 * This function is called when a deleted virtual function is invoked.
 * This happens when a virtual function is explicitly deleted (= delete)
 * but somehow gets called (which should be a compile-time error).
 *
 * This is part of the Itanium C++ ABI used by GCC and Clang.
 */
ANX_ATTR_WEAK
ANX_ATTR_NORETURN
void __cxa_deleted_virtual(void)
{
    /*
     * Deleted virtual function called
     *
     * This should never happen in correct code.
     * Provide a weak implementation that can be overridden.
     */

    while (1) {
        /* Infinite loop - application should override this function */
    }

    ANX_UNREACHABLE();
}

/*
 * Exit Handlers
 *
 * The Itanium C++ ABI requires __cxa_atexit/__cxa_finalize for
 * registering and calling destructors for static objects.
 *
 * We provide a simple implementation with a fixed-size array.
 * For embedded/kernel use, this is typically sufficient.
 */

#define MAX_EXIT_HANDLERS 128

typedef void (*destructor_func_t)(void *);

typedef struct {
    destructor_func_t func;
    void *arg;
    void *dso_handle;
} exit_handler_t;

static exit_handler_t exit_handlers[MAX_EXIT_HANDLERS];
static UINTN exit_handler_count = 0;

/*
 * DSO handle for the main executable
 * This is referenced by global/static object constructors
 */
void *__dso_handle ANX_ATTR_WEAK = &__dso_handle;

/*
 * __cxa_atexit
 *
 * Register a destructor function to be called at exit.
 *
 * Parameters:
 *   func       - Destructor function
 *   arg        - Argument to pass to destructor
 *   dso_handle - Handle to the DSO (dynamic shared object)
 *
 * Returns:
 *   0 on success, non-zero on failure
 */
int __cxa_atexit(destructor_func_t func, void *arg, void *dso_handle)
{
    if (exit_handler_count >= MAX_EXIT_HANDLERS) {
        return -1; /* Out of space */
    }

    exit_handlers[exit_handler_count].func = func;
    exit_handlers[exit_handler_count].arg = arg;
    exit_handlers[exit_handler_count].dso_handle = dso_handle;
    exit_handler_count++;

    return 0;
}

/*
 * __cxa_finalize
 *
 * Call all registered destructors for a given DSO.
 * If dso_handle is NULL, call all destructors.
 *
 * Destructors are called in reverse order of registration (LIFO).
 *
 * Parameters:
 *   dso_handle - Handle to the DSO, or NULL for all
 */
void __cxa_finalize(void *dso_handle)
{
    INTN i;

    /*
     * Walk the handler array backwards (LIFO order)
     * This ensures destructors are called in reverse
     * order of construction.
     */
    for (i = (INTN)exit_handler_count - 1; i >= 0; i--)
    {
        exit_handler_t *handler = &exit_handlers[i];

        /*
         * If dso_handle is NULL, call all handlers.
         * Otherwise, only call handlers for the specified DSO.
         */
        if (dso_handle == NULL || handler->dso_handle == dso_handle)
        {
            if (handler->func != NULL)
            {
                handler->func(handler->arg);
                handler->func = NULL; /* Mark as called */
            }
        }
    }

    /*
     * If dso_handle was NULL, we called all handlers.
     * Reset the count.
     */
    if (dso_handle == NULL) {
        exit_handler_count = 0;
    }
}
