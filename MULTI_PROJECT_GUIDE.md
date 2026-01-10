# NUX Suite Multi-Project Structure

## Overview

This repository contains **three independent projects** with clear dependency relationships:

```
┌──────────┐
│  Ananke  │  ← Foundation library (no dependencies)
└────┬─────┘
     │
     ├─────────┐
     │         │
┌────▼────┐ ┌─▼─────┐
│  APXH   │ │  NUX  │  ← Both depend on Ananke
└─────────┘ └───────┘
```

### 1. **Ananke** - Foundation Library
- **Purpose**: Portable, compiler-agnostic foundation layer
- **Dependencies**: None (standalone)
- **Provides**:
  - UEFI-width base types (UINT8, UINT16, UINT32, UINT64, UINTN, INTN)
  - COM/IUnknown ABI (HRESULT, GUID, interface definitions)
  - Atomic operations
  - Compiler attributes
  - Calling conventions
  - Platform detection
  - NT Runtime Library (NTRTL) - optional compiled components

### 2. **APXH** - Bootloader
- **Purpose**: Boot protocol handler for NUX kernel
- **Dependencies**: Ananke
- **Provides**:
  - Multiboot support (i386, amd64)
  - UEFI support (amd64, riscv64)
  - SBI support (riscv64)
  - ELF/PE/Mach-O loader

### 3. **NUX** - Kernel Framework
- **Purpose**: Kernel framework for modern hardware
- **Dependencies**: Ananke
- **Provides**:
  - Hardware Abstraction Layer (HAL)
  - Platform Layer (PLT)
  - Kernel libraries (libnux)
  - Embedded C runtime (libec)
  - Example kernel

## Building

### Option 1: Build All Projects (Recommended)

Build everything in one command, respecting dependencies:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

This builds in order: Ananke → APXH → NUX

### Option 2: Build Individual Projects

Each project can be built independently:

#### Build Only Ananke

```bash
cd ananke
cmake -B build
cmake --build build
```

Or from root:

```bash
cmake -B build -DBUILD_APXH=OFF -DBUILD_NUX=OFF
cmake --build build
```

#### Build Only APXH (requires Ananke)

```bash
# First build Ananke
cd ananke && cmake -B build && cmake --build build && cd ..

# Then build APXH
cd apxh
cmake -B build
cmake --build build
```

Or from root:

```bash
cmake -B build -DBUILD_NUX=OFF
cmake --build build
```

#### Build Only NUX (requires Ananke)

```bash
# First build Ananke
cd ananke && cmake -B build && cmake --build build && cd ..

# Then build NUX
cd nux
cmake -B build
cmake --build build
```

Or from root:

```bash
cmake -B build -DBUILD_APXH=OFF
cmake --build build
```

### Option 3: Build with Custom Configuration

Specify targets, image format, and compiler:

```bash
mkdir build && cd build

# Universal binary for x86, using Clang
cmake .. \
  -DCMAKE_C_COMPILER=clang \
  -DNUX_TARGETS="i386;amd64" \
  -DNUX_IMAGE_FORMAT=elf

# Single target for RISC-V with GCC
cmake .. \
  -DCMAKE_C_COMPILER=riscv64-unknown-elf-gcc \
  -DNUX_TARGETS="riscv64" \
  -DNUX_IMAGE_FORMAT=elf

# Windows PE/COFF with MSVC
cmake .. \
  -G "Visual Studio 16 2019" \
  -A x64 \
  -DNUX_TARGETS="amd64" \
  -DNUX_IMAGE_FORMAT=pecoff

cmake --build .
```

## Project Structure

