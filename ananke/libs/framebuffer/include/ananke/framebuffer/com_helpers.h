/*++
    Module Name:

        com_helpers.h

    Abstract:

        Helper macros for implementing COM interfaces in the framebuffer library.
        Reduces boilerplate code for IUnknown implementations.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/com.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  IUnknown Implementation Helpers                                */
/* --------------------------------------------------------------- */

/*
 * Implement QueryInterface for a COM object.
 *
 * Parameters:
 *   Prefix      - Function name prefix (e.g., FbPalette)
 *   StructType  - Structure type name (e.g., FB_PALETTE_MGR)
 *   IfaceType   - Interface type name (e.g., IFramebufferPalette)
 *   IfaceIID    - Interface IID (e.g., IID_IFramebufferPalette)
 *
 * Generates:
 *   static HRESULT STDMETHODCALLTYPE Prefix_QueryInterface(...)
 *
 * Usage:
 *   FB_IMPLEMENT_QUERYINTERFACE(FbPalette, FB_PALETTE_MGR, IFramebufferPalette, IID_IFramebufferPalette)
 */
#define FB_IMPLEMENT_QUERYINTERFACE(Prefix, StructType, IfaceType, IfaceIID) \
    static HRESULT STDMETHODCALLTYPE \
    Prefix##_QueryInterface( \
        IfaceType *This, \
        REFIID riid, \
        VOID **ppvObject \
        ) \
    { \
        StructType *Obj = (StructType *)This; \
        \
        if (ppvObject == NULL) { \
            return E_POINTER; \
        } \
        \
        if (IsEqualGUID(riid, &IID_IUnknown) || \
            IsEqualGUID(riid, &IfaceIID)) { \
            *ppvObject = &Obj->Base; \
            Prefix##_AddRef(This); \
            return S_OK; \
        } \
        \
        *ppvObject = NULL; \
        return E_NOINTERFACE; \
    }

/*
 * Implement AddRef for a COM object.
 *
 * Parameters:
 *   Prefix      - Function name prefix (e.g., FbPalette)
 *   StructType  - Structure type name (e.g., FB_PALETTE_MGR)
 *   IfaceType   - Interface type name (e.g., IFramebufferPalette)
 *
 * Generates:
 *   static UINT32 STDMETHODCALLTYPE Prefix_AddRef(...)
 *
 * Usage:
 *   FB_IMPLEMENT_ADDREF(FbPalette, FB_PALETTE_MGR, IFramebufferPalette)
 */
#define FB_IMPLEMENT_ADDREF(Prefix, StructType, IfaceType) \
    static UINT32 STDMETHODCALLTYPE \
    Prefix##_AddRef( \
        IfaceType *This \
        ) \
    { \
        StructType *Obj = (StructType *)This; \
        return ANX_REF_INC(&Obj->RefCount); \
    }

/*
 * Implement Release for a COM object.
 *
 * Parameters:
 *   Prefix      - Function name prefix (e.g., FbPalette)
 *   StructType  - Structure type name (e.g., FB_PALETTE_MGR)
 *   IfaceType   - Interface type name (e.g., IFramebufferPalette)
 *
 * Generates:
 *   static UINT32 STDMETHODCALLTYPE Prefix_Release(...)
 *
 * Usage:
 *   FB_IMPLEMENT_RELEASE(FbPalette, FB_PALETTE_MGR, IFramebufferPalette)
 */
#define FB_IMPLEMENT_RELEASE(Prefix, StructType, IfaceType) \
    static UINT32 STDMETHODCALLTYPE \
    Prefix##_Release( \
        IfaceType *This \
        ) \
    { \
        StructType *Obj = (StructType *)This; \
        return ANX_REF_DEC(&Obj->RefCount); \
    }

/*
 * Implement complete IUnknown interface (QueryInterface, AddRef, Release).
 *
 * Parameters:
 *   Prefix      - Function name prefix (e.g., FbPalette)
 *   StructType  - Structure type name (e.g., FB_PALETTE_MGR)
 *   IfaceType   - Interface type name (e.g., IFramebufferPalette)
 *   IfaceIID    - Interface IID (e.g., IID_IFramebufferPalette)
 *
 * Generates:
 *   static HRESULT STDMETHODCALLTYPE Prefix_QueryInterface(...)
 *   static UINT32 STDMETHODCALLTYPE Prefix_AddRef(...)
 *   static UINT32 STDMETHODCALLTYPE Prefix_Release(...)
 *
 * Usage:
 *   FB_IMPLEMENT_IUNKNOWN(FbPalette, FB_PALETTE_MGR, IFramebufferPalette, IID_IFramebufferPalette)
 *
 * This is the most commonly used macro. It generates all three IUnknown methods.
 */
