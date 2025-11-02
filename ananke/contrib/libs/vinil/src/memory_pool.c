/** @file
  VINIL Memory Pool COM Implementation

  Pool-based memory allocator with COM interface and error sink callbacks.
  Production implementation with statistics tracking.

  Copyright (C) 2003-2007 Hans-Martin Will.
  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#include <vinil/memory.h>
#include <vinil/error.h>
#include <stdlib.h>
#include <string.h>

//
// Memory Page Structure
//

typedef struct _MEMORY_PAGE {
    struct _MEMORY_PAGE  *Next;
    UINT8                *Data;
    UINTN                Size;
    UINTN                Used;
} MEMORY_PAGE;

//
// Memory Pool Implementation
//

typedef struct _VINIL_MEMORY_POOL_IMPL {
    IVinilMemoryPoolVtbl  *lpVtbl;
    UINT32                RefCount;
    UINTN                 DefaultPageSize;
    MEMORY_PAGE           *FirstPage;
    IVinilErrorSink       *ErrorSink;
    UINTN                 TotalAllocated;
    UINT32                PageCount;
} VINIL_MEMORY_POOL_IMPL;

//
// Forward Declarations
//

static HRESULT STDMETHODCALLTYPE MemoryPool_QueryInterface(IVinilMemoryPool *This, REFIID riid, void **ppvObject);
static UINT32 STDMETHODCALLTYPE MemoryPool_AddRef(IVinilMemoryPool *This);
static UINT32 STDMETHODCALLTYPE MemoryPool_Release(IVinilMemoryPool *This);
static HRESULT STDMETHODCALLTYPE MemoryPool_Allocate(IVinilMemoryPool *This, UINTN Size, VOID **Memory);
static HRESULT STDMETHODCALLTYPE MemoryPool_Clear(IVinilMemoryPool *This);
static HRESULT STDMETHODCALLTYPE MemoryPool_SetErrorSink(IVinilMemoryPool *This, IVinilErrorSink *ErrorSink);
static HRESULT STDMETHODCALLTYPE MemoryPool_GetStatistics(IVinilMemoryPool *This, UINTN *TotalAllocated, UINT32 *PageCount, UINTN *WastedBytes);

//
// Vtable
//

static IVinilMemoryPoolVtbl gMemoryPoolVtbl = {
    MemoryPool_QueryInterface,
    MemoryPool_AddRef,
    MemoryPool_Release,
    MemoryPool_Allocate,
    MemoryPool_Clear,
    MemoryPool_SetErrorSink,
    MemoryPool_GetStatistics
};

//
// IUnknown Implementation
//

static
HRESULT
STDMETHODCALLTYPE
MemoryPool_QueryInterface (
    IVinilMemoryPool  *This,
    REFIID            riid,
    void              **ppvObject
    )
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(*riid, IID_IUnknown) ||
        IsEqualGUID(*riid, IID_IVinilMemoryPool))
    {
        *ppvObject = This;
        MemoryPool_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static
UINT32
STDMETHODCALLTYPE
MemoryPool_AddRef (
    IVinilMemoryPool  *This
    )
{
    VINIL_MEMORY_POOL_IMPL  *Pool = (VINIL_MEMORY_POOL_IMPL *)This;
    return ++Pool->RefCount;
}

static
UINT32
STDMETHODCALLTYPE
MemoryPool_Release (
    IVinilMemoryPool  *This
    )
{
    VINIL_MEMORY_POOL_IMPL  *Pool = (VINIL_MEMORY_POOL_IMPL *)This;
    UINT32                  RefCount;
    MEMORY_PAGE             *Page;
    MEMORY_PAGE             *NextPage;

    RefCount = --Pool->RefCount;
    if (RefCount == 0) {
        /* Release error sink */
        if (Pool->ErrorSink != NULL) {
            Pool->ErrorSink->lpVtbl->Release(Pool->ErrorSink);
        }

        /* Free all pages */
        Page = Pool->FirstPage;
        while (Page != NULL) {
            NextPage = Page->Next;
            free(Page->Data);
            free(Page);
            Page = NextPage;
        }

        /* Free pool structure */
        free(Pool);
    }

    return RefCount;
}

//
// IVinilMemoryPool Implementation
//

