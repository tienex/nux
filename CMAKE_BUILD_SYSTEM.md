# NUX CMake Build System

## Overview

The NUX project has been revamped with a comprehensive CMake-based build system that supports an extensive range of compilers and toolchains, eliminating hard dependencies on GNU-specific linker scripts.

## Supported Compilers

The build system automatically detects and supports the following compilers:

### Tier 1 (Fully Tested)
- **GCC** (GNU Compiler Collection) - All versions 7.0+
- **Clang/LLVM** - All versions 10.0+
- **Intel C++ Compiler** (ICC) - Classic and LLVM-based (ICX)
- **MSVC** (Microsoft Visual C++) - 2017 and later

### Tier 2 (Supported with Limitations)
- **Sun Studio** (Oracle Developer Studio) - Solaris/SPARC
- **HP-UX CC** - HP-UX compiler
- **IBM XL C/C++** (xlC) - AIX and Linux on POWER
- **Open Watcom** - Cross-platform C/C++ compiler
- **Open64** - Open source compiler for various architectures

### Tier 3 (Basic Support)
- **Tiny C Compiler** (TCC) - Fast, lightweight compiler
- **Portable C Compiler** (PCC) - BSD-licensed compiler
- **LCC** - Retargetable C compiler

## Quick Start

### Prerequisites

- CMake 3.15 or later
- A supported compiler (see above)
- Make or Ninja build system

### Basic Build

```bash
# Create build directory
mkdir build && cd build

# Configure with default settings (single target, ELF format, GCC)
cmake ..

# Build
cmake --build .
```

### Specifying Compiler

```bash
# Use Clang
cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

# Use Intel Compiler
cmake .. -DCMAKE_C_COMPILER=icc -DCMAKE_CXX_COMPILER=icpc

# Use MSVC (Windows)
cmake .. -G "Visual Studio 16 2019" -A x64

# Use TinyCC
cmake .. -DCMAKE_C_COMPILER=tcc

# Use specific compiler path
cmake .. -DCMAKE_C_COMPILER=/opt/custom/bin/gcc-12
```

## Build Configuration

### Target Architectures

Specify one or more target architectures. Multiple targets create universal/fat binaries (ELF and Mach-O only).

```bash
# Single target (i386)
cmake .. -DNUX_TARGETS="i386"

# Single target (amd64)
cmake .. -DNUX_TARGETS="amd64"

# Single target (riscv64)
cmake .. -DNUX_TARGETS="riscv64"

# Universal binary (ELF/Mach-O only)
cmake .. -DNUX_TARGETS="i386;amd64"

# Multiple targets for RISC-V
cmake .. -DNUX_TARGETS="riscv64;riscv32"
```

### Image Format

Specify the output image format:

```bash
# ELF format (default, supports universal binaries)
cmake .. -DNUX_IMAGE_FORMAT=elf

# Mach-O format (macOS, supports universal binaries)
cmake .. -DNUX_IMAGE_FORMAT=macho

# PE/COFF format (Windows, single target only)
cmake .. -DNUX_IMAGE_FORMAT=pecoff
```

**Important:** PE/COFF format does not support multiple targets. If you specify multiple targets with `pecoff`, CMake will error.

### Linker Script Mode

By default, the build system uses compiler-agnostic section attributes and runtime layout generation. You can optionally enable traditional linker scripts (GNU LD only):

```bash
# Use linker scripts (requires GNU LD)
cmake .. -DNUX_USE_LINKER_SCRIPTS=ON

# Use portable section generation (default, works with all compilers)
cmake .. -DNUX_USE_LINKER_SCRIPTS=OFF
```

### Build Components

Control which components to build:

```bash
# Build everything (default)
cmake .. -DNUX_BUILD_APXH=ON -DNUX_BUILD_LIBS=ON -DNUX_BUILD_EXAMPLE=ON

# Build only libraries
cmake .. -DNUX_BUILD_APXH=OFF -DNUX_BUILD_EXAMPLE=OFF

# Build only tools
cmake .. -DNUX_BUILD_LIBS=OFF -DNUX_BUILD_APXH=OFF -DNUX_BUILD_TOOLS=ON
```

