/*++
    Module Name:

        manager.h

    Abstract:

        Framebuffer manager interface for device enumeration and management.
        This is the main user-facing API for accessing framebuffers.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>
#include <ananke/framebuffer.h>

/* --------------------------------------------------------------- */
/*  Graphics Mode Descriptor                                        */
/* --------------------------------------------------------------- */

typedef enum _FB_MODE_FLAGS {
    FbModeFlagNone              = 0x00000000,
    FbModeFlagHardwareAccel     = 0x00000001,  /* Hardware acceleration available */
    FbModeFlagPageFlipping      = 0x00000002,  /* Page flipping/double buffering */
    FbModeFlagVBlankSync        = 0x00000004,  /* VBlank synchronization */
    FbModeFlagPlanar            = 0x00000008,  /* Planar mode */
    FbModeFlagIndexed           = 0x00000010,  /* Indexed color mode */
    FbModeFlagInterlaced        = 0x00000020,  /* Interlaced mode */
    FbModeFlagStereoscopic      = 0x00000040,  /* Stereoscopic 3D */
} FB_MODE_FLAGS;

typedef struct _FB_MODE_DESC {
    UINT32              ModeNumber;         /* Backend-specific mode number */
    UINT32              Width;              /* Width in pixels */
    UINT32              Height;             /* Height in pixels */
    FB_PIXEL_FORMAT     PixelFormat;        /* Pixel format */
    UINT32              RefreshRate;        /* Refresh rate in Hz (0 if unknown) */
    UINT32              Flags;              /* FB_MODE_FLAGS */
    UINT32              MaxPages;           /* Number of pages for page flipping */

    /* RGB bit masks (for RGB modes) */
    UINT32              RedMask;
    UINT32              GreenMask;
    UINT32              BlueMask;
    UINT32              AlphaMask;

    /* Planar mode info */
    UINT32              NumPlanes;          /* Number of bit planes */
    UINT32              PlaneStride;        /* Bytes between planes */

    /* Palette info */
    UINT32              PaletteSize;        /* Number of palette entries */
} FB_MODE_DESC;

/* --------------------------------------------------------------- */
/*  Hardware Capabilities                                           */
/* --------------------------------------------------------------- */

typedef enum _FB_CAPABILITIES {
    FbCapNone                   = 0x00000000,
    FbCapHardwareFill           = 0x00000001,  /* Hardware rectangle fill */
    FbCapHardwareBlit           = 0x00000002,  /* Hardware blitting */
    FbCapHardwareROP            = 0x00000004,  /* Hardware ROP operations */
    FbCapHardwareLine           = 0x00000008,  /* Hardware line drawing */
    FbCapHardwareCursor         = 0x00000010,  /* Hardware cursor */
    FbCapColorCursor            = 0x00000020,  /* Color cursor (not just mono) */
    FbCapPageFlipping           = 0x00000040,  /* Page flipping */
    FbCapVBlankSync             = 0x00000080,  /* VBlank synchronization */
    FbCapPalette                = 0x00000100,  /* Palette support */
    FbCapAlpha                  = 0x00000200,  /* Alpha blending */
    FbCapOverlay                = 0x00000400,  /* Video overlay */
} FB_CAPABILITIES;

/* --------------------------------------------------------------- */
/*  Framebuffer Device Information                                  */
/* --------------------------------------------------------------- */

typedef struct _FB_DEVICE_INFO {
    UINT32              DeviceId;           /* Device identifier */
    CHAR16              DeviceName[64];     /* Device name (e.g., "VGA", "VESA", "GOP") */
    CHAR16              VendorName[64];     /* Vendor name */
    UINT32              Capabilities;       /* FB_CAPABILITIES flags */
    UINT32              VideoMemorySize;    /* Total video memory in bytes */
    UINT32              NumModes;           /* Number of available modes */
    UINT32              CurrentMode;        /* Current mode number */
} FB_DEVICE_INFO;

/* --------------------------------------------------------------- */
/*  IFramebufferManager - Main manager interface                    */
/* --------------------------------------------------------------- */

