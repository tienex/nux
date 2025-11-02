/*++
    Module Name:

        vinil.c

    Abstract:

        VINIL COM interface implementations with vtables and reference counting.

    Copyright (C) 2003-2007 Hans-Martin Will
    Copyright (C) 2025 NUX Project

    SPDX-License-Identifier:    CDDL-1.0
--*/

#include <vinil/vinil.h>
#include <ananke/atomics.h>
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------- */
/*  Internal Structure Definitions                                 */
/* --------------------------------------------------------------- */

typedef struct _VINIL_CONTEXT_IMPL {
    IVinilContext       Base;
    REFOBJ              RefCount;
    CHAR8               LastError[256];
} VINIL_CONTEXT_IMPL;

typedef struct _VINIL_PROGRAM_IMPL {
    IVinilProgram       Base;
    REFOBJ              RefCount;
    IVinilContext       *Context;
    CHAR8               CompileLog[1024];
} VINIL_PROGRAM_IMPL;

typedef struct _VINIL_EXECUTABLE_IMPL {
    IVinilExecutable    Base;
    REFOBJ              RefCount;
    VOID                *Code;
    BOOLEAN             IsJit;
    UINT64              CyclesExecuted;
} VINIL_EXECUTABLE_IMPL;

/* --------------------------------------------------------------- */
/*  IVinilContext Implementation                                   */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
VinilContext_QueryInterface (
    IVinilContext  *This,
    REFIID         riid,
    VOID           **ppvObject
    )
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IVinilContext)) {
        *ppvObject = This;
        This->lpVtbl->AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
VinilContext_AddRef (
    IVinilContext  *This
    )
{
    VINIL_CONTEXT_IMPL *Impl = (VINIL_CONTEXT_IMPL *)This;
    return (UINT32)AtomicIncrement(&Impl->RefCount);
}

static UINT32 STDMETHODCALLTYPE
VinilContext_Release (
    IVinilContext  *This
    )
{
    VINIL_CONTEXT_IMPL *Impl = (VINIL_CONTEXT_IMPL *)This;
    UINT32 RefCount = (UINT32)AtomicDecrement(&Impl->RefCount);
    
    if (RefCount == 0) {
        free(Impl);
    }
    
    return RefCount;
}

static HRESULT STDMETHODCALLTYPE
VinilContext_CreateProgram (
    IVinilContext   *This,
    IVinilProgram   **Program
    );

static HRESULT STDMETHODCALLTYPE
VinilContext_GetLastError (
    IVinilContext   *This,
    CONST CHAR8     **ErrorMessage
    )
{
    VINIL_CONTEXT_IMPL *Impl = (VINIL_CONTEXT_IMPL *)This;
    
    if (ErrorMessage == NULL) {
        return E_POINTER;
    }
    
    *ErrorMessage = Impl->LastError;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VinilContext_GetVersion (
    IVinilContext  *This,
    UINT32         *Major,
    UINT32         *Minor,
    UINT32         *Patch
    )
{
    (VOID)This;
    
    if (Major == NULL || Minor == NULL || Patch == NULL) {
        return E_POINTER;
    }
    
    *Major = VINIL_VERSION_MAJOR;
    *Minor = VINIL_VERSION_MINOR;
    *Patch = VINIL_VERSION_PATCH;
    
    return S_OK;
}

static CONST IVinilContextVtbl gVinilContextVtbl = {
    .QueryInterface = VinilContext_QueryInterface,
    .AddRef         = VinilContext_AddRef,
    .Release        = VinilContext_Release,
    .CreateProgram  = VinilContext_CreateProgram,
    .GetLastError   = VinilContext_GetLastError,
    .GetVersion     = VinilContext_GetVersion,
};

/* --------------------------------------------------------------- */
/*  IVinilProgram Implementation                                   */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
VinilProgram_QueryInterface (
    IVinilProgram  *This,
    REFIID         riid,
    VOID           **ppvObject
    )
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IVinilProgram)) {
        *ppvObject = This;
        This->lpVtbl->AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
VinilProgram_AddRef (
    IVinilProgram  *This
    )
{
    VINIL_PROGRAM_IMPL *Impl = (VINIL_PROGRAM_IMPL *)This;
    return (UINT32)AtomicIncrement(&Impl->RefCount);
}

static UINT32 STDMETHODCALLTYPE
VinilProgram_Release (
    IVinilProgram  *This
    )
{
    VINIL_PROGRAM_IMPL *Impl = (VINIL_PROGRAM_IMPL *)This;
    UINT32 RefCount = (UINT32)AtomicDecrement(&Impl->RefCount);
    
    if (RefCount == 0) {
        if (Impl->Context != NULL) {
            Impl->Context->lpVtbl->Release(Impl->Context);
        }
        free(Impl);
    }
    
    return RefCount;
}

static HRESULT STDMETHODCALLTYPE
VinilProgram_Compile (
    IVinilProgram       *This,
    VINIL_COMPILE_FLAGS Flags,
    IVinilExecutable    **Executable
    );

static HRESULT STDMETHODCALLTYPE
VinilProgram_GetCompileLog (
    IVinilProgram  *This,
    CONST CHAR8    **Log
    )
{
    VINIL_PROGRAM_IMPL *Impl = (VINIL_PROGRAM_IMPL *)This;
    
    if (Log == NULL) {
        return E_POINTER;
    }
    
    *Log = Impl->CompileLog;
    return S_OK;
}

static CONST IVinilProgramVtbl gVinilProgramVtbl = {
    .QueryInterface = VinilProgram_QueryInterface,
    .AddRef         = VinilProgram_AddRef,
    .Release        = VinilProgram_Release,
    .Compile        = VinilProgram_Compile,
    .GetCompileLog  = VinilProgram_GetCompileLog,
};

/* --------------------------------------------------------------- */
/*  IVinilExecutable Implementation                                */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
VinilExecutable_QueryInterface (
    IVinilExecutable  *This,
    REFIID            riid,
    VOID              **ppvObject
    )
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IVinilExecutable)) {
        *ppvObject = This;
        This->lpVtbl->AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
VinilExecutable_AddRef (
    IVinilExecutable  *This
    )
{
    VINIL_EXECUTABLE_IMPL *Impl = (VINIL_EXECUTABLE_IMPL *)This;
    return (UINT32)AtomicIncrement(&Impl->RefCount);
}

static UINT32 STDMETHODCALLTYPE
VinilExecutable_Release (
    IVinilExecutable  *This
    )
{
    VINIL_EXECUTABLE_IMPL *Impl = (VINIL_EXECUTABLE_IMPL *)This;
    UINT32 RefCount = (UINT32)AtomicDecrement(&Impl->RefCount);
    
    if (RefCount == 0) {
        /* TODO: Free JIT code if allocated */
        free(Impl);
    }
    
    return RefCount;
}

static HRESULT STDMETHODCALLTYPE
VinilExecutable_Execute (
    IVinilExecutable  *This,
    VINIL_EXEC_MODE   Mode,
    VOID              *UserData
    )
{
    VINIL_EXECUTABLE_IMPL *Impl = (VINIL_EXECUTABLE_IMPL *)This;
    
    (VOID)Mode;
    (VOID)UserData;
    
    /* TODO: Actual execution */
    Impl->CyclesExecuted = 0;
    
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
VinilExecutable_GetStats (
    IVinilExecutable  *This,
    UINT64            *CyclesExecuted
    )
{
    VINIL_EXECUTABLE_IMPL *Impl = (VINIL_EXECUTABLE_IMPL *)This;
    
    if (CyclesExecuted == NULL) {
        return E_POINTER;
    }
    
    *CyclesExecuted = Impl->CyclesExecuted;
    return S_OK;
}

static CONST IVinilExecutableVtbl gVinilExecutableVtbl = {
    .QueryInterface = VinilExecutable_QueryInterface,
    .AddRef         = VinilExecutable_AddRef,
    .Release        = VinilExecutable_Release,
    .Execute        = VinilExecutable_Execute,
    .GetStats       = VinilExecutable_GetStats,
};

/* --------------------------------------------------------------- */
/*  Forward Declaration Implementations                            */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
VinilContext_CreateProgram (
    IVinilContext   *This,
    IVinilProgram   **Program
    )
{
    VINIL_PROGRAM_IMPL *Impl;
    
    if (Program == NULL) {
        return E_POINTER;
    }
    
    Impl = (VINIL_PROGRAM_IMPL *)malloc(sizeof(VINIL_PROGRAM_IMPL));
    if (Impl == NULL) {
        return E_OUTOFMEMORY;
    }
    
    Impl->Base.lpVtbl = &gVinilProgramVtbl;
    Impl->RefCount = 1;
    Impl->Context = This;
    Impl->CompileLog[0] = '\0';
    
    /* AddRef the context */
    This->lpVtbl->AddRef(This);
    
    *Program = &Impl->Base;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VinilProgram_Compile (
    IVinilProgram       *This,
    VINIL_COMPILE_FLAGS Flags,
    IVinilExecutable    **Executable
    )
{
    VINIL_PROGRAM_IMPL *ProgImpl = (VINIL_PROGRAM_IMPL *)This;
    VINIL_EXECUTABLE_IMPL *ExecImpl;
    
    if (Executable == NULL) {
        return E_POINTER;
    }
    
    ExecImpl = (VINIL_EXECUTABLE_IMPL *)malloc(sizeof(VINIL_EXECUTABLE_IMPL));
    if (ExecImpl == NULL) {
        return E_OUTOFMEMORY;
    }
    
    ExecImpl->Base.lpVtbl = &gVinilExecutableVtbl;
    ExecImpl->RefCount = 1;
    ExecImpl->Code = NULL;
    ExecImpl->IsJit = (Flags & VinilCompileFlagUseJit) != 0;
    ExecImpl->CyclesExecuted = 0;
    
    /* TODO: Actual compilation */
    strncpy(ProgImpl->CompileLog, "Compilation not yet implemented", 
            sizeof(ProgImpl->CompileLog) - 1);
    
    *Executable = &ExecImpl->Base;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Factory Function                                                */
/* --------------------------------------------------------------- */

HRESULT
VinilCreateContext (
    IVinilContext  **Context
    )
{
    VINIL_CONTEXT_IMPL *Impl;
    
    if (Context == NULL) {
        return E_POINTER;
    }
    
    Impl = (VINIL_CONTEXT_IMPL *)malloc(sizeof(VINIL_CONTEXT_IMPL));
    if (Impl == NULL) {
        return E_OUTOFMEMORY;
    }
    
    Impl->Base.lpVtbl = &gVinilContextVtbl;
    Impl->RefCount = 1;
    Impl->LastError[0] = '\0';
    
    *Context = &Impl->Base;
    return S_OK;
}
