/** @file
  VINIL Type System COM Interfaces

  Unified type system for graphics and compute workloads.

  Copyright (C) 2003-2007 Hans-Martin Will.
  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#pragma once
#include <vinil/base.h>
#include <vinil/memory.h>
#include <ananke/com.h>

//
// GUID
//

ANX_DEFINE_GUID(IID_IVinilType, 0x89012345, 0x8901, 0x8901, 0x89, 0x01, 0x23, 0x45, 0xAB, 0xCD, 0xEF, 0x67);

//
// Type Value Enumeration
//

typedef enum _VINIL_TYPE_VALUE {
    /* Invalid/special types */
    VINIL_TYPE_INVALID = 0,
    VINIL_TYPE_VOID,

    /* Scalar types - Graphics + Compute */
    VINIL_TYPE_BOOL,
    VINIL_TYPE_INT,
    VINIL_TYPE_UINT,        /* Compute extension */
    VINIL_TYPE_FLOAT,
    VINIL_TYPE_DOUBLE,      /* Compute extension */
    VINIL_TYPE_HALF,        /* Compute extension (fp16) */

    /* Additional scalar types for compute */
    VINIL_TYPE_CHAR,        /* 8-bit signed */
    VINIL_TYPE_UCHAR,       /* 8-bit unsigned */
    VINIL_TYPE_SHORT,       /* 16-bit signed */
    VINIL_TYPE_USHORT,      /* 16-bit unsigned */
    VINIL_TYPE_LONG,        /* 64-bit signed */
    VINIL_TYPE_ULONG,       /* 64-bit unsigned */

    /* Boolean vectors - Graphics + Compute */
    VINIL_TYPE_BOOL_VEC2,
    VINIL_TYPE_BOOL_VEC3,
    VINIL_TYPE_BOOL_VEC4,
    VINIL_TYPE_BOOL_VEC8,   /* Compute extension */
    VINIL_TYPE_BOOL_VEC16,  /* Compute extension */

    /* Integer vectors - Graphics + Compute */
    VINIL_TYPE_INT_VEC2,
    VINIL_TYPE_INT_VEC3,
    VINIL_TYPE_INT_VEC4,
    VINIL_TYPE_INT_VEC8,    /* Compute extension */
    VINIL_TYPE_INT_VEC16,   /* Compute extension */

    /* Unsigned integer vectors - Compute */
    VINIL_TYPE_UINT_VEC2,
    VINIL_TYPE_UINT_VEC3,
    VINIL_TYPE_UINT_VEC4,
    VINIL_TYPE_UINT_VEC8,
    VINIL_TYPE_UINT_VEC16,

    /* Float vectors - Graphics + Compute */
    VINIL_TYPE_FLOAT_VEC2,
    VINIL_TYPE_FLOAT_VEC3,
    VINIL_TYPE_FLOAT_VEC4,
    VINIL_TYPE_FLOAT_VEC8,  /* Compute extension */
    VINIL_TYPE_FLOAT_VEC16, /* Compute extension */

    /* Double vectors - Compute */
    VINIL_TYPE_DOUBLE_VEC2,
    VINIL_TYPE_DOUBLE_VEC3,
    VINIL_TYPE_DOUBLE_VEC4,
    VINIL_TYPE_DOUBLE_VEC8,
    VINIL_TYPE_DOUBLE_VEC16,

    /* Half vectors - Compute (fp16) */
    VINIL_TYPE_HALF_VEC2,
    VINIL_TYPE_HALF_VEC3,
    VINIL_TYPE_HALF_VEC4,
    VINIL_TYPE_HALF_VEC8,
    VINIL_TYPE_HALF_VEC16,

    /* Matrices - Graphics */
    VINIL_TYPE_FLOAT_MAT2,
    VINIL_TYPE_FLOAT_MAT3,
    VINIL_TYPE_FLOAT_MAT4,
    VINIL_TYPE_FLOAT_MAT2x3,
    VINIL_TYPE_FLOAT_MAT2x4,
    VINIL_TYPE_FLOAT_MAT3x2,
    VINIL_TYPE_FLOAT_MAT3x4,
    VINIL_TYPE_FLOAT_MAT4x2,
    VINIL_TYPE_FLOAT_MAT4x3,

    /* Double matrices - Compute */
    VINIL_TYPE_DOUBLE_MAT2,
    VINIL_TYPE_DOUBLE_MAT3,
    VINIL_TYPE_DOUBLE_MAT4,

    /* Samplers - Graphics */
    VINIL_TYPE_SAMPLER_2D,
    VINIL_TYPE_SAMPLER_3D,
    VINIL_TYPE_SAMPLER_CUBE,
    VINIL_TYPE_SAMPLER_2D_SHADOW,
    VINIL_TYPE_SAMPLER_2D_ARRAY,

    /* Images - Compute (read/write access) */
    VINIL_TYPE_IMAGE_1D,
    VINIL_TYPE_IMAGE_2D,
    VINIL_TYPE_IMAGE_3D,
    VINIL_TYPE_IMAGE_1D_ARRAY,
    VINIL_TYPE_IMAGE_2D_ARRAY,

    /* Pointers - Compute */
    VINIL_TYPE_POINTER,

    /* Composite types */
    VINIL_TYPE_ARRAY,
    VINIL_TYPE_STRUCT,
    VINIL_TYPE_FUNCTION,
} VINIL_TYPE_VALUE;