```
nux/
├── ananke/                    # Project 1: Foundation Library
│   ├── CMakeLists.txt         # Ananke build system
│   ├── AnankeConfig.cmake.in  # CMake package config
│   ├── include/
│   │   └── ananke/
│   │       ├── ananke.h       # Main header
│   │       ├── types.h        # UEFI-style types
│   │       ├── hresult.h      # COM HRESULT
│   │       ├── guid.h         # GUID definitions
│   │       └── ntrtl/         # NT Runtime Library
│   ├── sources/
│   │   └── ntrtl/             # NTRTL implementation
│   └── README.md              # Ananke documentation
│
├── apxh/                      # Project 2: Bootloader
│   ├── CMakeLists.txt         # APXH build system
│   ├── include/
│   ├── sources/
│   │   ├── common/            # Common bootloader code
│   │   ├── loader/            # Image loaders
│   │   └── platform/          # Platform implementations
│   │       ├── multiboot/     # Multiboot support
│   │       ├── uefi/          # UEFI support
│   │       └── sbi/           # SBI support (RISC-V)
│   └── README.md
│
├── nux/                       # Project 3: Kernel Framework
│   ├── CMakeLists.txt         # NUX build system
│   ├── libs/
│   │   ├── ecrt/              # Embedded C runtime
│   │   ├── hal/               # Hardware Abstraction Layer
│   │   ├── nux/               # Core kernel library
│   │   ├── platform/          # Platform layer
│   │   └── unux/              # Userspace library
│   ├── example/               # Example kernel
│   ├── tools/                 # Build tools
│   │   └── objlayout/         # Object layout tool
│   └── README.md
│
├── CMakeLists.txt             # Root coordinator (builds all)
├── MULTI_PROJECT_GUIDE.md     # This file
├── CMAKE_BUILD_SYSTEM.md      # Detailed build system docs
└── cmake/                     # Shared CMake modules
    └── modules/
        ├── NuxCompilerSetup.cmake
        ├── NuxToolchainAbstraction.cmake
        └── NuxSectionGeneration.cmake
```

## Dependency Management

### Using find_package()

If Ananke is installed system-wide, APXH and NUX can find it automatically:

```bash
# Install Ananke
cd ananke
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build

# Now APXH can find it
cd ../apxh
cmake -B build  # Automatically finds installed Ananke
cmake --build build

# Same for NUX
cd ../nux
cmake -B build  # Automatically finds installed Ananke
cmake --build build
```

### Using add_subdirectory()

The root CMakeLists.txt uses `add_subdirectory()` to build all projects together, which is the recommended approach for development.

### Manual Configuration

If Ananke is in a custom location:

```bash
cd apxh
cmake -B build -DAnanke_DIR=/path/to/ananke/build
cmake --build build
```

## CMake Options

### Global Options (Root Level)

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_ANANKE` | ON | Build Ananke foundation library |
| `BUILD_APXH` | ON | Build APXH bootloader |
| `BUILD_NUX` | ON | Build NUX kernel framework |
| `NUX_TARGETS` | i386 | Target architectures (semicolon-separated) |
| `NUX_IMAGE_FORMAT` | elf | Image format: elf, macho, or pecoff |
| `NUX_USE_LINKER_SCRIPTS` | OFF | Use GNU LD linker scripts |
| `NUX_ENABLE_TESTS` | OFF | Enable testing |

### Ananke Options

| Option | Default | Description |
|--------|---------|-------------|
| `ANANKE_BUILD_SHARED` | OFF | Build shared library |
| `ANANKE_BUILD_STATIC` | ON | Build static library |
| `ANANKE_BUILD_NTRTL` | ON | Build NT Runtime Library |
| `ANANKE_INSTALL` | ON | Install headers and library |

### APXH Options

| Option | Default | Description |
|--------|---------|-------------|
| `APXH_BUILD_MULTIBOOT` | ON | Build Multiboot platform |
| `APXH_BUILD_UEFI` | ON | Build UEFI platform |
| `APXH_BUILD_SBI` | ON | Build SBI platform (RISC-V) |
| `APXH_TARGETS` | (inherited) | Target architectures |
| `APXH_IMAGE_FORMAT` | (inherited) | Image format |

### NUX Options

| Option | Default | Description |
|--------|---------|-------------|
| `NUX_BUILD_LIBS` | ON | Build NUX libraries |
| `NUX_BUILD_HAL` | ON | Build Hardware Abstraction Layer |
| `NUX_BUILD_PLATFORM` | ON | Build Platform Layer |
| `NUX_BUILD_ECRT` | ON | Build embedded C runtime |
| `NUX_BUILD_EXAMPLE` | ON | Build example kernel |
| `NUX_BUILD_TOOLS` | ON | Build NUX tools |

## Usage Examples

### Example 1: Development Build (All Projects)

```bash
mkdir build-dev && cd build-dev
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
```

### Example 2: Production Build (Specific Targets)

```bash
mkdir build-prod && cd build-prod
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DNUX_TARGETS="amd64" \
  -DNUX_IMAGE_FORMAT=elf \
  -DNUX_USE_LINKER_SCRIPTS=OFF