### Build Type

```bash
# Debug build (with debug symbols)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build (optimized)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Release with debug info
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Minimum size release
cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel
```

## Complete Examples

### Example 1: GCC with ELF Universal Binary

```bash
mkdir build-gcc && cd build-gcc
cmake .. \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_BUILD_TYPE=Release \
  -DNUX_TARGETS="i386;amd64" \
  -DNUX_IMAGE_FORMAT=elf
cmake --build . -j$(nproc)
```

### Example 2: Clang with Mach-O for macOS

```bash
mkdir build-macos && cd build-macos
cmake .. \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_BUILD_TYPE=Release \
  -DNUX_TARGETS="x86_64;arm64" \
  -DNUX_IMAGE_FORMAT=macho
cmake --build . -j$(sysctl -n hw.ncpu)
```

### Example 3: MSVC with PE/COFF for Windows

```bash
mkdir build-windows && cd build-windows
cmake .. \
  -G "Visual Studio 16 2019" \
  -A x64 \
  -DNUX_TARGETS="amd64" \
  -DNUX_IMAGE_FORMAT=pecoff
cmake --build . --config Release
```

### Example 4: Intel Compiler with Optimizations

```bash
mkdir build-intel && cd build-intel
cmake .. \
  -DCMAKE_C_COMPILER=icc \
  -DCMAKE_C_FLAGS="-xHost -ipo" \
  -DCMAKE_BUILD_TYPE=Release \
  -DNUX_TARGETS="amd64"
cmake --build . -j$(nproc)
```

### Example 5: Cross-Compilation with Custom Toolchain

```bash
mkdir build-cross && cd build-cross
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/riscv64-gcc.cmake \
  -DNUX_TARGETS="riscv64" \
  -DNUX_IMAGE_FORMAT=elf
cmake --build . -j$(nproc)
```

### Example 6: TinyCC for Fast Development Builds

```bash
mkdir build-tcc && cd build-tcc
cmake .. \
  -DCMAKE_C_COMPILER=tcc \
  -DCMAKE_BUILD_TYPE=Debug \
  -DNUX_TARGETS="i386"
cmake --build .
```

## Architecture

### Compiler Detection

The build system automatically detects the compiler vendor using CMake's built-in detection and custom probes. Detected compilers are reported during configuration:

```
-- Compiler vendor: gcc
-- Compiler ID: GNU
-- Compiler version: 11.3.0
```

### Compiler Flag Abstraction

Compiler-specific flags are automatically selected based on the detected vendor:

| Feature | GCC/Clang | MSVC | Sun Studio | HP-UX | Others |
|---------|-----------|------|------------|-------|--------|
| Optimization | `-O2` | `/O2` | `-xO2` | `+O2` | `-O` |
| Debug | `-g` | `/Zi` | `-g` | `-g` | `-g` |
| Warnings | `-Wall` | `/W3` | `-v` | `+w` | `-Wall` |
| Freestanding | `-ffreestanding` | `/kernel` | N/A | N/A | N/A |

### Section Layout

Instead of linker scripts, the build system uses:

1. **Compiler Attributes** - Section placement via `__attribute__((section())` or `__declspec(allocate())`
2. **Runtime Layout Generation** - C code that defines memory regions and section boundaries
3. **Post-Link Tools** - The `objlayout` tool manipulates ELF/PE/Mach-O headers after linking

#### Generated Headers

The build system auto-generates:

- `nux/compiler_attrs.h` - Compiler-specific attribute macros
- `nux/build_config.h` - Build configuration constants
- `nux/<target>_layout.h` - Memory layout definitions

### Custom Program Headers (ELF)

For ELF binaries, APXH-specific program headers are added via the `objlayout` tool:

