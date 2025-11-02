/** @file
  VINIL Type System COM Implementation

  Production type system with singleton caching and COM interfaces.
  Supports graphics (scalars, vectors, matrices) and compute (all types).

  Copyright (C) 2003-2007 Hans-Martin Will.
  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#include <vinil/types.h>
#include <stdlib.h>
#include <string.h>

//
// Type Information Table
//

typedef struct _TYPE_INFO {
  VINIL_TYPE_VALUE  TypeValue;
  UINTN             Size;         /* Size in bytes */
  UINT32            Components;   /* Number of components */
  BOOLEAN           IsScalar;
  BOOLEAN           IsVector;
  BOOLEAN           IsMatrix;
} TYPE_INFO;

static const TYPE_INFO gTypeInfoTable[] = {
  /* Scalars */
  { VINIL_TYPE_BOOL,     1,  1, TRUE,  FALSE, FALSE },
  { VINIL_TYPE_INT,      4,  1, TRUE,  FALSE, FALSE },
  { VINIL_TYPE_UINT,     4,  1, TRUE,  FALSE, FALSE },
  { VINIL_TYPE_FLOAT,    4,  1, TRUE,  FALSE, FALSE },
  { VINIL_TYPE_DOUBLE,   8,  1, TRUE,  FALSE, FALSE },
  { VINIL_TYPE_HALF,     2,  1, TRUE,  FALSE, FALSE },
  { VINIL_TYPE_CHAR,     1,  1, TRUE,  FALSE, FALSE },
  { VINIL_TYPE_UCHAR,    1,  1, TRUE,  FALSE, FALSE },
  { VINIL_TYPE_SHORT,    2,  1, TRUE,  FALSE, FALSE },
  { VINIL_TYPE_USHORT,   2,  1, TRUE,  FALSE, FALSE },
  { VINIL_TYPE_LONG,     8,  1, TRUE,  FALSE, FALSE },
  { VINIL_TYPE_ULONG,    8,  1, TRUE,  FALSE, FALSE },

  /* Boolean vectors */
  { VINIL_TYPE_BOOL_VEC2,   2,  2, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_BOOL_VEC3,   3,  3, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_BOOL_VEC4,   4,  4, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_BOOL_VEC8,   8,  8, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_BOOL_VEC16, 16, 16, FALSE, TRUE,  FALSE },

  /* Integer vectors */
  { VINIL_TYPE_INT_VEC2,    8,  2, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_INT_VEC3,   12,  3, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_INT_VEC4,   16,  4, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_INT_VEC8,   32,  8, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_INT_VEC16,  64, 16, FALSE, TRUE,  FALSE },

  /* Unsigned integer vectors */
  { VINIL_TYPE_UINT_VEC2,   8,  2, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_UINT_VEC3,  12,  3, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_UINT_VEC4,  16,  4, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_UINT_VEC8,  32,  8, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_UINT_VEC16, 64, 16, FALSE, TRUE,  FALSE },

  /* Float vectors */
  { VINIL_TYPE_FLOAT_VEC2,   8,  2, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_FLOAT_VEC3,  12,  3, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_FLOAT_VEC4,  16,  4, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_FLOAT_VEC8,  32,  8, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_FLOAT_VEC16, 64, 16, FALSE, TRUE,  FALSE },

  /* Double vectors */
  { VINIL_TYPE_DOUBLE_VEC2,   16,  2, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_DOUBLE_VEC3,   24,  3, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_DOUBLE_VEC4,   32,  4, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_DOUBLE_VEC8,   64,  8, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_DOUBLE_VEC16, 128, 16, FALSE, TRUE,  FALSE },

  /* Half vectors */
  { VINIL_TYPE_HALF_VEC2,   4,  2, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_HALF_VEC3,   6,  3, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_HALF_VEC4,   8,  4, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_HALF_VEC8,  16,  8, FALSE, TRUE,  FALSE },
  { VINIL_TYPE_HALF_VEC16, 32, 16, FALSE, TRUE,  FALSE },

  /* Float matrices */
  { VINIL_TYPE_FLOAT_MAT2,   16,  4, FALSE, FALSE, TRUE },
  { VINIL_TYPE_FLOAT_MAT3,   36,  9, FALSE, FALSE, TRUE },
  { VINIL_TYPE_FLOAT_MAT4,   64, 16, FALSE, FALSE, TRUE },
  { VINIL_TYPE_FLOAT_MAT2x3, 24,  6, FALSE, FALSE, TRUE },
  { VINIL_TYPE_FLOAT_MAT2x4, 32,  8, FALSE, FALSE, TRUE },
  { VINIL_TYPE_FLOAT_MAT3x2, 24,  6, FALSE, FALSE, TRUE },
  { VINIL_TYPE_FLOAT_MAT3x4, 48, 12, FALSE, FALSE, TRUE },
  { VINIL_TYPE_FLOAT_MAT4x2, 32,  8, FALSE, FALSE, TRUE },
  { VINIL_TYPE_FLOAT_MAT4x3, 48, 12, FALSE, FALSE, TRUE },

  /* Double matrices */
  { VINIL_TYPE_DOUBLE_MAT2,  32,  4, FALSE, FALSE, TRUE },
  { VINIL_TYPE_DOUBLE_MAT3,  72,  9, FALSE, FALSE, TRUE },
  { VINIL_TYPE_DOUBLE_MAT4, 128, 16, FALSE, FALSE, TRUE },
};

