# Cross-Endian Boot Examples

This directory contains example test kernels demonstrating APXH's cross-endian boot functionality. These kernels verify that boot structures are correctly byte-swapped when the bootloader and kernel have different endianness.

## Overview

The examples demonstrate three scenarios:

1. **RISC-V Big-Endian**: Little-endian bootloader booting big-endian kernel
2. **RISC-V Little-Endian**: Big-endian bootloader booting little-endian kernel
3. **x86-64 Pseudo-Big-Endian**: Little-endian bootloader booting "pseudo-BE" kernel

## Files

### RISC-V Examples

- `riscv64-be-kernel.c` - Big-endian RISC-V64 test kernel
- `riscv64-le-kernel.c` - Little-endian RISC-V64 test kernel
- `riscv64-start.S` - RISC-V64 assembly startup code (shared)
- `riscv64-be.lds` - Linker script for big-endian kernel
- `riscv64-le.lds` - Linker script for little-endian kernel

### x86-64 Examples

- `x86_64-pseudo-be-kernel.c` - Pseudo-big-endian x86-64 test kernel
- `x86_64-start.S` - x86-64 assembly startup code
- `x86_64-pseudo-be.lds` - Linker script for pseudo-BE kernel

### Build Infrastructure

- `Makefile` - Builds all example kernels
- `patch-elf-endian.sh` - Patches ELF header to mark as big-endian

## Building

### Prerequisites

Install cross-compilation toolchains:

```bash
# Debian/Ubuntu
sudo apt-get install gcc-riscv64-linux-gnu gcc-x86-64-linux-gnu

# Fedora/RHEL
sudo dnf install gcc-riscv64-linux-gnu gcc-x86_64-linux-gnu
```

### Build All Examples

```bash
make all
```

This builds:
- `riscv64-be-kernel.elf` - RISC-V64 big-endian kernel
- `riscv64-le-kernel.elf` - RISC-V64 little-endian kernel
- `x86_64-pseudo-be-kernel.elf` - x86-64 pseudo-BE kernel

### Build Specific Examples

```bash
make riscv64              # Build only RISC-V examples
make x86_64               # Build only x86-64 examples
make riscv64-be-kernel.elf  # Build specific kernel
```

### Clean

```bash
make clean    # Remove built files
```

## What the Test Kernels Do

Each test kernel:

1. **Receives boot info** from APXH bootloader in kernel's expected endianness
2. **Verifies magic number** - Tests that UINT64 fields were byte-swapped correctly
3. **Checks endianness fields** - Tests that UINT8 fields (endian-safe) are correct
4. **Verifies architecture** - Tests that architecture enums are readable
5. **Validates memory regions** - Tests that region structures were converted
6. **Reports results** to serial console (RISC-V) or VGA (x86-64)

### Test Output

On success, you'll see:

```
========================================
RISC-V64 Big-Endian Test Kernel
========================================

Testing cross-endian boot scenario:
  Bootloader: little-endian (assumed)
  Kernel:     big-endian (this kernel)

Native CPU endianness: big-endian

Verifying boot info magic number...
  Expected: 0x4150584842494E46
  Got:      0x4150584842494E46
  [OK] Magic number matches!

Verifying endianness fields...
  Kernel endianness: 2 (big-endian) [OK]
  User endianness:   0 (unknown/no-user) [OK]
  Mixed-endian:      0 (FALSE)

Verifying architecture fields...
  Kernel architecture: 8 (RISCV64) [OK]
  Host architecture:   8

Verifying memory regions...
  Number of regions: 4
  First region:
    Type:   1
    PFN:    0x0000000000000080
    Length: 32512
  [OK] Memory regions valid!

========================================
RESULT: ALL TESTS PASSED!
Cross-endian boot working correctly.
========================================
```

## Running with QEMU

### RISC-V Big-Endian Kernel

```bash
# Boot with little-endian APXH bootloader
qemu-system-riscv64 \
  -M virt \
  -cpu rv64 \
  -kernel riscv64-be-kernel.elf \
  -nographic

# Exit QEMU: Ctrl-A, then X
```

### RISC-V Little-Endian Kernel

