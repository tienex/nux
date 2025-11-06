/*++
    Module Name:

        d3d6main.c

    Abstract:

        Direct3D 6 implementation - Multitexture support (2-4 stages).
        Thin wrapper around d3d_common fixed-function pipeline.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/ntrtl.h>
#include <ananke/d3d6.h>
#include <ananke/d3d_common.h>
#include <ananke/gles20com.h>

/* --------------------------------------------------------------- */
/*  D3D6 Device Structure                                          */
/* --------------------------------------------------------------- */

typedef struct _D3D6_DEVICE {
    IDirect3DDevice3Vtbl *lpVtbl;
    UINT32                RefCount;
    IGLDevice            *GlDevice;
    IGLContext           *GlContext;
    D3D_FFP_STATE         FfpState;
    DWORD                 CurrentFVF;
    D3D_FVF_DESCRIPTOR    CurrentFVFDesc;
    BOOLEAN               InScene;
} D3D6_DEVICE;

/* --------------------------------------------------------------- */
/*  Device Implementation                                          */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
D3D6Device_QueryInterface(IDirect3DDevice3 *This, REFIID riid, void **ppvObject)
{
    if (!ppvObject) return E_POINTER;
    if (RtlIsEqualGuid(riid, &IID_IUnknown) || RtlIsEqualGuid(riid, &IID_IDirect3DDevice3)) {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D6Device_AddRef(IDirect3DDevice3 *This)
{
    D3D6_DEVICE *device = (D3D6_DEVICE*)This;
    return ++device->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D6Device_Release(IDirect3DDevice3 *This)
{
    D3D6_DEVICE *device = (D3D6_DEVICE*)This;
    UINT32 refCount = --device->RefCount;
    if (refCount == 0) {
        if (device->GlContext) IUnknown_Release((IUnknown*)device->GlContext);
        if (device->GlDevice) IUnknown_Release((IUnknown*)device->GlDevice);
        RtlFreeMemory(device);
    }
    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D6Device_BeginScene(IDirect3DDevice3 *This)
{
    D3D6_DEVICE *device = (D3D6_DEVICE*)This;
    device->InScene = TRUE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D6Device_EndScene(IDirect3DDevice3 *This)
{
    D3D6_DEVICE *device = (D3D6_DEVICE*)This;
    device->InScene = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D6Device_SetTransform(
    IDirect3DDevice3 *This,
    D3DTS6 dtstTransformStateType,
    D3DMATRIX6 *lpD3DMatrix)
{
    D3D6_DEVICE *device = (D3D6_DEVICE*)This;
    D3D_TRANSFORM_TYPE type;

    if (!lpD3DMatrix) return E_POINTER;

    switch (dtstTransformStateType) {
    case D3DTS6_WORLD:      type = D3D_TRANSFORM_WORLD; break;
    case D3DTS6_VIEW:       type = D3D_TRANSFORM_VIEW; break;
    case D3DTS6_PROJECTION: type = D3D_TRANSFORM_PROJECTION; break;
    default:                return E_INVALIDARG;
    }

    return D3DSetFFPTransform(&device->FfpState, type, (D3D_MATRIX*)lpD3DMatrix);
}

static HRESULT STDMETHODCALLTYPE
D3D6Device_SetViewport2(
    IDirect3DDevice3 *This,
    D3DVIEWPORT26 *lpViewport)
{
    D3D6_DEVICE *device = (D3D6_DEVICE*)This;
    if (!lpViewport) return E_POINTER;

    return IGLContext_Viewport(device->GlContext,
                               lpViewport->dwX,
                               lpViewport->dwY,
                               lpViewport->dwWidth,
                               lpViewport->dwHeight);
}

static HRESULT STDMETHODCALLTYPE
D3D6Device_SetRenderState(
    IDirect3DDevice3 *This,
    D3DRENDERSTATETYPE6 dwRenderStateType,
    DWORD dwRenderState)
{
    D3D6_DEVICE *device = (D3D6_DEVICE*)This;

    switch (dwRenderStateType) {
    case D3DRS6_LIGHTING:
        device->FfpState.lightingEnabled = (dwRenderState != 0);
        break;
    case D3DRS6_ZENABLE:
        device->FfpState.depthTestEnable = (dwRenderState != 0);
        break;
    case D3DRS6_ALPHABLENDENABLE:
        device->FfpState.alphaBlendEnable = (dwRenderState != 0);
        break;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D6Device_SetTextureStageState(
    IDirect3DDevice3 *This,
    DWORD dwStage,
    D3DTSS6 dwState,
    DWORD dwValue)
{
    D3D6_DEVICE *device = (D3D6_DEVICE*)This;

    /* D3D6 supports max 4 texture stages */
    if (dwStage >= 4) return E_INVALIDARG;

    switch (dwState) {
    case D3DTSS6_COLOROP:
        device->FfpState.textureStages[dwStage].colorOp = (D3D_TEXTURE_OP)dwValue;
        break;
    case D3DTSS6_COLORARG1:
        device->FfpState.textureStages[dwStage].colorArg1 = (D3D_TEXTURE_ARG)dwValue;
        break;
    case D3DTSS6_COLORARG2:
        device->FfpState.textureStages[dwStage].colorArg2 = (D3D_TEXTURE_ARG)dwValue;
        break;
    case D3DTSS6_ALPHAOP:
        device->FfpState.textureStages[dwStage].alphaOp = (D3D_TEXTURE_OP)dwValue;
        break;
    }

    device->FfpState.stateHash++;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D6Device_DrawPrimitive(
    IDirect3DDevice3 *This,
    D3DPRIMITIVETYPE6 dptPrimitiveType,
    DWORD dwVertexTypeDesc,
    VOID *lpvVertices,
    DWORD dwVertexCount,
    DWORD dwFlags)
{
    D3D6_DEVICE *device = (D3D6_DEVICE*)This;
    GLenum glPrimType;
    HRESULT hr;

    if (!lpvVertices) return E_POINTER;

    /* Parse FVF if changed */
    if (dwVertexTypeDesc != device->CurrentFVF) {
        hr = D3DParseFVF(dwVertexTypeDesc, &device->CurrentFVFDesc);
        if (FAILED(hr)) return hr;
        device->CurrentFVF = dwVertexTypeDesc;

        /* Regenerate shader */
        hr = D3DUpdateFFPShaderProgram(device->GlDevice,
                                       &device->FfpState,
                                       &device->CurrentFVFDesc);
        if (FAILED(hr)) return hr;
    }

    /* Apply state */
    D3DApplyFFPState(device->GlContext, &device->FfpState);

    /* Use shader */
    if (device->FfpState.currentProgram) {
        IGLProgram_UseProgram(device->FfpState.currentProgram);

        /* Update shader uniforms */
        hr = D3DUpdateFFPUniforms(device->FfpState.currentProgram, &device->FfpState);
        if (FAILED(hr)) return hr;
    }

    glPrimType = D3DPrimitiveTypeToGL(dptPrimitiveType);

    /* Bind vertex attributes */
    if (device->FfpState.currentProgram) {
        hr = D3DBindVertexAttributes(device->GlContext,
                                      device->FfpState.currentProgram,
                                      &device->CurrentFVFDesc,
                                      lpvVertices);
        if (FAILED(hr)) return hr;
    }

    return IGLContext_DrawArrays(device->GlContext, glPrimType, 0, dwVertexCount);
}

/* Device vtable */
static IDirect3DDevice3Vtbl D3D6DeviceVtbl = {
    .QueryInterface         = D3D6Device_QueryInterface,
    .AddRef                 = D3D6Device_AddRef,
    .Release                = D3D6Device_Release,
    .BeginScene             = D3D6Device_BeginScene,
    .EndScene               = D3D6Device_EndScene,
    .SetTransform           = D3D6Device_SetTransform,
    .SetViewport2           = D3D6Device_SetViewport2,
    .SetRenderState         = D3D6Device_SetRenderState,
    .SetTextureStageState   = D3D6Device_SetTextureStageState,
    .DrawPrimitive          = D3D6Device_DrawPrimitive,
};

/* --------------------------------------------------------------- */
/*  IDirect3D3 Main Interface                                      */
/* --------------------------------------------------------------- */

typedef struct _D3D6_MAIN {
    IDirect3D3Vtbl *lpVtbl;
    UINT32          RefCount;
    IGLDevice      *GlDevice;
} D3D6_MAIN;

static HRESULT STDMETHODCALLTYPE
D3D6_QueryInterface(IDirect3D3 *This, REFIID riid, void **ppvObject)
{
    if (!ppvObject) return E_POINTER;
    if (RtlIsEqualGuid(riid, &IID_IUnknown) || RtlIsEqualGuid(riid, &IID_IDirect3D3)) {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D6_AddRef(IDirect3D3 *This)
{
    D3D6_MAIN *d3d = (D3D6_MAIN*)This;
    return ++d3d->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D6_Release(IDirect3D3 *This)
{
    D3D6_MAIN *d3d = (D3D6_MAIN*)This;
    UINT32 refCount = --d3d->RefCount;
    if (refCount == 0) {
        if (d3d->GlDevice) IUnknown_Release((IUnknown*)d3d->GlDevice);
        RtlFreeMemory(d3d);
    }
    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D6_CreateDevice(
    IDirect3D3 *This,
    CONST GUID *rclsid,
    VOID *lpSurface,
    IDirect3DDevice3 **lplpDirect3DDevice3,
    VOID *lpUnkOuter)
{
    D3D6_MAIN *d3d = (D3D6_MAIN*)This;
    D3D6_DEVICE *device;
    HRESULT hr;

    if (!lplpDirect3DDevice3) return E_POINTER;

    device = (D3D6_DEVICE*)RtlAllocateMemory(sizeof(D3D6_DEVICE));
    if (!device) return E_OUTOFMEMORY;

    RtlZeroMemory(device, sizeof(D3D6_DEVICE));
    device->lpVtbl = &D3D6DeviceVtbl;
    device->RefCount = 1;

    device->GlDevice = d3d->GlDevice;
    IUnknown_AddRef((IUnknown*)device->GlDevice);

    hr = IGLDevice_GetContext(d3d->GlDevice, &device->GlContext);
    if (FAILED(hr)) {
        RtlFreeMemory(device);
        return hr;
    }

    D3DInitializeFFPState(&device->FfpState);

    *lplpDirect3DDevice3 = (IDirect3DDevice3*)device;
    return S_OK;
}

static IDirect3D3Vtbl D3D6Vtbl = {
    .QueryInterface = D3D6_QueryInterface,
    .AddRef         = D3D6_AddRef,
    .Release        = D3D6_Release,
    .CreateDevice   = D3D6_CreateDevice,
};

/* --------------------------------------------------------------- */
/*  Direct3DCreate6                                                */
/* --------------------------------------------------------------- */

HRESULT
Direct3DCreate6(
    IDirect3D3 **ppDirect3D3)
{
    D3D6_MAIN *d3d;
    HRESULT hr;

    if (!ppDirect3D3) return E_POINTER;

    d3d = (D3D6_MAIN*)RtlAllocateMemory(sizeof(D3D6_MAIN));
    if (!d3d) return E_OUTOFMEMORY;

    RtlZeroMemory(d3d, sizeof(D3D6_MAIN));
    d3d->lpVtbl = &D3D6Vtbl;
    d3d->RefCount = 1;

    hr = GLCreateDevice(&d3d->GlDevice);
    if (FAILED(hr)) {
        RtlFreeMemory(d3d);
        return hr;
    }

    *ppDirect3D3 = (IDirect3D3*)d3d;
    return S_OK;
}
