/** @file
  VINIL Backend Operation Interfaces

  COM interfaces for backend-specific operations (textures, memory, atomics).
  These interfaces must be implemented by the backend (OpenGL, Vulkan, etc.)
  and provided to the VINIL execution context.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#pragma once
#include <vinil/base.h>
#include <ananke/com.h>

//
// GUIDs
//

ANX_DEFINE_GUID (IID_IVinilTextureSampler, 0x89ab1234, 0x5678, 0x9abc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc);
ANX_DEFINE_GUID (IID_IVinilMemoryOperations, 0x9bcd2345, 0x6789, 0xabcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd);
ANX_DEFINE_GUID (IID_IVinilAtomicOperations, 0xacde3456, 0x789a, 0xbcde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde);

//
// IVinilTextureSampler Interface
//

ANX_BEGIN_INTERFACE (IVinilTextureSampler, IUnknown, IID_IVinilTextureSampler, "89ab1234-5678-9abc-def0-123456789abc")
  /**
    Sample texture with coordinates.

    @param[in]  TextureUnit  Texture unit index.
    @param[in]  Coords       Texture coordinates (vec4).
    @param[out] Color        Sampled color (vec4).

    @retval S_OK            Success.
    @retval E_INVALIDARG    Invalid texture unit.
  **/
  ANX_IFACE_METHOD (HRESULT, Sample, (UINT32 TextureUnit, CONST float *Coords, float *Color))

  /**
    Sample texture with coordinates and LOD.

    @param[in]  TextureUnit  Texture unit index.
    @param[in]  Coords       Texture coordinates (vec4).
    @param[in]  Lod          Level of detail.
    @param[out] Color        Sampled color (vec4).

    @retval S_OK            Success.
    @retval E_INVALIDARG    Invalid texture unit.
  **/
  ANX_IFACE_METHOD (HRESULT, SampleLod, (UINT32 TextureUnit, CONST float *Coords, float Lod, float *Color))

  /**
    Sample texture with coordinates and bias.

    @param[in]  TextureUnit  Texture unit index.
    @param[in]  Coords       Texture coordinates (vec4).
    @param[in]  Bias         LOD bias.
    @param[out] Color        Sampled color (vec4).

    @retval S_OK            Success.
    @retval E_INVALIDARG    Invalid texture unit.
  **/
  ANX_IFACE_METHOD (HRESULT, SampleBias, (UINT32 TextureUnit, CONST float *Coords, float Bias, float *Color))

  /**
    Sample texture with projective coordinates.

    @param[in]  TextureUnit  Texture unit index.
    @param[in]  Coords       Texture coordinates with projector (vec4).
    @param[out] Color        Sampled color (vec4).

    @retval S_OK            Success.
    @retval E_INVALIDARG    Invalid texture unit.
  **/
  ANX_IFACE_METHOD (HRESULT, SampleProj, (UINT32 TextureUnit, CONST float *Coords, float *Color))

  /**
    Sample texture with explicit gradients.

    @param[in]  TextureUnit  Texture unit index.
    @param[in]  Coords       Texture coordinates (vec4).
    @param[in]  DDx          Gradient in X direction.
    @param[in]  DDy          Gradient in Y direction.
    @param[out] Color        Sampled color (vec4).

    @retval S_OK            Success.
    @retval E_INVALIDARG    Invalid texture unit.
  **/
  ANX_IFACE_METHOD (HRESULT, SampleGrad, (UINT32 TextureUnit, CONST float *Coords, CONST float *DDx, CONST float *DDy, float *Color))

  /**
    Fetch texel at integer coordinates.

    @param[in]  TextureUnit  Texture unit index.
    @param[in]  Coords       Integer texel coordinates (ivec4).
    @param[in]  Lod          LOD level.
    @param[out] Color        Fetched color (vec4).

    @retval S_OK            Success.
    @retval E_INVALIDARG    Invalid texture unit.
  **/
  ANX_IFACE_METHOD (HRESULT, Fetch, (UINT32 TextureUnit, CONST INT32 *Coords, INT32 Lod, float *Color))
ANX_END_INTERFACE (IVinilTextureSampler, IID_IVinilTextureSampler)

//
// IVinilMemoryOperations Interface
//

