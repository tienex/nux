/*++
    Module Name:

        d3d9main.c

    Abstract:

        Direct3D 9 main entry point and device implementation.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/d3d9.h>
#include <ananke/gles20com.h>
#include <ananke/ntrtl.h>
#include <GLES/gl.h>

/* External functions from other modules */
extern HRESULT D3D9CreateVertexShader(IGLDevice*, CONST UINT32*, IDirect3DVertexShader9**);
extern HRESULT D3D9CreatePixelShader(IGLDevice*, CONST UINT32*, IDirect3DPixelShader9**);
extern HRESULT D3D9CreateVertexDeclaration(CONST D3DVERTEXELEMENT9*, IDirect3DVertexDeclaration9**);

/* Include internal header for new functions */
#include "d3d9_internal.h"

/* --------------------------------------------------------------- */
/*  Internal Structures                                            */
/* --------------------------------------------------------------- */

typedef struct _D3D9_VERTEX_BUFFER D3D9_VERTEX_BUFFER;
typedef struct _D3D9_INDEX_BUFFER D3D9_INDEX_BUFFER;
typedef struct _D3D9_TEXTURE D3D9_TEXTURE;

typedef struct _D3D9_DEVICE {
    IDirect3DDevice9Vtbl *lpVtbl;
    UINT32                RefCount;
    IGLDevice            *GlDevice;
    IGLContext           *GlContext;

    /* Current state */
    IDirect3DVertexBuffer9       *CurrentVertexBuffer;
    IDirect3DIndexBuffer9        *CurrentIndexBuffer;
    IDirect3DVertexShader9       *CurrentVertexShader;
    IDirect3DPixelShader9        *CurrentPixelShader;
    IDirect3DVertexDeclaration9  *CurrentVertexDeclaration;
    IDirect3DTexture9            *CurrentTextures[8];

    UINT32                CurrentStride;
    UINT32                CurrentOffset;

    /* Shader constants */
    D3D9_SHADER_CONSTANTS *ShaderConstants;

    /* State tracking */
    D3DBLEND              CurrentSrcBlend;
    D3DBLEND              CurrentDestBlend;
    UINT32                RenderStates[256];  /* Cache of render states */

    /* Presentation parameters */
    D3DPRESENT_PARAMETERS PresentParams;
} D3D9_DEVICE;

typedef struct _D3D9_MAIN {
    IDirect3D9Vtbl *lpVtbl;
    UINT32          RefCount;
} D3D9_MAIN;

/* External vtables */
extern IDirect3DVertexBuffer9Vtbl D3D9VertexBufferVtbl;
extern IDirect3DIndexBuffer9Vtbl D3D9IndexBufferVtbl;
extern IDirect3DTexture9Vtbl D3D9TextureVtbl;

/* --------------------------------------------------------------- */
/*  Helper: Convert D3D primitive type to GL                      */
/* --------------------------------------------------------------- */

static GLenum
D3DPrimitiveTypeToGL(D3DPRIMITIVETYPE PrimitiveType)
{
    switch (PrimitiveType) {
        case D3DPT_POINTLIST:     return GL_POINTS;
        case D3DPT_LINELIST:      return GL_LINES;
        case D3DPT_LINESTRIP:     return GL_LINE_STRIP;
        case D3DPT_TRIANGLELIST:  return GL_TRIANGLES;
        case D3DPT_TRIANGLESTRIP: return GL_TRIANGLE_STRIP;
        case D3DPT_TRIANGLEFAN:   return GL_TRIANGLE_FAN;
        default:                  return GL_TRIANGLES;
    }
}

/* --------------------------------------------------------------- */
/*  Helper: Calculate vertex count from primitive type            */
/* --------------------------------------------------------------- */

