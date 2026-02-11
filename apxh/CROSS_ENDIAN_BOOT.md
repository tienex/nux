# Cross-Endian Boot Support

APXH bootloader supports cross-endian boot scenarios where the bootloader and kernel have different byte ordering (endianness). This document describes the implementation and provides examples for RISC-V and x86 pseudo-endian configurations.

## Overview

Cross-endian boot allows:
- **Little-endian bootloader** booting **big-endian kernel**
- **Big-endian bootloader** booting **little-endian kernel**
- Mixed-endian execution where kernel and user space have different endianness

All boot structures (APXH_BOOT_INFO, APXH_REGION, APXH_BATREE) are automatically byte-swapped to match the target kernel's endianness before transferring control.

## Architecture Support

### Native Endianness Support

Architectures with native support for both endianness modes:

- **RISC-V** (RV32, RV64, RV128): Little-endian and big-endian
- **PowerPC** (PPC32, PPC64): Little-endian and big-endian
- **ARM** (ARM32, ARM64): Little-endian and big-endian
- **MIPS** (MIPS32, MIPS64): Little-endian and big-endian
- **LoongArch** (LA32, LA64): Little-endian and big-endian

### Pseudo-Endian Support

Architectures with **pseudo-endian** modes (software byte-swapping at kernel level):

- **x86** (i386, x86-64, x32): Natively little-endian, pseudo-big-endian via kernel
- **s390x**: Natively big-endian, pseudo-little-endian via kernel
- **SPARC**: Natively big-endian, pseudo-little-endian via kernel
- **m68k**: Natively big-endian, pseudo-little-endian via kernel

In pseudo-endian mode, the CPU remains in its native endianness, but the kernel performs byte-swapping for data structures, syscalls, and potentially user space.

## Implementation Details

### Endianness Detection

The bootloader detects endianness for both kernel and user payloads:

```c
// Detect kernel endianness from executable format
gKernelEndian = GetImageEndian(KernelImageStart);

// Detect user endianness (if user payload exists)
gUserEndian = GetImageEndian(UserImageStart);
```

Endianness is determined from executable format headers:
- **ELF**: `EI_DATA` field (ELFDATA2LSB = little-endian, ELFDATA2MSB = big-endian)
- **PE**: Always little-endian
- **Mach-O**: Magic number determines endianness

### Boot Structure Conversion

Before transferring control to the kernel, all boot structures are converted:

```c
// Convert boot info to kernel's endianness
ConvertBootInfoEndianness(&BootInfo, gKernelEndian);

// Convert memory regions
ConvertRegionEndianness(&ApxhReg, gKernelEndian);

// Convert BAtree header
ConvertBatreeHeaderEndianness(&BatreeHeader, gKernelEndian);
```

### Endian-Safe Encoding

Architecture and endianness enums use **UINT8** encoding for endian-safety:

```c
typedef struct _APXH_BOOT_INFO {
  // ... other fields ...

  // UINT8 fields - endian-safe, no conversion needed
  UINT8   KernelArchitecture;     // ARCH enum
  UINT8   UserArchitecture;       // ARCH enum
  UINT8   HostArchitecture;       // ARCH enum
  UINT8   KernelEndianness;       // IMGLOAD_ENDIAN enum
  UINT8   UserEndianness;         // IMGLOAD_ENDIAN enum
  UINT8   MixedEndian;            // Boolean flag

  UINT16  Reserved1;              // Alignment

  // Multi-byte fields - must be converted
  UINT32  MixedModeFlags;
  UINT64  Magic;
  // ...
} APXH_BOOT_INFO;
```

Single-byte values require no conversion across endianness boundaries.

## RISC-V Cross-Endian Examples

### Example 1: RISC-V64 LE Bootloader → RISC-V64 BE Kernel

**Scenario**: Little-endian APXH bootloader booting big-endian RISC-V kernel

**Configuration**:
```
Bootloader: RISC-V64 little-endian (ANX_ENDIAN_LE)
Kernel:     RISC-V64 big-endian (ELF EI_DATA = ELFDATA2MSB)
User:       None
```

**Boot Flow**:
1. Bootloader detects kernel is big-endian from ELF header
2. Sets `gKernelEndian = ImgEndianBig`
3. Loads kernel image (no conversion during load)
4. Builds boot structures in bootloader's native little-endian
5. Converts all structures to big-endian before kernel entry:
   - `ConvertBootInfoEndianness(&BootInfo, ImgEndianBig)`
   - All UINT16/32/64 fields byte-swapped
   - UINT8 fields (architecture, endianness) unchanged
