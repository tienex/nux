/*++
    Module Name:

        kms_device.c

    Abstract:

        KMS device implementation - emulates Linux KMS API for bare-metal.
        Provides plane management and atomic mode setting.

--*/

#include <ananke/kms.h>
#include <string.h>

/* --------------------------------------------------------------- */
/*  KMS Device Implementation                                       */
/* --------------------------------------------------------------- */

#define MAX_PLANES 8

typedef struct {
    /* COM interface */
    const struct IKmsDeviceVtbl* lpVtbl;
    UINT32 RefCount;

    /* DRM device */
    IDrmDevice* DrmDevice;

    /* Planes */
    UINT32 NumPlanes;
    KMS_PLANE_DESC Planes[MAX_PLANES];
} KmsDevice;

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE KmsDevice_QueryInterface(
    IKmsDevice* This,
    REFIID riid,
    VOID** ppvObject
)
{
    if (!ppvObject) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IKmsDevice)) {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE KmsDevice_AddRef(IKmsDevice* This)
{
    KmsDevice* dev = (KmsDevice*)This;
    return ++dev->RefCount;
}

static UINT32 STDMETHODCALLTYPE KmsDevice_Release(IKmsDevice* This)
{
    KmsDevice* dev = (KmsDevice*)This;
    UINT32 refCount = --dev->RefCount;

    if (refCount == 0) {
        if (dev->DrmDevice) {
            IUnknown_Release((IUnknown*)dev->DrmDevice);
        }
        free(dev);
    }

    return refCount;
}

/* --------------------------------------------------------------- */
/*  IKmsDevice Implementation                                       */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE KmsDevice_Initialize(
    IKmsDevice* This,
    IDrmDevice* pDrmDevice
)
{
    KmsDevice* dev = (KmsDevice*)This;
    if (!pDrmDevice) return E_POINTER;

    dev->DrmDevice = pDrmDevice;
    IUnknown_AddRef((IUnknown*)pDrmDevice);

    /* Initialize default primary plane */
    dev->NumPlanes = 1;
    dev->Planes[0].PlaneId = 0;
    dev->Planes[0].Type = KmsPlaneTypePrimary;
    dev->Planes[0].PossibleCrtcs = 1;  /* Bit 0 = CRTC 0 */
    dev->Planes[0].ZPos = 0;
    dev->Planes[0].Enabled = FALSE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KmsDevice_GetPlaneCount(
    IKmsDevice* This,
    UINT32* pCount
)
{
    KmsDevice* dev = (KmsDevice*)This;
    if (!pCount) return E_POINTER;

    *pCount = dev->NumPlanes;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KmsDevice_GetPlaneDesc(
    IKmsDevice* This,
    UINT32 PlaneId,
    KMS_PLANE_DESC* pDesc
)
{
    KmsDevice* dev = (KmsDevice*)This;
    if (!pDesc) return E_POINTER;
    if (PlaneId >= dev->NumPlanes) return E_INVALIDARG;

    memcpy(pDesc, &dev->Planes[PlaneId], sizeof(KMS_PLANE_DESC));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KmsDevice_AddFramebuffer(
    IKmsDevice* This,
    UINT32 Width,
    UINT32 Height,
    UINT32 Pitch,
    UINT32 BitsPerPixel,
    UINT64 PhysicalBase,
    UINT32* pFbId
)
{
    KmsDevice* dev = (KmsDevice*)This;
    if (!dev->DrmDevice || !pFbId) return E_POINTER;

    /* Use DRM to create framebuffer */
    DRM_FRAMEBUFFER fb;
    HRESULT hr = IDrmDevice_CreateFramebuffer(
        dev->DrmDevice,
        Width,
        Height,
        BitsPerPixel,
        &fb
    );

    if (SUCCEEDED(hr)) {
        *pFbId = fb.FbId;
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE KmsDevice_RemoveFramebuffer(
    IKmsDevice* This,
    UINT32 FbId
)
{
    KmsDevice* dev = (KmsDevice*)This;
    if (!dev->DrmDevice) return E_POINTER;

    return IDrmDevice_DestroyFramebuffer(dev->DrmDevice, FbId);
}

static HRESULT STDMETHODCALLTYPE KmsDevice_SetPlane(
    IKmsDevice* This,
    UINT32 PlaneId,
    UINT32 CrtcId,
    UINT32 FbId,
    UINT32 Flags,
    INT32  CrtcX,
    INT32  CrtcY,
    UINT32 CrtcW,
    UINT32 CrtcH,
    INT32  SrcX,
    INT32  SrcY,
    UINT32 SrcW,
    UINT32 SrcH
)
{
    KmsDevice* dev = (KmsDevice*)This;
    if (PlaneId >= dev->NumPlanes) return E_INVALIDARG;

    /* Enable plane and store configuration */
    dev->Planes[PlaneId].Enabled = TRUE;

    /* For primary plane, update DRM framebuffer */
    if (dev->Planes[PlaneId].Type == KmsPlaneTypePrimary && dev->DrmDevice) {
        return IDrmDevice_DisplayFramebuffer(dev->DrmDevice, CrtcId, FbId);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KmsDevice_DisablePlane(
    IKmsDevice* This,
    UINT32 PlaneId
)
{
    KmsDevice* dev = (KmsDevice*)This;
    if (PlaneId >= dev->NumPlanes) return E_INVALIDARG;

    dev->Planes[PlaneId].Enabled = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KmsDevice_AtomicCommit(
    IKmsDevice* This,
    const KMS_ATOMIC_STATE* pState,
    UINT32 NumStates,
    UINT32 Flags
)
{
    KmsDevice* dev = (KmsDevice*)This;
    if (!pState || NumStates == 0) return E_POINTER;

    /* Process each atomic state */
    for (UINT32 i = 0; i < NumStates; i++) {
        const KMS_ATOMIC_STATE* state = &pState[i];

        /* Apply plane configuration */
        HRESULT hr = IKmsDevice_SetPlane(
            This,
            state->PlaneId,
            state->CrtcId,
            state->FbId,
            state->Flags,
            state->CrtcX,
            state->CrtcY,
            state->CrtcW,
            state->CrtcH,
            state->SrcX,
            state->SrcY,
            state->SrcW,
            state->SrcH
        );

        if (FAILED(hr)) return hr;
    }

    /* Wait for VBlank unless non-blocking */
    if (!(Flags & KMS_ATOMIC_NONBLOCK) && dev->DrmDevice) {
        IDrmDevice_WaitVBlank(dev->DrmDevice, 0);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KmsDevice_PageFlip(
    IKmsDevice* This,
    UINT32 CrtcId,
    UINT32 FbId,
    UINT32 Flags
)
{
    KmsDevice* dev = (KmsDevice*)This;
    if (!dev->DrmDevice) return E_POINTER;

    /* Display framebuffer on CRTC */
    HRESULT hr = IDrmDevice_DisplayFramebuffer(dev->DrmDevice, CrtcId, FbId);

    /* Wait for VBlank if blocking */
    if (SUCCEEDED(hr) && !(Flags & KMS_ATOMIC_NONBLOCK)) {
        IDrmDevice_WaitVBlank(dev->DrmDevice, CrtcId);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE KmsDevice_WaitVBlank(
    IKmsDevice* This,
    UINT32 CrtcId
)
{
    KmsDevice* dev = (KmsDevice*)This;
    if (!dev->DrmDevice) return E_POINTER;

    return IDrmDevice_WaitVBlank(dev->DrmDevice, CrtcId);
}

static HRESULT STDMETHODCALLTYPE KmsDevice_GetDrmDevice(
    IKmsDevice* This,
    IDrmDevice** ppDrmDevice
)
{
    KmsDevice* dev = (KmsDevice*)This;
    if (!ppDrmDevice) return E_POINTER;

    *ppDrmDevice = dev->DrmDevice;
    if (dev->DrmDevice) {
        IUnknown_AddRef((IUnknown*)dev->DrmDevice);
        return S_OK;
    }

    return E_FAIL;
}

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static const struct IKmsDeviceVtbl KmsDeviceVtbl = {
    KmsDevice_QueryInterface,
    KmsDevice_AddRef,
    KmsDevice_Release,
    KmsDevice_Initialize,
    KmsDevice_GetPlaneCount,
    KmsDevice_GetPlaneDesc,
    KmsDevice_AddFramebuffer,
    KmsDevice_RemoveFramebuffer,
    KmsDevice_SetPlane,
    KmsDevice_DisablePlane,
    KmsDevice_AtomicCommit,
    KmsDevice_PageFlip,
    KmsDevice_WaitVBlank,
    KmsDevice_GetDrmDevice,
};

/* --------------------------------------------------------------- */
/*  Factory Functions                                               */
/* --------------------------------------------------------------- */

IKmsDevice* KmsCreateDevice(VOID)
{
    KmsDevice* dev = (KmsDevice*)malloc(sizeof(KmsDevice));
    if (!dev) return NULL;

    memset(dev, 0, sizeof(KmsDevice));
    dev->lpVtbl = &KmsDeviceVtbl;
    dev->RefCount = 1;

    return (IKmsDevice*)dev;
}