static UINT32
D3DPrimitiveCountToVertexCount(D3DPRIMITIVETYPE PrimitiveType, UINT32 PrimitiveCount)
{
    switch (PrimitiveType) {
        case D3DPT_POINTLIST:     return PrimitiveCount;
        case D3DPT_LINELIST:      return PrimitiveCount * 2;
        case D3DPT_LINESTRIP:     return PrimitiveCount + 1;
        case D3DPT_TRIANGLELIST:  return PrimitiveCount * 3;
        case D3DPT_TRIANGLESTRIP: return PrimitiveCount + 2;
        case D3DPT_TRIANGLEFAN:   return PrimitiveCount + 2;
        default:                  return 0;
    }
}

/* --------------------------------------------------------------- */
/*  Device Implementation                                          */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
D3D9Device_QueryInterface(
    IDirect3DDevice9 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3DDevice9))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D9Device_AddRef(IDirect3DDevice9 *This)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    return ++device->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D9Device_Release(IDirect3DDevice9 *This)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    UINT32 refCount = --device->RefCount;
    UINT32 i;

    if (refCount == 0) {
        if (device->ShaderConstants) {
            D3D9DestroyShaderConstants(device->ShaderConstants);
        }
        if (device->GlContext) {
            IUnknown_Release((IUnknown*)device->GlContext);
        }
        if (device->GlDevice) {
            IUnknown_Release((IUnknown*)device->GlDevice);
        }
        if (device->CurrentVertexBuffer) {
            IUnknown_Release((IUnknown*)device->CurrentVertexBuffer);
        }
        if (device->CurrentIndexBuffer) {
            IUnknown_Release((IUnknown*)device->CurrentIndexBuffer);
        }
        if (device->CurrentVertexShader) {
            IUnknown_Release((IUnknown*)device->CurrentVertexShader);
        }
        if (device->CurrentPixelShader) {
            IUnknown_Release((IUnknown*)device->CurrentPixelShader);
        }
        if (device->CurrentVertexDeclaration) {
            IUnknown_Release((IUnknown*)device->CurrentVertexDeclaration);
        }
        for (i = 0; i < 8; i++) {
            if (device->CurrentTextures[i]) {
                IUnknown_Release((IUnknown*)device->CurrentTextures[i]);
            }
        }
        RtlFreeMemory(device);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_BeginScene(IDirect3DDevice9 *This)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_EndScene(IDirect3DDevice9 *This)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_Clear(
    IDirect3DDevice9 *This,
    UINT32 Count,
    CONST D3DRECT *pRects,
    UINT32 Flags,
    D3DCOLOR Color,
    FLOAT Z,
    UINT32 Stencil)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    GLbitfield glMask = 0;
    FLOAT r, g, b, a;

    /* Extract color components */
    a = ((Color >> 24) & 0xFF) / 255.0f;
    r = ((Color >> 16) & 0xFF) / 255.0f;
    g = ((Color >> 8) & 0xFF) / 255.0f;
    b = ((Color) & 0xFF) / 255.0f;

    if (Flags & D3DCLEAR_TARGET) {
        glMask |= GL_COLOR_BUFFER_BIT;
        IGLContext_ClearColor(device->GlContext, r, g, b, a);
    }

    if (Flags & D3DCLEAR_ZBUFFER) {
        glMask |= GL_DEPTH_BUFFER_BIT;
        IGLContext_ClearDepth(device->GlContext, Z);
    }

    if (Flags & D3DCLEAR_STENCIL) {
        glMask |= GL_STENCIL_BUFFER_BIT;
    }

    IGLContext_Clear(device->GlContext, glMask);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_Present(
    IDirect3DDevice9 *This,
    CONST D3DRECT *pSourceRect,
    CONST D3DRECT *pDestRect,
    VOID *hDestWindowOverride,
    CONST VOID *pDirtyRegion)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    IGLContext_SwapBuffers(device->GlContext);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_CreateVertexBuffer(
    IDirect3DDevice9 *This,
    UINT32 Length,
    UINT32 Usage,
    UINT32 FVF,
    D3DPOOL Pool,
    IDirect3DVertexBuffer9 **ppVertexBuffer,
    VOID *pSharedHandle)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    D3D9_VERTEX_BUFFER *vb;
    HRESULT hr;

    if (!ppVertexBuffer) return E_POINTER;

    vb = (D3D9_VERTEX_BUFFER*)RtlAllocateMemory(sizeof(D3D9_VERTEX_BUFFER));
    if (!vb) return E_OUTOFMEMORY;

    RtlZeroMemory(vb, sizeof(D3D9_VERTEX_BUFFER));
    vb->lpVtbl = &D3D9VertexBufferVtbl;
    vb->RefCount = 1;
    vb->Length = Length;

    /* Allocate local copy */
    vb->LocalCopy = RtlAllocateMemory(Length);
    if (!vb->LocalCopy) {
        RtlFreeMemory(vb);
        return E_OUTOFMEMORY;
    }

    /* Create GL buffer */
    hr = IGLDevice_CreateBuffer(device->GlDevice, &vb->GlBuffer);
    if (FAILED(hr)) {
        RtlFreeMemory(vb->LocalCopy);
        RtlFreeMemory(vb);
        return hr;
    }

    *ppVertexBuffer = (IDirect3DVertexBuffer9*)vb;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_CreateIndexBuffer(
    IDirect3DDevice9 *This,
    UINT32 Length,
    UINT32 Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    IDirect3DIndexBuffer9 **ppIndexBuffer,
    VOID *pSharedHandle)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    D3D9_INDEX_BUFFER *ib;
    HRESULT hr;

    if (!ppIndexBuffer) return E_POINTER;

    ib = (D3D9_INDEX_BUFFER*)RtlAllocateMemory(sizeof(D3D9_INDEX_BUFFER));
    if (!ib) return E_OUTOFMEMORY;

    RtlZeroMemory(ib, sizeof(D3D9_INDEX_BUFFER));
    ib->lpVtbl = &D3D9IndexBufferVtbl;
    ib->RefCount = 1;
    ib->Length = Length;

    /* Allocate local copy */
    ib->LocalCopy = RtlAllocateMemory(Length);
    if (!ib->LocalCopy) {
        RtlFreeMemory(ib);
        return E_OUTOFMEMORY;
    }

    /* Create GL buffer */
    hr = IGLDevice_CreateBuffer(device->GlDevice, &ib->GlBuffer);
    if (FAILED(hr)) {
        RtlFreeMemory(ib->LocalCopy);
        RtlFreeMemory(ib);
        return hr;
    }

    *ppIndexBuffer = (IDirect3DIndexBuffer9*)ib;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_CreateTexture(
    IDirect3DDevice9 *This,
    UINT32 Width,
    UINT32 Height,
    UINT32 Levels,
    UINT32 Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    IDirect3DTexture9 **ppTexture,
    VOID *pSharedHandle)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    D3D9_TEXTURE *texture;
    HRESULT hr;
    UINT32 size;

    if (!ppTexture) return E_POINTER;
    if (Levels != 1) return E_NOTIMPL; /* Only single level for now */

    texture = (D3D9_TEXTURE*)RtlAllocateMemory(sizeof(D3D9_TEXTURE));
    if (!texture) return E_OUTOFMEMORY;

    RtlZeroMemory(texture, sizeof(D3D9_TEXTURE));
    texture->lpVtbl = &D3D9TextureVtbl;
    texture->RefCount = 1;
    texture->Width = Width;
    texture->Height = Height;
    texture->Format = Format;

    /* Allocate local copy based on format */
    UINT32 bpp = 4;
    switch (Format) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            bpp = 4;
            break;
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
            bpp = 2;
            break;
        case D3DFMT_R8G8B8:
            bpp = 3;
            break;
    }

    size = Width * Height * bpp;
    texture->LocalCopy = RtlAllocateMemory(size);
    if (!texture->LocalCopy) {
        RtlFreeMemory(texture);
        return E_OUTOFMEMORY;
    }

    /* Create GL texture */
    hr = IGLDevice_CreateTexture(device->GlDevice, &texture->GlTexture);
    if (FAILED(hr)) {
        RtlFreeMemory(texture->LocalCopy);
        RtlFreeMemory(texture);
        return hr;
    }

    /* Set default texture parameters */
    IGLTexture_Bind(texture->GlTexture, GL_TEXTURE_2D);
    IGLTexture_TexParameteri(texture->GlTexture, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    IGLTexture_TexParameteri(texture->GlTexture, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    *ppTexture = (IDirect3DTexture9*)texture;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_CreateVertexShader(
    IDirect3DDevice9 *This,
    CONST UINT32 *pFunction,
    IDirect3DVertexShader9 **ppShader)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    return D3D9CreateVertexShader(device->GlDevice, pFunction, ppShader);
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_CreatePixelShader(
    IDirect3DDevice9 *This,
    CONST UINT32 *pFunction,
    IDirect3DPixelShader9 **ppShader)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    return D3D9CreatePixelShader(device->GlDevice, pFunction, ppShader);
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_CreateVertexDeclaration(
    IDirect3DDevice9 *This,
    CONST D3DVERTEXELEMENT9 *pVertexElements,
    IDirect3DVertexDeclaration9 **ppDecl)
{
    return D3D9CreateVertexDeclaration(pVertexElements, ppDecl);
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_DrawPrimitive(
    IDirect3DDevice9 *This,
    D3DPRIMITIVETYPE PrimitiveType,
    UINT32 StartVertex,
    UINT32 PrimitiveCount)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    GLenum glPrimType = D3DPrimitiveTypeToGL(PrimitiveType);
    UINT32 vertexCount = D3DPrimitiveCountToVertexCount(PrimitiveType, PrimitiveCount);

    /* Apply vertex declaration if set */
    if (device->CurrentVertexDeclaration && device->CurrentStride > 0) {
        D3D9ApplyVertexDeclaration(device->CurrentVertexDeclaration,
                                   device->CurrentStride,
                                   device->CurrentOffset);
    }

    IGLContext_DrawArrays(device->GlContext, glPrimType, StartVertex, vertexCount);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_DrawIndexedPrimitive(
    IDirect3DDevice9 *This,
    D3DPRIMITIVETYPE PrimitiveType,
    INT32 BaseVertexIndex,
    UINT32 MinVertexIndex,
    UINT32 NumVertices,
    UINT32 StartIndex,
    UINT32 PrimitiveCount)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    GLenum glPrimType = D3DPrimitiveTypeToGL(PrimitiveType);
    UINT32 indexCount = D3DPrimitiveCountToVertexCount(PrimitiveType, PrimitiveCount);

    IGLContext_DrawElements(device->GlContext, glPrimType, indexCount,
                           GL_UNSIGNED_SHORT, (VOID*)(UINTN)(StartIndex * 2));

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_SetRenderState(
    IDirect3DDevice9 *This,
    D3DRENDERSTATETYPE State,
    UINT32 Value)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    HRESULT hr;

    /* Cache the state value */
    if (State < 256) {
        device->RenderStates[State] = Value;
    }

    /* Handle blend states specially to apply them together */
    if (State == D3DRS_SRCBLEND) {
        device->CurrentSrcBlend = (D3DBLEND)Value;
        return D3D9ApplyBlendState(device->GlContext,
                                   device->CurrentSrcBlend,
                                   device->CurrentDestBlend);
    }

    if (State == D3DRS_DESTBLEND) {
        device->CurrentDestBlend = (D3DBLEND)Value;
        return D3D9ApplyBlendState(device->GlContext,
                                   device->CurrentSrcBlend,
                                   device->CurrentDestBlend);
    }

    /* Delegate to state manager for other states */
    hr = D3D9ApplyRenderState(device->GlContext, State, Value);

    return hr;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_GetRenderState(
    IDirect3DDevice9 *This,
    D3DRENDERSTATETYPE State,
    UINT32 *pValue)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;

    if (!pValue) return E_POINTER;
    if (State >= 256) return E_INVALIDARG;

    *pValue = device->RenderStates[State];
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_SetTextureStageState(
    IDirect3DDevice9 *This,
    UINT32 Stage,
    D3DTEXTURESTAGESTATETYPE Type,
    UINT32 Value)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;

    if (Stage >= 8) return E_INVALIDARG;

    /* Delegate to state manager */
    return D3D9ApplyTextureStageState(device->GlContext, Stage, Type, Value);
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_SetSamplerState(
    IDirect3DDevice9 *This,
    UINT32 Sampler,
    D3DSAMPLERSTATETYPE Type,
    UINT32 Value)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;

    if (Sampler >= 8) return E_INVALIDARG;

    /* Delegate to state manager */
    return D3D9ApplySamplerState(device->GlContext, Sampler, Type, Value);
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_SetTexture(
    IDirect3DDevice9 *This,
    UINT32 Stage,
    IDirect3DTexture9 *pTexture)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    D3D9_TEXTURE *texture = (D3D9_TEXTURE*)pTexture;

    if (Stage >= 8) return E_INVALIDARG;

    if (device->CurrentTextures[Stage]) {
        IUnknown_Release((IUnknown*)device->CurrentTextures[Stage]);
    }

    device->CurrentTextures[Stage] = pTexture;

    if (pTexture) {
        IUnknown_AddRef((IUnknown*)pTexture);
        glActiveTexture(GL_TEXTURE0 + Stage);
        IGLTexture_Bind(texture->GlTexture, GL_TEXTURE_2D);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_SetStreamSource(
    IDirect3DDevice9 *This,
    UINT32 StreamNumber,
    IDirect3DVertexBuffer9 *pStreamData,
    UINT32 OffsetInBytes,
    UINT32 Stride)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    D3D9_VERTEX_BUFFER *vb = (D3D9_VERTEX_BUFFER*)pStreamData;

    if (StreamNumber != 0) return E_NOTIMPL; /* Only stream 0 for now */

    if (device->CurrentVertexBuffer) {
        IUnknown_Release((IUnknown*)device->CurrentVertexBuffer);
    }

    device->CurrentVertexBuffer = pStreamData;
    device->CurrentStride = Stride;
    device->CurrentOffset = OffsetInBytes;

    if (pStreamData) {
        IUnknown_AddRef((IUnknown*)pStreamData);
        IGLBuffer_Bind(vb->GlBuffer, GL_ARRAY_BUFFER);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_SetIndices(
    IDirect3DDevice9 *This,
    IDirect3DIndexBuffer9 *pIndexData)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;
    D3D9_INDEX_BUFFER *ib = (D3D9_INDEX_BUFFER*)pIndexData;

    if (device->CurrentIndexBuffer) {
        IUnknown_Release((IUnknown*)device->CurrentIndexBuffer);
    }

    device->CurrentIndexBuffer = pIndexData;

    if (pIndexData) {
        IUnknown_AddRef((IUnknown*)pIndexData);
        IGLBuffer_Bind(ib->GlBuffer, GL_ELEMENT_ARRAY_BUFFER);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_SetVertexShader(
    IDirect3DDevice9 *This,
    IDirect3DVertexShader9 *pShader)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;

    if (device->CurrentVertexShader) {
        IUnknown_Release((IUnknown*)device->CurrentVertexShader);
    }

    device->CurrentVertexShader = pShader;

    if (pShader) {
        IUnknown_AddRef((IUnknown*)pShader);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_SetPixelShader(
    IDirect3DDevice9 *This,
    IDirect3DPixelShader9 *pShader)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;

    if (device->CurrentPixelShader) {
        IUnknown_Release((IUnknown*)device->CurrentPixelShader);
    }

    device->CurrentPixelShader = pShader;

    if (pShader) {
        IUnknown_AddRef((IUnknown*)pShader);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_SetVertexDeclaration(
    IDirect3DDevice9 *This,
    IDirect3DVertexDeclaration9 *pDecl)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;

    if (device->CurrentVertexDeclaration) {
        IUnknown_Release((IUnknown*)device->CurrentVertexDeclaration);
    }

    device->CurrentVertexDeclaration = pDecl;

    if (pDecl) {
        IUnknown_AddRef((IUnknown*)pDecl);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_SetViewport(
    IDirect3DDevice9 *This,
    CONST D3DVIEWPORT9 *pViewport)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;

    if (!pViewport) return E_POINTER;

    IGLContext_Viewport(device->GlContext, pViewport->X, pViewport->Y,
                       pViewport->Width, pViewport->Height);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_SetVertexShaderConstantF(
    IDirect3DDevice9 *This,
    UINT32 StartRegister,
    CONST FLOAT *pConstantData,
    UINT32 Vector4fCount)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;

    if (!device->ShaderConstants) {
        return E_FAIL;
    }

    return D3D9SetVertexShaderConstantF(device->ShaderConstants,
                                        StartRegister,
                                        pConstantData,
                                        Vector4fCount);
}

static HRESULT STDMETHODCALLTYPE
D3D9Device_SetPixelShaderConstantF(
    IDirect3DDevice9 *This,
    UINT32 StartRegister,
    CONST FLOAT *pConstantData,
    UINT32 Vector4fCount)
{
    D3D9_DEVICE *device = (D3D9_DEVICE*)This;

    if (!device->ShaderConstants) {
        return E_FAIL;
    }

    return D3D9SetPixelShaderConstantF(device->ShaderConstants,
                                       StartRegister,
                                       pConstantData,
                                       Vector4fCount);
}

static IDirect3DDevice9Vtbl D3D9DeviceVtbl = {
    .QueryInterface            = D3D9Device_QueryInterface,
    .AddRef                    = D3D9Device_AddRef,
    .Release                   = D3D9Device_Release,
    .BeginScene                = D3D9Device_BeginScene,
    .EndScene                  = D3D9Device_EndScene,
    .Clear                     = D3D9Device_Clear,
    .Present                   = D3D9Device_Present,
    .CreateVertexBuffer        = D3D9Device_CreateVertexBuffer,
    .CreateIndexBuffer         = D3D9Device_CreateIndexBuffer,
    .CreateTexture             = D3D9Device_CreateTexture,
    .CreateVertexShader        = D3D9Device_CreateVertexShader,
    .CreatePixelShader         = D3D9Device_CreatePixelShader,
    .CreateVertexDeclaration   = D3D9Device_CreateVertexDeclaration,
    .DrawPrimitive             = D3D9Device_DrawPrimitive,
    .DrawIndexedPrimitive      = D3D9Device_DrawIndexedPrimitive,
    .SetRenderState            = D3D9Device_SetRenderState,
    .SetTexture                = D3D9Device_SetTexture,
    .SetStreamSource           = D3D9Device_SetStreamSource,
    .SetIndices                = D3D9Device_SetIndices,
    .SetVertexShader           = D3D9Device_SetVertexShader,
    .SetPixelShader            = D3D9Device_SetPixelShader,
    .SetVertexDeclaration      = D3D9Device_SetVertexDeclaration,
    .SetViewport               = D3D9Device_SetViewport,
    .SetVertexShaderConstantF  = D3D9Device_SetVertexShaderConstantF,
    .SetPixelShaderConstantF   = D3D9Device_SetPixelShaderConstantF,
    .GetRenderState            = D3D9Device_GetRenderState,
    .SetTextureStageState      = D3D9Device_SetTextureStageState,
    .SetSamplerState           = D3D9Device_SetSamplerState,
};

/* --------------------------------------------------------------- */
/*  Main D3D9 Interface                                            */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
D3D9Main_QueryInterface(
    IDirect3D9 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3D9))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D9Main_AddRef(IDirect3D9 *This)
{
    D3D9_MAIN *main = (D3D9_MAIN*)This;
    return ++main->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D9Main_Release(IDirect3D9 *This)
{
    D3D9_MAIN *main = (D3D9_MAIN*)This;
    UINT32 refCount = --main->RefCount;

    if (refCount == 0) {
        RtlFreeMemory(main);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D9Main_CreateDevice(
    IDirect3D9 *This,
    UINT32 Adapter,
    D3DDEVTYPE DeviceType,
    VOID *hFocusWindow,
    UINT32 BehaviorFlags,
    D3DPRESENT_PARAMETERS *pPresentationParameters,
    IDirect3DDevice9 **ppReturnedDeviceInterface)
{
    D3D9_DEVICE *device;
    HRESULT hr;

    if (!ppReturnedDeviceInterface) return E_POINTER;

    device = (D3D9_DEVICE*)RtlAllocateMemory(sizeof(D3D9_DEVICE));
    if (!device) return E_OUTOFMEMORY;

    RtlZeroMemory(device, sizeof(D3D9_DEVICE));
    device->lpVtbl = &D3D9DeviceVtbl;
    device->RefCount = 1;

    /* Copy presentation parameters */
    if (pPresentationParameters) {
        RtlCopyMemory(&device->PresentParams, pPresentationParameters, sizeof(D3DPRESENT_PARAMETERS));
    }

    /* Create GL device */
    hr = AnxCreateGLDevice(&device->GlDevice);
    if (FAILED(hr)) {
        RtlFreeMemory(device);
        return hr;
    }

    /* Get GL context */
    hr = IGLDevice_GetContext(device->GlDevice, &device->GlContext);
    if (FAILED(hr)) {
        IUnknown_Release((IUnknown*)device->GlDevice);
        RtlFreeMemory(device);
        return hr;
    }

    /* Create shader constants storage */
    hr = D3D9CreateShaderConstants(&device->ShaderConstants);
    if (FAILED(hr)) {
        IUnknown_Release((IUnknown*)device->GlContext);
        IUnknown_Release((IUnknown*)device->GlDevice);
        RtlFreeMemory(device);
        return hr;
    }

    /* Initialize default blend states */
    device->CurrentSrcBlend = D3DBLEND_ONE;
    device->CurrentDestBlend = D3DBLEND_ZERO;

    /* Initialize render states to defaults */
    RtlZeroMemory(device->RenderStates, sizeof(device->RenderStates));
    device->RenderStates[D3DRS_ZENABLE] = 1;             /* Depth test enabled */
    device->RenderStates[D3DRS_ZWRITEENABLE] = 1;        /* Depth write enabled */
    device->RenderStates[D3DRS_CULLMODE] = D3DCULL_CCW;  /* CCW culling */
    device->RenderStates[D3DRS_SRCBLEND] = D3DBLEND_ONE;
    device->RenderStates[D3DRS_DESTBLEND] = D3DBLEND_ZERO;
    device->RenderStates[D3DRS_ALPHABLENDENABLE] = 0;    /* Blending disabled */

    /* Setup viewport */
    if (pPresentationParameters) {
        IGLContext_Viewport(device->GlContext, 0, 0,
                          pPresentationParameters->BackBufferWidth,
                          pPresentationParameters->BackBufferHeight);
    }

    *ppReturnedDeviceInterface = (IDirect3DDevice9*)device;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D9Main_GetAdapterCount(
    IDirect3D9 *This,
    UINT32 *Count)
{
    if (!Count) return E_POINTER;
    *Count = 1; /* Always return 1 adapter */
    return S_OK;
}

static IDirect3D9Vtbl D3D9MainVtbl = {
    .QueryInterface   = D3D9Main_QueryInterface,
    .AddRef           = D3D9Main_AddRef,
    .Release          = D3D9Main_Release,
    .CreateDevice     = D3D9Main_CreateDevice,
    .GetAdapterCount  = D3D9Main_GetAdapterCount,
};

/* --------------------------------------------------------------- */
/*  Factory Function                                               */
/* --------------------------------------------------------------- */

IDirect3D9*
Direct3DCreate9(UINT32 SDKVersion)
{
    D3D9_MAIN *main;

    main = (D3D9_MAIN*)RtlAllocateMemory(sizeof(D3D9_MAIN));
    if (!main) return NULL;

    RtlZeroMemory(main, sizeof(D3D9_MAIN));
    main->lpVtbl = &D3D9MainVtbl;
    main->RefCount = 1;

    return (IDirect3D9*)main;
}