6. Transfers control to kernel

**Result**: Kernel receives all boot structures in native big-endian format.

### Example 2: RISC-V64 BE Bootloader → RISC-V64 LE Kernel

**Scenario**: Big-endian APXH bootloader booting little-endian RISC-V kernel

**Configuration**:
```
Bootloader: RISC-V64 big-endian (ANX_ENDIAN_BE)
Kernel:     RISC-V64 little-endian (ELF EI_DATA = ELFDATA2LSB)
User:       None
```

**Boot Flow**:
1. Bootloader detects kernel is little-endian from ELF header
2. Sets `gKernelEndian = ImgEndianLittle`
3. Builds boot structures in bootloader's native big-endian
4. Converts all structures to little-endian before kernel entry
5. Transfers control to kernel

**Result**: Kernel receives all boot structures in native little-endian format.

### Example 3: RISC-V64 LE Bootloader → Mixed-Endian (BE Kernel + LE User)

**Scenario**: Little-endian bootloader with big-endian kernel and little-endian user space

**Configuration**:
```
Bootloader: RISC-V64 little-endian
Kernel:     RISC-V64 big-endian
User:       RISC-V64 little-endian
```

**Boot Flow**:
1. Detects: `gKernelEndian = ImgEndianBig`, `gUserEndian = ImgEndianLittle`
2. Sets `gMixedEndian = TRUE` (kernel and user have different endianness)
3. Boot structures converted to kernel's big-endian format
4. Kernel must handle endianness conversion for user space

**Boot Info Contents**:
```c
BootInfo.KernelArchitecture = ArchRiscV64;
BootInfo.UserArchitecture = ArchRiscV64;
BootInfo.KernelEndianness = ImgEndianBig;      // 2
BootInfo.UserEndianness = ImgEndianLittle;    // 1
BootInfo.MixedEndian = TRUE;                  // 1
```

**Result**: Kernel knows it must perform endianness conversion for user space operations.

### Example 4: RISC-V32 LE Bootloader → RISC-V32 BE Kernel

**Configuration**:
```
Bootloader: RISC-V32 little-endian
Kernel:     RISC-V32 big-endian
User:       None
```

Same conversion process as RV64, but with 32-bit architecture.

## x86 Pseudo-Endian Examples

x86 family (i386, x86-64, x32) are natively little-endian. Pseudo-big-endian mode requires kernel-level support for byte-swapping.

### Example 5: x86-64 LE Bootloader → x86-64 Pseudo-BE Kernel

**Scenario**: Little-endian bootloader booting pseudo-big-endian x86-64 kernel

**Configuration**:
```
Bootloader: x86-64 little-endian (native)
Kernel:     x86-64 pseudo-big-endian (ELF marked as big-endian)
User:       None
```

**Boot Flow**:
1. Kernel ELF header marked with `EI_DATA = ELFDATA2MSB`
2. Bootloader detects: `gKernelArch = ArchAmd64`, `gKernelEndian = ImgEndianBig`
3. CPU remains in little-endian mode (x86 hardware restriction)
4. Boot structures converted to big-endian
5. Transfers control to kernel

**Kernel Responsibilities**:
- CPU runs in native little-endian mode
- Kernel entry point performs byte-swapping setup
- All kernel data structures maintained in big-endian
- Memory accesses byte-swapped by kernel

**Use Cases**:
- Cross-platform testing (test BE code on LE hardware)
- Emulation of big-endian systems
- Educational purposes

### Example 6: x32 LE Bootloader → x32 Pseudo-BE Kernel

**Scenario**: ILP32 x32 bootloader booting pseudo-big-endian x32 kernel

**Configuration**:
```
Bootloader: x32 little-endian (ILP32 on x86-64)
Kernel:     x32 pseudo-big-endian
User:       None
```

**Boot Flow**:
1. Bootloader compiled as x32 (32-bit pointers on 64-bit ISA)
2. Kernel marked as big-endian x32
3. Boot structures converted to big-endian
4. Uses 64-bit page tables (ILP32 requirement)

**Notes**:
- x32 uses 64-bit ISA, so `Is64BitArch(ArchAmd64_32) == TRUE`
- Page tables must be 64-bit even though pointers are 32-bit