static
HRESULT
STDMETHODCALLTYPE
MemoryPool_Allocate (
    IVinilMemoryPool  *This,
    UINTN             Size,
    VOID              **Memory
    )
{
    VINIL_MEMORY_POOL_IMPL  *Pool = (VINIL_MEMORY_POOL_IMPL *)This;
    MEMORY_PAGE             *Page;
    UINTN                   AlignedSize;
    UINTN                   PageSize;

    if (Memory == NULL) {
        return E_POINTER;
    }

    if (Size == 0) {
        *Memory = NULL;
        return S_OK;
    }

    /* Align to 8 bytes */
    AlignedSize = (Size + 7) & ~7;

    /* Try to allocate from current page */
    Page = Pool->FirstPage;
    if (Page != NULL && (Page->Used + AlignedSize) <= Page->Size) {
        *Memory = Page->Data + Page->Used;
        Page->Used += AlignedSize;
        return S_OK;
    }

    /* Need a new page */
    PageSize = (AlignedSize > Pool->DefaultPageSize) ? AlignedSize : Pool->DefaultPageSize;

    Page = (MEMORY_PAGE *)malloc(sizeof(MEMORY_PAGE));
    if (Page == NULL) {
        VINIL_ERROR(Pool->ErrorSink, VinilErrorCategoryMemory, E_OUTOFMEMORY,
                    "Failed to allocate memory page structure");
        return E_OUTOFMEMORY;
    }

    Page->Data = (UINT8 *)malloc(PageSize);
    if (Page->Data == NULL) {
        free(Page);
        VINIL_ERROR(Pool->ErrorSink, VinilErrorCategoryMemory, E_OUTOFMEMORY,
                    "Failed to allocate memory page data");
        return E_OUTOFMEMORY;
    }

    Page->Size = PageSize;
    Page->Used = AlignedSize;
    Page->Next = Pool->FirstPage;
    Pool->FirstPage = Page;
    Pool->PageCount++;
    Pool->TotalAllocated += PageSize;

    *Memory = Page->Data;
    return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
MemoryPool_Clear (
    IVinilMemoryPool  *This
    )
{
    VINIL_MEMORY_POOL_IMPL  *Pool = (VINIL_MEMORY_POOL_IMPL *)This;
    MEMORY_PAGE             *Page;

    /* Reset all pages */
    Page = Pool->FirstPage;
    while (Page != NULL) {
        Page->Used = 0;
        Page = Page->Next;
    }

    return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
MemoryPool_SetErrorSink (
    IVinilMemoryPool  *This,
    IVinilErrorSink   *ErrorSink
    )
{
    VINIL_MEMORY_POOL_IMPL  *Pool = (VINIL_MEMORY_POOL_IMPL *)This;

    /* Release old error sink if present */
    if (Pool->ErrorSink != NULL) {
        Pool->ErrorSink->lpVtbl->Release(Pool->ErrorSink);
    }

    /* AddRef new error sink */
    Pool->ErrorSink = ErrorSink;
    if (ErrorSink != NULL) {
        ErrorSink->lpVtbl->AddRef(ErrorSink);
    }

    return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
MemoryPool_GetStatistics (
    IVinilMemoryPool  *This,
    UINTN             *TotalAllocated,
    UINT32            *PageCount,
    UINTN             *WastedBytes
    )
{
    VINIL_MEMORY_POOL_IMPL  *Pool = (VINIL_MEMORY_POOL_IMPL *)This;
    MEMORY_PAGE             *Page;
    UINTN                   Wasted;

    if (TotalAllocated == NULL || PageCount == NULL || WastedBytes == NULL) {
        return E_POINTER;
    }

    *TotalAllocated = Pool->TotalAllocated;
    *PageCount = Pool->PageCount;

    /* Calculate wasted bytes (allocated but not used) */
    Wasted = 0;
    Page = Pool->FirstPage;
    while (Page != NULL) {
        Wasted += (Page->Size - Page->Used);
        Page = Page->Next;
    }

    *WastedBytes = Wasted;
    return S_OK;
}

//
// Factory Function
//

HRESULT
VinilCreateMemoryPool (
    UINTN               DefaultPageSize,
    IVinilMemoryPool    **MemoryPool
    )
{
    VINIL_MEMORY_POOL_IMPL  *Pool;

    if (MemoryPool == NULL) {
        return E_POINTER;
    }

    if (DefaultPageSize == 0) {
        DefaultPageSize = VINIL_DEFAULT_PAGE_SIZE;
    }

    Pool = (VINIL_MEMORY_POOL_IMPL *)malloc(sizeof(VINIL_MEMORY_POOL_IMPL));
    if (Pool == NULL) {
        return E_OUTOFMEMORY;
    }

    memset(Pool, 0, sizeof(VINIL_MEMORY_POOL_IMPL));
    Pool->lpVtbl = &gMemoryPoolVtbl;
    Pool->RefCount = 1;
    Pool->DefaultPageSize = DefaultPageSize;
    Pool->FirstPage = NULL;
    Pool->ErrorSink = NULL;
    Pool->TotalAllocated = 0;
    Pool->PageCount = 0;

    *MemoryPool = (IVinilMemoryPool *)Pool;
    return S_OK;
}
