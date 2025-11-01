# GLESv20 Integration with Ananke Framebuffer

This directory contains Vincent ES 2.0, a software OpenGL ES 2.0 renderer, integrated with the Ananke framebuffer library.

## Overview

Vincent ES 2.0 (GLESv20) is a complete software implementation of OpenGL ES 2.0 with the following features:
- Full programmable pipeline (vertex and fragment shaders)
- GLSL ES shader compiler
- Software rasterizer
- No hardware dependencies

The Ananke integration provides a framebuffer backend that allows Vincent to render directly to any framebuffer managed by the Ananke framebuffer library.

## Features

### Core OpenGL ES 2.0 Support
- Programmable vertex and fragment shaders
- GLSL ES 1.0 shader language
- Texturing (2D, cubemap)
- Blending, alpha testing
- Depth and stencil buffers
- Vertex buffer objects (VBO)
- Frame buffer objects (FBO)

### Ananke Integration
- Direct framebuffer rendering
- Support for all Ananke pixel formats (RGB888, RGB565, RGB555)
- Works with all Ananke backends (UEFI, VESA, VGA, Hercules, Apple EFI)
- Zero-copy rendering when possible
- COM-based interface

## Building

GLESv20 is automatically built as part of the Ananke library:

```bash
cd ananke
make
```

The library will be compiled into `libananke.a`.

## Usage Example

```c
#include <ananke/framebuffer.h>
#include <ananke/framebuffer/backends.h>
#include <GLES/gl.h>
#include <vin.h>

/* Create framebuffer backend */
FRAMEBUFFER_DESC desc = {
    .PixelFormat = FbPixelFormatRgb888,
    .Width = 1024,
    .Height = 768,
    .Pitch = 1024 * 4,
    .PhysicalBase = 0xE0000000,
    .Size = 1024 * 768 * 4,
};

IFramebufferBackend *backend = FbCreateGenericBackend();
IFramebufferBackend_Initialize(backend, &desc);

/* Initialize Vincent ES 2.0 */
vinInitialize();

/* Create OpenGL ES surface */
VinSurface surface = vinCreateFramebufferSurface(
    backend,
    GL_DEPTH_COMPONENT16,  /* 16-bit depth buffer */
    GL_STENCIL_INDEX8_OES  /* 8-bit stencil buffer */
);

vinMakeCurrent(surface);

/* Now you can use standard OpenGL ES 2.0 calls */
glClearColor(0.0f, 0.0f, 0.5f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

glViewport(0, 0, 1024, 768);

/* Load and compile shaders */
const char *vertexShader =
    "attribute vec4 position;\n"
    "void main() {\n"
    "    gl_Position = position;\n"
    "}\n";

const char *fragmentShader =
    "void main() {\n"
    "    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";

GLuint vs = glCreateShader(GL_VERTEX_SHADER);
glShaderSource(vs, 1, &vertexShader, NULL);
glCompileShader(vs);

GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
glShaderSource(fs, 1, &fragmentShader, NULL);
glCompileShader(fs);

GLuint program = glCreateProgram();
glAttachShader(program, vs);
glAttachShader(program, fs);
glLinkProgram(program);
glUseProgram(program);

/* Draw a triangle */
GLfloat vertices[] = {
     0.0f,  0.5f,
    -0.5f, -0.5f,
     0.5f, -0.5f
};

GLuint vbo;
glGenBuffers(1, &vbo);
glBindBuffer(GL_ARRAY_BUFFER, vbo);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

GLint posAttrib = glGetAttribLocation(program, "position");
glEnableVertexAttribArray(posAttrib);
glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

glDrawArrays(GL_TRIANGLES, 0, 3);

/* Cleanup */
vinDestroySurface(surface);
vinTerminate();
IUnknown_Release((IUnknown *)backend);
```

## API Reference

### vinInitialize()
Initialize the Vincent ES 2.0 renderer. Must be called before any other Vincent functions.

### vinTerminate()
Shut down the Vincent ES 2.0 renderer and free all resources.

### vinCreateFramebufferSurface()
Create a rendering surface backed by an Ananke framebuffer backend.

Parameters:
- `backend`: IFramebufferBackend instance
- `depthFormat`: GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24, or GL_DEPTH_COMPONENT32
- `stencilFormat`: GL_STENCIL_INDEX1_OES, GL_STENCIL_INDEX4_OES, GL_STENCIL_INDEX8_OES, or 0 for no stencil

Returns: VinSurface handle or NULL on failure

### vinMakeCurrent()
Make a surface current for rendering. All subsequent OpenGL ES calls will render to this surface.

### vinDestroySurface()
Destroy a surface and free its resources.

## Pixel Format Support

| Framebuffer Format | OpenGL ES Format | Notes |
|-------------------|------------------|-------|
| FbPixelFormatRgb888 | GL_RGBA (8:8:8:0) | Best performance |
| FbPixelFormatRgb565 | GL_RGB (5:6:5) | Good for 16-bit displays |
| FbPixelFormatRgb555 | GL_RGBA (5:5:5:1) | Legacy support |

## Performance Considerations

Vincent ES 2.0 is a software renderer, so performance is CPU-bound:

1. **Shader Complexity**: Keep shaders simple for better performance
2. **Draw Calls**: Minimize draw calls by batching geometry
3. **Texture Size**: Use smaller textures when possible
4. **Buffer Usage**: Use VBOs for frequently drawn geometry
5. **Pixel Format**: RGB565 is faster than RGB888 on lower-end hardware

## Limitations

- Software rendering only (no hardware acceleration)
- Performance depends entirely on CPU speed
- Limited to OpenGL ES 2.0 feature set
- No threading/multi-core support in this version

## License

Vincent ES 2.0 is licensed under the Common Development and Distribution License (CDDL).
See LICENSE.txt for details.

Original project: https://github.com/hmwill/GLESv20

## Directory Structure

```
gles20/
├── egl/
│   ├── framebuffer.c       # Ananke framebuffer backend
│   └── sdl.c               # Original SDL backend (reference)
├── gl/
│   ├── frontend/           # OpenGL ES API implementation
│   ├── backend/            # JIT compiler
│   ├── raster/             # Software rasterizer
│   ├── platform/           # Platform abstraction
│   └── gl/                 # Core GL state management
├── public/
│   ├── GLES/
│   │   └── gl.h            # OpenGL ES 2.0 header
│   └── vin.h               # Vincent/Ananke integration API
├── utils/                  # Utility functions
├── tests/                  # Test programs
├── LICENSE.txt
├── README.md               # Original Vincent README
└── README_ANANKE.md        # This file
```
