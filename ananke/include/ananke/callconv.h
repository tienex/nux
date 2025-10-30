/++
    Module Name:

        callconv.h

    Abstract:

        Calling conventions, NOVTABLE, UUID attributes, and __uuidof emulation.

    Environment:

        C and C++ compatible.
--/

#pragma once

#include <ananke/hresult.h>
#include <ananke/guid.h>

/* --------------------------------------------------------------- */
/*  Calling conventions, NOVTABLE, UUID attributes.                */
/* --------------------------------------------------------------- */
#ifndef STDMETHODCALLTYPE
#   if defined(_MSC_VER) || (defined(__clang__) && defined(_MSC_EXTENSIONS))
#       define STDMETHODCALLTYPE __stdcall
#       define STDAPICALLTYPE   __stdcall
#       define ANX_NOVTABLE     __declspec(novtable)
#       define ANX_UUID_ATTR(x) __declspec(uuid(x))
#   elif defined(__i386__)
#       define STDMETHODCALLTYPE __attribute__((stdcall))
#       define STDAPICALLTYPE   __attribute__((stdcall))
#       define ANX_NOVTABLE
#       define ANX_UUID_ATTR(x)
#   else
#       define STDMETHODCALLTYPE /* default */
#       define STDAPICALLTYPE
#       define ANX_NOVTABLE
#       define ANX_UUID_ATTR(x)
#   endif
#endif

#ifndef ANX_STDMETHOD
#   define ANX_STDMETHOD(m)        HRESULT (STDMETHODCALLTYPE * m)
#   define ANX_STDMETHOD_(t, m)    t       (STDMETHODCALLTYPE * m)
#endif
#ifndef ANX_STDMETHODIMP
#   define ANX_STDMETHODIMP        HRESULT STDAPICALLTYPE
#   define ANX_STDMETHODIMP_(t)    t       STDAPICALLTYPE
#endif

/* --------------------------------------------------------------- */
/*  __uuidof support and emulation.                                */
/* --------------------------------------------------------------- */
#ifdef __cplusplus
#   if defined(_MSC_VER) || (defined(__clang__) && defined(_MSC_EXTENSIONS))
#       define ANX_HAS_NATIVE_UUIDOF 1
#       define ANX_UUIDOF(T) __uuidof(T)
#   else
        template <class T> struct ANX_uuidof_trait { static const GUID& get(); };
#       define ANX_DECLARE_UUIDOF(T, GINIT) \
            template<> struct ANX_uuidof_trait< T > { \
                static const GUID& get() { static const GUID k = GINIT; return k; } \
            };
#       define ANX_UUIDOF(T) (ANX_uuidof_trait< T >::get())
#   endif
#else
#   define ANX_DECLARE_UUIDOF_C(TypedefName, GuidSymbol) \
        static CONST GUID* Anx_uuidof_fn_##TypedefName(VOID) { return &(GuidSymbol); }
#   define ANX_UUIDOF(TypedefName) (*(Anx_uuidof_fn_##TypedefName()))
#endif

#ifdef __cplusplus
#   define ANX_MIDL_INTERFACE(x) struct ANX_UUID_ATTR(x) ANX_NOVTABLE
#else
#   define ANX_MIDL_INTERFACE(x) struct
#endif