### Example 7: i386 LE Bootloader → i386 Pseudo-BE Kernel + BE User

**Scenario**: 32-bit x86 with pseudo-big-endian kernel and user

**Configuration**:
```
Bootloader: i386 little-endian
Kernel:     i386 pseudo-big-endian
User:       i386 pseudo-big-endian
```

**Boot Flow**:
1. Both kernel and user marked as big-endian
2. `gMixedEndian = FALSE` (both have same endianness)
3. Boot structures converted to big-endian once

**Boot Info**:
```c
BootInfo.KernelArchitecture = Arch386;
BootInfo.UserArchitecture = Arch386;
BootInfo.KernelEndianness = ImgEndianBig;
BootInfo.UserEndianness = ImgEndianBig;
BootInfo.MixedEndian = FALSE;
```

### Example 8: x86-64 LE Bootloader → x86-64 Pseudo-BE Kernel + LE User

**Scenario**: Pseudo-big-endian kernel with little-endian user space

**Configuration**:
```
Bootloader: x86-64 little-endian
Kernel:     x86-64 pseudo-big-endian
User:       x86-64 little-endian (native)
```

**Boot Flow**:
1. Kernel marked as big-endian, user as little-endian
2. `gMixedEndian = TRUE`
3. Boot structures converted to big-endian for kernel

**Kernel Responsibilities**:
- Kernel operates in pseudo-big-endian mode
- User space runs in native little-endian
- Kernel must convert syscall arguments/results between endiannesses
- File I/O endianness handling per user

**Use Cases**:
- Running big-endian kernel with native little-endian applications
- Network stack testing (network byte order = big-endian)

## Boot Info Fields for Cross-Endian Scenarios

The `APXH_BOOT_INFO` structure provides endianness information to the kernel:

```c
typedef struct _APXH_BOOT_INFO {
  // ...

  /// Kernel endianness (IMGLOAD_ENDIAN enum)
  /// Values: 0=Unknown, 1=Little, 2=Big
  UINT8   KernelEndianness;

  /// User endianness (IMGLOAD_ENDIAN enum)
  /// Values: 0=Unknown, 1=Little, 2=Big
  UINT8   UserEndianness;

  /// Mixed-endian flag
  /// TRUE if kernel and user have different endianness
  UINT8   MixedEndian;

  // ...
} APXH_BOOT_INFO;
```

## Verifying Cross-Endian Boot

To verify cross-endian boot is working correctly:

### 1. Check Magic Number

The kernel should check the boot info magic number:

```c
// Kernel code
APXH_BOOT_INFO *BootInfo = GetBootInfo();

if (BootInfo->Magic != APXH_BOOTINFO_MAGIC) {
  panic("Boot info magic mismatch - endianness conversion failed!");
}
```

If magic matches, endianness conversion succeeded.

### 2. Check Endianness Fields

```c
// Kernel should see its own endianness
if (BootInfo->KernelEndianness != GetKernelNativeEndianness()) {
  panic("Kernel endianness mismatch!");
}

// Check if mixed-endian mode
if (BootInfo->MixedEndian) {
  info("Mixed-endian mode: kernel=%s, user=%s",
       EndianName(BootInfo->KernelEndianness),
       EndianName(BootInfo->UserEndianness));
  EnableEndianConversionForUserSpace();
}
```

### 3. Check Memory Regions

```c
// First region should have valid type/PFN/length
APXH_REGION *Regions = GetMemoryRegions();

if (Regions[0].Type > BootInfoRegionMax ||
    Regions[0].Pfn > 0xFFFFFFFF ||
    Regions[0].Length == 0) {
  panic("Memory region corrupted - endianness conversion failed!");
}
```

## Byte-Swapping Details

### What Gets Swapped

Multi-byte fields in boot structures:
- **UINT16**: All 16-bit fields
- **UINT32**: All 32-bit fields
- **UINT64**: All 64-bit fields

### What Doesn't Get Swapped

Single-byte fields (endian-safe):
- **UINT8**: Architecture enums, endianness enums, flags
- **Arrays of UINT8**: Byte arrays, strings

### Conversion Functions

```c
// Swap if target endianness differs from bootloader
UINT32 ConvertEndian32(UINT32 Value, IMGLOAD_ENDIAN TargetEndian);
UINT64 ConvertEndian64(UINT64 Value, IMGLOAD_ENDIAN TargetEndian);

// Structure conversion
void ConvertBootInfoEndianness(APXH_BOOT_INFO *BootInfo, IMGLOAD_ENDIAN Target);
void ConvertRegionEndianness(APXH_REGION *Region, IMGLOAD_ENDIAN Target);
void ConvertBatreeHeaderEndianness(APXH_BATREE *Header, IMGLOAD_ENDIAN Target);
```