cmake --build . -j$(nproc)
```

### Example 3: Universal Binary (macOS)

```bash
mkdir build-macos && cd build-macos
cmake .. \
  -DCMAKE_C_COMPILER=clang \
  -DNUX_TARGETS="x86_64;arm64" \
  -DNUX_IMAGE_FORMAT=macho
cmake --build . -j$(sysctl -n hw.ncpu)
```

### Example 4: Windows Build (MSVC)

```bash
mkdir build-windows && cd build-windows
cmake .. \
  -G "Visual Studio 16 2019" \
  -A x64 \
  -DNUX_TARGETS="amd64" \
  -DNUX_IMAGE_FORMAT=pecoff
cmake --build . --config Release
```

### Example 5: Ananke Only (For Use in Other Projects)

```bash
cd ananke
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DANANKE_BUILD_SHARED=ON \
  -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

Now other projects can use:

```cmake
find_package(Ananke REQUIRED)
target_link_libraries(my_target PRIVATE Ananke::Headers)
# or
target_link_libraries(my_target PRIVATE Ananke::NTRTL)
```

## Integration with External Projects

### Using Ananke in Your Project

After installing Ananke or building it:

```cmake
# In your CMakeLists.txt
find_package(Ananke REQUIRED)

add_executable(my_program main.c)
target_link_libraries(my_program PRIVATE Ananke::Headers)

# Or use NTRTL
target_link_libraries(my_program PRIVATE Ananke::NTRTL)
```

In your code:

```c
#include <ananke/ananke.h>

UINT32 MyFunction(VOID) {
    UINTN value = sizeof(UINTN);
    return S_OK;
}
```

## Dependency Graph

```
External Projects
       │
       └─── find_package(Ananke)
               │
               │
       ┌───────┴───────┐
       │               │
    APXH             NUX
       │               │
       │               ├─── HAL (Hardware Abstraction)
       │               ├─── PLT (Platform Layer)
       │               ├─── ECRT (C Runtime)
       │               └─── Example Kernel
       │
       └─── Multiboot/UEFI/SBI
```

## Migration from Monolithic Build

If you were using the old single-project approach:

**Old Way (Autoconf)**:
```bash
./bootstrap.sh
./configure --enable-targets=i386
make -j
```

**New Way (CMake Multi-Project)**:
```bash
cmake -B build -DNUX_TARGETS=i386
cmake --build build -j
```

The new structure provides:
- ✅ Clear separation of concerns
- ✅ Independent versioning
- ✅ Reusable foundation (Ananke)
- ✅ Flexible builds (build what you need)
- ✅ Better dependency management
- ✅ Easier integration with external projects

## See Also

- [CMAKE_BUILD_SYSTEM.md](CMAKE_BUILD_SYSTEM.md) - Comprehensive build system documentation
- [ananke/README.md](ananke/README.md) - Ananke foundation library details
- [apxh/README.md](apxh/README.md) - APXH bootloader documentation
- [nux/README.md](nux/README.md) - NUX kernel framework documentation
