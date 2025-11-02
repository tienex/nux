# Direct3D 1-9 Architecture

## Overview

This document describes the architecture for implementing Direct3D versions 1 through 9 on top of OpenGL ES 2.0 (GLES20) backend.

## Directory Structure

```
ananke/libs/
├── d3d9/          # Direct3D 9.0c (Shader Model 3.0) - COMPLETE
├── d3d8/          # Direct3D 8.0/8.1 (Shader Model 1.0-1.4)
├── d3d7/          # Direct3D 7.0 (Hardware T&L, fixed-function)
├── d3d6/          # Direct3D 6.0 (Multitexture support)
├── d3d5/          # Direct3D 5.0 (DrawPrimitive API)
└── d3d3/          # Direct3D 1-3 (Immediate mode, execute buffers)
```

## Version Feature Matrix

| Version | Year | Key Features | Shader Support | Implementation Priority |
|---------|------|--------------|----------------|------------------------|
| D3D9    | 2002 | SM 3.0, Full programmable | Vertex/Pixel SM 3.0 | ✅ COMPLETE |
| D3D8    | 2000 | SM 1.x, First shaders | Vertex/Pixel SM 1.0-1.4 | 🔴 HIGH |
| D3D7    | 1999 | Hardware T&L, Clean API | Fixed-function only | 🔴 HIGH |
| D3D6    | 1998 | Multitexture | Fixed-function only | 🟡 MEDIUM |
| D3D5    | 1997 | DrawPrimitive | Fixed-function only | 🟡 MEDIUM |
| D3D3    | 1996 | Immediate mode | Fixed-function only | 🟢 LOW |
| D3D1-2  | 1995 | Execute buffers (obsolete) | None | 🟢 LOW |

## Technical Architecture

### 1. Common Backend (GLES20COM)

All Direct3D versions share the same GLES20 COM wrapper:
- IGLDevice, IGLContext, IGLBuffer, IGLTexture
- IGLShader, IGLProgram (for shader-based versions)
- Located in: `libs/d3d9/sources/gles20com/`

### 2. Fixed-Function Pipeline Emulation

D3D3-D3D7 require fixed-function pipeline emulation via generated shaders:

**Vertex Shader Generation:**
- Lighting calculations (up to 8 lights)
- Fog calculations (linear, exp, exp2)
- Texture coordinate generation
- Vertex blending (skinning)

**Pixel Shader Generation:**
- Texture stage operations (up to 8 stages)
- Texture blending modes
- Alpha testing
- Fog application

**Shared Components:**
```
libs/d3d_common/
├── ffp_generator.c      # Fixed-function pipeline shader generator
├── light_manager.c      # Lighting state management
├── texture_stages.c     # Texture stage state management
└── matrix_stack.c       # Matrix stack implementation
```

### 3. Version-Specific Implementations

#### Direct3D 9 (COMPLETE)
- Full Shader Model 3.0 support
- Vertex/pixel shader compilation
- Render states, texture states, sampler states
- Vertex declarations

#### Direct3D 8
**Features:**
- Shader Model 1.0-1.4 support
- Vertex shaders (up to 96 instructions)
- Pixel shaders (up to 8 texture + 8 arithmetic)
- FVF (Flexible Vertex Format) descriptors
- Fixed-function and programmable pipeline mixing

**Implementation:**
```
libs/d3d8/
├── include/ananke/d3d8.h
├── sources/
│   ├── d3d8main.c           # IDirect3D8, IDirect3DDevice8
│   ├── resources.c          # Buffers, textures, surfaces
│   ├── shader_sm1.c         # Shader Model 1.x translator
│   ├── fvf.c                # FVF to vertex declaration converter
│   └── statemanager.c       # State management
```

#### Direct3D 7
**Features:**
- Hardware T&L (Transform & Lighting)
- 8 texture stages
- Fixed-function lighting (up to 8 lights)
- Matrix stacks (world, view, projection)
- No COM interfaces (uses plain structs)

**Implementation:**
```
libs/d3d7/
├── include/ananke/d3d7.h
├── sources/
│   ├── d3d7main.c           # DirectDraw7/Direct3D7 integration
│   ├── device.c             # IDirect3DDevice7
│   ├── lighting.c           # Light and material management
│   ├── matrix.c             # Transform matrix management
│   ├── ffp_vertex.c         # Fixed-function vertex processing
│   └── ffp_pixel.c          # Fixed-function pixel processing
```

#### Direct3D 6
**Features:**
- Multitexture support (2-4 stages)
- Texture coordinate sets
- Still COM-based like D3D5

**Implementation:**
```
libs/d3d6/
├── include/ananke/d3d6.h
├── sources/
│   ├── d3d6main.c           # IDirect3D3, IDirect3DDevice3
│   ├── multitexture.c       # Multitexture stage management
│   └── ffp_multitex.c       # Multitexture shader generation
```

