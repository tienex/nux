/*++
    Module Name:

        guid.h

    Abstract:

        GUID/IID/CLSID support, comparison, and definition macros.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>

/* --------------------------------------------------------------- */
/*  GUID / IID / CLSID support (+ pointer/ref families).           */
/* --------------------------------------------------------------- */
#ifndef GUID_DEFINED
#define GUID_DEFINED 1
    typedef struct _GUID { UINT32 Data1; UINT16 Data2; UINT16 Data3; UINT8 Data4[8]; } GUID;
    typedef GUID IID;    typedef GUID CLSID;
#endif

#ifdef __cplusplus
    typedef const GUID &REFGUID; typedef const IID &REFIID; typedef const CLSID &REFCLSID;
#else
#   define REFGUID  const GUID *
#   define REFIID   const IID  *
#   define REFCLSID const CLSID*
#endif

#ifndef IsEqualGUID
#   define IsEqualGUID(a,b) \
        (((a).Data1==(b).Data1) && ((a).Data2==(b).Data2) && ((a).Data3==(b).Data3) && \
         ((a).Data4[0]==(b).Data4[0]) && ((a).Data4[1]==(b).Data4[1]) && \
         ((a).Data4[2]==(b).Data4[2]) && ((a).Data4[3]==(b).Data4[3]) && \
         ((a).Data4[4]==(b).Data4[4]) && ((a).Data4[5]==(b).Data4[5]) && \
         ((a).Data4[6]==(b).Data4[6]) && ((a).Data4[7]==(b).Data4[7]))
#endif

ANX_DECLARE_TPTRS(GUID);
#ifdef __cplusplus
ANX_DECLARE_TREFS(GUID);
#endif

/* GUID definition helper honoring INITGUID */
#ifndef ANX_DEFINE_GUID
#   ifdef INITGUID
#       define ANX_DEFINE_GUID(name, l,w1,w2,b0,b1,b2,b3,b4,b5,b6,b7) \
            const GUID name = { (l),(w1),(w2), { (b0),(b1),(b2),(b3),(b4),(b5),(b6),(b7) } }
#   else
#       define ANX_DEFINE_GUID(name, l,w1,w2,b0,b1,b2,b3,b4,b5,b6,b7) \
            extern const GUID name
#   endif
#endif
