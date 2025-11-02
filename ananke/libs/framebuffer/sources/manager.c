/*++
    Module Name:

        manager.c

    Abstract:

        IFramebufferManager implementation.

        Provides high-level device enumeration, mode management, and
        screen/surface creation. This is the main entry point for
        applications using the framebuffer library.

--*/

#include <ananke/framebuffer/manager.h>
#include <ananke/framebuffer/screen.h>
#include <ananke/framebuffer/backends.h>
#include <ananke/framebuffer/com_helpers.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Manager Implementation Structure                               */
/* --------------------------------------------------------------- */

#define FB_MAX_DEVICES  8

typedef struct _FB_MANAGER_DEVICE {
    UINT32                  DeviceId;
    FB_BACKEND_TYPE         BackendType;
    FB_DEVICE_INFO          Info;
    IFramebufferScreen      *Screen;
    FRAMEBUFFER_DESC        CurrentDescriptor;
} FB_MANAGER_DEVICE;

typedef struct _FB_MANAGER_IMPL {
    IFramebufferManager     Base;
    REFOBJ                  RefCount;
    UINT32                  DeviceCount;
    FB_MANAGER_DEVICE       Devices[FB_MAX_DEVICES];
} FB_MANAGER_IMPL;

/* --------------------------------------------------------------- */
/*  Forward Declarations                                            */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE FbManager_QueryInterface(
    IFramebufferManager *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE FbManager_AddRef(IFramebufferManager *This);
static UINT32 STDMETHODCALLTYPE FbManager_Release(IFramebufferManager *This);
static HRESULT STDMETHODCALLTYPE FbManager_GetDeviceCount(
    IFramebufferManager *This, UINT32 *Count);
static HRESULT STDMETHODCALLTYPE FbManager_GetDeviceInfo(
    IFramebufferManager *This, UINT32 DeviceId, FB_DEVICE_INFO *Info);
static HRESULT STDMETHODCALLTYPE FbManager_EnumerateModes(
    IFramebufferManager *This, UINT32 DeviceId, FB_MODE_DESC *Modes,
    UINT32 MaxModes, UINT32 *NumModes);
static HRESULT STDMETHODCALLTYPE FbManager_GetCurrentMode(
    IFramebufferManager *This, UINT32 DeviceId, FB_MODE_DESC *Mode);
static HRESULT STDMETHODCALLTYPE FbManager_SetMode(
    IFramebufferManager *This, UINT32 DeviceId, UINT32 ModeNumber);
static HRESULT STDMETHODCALLTYPE FbManager_GetScreen(
    IFramebufferManager *This, UINT32 DeviceId, IFramebufferScreen **Screen);
static HRESULT STDMETHODCALLTYPE FbManager_CreateSurface(
    IFramebufferManager *This, UINT32 Width, UINT32 Height,
    FB_PIXEL_FORMAT PixelFormat, IFramebufferSurface **Surface);
static HRESULT STDMETHODCALLTYPE FbManager_GetCapabilities(
    IFramebufferManager *This, UINT32 DeviceId, UINT32 *Capabilities);

/* Internal helpers */
static VOID FbManager_EnumerateDevices(FB_MANAGER_IMPL *Manager);
static FB_MANAGER_DEVICE *FbManager_FindDevice(FB_MANAGER_IMPL *Manager, UINT32 DeviceId);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferManagerVtbl gManagerVtbl = {
    .QueryInterface     = FbManager_QueryInterface,
    .AddRef             = FbManager_AddRef,
    .Release            = FbManager_Release,
    .GetDeviceCount     = FbManager_GetDeviceCount,
    .GetDeviceInfo      = FbManager_GetDeviceInfo,
    .EnumerateModes     = FbManager_EnumerateModes,
    .GetCurrentMode     = FbManager_GetCurrentMode,
    .SetMode            = FbManager_SetMode,
    .GetScreen          = FbManager_GetScreen,
    .CreateSurface      = FbManager_CreateSurface,
    .GetCapabilities    = FbManager_GetCapabilities,
};

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

FB_IMPLEMENT_IUNKNOWN(FbManager, FB_MANAGER_IMPL, IFramebufferManager, IID_IFramebufferManager)

/* --------------------------------------------------------------- */
/*  IFramebufferManager Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbManager_GetDeviceCount(
    IFramebufferManager *This,
    UINT32 *Count
    )
{
    FB_MANAGER_IMPL *Manager = (FB_MANAGER_IMPL *)This;

    if (Count == NULL) {
        return E_POINTER;
    }

    *Count = Manager->DeviceCount;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbManager_GetDeviceInfo(
    IFramebufferManager *This,
    UINT32 DeviceId,
    FB_DEVICE_INFO *Info
    )
{
    FB_MANAGER_IMPL *Manager = (FB_MANAGER_IMPL *)This;
    FB_MANAGER_DEVICE *Device;

    if (Info == NULL) {
        return E_POINTER;
    }

    Device = FbManager_FindDevice(Manager, DeviceId);
    if (Device == NULL) {
        return E_INVALIDARG;
    }

    ANX_MEMCPY(Info, &Device->Info, sizeof(FB_DEVICE_INFO));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbManager_EnumerateModes(
    IFramebufferManager *This,
    UINT32 DeviceId,
    FB_MODE_DESC *Modes,
    UINT32 MaxModes,
    UINT32 *NumModes
    )
{
    FB_MANAGER_IMPL *Manager = (FB_MANAGER_IMPL *)This;
    FB_MANAGER_DEVICE *Device;

    if (Modes == NULL || NumModes == NULL) {
        return E_POINTER;
    }

    Device = FbManager_FindDevice(Manager, DeviceId);
    if (Device == NULL) {
        return E_INVALIDARG;
    }

    /* For now, return current mode only */
    if (MaxModes < 1) {
        *NumModes = 0;
        return S_OK;
    }

    /* Fill in current mode */
    ANX_MEMSET(&Modes[0], 0, sizeof(FB_MODE_DESC));
    Modes[0].ModeNumber = 0;
    Modes[0].Width = Device->CurrentDescriptor.Width;
    Modes[0].Height = Device->CurrentDescriptor.Height;
    Modes[0].PixelFormat = Device->CurrentDescriptor.PixelFormat;
    Modes[0].RefreshRate = 60;
    Modes[0].Flags = FbModeFlagNone;
    Modes[0].MaxPages = 1;

    *NumModes = 1;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbManager_GetCurrentMode(
    IFramebufferManager *This,
    UINT32 DeviceId,
    FB_MODE_DESC *Mode
    )
{
    FB_MANAGER_IMPL *Manager = (FB_MANAGER_IMPL *)This;
    FB_MANAGER_DEVICE *Device;

    if (Mode == NULL) {
        return E_POINTER;
    }

    Device = FbManager_FindDevice(Manager, DeviceId);
    if (Device == NULL) {
        return E_INVALIDARG;
    }

    /* Fill in current mode */
    ANX_MEMSET(Mode, 0, sizeof(FB_MODE_DESC));
    Mode->ModeNumber = 0;
    Mode->Width = Device->CurrentDescriptor.Width;
    Mode->Height = Device->CurrentDescriptor.Height;
    Mode->PixelFormat = Device->CurrentDescriptor.PixelFormat;
    Mode->RefreshRate = 60;
    Mode->Flags = FbModeFlagNone;
    Mode->MaxPages = 1;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbManager_SetMode(
    IFramebufferManager *This,
    UINT32 DeviceId,
    UINT32 ModeNumber
    )
{
    /* Mode switching not implemented - would need backend-specific code */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
FbManager_GetScreen(
    IFramebufferManager *This,
    UINT32 DeviceId,
    IFramebufferScreen **Screen
    )
{
    FB_MANAGER_IMPL *Manager = (FB_MANAGER_IMPL *)This;
    FB_MANAGER_DEVICE *Device;

    if (Screen == NULL) {
        return E_POINTER;
    }

    Device = FbManager_FindDevice(Manager, DeviceId);
    if (Device == NULL) {
        return E_INVALIDARG;
    }

    if (Device->Screen == NULL) {
        /* Create screen on demand */
        Device->Screen = FbCreateScreenByType(Device->BackendType, &Device->CurrentDescriptor);
        if (Device->Screen == NULL) {
            return E_FAIL;
        }
    }

    *Screen = Device->Screen;
    IUnknown_AddRef((IUnknown *)Device->Screen);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbManager_CreateSurface(
    IFramebufferManager *This,
    UINT32 Width,
    UINT32 Height,
    FB_PIXEL_FORMAT PixelFormat,
    IFramebufferSurface **Surface
    )
{
    if (Surface == NULL) {
        return E_POINTER;
    }

    *Surface = FbCreateSurface(Width, Height, PixelFormat);
    if (*Surface == NULL) {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbManager_GetCapabilities(
    IFramebufferManager *This,
    UINT32 DeviceId,
    UINT32 *Capabilities
    )
{
    FB_MANAGER_IMPL *Manager = (FB_MANAGER_IMPL *)This;
    FB_MANAGER_DEVICE *Device;

    if (Capabilities == NULL) {
        return E_POINTER;
    }

    Device = FbManager_FindDevice(Manager, DeviceId);
    if (Device == NULL) {
        return E_INVALIDARG;
    }

    *Capabilities = Device->Info.Capabilities;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Internal Helper Functions                                       */
/* --------------------------------------------------------------- */

static VOID
FbManager_EnumerateDevices(
    FB_MANAGER_IMPL *Manager
    )
{
    IFramebufferBackend *Backend;
    FRAMEBUFFER_DESC Descriptor;
    HRESULT Hr;

    Manager->DeviceCount = 0;

    /* Try to detect PC graphics (VESA/VGA) */
    Backend = FbCreateBackend(FbBackendPcGraphics);
    if (Backend != NULL) {
        Hr = IFramebufferBackend_GetDescriptor(Backend, &Descriptor);
        if (SUCCEEDED(Hr) && Descriptor.Width > 0) {
            FB_MANAGER_DEVICE *Device = &Manager->Devices[Manager->DeviceCount];
            Device->DeviceId = Manager->DeviceCount;
            Device->BackendType = FbBackendPcGraphics;
            ANX_MEMCPY(&Device->CurrentDescriptor, &Descriptor, sizeof(FRAMEBUFFER_DESC));
            Device->Screen = NULL;

            /* Fill device info */
            ANX_MEMSET(&Device->Info, 0, sizeof(FB_DEVICE_INFO));
            Device->Info.DeviceId = Device->DeviceId;
            ANX_STRCPY_S(Device->Info.DeviceName, 64, L"PC Graphics");
            ANX_STRCPY_S(Device->Info.VendorName, 64, L"Standard VGA/VESA");
            Device->Info.Capabilities = FbCapPalette;
            Device->Info.VideoMemorySize = Descriptor.Size;
            Device->Info.NumModes = 1;
            Device->Info.CurrentMode = 0;

            Manager->DeviceCount++;
        }
        IUnknown_Release((IUnknown *)Backend);
    }

    /* Try to detect UEFI GOP */
    Backend = FbCreateBackend(FbBackendUefiGop);
    if (Backend != NULL && Manager->DeviceCount < FB_MAX_DEVICES) {
        Hr = IFramebufferBackend_GetDescriptor(Backend, &Descriptor);
        if (SUCCEEDED(Hr) && Descriptor.Width > 0) {
            FB_MANAGER_DEVICE *Device = &Manager->Devices[Manager->DeviceCount];
            Device->DeviceId = Manager->DeviceCount;
            Device->BackendType = FbBackendUefiGop;
            ANX_MEMCPY(&Device->CurrentDescriptor, &Descriptor, sizeof(FRAMEBUFFER_DESC));
            Device->Screen = NULL;

            /* Fill device info */
            ANX_MEMSET(&Device->Info, 0, sizeof(FB_DEVICE_INFO));
            Device->Info.DeviceId = Device->DeviceId;
            ANX_STRCPY_S(Device->Info.DeviceName, 64, L"UEFI Graphics");
            ANX_STRCPY_S(Device->Info.VendorName, 64, L"UEFI GOP");
            Device->Info.Capabilities = FbCapHardwareBlit;
            Device->Info.VideoMemorySize = Descriptor.Size;
            Device->Info.NumModes = 1;
            Device->Info.CurrentMode = 0;

            Manager->DeviceCount++;
        }
        IUnknown_Release((IUnknown *)Backend);
    }

    /* Try generic backend as fallback */
    if (Manager->DeviceCount == 0) {
        Backend = FbCreateBackend(FbBackendGeneric);
        if (Backend != NULL) {
            /* Create a default 640x480 16-color mode */
            ANX_MEMSET(&Descriptor, 0, sizeof(FRAMEBUFFER_DESC));
            Descriptor.Width = 640;
            Descriptor.Height = 480;
            Descriptor.Pitch = 640;
            Descriptor.PixelFormat = FbPixelFormatRgb888;
            Descriptor.MemoryOrganization = FbMemoryLinear;
            Descriptor.BitsPerPixel = 24;
            Descriptor.Size = 640 * 480 * 3;
            Descriptor.IsAddressable = FALSE;

            Hr = IFramebufferBackend_Initialize(Backend, &Descriptor);
            if (SUCCEEDED(Hr)) {
                FB_MANAGER_DEVICE *Device = &Manager->Devices[Manager->DeviceCount];
                Device->DeviceId = Manager->DeviceCount;
                Device->BackendType = FbBackendGeneric;
                ANX_MEMCPY(&Device->CurrentDescriptor, &Descriptor, sizeof(FRAMEBUFFER_DESC));
                Device->Screen = NULL;

                /* Fill device info */
                ANX_MEMSET(&Device->Info, 0, sizeof(FB_DEVICE_INFO));
                Device->Info.DeviceId = Device->DeviceId;
                ANX_STRCPY_S(Device->Info.DeviceName, 64, L"Generic");
                ANX_STRCPY_S(Device->Info.VendorName, 64, L"Software");
                Device->Info.Capabilities = FbCapNone;
                Device->Info.VideoMemorySize = Descriptor.Size;
                Device->Info.NumModes = 1;
                Device->Info.CurrentMode = 0;

                Manager->DeviceCount++;
            }
            IUnknown_Release((IUnknown *)Backend);
        }
    }
}

static FB_MANAGER_DEVICE *
FbManager_FindDevice(
    FB_MANAGER_IMPL *Manager,
    UINT32 DeviceId
    )
{
    for (UINT32 i = 0; i < Manager->DeviceCount; i++) {
        if (Manager->Devices[i].DeviceId == DeviceId) {
            return &Manager->Devices[i];
        }
    }
    return NULL;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

/*
 * Create the global framebuffer manager.
 */
IFramebufferManager *
FbCreateManager(
    VOID
    )
{
    FB_MANAGER_IMPL *Manager;

    /* Allocate manager object */
    Manager = (FB_MANAGER_IMPL *)ANX_MALLOC(sizeof(FB_MANAGER_IMPL));
    if (Manager == NULL) {
        return NULL;
    }

    /* Initialize */
    ANX_MEMSET(Manager, 0, sizeof(FB_MANAGER_IMPL));
    Manager->Base.lpVtbl = &gManagerVtbl;
    Manager->RefCount.RefCount = 1;

    /* Enumerate available devices */
    FbManager_EnumerateDevices(Manager);

    return &Manager->Base;
}
