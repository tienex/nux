/*++
    Module Name:

        d3d7main.c

    Abstract:

        Direct3D 7 main implementation.
        Thin wrapper around common D3D fixed-function pipeline.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/ntrtl.h>
#include <ananke/d3d7.h>
#include <ananke/d3d_common.h>
#include <ananke/gles20com.h>

/* --------------------------------------------------------------- */
/*  D3D7 Device Structure                                          */
/* --------------------------------------------------------------- */

typedef struct _D3D7_DEVICE {
    IDirect3DDevice7Vtbl *lpVtbl;
    UINT32                RefCount;

    /* GL backend */
    IGLDevice            *GlDevice;
    IGLContext           *GlContext;

    /* Fixed-function pipeline state */
    D3D_FFP_STATE         FfpState;

    /* Current FVF */
    DWORD                 CurrentFVF;
    D3D_FVF_DESCRIPTOR    CurrentFVFDesc;

    /* Scene tracking */
    BOOLEAN               InScene;
} D3D7_DEVICE;

/* --------------------------------------------------------------- */
/*  Device Implementation                                          */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
D3D7Device_QueryInterface(
    IDirect3DDevice7 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3DDevice7))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D7Device_AddRef(IDirect3DDevice7 *This)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;
    return ++device->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D7Device_Release(IDirect3DDevice7 *This)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;
    UINT32 refCount = --device->RefCount;

    if (refCount == 0) {
        if (device->FfpState.currentProgram) {
            IUnknown_Release((IUnknown*)device->FfpState.currentProgram);
        }
        if (device->GlContext) {
            IUnknown_Release((IUnknown*)device->GlContext);
        }
        if (device->GlDevice) {
            IUnknown_Release((IUnknown*)device->GlDevice);
        }
        RtlFreeMemory(device);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_GetCaps(
    IDirect3DDevice7 *This,
    D3DDEVICEDESC7 *lpD3DDevDesc)
{
    if (!lpD3DDevDesc) return E_POINTER;

    /* Return basic capabilities */
    RtlZeroMemory(lpD3DDevDesc, sizeof(D3DDEVICEDESC7));
    lpD3DDevDesc->dwDevCaps = 0x00010000;  /* D3DDEVCAPS_FLOATTLVERTEX */
    lpD3DDevDesc->wMaxTextureBlendStages = 8;
    lpD3DDevDesc->wMaxSimultaneousTextures = 8;
    lpD3DDevDesc->dwMaxActiveLights = 8;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_BeginScene(IDirect3DDevice7 *This)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;
    device->InScene = TRUE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_EndScene(IDirect3DDevice7 *This)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;
    device->InScene = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_Clear(
    IDirect3DDevice7 *This,
    DWORD dwCount,
    VOID *lpRects,
    DWORD dwFlags,
    DWORD dwColor,
    FLOAT dvZ,
    DWORD dwStencil)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;
    GLenum glClearFlags = 0;

    /* Convert color from ARGB to RGBA components */
    FLOAT r = ((dwColor >> 16) & 0xFF) / 255.0f;
    FLOAT g = ((dwColor >> 8) & 0xFF) / 255.0f;
    FLOAT b = (dwColor & 0xFF) / 255.0f;
    FLOAT a = ((dwColor >> 24) & 0xFF) / 255.0f;

    IGLContext_ClearColor(device->GlContext, r, g, b, a);
    IGLContext_ClearDepth(device->GlContext, dvZ);

    if (dwFlags & 0x00000001) glClearFlags |= 0x00000100;  /* GL_COLOR_BUFFER_BIT */
    if (dwFlags & 0x00000002) glClearFlags |= 0x00000040;  /* GL_DEPTH_BUFFER_BIT */
    if (dwFlags & 0x00000004) glClearFlags |= 0x00000400;  /* GL_STENCIL_BUFFER_BIT */

    return IGLContext_Clear(device->GlContext, glClearFlags);
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_SetTransform(
    IDirect3DDevice7 *This,
    D3DTRANSFORMSTATETYPE7 dtstTransformStateType,
    CONST D3DMATRIX7 *lpD3DMatrix)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;
    D3D_TRANSFORM_TYPE type;
    D3D_MATRIX *matrix = (D3D_MATRIX*)lpD3DMatrix;

    if (!lpD3DMatrix) return E_POINTER;

    /* Map D3D7 transform type to common type */
    switch (dtstTransformStateType) {
    case D3DTS7_WORLD:      type = D3D_TRANSFORM_WORLD; break;
    case D3DTS7_VIEW:       type = D3D_TRANSFORM_VIEW; break;
    case D3DTS7_PROJECTION: type = D3D_TRANSFORM_PROJECTION; break;
    case D3DTS7_WORLD1:     type = D3D_TRANSFORM_WORLD1; break;
    case D3DTS7_WORLD2:     type = D3D_TRANSFORM_WORLD2; break;
    case D3DTS7_WORLD3:     type = D3D_TRANSFORM_WORLD3; break;
    default:                return E_INVALIDARG;
    }

    return D3DSetFFPTransform(&device->FfpState, type, matrix);
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_GetTransform(
    IDirect3DDevice7 *This,
    D3DTRANSFORMSTATETYPE7 dtstTransformStateType,
    D3DMATRIX7 *lpD3DMatrix)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;
    D3D_TRANSFORM_TYPE type;

    if (!lpD3DMatrix) return E_POINTER;

    /* Map transform type */
    switch (dtstTransformStateType) {
    case D3DTS7_WORLD:      type = D3D_TRANSFORM_WORLD; break;
    case D3DTS7_VIEW:       type = D3D_TRANSFORM_VIEW; break;
    case D3DTS7_PROJECTION: type = D3D_TRANSFORM_PROJECTION; break;
    default:                return E_INVALIDARG;
    }

    RtlCopyMemory(lpD3DMatrix, &device->FfpState.transforms[type], sizeof(D3DMATRIX7));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_SetViewport(
    IDirect3DDevice7 *This,
    D3DVIEWPORT7 *lpViewport)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;

    if (!lpViewport) return E_POINTER;

    return IGLContext_Viewport(device->GlContext,
                               lpViewport->dwX,
                               lpViewport->dwY,
                               lpViewport->dwWidth,
                               lpViewport->dwHeight);
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_SetMaterial(
    IDirect3DDevice7 *This,
    D3DMATERIAL7 *lpMaterial)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;
    D3D_MATERIAL *material = (D3D_MATERIAL*)lpMaterial;

    if (!lpMaterial) return E_POINTER;

    return D3DSetFFPMaterial(&device->FfpState, material);
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_SetLight(
    IDirect3DDevice7 *This,
    DWORD dwLightIndex,
    D3DLIGHT7 *lpLight)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;
    D3D_LIGHT *light = (D3D_LIGHT*)lpLight;

    if (!lpLight) return E_POINTER;

    return D3DSetFFPLight(&device->FfpState, dwLightIndex, light);
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_LightEnable(
    IDirect3DDevice7 *This,
    DWORD dwLightIndex,
    BOOLEAN bEnable)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;
    return D3DEnableFFPLight(&device->FfpState, dwLightIndex, bEnable);
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_SetRenderState(
    IDirect3DDevice7 *This,
    D3DRENDERSTATETYPE7 dwRenderStateType,
    DWORD dwRenderState)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;

    /* Handle common render states */
    switch (dwRenderStateType) {
    case D3DRS7_LIGHTING:
        device->FfpState.lightingEnabled = (dwRenderState != 0);
        break;

    case D3DRS7_ZENABLE:
        device->FfpState.depthTestEnable = (dwRenderState != 0);
        break;

    case D3DRS7_ALPHABLENDENABLE:
        device->FfpState.alphaBlendEnable = (dwRenderState != 0);
        break;

    /* Add other states as needed */
    default:
        break;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_GetRenderState(
    IDirect3DDevice7 *This,
    D3DRENDERSTATETYPE7 dwRenderStateType,
    DWORD *lpdwRenderState)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;

    if (!lpdwRenderState) return E_POINTER;

    switch (dwRenderStateType) {
    case D3DRS7_LIGHTING:
        *lpdwRenderState = device->FfpState.lightingEnabled ? 1 : 0;
        break;

    default:
        *lpdwRenderState = 0;
        break;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_SetTextureStageState(
    IDirect3DDevice7 *This,
    DWORD dwStage,
    D3DTEXTURESTAGESTATETYPE7 dwState,
    DWORD dwValue)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;

    if (dwStage >= D3D_MAX_TEXTURE_STAGES) return E_INVALIDARG;

    /* Handle texture stage states */
    switch (dwState) {
    case D3DTSS7_COLOROP:
        device->FfpState.textureStages[dwStage].colorOp = (D3D_TEXTURE_OP)dwValue;
        break;

    case D3DTSS7_COLORARG1:
        device->FfpState.textureStages[dwStage].colorArg1 = (D3D_TEXTURE_ARG)dwValue;
        break;

    case D3DTSS7_COLORARG2:
        device->FfpState.textureStages[dwStage].colorArg2 = (D3D_TEXTURE_ARG)dwValue;
        break;

    default:
        break;
    }

    device->FfpState.stateHash++;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_SetTexture(
    IDirect3DDevice7 *This,
    DWORD dwStage,
    IDirectDrawSurface7 *lpTexture)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;

    if (dwStage >= D3D_MAX_TEXTURE_STAGES) return E_INVALIDARG;

    /* TODO: Convert DirectDrawSurface7 to IGLTexture */
    device->FfpState.textureStages[dwStage].texture = NULL;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_DrawPrimitive(
    IDirect3DDevice7 *This,
    D3DPRIMITIVETYPE7 dptPrimitiveType,
    DWORD dwVertexTypeDesc,
    VOID *lpvVertices,
    DWORD dwVertexCount,
    DWORD dwFlags)
{
    D3D7_DEVICE *device = (D3D7_DEVICE*)This;
    GLenum glPrimType;
    HRESULT hr;

    if (!lpvVertices) return E_POINTER;

    /* Parse FVF if it changed */
    if (dwVertexTypeDesc != device->CurrentFVF) {
        hr = D3DParseFVF(dwVertexTypeDesc, &device->CurrentFVFDesc);
        if (FAILED(hr)) return hr;
        device->CurrentFVF = dwVertexTypeDesc;

        /* Regenerate shader for new FVF */
        hr = D3DUpdateFFPShaderProgram(device->GlDevice,
                                       &device->FfpState,
                                       &device->CurrentFVFDesc);
        if (FAILED(hr)) return hr;
    }

    /* Apply state */
    D3DApplyFFPState(device->GlContext, &device->FfpState);

    /* Convert primitive type */
    glPrimType = D3DPrimitiveTypeToGL(dptPrimitiveType);

    /* Use shader program */
    if (device->FfpState.currentProgram) {
        IGLContext_UseProgram(device->GlContext, device->FfpState.currentProgram);
    }

    /* TODO: Setup vertex attributes from FVF descriptor */
    /* TODO: Actually draw the primitives */

    return IGLContext_DrawArrays(device->GlContext, glPrimType, 0, dwVertexCount);
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_DrawIndexedPrimitive(
    IDirect3DDevice7 *This,
    D3DPRIMITIVETYPE7 d3dptPrimitiveType,
    DWORD dwVertexTypeDesc,
    VOID *lpvVertices,
    DWORD dwVertexCount,
    UINT16 *lpwIndices,
    DWORD dwIndexCount,
    DWORD dwFlags)
{
    /* Similar to DrawPrimitive but with indices */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_DrawPrimitiveVB(
    IDirect3DDevice7 *This,
    D3DPRIMITIVETYPE7 d3dptPrimitiveType,
    IDirect3DVertexBuffer7 *lpd3dVertexBuffer,
    DWORD dwStartVertex,
    DWORD dwNumVertices,
    DWORD dwFlags)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
D3D7Device_DrawIndexedPrimitiveVB(
    IDirect3DDevice7 *This,
    D3DPRIMITIVETYPE7 d3dptPrimitiveType,
    IDirect3DVertexBuffer7 *lpd3dVertexBuffer,
    DWORD dwStartVertex,
    DWORD dwNumVertices,
    UINT16 *lpwIndices,
    DWORD dwIndexCount,
    DWORD dwFlags)
{
    return E_NOTIMPL;
}

/* Device vtable */
static IDirect3DDevice7Vtbl D3D7DeviceVtbl = {
    .QueryInterface            = D3D7Device_QueryInterface,
    .AddRef                    = D3D7Device_AddRef,
    .Release                   = D3D7Device_Release,
    .GetCaps                   = D3D7Device_GetCaps,
    .BeginScene                = D3D7Device_BeginScene,
    .EndScene                  = D3D7Device_EndScene,
    .Clear                     = D3D7Device_Clear,
    .SetTransform              = D3D7Device_SetTransform,
    .GetTransform              = D3D7Device_GetTransform,
    .SetViewport               = D3D7Device_SetViewport,
    .SetMaterial               = D3D7Device_SetMaterial,
    .SetLight                  = D3D7Device_SetLight,
    .LightEnable               = D3D7Device_LightEnable,
    .SetRenderState            = D3D7Device_SetRenderState,
    .GetRenderState            = D3D7Device_GetRenderState,
    .SetTextureStageState      = D3D7Device_SetTextureStageState,
    .SetTexture                = D3D7Device_SetTexture,
    .DrawPrimitive             = D3D7Device_DrawPrimitive,
    .DrawIndexedPrimitive      = D3D7Device_DrawIndexedPrimitive,
    .DrawPrimitiveVB           = D3D7Device_DrawPrimitiveVB,
    .DrawIndexedPrimitiveVB    = D3D7Device_DrawIndexedPrimitiveVB,
};

/* --------------------------------------------------------------- */
/*  IDirect3D7 - Main interface (stub)                             */
/* --------------------------------------------------------------- */

typedef struct _D3D7_MAIN {
    IDirect3D7Vtbl *lpVtbl;
    UINT32          RefCount;
    IGLDevice      *GlDevice;
} D3D7_MAIN;

static HRESULT STDMETHODCALLTYPE
D3D7_QueryInterface(
    IDirect3D7 *This,
    REFIID riid,
    void **ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (RtlIsEqualGuid(riid, &IID_IUnknown) ||
        RtlIsEqualGuid(riid, &IID_IDirect3D7))
    {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
D3D7_AddRef(IDirect3D7 *This)
{
    D3D7_MAIN *d3d = (D3D7_MAIN*)This;
    return ++d3d->RefCount;
}

static UINT32 STDMETHODCALLTYPE
D3D7_Release(IDirect3D7 *This)
{
    D3D7_MAIN *d3d = (D3D7_MAIN*)This;
    UINT32 refCount = --d3d->RefCount;

    if (refCount == 0) {
        if (d3d->GlDevice) {
            IUnknown_Release((IUnknown*)d3d->GlDevice);
        }
        RtlFreeMemory(d3d);
    }

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
D3D7_EnumDevices(
    IDirect3D7 *This,
    VOID *lpEnumCallback,
    VOID *lpUserArg)
{
    /* Stub: enumerate HAL device */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D7_CreateDevice(
    IDirect3D7 *This,
    CONST GUID *rclsid,
    IDirectDrawSurface7 *lpDDS,
    IDirect3DDevice7 **lplpD3DDevice)
{
    D3D7_MAIN *d3d = (D3D7_MAIN*)This;
    D3D7_DEVICE *device;
    HRESULT hr;

    if (!lplpD3DDevice) return E_POINTER;

    device = (D3D7_DEVICE*)RtlAllocateMemory(sizeof(D3D7_DEVICE));
    if (!device) return E_OUTOFMEMORY;

    RtlZeroMemory(device, sizeof(D3D7_DEVICE));
    device->lpVtbl = &D3D7DeviceVtbl;
    device->RefCount = 1;

    /* Share GL device */
    device->GlDevice = d3d->GlDevice;
    IUnknown_AddRef((IUnknown*)device->GlDevice);

    /* Create GL context */
    hr = IGLDevice_CreateContext(d3d->GlDevice, &device->GlContext);
    if (FAILED(hr)) {
        RtlFreeMemory(device);
        return hr;
    }

    /* Initialize FFP state */
    hr = D3DInitializeFFPState(&device->FfpState);
    if (FAILED(hr)) {
        IUnknown_Release((IUnknown*)device->GlContext);
        RtlFreeMemory(device);
        return hr;
    }

    device->InScene = FALSE;
    device->CurrentFVF = 0;

    *lplpD3DDevice = (IDirect3DDevice7*)device;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
D3D7_CreateVertexBuffer(
    IDirect3D7 *This,
    D3DVERTEXBUFFERDESC7 *lpVBDesc,
    IDirect3DVertexBuffer7 **lplpD3DVertexBuffer,
    DWORD dwFlags)
{
    /* TODO: Implement vertex buffer */
    return E_NOTIMPL;
}

static IDirect3D7Vtbl D3D7Vtbl = {
    .QueryInterface       = D3D7_QueryInterface,
    .AddRef               = D3D7_AddRef,
    .Release              = D3D7_Release,
    .EnumDevices          = D3D7_EnumDevices,
    .CreateDevice         = D3D7_CreateDevice,
    .CreateVertexBuffer   = D3D7_CreateVertexBuffer,
};

/* --------------------------------------------------------------- */
/*  Direct3DCreate7 - Entry point                                  */
/* --------------------------------------------------------------- */

HRESULT
Direct3DCreate7(
    IDirect3D7 **ppDirect3D7)
{
    D3D7_MAIN *d3d;
    HRESULT hr;

    if (!ppDirect3D7) return E_POINTER;

    d3d = (D3D7_MAIN*)RtlAllocateMemory(sizeof(D3D7_MAIN));
    if (!d3d) return E_OUTOFMEMORY;

    RtlZeroMemory(d3d, sizeof(D3D7_MAIN));
    d3d->lpVtbl = &D3D7Vtbl;
    d3d->RefCount = 1;

    /* Create GL device (shared GLES20 backend) */
    hr = GLCreateDevice(&d3d->GlDevice);
    if (FAILED(hr)) {
        RtlFreeMemory(d3d);
        return hr;
    }

    *ppDirect3D7 = (IDirect3D7*)d3d;
    return S_OK;
}