#define FB_IMPLEMENT_IUNKNOWN(Prefix, StructType, IfaceType, IfaceIID) \
    FB_IMPLEMENT_QUERYINTERFACE(Prefix, StructType, IfaceType, IfaceIID) \
    FB_IMPLEMENT_ADDREF(Prefix, StructType, IfaceType) \
    FB_IMPLEMENT_RELEASE(Prefix, StructType, IfaceType)

/* --------------------------------------------------------------- */
/*  Backend IUnknown Implementation Helpers                        */
/* --------------------------------------------------------------- */

/*
 * Implement IUnknown for framebuffer backends.
 * Backends use IFramebufferBackend interface.
 *
 * Parameters:
 *   Prefix      - Function name prefix (e.g., GenericFb)
 *   StructType  - Structure type name (e.g., GENERIC_FB_BACKEND)
 *
 * Generates:
 *   static HRESULT STDMETHODCALLTYPE Prefix_QueryInterface(...)
 *   static UINT32 STDMETHODCALLTYPE Prefix_AddRef(...)
 *   static UINT32 STDMETHODCALLTYPE Prefix_Release(...)
 *
 * Usage:
 *   FB_IMPLEMENT_BACKEND_IUNKNOWN(GenericFb, GENERIC_FB_BACKEND)
 */
#define FB_IMPLEMENT_BACKEND_IUNKNOWN(Prefix, StructType) \
    FB_IMPLEMENT_IUNKNOWN(Prefix, StructType, IFramebufferBackend, IID_IFramebufferBackend)

/* --------------------------------------------------------------- */
/*  Common Object Patterns                                         */
/* --------------------------------------------------------------- */

/*
 * Define a singleton COM object with standard initialization.
 *
 * Parameters:
 *   StructType  - Structure type name
 *   VarName     - Variable name for the singleton instance
 *   VtblName    - VTable variable name
 *
 * Generates:
 *   static StructType VarName = {
 *       .Base.lpVtbl = &VtblName,
 *       .RefCount.RefCount = 1,
 *   };
 *
 * Usage:
 *   FB_DEFINE_SINGLETON(FB_PALETTE_MGR, gPaletteManagerInstance, gPaletteMgrVtbl)
 */
#define FB_DEFINE_SINGLETON(StructType, VarName, VtblName) \
    static StructType VarName = { \
        .Base.lpVtbl = &VtblName, \
        .RefCount.RefCount = 1, \
    }

/* --------------------------------------------------------------- */
/*  Usage Example                                                  */
/* --------------------------------------------------------------- */

#if 0
/*
 * Example: Implementing a COM interface with helpers
 *
 * Before (manual implementation):
 *   static HRESULT STDMETHODCALLTYPE FbPalette_QueryInterface(
 *       IFramebufferPalette *This, REFIID riid, VOID **ppvObject)
 *   {
 *       FB_PALETTE_MGR *Manager = (FB_PALETTE_MGR *)This;
 *       if (ppvObject == NULL) return E_POINTER;
 *       if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IFramebufferPalette)) {
 *           *ppvObject = &Manager->Base;
 *           FbPalette_AddRef(This);
 *           return S_OK;
 *       }
 *       *ppvObject = NULL;
 *       return E_NOINTERFACE;
 *   }
 *   // ... 40 more lines for AddRef and Release
 *
 * After (using helpers):
 *   FB_IMPLEMENT_IUNKNOWN(FbPalette, FB_PALETTE_MGR, IFramebufferPalette, IID_IFramebufferPalette)
 *
 * That's it! All three IUnknown methods are now implemented.
 */

typedef struct _EXAMPLE_OBJECT {
    IFramebufferPalette Base;
    REFOBJ              RefCount;
    /* ... additional fields ... */
} EXAMPLE_OBJECT;

/* Implement all IUnknown methods with one line */
FB_IMPLEMENT_IUNKNOWN(Example, EXAMPLE_OBJECT, IFramebufferPalette, IID_IFramebufferPalette)

/* Now implement the interface-specific methods */
static HRESULT STDMETHODCALLTYPE Example_GetSize(IFramebufferPalette *This, UINT32 *Size)
{
    /* ... implementation ... */
}

/* Define the VTable */
static CONST IFramebufferPaletteVtbl gExampleVtbl = {
    .QueryInterface = Example_QueryInterface,
    .AddRef         = Example_AddRef,
    .Release        = Example_Release,
    .GetSize        = Example_GetSize,
    /* ... */
};

/* Define singleton instance */
FB_DEFINE_SINGLETON(EXAMPLE_OBJECT, gExampleInstance, gExampleVtbl)

#endif /* Example */
