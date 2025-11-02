/*++
    Module Name:

        com.h

    Abstract:

        COM interface definitions: IUnknown, IClassFactory, and unified
        interface declaration macros.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>
#include <ananke/callconv.h>

/* --------------------------------------------------------------- */
/*  Unified interface declaration (no user #ifdef).                 */
/* --------------------------------------------------------------- */
#define ANX_UNPAREN(...) __VA_ARGS__

#ifdef __cplusplus
#   define ANX_BEGIN_INTERFACE(iface, base, iid_sym, iid_str) \
        struct iface; \
        ANX_MIDL_INTERFACE(iid_str) iface : public base {
#   define ANX_IFACE_METHOD(ret, name, args) \
        virtual ret STDMETHODCALLTYPE name args = 0;
#   define ANX_END_INTERFACE(iface, iid_sym) \
        }; \
        ANX_BIND_UUIDOF_CXX(iface, iid_sym)
#else
#   define ANX_BEGIN_INTERFACE(iface, base, iid_sym, iid_str) \
        typedef struct iface iface; \
        typedef struct iface##Vtbl { \
            ANX_STDMETHOD(QueryInterface)(iface* This, REFIID riid, void** ppvObject); \
            ANX_STDMETHOD_(UINT32, AddRef)(iface* This); \
            ANX_STDMETHOD_(UINT32, Release)(iface* This);
#   define ANX_IFACE_METHOD_FULL(iface, ret, name, args) \
        ret (STDMETHODCALLTYPE *name)(iface* This, ANX_UNPAREN args);
#   define ANX_IFACE_METHOD(ret, name, args) \
        ret (STDMETHODCALLTYPE *name)(void* This, ANX_UNPAREN args);
#   define ANX_END_INTERFACE(iface, iid_sym) \
        } iface##Vtbl; \
        struct iface { const iface##Vtbl* lpVtbl; }; \
        ANX_BIND_UUIDOF_C(iface, iid_sym)
#endif

#ifdef __cplusplus
#   if !defined(ANX_HAS_NATIVE_UUIDOF)
#       define ANX_BIND_UUIDOF_CXX(iface, iid_sym) ANX_DECLARE_UUIDOF(struct iface, iid_sym)
#   else
#       define ANX_BIND_UUIDOF_CXX(iface, iid_sym)
#   endif
#else
#   define ANX_BIND_UUIDOF_CXX(iface, iid_sym)
#endif
#define ANX_BIND_UUIDOF_C(iface, iid_sym) ANX_DECLARE_UUIDOF_C(iface, iid_sym)

/* Standard COBJMACROS (unprefixed) */
#ifndef COBJMACROS
#   define COBJMACROS 1
#   define IUnknown_QueryInterface(This, riid, ppv)   ((This)->lpVtbl->QueryInterface((This),(riid),(ppv)))
#   define IUnknown_AddRef(This)                      ((This)->lpVtbl->AddRef((This)))
#   define IUnknown_Release(This)                     ((This)->lpVtbl->Release((This)))
#endif

/* --------------------------------------------------------------- */
/*  IUnknown and IClassFactory (C and C++ views).                   */
/* --------------------------------------------------------------- */
#ifdef __cplusplus
struct ANX_NOVTABLE IUnknown {
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvObject) = 0;
    virtual UINT32  STDMETHODCALLTYPE AddRef(VOID) = 0;
    virtual UINT32  STDMETHODCALLTYPE Release(VOID) = 0;
protected:
    virtual ~IUnknown() {}
};
struct ANX_NOVTABLE IClassFactory : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, VOID** ppvObject) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockServer(int fLock) = 0;
};
#else
typedef struct IUnknown IUnknown;
typedef struct IUnknownVtbl {
    ANX_STDMETHOD(QueryInterface)(IUnknown* This, REFIID riid, VOID **ppvObject);
    ANX_STDMETHOD_(UINT32, AddRef)(IUnknown* This);
    ANX_STDMETHOD_(UINT32, Release)(IUnknown* This);
} IUnknownVtbl;
struct IUnknown { const IUnknownVtbl* lpVtbl; };

typedef struct IClassFactory IClassFactory;
typedef struct IClassFactoryVtbl {
    ANX_STDMETHOD(QueryInterface)(IClassFactory* This, REFIID riid, VOID **ppvObject);
    ANX_STDMETHOD_(UINT32, AddRef)(IClassFactory* This);
    ANX_STDMETHOD_(UINT32, Release)(IClassFactory* This);
    ANX_STDMETHOD(CreateInstance)(IClassFactory* This, IUnknown* pUnkOuter, REFIID riid, VOID** ppvObject);
    ANX_STDMETHOD(LockServer)(IClassFactory* This, int fLock);
} IClassFactoryVtbl;
struct IClassFactory { const IClassFactoryVtbl* lpVtbl; };
#endif

/* IUnknown and IClassFactory GUIDs */
ANX_DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
ANX_DEFINE_GUID(IID_IClassFactory, 0x00000001, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

/* Unprefixed COBJMACROS */
#ifndef COBJMACROS
#   define COBJMACROS 1
#   define IUnknown_QueryInterface(This, riid, ppv)   ((This)->lpVtbl->QueryInterface((This),(riid),(ppv)))
#   define IUnknown_AddRef(This)                      ((This)->lpVtbl->AddRef((This)))
#   define IUnknown_Release(This)                     ((This)->lpVtbl->Release((This)))
#endif
#define IClassFactory_CreateInstance(This, pOuter, riid, ppv) ((This)->lpVtbl->CreateInstance((This),(pOuter),(riid),(ppv)))
#define IClassFactory_LockServer(This, f)                    ((This)->lpVtbl->LockServer((This),(f)))

/* --------------------------------------------------------------- */
/*  Example: unified interface declaration (commented out)         */
/* --------------------------------------------------------------- */
/*
   To define a custom COM interface, use this pattern:

   // {7E6B9B33-4B40-4D1D-9F2B-6F7C6F6C4C11}
   #define ANX_IID_IAnxSample "7E6B9B33-4B40-4D1D-9F2B-6F7C6F6C4C11"
   ANX_DEFINE_GUID(IID_IAnxSample, 0x7E6B9B33,0x4B40,0x4D1D,0x9F,0x2B,0x6F,0x7C,0x6F,0x6C,0x4C,0x11);

   ANX_BEGIN_INTERFACE(IAnxSample, IUnknown, IID_IAnxSample, ANX_IID_IAnxSample)
       ANX_IFACE_METHOD(HRESULT, Reset,    (VOID))
       ANX_IFACE_METHOD(HRESULT, GetValue, (OUT UINT32* value))
   ANX_END_INTERFACE(IAnxSample)
*/