#define TYPE_INFO_COUNT  (sizeof(gTypeInfoTable) / sizeof(TYPE_INFO))

//
// Type Implementation Structure
//

typedef struct _VINIL_TYPE_IMPL {
  IVinilTypeVtbl      *lpVtbl;
  UINT32              RefCount;
  VINIL_TYPE_VALUE    TypeValue;
  VINIL_PRECISION     Precision;
  CONST TYPE_INFO     *Info;

  /* For composite types */
  IVinilType          *ElementType;      /* Array/Pointer element type */
  UINTN               NumElements;       /* Array element count */
  VINIL_ADDRESS_SPACE AddressSpace;      /* Pointer address space */
} VINIL_TYPE_IMPL;

//
// Singleton Cache (immutable, never released)
//

#define MAX_CACHED_TYPES  512

typedef struct _TYPE_CACHE {
  VINIL_TYPE_IMPL  Types[MAX_CACHED_TYPES];
  UINT32           Count;
  BOOLEAN          Initialized;
} TYPE_CACHE;

static TYPE_CACHE gTypeCache = { 0 };

//
// Forward Declarations
//

static HRESULT STDMETHODCALLTYPE Type_QueryInterface (IVinilType *This, REFIID riid, void **ppvObject);
static UINT32 STDMETHODCALLTYPE Type_AddRef (IVinilType *This);
static UINT32 STDMETHODCALLTYPE Type_Release (IVinilType *This);
static HRESULT STDMETHODCALLTYPE Type_GetTypeValue (IVinilType *This, VINIL_TYPE_VALUE *TypeValue);
static HRESULT STDMETHODCALLTYPE Type_GetPrecision (IVinilType *This, VINIL_PRECISION *Precision);
static HRESULT STDMETHODCALLTYPE Type_GetSize (IVinilType *This, UINTN *Size);
static HRESULT STDMETHODCALLTYPE Type_IsScalar (IVinilType *This, BOOLEAN *IsScalar);
static HRESULT STDMETHODCALLTYPE Type_IsVector (IVinilType *This, BOOLEAN *IsVector);
static HRESULT STDMETHODCALLTYPE Type_IsMatrix (IVinilType *This, BOOLEAN *IsMatrix);
static HRESULT STDMETHODCALLTYPE Type_GetComponents (IVinilType *This, UINT32 *Components);

//
// Vtable
//

static IVinilTypeVtbl gTypeVtbl = {
  Type_QueryInterface,
  Type_AddRef,
  Type_Release,
  Type_GetTypeValue,
  Type_GetPrecision,
  Type_GetSize,
  Type_IsScalar,
  Type_IsVector,
  Type_IsMatrix,
  Type_GetComponents
};

//
// Helper Functions
//

static
CONST TYPE_INFO *
GetTypeInfo (
  VINIL_TYPE_VALUE  TypeValue
  )
{
  for (UINTN i = 0; i < TYPE_INFO_COUNT; i++) {
    if (gTypeInfoTable[i].TypeValue == TypeValue) {
      return &gTypeInfoTable[i];
    }
  }

  return NULL;
}

//
// IUnknown Implementation
//

