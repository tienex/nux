/*++
    Module Name:

        d3d8main.c

    Abstract:

        Direct3D 8 implementation - Shader Model 1.x support.
        Hybrid of fixed-function (via d3d_common) and programmable shaders.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/ntrtl.h>
#include <ananke/d3d8.h>
#include <ananke/d3d_common.h>
#include <ananke/gles20com.h>

/* --------------------------------------------------------------- */
/*  D3D8 Device Structure                                          */
/* --------------------------------------------------------------- */

typedef struct _D3D8_DEVICE {
    IDirect3DDevice8Vtbl *lpVtbl;
    UINT32                RefCount;

    IGLDevice            *GlDevice;
    IGLContext           *GlContext;

    /* Fixed-function pipeline state (for FVF rendering) */
    D3D_FFP_STATE         FfpState;

    /* Current shader handles (0 = use FFP) */
    DWORD                 CurrentVertexShader;
    DWORD                 CurrentPixelShader;

    /* Current FVF */
    DWORD                 CurrentFVF;
    D3D_FVF_DESCRIPTOR    CurrentFVFDesc;

    /* Current streams */
    IDirect3DVertexBuffer8 *StreamSource;
    UINT32                StreamStride;

    BOOLEAN               InScene;
} D3D8_DEVICE;