| Type | Value | Purpose |
|------|-------|---------|
| PT_APXH_INFO | 0xAF100000 | Boot information |
| PT_APXH_PHYSMAP | 0xAF100002 | Physical memory map |
| PT_APXH_BATREE | 0xAF100004 | Binary tree structures |
| PT_APXH_PFNCACHE | 0xAF100005 | Page frame cache |
| PT_APXH_FBUF | 0xAF100006 | Framebuffer region |
| PT_APXH_REGIONS | 0xAF100007 | Memory regions |
| PT_APXH_TOPPGTALLOC | 0xAF100008 | Top-level page table |
| PT_APXH_LINEAR | 0xAF10FFFF | Linear mapping |

## Troubleshooting

### Compiler Not Detected

If your compiler isn't automatically detected:

```bash
# Check detection
cmake .. --debug-output

# Manually specify vendor
cmake .. -DNUX_COMPILER_VENDOR=gcc
```

### Linker Errors with Non-GNU Linkers

Disable linker scripts:

```bash
cmake .. -DNUX_USE_LINKER_SCRIPTS=OFF
```

### Assembly Code Issues

The build system attempts to handle different assembly syntaxes. If you encounter issues:

1. Check the generated `nux/compiler_attrs.h` for `NUX_ASM_SYNTAX_*` definitions
2. Ensure assembly files use appropriate syntax for your compiler
3. Consider providing C implementations for compiler portability

### Missing Features on Minimal Compilers

Some compilers (TCC, PCC, LCC) have limited support for attributes and optimizations. The build system degrades gracefully, but some features may be unavailable.

## Migration from Autoconf

The old autoconf-based build system is still available but deprecated. To migrate:

### Old (Autoconf)
```bash
./bootstrap.sh
mkdir build && cd build
../configure --enable-targets=i386 --with-toolchain=llvm
make -j
```

### New (CMake)
```bash
mkdir build && cd build
cmake .. -DNUX_TARGETS=i386 -DCMAKE_C_COMPILER=clang
cmake --build . -j
```

### Key Differences

| Feature | Autoconf | CMake |
|---------|----------|-------|
| Compiler Support | GCC, Clang, limited others | All major compilers |
| Multi-target | Limited | Full support |
| IDE Integration | None | Full (compile_commands.json) |
| Cross-compilation | Manual toolchain setup | CMake toolchain files |
| Windows Native | MinGW only | Native MSVC, MinGW, Clang-CL |
| Linker Scripts | Required | Optional |

## Advanced Topics

### Creating Custom Toolchain Files

For cross-compilation, create a toolchain file:

```cmake
# cmake/toolchains/riscv64-gcc.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(CMAKE_C_COMPILER riscv64-unknown-elf-gcc)
set(CMAKE_CXX_COMPILER riscv64-unknown-elf-g++)
set(CMAKE_ASM_COMPILER riscv64-unknown-elf-gcc)

set(CMAKE_FIND_ROOT_PATH /opt/riscv)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

Use it:

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/riscv64-gcc.cmake
```

### Extending Compiler Support

To add support for a new compiler, edit `cmake/modules/NuxCompilerSetup.cmake`:

```cmake
elseif(CMAKE_C_COMPILER_ID MATCHES "MyCompiler")
    set(NUX_COMPILER_VENDOR "mycompiler" PARENT_SCOPE)
    message(STATUS "Detected MyCompiler")
```

Then add flag mappings in `nux_setup_compiler_flags()`.

### Building with Sanitizers

```bash
# AddressSanitizer
cmake .. -DCMAKE_C_FLAGS="-fsanitize=address"

# UndefinedBehaviorSanitizer
cmake .. -DCMAKE_C_FLAGS="-fsanitize=undefined"

# MemorySanitizer
cmake .. -DCMAKE_C_FLAGS="-fsanitize=memory"
```

## Contributing

When adding new source files or libraries:

1. Add appropriate `CMakeLists.txt` in the subdirectory
2. Use `target_sources()` instead of globbing
3. Use `target_include_directories()` for includes
4. Use generator expressions for compiler-specific flags
5. Test with at least GCC, Clang, and MSVC

## See Also

- [CMake Documentation](https://cmake.org/documentation/)
- [NUX Architecture Guide](TRANSFORMATION_GUIDE.md)
- [Compiler Compatibility Matrix](docs/compiler_compat.md)
