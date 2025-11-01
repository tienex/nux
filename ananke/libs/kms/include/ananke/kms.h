/*++
    Module Name:

        kms.h

    Abstract:

        KMS (Kernel Mode Setting) emulation layer for ananke.
        Provides a Linux KMS-compatible API for bare-metal/UEFI environments.
        Works with DRM for mode setting and GLESv20 for rendering.

    Environment:

        C and C++ compatible. Bare-metal UEFI/bootloader environment.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>
#include <ananke/drm.h>
#include <ananke/framebuffer.h>

/* --------------------------------------------------------------- */
/*  KMS Plane Types                                                 */
/* --------------------------------------------------------------- */

typedef enum _KMS_PLANE_TYPE {
    KmsPlaneTypePrimary = 0,    /* Primary display plane */
    KmsPlaneTypeOverlay = 1,    /* Overlay plane */
    KmsPlaneTypeCursor = 2,     /* Hardware cursor plane */
} KMS_PLANE_TYPE;

/* --------------------------------------------------------------- */
/*  KMS Plane Properties                                            */
/* --------------------------------------------------------------- */

typedef struct _KMS_PLANE_DESC {
    UINT32          PlaneId;
    KMS_PLANE_TYPE  Type;
    UINT32          PossibleCrtcs;  /* Bitmask of CRTCs this plane can use */
    UINT32          ZPos;           /* Z position (0 = bottom) */
    BOOLEAN         Enabled;
} KMS_PLANE_DESC;

/* --------------------------------------------------------------- */
/*  KMS Atomic State (for atomic mode setting)                     */
/* --------------------------------------------------------------- */

typedef struct _KMS_ATOMIC_STATE {
    UINT32  CrtcId;
    UINT32  ConnectorId;
    UINT32  FbId;
    UINT32  PlaneId;
    INT32   SrcX;       /* Source rectangle in framebuffer */
    INT32   SrcY;
    UINT32  SrcW;
    UINT32  SrcH;
    INT32   CrtcX;      /* Destination rectangle on CRTC */
    INT32   CrtcY;
    UINT32  CrtcW;
    UINT32  CrtcH;
    UINT32  Flags;
} KMS_ATOMIC_STATE;

/* Atomic commit flags */
#define KMS_ATOMIC_NONBLOCK     (1 << 0)    /* Non-blocking commit */
#define KMS_ATOMIC_ALLOW_MODESET (1 << 1)   /* Allow mode change */

/* --------------------------------------------------------------- */
/*  IKmsDevice Interface                                            */
/* --------------------------------------------------------------- */

/* {D4E5F6A7-B8C9-4A3B-8D1E-2F3A4B5C6D7E} */
#define ANX_IID_IKmsDevice "D4E5F6A7-B8C9-4A3B-8D1E-2F3A4B5C6D7E"
ANX_DEFINE_GUID(IID_IKmsDevice, 0xD4E5F6A7,0xB8C9,0x4A3B,0x8D,0x1E,0x2F,0x3A,0x4B,0x5C,0x6D,0x7E);

ANX_BEGIN_INTERFACE(IKmsDevice, IUnknown, IID_IKmsDevice, ANX_IID_IKmsDevice)
    /* Initialize KMS device with DRM backend */
    ANX_IFACE_METHOD(HRESULT, Initialize, (IN IDrmDevice* pDrmDevice))

    /* Get number of planes */
    ANX_IFACE_METHOD(HRESULT, GetPlaneCount, (OUT UINT32* pCount))

    /* Get plane properties */
    ANX_IFACE_METHOD(HRESULT, GetPlaneDesc, (
        IN  UINT32 PlaneId,
        OUT KMS_PLANE_DESC* pDesc
    ))

    /* Add framebuffer (returns FB ID) */
    ANX_IFACE_METHOD(HRESULT, AddFramebuffer, (
        IN  UINT32 Width,
        IN  UINT32 Height,
        IN  UINT32 Pitch,
        IN  UINT32 BitsPerPixel,
        IN  UINT64 PhysicalBase,
        OUT UINT32* pFbId
    ))

    /* Remove framebuffer */
    ANX_IFACE_METHOD(HRESULT, RemoveFramebuffer, (IN UINT32 FbId))

    /* Set plane framebuffer (legacy API) */
    ANX_IFACE_METHOD(HRESULT, SetPlane, (
        IN UINT32 PlaneId,
        IN UINT32 CrtcId,
        IN UINT32 FbId,
        IN UINT32 Flags,
        IN INT32  CrtcX,
        IN INT32  CrtcY,
        IN UINT32 CrtcW,
        IN UINT32 CrtcH,
        IN INT32  SrcX,
        IN INT32  SrcY,
        IN UINT32 SrcW,
        IN UINT32 SrcH
    ))

    /* Disable plane */
    ANX_IFACE_METHOD(HRESULT, DisablePlane, (IN UINT32 PlaneId))

    /* Atomic commit (modern API) */
    ANX_IFACE_METHOD(HRESULT, AtomicCommit, (
        IN const KMS_ATOMIC_STATE* pState,
        IN UINT32 NumStates,
        IN UINT32 Flags
    ))

    /* Page flip (swap framebuffers) */
    ANX_IFACE_METHOD(HRESULT, PageFlip, (
        IN UINT32 CrtcId,
        IN UINT32 FbId,
        IN UINT32 Flags
    ))

    /* Wait for VBlank */
    ANX_IFACE_METHOD(HRESULT, WaitVBlank, (IN UINT32 CrtcId))

    /* Get DRM device */
    ANX_IFACE_METHOD(HRESULT, GetDrmDevice, (OUT IDrmDevice** ppDrmDevice))