static
HRESULT
STDMETHODCALLTYPE
Type_QueryInterface (
  IVinilType  *This,
  REFIID      riid,
  void        **ppvObject
  )
{
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  if (IsEqualGUID (*riid, IID_IUnknown) ||
      IsEqualGUID (*riid, IID_IVinilType))
  {
    *ppvObject = This;
    Type_AddRef (This);
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static
UINT32
STDMETHODCALLTYPE
Type_AddRef (
  IVinilType  *This
  )
{
  VINIL_TYPE_IMPL  *Type = (VINIL_TYPE_IMPL *)This;

  /* Singleton types are immortal */
  if (Type->RefCount == 0xFFFFFFFF) {
    return 0xFFFFFFFF;
  }

  return ++Type->RefCount;
}

static
UINT32
STDMETHODCALLTYPE
Type_Release (
  IVinilType  *This
  )
{
  VINIL_TYPE_IMPL  *Type = (VINIL_TYPE_IMPL *)This;
  UINT32           RefCount;

  /* Singleton types are immortal */
  if (Type->RefCount == 0xFFFFFFFF) {
    return 0xFFFFFFFF;
  }

  RefCount = --Type->RefCount;
  if (RefCount == 0) {
    /* Release element type for composite types */
    if (Type->ElementType != NULL) {
      Type->ElementType->lpVtbl->Release (Type->ElementType);
    }

    free (Type);
  }

  return RefCount;
}

//
// IVinilType Implementation
//

static
HRESULT
STDMETHODCALLTYPE
Type_GetTypeValue (
  IVinilType        *This,
  VINIL_TYPE_VALUE  *TypeValue
  )
{
  VINIL_TYPE_IMPL  *Type = (VINIL_TYPE_IMPL *)This;

  if (TypeValue == NULL) {
    return E_POINTER;
  }

  *TypeValue = Type->TypeValue;
  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Type_GetPrecision (
  IVinilType       *This,
  VINIL_PRECISION  *Precision
  )
{
  VINIL_TYPE_IMPL  *Type = (VINIL_TYPE_IMPL *)This;

  if (Precision == NULL) {
    return E_POINTER;
  }

  *Precision = Type->Precision;
  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Type_GetSize (
  IVinilType  *This,
  UINTN       *Size
  )
{
  VINIL_TYPE_IMPL  *Type = (VINIL_TYPE_IMPL *)This;

  if (Size == NULL) {
    return E_POINTER;
  }

  /* Basic types use info table */
  if (Type->Info != NULL) {
    *Size = Type->Info->Size;
    return S_OK;
  }

  /* Composite types */
  if (Type->TypeValue == VINIL_TYPE_ARRAY && Type->ElementType != NULL) {
    UINTN  ElementSize;
    HRESULT  Result;

    Result = Type->ElementType->lpVtbl->GetSize (Type->ElementType, &ElementSize);
    if (FAILED (Result)) {
      return Result;
    }

    *Size = ElementSize * Type->NumElements;
    return S_OK;
  }

  if (Type->TypeValue == VINIL_TYPE_POINTER) {
    *Size = sizeof (VOID *);
    return S_OK;
  }

  return E_FAIL;
}

static
HRESULT
STDMETHODCALLTYPE
Type_IsScalar (
  IVinilType  *This,
  BOOLEAN     *IsScalar
  )
{
  VINIL_TYPE_IMPL  *Type = (VINIL_TYPE_IMPL *)This;

  if (IsScalar == NULL) {
    return E_POINTER;
  }

  if (Type->Info != NULL) {
    *IsScalar = Type->Info->IsScalar;
  } else {
    *IsScalar = FALSE;
  }

  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Type_IsVector (
  IVinilType  *This,
  BOOLEAN     *IsVector
  )
{
  VINIL_TYPE_IMPL  *Type = (VINIL_TYPE_IMPL *)This;

  if (IsVector == NULL) {
    return E_POINTER;
  }

  if (Type->Info != NULL) {
    *IsVector = Type->Info->IsVector;
  } else {
    *IsVector = FALSE;
  }

  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Type_IsMatrix (
  IVinilType  *This,
  BOOLEAN     *IsMatrix
  )
{
  VINIL_TYPE_IMPL  *Type = (VINIL_TYPE_IMPL *)This;

  if (IsMatrix == NULL) {
    return E_POINTER;
  }

  if (Type->Info != NULL) {
    *IsMatrix = Type->Info->IsMatrix;
  } else {
    *IsMatrix = FALSE;
  }

  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Type_GetComponents (
  IVinilType  *This,
  UINT32      *Components
  )
{
  VINIL_TYPE_IMPL  *Type = (VINIL_TYPE_IMPL *)This;

  if (Components == NULL) {
    return E_POINTER;
  }

  if (Type->Info == NULL) {
    return E_FAIL;
  }

  *Components = Type->Info->Components;
  return S_OK;
}

//
// Factory Functions
//

HRESULT
VinilGetBasicType (
  VINIL_TYPE_VALUE  TypeValue,
  VINIL_PRECISION   Precision,
  IVinilType        **Type
  )
{
  CONST TYPE_INFO  *Info;
  VINIL_TYPE_IMPL  *TypeImpl;

  if (Type == NULL) {
    return E_POINTER;
  }

  /* Get type info */
  Info = GetTypeInfo (TypeValue);
  if (Info == NULL) {
    return E_INVALIDARG;
  }

  /* Search singleton cache */
  for (UINT32 i = 0; i < gTypeCache.Count; i++) {
    TypeImpl = &gTypeCache.Types[i];
    if (TypeImpl->TypeValue == TypeValue && TypeImpl->Precision == Precision) {
      *Type = (IVinilType *)TypeImpl;
      /* Singletons don't increment refcount */
      return S_OK;
    }
  }

  /* Create new singleton */
  if (gTypeCache.Count >= MAX_CACHED_TYPES) {
    return E_OUTOFMEMORY;
  }

  TypeImpl = &gTypeCache.Types[gTypeCache.Count++];
  memset (TypeImpl, 0, sizeof (VINIL_TYPE_IMPL));
  TypeImpl->lpVtbl = &gTypeVtbl;
  TypeImpl->RefCount = 0xFFFFFFFF;  /* Immortal singleton */
  TypeImpl->TypeValue = TypeValue;
  TypeImpl->Precision = Precision;
  TypeImpl->Info = Info;
  TypeImpl->ElementType = NULL;

  *Type = (IVinilType *)TypeImpl;
  return S_OK;
}

HRESULT
VinilCreateArrayType (
  IVinilType  *ElementType,
  UINTN       NumElements,
  IVinilType  **ArrayType
  )
{
  VINIL_TYPE_IMPL  *TypeImpl;

  if (ElementType == NULL || ArrayType == NULL) {
    return E_POINTER;
  }

  if (NumElements == 0) {
    return E_INVALIDARG;
  }

  /* Allocate dynamically (not a singleton) */
  TypeImpl = (VINIL_TYPE_IMPL *)malloc (sizeof (VINIL_TYPE_IMPL));
  if (TypeImpl == NULL) {
    return E_OUTOFMEMORY;
  }

  memset (TypeImpl, 0, sizeof (VINIL_TYPE_IMPL));
  TypeImpl->lpVtbl = &gTypeVtbl;
  TypeImpl->RefCount = 1;
  TypeImpl->TypeValue = VINIL_TYPE_ARRAY;
  TypeImpl->Precision = VinilPrecisionUndefined;
  TypeImpl->Info = NULL;
  TypeImpl->ElementType = ElementType;
  TypeImpl->NumElements = NumElements;

  /* AddRef element type */
  ElementType->lpVtbl->AddRef (ElementType);

  *ArrayType = (IVinilType *)TypeImpl;
  return S_OK;
}

HRESULT
VinilCreatePointerType (
  IVinilType           *PointeeType,
  VINIL_ADDRESS_SPACE  AddrSpace,
  IVinilType           **PointerType
  )
{
  VINIL_TYPE_IMPL  *TypeImpl;

  if (PointeeType == NULL || PointerType == NULL) {
    return E_POINTER;
  }

  /* Allocate dynamically */
  TypeImpl = (VINIL_TYPE_IMPL *)malloc (sizeof (VINIL_TYPE_IMPL));
  if (TypeImpl == NULL) {
    return E_OUTOFMEMORY;
  }

  memset (TypeImpl, 0, sizeof (VINIL_TYPE_IMPL));
  TypeImpl->lpVtbl = &gTypeVtbl;
  TypeImpl->RefCount = 1;
  TypeImpl->TypeValue = VINIL_TYPE_POINTER;
  TypeImpl->Precision = VinilPrecisionUndefined;
  TypeImpl->Info = NULL;
  TypeImpl->ElementType = PointeeType;
  TypeImpl->AddressSpace = AddrSpace;

  /* AddRef pointee type */
  PointeeType->lpVtbl->AddRef (PointeeType);

  *PointerType = (IVinilType *)TypeImpl;
  return S_OK;
}
