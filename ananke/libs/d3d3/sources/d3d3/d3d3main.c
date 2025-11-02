/*++
    Module Name:

        d3d3main.c

    Abstract:

        Direct3D 1-3 implementation - Immediate mode and execute buffers.
        Simplest fixed-function pipeline via d3d_common.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/ntrtl.h>
#include <ananke/d3d3.h>
#include <ananke/d3d_common.h>
#include <ananke/gles20com.h>

/* --------------------------------------------------------------- */
/*  D3D3 Device Structure                                          */
/* --------------------------------------------------------------- */

typedef struct _D3D3_DEVICE {
    IDirect3DDeviceVtbl *lpVtbl;
    UINT32               RefCount;
    IGLDevice           *GlDevice;
    IGLContext          *GlContext;
    D3D_FFP_STATE        FfpState;
    BOOLEAN              InScene;
} D3D3_DEVICE;

/* --------------------------------------------------------------- */
/*  Device Implementation                                          */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
D3D3Device_QueryInterface(IDirect3DDevice *This, REFIID riid, void **ppvObject)
{
    if (!ppvObject) return E_POINTER;
    if (RtlIsEqualGuid(riid, &IID_IUnknown) || RtlIsEqualGuid(riid, &IID_IDirect3DDevice)) {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D3Device_AddRef(IDirect3DDevice *This)
{
    D3D3_DEVICE *device = (D3D3_DEVICE*)This;
    return ++device->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D3Device_Release(IDirect3DDevice *This)
{
    D3D3_DEVICE *device = (D3D3_DEVICE*)This;
    UINT32 refCount = --device->RefCount;
    if (refCount == 0) {
        if (device->GlContext) IUnknown_Release((IUnknown*)device->GlContext);
        if (device->GlDevice) IUnknown_Release((IUnknown*)device->GlDevice);
        RtlFreeMemory(device);
    }
    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D3Device_BeginScene(IDirect3DDevice *This)
{
    D3D3_DEVICE *device = (D3D3_DEVICE*)This;
    device->InScene = TRUE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D3Device_EndScene(IDirect3DDevice *This)
{
    D3D3_DEVICE *device = (D3D3_DEVICE*)This;
    device->InScene = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D3Device_SetTransform(
    IDirect3DDevice *This,
    D3DTS3 dtstTransformStateType,
    D3DMATRIX3 *lpD3DMatrix)
{
    D3D3_DEVICE *device = (D3D3_DEVICE*)This;
    D3D_TRANSFORM_TYPE type;

    if (!lpD3DMatrix) return E_POINTER;

    switch (dtstTransformStateType) {
    case D3DTS3_WORLD:      type = D3D_TRANSFORM_WORLD; break;
    case D3DTS3_VIEW:       type = D3D_TRANSFORM_VIEW; break;
    case D3DTS3_PROJECTION: type = D3D_TRANSFORM_PROJECTION; break;
    default:                return E_INVALIDARG;
    }

    return D3DSetFFPTransform(&device->FfpState, type, (D3D_MATRIX*)lpD3DMatrix);
}

static HRESULT STDMETHODCALLTYPE
D3D3Device_CreateExecuteBuffer(
    IDirect3DDevice *This,
    D3DEXECUTEBUFFERDESC *lpDesc,
    IDirect3DExecuteBuffer **lplpExecuteBuffer,
    VOID *pUnkOuter)
{
    /* Execute buffers are legacy - minimal stub implementation */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
D3D3Device_Execute(
    IDirect3DDevice *This,
    IDirect3DExecuteBuffer *lpExecuteBuffer,
    VOID *lpViewport,
    DWORD dwFlags)
{
    /* Execute buffers are legacy */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
D3D3Device_DrawPrimitiveImmediate(
    IDirect3DDevice *This,
    D3DPT3 primitiveType,
    D3DVERTEX3 *vertices,
    DWORD vertexCount)
{
    D3D3_DEVICE *device = (D3D3_DEVICE*)This;
    D3D_FVF_DESCRIPTOR fvfDesc;
    GLenum glPrimType;
    HRESULT hr;

    if (!vertices) return E_POINTER;

    /* Setup simple FVF for immediate mode vertices */
    RtlZeroMemory(&fvfDesc, sizeof(fvfDesc));
    fvfDesc.hasPosition = TRUE;
    fvfDesc.hasNormal = TRUE;
    fvfDesc.texCoordCount = 1;
    fvfDesc.vertexSize = sizeof(D3DVERTEX3);
    fvfDesc.positionOffset = 0;
    fvfDesc.normalOffset = 12;
    fvfDesc.texCoordOffset[0] = 24;

    /* Generate/update shader */
    hr = D3DUpdateFFPShaderProgram(device->GlDevice,
                                   &device->FfpState,
                                   &fvfDesc);
    if (FAILED(hr)) return hr;

    /* Apply state */
    D3DApplyFFPState(device->GlContext, &device->FfpState);

    /* Use shader */
    if (device->FfpState.currentProgram) {
        IGLProgram_UseProgram(device->FfpState.currentProgram);
    }

    glPrimType = D3DPrimitiveTypeToGL(primitiveType);

    /* TODO: Setup vertex attributes from immediate mode data */
    return IGLContext_DrawArrays(device->GlContext, glPrimType, 0, vertexCount);
}

/* Device vtable */
static IDirect3DDeviceVtbl D3D3DeviceVtbl = {
    .QueryInterface          = D3D3Device_QueryInterface,
    .AddRef                  = D3D3Device_AddRef,
    .Release                 = D3D3Device_Release,
    .BeginScene              = D3D3Device_BeginScene,
    .EndScene                = D3D3Device_EndScene,
    .SetTransform            = D3D3Device_SetTransform,
    .CreateExecuteBuffer     = D3D3Device_CreateExecuteBuffer,
    .Execute                 = D3D3Device_Execute,
    .DrawPrimitiveImmediate  = D3D3Device_DrawPrimitiveImmediate,
};

/* --------------------------------------------------------------- */
/*  IDirect3D Main Interface                                       */
/* --------------------------------------------------------------- */

typedef struct _D3D3_MAIN {
    IDirect3DVtbl *lpVtbl;
    UINT32         RefCount;
    IGLDevice     *GlDevice;
} D3D3_MAIN;

static HRESULT STDMETHODCALLTYPE
D3D3_QueryInterface(IDirect3D *This, REFIID riid, void **ppvObject)
{
    if (!ppvObject) return E_POINTER;
    if (RtlIsEqualGuid(riid, &IID_IUnknown) || RtlIsEqualGuid(riid, &IID_IDirect3D)) {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D3_AddRef(IDirect3D *This)
{
    D3D3_MAIN *d3d = (D3D3_MAIN*)This;
    return ++d3d->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D3_Release(IDirect3D *This)
{
    D3D3_MAIN *d3d = (D3D3_MAIN*)This;
    UINT32 refCount = --d3d->RefCount;
    if (refCount == 0) {
        if (d3d->GlDevice) IUnknown_Release((IUnknown*)d3d->GlDevice);
        RtlFreeMemory(d3d);
    }
    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D3_CreateDevice(
    IDirect3D *This,
    CONST GUID *rclsid,
    VOID *lpSurface,
    IDirect3DDevice **lplpDirect3DDevice)
{
    D3D3_MAIN *d3d = (D3D3_MAIN*)This;
    D3D3_DEVICE *device;
    HRESULT hr;

    if (!lplpDirect3DDevice) return E_POINTER;

    device = (D3D3_DEVICE*)RtlAllocateMemory(sizeof(D3D3_DEVICE));
    if (!device) return E_OUTOFMEMORY;

    RtlZeroMemory(device, sizeof(D3D3_DEVICE));
    device->lpVtbl = &D3D3DeviceVtbl;
    device->RefCount = 1;

    device->GlDevice = d3d->GlDevice;
    IUnknown_AddRef((IUnknown*)device->GlDevice);

    hr = IGLDevice_GetContext(d3d->GlDevice, &device->GlContext);
    if (FAILED(hr)) {
        RtlFreeMemory(device);
        return hr;
    }

    D3DInitializeFFPState(&device->FfpState);

    /* D3D3 has simplest rendering: single texture, basic lighting */
    device->FfpState.textureStages[0].colorOp = D3D_TOP_SELECTARG1;
    device->FfpState.textureStages[0].colorArg1 = D3D_TA_TEXTURE;
    for (UINT32 i = 1; i < D3D_MAX_TEXTURE_STAGES; i++) {
        device->FfpState.textureStages[i].colorOp = D3D_TOP_DISABLE;
    }

    *lplpDirect3DDevice = (IDirect3DDevice*)device;
    return S_OK;
}

static IDirect3DVtbl D3D3Vtbl = {
    .QueryInterface = D3D3_QueryInterface,
    .AddRef         = D3D3_AddRef,
    .Release        = D3D3_Release,
    .CreateDevice   = D3D3_CreateDevice,
};

/* --------------------------------------------------------------- */
/*  Direct3DCreate3                                                */
/* --------------------------------------------------------------- */

HRESULT
Direct3DCreate3(
    IDirect3D **ppDirect3D)
{
    D3D3_MAIN *d3d;
    HRESULT hr;

    if (!ppDirect3D) return E_POINTER;

    d3d = (D3D3_MAIN*)RtlAllocateMemory(sizeof(D3D3_MAIN));
    if (!d3d) return E_OUTOFMEMORY;

    RtlZeroMemory(d3d, sizeof(D3D3_MAIN));
    d3d->lpVtbl = &D3D3Vtbl;
    d3d->RefCount = 1;

    hr = GLCreateDevice(&d3d->GlDevice);
    if (FAILED(hr)) {
        RtlFreeMemory(d3d);
        return hr;
    }

    *ppDirect3D = (IDirect3D*)d3d;
    return S_OK;
}
