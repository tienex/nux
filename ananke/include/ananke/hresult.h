/*++
    Module Name:

        hresult.h

    Abstract:

        HRESULT type and common error codes (COM-style).

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>

/* --------------------------------------------------------------- */
/*  HRESULT and common codes.                                      */
/* --------------------------------------------------------------- */
#ifndef HRESULT_DEFINED
#define HRESULT_DEFINED 1
    typedef INT32 HRESULT;
#   define S_OK               ((HRESULT)0)
#   define S_FALSE            ((HRESULT)1)
#   define E_UNEXPECTED       ((HRESULT)0x8000FFFFL)
#   define E_NOTIMPL          ((HRESULT)0x80004001L)
#   define E_NOINTERFACE      ((HRESULT)0x80004002L)
#   define E_POINTER          ((HRESULT)0x80004003L)
#   define E_ABORT            ((HRESULT)0x80004004L)
#   define E_FAIL             ((HRESULT)0x80004005L)
#   define E_ACCESSDENIED     ((HRESULT)0x80070005L)
#   define E_HANDLE           ((HRESULT)0x80070006L)
#   define E_OUTOFMEMORY      ((HRESULT)0x8007000EL)
#   define E_INVALIDARG       ((HRESULT)0x80070057L)
#   define E_BOUNDS           ((HRESULT)0x8000000BL)
#   define E_BUSY             ((HRESULT)0x8000000AL)
#   define E_TIMEOUT          ((HRESULT)0x800705B4L)
#   define SUCCEEDED(hr)      ((hr) >= 0)
#   define FAILED(hr)         ((hr) < 0)
#endif