#### Direct3D 5
**Features:**
- DrawPrimitive API (first modern API)
- Single texture support
- Execute buffers deprecated

**Implementation:**
```
libs/d3d5/
├── include/ananke/d3d5.h
├── sources/
│   ├── d3d5main.c           # IDirect3D2, IDirect3DDevice2
│   ├── drawprim.c           # DrawPrimitive implementation
│   └── ffp_basic.c          # Basic fixed-function shaders
```

#### Direct3D 3 (and 1-2)
**Features:**
- Immediate mode rendering
- Execute buffers (D3D1-2)
- IDirect3D, IDirect3DDevice, IDirect3DExecuteBuffer

**Implementation:**
```
libs/d3d3/
├── include/ananke/d3d3.h
├── sources/
│   ├── d3d3main.c           # IDirect3D, IDirect3DDevice
│   ├── immediate.c          # Immediate mode rendering
│   ├── execute.c            # Execute buffer emulation
│   └── vertex.c             # Vertex processing
```

## Fixed-Function Pipeline State

All versions manage these states:
- **Transform States**: World, View, Projection matrices
- **Lighting States**: Ambient, diffuse, specular, emissive
- **Material States**: Material properties
- **Texture States**: Texture operations, arguments, blend modes
- **Render States**: Alpha blend, depth test, culling, fog

## Shader Generation Strategy

### Vertex Shader Template
```glsl
// Generated based on enabled states
uniform mat4 u_worldMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
uniform vec4 u_lights[8];         // Light positions/directions
uniform vec4 u_lightColors[8];    // Light colors
uniform vec4 u_materialAmbient;
uniform vec4 u_materialDiffuse;

attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_texcoord0;

varying vec4 v_color;
varying vec2 v_texcoord0;

void main() {
    // Transform
    vec4 worldPos = u_worldMatrix * vec4(a_position, 1.0);
    vec4 viewPos = u_viewMatrix * worldPos;
    gl_Position = u_projMatrix * viewPos;

    // Lighting calculations
    vec3 normal = normalize((u_worldMatrix * vec4(a_normal, 0.0)).xyz);
    v_color = calculateLighting(worldPos.xyz, normal);

    v_texcoord0 = a_texcoord0;
}
```

### Pixel Shader Template
```glsl
// Generated based on texture stages
varying vec4 v_color;
varying vec2 v_texcoord0;

uniform sampler2D u_texture0;

void main() {
    vec4 color = v_color;
    vec4 texColor = texture2D(u_texture0, v_texcoord0);

    // Texture stage operations
    color = applyTextureStages(color, texColor);

    gl_FragColor = color;
}
```

## Build Integration

Each library is built as part of libananke.a:

```makefile
# Makefile.in
SRCS += libs/d3d9/sources/**/*.c \
        libs/d3d8/sources/**/*.c \
        libs/d3d7/sources/**/*.c \
        libs/d3d6/sources/**/*.c \
        libs/d3d5/sources/**/*.c \
        libs/d3d3/sources/**/*.c \
        libs/d3d_common/sources/**/*.c
```

## API Compatibility

Each version maintains Windows API compatibility:
- Uses exact Windows type definitions (DWORD, FLOAT, etc.)
- Maintains COM interface layouts
- Preserves calling conventions (STDMETHODCALLTYPE)
- Ensures binary compatibility with Windows applications

## Testing Strategy

Each version includes:
1. **Simple triangle test** - Basic rendering
2. **Textured quad test** - Texture mapping
3. **Lighting test** - Material and light interaction (D3D3-7)
4. **Shader test** - Programmable pipeline (D3D8-9)
5. **State change test** - Performance and correctness

## Implementation Phases

### Phase 1: Core Fixed-Function Support ✅
- [x] D3D9 implementation (COMPLETE)
- [ ] Shared FFP shader generator
- [ ] D3D7 implementation (highest priority)

### Phase 2: Shader Model 1.x
- [ ] D3D8 implementation
- [ ] SM 1.x shader translator

### Phase 3: Legacy Multitexture
- [ ] D3D6 implementation
- [ ] Multitexture stage manager

### Phase 4: Early APIs
- [ ] D3D5 implementation
- [ ] D3D3 implementation
- [ ] Execute buffer emulation

## Performance Considerations

1. **Shader Caching**: Generated FFP shaders are cached by state combination
2. **State Batching**: Minimize GL state changes
3. **Buffer Management**: Efficient vertex/index buffer updates
4. **Draw Call Batching**: Combine compatible draw calls

## Future Enhancements

1. **State optimization**: Detect and eliminate redundant state changes
2. **Shader optimization**: Optimize generated shader code
3. **Hardware capability detection**: Adapt to GL capabilities
4. **Debug layer**: Validation and error reporting
