/*++
    Module Name:

        d3d5main.c

    Abstract:

        Direct3D 5 implementation - DrawPrimitive API (single texture).
        Thin wrapper around d3d_common fixed-function pipeline.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/ntrtl.h>
#include <ananke/d3d5.h>
#include <ananke/d3d_common.h>
#include <ananke/gles20com.h>

/* --------------------------------------------------------------- */
/*  D3D5 Device Structure                                          */
/* --------------------------------------------------------------- */

typedef struct _D3D5_DEVICE {
    IDirect3DDevice2Vtbl *lpVtbl;
    UINT32                RefCount;
    IGLDevice            *GlDevice;
    IGLContext           *GlContext;
    D3D_FFP_STATE         FfpState;
    DWORD                 CurrentFVF;
    D3D_FVF_DESCRIPTOR    CurrentFVFDesc;
    BOOLEAN               InScene;
} D3D5_DEVICE;

/* --------------------------------------------------------------- */
/*  Device Implementation                                          */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
D3D5Device_QueryInterface(IDirect3DDevice2 *This, REFIID riid, void **ppvObject)
{
    if (!ppvObject) return E_POINTER;
    if (RtlIsEqualGuid(riid, &IID_IUnknown) || RtlIsEqualGuid(riid, &IID_IDirect3DDevice2)) {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D5Device_AddRef(IDirect3DDevice2 *This)
{
    D3D5_DEVICE *device = (D3D5_DEVICE*)This;
    return ++device->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D5Device_Release(IDirect3DDevice2 *This)
{
    D3D5_DEVICE *device = (D3D5_DEVICE*)This;
    UINT32 refCount = --device->RefCount;
    if (refCount == 0) {
        if (device->GlContext) IUnknown_Release((IUnknown*)device->GlContext);
        if (device->GlDevice) IUnknown_Release((IUnknown*)device->GlDevice);
        RtlFreeMemory(device);
    }
    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D5Device_BeginScene(IDirect3DDevice2 *This)
{
    D3D5_DEVICE *device = (D3D5_DEVICE*)This;
    device->InScene = TRUE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D5Device_EndScene(IDirect3DDevice2 *This)
{
    D3D5_DEVICE *device = (D3D5_DEVICE*)This;
    device->InScene = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D5Device_SetTransform(
    IDirect3DDevice2 *This,
    D3DTS5 dtstTransformStateType,
    D3DMATRIX5 *lpD3DMatrix)
{
    D3D5_DEVICE *device = (D3D5_DEVICE*)This;
    D3D_TRANSFORM_TYPE type;

    if (!lpD3DMatrix) return E_POINTER;

    switch (dtstTransformStateType) {
    case D3DTS5_WORLD:      type = D3D_TRANSFORM_WORLD; break;
    case D3DTS5_VIEW:       type = D3D_TRANSFORM_VIEW; break;
    case D3DTS5_PROJECTION: type = D3D_TRANSFORM_PROJECTION; break;
    default:                return E_INVALIDARG;
    }

    return D3DSetFFPTransform(&device->FfpState, type, (D3D_MATRIX*)lpD3DMatrix);
}

static HRESULT STDMETHODCALLTYPE
D3D5Device_SetRenderState(
    IDirect3DDevice2 *This,
    D3DRS5 dwRenderStateType,
    DWORD dwRenderState)
{
    D3D5_DEVICE *device = (D3D5_DEVICE*)This;

    switch (dwRenderStateType) {
    case D3DRS5_ZENABLE:
        device->FfpState.depthTestEnable = (dwRenderState != 0);
        break;
    case D3DRS5_ALPHABLENDENABLE:
        device->FfpState.alphaBlendEnable = (dwRenderState != 0);
        break;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D5Device_DrawPrimitive(
    IDirect3DDevice2 *This,
    D3DPRIMITIVETYPE5 dptPrimitiveType,
    DWORD dwVertexTypeDesc,
    VOID *lpvVertices,
    DWORD dwVertexCount,
    DWORD dwFlags)
{
    D3D5_DEVICE *device = (D3D5_DEVICE*)This;
    GLenum glPrimType;
    HRESULT hr;

    if (!lpvVertices) return E_POINTER;

    /* Parse FVF if changed */
    if (dwVertexTypeDesc != device->CurrentFVF) {
        hr = D3DParseFVF(dwVertexTypeDesc, &device->CurrentFVFDesc);
        if (FAILED(hr)) return hr;
        device->CurrentFVF = dwVertexTypeDesc;

        /* Limit to single texture for D3D5 */
        if (device->CurrentFVFDesc.texCoordCount > 1) {
            device->CurrentFVFDesc.texCoordCount = 1;
        }

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
        IGLContext_UseProgram(device->GlContext, device->FfpState.currentProgram);
    }

    glPrimType = D3DPrimitiveTypeToGL(dptPrimitiveType);

    return IGLContext_DrawArrays(device->GlContext, glPrimType, 0, dwVertexCount);
}

/* Device vtable */
static IDirect3DDevice2Vtbl D3D5DeviceVtbl = {
    .QueryInterface  = D3D5Device_QueryInterface,
    .AddRef          = D3D5Device_AddRef,
    .Release         = D3D5Device_Release,
    .BeginScene      = D3D5Device_BeginScene,
    .EndScene        = D3D5Device_EndScene,
    .SetTransform    = D3D5Device_SetTransform,
    .SetRenderState  = D3D5Device_SetRenderState,
    .DrawPrimitive   = D3D5Device_DrawPrimitive,
};

/* --------------------------------------------------------------- */
/*  IDirect3D2 Main Interface                                      */
/* --------------------------------------------------------------- */

typedef struct _D3D5_MAIN {
    IDirect3D2Vtbl *lpVtbl;
    UINT32          RefCount;
    IGLDevice      *GlDevice;
} D3D5_MAIN;

static HRESULT STDMETHODCALLTYPE
D3D5_QueryInterface(IDirect3D2 *This, REFIID riid, void **ppvObject)
{
    if (!ppvObject) return E_POINTER;
    if (RtlIsEqualGuid(riid, &IID_IUnknown) || RtlIsEqualGuid(riid, &IID_IDirect3D2)) {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D5_AddRef(IDirect3D2 *This)
{
    D3D5_MAIN *d3d = (D3D5_MAIN*)This;
    return ++d3d->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D5_Release(IDirect3D2 *This)
{
    D3D5_MAIN *d3d = (D3D5_MAIN*)This;
    UINT32 refCount = --d3d->RefCount;
    if (refCount == 0) {
        if (d3d->GlDevice) IUnknown_Release((IUnknown*)d3d->GlDevice);
        RtlFreeMemory(d3d);
    }
    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D5_CreateDevice(
    IDirect3D2 *This,
    CONST GUID *rclsid,
    VOID *lpSurface,
    IDirect3DDevice2 **lplpDirect3DDevice2)
{
    D3D5_MAIN *d3d = (D3D5_MAIN*)This;
    D3D5_DEVICE *device;
    HRESULT hr;

    if (!lplpDirect3DDevice2) return E_POINTER;

    device = (D3D5_DEVICE*)RtlAllocateMemory(sizeof(D3D5_DEVICE));
    if (!device) return E_OUTOFMEMORY;

    RtlZeroMemory(device, sizeof(D3D5_DEVICE));
    device->lpVtbl = &D3D5DeviceVtbl;
    device->RefCount = 1;

    device->GlDevice = d3d->GlDevice;
    IUnknown_AddRef((IUnknown*)device->GlDevice);

    hr = IGLDevice_CreateContext(d3d->GlDevice, &device->GlContext);
    if (FAILED(hr)) {
        RtlFreeMemory(device);
        return hr;
    }

    D3DInitializeFFPState(&device->FfpState);

    /* D3D5 only supports single texture stage */
    device->FfpState.textureStages[0].colorOp = D3D_TOP_MODULATE;
    for (UINT32 i = 1; i < D3D_MAX_TEXTURE_STAGES; i++) {
        device->FfpState.textureStages[i].colorOp = D3D_TOP_DISABLE;
    }

    *lplpDirect3DDevice2 = (IDirect3DDevice2*)device;
    return S_OK;
}

static IDirect3D2Vtbl D3D5Vtbl = {
    .QueryInterface = D3D5_QueryInterface,
    .AddRef         = D3D5_AddRef,
    .Release        = D3D5_Release,
    .CreateDevice   = D3D5_CreateDevice,
};

/* --------------------------------------------------------------- */
/*  Direct3DCreate5                                                */
/* --------------------------------------------------------------- */

HRESULT
Direct3DCreate5(
    IDirect3D2 **ppDirect3D2)
{
    D3D5_MAIN *d3d;
    HRESULT hr;

    if (!ppDirect3D2) return E_POINTER;

    d3d = (D3D5_MAIN*)RtlAllocateMemory(sizeof(D3D5_MAIN));
    if (!d3d) return E_OUTOFMEMORY;

    RtlZeroMemory(d3d, sizeof(D3D5_MAIN));
    d3d->lpVtbl = &D3D5Vtbl;
    d3d->RefCount = 1;

    hr = GLCreateDevice(&d3d->GlDevice);
    if (FAILED(hr)) {
        RtlFreeMemory(d3d);
        return hr;
    }

    *ppDirect3D2 = (IDirect3D2*)d3d;
    return S_OK;
}
