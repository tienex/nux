/** @file
  EFI Runtime Component Test Program

  Demonstrates usage of the IEfiRuntime COM interface for loading
  and validating EFI binaries (PE/COFF and TE formats).

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/efiruntime.h>
#include <ecrt/stdio.h>
#include <ecrt/stdlib.h>

//
// Sample minimal EFI PE32 binary header
// This is a synthetic test binary header for validation testing
//

static CONST UINT8 TestEfiPeBinary[] = {
  // DOS Header (64 bytes)
  'M', 'Z',                           // e_magic: MZ signature
  0x90, 0x00,                         // e_cblp: Bytes on last page
  0x03, 0x00,                         // e_cp: Pages in file
  0x00, 0x00,                         // e_crlc: Relocations
  0x04, 0x00,                         // e_cparhdr: Size of header
  0x00, 0x00,                         // e_minalloc: Min extra paragraphs
  0xFF, 0xFF,                         // e_maxalloc: Max extra paragraphs
  0x00, 0x00,                         // e_ss: Initial SS
  0xB8, 0x00,                         // e_sp: Initial SP
  0x00, 0x00,                         // e_csum: Checksum
  0x00, 0x00,                         // e_ip: Initial IP
  0x00, 0x00,                         // e_cs: Initial CS
  0x40, 0x00,                         // e_lfarlc: Reloc table offset
  0x00, 0x00,                         // e_ovno: Overlay number
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // e_res: Reserved
  0x00, 0x00,                         // e_oemid: OEM identifier
  0x00, 0x00,                         // e_oeminfo: OEM information
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // e_res2: Reserved
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x80, 0x00, 0x00, 0x00,             // e_lfanew: PE header offset (0x80)

  // Padding to PE header offset (0x80 - 0x40 = 0x40 bytes)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

  // PE Signature (4 bytes) at offset 0x80
  'P', 'E', 0x00, 0x00,

  // COFF Header (20 bytes)
  0x64, 0x86,                         // Machine: IMAGE_FILE_MACHINE_AMD64 (0x8664)
  0x01, 0x00,                         // NumberOfSections: 1
  0x00, 0x00, 0x00, 0x00,             // TimeDateStamp
  0x00, 0x00, 0x00, 0x00,             // PointerToSymbolTable
  0x00, 0x00, 0x00, 0x00,             // NumberOfSymbols
  0xF0, 0x00,                         // SizeOfOptionalHeader: 240 (PE32+)
  0x22, 0x00,                         // Characteristics

  // PE32+ Optional Header (240 bytes minimum)
  0x0B, 0x02,                         // Magic: PE32+ (0x20B)
  0x0E, 0x00,                         // Linker version
  0x00, 0x10, 0x00, 0x00,             // SizeOfCode: 0x1000
  0x00, 0x00, 0x00, 0x00,             // SizeOfInitializedData
  0x00, 0x00, 0x00, 0x00,             // SizeOfUninitializedData
  0x00, 0x10, 0x00, 0x00,             // AddressOfEntryPoint: 0x1000
  0x00, 0x10, 0x00, 0x00,             // BaseOfCode: 0x1000
  0x00, 0x00, 0x00, 0x00,             // ImageBase (low 32 bits)
  0x00, 0x00, 0x01, 0x00,             // ImageBase (high 32 bits): 0x100000000
  0x00, 0x10, 0x00, 0x00,             // SectionAlignment: 0x1000
  0x00, 0x02, 0x00, 0x00,             // FileAlignment: 0x200
  0x06, 0x00,                         // MajorOperatingSystemVersion: 6
  0x00, 0x00,                         // MinorOperatingSystemVersion: 0
  0x00, 0x00,                         // MajorImageVersion
  0x00, 0x00,                         // MinorImageVersion
  0x06, 0x00,                         // MajorSubsystemVersion: 6
  0x00, 0x00,                         // MinorSubsystemVersion: 0
  0x00, 0x00, 0x00, 0x00,             // Win32VersionValue
  0x00, 0x20, 0x00, 0x00,             // SizeOfImage: 0x2000
  0x00, 0x02, 0x00, 0x00,             // SizeOfHeaders: 0x200
  0x00, 0x00, 0x00, 0x00,             // CheckSum
  0x0A, 0x00,                         // Subsystem: EFI_APPLICATION (10)
  0x00, 0x00,                         // DllCharacteristics
  // ... rest of optional header would continue
};

//
// Sample TE binary header for testing
//

static CONST UINT8 TestEfiTeBinary[] = {
  'V', 'Z',                           // Signature: TE (0x5A56)
  0x64, 0x86,                         // Machine: AMD64 (0x8664)
  0x01,                               // NumberOfSections: 1
  0x0A,                               // Subsystem: EFI_APPLICATION (10)
  0x28, 0x00,                         // StrippedSize: 40 bytes
  0x00, 0x10, 0x00, 0x00,             // AddressOfEntryPoint: 0x1000
  0x00, 0x10, 0x00, 0x00,             // BaseOfCode: 0x1000
  0x00, 0x00, 0x00, 0x00,             // ImageBase (low 32 bits)
  0x00, 0x00, 0x01, 0x00,             // ImageBase (high 32 bits)
  // DataDirectory[0] - Base Relocation
  0x00, 0x00, 0x00, 0x00,             // VirtualAddress
  0x00, 0x00, 0x00, 0x00,             // Size
  // DataDirectory[1] - Debug
  0x00, 0x00, 0x00, 0x00,             // VirtualAddress
  0x00, 0x00, 0x00, 0x00,             // Size
};

static VOID
PrintBinaryInfo (
  CONST EFI_BINARY_INFO *Info
  )
{
  printf("Binary Information:\n");
  printf("  Type: ");
  switch (Info->BinaryType) {
    case EfiBinaryTypePe32:
      printf("PE32 (32-bit)\n");
      break;
    case EfiBinaryTypePe32Plus:
      printf("PE32+ (64-bit)\n");
      break;
    case EfiBinaryTypeTe:
      printf("TE (Terse Executable)\n");
      break;
    default:
      printf("Unknown\n");
      break;
  }

  printf("  Subsystem: ");
  switch (Info->Subsystem) {
    case EfiSubsystemApplication:
      printf("EFI Application\n");
      break;
    case EfiSubsystemBootServiceDriver:
      printf("EFI Boot Service Driver\n");
      break;
    case EfiSubsystemRuntimeDriver:
      printf("EFI Runtime Driver\n");
      break;
    case EfiSubsystemRom:
      printf("EFI ROM\n");
      break;
    default:
      printf("Unknown\n");
      break;
  }

  printf("  Architecture: ");
  switch (Info->Architecture) {
    case ARCH_I386:
      printf("x86 (i386)\n");
      break;
    case ARCH_AMD64:
      printf("x86-64 (AMD64)\n");
      break;
    case ARCH_ARM:
      printf("ARM\n");
      break;
    case ARCH_AARCH64:
      printf("ARM64 (AArch64)\n");
      break;
    case ARCH_RISCV32:
      printf("RISC-V 32-bit\n");
      break;
    case ARCH_RISCV64:
      printf("RISC-V 64-bit\n");
      break;
    default:
      printf("Unknown\n");
      break;
  }

  printf("  Entry Point: 0x%lX\n", (unsigned long)Info->EntryPoint);
  printf("  Image Base: 0x%lX\n", (unsigned long)Info->ImageBase);
  printf("  Image Size: 0x%lX bytes\n", (unsigned long)Info->ImageSize);
  printf("  Version: %u.%u\n", Info->MajorVersion, Info->MinorVersion);
}

static VOID
TestValidatePeBinary (
  VOID
  )
{
  IEfiRuntime     *Runtime;
  HRESULT         Status;
  EFI_BINARY_INFO Info;

  printf("\n=== Test 1: Validate PE32+ EFI Binary ===\n");

  // Create runtime instance
  Status = EfiRuntimeCreate(&Runtime);
  if (FAILED(Status)) {
    printf("ERROR: Failed to create EFI runtime: 0x%08X\n", Status);
    return;
  }

  // Validate PE binary
  Status = IEfiRuntime_ValidateBinary(
    Runtime,
    TestEfiPeBinary,
    sizeof(TestEfiPeBinary),
    &Info
  );

  if (SUCCEEDED(Status)) {
    printf("SUCCESS: PE binary is valid!\n");
    PrintBinaryInfo(&Info);
  } else {
    printf("ERROR: PE binary validation failed: 0x%08X\n", Status);
  }

  IEfiRuntime_Release(Runtime);
}

static VOID
TestValidateTeBinary (
  VOID
  )
{
  IEfiRuntime     *Runtime;
  HRESULT         Status;
  EFI_BINARY_INFO Info;

  printf("\n=== Test 2: Validate TE EFI Binary ===\n");

  // Create runtime instance
  Status = EfiRuntimeCreate(&Runtime);
  if (FAILED(Status)) {
    printf("ERROR: Failed to create EFI runtime: 0x%08X\n", Status);
    return;
  }

  // Validate TE binary
  Status = IEfiRuntime_ValidateBinary(
    Runtime,
    TestEfiTeBinary,
    sizeof(TestEfiTeBinary),
    &Info
  );

  if (SUCCEEDED(Status)) {
    printf("SUCCESS: TE binary is valid!\n");
    PrintBinaryInfo(&Info);
  } else {
    printf("ERROR: TE binary validation failed: 0x%08X\n", Status);
  }

  IEfiRuntime_Release(Runtime);
}

static VOID
TestLoadBinary (
  VOID
  )
{
  IEfiRuntime          *Runtime;
  HRESULT              Status;
  EFI_BINARY_INFO      Info;
  EFI_EXECUTION_STATUS ExecStatus;

  printf("\n=== Test 3: Load EFI Binary ===\n");

  // Create runtime instance
  Status = EfiRuntimeCreate(&Runtime);
  if (FAILED(Status)) {
    printf("ERROR: Failed to create EFI runtime: 0x%08X\n", Status);
    return;
  }

  // Load PE binary
  Status = IEfiRuntime_LoadBinary(
    Runtime,
    TestEfiPeBinary,
    sizeof(TestEfiPeBinary)
  );

  if (SUCCEEDED(Status)) {
    printf("SUCCESS: Binary loaded!\n");

    // Get binary info
    Status = IEfiRuntime_GetBinaryInfo(Runtime, &Info);
    if (SUCCEEDED(Status)) {
      PrintBinaryInfo(&Info);
    }

    // Get status
    Status = IEfiRuntime_GetStatus(Runtime, &ExecStatus);
    if (SUCCEEDED(Status)) {
      printf("\nExecution Status:\n");
      printf("  State: ");
      switch (ExecStatus.State) {
        case EfiStateUnloaded:
          printf("Unloaded\n");
          break;
        case EfiStateLoaded:
          printf("Loaded\n");
          break;
        case EfiStateRunning:
          printf("Running\n");
          break;
        case EfiStateCompleted:
          printf("Completed\n");
          break;
        case EfiStateFailed:
          printf("Failed\n");
          break;
        default:
          printf("Unknown\n");
          break;
      }
    }

    // Reset runtime
    IEfiRuntime_Reset(Runtime);
    printf("\nRuntime reset successfully.\n");
  } else {
    printf("ERROR: Binary load failed: 0x%08X\n", Status);
  }

  IEfiRuntime_Release(Runtime);
}

INT32
main (
  INT32 argc,
  CHAR8 **argv
  )
{
  printf("ANANKE EFI Runtime Component Test\n");
  printf("==================================\n");

  TestValidatePeBinary();
  TestValidateTeBinary();
  TestLoadBinary();

  printf("\n=== All Tests Complete ===\n");
  return 0;
}