```bash
# Boot with big-endian APXH bootloader (if available)
qemu-system-riscv64 \
  -M virt \
  -cpu rv64 \
  -kernel riscv64-le-kernel.elf \
  -nographic
```

### x86-64 Pseudo-Big-Endian Kernel

```bash
# Boot with little-endian APXH bootloader
qemu-system-x86_64 \
  -kernel x86_64-pseudo-be-kernel.elf \
  -nographic \
  -vga std

# Exit QEMU: Ctrl-A, then X
```

## Architecture-Specific Details

### RISC-V

RISC-V supports native big-endian and little-endian modes. The GCC `-mbig-endian` and `-mlittle-endian` flags generate truly big-endian or little-endian code.

**Big-Endian Compilation:**
```bash
riscv64-linux-gnu-gcc -mbig-endian -march=rv64imafdc -mabi=lp64d ...
```

**Little-Endian Compilation:**
```bash
riscv64-linux-gnu-gcc -mlittle-endian -march=rv64imafdc -mabi=lp64d ...
```

**ELF Header:**
- Big-endian: `EI_DATA = ELFDATA2MSB (2)`
- Little-endian: `EI_DATA = ELFDATA2LSB (1)`

APXH detects endianness from the ELF `EI_DATA` field.

### x86-64 Pseudo-Endian

x86-64 CPUs are **always little-endian** in hardware. Pseudo-big-endian mode means:

1. **CPU**: Remains in native little-endian mode
2. **Boot structures**: Provided in big-endian format by bootloader
3. **Kernel**: Performs software byte-swapping to read boot structures

**Why Pseudo-Endian?**
- Testing cross-endian code on LE-only hardware
- Emulating big-endian systems
- Educational purposes

**ELF Header Patching:**

After compilation, we patch the ELF header:

```bash
# Compile as normal (little-endian)
x86_64-linux-gnu-gcc -m64 ... -o kernel.elf

# Patch ELF header byte 5 to mark as big-endian
./patch-elf-endian.sh kernel.elf
```

This tells APXH to convert boot structures to big-endian, even though the CPU and code remain little-endian.

**Byte-Swapping Example:**

```c
// Boot info is in big-endian, CPU is little-endian
uint64_t magic_be = gBootInfo->Magic;
uint64_t magic_le = bswap64(magic_be);  // Convert BE → LE

if (magic_le == APXH_BOOTINFO_MAGIC) {
    // Success!
}
```

## Boot Info Structure

The test kernels expect this boot info structure:

```c
typedef struct {
    uint64_t Magic;                  // APXH_BOOTINFO_MAGIC
    uint64_t MaxPfn;
    uint64_t MaxRamPfn;
    uint64_t NumRegions;
    uint64_t UserEntry;

    // ... framebuffer, platform, TLS ...

    // Endian-safe UINT8 fields (no conversion needed)
    uint8_t  KernelArchitecture;    // ARCH enum
    uint8_t  UserArchitecture;
    uint8_t  HostArchitecture;
    uint8_t  KernelEndianness;      // IMGLOAD_ENDIAN enum
    uint8_t  UserEndianness;
    uint8_t  MixedEndian;
    uint16_t Reserved1;

    uint32_t MixedModeFlags;
} APXH_BOOT_INFO;
```

### Endian-Safe Fields (UINT8)

These fields require **no byte-swapping** across endianness boundaries:

- `KernelArchitecture`
- `UserArchitecture`
- `HostArchitecture`
- `KernelEndianness`
- `UserEndianness`
- `MixedEndian`

Single-byte values are endian-neutral!

### Multi-Byte Fields

These fields **must be byte-swapped** if endianness differs:

- All `uint16_t` fields
- All `uint32_t` fields
- All `uint64_t` fields

## Verification Tests

### Test 1: Magic Number

Verifies that 64-bit magic number was correctly byte-swapped:

```c
if (gBootInfo->Magic == APXH_BOOTINFO_MAGIC) {
    // ✓ Endianness conversion working
}
```

**Expected**: `0x4150584842494E46` ("APXHBINF" in ASCII)

### Test 2: Endianness Fields

