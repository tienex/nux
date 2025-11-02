/*++
    Module Name:

        d3d9_internal.h

    Abstract:

        Internal structures and functions for D3D9 implementation.

    Environment:

        C99 compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/d3d9.h>
#include <ananke/gles20com.h>

/* --------------------------------------------------------------- */
/*  Vertex Buffer                                                  */
/* --------------------------------------------------------------- */

typedef struct _D3D9_VERTEX_BUFFER {
    IDirect3DVertexBuffer9Vtbl *lpVtbl;
    UINT32                      RefCount;
    IGLBuffer                  *GlBuffer;
    UINT32                      Length;
    VOID                       *LocalCopy;
} D3D9_VERTEX_BUFFER;

extern IDirect3DVertexBuffer9Vtbl D3D9VertexBufferVtbl;

/* --------------------------------------------------------------- */
/*  Index Buffer                                                   */
/* --------------------------------------------------------------- */

typedef struct _D3D9_INDEX_BUFFER {
    IDirect3DIndexBuffer9Vtbl *lpVtbl;
    UINT32                     RefCount;
    IGLBuffer                 *GlBuffer;
    UINT32                     Length;
    VOID                      *LocalCopy;
} D3D9_INDEX_BUFFER;

extern IDirect3DIndexBuffer9Vtbl D3D9IndexBufferVtbl;

/* --------------------------------------------------------------- */
/*  Texture                                                        */
/* --------------------------------------------------------------- */

typedef struct _D3D9_TEXTURE {
    IDirect3DTexture9Vtbl *lpVtbl;
    UINT32                 RefCount;
    IGLTexture            *GlTexture;
    UINT32                 Width;
    UINT32                 Height;
    D3DFORMAT              Format;
    VOID                  *LocalCopy;
} D3D9_TEXTURE;

extern IDirect3DTexture9Vtbl D3D9TextureVtbl;

/* --------------------------------------------------------------- */
/*  Shader Functions                                               */
/* --------------------------------------------------------------- */

HRESULT
D3D9CreateVertexShader(
    IGLDevice *GlDevice,
    CONST UINT32 *pFunction,
    IDirect3DVertexShader9 **ppShader
);

HRESULT
D3D9CreatePixelShader(
    IGLDevice *GlDevice,
    CONST UINT32 *pFunction,
    IDirect3DPixelShader9 **ppShader
);

HRESULT
D3D9CreateVertexDeclaration(
    CONST D3DVERTEXELEMENT9 *pVertexElements,
    IDirect3DVertexDeclaration9 **ppDecl
);

/* --------------------------------------------------------------- */
/*  State Management Functions                                     */
/* --------------------------------------------------------------- */

HRESULT
D3D9ApplyRenderState(
    IGLContext *GlContext,
    D3DRENDERSTATETYPE State,
    UINT32 Value
);

HRESULT
D3D9ApplyTextureStageState(
    IGLContext *GlContext,
    UINT32 Stage,
    D3DTEXTURESTAGESTATETYPE Type,
    UINT32 Value
);

HRESULT
D3D9ApplySamplerState(
    IGLContext *GlContext,
    UINT32 Sampler,
    D3DSAMPLERSTATETYPE Type,
    UINT32 Value
);

HRESULT
D3D9ApplyBlendState(
    IGLContext *GlContext,
    D3DBLEND SrcBlend,
    D3DBLEND DestBlend
);

/* --------------------------------------------------------------- */
/*  Vertex Binding Functions                                       */
/* --------------------------------------------------------------- */

HRESULT
D3D9ApplyVertexDeclaration(
    IDirect3DVertexDeclaration9 *pDecl,
    UINT32 Stride,
    UINTN BaseOffset
);

HRESULT
D3D9DisableVertexAttributes(VOID);

UINT32
D3D9GetDeclTypeSize(D3DDECLTYPE Type);

HRESULT
D3D9CalculateVertexStride(
    IDirect3DVertexDeclaration9 *pDecl,
    UINT32 *OutStride
);

HRESULT
D3D9BindVertexDeclToShader(
    IDirect3DVertexDeclaration9 *pDecl,
    IGLProgram *pProgram,
    UINT32 Stride,
    UINTN BaseOffset
);

/* --------------------------------------------------------------- */
/*  Shader Constants                                               */
/* --------------------------------------------------------------- */

typedef struct _D3D9_SHADER_CONSTANTS D3D9_SHADER_CONSTANTS;

HRESULT
D3D9CreateShaderConstants(
    D3D9_SHADER_CONSTANTS **ppConstants
);

VOID
D3D9DestroyShaderConstants(
    D3D9_SHADER_CONSTANTS *pConstants
);

HRESULT
D3D9SetVertexShaderConstantF(
    D3D9_SHADER_CONSTANTS *pConstants,
    UINT32 StartRegister,
    CONST FLOAT *pConstantData,
    UINT32 Vector4fCount
);

HRESULT
D3D9SetPixelShaderConstantF(
    D3D9_SHADER_CONSTANTS *pConstants,
    UINT32 StartRegister,
    CONST FLOAT *pConstantData,
    UINT32 Vector4fCount
);

HRESULT
D3D9ApplyVertexShaderConstants(
    D3D9_SHADER_CONSTANTS *pConstants,
    IGLProgram *pProgram
);

HRESULT
D3D9ApplyPixelShaderConstants(
    D3D9_SHADER_CONSTANTS *pConstants,
    IGLProgram *pProgram
);

HRESULT
D3D9InvalidateShaderConstants(
    D3D9_SHADER_CONSTANTS *pConstants
);