## Performance Considerations

### Minimal Overhead

- Detection: Single ELF header read (~1 μs)
- Conversion: Only if endianness differs (~10-50 μs for all structures)
- No conversion needed if bootloader and kernel have same endianness

### Zero Runtime Cost

- Conversion happens once at boot time before kernel entry
- No ongoing performance impact during kernel execution
- UINT8 fields require zero conversion cycles

## Limitations

### 1. BAtree Contents

The BAtree (buddy allocator tree) contents are **not** byte-swapped. Only the header is converted. The BAtree uses bit operations that are endian-neutral:

```c
// Bit operations work regardless of endianness
BatreeSetBit(batree, order, pfn);
BatreeClrBit(batree, order, pfn);
```

### 2. Page Table Contents

Page tables are **not** byte-swapped. They must be built by the kernel in the correct endianness for the MMU:

```c
// Kernel must build page tables in hardware-expected format
PageTableEntry *pte = &PageTable[index];
pte->pfn = physical_address >> PAGE_SHIFT;  // Hardware determines endianness
```

### 3. Platform-Specific Data

Platform-specific pointers (ACPI, device tree, etc.) are converted as 64-bit values, but the structures they point to are not converted:

```c
BootInfo.PlatformDesc.PlatformPointer = ConvertEndian64(...);

// But the data at PlatformPointer is NOT converted
// Kernel must handle this data appropriately
```

## Testing Recommendations

### Unit Tests

Test conversion functions with known values:

```c
// Test UINT32 conversion LE→BE
UINT32 value_le = 0x12345678;
UINT32 value_be = ConvertEndian32(value_le, ImgEndianBig);
assert(value_be == 0x78563412);  // Bytes reversed

// Test UINT64 conversion BE→LE
UINT64 value_be = 0x0102030405060708ULL;
UINT64 value_le = ConvertEndian64(value_be, ImgEndianLittle);
assert(value_le == 0x0807060504030201ULL);
```

### Integration Tests

Test complete boot scenarios:

1. **RISC-V LE→BE**: Boot RISC-V BE kernel, verify magic number
2. **RISC-V BE→LE**: Boot RISC-V LE kernel, verify memory regions
3. **x86-64 LE→Pseudo-BE**: Boot pseudo-BE kernel, verify conversion
4. **Mixed-endian**: Boot BE kernel + LE user, verify mixed flag

### QEMU Testing

Use QEMU to test real cross-endian scenarios:

```bash
# Test RISC-V64 BE kernel boot
qemu-system-riscv64 \
  -M virt -cpu rv64,x-h=true \
  -kernel apxh.elf \
  -device loader,file=kernel-riscv64-be.elf,addr=0x80000000

# Test x86-64 pseudo-BE kernel boot
qemu-system-x86_64 \
  -kernel apxh.elf \
  -append "kernel=kernel-x64-pseudo-be.elf"
```

## Future Enhancements

### Potential Improvements

1. **Bi-endian page tables**: Support byte-swapped page table entries
2. **BAtree endianness**: Full BAtree content conversion (expensive)
3. **Automatic detection**: Detect pseudo-endian vs. native endian kernels
4. **Validation**: CRC/checksum verification after conversion

### Additional Architectures

Extend support to:
- **Alpha**: Supports big-endian mode (rare)
- **SuperH** (SH-4): Bi-endian capable
- **OpenRISC**: Bi-endian capable

## References

- ELF Specification: `EI_DATA` field (byte 5 of e_ident)
- RISC-V Privileged Spec: Endianness configuration
- x86 Architecture: Little-endian only (no hardware BE support)
- PowerPC ELF: EF_PPC64_ABI bit determines ABI endianness

## Summary

APXH provides complete cross-endian boot support:

✅ Automatic endianness detection from executable headers
✅ Transparent boot structure conversion
✅ Endian-safe UINT8 encoding for enums
✅ Mixed-endian kernel+user support
✅ Pseudo-endian support (software byte-swapping)
✅ Zero performance impact for same-endian boot

This enables flexible deployment scenarios including development, testing, and specialized embedded systems requiring mixed-endian configurations.
