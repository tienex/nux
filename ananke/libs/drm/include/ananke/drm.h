/*++
    Module Name:

        drm.h

    Abstract:

        DRM (Direct Rendering Manager) abstraction layer for ananke.
        Provides mode setting, display management, and framebuffer allocation
        compatible with Linux KMS/DRM concepts.

    Environment:

        C and C++ compatible. Bare-metal UEFI/bootloader environment.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>
#include <ananke/framebuffer.h>

/* --------------------------------------------------------------- */
/*  Display Mode Definitions                                        */
/* --------------------------------------------------------------- */

typedef struct _DRM_MODE {
    UINT32  Width;          /* Horizontal resolution in pixels */
    UINT32  Height;         /* Vertical resolution in pixels */
    UINT32  RefreshRate;    /* Refresh rate in Hz */
    UINT32  BitsPerPixel;   /* Color depth */
    UINT32  Flags;          /* Mode flags (interlaced, vsync, etc.) */
    CHAR8   Name[32];       /* Human-readable mode name */
} DRM_MODE;

/* Mode flags */
#define DRM_MODE_FLAG_INTERLACED    (1 << 0)
#define DRM_MODE_FLAG_VSYNC         (1 << 1)
#define DRM_MODE_FLAG_HSYNC         (1 << 2)
#define DRM_MODE_FLAG_PREFERRED     (1 << 3)

/* --------------------------------------------------------------- */
/*  Connector Types (Display Output)                               */
/* --------------------------------------------------------------- */

typedef enum _DRM_CONNECTOR_TYPE {
    DrmConnectorUnknown = 0,
    DrmConnectorVGA = 1,
    DrmConnectorDVI = 2,
    DrmConnectorHDMI = 3,
    DrmConnectorDisplayPort = 4,
    DrmConnectorEDP = 5,        /* Embedded DisplayPort (laptop screens) */
    DrmConnectorLVDS = 6,       /* Low Voltage Differential Signaling */
} DRM_CONNECTOR_TYPE;

typedef enum _DRM_CONNECTOR_STATUS {
    DrmStatusUnknown = 0,
    DrmStatusConnected = 1,
    DrmStatusDisconnected = 2,
} DRM_CONNECTOR_STATUS;

/* --------------------------------------------------------------- */
/*  Connector Information                                           */
/* --------------------------------------------------------------- */

typedef struct _DRM_CONNECTOR_INFO {
    UINT32                  ConnectorId;
    DRM_CONNECTOR_TYPE      Type;
    DRM_CONNECTOR_STATUS    Status;
    UINT32                  PhysicalWidthMm;   /* Physical width in mm */
    UINT32                  PhysicalHeightMm;  /* Physical height in mm */
    UINT32                  NumModes;          /* Number of supported modes */
    UINT32                  CurrentModeIndex;  /* Index of current mode (-1 if none) */
    CHAR8                   Name[64];          /* Connector name */
} DRM_CONNECTOR_INFO;

/* --------------------------------------------------------------- */
/*  Framebuffer Handle                                              */
/* --------------------------------------------------------------- */

typedef struct _DRM_FRAMEBUFFER {
    UINT32  FbId;           /* Framebuffer ID */
    UINT32  Width;          /* Width in pixels */
    UINT32  Height;         /* Height in pixels */
    UINT32  Pitch;          /* Bytes per scanline */
    UINT32  BitsPerPixel;   /* Color depth */
    UINT64  PhysicalBase;   /* Physical memory address */
    UINT64  Size;           /* Total size in bytes */
    VOID*   VirtualBase;    /* Mapped virtual address (if available) */
} DRM_FRAMEBUFFER;

/* --------------------------------------------------------------- */
/*  CRTC (Display Controller) Information                           */
/* --------------------------------------------------------------- */

typedef struct _DRM_CRTC_INFO {
    UINT32  CrtcId;
    UINT32  FbId;           /* Currently attached framebuffer */
    UINT32  ConnectorId;    /* Currently attached connector */
    UINT32  X;              /* Framebuffer X offset */
    UINT32  Y;              /* Framebuffer Y offset */
    BOOLEAN Active;         /* Is CRTC active? */
} DRM_CRTC_INFO;

/* --------------------------------------------------------------- */
/*  IDrmDevice Interface                                            */
/* --------------------------------------------------------------- */

/* {A1B2C3D4-E5F6-4718-9A0B-1C2D3E4F5A6B} */
#define ANX_IID_IDrmDevice "A1B2C3D4-E5F6-4718-9A0B-1C2D3E4F5A6B"
ANX_DEFINE_GUID(IID_IDrmDevice, 0xA1B2C3D4,0xE5F6,0x4718,0x9A,0x0B,0x1C,0x2D,0x3E,0x4F,0x5A,0x6B);