//
// IVinilType Interface
//

ANX_BEGIN_INTERFACE(IVinilType, IUnknown, IID_IVinilType, "89012345-8901-8901-8901-2345ABCDEF67")
    /**
      Get type value (scalar, vector, matrix, etc.).

      @param[out]  TypeValue  Type value enumeration.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, GetTypeValue, (VINIL_TYPE_VALUE *TypeValue))

    /**
      Get type precision.

      @param[out]  Precision  Type precision.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, GetPrecision, (VINIL_PRECISION *Precision))

    /**
      Get size in bytes.

      @param[out]  Size  Type size in bytes.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, GetSize, (UINTN *Size))

    /**
      Check if type is scalar.

      @param[out]  IsScalar  TRUE if scalar, FALSE otherwise.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, IsScalar, (BOOLEAN *IsScalar))

    /**
      Check if type is vector.

      @param[out]  IsVector  TRUE if vector, FALSE otherwise.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, IsVector, (BOOLEAN *IsVector))

    /**
      Check if type is matrix.

      @param[out]  IsMatrix  TRUE if matrix, FALSE otherwise.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
    **/
    ANX_IFACE_METHOD(HRESULT, IsMatrix, (BOOLEAN *IsMatrix))

    /**
      Get number of components (for vectors/matrices).

      @param[out]  Components  Number of components.

      @retval  S_OK       Success.
      @retval  E_POINTER  Invalid pointer.
      @retval  E_FAIL     Not a vector or matrix type.
    **/
    ANX_IFACE_METHOD(HRESULT, GetComponents, (UINT32 *Components))
ANX_END_INTERFACE(IVinilType, IID_IVinilType)

//
// COBJMACROS
//

#ifdef COBJMACROS

#define IVinilType_QueryInterface(This, riid, ppv) \
    (This)->lpVtbl->QueryInterface(This, riid, ppv)
#define IVinilType_AddRef(This) \
    (This)->lpVtbl->AddRef(This)
#define IVinilType_Release(This) \
    (This)->lpVtbl->Release(This)
#define IVinilType_GetTypeValue(This, TypeValue) \
    (This)->lpVtbl->GetTypeValue(This, TypeValue)
#define IVinilType_GetPrecision(This, Precision) \
    (This)->lpVtbl->GetPrecision(This, Precision)
#define IVinilType_GetSize(This, Size) \
    (This)->lpVtbl->GetSize(This, Size)
#define IVinilType_IsScalar(This, IsScalar) \
    (This)->lpVtbl->IsScalar(This, IsScalar)
#define IVinilType_IsVector(This, IsVector) \
    (This)->lpVtbl->IsVector(This, IsVector)
#define IVinilType_IsMatrix(This, IsMatrix) \
    (This)->lpVtbl->IsMatrix(This, IsMatrix)
#define IVinilType_GetComponents(This, Components) \
    (This)->lpVtbl->GetComponents(This, Components)

#endif /* COBJMACROS */

//
// Factory Functions
//

/**
  Get a basic type (scalar, vector, or matrix).
  These are cached singleton instances.

  @param[in]   TypeValue  Type value (scalar, vector, matrix).
  @param[in]   Precision  Type precision.
  @param[out]  Type       Type interface.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_INVALIDARG  Invalid type value.
**/
HRESULT
VinilGetBasicType (
    VINIL_TYPE_VALUE  TypeValue,
    VINIL_PRECISION   Precision,
    IVinilType        **Type
    );

/**
  Create an array type.

  @param[in]   ElementType  Element type.
  @param[in]   NumElements  Number of elements.
  @param[out]  ArrayType    Created array type.

  @retval  S_OK           Success.
  @retval  E_POINTER      Invalid pointer.
  @retval  E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
VinilCreateArrayType (
    IVinilType  *ElementType,
    UINTN       NumElements,
    IVinilType  **ArrayType
    );

/**
  Create a pointer type (compute).

  @param[in]   PointeeType  Type being pointed to.
  @param[in]   AddrSpace    Address space.
  @param[out]  PointerType  Created pointer type.

  @retval  S_OK           Success.
  @retval  E_POINTER      Invalid pointer.
  @retval  E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
VinilCreatePointerType (
    IVinilType              *PointeeType,
    VINIL_ADDRESS_SPACE     AddrSpace,
    IVinilType              **PointerType
    );