ANX_BEGIN_INTERFACE (IVinilMemoryOperations, IUnknown, IID_IVinilMemoryOperations, "9bcd2345-6789-abcd-ef01-23456789abcd")
  /**
    Load scalar from memory.

    @param[in]  Address  Memory address.
    @param[out] Value    Loaded value.

    @retval S_OK         Success.
    @retval E_POINTER    Invalid address.
  **/
  ANX_IFACE_METHOD (HRESULT, Load, (VOID *Address, VOID *Value))

  /**
    Store scalar to memory.

    @param[in]  Address  Memory address.
    @param[in]  Value    Value to store.

    @retval S_OK         Success.
    @retval E_POINTER    Invalid address.
  **/
  ANX_IFACE_METHOD (HRESULT, Store, (VOID *Address, CONST VOID *Value))

  /**
    Load vector from memory.

    @param[in]  Address  Memory address.
    @param[in]  Count    Number of components (1-4).
    @param[out] Value    Loaded vector.

    @retval S_OK         Success.
    @retval E_POINTER    Invalid address.
  **/
  ANX_IFACE_METHOD (HRESULT, LoadVector, (VOID *Address, UINT32 Count, float *Value))

  /**
    Store vector to memory.

    @param[in]  Address  Memory address.
    @param[in]  Count    Number of components (1-4).
    @param[in]  Value    Vector to store.

    @retval S_OK         Success.
    @retval E_POINTER    Invalid address.
  **/
  ANX_IFACE_METHOD (HRESULT, StoreVector, (VOID *Address, UINT32 Count, CONST float *Value))

  /**
    Memory barrier (all memory types).

    @retval S_OK  Success.
  **/
  HRESULT (STDMETHODCALLTYPE *Barrier)(void* This);

  /**
    Memory fence (global memory).

    @retval S_OK  Success.
  **/
  HRESULT (STDMETHODCALLTYPE *MemFence)(void* This);

  /**
    Read fence.

    @retval S_OK  Success.
  **/
  HRESULT (STDMETHODCALLTYPE *ReadFence)(void* This);

  /**
    Write fence.

    @retval S_OK  Success.
  **/
  HRESULT (STDMETHODCALLTYPE *WriteFence)(void* This);
ANX_END_INTERFACE (IVinilMemoryOperations, IID_IVinilMemoryOperations)

//
// IVinilAtomicOperations Interface
//

ANX_BEGIN_INTERFACE (IVinilAtomicOperations, IUnknown, IID_IVinilAtomicOperations, "acde3456-789a-bcde-f012-3456789abcde")
  ANX_IFACE_METHOD (HRESULT, Add, (VOID *Address, INT32 Value, INT32 *OldValue))
  ANX_IFACE_METHOD (HRESULT, Sub, (VOID *Address, INT32 Value, INT32 *OldValue))
  ANX_IFACE_METHOD (HRESULT, Min, (VOID *Address, INT32 Value, INT32 *OldValue))
  ANX_IFACE_METHOD (HRESULT, Max, (VOID *Address, INT32 Value, INT32 *OldValue))
  ANX_IFACE_METHOD (HRESULT, And, (VOID *Address, UINT32 Value, UINT32 *OldValue))
  ANX_IFACE_METHOD (HRESULT, Or, (VOID *Address, UINT32 Value, UINT32 *OldValue))
  ANX_IFACE_METHOD (HRESULT, Xor, (VOID *Address, UINT32 Value, UINT32 *OldValue))
  ANX_IFACE_METHOD (HRESULT, Exchange, (VOID *Address, UINT32 Value, UINT32 *OldValue))
  ANX_IFACE_METHOD (HRESULT, CompareExchange, (VOID *Address, UINT32 Compare, UINT32 Value, UINT32 *OldValue))
ANX_END_INTERFACE (IVinilAtomicOperations, IID_IVinilAtomicOperations)

//
// COBJMACROS
//

#ifdef COBJMACROS

/* IVinilTextureSampler */
#define IVinilTextureSampler_QueryInterface(This, riid, ppv) \
  (This)->lpVtbl->QueryInterface (This, riid, ppv)
#define IVinilTextureSampler_AddRef(This) \
  (This)->lpVtbl->AddRef (This)
#define IVinilTextureSampler_Release(This) \
  (This)->lpVtbl->Release (This)