/* --------------------------------------------------------------- */
/* Basic COM Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
D3D8Device_QueryInterface(
    IDirect3DDevice8 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3DDevice8))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D8Device_AddRef(IDirect3DDevice8 *This)
{
    D3D8_DEVICE *device = (D3D8_DEVICE*)This;
    return ++device->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D8Device_Release(IDirect3DDevice8 *This)
{
    D3D8_DEVICE *device = (D3D8_DEVICE*)This;
    UINT32 refCount = --device->RefCount;

    if (refCount == 0) {
        if (device->GlContext) IUnknown_Release((IUnknown*)device->GlContext);
        if (device->GlDevice) IUnknown_Release((IUnknown*)device->GlDevice);
        RtlFreeMemory(device);
    }

    return refCount;
}

/* --------------------------------------------------------------- */
/* Device Methods                                                   */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
D3D8Device_BeginScene(IDirect3DDevice8 *This)
{
    D3D8_DEVICE *device = (D3D8_DEVICE*)This;
    device->InScene = TRUE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_EndScene(IDirect3DDevice8 *This)
{
    D3D8_DEVICE *device = (D3D8_DEVICE*)This;
    device->InScene = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_Present(
    IDirect3DDevice8 *This,
    VOID *pSourceRect,
    VOID *pDestRect,
    VOID *hDestWindowOverride,
    VOID *pDirtyRegion)
{
    /* Swap buffers */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_Clear(
    IDirect3DDevice8 *This,
    DWORD Count,
    VOID *pRects,
    DWORD Flags,
    DWORD Color,
    FLOAT Z,
    DWORD Stencil)
{
    D3D8_DEVICE *device = (D3D8_DEVICE*)This;
    FLOAT r = ((Color >> 16) & 0xFF) / 255.0f;
    FLOAT g = ((Color >> 8) & 0xFF) / 255.0f;
    FLOAT b = (Color & 0xFF) / 255.0f;
    FLOAT a = ((Color >> 24) & 0xFF) / 255.0f;

    IGLContext_ClearColor(device->GlContext, r, g, b, a);
    IGLContext_ClearDepth(device->GlContext, Z);

    GLenum glClearFlags = 0;
    if (Flags & 0x1) glClearFlags |= 0x00000100;  /* COLOR */
    if (Flags & 0x2) glClearFlags |= 0x00000040;  /* DEPTH */

    return IGLContext_Clear(device->GlContext, glClearFlags);
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_CreateVertexBuffer(
    IDirect3DDevice8 *This,
    UINT32 Length,
    DWORD Usage,
    DWORD FVF,
    DWORD Pool,
    IDirect3DVertexBuffer8 **ppVertexBuffer)
{
    /* Simplified: create buffer via D3D9 resources (reuse code) */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_CreateIndexBuffer(
    IDirect3DDevice8 *This,
    UINT32 Length,
    DWORD Usage,
    DWORD Format,
    DWORD Pool,
    IDirect3DIndexBuffer8 **ppIndexBuffer)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_CreateVertexShader(
    IDirect3DDevice8 *This,
    CONST DWORD *pDeclaration,
    CONST DWORD *pFunction,
    DWORD *pHandle,
    DWORD Usage)
{
    /* TODO: Translate SM1.x vertex shader to GLSL ES */
    /* For now, assign a handle */
    static DWORD nextHandle = 1;
    *pHandle = nextHandle++;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_CreatePixelShader(
    IDirect3DDevice8 *This,
    CONST DWORD *pFunction,
    DWORD *pHandle)
{
    /* TODO: Translate SM1.x pixel shader to GLSL ES */
    static DWORD nextHandle = 0x10000;
    *pHandle = nextHandle++;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_SetVertexShader(
    IDirect3DDevice8 *This,
    DWORD Handle)
{
    D3D8_DEVICE *device = (D3D8_DEVICE*)This;

    /* If handle < 0x10000, it's an FVF */
    if (Handle < 0x10000) {
        device->CurrentVertexShader = 0;
        device->CurrentFVF = Handle;
        D3DParseFVF(Handle, &device->CurrentFVFDesc);
    } else {
        device->CurrentVertexShader = Handle;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_SetPixelShader(
    IDirect3DDevice8 *This,
    DWORD Handle)
{
    D3D8_DEVICE *device = (D3D8_DEVICE*)This;
    device->CurrentPixelShader = Handle;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_SetRenderState(
    IDirect3DDevice8 *This,
    DWORD State,
    DWORD Value)
{
    D3D8_DEVICE *device = (D3D8_DEVICE*)This;

    /* Delegate to FFP state */
    switch (State) {
    case 137:  /* D3DRS_LIGHTING */
        device->FfpState.lightingEnabled = (Value != 0);
        break;
    case 27:   /* D3DRS_ALPHABLENDENABLE */
        device->FfpState.alphaBlendEnable = (Value != 0);
        break;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_SetTexture(
    IDirect3DDevice8 *This,
    DWORD Stage,
    IDirect3DTexture8 *pTexture)
{
    /* TODO: Set texture */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_SetStreamSource(
    IDirect3DDevice8 *This,
    UINT32 StreamNumber,
    IDirect3DVertexBuffer8 *pStreamData,
    UINT32 Stride)
{
    D3D8_DEVICE *device = (D3D8_DEVICE*)This;

    if (StreamNumber == 0) {
        device->StreamSource = pStreamData;
        device->StreamStride = Stride;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_SetIndices(
    IDirect3DDevice8 *This,
    IDirect3DIndexBuffer8 *pIndexData,
    UINT32 BaseVertexIndex)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_DrawPrimitive(
    IDirect3DDevice8 *This,
    D3DPRIMITIVETYPE8 PrimitiveType,
    UINT32 StartVertex,
    UINT32 PrimitiveCount)
{
    D3D8_DEVICE *device = (D3D8_DEVICE*)This;
    GLenum glPrimType = D3DPrimitiveTypeToGL(PrimitiveType);

    /* If using FVF (no programmable shader), use FFP */
    if (device->CurrentVertexShader == 0) {
        /* Update FFP shader if needed */
        D3DUpdateFFPShaderProgram(device->GlDevice,
                                  &device->FfpState,
                                  &device->CurrentFVFDesc);

        if (device->FfpState.currentProgram) {
            IGLProgram_UseProgram(device->FfpState.currentProgram);
        }
    }

    /* Calculate vertex count */
    UINT32 vertexCount = PrimitiveCount;
    if (PrimitiveType == D3DPT8_TRIANGLELIST) vertexCount *= 3;
    else if (PrimitiveType == D3DPT8_TRIANGLESTRIP) vertexCount += 2;

    return IGLContext_DrawArrays(device->GlContext, glPrimType, StartVertex, vertexCount);
}

static HRESULT STDMETHODCALLTYPE
D3D8Device_DrawIndexedPrimitive(
    IDirect3DDevice8 *This,
    D3DPRIMITIVETYPE8 PrimitiveType,
    UINT32 MinVertexIndex,
    UINT32 NumVertices,
    UINT32 StartIndex,
    UINT32 PrimitiveCount)
{
    return E_NOTIMPL;
}

/* Device vtable */
static IDirect3DDevice8Vtbl D3D8DeviceVtbl = {
    .QueryInterface         = D3D8Device_QueryInterface,
    .AddRef                 = D3D8Device_AddRef,
    .Release                = D3D8Device_Release,
    .BeginScene             = D3D8Device_BeginScene,
    .EndScene               = D3D8Device_EndScene,
    .Present                = D3D8Device_Present,
    .Clear                  = D3D8Device_Clear,
    .CreateVertexBuffer     = D3D8Device_CreateVertexBuffer,
    .CreateIndexBuffer      = D3D8Device_CreateIndexBuffer,
    .CreateVertexShader     = D3D8Device_CreateVertexShader,
    .CreatePixelShader      = D3D8Device_CreatePixelShader,
    .SetVertexShader        = D3D8Device_SetVertexShader,
    .SetPixelShader         = D3D8Device_SetPixelShader,
    .SetRenderState         = D3D8Device_SetRenderState,
    .SetTexture             = D3D8Device_SetTexture,
    .SetStreamSource        = D3D8Device_SetStreamSource,
    .SetIndices             = D3D8Device_SetIndices,
    .DrawPrimitive          = D3D8Device_DrawPrimitive,
    .DrawIndexedPrimitive   = D3D8Device_DrawIndexedPrimitive,
};

/* --------------------------------------------------------------- */
/* IDirect3D8 Main Interface                                        */
/* --------------------------------------------------------------- */

typedef struct _D3D8_MAIN {
    IDirect3D8Vtbl *lpVtbl;
    UINT32          RefCount;
    IGLDevice      *GlDevice;
} D3D8_MAIN;

static HRESULT STDMETHODCALLTYPE
D3D8_QueryInterface(IDirect3D8 *This, REFIID riid, void **ppvObject)
{
    if (!ppvObject) return E_POINTER;
    if (RtlIsEqualGuid(riid, &IID_IUnknown) || RtlIsEqualGuid(riid, &IID_IDirect3D8)) {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D8_AddRef(IDirect3D8 *This)
{
    D3D8_MAIN *d3d = (D3D8_MAIN*)This;
    return ++d3d->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D8_Release(IDirect3D8 *This)
{
    D3D8_MAIN *d3d = (D3D8_MAIN*)This;
    UINT32 refCount = --d3d->RefCount;
    if (refCount == 0) {
        if (d3d->GlDevice) IUnknown_Release((IUnknown*)d3d->GlDevice);
        RtlFreeMemory(d3d);
    }
    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D8_CreateDevice(
    IDirect3D8 *This,
    UINT32 Adapter,
    UINT32 DeviceType,
    VOID *hFocusWindow,
    DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS8 *pPresentationParameters,
    IDirect3DDevice8 **ppReturnedDeviceInterface)
{
    D3D8_MAIN *d3d = (D3D8_MAIN*)This;
    D3D8_DEVICE *device;
    HRESULT hr;

    if (!ppReturnedDeviceInterface) return E_POINTER;

    device = (D3D8_DEVICE*)RtlAllocateMemory(sizeof(D3D8_DEVICE));
    if (!device) return E_OUTOFMEMORY;

    RtlZeroMemory(device, sizeof(D3D8_DEVICE));
    device->lpVtbl = &D3D8DeviceVtbl;
    device->RefCount = 1;

    device->GlDevice = d3d->GlDevice;
    IUnknown_AddRef((IUnknown*)device->GlDevice);

    hr = IGLDevice_GetContext(d3d->GlDevice, &device->GlContext);
    if (FAILED(hr)) {
        RtlFreeMemory(device);
        return hr;
    }

    D3DInitializeFFPState(&device->FfpState);

    *ppReturnedDeviceInterface = (IDirect3DDevice8*)device;
    return S_OK;
}

static IDirect3D8Vtbl D3D8Vtbl = {
    .QueryInterface = D3D8_QueryInterface,
    .AddRef         = D3D8_AddRef,
    .Release        = D3D8_Release,
    .CreateDevice   = D3D8_CreateDevice,
};

/* --------------------------------------------------------------- */
/* Direct3DCreate8                                                  */
/* --------------------------------------------------------------- */

IDirect3D8*
Direct3DCreate8(
    UINT32 SDKVersion)
{
    D3D8_MAIN *d3d;
    HRESULT hr;

    d3d = (D3D8_MAIN*)RtlAllocateMemory(sizeof(D3D8_MAIN));
    if (!d3d) return NULL;

    RtlZeroMemory(d3d, sizeof(D3D8_MAIN));
    d3d->lpVtbl = &D3D8Vtbl;
    d3d->RefCount = 1;

    hr = GLCreateDevice(&d3d->GlDevice);
    if (FAILED(hr)) {
        RtlFreeMemory(d3d);
        return NULL;
    }

    return (IDirect3D8*)d3d;
}