#define ANX_IID_IFramebufferManager "FB000010-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferManager,
    0xFB000010, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

/* Forward declarations */
typedef struct _IFramebufferScreen IFramebufferScreen;
typedef struct _IFramebufferSurface IFramebufferSurface;

ANX_BEGIN_INTERFACE(IFramebufferManager, IUnknown,
    IID_IFramebufferManager, ANX_IID_IFramebufferManager)

    /* Get number of available framebuffer devices */
    ANX_IFACE_METHOD(HRESULT, GetDeviceCount, (
        OUT UINT32 *Count))

    /* Get information about a specific device */
    ANX_IFACE_METHOD(HRESULT, GetDeviceInfo, (
        IN UINT32 DeviceId,
        OUT FB_DEVICE_INFO *Info))

    /* Enumerate available graphics modes for a device */
    ANX_IFACE_METHOD(HRESULT, EnumerateModes, (
        IN UINT32 DeviceId,
        OUT FB_MODE_DESC *Modes,
        IN UINT32 MaxModes,
        OUT UINT32 *NumModes))

    /* Get current graphics mode */
    ANX_IFACE_METHOD(HRESULT, GetCurrentMode, (
        IN UINT32 DeviceId,
        OUT FB_MODE_DESC *Mode))

    /* Set graphics mode */
    ANX_IFACE_METHOD(HRESULT, SetMode, (
        IN UINT32 DeviceId,
        IN UINT32 ModeNumber))

    /* Get framebuffer screen object for a device */
    ANX_IFACE_METHOD(HRESULT, GetScreen, (
        IN UINT32 DeviceId,
        OUT IFramebufferScreen **Screen))

    /* Create an offscreen surface */
    ANX_IFACE_METHOD(HRESULT, CreateSurface, (
        IN UINT32 Width,
        IN UINT32 Height,
        IN FB_PIXEL_FORMAT PixelFormat,
        OUT IFramebufferSurface **Surface))

    /* Get hardware capabilities */
    ANX_IFACE_METHOD(HRESULT, GetCapabilities, (
        IN UINT32 DeviceId,
        OUT UINT32 *Capabilities))

ANX_END_INTERFACE(IFramebufferManager)

/* --------------------------------------------------------------- */
/*  Helper Macros for C (COBJMACROS style)                          */
/* --------------------------------------------------------------- */

#ifndef __cplusplus

#define IFramebufferManager_GetDeviceCount(This, Count) \
    ((This)->lpVtbl->GetDeviceCount(This, Count))
#define IFramebufferManager_GetDeviceInfo(This, Id, Info) \
    ((This)->lpVtbl->GetDeviceInfo(This, Id, Info))
#define IFramebufferManager_EnumerateModes(This, Id, Modes, Max, Num) \
    ((This)->lpVtbl->EnumerateModes(This, Id, Modes, Max, Num))
#define IFramebufferManager_GetCurrentMode(This, Id, Mode) \
    ((This)->lpVtbl->GetCurrentMode(This, Id, Mode))
#define IFramebufferManager_SetMode(This, Id, Mode) \
    ((This)->lpVtbl->SetMode(This, Id, Mode))
#define IFramebufferManager_GetScreen(This, Id, Screen) \
    ((This)->lpVtbl->GetScreen(This, Id, Screen))
#define IFramebufferManager_CreateSurface(This, W, H, Fmt, Surf) \
    ((This)->lpVtbl->CreateSurface(This, W, H, Fmt, Surf))
#define IFramebufferManager_GetCapabilities(This, Id, Caps) \
    ((This)->lpVtbl->GetCapabilities(This, Id, Caps))

#endif /* !__cplusplus */

/* --------------------------------------------------------------- */
/*  Factory Function                                                */
/* --------------------------------------------------------------- */

/*
 * Create the global framebuffer manager.
 * This is the main entry point for the framebuffer library.
 */
IFramebufferManager *
FbCreateManager(
    VOID
    );