ANX_END_INTERFACE(IKmsDevice)

/* COM method wrappers for C */
#ifndef __cplusplus
#define IKmsDevice_Initialize(This, pDrmDevice) \
    ((This)->lpVtbl->Initialize((This), (pDrmDevice)))
#define IKmsDevice_GetPlaneCount(This, pCount) \
    ((This)->lpVtbl->GetPlaneCount((This), (pCount)))
#define IKmsDevice_GetPlaneDesc(This, PlaneId, pDesc) \
    ((This)->lpVtbl->GetPlaneDesc((This), (PlaneId), (pDesc)))
#define IKmsDevice_AddFramebuffer(This, W, H, Pitch, Bpp, PhysBase, pFbId) \
    ((This)->lpVtbl->AddFramebuffer((This), (W), (H), (Pitch), (Bpp), (PhysBase), (pFbId)))
#define IKmsDevice_RemoveFramebuffer(This, FbId) \
    ((This)->lpVtbl->RemoveFramebuffer((This), (FbId)))
#define IKmsDevice_SetPlane(This, PlaneId, CrtcId, FbId, Flags, CrtcX, CrtcY, CrtcW, CrtcH, SrcX, SrcY, SrcW, SrcH) \
    ((This)->lpVtbl->SetPlane((This), (PlaneId), (CrtcId), (FbId), (Flags), (CrtcX), (CrtcY), (CrtcW), (CrtcH), (SrcX), (SrcY), (SrcW), (SrcH)))
#define IKmsDevice_DisablePlane(This, PlaneId) \
    ((This)->lpVtbl->DisablePlane((This), (PlaneId)))
#define IKmsDevice_AtomicCommit(This, pState, NumStates, Flags) \
    ((This)->lpVtbl->AtomicCommit((This), (pState), (NumStates), (Flags)))
#define IKmsDevice_PageFlip(This, CrtcId, FbId, Flags) \
    ((This)->lpVtbl->PageFlip((This), (CrtcId), (FbId), (Flags)))
#define IKmsDevice_WaitVBlank(This, CrtcId) \
    ((This)->lpVtbl->WaitVBlank((This), (CrtcId)))
#define IKmsDevice_GetDrmDevice(This, ppDrmDevice) \
    ((This)->lpVtbl->GetDrmDevice((This), (ppDrmDevice)))
#endif

/* --------------------------------------------------------------- */
/*  IKmsRenderer Interface (GLESv20 integration)                   */
/* --------------------------------------------------------------- */

/* {E5F6A7B8-C9DA-4B4C-9E2F-3A4B5C6D7E8F} */
#define ANX_IID_IKmsRenderer "E5F6A7B8-C9DA-4B4C-9E2F-3A4B5C6D7E8F"
ANX_DEFINE_GUID(IID_IKmsRenderer, 0xE5F6A7B8,0xC9DA,0x4B4C,0x9E,0x2F,0x3A,0x4B,0x5C,0x6D,0x7E,0x8F);

ANX_BEGIN_INTERFACE(IKmsRenderer, IUnknown, IID_IKmsRenderer, ANX_IID_IKmsRenderer)
    /* Initialize renderer with KMS device */
    ANX_IFACE_METHOD(HRESULT, Initialize, (
        IN IKmsDevice* pKmsDevice,
        IN UINT32 Width,
        IN UINT32 Height
    ))

    /* Get GLESv20 surface for rendering */
    ANX_IFACE_METHOD(HRESULT, GetGLSurface, (OUT VOID** ppSurface))

    /* Get framebuffer backend for 2D drawing */
    ANX_IFACE_METHOD(HRESULT, GetFramebuffer, (OUT IFramebufferBackend** ppBackend))

    /* Begin frame */
    ANX_IFACE_METHOD(HRESULT, BeginFrame, (VOID))

    /* End frame and swap buffers */
    ANX_IFACE_METHOD(HRESULT, EndFrame, (VOID))

    /* Swap buffers (present to screen) */
    ANX_IFACE_METHOD(HRESULT, SwapBuffers, (VOID))
ANX_END_INTERFACE(IKmsRenderer)

/* COM method wrappers for C */
#ifndef __cplusplus
#define IKmsRenderer_Initialize(This, pKmsDevice, Width, Height) \
    ((This)->lpVtbl->Initialize((This), (pKmsDevice), (Width), (Height)))
#define IKmsRenderer_GetGLSurface(This, ppSurface) \
    ((This)->lpVtbl->GetGLSurface((This), (ppSurface)))
#define IKmsRenderer_GetFramebuffer(This, ppBackend) \
    ((This)->lpVtbl->GetFramebuffer((This), (ppBackend)))
#define IKmsRenderer_BeginFrame(This) \
    ((This)->lpVtbl->BeginFrame(This))
#define IKmsRenderer_EndFrame(This) \
    ((This)->lpVtbl->EndFrame(This))
#define IKmsRenderer_SwapBuffers(This) \
    ((This)->lpVtbl->SwapBuffers(This))
#endif

/* --------------------------------------------------------------- */
/*  Factory Functions                                               */
/* --------------------------------------------------------------- */

/* Create KMS device */
IKmsDevice* KmsCreateDevice(VOID);

/* Create KMS renderer with GLESv20 support */
IKmsRenderer* KmsCreateRenderer(VOID);
