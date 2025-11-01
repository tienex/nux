/** @file
  cCRT - Compiler Runtime Library

  MSVC CRT initialization for amd64

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

/*
 * MSVC CRT Initialization
 *
 * MSVC uses special section names for constructor/destructor arrays:
 * - .CRT$XCA to .CRT$XCZ: C++ constructors (XCU is user constructors)
 * - .CRT$XPA to .CRT$XPZ: Pre-terminators (XPU is user pre-terminators)
 * - .CRT$XTA to .CRT$XTZ: Terminators (XTU is user terminators)
 *
 * The linker sorts these sections alphabetically, so we use:
 * - .CRT$XCA/.CRT$XCZ as bookends for constructor array
 * - .CRT$XPA/.CRT$XPZ as bookends for pre-terminator array
 * - .CRT$XTA/.CRT$XTZ as bookends for terminator array
 */

#if defined(_MSC_VER)

typedef void (__cdecl *_PVFV)(void);
typedef int  (__cdecl *_PIFV)(void);

/*
 * Constructor section bookends
 * XCA = start, XCZ = end
 */
#pragma section(".CRT$XCA", long, read)
#pragma section(".CRT$XCZ", long, read)

__declspec(allocate(".CRT$XCA")) _PVFV __xc_a[] = { 0 };
__declspec(allocate(".CRT$XCZ")) _PVFV __xc_z[] = { 0 };

/*
 * Pre-terminator section bookends
 * XPA = start, XPZ = end
 */
#pragma section(".CRT$XPA", long, read)
#pragma section(".CRT$XPZ", long, read)

__declspec(allocate(".CRT$XPA")) _PVFV __xp_a[] = { 0 };
__declspec(allocate(".CRT$XPZ")) _PVFV __xp_z[] = { 0 };

/*
 * Terminator section bookends
 * XTA = start, XTZ = end
 */
#pragma section(".CRT$XTA", long, read)
#pragma section(".CRT$XTZ", long, read)

__declspec(allocate(".CRT$XTA")) _PVFV __xt_a[] = { 0 };
__declspec(allocate(".CRT$XTZ")) _PVFV __xt_z[] = { 0 };

/*
 * Execute all C++ constructors
 */
static void __cdecl _initterm(_PVFV *pfbegin, _PVFV *pfend)
{
    while (pfbegin < pfend)
    {
        if (*pfbegin != 0)
            (**pfbegin)();
        ++pfbegin;
    }
}

/*
 * Execute all C++ constructors (with return value)
 */
static int __cdecl _initterm_e(_PIFV *pfbegin, _PIFV *pfend)
{
    while (pfbegin < pfend)
    {
        if (*pfbegin != 0)
        {
            int ret = (**pfbegin)();
            if (ret != 0)
                return ret;
        }
        ++pfbegin;
    }
    return 0;
}

/*
 * Call all C++ constructors
 * This should be called early in program initialization
 */
void __cdecl _do_global_ctors(void)
{
    _initterm(__xc_a, __xc_z);
}

/*
 * Call all pre-terminators and terminators
 * This should be called during program shutdown
 */
void __cdecl _do_global_dtors(void)
{
    _initterm(__xp_a, __xp_z);
    _initterm(__xt_a, __xt_z);
}

#endif /* _MSC_VER */
