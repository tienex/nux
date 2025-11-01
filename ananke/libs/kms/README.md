# ANANKE KMS Emulation Library

Kernel Mode Setting emulation for bare-metal environments with GLESv20 integration.

## Overview

This library emulates the Linux KMS API, providing plane management, atomic mode setting, and GLESv20-accelerated rendering for bare-metal/UEFI environments.

## Features

- **KMS Plane Management**: Primary, overlay, and cursor planes
- **Atomic Mode Setting**: Modern KMS atomic API
- **Page Flipping**: Double-buffered rendering with VSync
- **GLESv20 Integration**: Hardware-accelerated 3D rendering via Vincent ES 2.0
- **DRM Backend**: Built on top of DRM library
- **COM-based Interface**: Clean, modern API design

## Components

### IKmsDevice
Core KMS functionality:
- Plane management
- Framebuffer operations
- Atomic commits
- Page flipping

### IKmsRenderer
GLESv20 rendering integration:
- Double-buffered rendering
- OpenGL ES 2.0 context
- Frame synchronization
- Automatic buffer swapping

## Usage Example

### Basic KMS Setup

```c
#include <ananke/kms.h>
#include <ananke/drm.h>

/* Create DRM device */
IDrmDevice *drm = DrmCreateDevice();
IDrmDevice_Initialize(drm);

/* Create KMS device */
IKmsDevice *kms = KmsCreateDevice();
IKmsDevice_Initialize(kms, drm);

/* Get plane information */
UINT32 numPlanes;
IKmsDevice_GetPlaneCount(kms, &numPlanes);

KMS_PLANE_DESC planeDesc;
IKmsDevice_GetPlaneDesc(kms, 0, &planeDesc);

/* Create framebuffer */
UINT32 fbId;
IKmsDevice_AddFramebuffer(kms, 1920, 1080, 1920*4, 32, 0, &fbId);

/* Set plane (display framebuffer) */
IKmsDevice_SetPlane(kms, 0, 0, fbId, 0,
                    0, 0, 1920, 1080,
                    0, 0, 1920, 1080);

/* Page flip */
IKmsDevice_PageFlip(kms, 0, fbId, 0);

/* Cleanup */
IUnknown_Release((IUnknown*)kms);
IUnknown_Release((IUnknown*)drm);
```

### GLESv20 Rendering

```c
#include <ananke/kms.h>
#include <GLES/gl.h>

/* Create KMS renderer */
IKmsRenderer *renderer = KmsCreateRenderer();
IKmsRenderer_Initialize(renderer, kms, 1920, 1080);

/* Get GL surface */
void *glSurface;
IKmsRenderer_GetGLSurface(renderer, &glSurface);

/* Rendering loop */
while (running) {
    /* Begin frame */
    IKmsRenderer_BeginFrame(renderer);

    /* OpenGL ES 2.0 rendering */
    glClearColor(0.0f, 0.0f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, 1920, 1080);

    /* Draw your scene here */
    // glUseProgram(program);
    // glDrawArrays(GL_TRIANGLES, 0, 3);

    /* End frame */
    IKmsRenderer_EndFrame(renderer);

    /* Swap buffers (presents to screen) */
    IKmsRenderer_SwapBuffers(renderer);
}

/* Cleanup */
IUnknown_Release((IUnknown*)renderer);
```

### Atomic Mode Setting

```c
/* Prepare atomic state */
KMS_ATOMIC_STATE state;
state.CrtcId = 0;
state.ConnectorId = 0;
state.FbId = fbId;
state.PlaneId = 0;
state.SrcX = 0;
state.SrcY = 0;
state.SrcW = 1920;
state.SrcH = 1080;
state.CrtcX = 0;
state.CrtcY = 0;
state.CrtcW = 1920;
state.CrtcH = 1080;
state.Flags = 0;

/* Atomic commit */
IKmsDevice_AtomicCommit(kms, &state, 1, KMS_ATOMIC_ALLOW_MODESET);
```

## Integration

Works with:
- **DRM Library**: Provides underlying display management
- **GLESv20**: Vincent ES 2.0 for 3D rendering
- **Framebuffer Library**: For 2D drawing operations

## Plane Types

- **KmsPlaneTypePrimary**: Main display plane (scanout)
- **KmsPlaneTypeOverlay**: Overlay planes (picture-in-picture, OSD)
- **KmsPlaneTypeCursor**: Hardware cursor plane

## Factory Functions

```c
/* Create KMS device */
IKmsDevice* KmsCreateDevice(VOID);

/* Create KMS renderer with GLESv20 */
IKmsRenderer* KmsCreateRenderer(VOID);
```

## Building

Built as part of libananke.a:

```bash
cd ananke
make
```

## Performance

- **Double Buffering**: Eliminates tearing
- **VSync**: Synchronized with display refresh
- **GLESv20 JIT**: Native code generation via sljit for maximum performance
- **Zero-Copy**: Direct rendering to framebuffer when possible

## Architecture Compatibility

Supports all architectures via GLESv20:
- i386
- x86-64 / x32
- RISC-V 32/64
- ARM 32/64
- Generic fallback for others

## License

BSD-2-Clause

## Copyright

Copyright (C) 2025 ANANKE Project