#define IVinilTextureSampler_Sample(This, Unit, Coords, Color) \
  (This)->lpVtbl->Sample (This, Unit, Coords, Color)
#define IVinilTextureSampler_SampleLod(This, Unit, Coords, Lod, Color) \
  (This)->lpVtbl->SampleLod (This, Unit, Coords, Lod, Color)
#define IVinilTextureSampler_SampleBias(This, Unit, Coords, Bias, Color) \
  (This)->lpVtbl->SampleBias (This, Unit, Coords, Bias, Color)
#define IVinilTextureSampler_SampleProj(This, Unit, Coords, Color) \
  (This)->lpVtbl->SampleProj (This, Unit, Coords, Color)
#define IVinilTextureSampler_SampleGrad(This, Unit, Coords, DDx, DDy, Color) \
  (This)->lpVtbl->SampleGrad (This, Unit, Coords, DDx, DDy, Color)
#define IVinilTextureSampler_Fetch(This, Unit, Coords, Lod, Color) \
  (This)->lpVtbl->Fetch (This, Unit, Coords, Lod, Color)

/* IVinilMemoryOperations */
#define IVinilMemoryOperations_QueryInterface(This, riid, ppv) \
  (This)->lpVtbl->QueryInterface (This, riid, ppv)
#define IVinilMemoryOperations_AddRef(This) \
  (This)->lpVtbl->AddRef (This)
#define IVinilMemoryOperations_Release(This) \
  (This)->lpVtbl->Release (This)
#define IVinilMemoryOperations_Load(This, Address, Value) \
  (This)->lpVtbl->Load (This, Address, Value)
#define IVinilMemoryOperations_Store(This, Address, Value) \
  (This)->lpVtbl->Store (This, Address, Value)
#define IVinilMemoryOperations_LoadVector(This, Address, Count, Value) \
  (This)->lpVtbl->LoadVector (This, Address, Count, Value)
#define IVinilMemoryOperations_StoreVector(This, Address, Count, Value) \
  (This)->lpVtbl->StoreVector (This, Address, Count, Value)
#define IVinilMemoryOperations_Barrier(This) \
  (This)->lpVtbl->Barrier (This)
#define IVinilMemoryOperations_MemFence(This) \
  (This)->lpVtbl->MemFence (This)
#define IVinilMemoryOperations_ReadFence(This) \
  (This)->lpVtbl->ReadFence (This)
#define IVinilMemoryOperations_WriteFence(This) \
  (This)->lpVtbl->WriteFence (This)

/* IVinilAtomicOperations */
#define IVinilAtomicOperations_QueryInterface(This, riid, ppv) \
  (This)->lpVtbl->QueryInterface (This, riid, ppv)
#define IVinilAtomicOperations_AddRef(This) \
  (This)->lpVtbl->AddRef (This)
#define IVinilAtomicOperations_Release(This) \
  (This)->lpVtbl->Release (This)
#define IVinilAtomicOperations_Add(This, Address, Value, OldValue) \
  (This)->lpVtbl->Add (This, Address, Value, OldValue)
#define IVinilAtomicOperations_Sub(This, Address, Value, OldValue) \
  (This)->lpVtbl->Sub (This, Address, Value, OldValue)
#define IVinilAtomicOperations_Min(This, Address, Value, OldValue) \
  (This)->lpVtbl->Min (This, Address, Value, OldValue)
#define IVinilAtomicOperations_Max(This, Address, Value, OldValue) \
  (This)->lpVtbl->Max (This, Address, Value, OldValue)
#define IVinilAtomicOperations_And(This, Address, Value, OldValue) \
  (This)->lpVtbl->And (This, Address, Value, OldValue)
#define IVinilAtomicOperations_Or(This, Address, Value, OldValue) \
  (This)->lpVtbl->Or (This, Address, Value, OldValue)
#define IVinilAtomicOperations_Xor(This, Address, Value, OldValue) \
  (This)->lpVtbl->Xor (This, Address, Value, OldValue)
#define IVinilAtomicOperations_Exchange(This, Address, Value, OldValue) \
  (This)->lpVtbl->Exchange (This, Address, Value, OldValue)
#define IVinilAtomicOperations_CompareExchange(This, Address, Compare, Value, OldValue) \
  (This)->lpVtbl->CompareExchange (This, Address, Compare, Value, OldValue)

#endif /* COBJMACROS */
