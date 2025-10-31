/*++
    Module Name:

        common.h

    Abstract:

        Common, portable foundation header for ANANKE/NUX kernels and user-mode
        components. Provides consistent UEFI-width base types, HRESULT/GUID,
        COM-style ABI surface, calling conventions, atomic/refcount helpers,
        pointer/reference typedef families, compiler attributes, and
        cross-compiler feature detection.

        This is a super header that includes all ANANKE foundation modules.

        Design goals:
            - C89-C23 and C++98-C++23 compatible.
            - Works on flat and segmented address models.
            - Strict COM ordering: QueryInterface, AddRef, Release are always first.
            - UEFI-width types (UINT8, UINT16, UINT32, UINT64, UINTN, etc.).
            - NT-era commenting style. Policy-oriented inline comments.
            - INITGUID respected.
            - Interfaces use plain names (IUnknown, IXxx). Only macros/functions carry ANX_/Anx.

    Environment:

        Compiler-agnostic: MSVC, Clang, GCC, Watcom; 16-32-64-bit.
--*/

#pragma once

/* --------------------------------------------------------------- */
/*  Standard C headers                                             */
/* --------------------------------------------------------------- */
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>

/* --------------------------------------------------------------- */
/*  ANANKE Foundation Modules                                      */
/* --------------------------------------------------------------- */
#include <ananke/compiler.h>      /* Compiler detection, INLINE, structure packing */
#include <ananke/platform.h>      /* Architecture, endianness, object format */
#include <ananke/types.h>         /* SAL annotations, BOOLEAN, UEFI types, pointer families */
#include <ananke/hresult.h>       /* HRESULT and common error codes */
#include <ananke/guid.h>          /* GUID/IID/CLSID support */
#include <ananke/callconv.h>      /* Calling conventions, NOVTABLE, UUID attributes, __uuidof */
#include <ananke/atomics.h>       /* Atomic operations and interlocked functions */
#include <ananke/attributes.h>    /* Compiler attributes (noinline, deprecated, etc.) */
#include <ananke/intrinsics.h>    /* Bit manipulation and overflow checking intrinsics */
#include <ananke/com.h>           /* IUnknown, IClassFactory, COM interface macros */
