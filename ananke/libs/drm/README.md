# ANANKE DRM Library

Direct Rendering Manager abstraction layer for bare-metal/UEFI environments.

## Overview

This library provides a Linux DRM-compatible API for mode setting, display management, and framebuffer allocation in bare-metal environments where no kernel DRM subsystem exists.

## Features

- **Mode Setting**: Configure display resolution, refresh rate, and pixel format
- **Connector Management**: Enumerate and configure display outputs (VGA, HDMI, DisplayPort, etc.)
- **Framebuffer Management**: Create, destroy, and display framebuffers
- **CRTC Control**: Manage display controllers and scanout
- **VBlank Synchronization**: Wait for vertical blank for tear-free rendering
- **COM-based Interface**: Follows ananke's COM architecture for clean abstraction

## Supported Connectors

- VGA
- DVI
- HDMI
- DisplayPort
- eDP (Embedded DisplayPort)
- LVDS

## API Example

```c
#include <ananke/drm.h>

/* Create DRM device */
IDrmDevice *drm = DrmCreateDevice();
IDrmDevice_Initialize(drm);

/* Get connectors */
UINT32 numConnectors;
IDrmDevice_GetConnectorCount(drm, &numConnectors);

DRM_CONNECTOR_INFO info;
IDrmDevice_GetConnectorInfo(drm, 0, &info);

/* Get supported modes */
DRM_MODE modes[32];
UINT32 numModes;
IDrmDevice_GetConnectorModes(drm, 0, modes, 32, &numModes);

/* Set mode */
IDrmDevice_SetMode(drm, 0, &modes[0]);

/* Create framebuffer */
DRM_FRAMEBUFFER fb;
IDrmDevice_CreateFramebuffer(drm, 1920, 1080, 32, &fb);

/* Display framebuffer */
IDrmDevice_DisplayFramebuffer(drm, 0, fb.FbId);

/* Wait for VBlank */
IDrmDevice_WaitVBlank(drm, 0);

/* Cleanup */
IDrmDevice_DestroyFramebuffer(drm, fb.FbId);
IUnknown_Release((IUnknown*)drm);
```

## Integration

The DRM library works with:
- **Framebuffer Library**: Uses `IFramebufferBackend` for rendering
- **UEFI GOP**: Can be backed by Graphics Output Protocol
- **KMS Library**: Provides higher-level KMS emulation on top of DRM

## Factory Functions

```c
/* Create generic DRM device */
IDrmDevice* DrmCreateDevice(VOID);

/* Create from UEFI GOP protocol */
IDrmDevice* DrmCreateUefiGopDevice(VOID* GopProtocol);

/* Create from existing framebuffer backend */
IDrmDevice* DrmCreateFromFramebuffer(IFramebufferBackend* Backend);
```

## Building

Built as part of libananke.a:

```bash
cd ananke
make
```

## Architecture

The DRM library provides a minimal implementation suitable for bare-metal:
- Single CRTC per connector (simplified)
- Software-based mode setting
- Framebuffer management without GPU acceleration
- Compatible with Linux DRM concepts for portability

## License

BSD-2-Clause

## Copyright

Copyright (C) 2025 ANANKE Project
