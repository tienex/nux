/** @file
  cCRT - Compiler Runtime Library

  Watcom CRT initialization for RISC-V 32

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

/*
 * Watcom CRT Initialization
 *
 * Watcom uses function pointers in special data segments for
 * constructor/destructor arrays. We provide GNU-compatible
 * .ctors and .dtors sections.
 */

#if defined(__WATCOMC__)

typedef void (*func_ptr)(void);

/*
 * Constructor array bookends
 * Watcom linker collects these into contiguous arrays
 */
#pragma data_seg(".ctors$a")
static func_ptr __CTOR_LIST__[] = { (func_ptr)(-1) };

#pragma data_seg(".ctors$z")
static func_ptr __CTOR_END__[] = { (func_ptr)0 };

/*
 * Destructor array bookends
 */
#pragma data_seg(".dtors$a")
static func_ptr __DTOR_LIST__[] = { (func_ptr)(-1) };

#pragma data_seg(".dtors$z")
static func_ptr __DTOR_END__[] = { (func_ptr)0 };

#pragma data_seg()

/*
 * Initialization state
 */
static char __initialized = 0;
static char __finished = 0;

/*
 * Execute destructor array
 */
void __cdecl __do_global_dtors(void)
{
    func_ptr *p;

    if (__finished)
        return;
    __finished = 1;

    /* Skip the first entry (sentinel -1) and iterate until NULL */
    for (p = __DTOR_LIST__ + 1; *p != (func_ptr)0; p++)
    {
        (**p)();
    }
}

/*
 * Execute constructor array
 * Constructors are stored in reverse order, so walk backwards
 */
void __cdecl __do_global_ctors(void)
{
    func_ptr *p;

    if (__initialized)
        return;
    __initialized = 1;

    /* Find the end of the array (marked by NULL) */
    for (p = __CTOR_END__ - 1; *p != (func_ptr)(-1); p--)
    {
        (**p)();
    }
}

#endif /* __WATCOMC__ */
