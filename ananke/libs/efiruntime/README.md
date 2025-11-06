# ANANKE EFI Runtime Library

A COM-based component for loading and executing EFI bytecode binaries (PE/COFF and TE format).

## Overview

The EFI Runtime library provides a high-level COM interface for working with UEFI executable binaries. It supports:

- **PE32** (32-bit Portable Executable) EFI binaries
- **PE32+** (64-bit Portable Executable) EFI binaries
- **TE** (Terse Executable) UEFI firmware binaries

## Features

- ✅ Binary validation and parsing
- ✅ PE/COFF format support (PE32 and PE32+)
- ✅ TE format support (Terse Executable for firmware)
- ✅ Multi-architecture support (x86, x86-64, ARM, ARM64, RISC-V)
- ✅ EFI subsystem detection
- ✅ Entry point resolution
- ✅ Binary information extraction
- ✅ Execution state tracking
- ✅ COM-based interface for easy integration

## Architecture Support

- x86 (i386)
- x86-64 (AMD64)
- ARM (32-bit)
- ARM64 (AArch64)
- RISC-V 32-bit
- RISC-V 64-bit

## EFI Subsystems

The library recognizes and supports the following EFI subsystems:

- **EFI Application** - Standard UEFI applications
- **EFI Boot Service Driver** - Boot-time drivers
- **EFI Runtime Driver** - Runtime service drivers
- **EFI ROM** - ROM images

## COM Interface: IEfiRuntime

### Methods

#### LoadBinary
```c
HRESULT LoadBinary(
  IN CONST VOID *BinaryData,
  IN UINTN      BinarySize
);
```
Load an EFI binary (PE/COFF or TE format) into the runtime environment.

#### GetBinaryInfo
```c
HRESULT GetBinaryInfo(
  OUT EFI_BINARY_INFO *BinaryInfo
);
```
Retrieve information about the loaded binary (type, architecture, entry point, etc.).

#### Execute
```c
HRESULT Execute(
  IN VOID *ImageHandle,
  IN VOID *SystemTable
);
```
Execute the loaded EFI binary's entry point with the provided EFI environment.

#### GetStatus
```c
HRESULT GetStatus(
  OUT EFI_EXECUTION_STATUS *Status
);
```
Get the current execution state and status information.

#### Reset
```c
HRESULT Reset(VOID);
```
Reset the runtime environment and unload the current binary.

#### ValidateBinary
```c
HRESULT ValidateBinary(
  IN  CONST VOID      *BinaryData,
  IN  UINTN           BinarySize,
  OUT EFI_BINARY_INFO *BinaryInfo
);
```
Validate an EFI binary without loading it, and optionally retrieve its information.

## Usage Example

```c
#include <ananke/efiruntime.h>

// Create runtime instance
IEfiRuntime *runtime;
HRESULT status = EfiRuntimeCreate(&runtime);

// Validate a binary
EFI_BINARY_INFO info;
status = IEfiRuntime_ValidateBinary(
  runtime,
  binaryData,
  binarySize,
  &info
);

if (SUCCEEDED(status)) {
  printf("Binary type: %s\n",
    info.BinaryType == EfiBinaryTypePe32Plus ? "PE32+" : "PE32");
  printf("Architecture: %d\n", info.Architecture);
  printf("Entry point: 0x%lX\n", info.EntryPoint);
}

// Load the binary
status = IEfiRuntime_LoadBinary(runtime, binaryData, binarySize);

// Execute it (requires proper EFI environment)
if (SUCCEEDED(status)) {
  status = IEfiRuntime_Execute(runtime, imageHandle, systemTable);
}

// Check execution status
EFI_EXECUTION_STATUS execStatus;
IEfiRuntime_GetStatus(runtime, &execStatus);

// Clean up
IEfiRuntime_Reset(runtime);
IEfiRuntime_Release(runtime);
```

## Binary Format Support

### PE/COFF Format
The library supports standard Windows PE/COFF executables with EFI subsystems:
- DOS header + PE signature
- COFF header with machine type
- Optional header (PE32 or PE32+)
- Subsystem field indicates EFI type

### TE Format
The library supports UEFI Terse Executable format:
- Compressed PE/COFF format for firmware
- Reduced header overhead
- Common in PEIMs and DXE drivers
- Signature: "VZ" (0x5A56)

## Error Codes

| Code | Description |
|------|-------------|
| `EFIRUNTIME_E_INVALID_BINARY` | Invalid EFI binary format |
| `EFIRUNTIME_E_UNSUPPORTED_ARCH` | Unsupported architecture |
| `EFIRUNTIME_E_LOAD_FAILED` | Failed to load binary |
| `EFIRUNTIME_E_EXECUTION_FAILED` | Execution failed |
| `EFIRUNTIME_E_NOT_LOADED` | No binary loaded |
| `EFIRUNTIME_E_ALREADY_LOADED` | Binary already loaded |
| `EFIRUNTIME_E_INVALID_SUBSYSTEM` | Invalid EFI subsystem |

## Building

The library is built as part of the ANANKE foundation:

```bash
cd ananke/libs/efiruntime
make
```

## Testing

A test program is provided in the `example/` directory:

```bash
cd example
gcc -o test_efi_loader test_efi_loader.c \
  -I../include -I../../include \
  -L.. -lefiruntime -lecrt
./test_efi_loader
```

## Integration

To use the EFI Runtime library in your project:

1. Include the header:
   ```c
   #include <ananke/efiruntime.h>
   ```

2. Link against the library:
   ```
   -lefiruntime
   ```

3. Ensure ANANKE foundation headers are in your include path:
   ```
   -I$(ANANKE)/include
   ```

## Implementation Notes

### Current Limitations

1. **Execution Environment**: The `Execute()` method currently performs a simple function call to the entry point. A full EFI execution environment with proper UEFI services is not yet implemented.

2. **Relocations**: The implementation does not currently apply PE relocations. Binaries are assumed to be loaded at their preferred base address.

3. **Section Loading**: Full section mapping and protection attributes are not yet implemented.

4. **Import Resolution**: Dynamic linking and import table processing are not implemented.

### Future Enhancements

- [ ] Full EFI Boot Services and Runtime Services implementation
- [ ] PE relocation processing
- [ ] Section mapping with proper memory attributes
- [ ] Import table resolution
- [ ] TLS (Thread Local Storage) support
- [ ] Exception handling and unwinding support
- [ ] Integration with UEFI firmware environment

## License

Copyright (C) 2025 A•NUX Project

SPDX-License-Identifier: BSD-2-Clause

## See Also

- APXH Bootloader image loaders (`/apxh/sources/loader/`)
- ANANKE Foundation (`/ananke/include/ananke/`)
- UEFI Specification: https://uefi.org/specifications
