/*++
    Module Name:

        drm_device.c

    Abstract:

        DRM device implementation using framebuffer backend.
        Provides mode setting and framebuffer management for bare-metal environments.

--*/

#include <ananke/drm.h>
#include <ananke/framebuffer.h>
#include <string.h>

/* --------------------------------------------------------------- */
/*  DRM Device Implementation                                       */
/* --------------------------------------------------------------- */

#define MAX_CONNECTORS 4
#define MAX_MODES 32
#define MAX_FRAMEBUFFERS 8

typedef struct {
    /* COM interface */
    const struct IDrmDeviceVtbl* lpVtbl;
    UINT32 RefCount;

    /* Framebuffer backend */
    IFramebufferBackend* Backend;

    /* Connectors */
    UINT32 NumConnectors;
    DRM_CONNECTOR_INFO Connectors[MAX_CONNECTORS];
    DRM_MODE Modes[MAX_CONNECTORS][MAX_MODES];
    UINT32 NumModes[MAX_CONNECTORS];

    /* Framebuffers */
    UINT32 NumFramebuffers;
    DRM_FRAMEBUFFER Framebuffers[MAX_FRAMEBUFFERS];
    UINT32 NextFbId;

    /* CRTC (one per connector for simplicity) */
    DRM_CRTC_INFO Crtcs[MAX_CONNECTORS];
} DrmDevice;

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE DrmDevice_QueryInterface(
    IDrmDevice* This,
    REFIID riid,
    VOID** ppvObject
)
{
    if (!ppvObject) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDrmDevice)) {
        *ppvObject = This;
        IUnknown_AddRef((IUnknown*)This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE DrmDevice_AddRef(IDrmDevice* This)
{
    DrmDevice* dev = (DrmDevice*)This;
    return ++dev->RefCount;
}

static UINT32 STDMETHODCALLTYPE DrmDevice_Release(IDrmDevice* This)
{
    DrmDevice* dev = (DrmDevice*)This;
    UINT32 refCount = --dev->RefCount;

    if (refCount == 0) {
        if (dev->Backend) {
            IUnknown_Release((IUnknown*)dev->Backend);
        }
        free(dev);
    }

    return refCount;
}

/* --------------------------------------------------------------- */
/*  IDrmDevice Implementation                                       */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE DrmDevice_Initialize(IDrmDevice* This)
{
    DrmDevice* dev = (DrmDevice*)This;

    /* Create default connector for framebuffer backend */
    dev->NumConnectors = 1;
    dev->Connectors[0].ConnectorId = 0;
    dev->Connectors[0].Type = DrmConnectorUnknown;
    dev->Connectors[0].Status = DrmStatusConnected;
    dev->Connectors[0].PhysicalWidthMm = 0;
    dev->Connectors[0].PhysicalHeightMm = 0;
    dev->Connectors[0].NumModes = 0;
    dev->Connectors[0].CurrentModeIndex = 0;
    strcpy(dev->Connectors[0].Name, "Framebuffer-0");

    /* Initialize CRTC */
    dev->Crtcs[0].CrtcId = 0;
    dev->Crtcs[0].FbId = 0;
    dev->Crtcs[0].ConnectorId = 0;
    dev->Crtcs[0].X = 0;
    dev->Crtcs[0].Y = 0;
    dev->Crtcs[0].Active = FALSE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DrmDevice_GetConnectorCount(
    IDrmDevice* This,
    UINT32* pCount
)
{
    DrmDevice* dev = (DrmDevice*)This;
    if (!pCount) return E_POINTER;

    *pCount = dev->NumConnectors;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DrmDevice_GetConnectorInfo(
    IDrmDevice* This,
    UINT32 Index,
    DRM_CONNECTOR_INFO* pInfo
)
{
    DrmDevice* dev = (DrmDevice*)This;
    if (!pInfo) return E_POINTER;
    if (Index >= dev->NumConnectors) return E_INVALIDARG;

    memcpy(pInfo, &dev->Connectors[Index], sizeof(DRM_CONNECTOR_INFO));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DrmDevice_GetConnectorModes(
    IDrmDevice* This,
    UINT32 ConnectorId,
    DRM_MODE* pModes,
    UINT32 MaxModes,
    UINT32* pNumModes
)
{
    DrmDevice* dev = (DrmDevice*)This;
    if (!pModes || !pNumModes) return E_POINTER;
    if (ConnectorId >= dev->NumConnectors) return E_INVALIDARG;

    UINT32 numModes = dev->NumModes[ConnectorId];
    if (numModes > MaxModes) numModes = MaxModes;

    memcpy(pModes, dev->Modes[ConnectorId], numModes * sizeof(DRM_MODE));
    *pNumModes = numModes;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DrmDevice_SetMode(
    IDrmDevice* This,
    UINT32 ConnectorId,
    const DRM_MODE* pMode
)
{
    DrmDevice* dev = (DrmDevice*)This;
    if (!pMode) return E_POINTER;
    if (ConnectorId >= dev->NumConnectors) return E_INVALIDARG;

    /* Store mode as current */
    /* In a real implementation, this would configure hardware */
    dev->Crtcs[ConnectorId].Active = TRUE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DrmDevice_CreateFramebuffer(
    IDrmDevice* This,
    UINT32 Width,
    UINT32 Height,
    UINT32 BitsPerPixel,
    DRM_FRAMEBUFFER* pFramebuffer
)
{
    DrmDevice* dev = (DrmDevice*)This;
    if (!pFramebuffer) return E_POINTER;
    if (dev->NumFramebuffers >= MAX_FRAMEBUFFERS) return E_OUTOFMEMORY;

    /* Calculate framebuffer parameters */
    UINT32 pitch = (Width * BitsPerPixel + 7) / 8;
    UINT64 size = pitch * Height;

    /* Allocate framebuffer (simplified - in real impl would allocate memory) */
    pFramebuffer->FbId = dev->NextFbId++;
    pFramebuffer->Width = Width;
    pFramebuffer->Height = Height;
    pFramebuffer->Pitch = pitch;
    pFramebuffer->BitsPerPixel = BitsPerPixel;
    pFramebuffer->PhysicalBase = 0;  /* Would be actual allocation */
    pFramebuffer->Size = size;
    pFramebuffer->VirtualBase = NULL;

    /* Store framebuffer */
    memcpy(&dev->Framebuffers[dev->NumFramebuffers], pFramebuffer, sizeof(DRM_FRAMEBUFFER));
    dev->NumFramebuffers++;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DrmDevice_DestroyFramebuffer(
    IDrmDevice* This,
    UINT32 FbId
)
{
    DrmDevice* dev = (DrmDevice*)This;

    /* Find and remove framebuffer */
    for (UINT32 i = 0; i < dev->NumFramebuffers; i++) {
        if (dev->Framebuffers[i].FbId == FbId) {
            /* Shift remaining framebuffers */
            for (UINT32 j = i; j < dev->NumFramebuffers - 1; j++) {
                dev->Framebuffers[j] = dev->Framebuffers[j + 1];
            }
            dev->NumFramebuffers--;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

static HRESULT STDMETHODCALLTYPE DrmDevice_DisplayFramebuffer(
    IDrmDevice* This,
    UINT32 ConnectorId,
    UINT32 FbId
)
{
    DrmDevice* dev = (DrmDevice*)This;
    if (ConnectorId >= dev->NumConnectors) return E_INVALIDARG;

    /* Set framebuffer as current */
    dev->Crtcs[ConnectorId].FbId = FbId;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DrmDevice_WaitVBlank(
    IDrmDevice* This,
    UINT32 ConnectorId
)
{
    /* Stub - in real implementation would wait for VBlank interrupt */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DrmDevice_GetCrtcInfo(
    IDrmDevice* This,
    UINT32 CrtcId,
    DRM_CRTC_INFO* pInfo
)
{
    DrmDevice* dev = (DrmDevice*)This;
    if (!pInfo) return E_POINTER;
    if (CrtcId >= dev->NumConnectors) return E_INVALIDARG;

    memcpy(pInfo, &dev->Crtcs[CrtcId], sizeof(DRM_CRTC_INFO));
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static const struct IDrmDeviceVtbl DrmDeviceVtbl = {
    DrmDevice_QueryInterface,
    DrmDevice_AddRef,
    DrmDevice_Release,
    DrmDevice_Initialize,
    DrmDevice_GetConnectorCount,
    DrmDevice_GetConnectorInfo,
    DrmDevice_GetConnectorModes,
    DrmDevice_SetMode,
    DrmDevice_CreateFramebuffer,
    DrmDevice_DestroyFramebuffer,
    DrmDevice_DisplayFramebuffer,
    DrmDevice_WaitVBlank,
    DrmDevice_GetCrtcInfo,
};

/* --------------------------------------------------------------- */
/*  Factory Functions                                               */
/* --------------------------------------------------------------- */

IDrmDevice* DrmCreateDevice(VOID)
{
    DrmDevice* dev = (DrmDevice*)malloc(sizeof(DrmDevice));
    if (!dev) return NULL;

    memset(dev, 0, sizeof(DrmDevice));
    dev->lpVtbl = &DrmDeviceVtbl;
    dev->RefCount = 1;
    dev->NextFbId = 1;

    return (IDrmDevice*)dev;
}

IDrmDevice* DrmCreateFromFramebuffer(IFramebufferBackend* Backend)
{
    if (!Backend) return NULL;

    DrmDevice* dev = (DrmDevice*)DrmCreateDevice();
    if (!dev) return NULL;

    dev->Backend = Backend;
    IUnknown_AddRef((IUnknown*)Backend);

    return (IDrmDevice*)dev;
}

IDrmDevice* DrmCreateUefiGopDevice(VOID* GopProtocol)
{
    /* For UEFI GOP, create framebuffer backend first */
    /* This is a simplified stub */
    return DrmCreateDevice();
}