Verifies that UINT8 endianness enum is correct:

```c
if (gBootInfo->KernelEndianness == ImgEndianBig) {
    // ✓ UINT8 fields are endian-safe
}
```

**Values**:
- `0` = Unknown
- `1` = Little-endian
- `2` = Big-endian

### Test 3: Architecture Fields

Verifies that UINT8 architecture enum is correct:

```c
if (gBootInfo->KernelArchitecture == ArchRiscV64) {
    // ✓ UINT8 architecture encoding working
}
```

**Architecture Values**:
- `3` = ArchAmd64
- `8` = ArchRiscV64
- (See `apxh/include/private/apxh/types.h`)

### Test 4: Memory Regions

Verifies that memory region array was correctly converted:

```c
uint64_t num_regions = gBootInfo->NumRegions;

for (int i = 0; i < num_regions; i++) {
    uint32_t type = gMemRegions[i].Type;
    uint64_t pfn = gMemRegions[i].Pfn;
    uint64_t len = gMemRegions[i].Length;

    // All fields should be in kernel's native endianness
}
```

## Troubleshooting

### Build Errors

**"riscv64-linux-gnu-gcc: command not found"**
- Install RISC-V cross-compiler: `sudo apt-get install gcc-riscv64-linux-gnu`

**"x86_64-linux-gnu-gcc: command not found"**
- Install x86-64 cross-compiler: `sudo apt-get install gcc-x86-64-linux-gnu`

### Runtime Errors

**"Magic number mismatch"**
- Boot structures were not converted to kernel's endianness
- Check that APXH bootloader supports cross-endian boot
- Verify ELF header `EI_DATA` field is correct

**"Invalid region count"**
- Memory region array was not byte-swapped
- Check `ConvertRegionEndianness()` is called in bootloader

**"Region data corrupted"**
- Individual region fields were not converted
- Verify all UINT32/UINT64 fields in `APXH_REGION` are swapped

## How APXH Performs Conversion

The APXH bootloader performs these steps:

1. **Detect kernel endianness** from ELF header `EI_DATA` field
2. **Build boot structures** in bootloader's native endianness
3. **Convert boot structures** before kernel entry:
   ```c
   ConvertBootInfoEndianness(&BootInfo, gKernelEndian);
   ConvertRegionEndianness(&Region, gKernelEndian);
   ConvertBatreeHeaderEndianness(&Batree, gKernelEndian);
   ```
4. **Skip UINT8 fields** (endian-safe)
5. **Byte-swap UINT16/32/64 fields** conditionally

See `apxh/sources/common/endian.c` for implementation.

## Implementation Notes

### Endian-Safe Encoding

Using UINT8 for enums is critical for cross-endian boot:

```c
// ✗ Bad - requires byte-swapping
uint32_t KernelArchitecture;

// ✓ Good - endian-safe
uint8_t KernelArchitecture;
```

### Byte-Swapping Macros

The bootloader uses ananke byte-swap intrinsics:

```c
#include <ananke/intrinsics.h>

uint16_t swapped16 = ANX_BSWAP16(value16);
uint32_t swapped32 = ANX_BSWAP32(value32);
uint64_t swapped64 = ANX_BSWAP64(value64);
```

### Conditional Conversion

Conversion only happens when needed:

```c
if (TargetEndian == APXH_BOOTLOADER_ENDIAN) {
    return;  // No conversion needed
}

// Perform byte-swapping
```

## Further Reading

- [CROSS_ENDIAN_BOOT.md](../../CROSS_ENDIAN_BOOT.md) - Full cross-endian boot documentation
- [apxh/sources/common/endian.c](../../sources/common/endian.c) - Conversion implementation
- [apxh/include/private/apxh/endian.h](../../include/private/apxh/endian.h) - Endianness utilities

## Contributing

To add more examples:

1. Create new kernel source file (e.g., `arm64-be-kernel.c`)
2. Create linker script (e.g., `arm64-be.lds`)
3. Add build targets to `Makefile`
4. Update this README

## License

These examples are part of the APXH bootloader project and follow the same BSD-2-Clause license.

Copyright (C) 2025 A•NUX Project
