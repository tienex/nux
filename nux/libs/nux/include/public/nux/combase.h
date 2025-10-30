/** @file
  NUX COM Base Definitions

  Provides fundamental COM infrastructure including IUnknown interface,
  GUID definitions, HRESULT types, and standard COM macros.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __nux_combase_h__
#define __nux_combase_h__

#include <stdint.h>
#include <stdbool.h>

//
// Standard COM Types
//

typedef uint32_t        HRESULT;
typedef uint32_t        ULONG;
typedef void            VOID;
typedef uint8_t         UINT8;
typedef uint16_t        UINT16;
typedef uint32_t        UINT32;
typedef uint64_t        UINT64;
typedef int8_t          INT8;
typedef int16_t         INT16;
typedef int32_t         INT32;
typedef int64_t         INT64;
typedef unsigned long   UINTN;
typedef long            INTN;
typedef bool            BOOLEAN;
typedef char            CHAR8;
typedef char            *PCHAR8;

#ifndef TRUE
#define TRUE  ((BOOLEAN)1)
#endif

#ifndef FALSE
#define FALSE ((BOOLEAN)0)
#endif

#ifndef NULL
#define NULL  ((VOID *)0)
#endif

//
// GUID Structure
//

typedef struct _GUID {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} GUID;

//
// Interface ID and Class ID
//

typedef GUID IID;
typedef GUID CLSID;

//
// Standard HRESULT Values
//

#define S_OK                            0x00000000
#define S_FALSE                         0x00000001
#define E_NOTIMPL                       0x80000001
#define E_NOINTERFACE                   0x80000002
#define E_POINTER                       0x80000003
#define E_ABORT                         0x80000004
#define E_FAIL                          0x80000005
#define E_UNEXPECTED                    0x8000FFFF
#define E_ACCESSDENIED                  0x80000009
#define E_HANDLE                        0x80000006
#define E_OUTOFMEMORY                   0x80000007
#define E_INVALIDARG                    0x80000008

//
// HRESULT Macros
//

#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#define FAILED(hr)      (((HRESULT)(hr)) < 0)

//
// IUnknown Interface GUID
//

#define IID_IUNKNOWN \
  { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } }

//
// Forward Declaration
//

typedef struct _IUnknown IUnknown;

/**
  IUnknown Interface VTable

  Base interface for all COM interfaces. Provides reference counting
  and interface querying capabilities.
**/
typedef struct _IUnknownVtbl {
  /**
    Query for a supported interface.

    @param[in]  This      Pointer to the IUnknown instance.
    @param[in]  riid      Reference to the interface identifier.
    @param[out] ppvObject Pointer to receive the interface pointer.

    @retval S_OK              Interface is supported.
    @retval E_NOINTERFACE     Interface is not supported.
    @retval E_POINTER         ppvObject is NULL.
  **/
  HRESULT
  (*QueryInterface)(
    IN  IUnknown    *This,
    IN  IID         *riid,
    OUT VOID        **ppvObject
    );

  /**
    Increment the reference count.

    @param[in]  This  Pointer to the IUnknown instance.

    @return The new reference count.
  **/
  ULONG
  (*AddRef)(
    IN  IUnknown    *This
    );

  /**
    Decrement the reference count.

    @param[in]  This  Pointer to the IUnknown instance.

    @return The new reference count.
  **/
  ULONG
  (*Release)(
    IN  IUnknown    *This
    );
} IUnknownVtbl;

/**
  IUnknown Interface Structure
**/
struct _IUnknown {
  IUnknownVtbl  *lpVtbl;
};

//
// Helper Macros for COM Interfaces
//

#define INTERFACE_DECL(iface) \
  typedef struct _##iface iface; \
  typedef struct _##iface##Vtbl iface##Vtbl;

#define INTERFACE_INHERIT_IUNKNOWN(iface) \
  struct _##iface { \
    iface##Vtbl *lpVtbl; \
  };

//
// GUID Comparison
//

/**
  Compare two GUIDs for equality.

  @param[in]  Guid1  Pointer to the first GUID.
  @param[in]  Guid2  Pointer to the second GUID.

  @retval TRUE   GUIDs are equal.
  @retval FALSE  GUIDs are not equal.
**/
static inline BOOLEAN
CompareGuid (
  IN CONST GUID   *Guid1,
  IN CONST GUID   *Guid2
  )
{
  UINT64  *p1;
  UINT64  *p2;

  p1 = (UINT64 *)Guid1;
  p2 = (UINT64 *)Guid2;

  return (BOOLEAN)(p1[0] == p2[0] && p1[1] == p2[1]);
}

/**
  Copy a GUID.

  @param[out] DestGuid  Pointer to the destination GUID.
  @param[in]  SrcGuid   Pointer to the source GUID.
**/
static inline VOID
CopyGuid (
  OUT GUID        *DestGuid,
  IN  CONST GUID  *SrcGuid
  )
{
  UINT64  *Dest;
  UINT64  *Src;

  Dest = (UINT64 *)DestGuid;
  Src  = (UINT64 *)SrcGuid;

  Dest[0] = Src[0];
  Dest[1] = Src[1];
}

//
// Standard Calling Conventions
//

#define IN
#define OUT
#define OPTIONAL
#define CONST   const

#endif // NUX_COMBASE_H