ANX_BEGIN_INTERFACE(IDrmDevice, IUnknown, IID_IDrmDevice, ANX_IID_IDrmDevice)
    /* Initialize DRM device */
    ANX_IFACE_METHOD(HRESULT, Initialize, (VOID))

    /* Get number of connectors */
    ANX_IFACE_METHOD(HRESULT, GetConnectorCount, (OUT UINT32* pCount))

    /* Get connector information by index */
    ANX_IFACE_METHOD(HRESULT, GetConnectorInfo, (
        IN  UINT32 Index,
        OUT DRM_CONNECTOR_INFO* pInfo
    ))

    /* Get supported modes for a connector */
    ANX_IFACE_METHOD(HRESULT, GetConnectorModes, (
        IN  UINT32 ConnectorId,
        OUT DRM_MODE* pModes,
        IN  UINT32 MaxModes,
        OUT UINT32* pNumModes
    ))

    /* Set display mode on a connector */
    ANX_IFACE_METHOD(HRESULT, SetMode, (
        IN UINT32 ConnectorId,
        IN const DRM_MODE* pMode
    ))

    /* Create a framebuffer */
    ANX_IFACE_METHOD(HRESULT, CreateFramebuffer, (
        IN  UINT32 Width,
        IN  UINT32 Height,
        IN  UINT32 BitsPerPixel,
        OUT DRM_FRAMEBUFFER* pFramebuffer
    ))

    /* Destroy a framebuffer */
    ANX_IFACE_METHOD(HRESULT, DestroyFramebuffer, (
        IN UINT32 FbId
    ))

    /* Display a framebuffer on a connector (page flip) */
    ANX_IFACE_METHOD(HRESULT, DisplayFramebuffer, (
        IN UINT32 ConnectorId,
        IN UINT32 FbId
    ))

    /* Wait for vertical blank (VSync) */
    ANX_IFACE_METHOD(HRESULT, WaitVBlank, (
        IN UINT32 ConnectorId
    ))

    /* Get current CRTC info */
    ANX_IFACE_METHOD(HRESULT, GetCrtcInfo, (
        IN  UINT32 CrtcId,
        OUT DRM_CRTC_INFO* pInfo
    ))
ANX_END_INTERFACE(IDrmDevice)

/* COM method wrappers for C */
#ifndef __cplusplus
#define IDrmDevice_Initialize(This) \
    ((This)->lpVtbl->Initialize(This))
#define IDrmDevice_GetConnectorCount(This, pCount) \
    ((This)->lpVtbl->GetConnectorCount((This), (pCount)))
#define IDrmDevice_GetConnectorInfo(This, Index, pInfo) \
    ((This)->lpVtbl->GetConnectorInfo((This), (Index), (pInfo)))
#define IDrmDevice_GetConnectorModes(This, ConnectorId, pModes, MaxModes, pNumModes) \
    ((This)->lpVtbl->GetConnectorModes((This), (ConnectorId), (pModes), (MaxModes), (pNumModes)))
#define IDrmDevice_SetMode(This, ConnectorId, pMode) \
    ((This)->lpVtbl->SetMode((This), (ConnectorId), (pMode)))
#define IDrmDevice_CreateFramebuffer(This, Width, Height, Bpp, pFb) \
    ((This)->lpVtbl->CreateFramebuffer((This), (Width), (Height), (Bpp), (pFb)))
#define IDrmDevice_DestroyFramebuffer(This, FbId) \
    ((This)->lpVtbl->DestroyFramebuffer((This), (FbId)))
#define IDrmDevice_DisplayFramebuffer(This, ConnectorId, FbId) \
    ((This)->lpVtbl->DisplayFramebuffer((This), (ConnectorId), (FbId)))
#define IDrmDevice_WaitVBlank(This, ConnectorId) \
    ((This)->lpVtbl->WaitVBlank((This), (ConnectorId)))
#define IDrmDevice_GetCrtcInfo(This, CrtcId, pInfo) \
    ((This)->lpVtbl->GetCrtcInfo((This), (CrtcId), (pInfo)))
#endif

/* --------------------------------------------------------------- */
/*  Factory Functions                                               */
/* --------------------------------------------------------------- */

/* Create DRM device using UEFI GOP (Graphics Output Protocol) */
IDrmDevice* DrmCreateUefiGopDevice(VOID* GopProtocol);

/* Create DRM device using existing framebuffer backend */
IDrmDevice* DrmCreateFromFramebuffer(IFramebufferBackend* Backend);

/* Create generic DRM device (auto-detect hardware) */
IDrmDevice* DrmCreateDevice(VOID);
