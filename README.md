# NUX
* _nux, nucis : .gen. plur. nucerum for nucum, f. etym. dub., a nut._

For a high-level introduction on NUX and its motivation, check [this article](https://nux.tlbflush.org/post/2024_12_24_notes_nux/).

## COM-Style Architecture

**This branch features a comprehensive transformation to COM-style architecture with NT coding conventions and UEFI documentation.**

### Key Changes

- **COM Infrastructure**: Complete IUnknown-based interface system with vtables
- **NT Coding Style**: PascalCase naming, Hungarian notation, proper parameter annotations
- **UEFI Documentation**: Professional Doxygen-style comments throughout
- **Backward Compatibility**: All legacy APIs preserved via inline wrappers

### New Features

- **Interface Segregation**: Clean separation of HAL/PLT subsystems into focused interfaces
- **Extensibility**: QueryInterface support for runtime interface discovery
- **Binary Compatibility**: Stable vtable-based ABI
- **Standard Error Handling**: Industry-standard HRESULT return values

### Documentation

See [TRANSFORMATION_GUIDE.md](TRANSFORMATION_GUIDE.md) for complete details on:
- Transformation rationale and benefits
- COM architecture overview
- Migration guide for developers
- NT coding style conventions
- UEFI commenting standards

### Transformed Files (15 Headers - Complete Public API)

**Core Infrastructure:**
- `include/nux/combase.h` - COM base types and IUnknown interface
- `include/nux/types.h` - NT-style type definitions

**Interface Layers (22 COM Interfaces Total):**
- `include/nux/hal.h` - Hardware Abstraction Layer (7 COM interfaces)
- `include/nux/plt.h` - Platform Layer (5 COM interfaces)
- `include/nux/nux.h` - Main Kernel API (10 COM interfaces)

**Utilities and Services:**
- `include/nux/defs.h` - Page size and alignment macros
- `include/nux/locks.h` - Spinlock and RW-lock primitives
- `include/nux/slab.h` - Slab allocator API
- `include/nux/slabinc.h` - Slab allocator internal structures
- `include/nux/cpumask.h` - CPU mask operations
- `include/nux/cache.h` - Generic cache with LRU eviction

**Boot and Platform:**
- `include/nux/apxh.h` - APXH boot protocol structures

**Performance and Debugging:**
- `include/nux/nuxperf.h` - Performance counters and measures
- `include/nux/symbol.h` - Symbol resolution utilities

**Architecture Support:**
- `include/nux/nmiemul.h` - NMI emulation layer

All files maintain 100% backward compatibility through legacy function wrappers and type aliases.

---

## What it is
NUX is a framework to prototype kernels and related userspace programs that run on real, modern hardware.
Currently supported architectures are x86_64, riscv64 and i386.

A kernel, with NUX, is nothing more than a C file with a `main` function and other functions
that defines how the kernel behaves on certain events:

- `main_ap` called by a secondary processor when it is booted
- `entry_ipi` called when an inter-processor interrupt is received by the current CPU
- `entry_alarm` called when the platform timer expires
- `entry_irq` called when the platform issues an IRQ.
- `entry_sysc` to handle user space system calls.
- `entry_ex` to handle user space exceptions
- `entry_pf` to handle user space page faults

See the [example kernel](https://github.com/glguida/nux/blob/main/example/kern/main.c) and
[exmaple userspace](https://github.com/glguida/nux/blob/main/example/user/main.c).

NUX also provides _libnux_, a runtime kernel support library to handle platform and memory,
and _libec_ a basic embedded C library based on the NetBSD libc.

On the userspace side, NUX provides `libnux_user`, that defines the syscall interface of the kernel,
and _libec_, the same embedded C library used by the kernel side.

NUX kernels are booted by APXH (uppercase for αρχη, or _beginning_ in ancient greek).
APXH currently supports:
- `EFI` on i386, amd64 and riscv64
- `multiboot` on i386 and amd64
- `SBI` (riscv64).

## Building NUX

You need to have and embedded ELF target compiler. If you're building for riscv, be sure to read instructions
below.

_If you have already your own embedded ELF compiler (such as amd64-unknown-elf-gcc or amd64-elf-gcc), you
can skip the following_.

### 1. Building the toolchain

`gcc_toolchain_build` is a super simple Makefile to automate building GCC for embedded targets.

If you want to build at once all the compilers and tools required to build all platforms supported by nux,
do the following: _(it'll take quite a while)_

```
git clone https://github.com/glguida/gcc_toolchain_build
cd gcc_toolchain_build
make populate
make amd64-unknown-elf-gcc
make i686-unknown-elf-gcc
make riscv64-unknown-elf-gcc
export PATH=$PWD/install/bin
cd ..
```

### 2. Compile NUX

Building NUX is as simple as using `configure` and `make`.

```
git clone https://github.com/glguida/nux
cd nux
git submodule update --init --recursive
mkdir build
cd build
../configure --enable-targets=i386
make -j
````

Now you can run the demo:

```
cd example
make qemu
```

### Build Configuration Options

**Target Architecture Selection:**

Use `--enable-targets=LIST` to specify one or more target architectures (comma-separated):

```bash
# Single target (i386)
../configure --enable-targets=i386

# Single target (amd64)
../configure --enable-targets=amd64

# Single target (riscv64)
../configure --enable-targets=riscv64

# Universal binary for ELF/Mach-O (multiple targets)
../configure --enable-targets=i386,amd64

# Note: PE/COFF format supports only a single target
```

**Toolchain Selection:**

Choose between GNU and LLVM toolchains using `--with-toolchain`:

```bash
# Use GNU toolchain (default)
../configure --enable-targets=i386 --with-toolchain=gnu

# Use LLVM/Clang toolchain
../configure --enable-targets=amd64 --with-toolchain=llvm
```

**Custom Toolchain Prefix and Suffix:**

If you need to specify a custom toolchain prefix or suffix:

```bash
# Custom prefix (e.g., for x86_64-elf-gcc instead of x86_64-unknown-elf-gcc)
../configure --enable-targets=amd64 --with-toolchain-prefix=x86_64-elf-

# Custom suffix (e.g., for clang-17 instead of clang)
../configure --enable-targets=i386 --with-toolchain=llvm --with-toolchain-suffix=-17

# Combined example
../configure --enable-targets=amd64 \
  --with-toolchain=llvm \
  --with-toolchain-prefix=x86_64-linux-gnu- \
  --with-toolchain-suffix=-17
```

**Examples:**

```bash
# GNU toolchain with custom prefix
../configure --enable-targets=amd64 --with-toolchain-prefix=x86_64-elf-

# LLVM toolchain with version suffix
../configure --enable-targets=i386 --with-toolchain=llvm --with-toolchain-suffix=-17

# Universal binary for x86 (ELF)
../configure --enable-targets=i386,amd64 --with-toolchain=gnu
```

**Note for RISCV64:**

NUX will attempt to build APXH with EFI support on riscv64. This is done using `gnu-efi`.
If you are _not_ using the toolchain built with `gcc_toolchain_build`, this will fail.

If you still intend to use another toolchain, then you have to edit apxh/Makefile.in,
removing 'efi' from the list of `SUBDIRS`.
