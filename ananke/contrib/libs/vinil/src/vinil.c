/*++
    Module Name:

        vinil.c

    Abstract:

        VINIL core API implementation.
        Clean, modern interface without legacy compatibility overhead.

    Copyright (C) 2025 NUX Project

    SPDX-License-Identifier:    CDDL-1.0
--*/

#include <vinil/vinil.h>
#include <vinil/memory.h>
#include "il_impl.h"
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------- */
/*  Context Structure                                              */
/* --------------------------------------------------------------- */

struct _VINIL_CONTEXT_IMPL {
    vinil_memory_pool  *MemoryPool;
    UINT32             Flags;
    CHAR8              LastError[256];
};

/* --------------------------------------------------------------- */
/*  Context Management                                             */
/* --------------------------------------------------------------- */

HRESULT
VinilCreateContext (
    VINIL_CONTEXT  *Context
    )
{
    struct _VINIL_CONTEXT_IMPL  *Impl;

    if (Context == NULL) {
        return E_POINTER;
    }

    Impl = (struct _VINIL_CONTEXT_IMPL *)malloc(sizeof(struct _VINIL_CONTEXT_IMPL));
    if (Impl == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Impl, 0, sizeof(struct _VINIL_CONTEXT_IMPL));

    /* Create memory pool */
    Impl->MemoryPool = vinil_memory_pool_create(4096, NULL);
    if (Impl->MemoryPool == NULL) {
        free(Impl);
        return E_OUTOFMEMORY;
    }

    *Context = (VINIL_CONTEXT)Impl;
    return S_OK;
}

HRESULT
VinilDestroyContext (
    VINIL_CONTEXT  Context
    )
{
    struct _VINIL_CONTEXT_IMPL  *Impl;

    if (Context == NULL) {
        return E_POINTER;
    }

    Impl = (struct _VINIL_CONTEXT_IMPL *)Context;

    if (Impl->MemoryPool != NULL) {
        vinil_memory_pool_destroy(Impl->MemoryPool);
    }

    free(Impl);
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Program Execution                                              */
/* --------------------------------------------------------------- */

HRESULT
VinilExecute (
    VINIL_CONTEXT   Context,
    VINIL_PROGRAM   Program,
    VINIL_BACKEND   Backend,
    VOID            *Inputs,
    VOID            *Outputs
    )
{
    if (Context == NULL || Program == NULL) {
        return E_POINTER;
    }

    /* TODO: Implement execution based on backend */
    switch (Backend) {
    case VinilBackendInterpreter:
        /* Use interpreter */
        break;

    case VinilBackendJIT:
        /* Use JIT compiler */
        break;

    case VinilBackendAOT:
        /* Use pre-compiled native code */
        break;

    default:
        return E_FAIL;
    }

    (VOID)Inputs;
    (VOID)Outputs;

    return E_NOTIMPL;
}

HRESULT
VinilExecuteKernel (
    VINIL_CONTEXT   Context,
    VINIL_PROGRAM   Program,
    VINIL_BACKEND   Backend,
    CONST UINT32    *GlobalSize,
    CONST UINT32    *LocalSize,
    VOID            *Args
    )
{
    if (Context == NULL || Program == NULL || GlobalSize == NULL || LocalSize == NULL) {
        return E_POINTER;
    }

    /* TODO: Implement kernel execution with work-group scheduling */

    (VOID)Backend;
    (VOID)Args;

    return E_NOTIMPL;
}

/* --------------------------------------------------------------- */
/*  Utility Functions                                              */
/* --------------------------------------------------------------- */

HRESULT
VinilGetVersion (
    UINT32  *Major,
    UINT32  *Minor,
    UINT32  *Patch
    )
{
    if (Major == NULL || Minor == NULL || Patch == NULL) {
        return E_POINTER;
    }

    *Major = VINIL_VERSION_MAJOR;
    *Minor = VINIL_VERSION_MINOR;
    *Patch = VINIL_VERSION_PATCH;

    return S_OK;
}

HRESULT
VinilGetSupportedBackends (
    UINT32  *Backends
    )
{
    if (Backends == NULL) {
        return E_POINTER;
    }

    *Backends = (1 << VinilBackendInterpreter);  /* Interpreter always available */

    /* Check for JIT support */
#if defined(__x86_64__) || defined(__i386__) || defined(__aarch64__) || defined(__arm__)
    *Backends |= (1 << VinilBackendJIT);
#endif

    return S_OK;
}
