/** @file
  VINIL Backend Operation Interfaces

  COM interfaces for backend-specific operations. Currently only texture
  sampling requires backend implementation (OpenGL, Vulkan, etc).

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

//
// IVinilTextureSampler Interface
//
// Backend implements texture sampling operations (OpenGL, Vulkan, etc.)
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

#endif /* COBJMACROS */
