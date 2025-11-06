/** @file
  ANANKE EFI Runtime Implementation

  Implements COM interface for loading and executing EFI bytecode binaries.
  Supports PE32, PE32+, and TE (Terse Executable) formats used by UEFI
  firmware and applications.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/efiruntime.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>
#include <ecrt/string.h>
#include <ecrt/stdlib.h>

//
// PE/COFF Magic Numbers
//

#define PE_DOS_SIGNATURE      0x5A4D      ///< "MZ" - DOS header signature
#define PE_NT_SIGNATURE       0x00004550  ///< "PE\0\0" - NT header signature
#define PE_OPT_MAGIC_PE32     0x10B       ///< PE32 optional header
#define PE_OPT_MAGIC_PE32PLUS 0x20B       ///< PE32+ (64-bit) optional header

//
// TE Image Signature
//

#define TE_IMAGE_SIGNATURE    0x5A56      ///< "VZ" (little-endian)

//
// Machine Types
//

#define IMAGE_FILE_MACHINE_I386    0x014C  ///< x86
#define IMAGE_FILE_MACHINE_AMD64   0x8664  ///< x86-64
#define IMAGE_FILE_MACHINE_ARM     0x01C0  ///< ARM
#define IMAGE_FILE_MACHINE_ARM64   0xAA64  ///< ARM64
#define IMAGE_FILE_MACHINE_RISCV32 0x5032  ///< RISC-V 32-bit
#define IMAGE_FILE_MACHINE_RISCV64 0x5064  ///< RISC-V 64-bit

//
// Subsystem Types
//

#define IMAGE_SUBSYSTEM_EFI_APPLICATION          10
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER  11
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER       12
#define IMAGE_SUBSYSTEM_EFI_ROM                  13

//
// PE/COFF Structures (minimal definitions)
//

ANX_PACK_PUSH(1)

typedef struct _DOS_HEADER {
  UINT16  Signature;         ///< "MZ"
  UINT8   Reserved[58];
  UINT32  NewHeaderOffset;   ///< Offset to PE header
} DOS_HEADER;

typedef struct _COFF_HEADER {
  UINT16  Machine;
  UINT16  NumberOfSections;
  UINT32  TimeDateStamp;
  UINT32  PointerToSymbolTable;
  UINT32  NumberOfSymbols;
  UINT16  SizeOfOptionalHeader;
  UINT16  Characteristics;
} COFF_HEADER;

typedef struct _PE_OPTIONAL_HEADER32 {
  UINT16  Magic;
  UINT8   MajorLinkerVersion;
  UINT8   MinorLinkerVersion;
  UINT32  SizeOfCode;
  UINT32  SizeOfInitializedData;
  UINT32  SizeOfUninitializedData;
  UINT32  AddressOfEntryPoint;
  UINT32  BaseOfCode;
  UINT32  BaseOfData;
  UINT32  ImageBase;
  UINT32  SectionAlignment;
  UINT32  FileAlignment;
  UINT16  MajorOperatingSystemVersion;
  UINT16  MinorOperatingSystemVersion;
  UINT16  MajorImageVersion;
  UINT16  MinorImageVersion;
  UINT16  MajorSubsystemVersion;
  UINT16  MinorSubsystemVersion;
  UINT32  Win32VersionValue;
  UINT32  SizeOfImage;
  UINT32  SizeOfHeaders;
  UINT32  CheckSum;
  UINT16  Subsystem;
  UINT16  DllCharacteristics;
} PE_OPTIONAL_HEADER32;

typedef struct _PE_OPTIONAL_HEADER64 {
  UINT16  Magic;
  UINT8   MajorLinkerVersion;
  UINT8   MinorLinkerVersion;
  UINT32  SizeOfCode;
  UINT32  SizeOfInitializedData;
  UINT32  SizeOfUninitializedData;
  UINT32  AddressOfEntryPoint;
  UINT32  BaseOfCode;
  UINT64  ImageBase;
  UINT32  SectionAlignment;
  UINT32  FileAlignment;
  UINT16  MajorOperatingSystemVersion;
  UINT16  MinorOperatingSystemVersion;
  UINT16  MajorImageVersion;
  UINT16  MinorImageVersion;
  UINT16  MajorSubsystemVersion;
  UINT16  MinorSubsystemVersion;
  UINT32  Win32VersionValue;
  UINT32  SizeOfImage;
  UINT32  SizeOfHeaders;
  UINT32  CheckSum;
  UINT16  Subsystem;
  UINT16  DllCharacteristics;
} PE_OPTIONAL_HEADER64;

typedef struct _TE_IMAGE_HEADER {
  UINT16  Signature;                    ///< TE signature (0x5A56 "VZ")
  UINT16  Machine;                      ///< Machine type
  UINT8   NumberOfSections;             ///< Number of sections
  UINT8   Subsystem;                    ///< Subsystem type
  UINT16  StrippedSize;                 ///< Bytes stripped from PE header
  UINT32  AddressOfEntryPoint;          ///< Entry point RVA
  UINT32  BaseOfCode;                   ///< Base of code RVA
  UINT64  ImageBase;                    ///< Image base address
} TE_IMAGE_HEADER;

ANX_PACK_POP()

//
// EFI Runtime Implementation Structure
//

typedef struct _EFI_RUNTIME_IMPL {
  IEfiRuntime             Base;             ///< Base COM interface
  REFOBJ                  RefCount;         ///< Reference count

  EFI_EXECUTION_STATE     State;            ///< Current state
  VOID                    *ImageBuffer;     ///< Loaded image buffer
  UINTN                   ImageBufferSize;  ///< Image buffer size
  EFI_BINARY_INFO         BinaryInfo;       ///< Binary information
  EFI_EXECUTION_STATUS    ExecutionStatus;  ///< Execution status

  UINT64                  LoadTime;         ///< Load timestamp
  UINT64                  ExecutionStart;   ///< Execution start time
} EFI_RUNTIME_IMPL;

//
// Forward Declarations
//

static HRESULT STDMETHODCALLTYPE EfiRuntime_QueryInterface(
  IEfiRuntime *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE EfiRuntime_AddRef(IEfiRuntime *This);
static UINT32 STDMETHODCALLTYPE EfiRuntime_Release(IEfiRuntime *This);
static HRESULT STDMETHODCALLTYPE EfiRuntime_LoadBinary(
  IEfiRuntime *This, CONST VOID *BinaryData, UINTN BinarySize);
static HRESULT STDMETHODCALLTYPE EfiRuntime_GetBinaryInfo(
  IEfiRuntime *This, EFI_BINARY_INFO *BinaryInfo);
static HRESULT STDMETHODCALLTYPE EfiRuntime_Execute(
  IEfiRuntime *This, VOID *ImageHandle, VOID *SystemTable);
static HRESULT STDMETHODCALLTYPE EfiRuntime_GetStatus(
  IEfiRuntime *This, EFI_EXECUTION_STATUS *Status);
static HRESULT STDMETHODCALLTYPE EfiRuntime_Reset(IEfiRuntime *This);
static HRESULT STDMETHODCALLTYPE EfiRuntime_ValidateBinary(
  IEfiRuntime *This, CONST VOID *BinaryData, UINTN BinarySize,
  EFI_BINARY_INFO *BinaryInfo);

//
// VTable
//

static CONST IEfiRuntimeVtbl gEfiRuntimeVtbl = {
  .QueryInterface  = EfiRuntime_QueryInterface,
  .AddRef          = EfiRuntime_AddRef,
  .Release         = EfiRuntime_Release,
  .LoadBinary      = EfiRuntime_LoadBinary,
  .GetBinaryInfo   = EfiRuntime_GetBinaryInfo,
  .Execute         = EfiRuntime_Execute,
  .GetStatus       = EfiRuntime_GetStatus,
  .Reset           = EfiRuntime_Reset,
  .ValidateBinary  = EfiRuntime_ValidateBinary,
};

//
// Helper Functions
//

static ARCH
MapMachineTypeToArch (
  UINT16 MachineType
  )
{
  switch (MachineType) {
    case IMAGE_FILE_MACHINE_I386:
      return ARCH_I386;
    case IMAGE_FILE_MACHINE_AMD64:
      return ARCH_AMD64;
    case IMAGE_FILE_MACHINE_ARM:
      return ARCH_ARM;
    case IMAGE_FILE_MACHINE_ARM64:
      return ARCH_AARCH64;
    case IMAGE_FILE_MACHINE_RISCV32:
      return ARCH_RISCV32;
    case IMAGE_FILE_MACHINE_RISCV64:
      return ARCH_RISCV64;
    default:
      return ARCH_UNKNOWN;
  }
}

static EFI_SUBSYSTEM_TYPE
MapPeSubsystemToEfi (
  UINT16 Subsystem
  )
{
  switch (Subsystem) {
    case IMAGE_SUBSYSTEM_EFI_APPLICATION:
      return EfiSubsystemApplication;
    case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
      return EfiSubsystemBootServiceDriver;
    case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
      return EfiSubsystemRuntimeDriver;
    case IMAGE_SUBSYSTEM_EFI_ROM:
      return EfiSubsystemRom;
    default:
      return EfiSubsystemApplication;
  }
}

static HRESULT
ParsePeBinary (
  CONST VOID      *BinaryData,
  UINTN           BinarySize,
  EFI_BINARY_INFO *Info
  )
{
  DOS_HEADER              *DosHeader;
  UINT32                  *PeSignature;
  COFF_HEADER             *CoffHeader;
  PE_OPTIONAL_HEADER32    *OptHeader32;
  PE_OPTIONAL_HEADER64    *OptHeader64;
  UINT16                  Magic;

  if (BinarySize < sizeof(DOS_HEADER)) {
    return EFIRUNTIME_E_INVALID_BINARY;
  }

  DosHeader = (DOS_HEADER *)BinaryData;
  if (DosHeader->Signature != PE_DOS_SIGNATURE) {
    return EFIRUNTIME_E_INVALID_BINARY;
  }

  if (DosHeader->NewHeaderOffset + 4 + sizeof(COFF_HEADER) > BinarySize) {
    return EFIRUNTIME_E_INVALID_BINARY;
  }

  PeSignature = (UINT32 *)((UINT8 *)BinaryData + DosHeader->NewHeaderOffset);
  if (*PeSignature != PE_NT_SIGNATURE) {
    return EFIRUNTIME_E_INVALID_BINARY;
  }

  CoffHeader = (COFF_HEADER *)((UINT8 *)PeSignature + 4);
  Info->Architecture = MapMachineTypeToArch(CoffHeader->Machine);
  if (Info->Architecture == ARCH_UNKNOWN) {
    return EFIRUNTIME_E_UNSUPPORTED_ARCH;
  }

  OptHeader32 = (PE_OPTIONAL_HEADER32 *)((UINT8 *)CoffHeader + sizeof(COFF_HEADER));
  Magic = OptHeader32->Magic;

  if (Magic == PE_OPT_MAGIC_PE32) {
    Info->BinaryType = EfiBinaryTypePe32;
    Info->EntryPoint = OptHeader32->AddressOfEntryPoint;
    Info->ImageBase = OptHeader32->ImageBase;
    Info->ImageSize = OptHeader32->SizeOfImage;
    Info->Subsystem = MapPeSubsystemToEfi(OptHeader32->Subsystem);
    Info->MajorVersion = OptHeader32->MajorSubsystemVersion;
    Info->MinorVersion = OptHeader32->MinorSubsystemVersion;
  } else if (Magic == PE_OPT_MAGIC_PE32PLUS) {
    OptHeader64 = (PE_OPTIONAL_HEADER64 *)OptHeader32;
    Info->BinaryType = EfiBinaryTypePe32Plus;
    Info->EntryPoint = OptHeader64->AddressOfEntryPoint;
    Info->ImageBase = OptHeader64->ImageBase;
    Info->ImageSize = OptHeader64->SizeOfImage;
    Info->Subsystem = MapPeSubsystemToEfi(OptHeader64->Subsystem);
    Info->MajorVersion = OptHeader64->MajorSubsystemVersion;
    Info->MinorVersion = OptHeader64->MinorSubsystemVersion;
  } else {
    return EFIRUNTIME_E_INVALID_BINARY;
  }

  return S_OK;
}

static HRESULT
ParseTeBinary (
  CONST VOID      *BinaryData,
  UINTN           BinarySize,
  EFI_BINARY_INFO *Info
  )
{
  TE_IMAGE_HEADER *TeHeader;

  if (BinarySize < sizeof(TE_IMAGE_HEADER)) {
    return EFIRUNTIME_E_INVALID_BINARY;
  }

  TeHeader = (TE_IMAGE_HEADER *)BinaryData;
  if (TeHeader->Signature != TE_IMAGE_SIGNATURE) {
    return EFIRUNTIME_E_INVALID_BINARY;
  }

  Info->BinaryType = EfiBinaryTypeTe;
  Info->Architecture = MapMachineTypeToArch(TeHeader->Machine);
  Info->EntryPoint = TeHeader->AddressOfEntryPoint;
  Info->ImageBase = TeHeader->ImageBase;
  Info->ImageSize = BinarySize + TeHeader->StrippedSize;
  Info->Subsystem = MapPeSubsystemToEfi(TeHeader->Subsystem);
  Info->MajorVersion = 0;
  Info->MinorVersion = 0;

  if (Info->Architecture == ARCH_UNKNOWN) {
    return EFIRUNTIME_E_UNSUPPORTED_ARCH;
  }

  return S_OK;
}

//
// IUnknown Methods
//

static HRESULT STDMETHODCALLTYPE
EfiRuntime_QueryInterface (
  IEfiRuntime *This,
  REFIID      riid,
  VOID        **ppvObject
  )
{
  EFI_RUNTIME_IMPL *Runtime = (EFI_RUNTIME_IMPL *)This;

  if (ppvObject == NULL) {
    return E_INVALIDARG;
  }

  if (AnxIsEqualGUID(riid, &IID_IUnknown) ||
      AnxIsEqualGUID(riid, &IID_IEfiRuntime)) {
    *ppvObject = &Runtime->Base;
    IEfiRuntime_AddRef(&Runtime->Base);
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
EfiRuntime_AddRef (
  IEfiRuntime *This
  )
{
  EFI_RUNTIME_IMPL *Runtime = (EFI_RUNTIME_IMPL *)This;
  return AnxInterlockedIncrement(&Runtime->RefCount);
}

static UINT32 STDMETHODCALLTYPE
EfiRuntime_Release (
  IEfiRuntime *This
  )
{
  EFI_RUNTIME_IMPL *Runtime = (EFI_RUNTIME_IMPL *)This;
  UINT32 RefCount;

  RefCount = AnxInterlockedDecrement(&Runtime->RefCount);
  if (RefCount == 0) {
    if (Runtime->ImageBuffer != NULL) {
      free(Runtime->ImageBuffer);
    }
    free(Runtime);
  }

  return RefCount;
}

//
// IEfiRuntime Methods
//

static HRESULT STDMETHODCALLTYPE
EfiRuntime_LoadBinary (
  IEfiRuntime *This,
  CONST VOID  *BinaryData,
  UINTN       BinarySize
  )
{
  EFI_RUNTIME_IMPL *Runtime = (EFI_RUNTIME_IMPL *)This;
  HRESULT          Status;
  EFI_BINARY_INFO  Info;

  if (BinaryData == NULL || BinarySize == 0) {
    return E_INVALIDARG;
  }

  if (Runtime->State != EfiStateUnloaded) {
    return EFIRUNTIME_E_ALREADY_LOADED;
  }

  // Validate the binary first
  Status = EfiRuntime_ValidateBinary(This, BinaryData, BinarySize, &Info);
  if (FAILED(Status)) {
    return Status;
  }

  // Allocate buffer and copy binary
  Runtime->ImageBuffer = malloc(BinarySize);
  if (Runtime->ImageBuffer == NULL) {
    return E_OUTOFMEMORY;
  }

  memcpy(Runtime->ImageBuffer, BinaryData, BinarySize);
  Runtime->ImageBufferSize = BinarySize;
  Runtime->BinaryInfo = Info;
  Runtime->State = EfiStateLoaded;

  // Initialize execution status
  Runtime->ExecutionStatus.State = EfiStateLoaded;
  Runtime->ExecutionStatus.LastError = S_OK;
  Runtime->ExecutionStatus.ReturnValue = 0;
  Runtime->ExecutionStatus.ExecutionTime = 0;

  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
EfiRuntime_GetBinaryInfo (
  IEfiRuntime     *This,
  EFI_BINARY_INFO *BinaryInfo
  )
{
  EFI_RUNTIME_IMPL *Runtime = (EFI_RUNTIME_IMPL *)This;

  if (BinaryInfo == NULL) {
    return E_INVALIDARG;
  }

  if (Runtime->State == EfiStateUnloaded) {
    return EFIRUNTIME_E_NOT_LOADED;
  }

  *BinaryInfo = Runtime->BinaryInfo;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
EfiRuntime_Execute (
  IEfiRuntime *This,
  VOID        *ImageHandle,
  VOID        *SystemTable
  )
{
  EFI_RUNTIME_IMPL *Runtime = (EFI_RUNTIME_IMPL *)This;
  UINTN            (*EntryPoint)(VOID *, VOID *);
  UINTN            Result;
  UINT64           StartTime, EndTime;

  if (Runtime->State != EfiStateLoaded && Runtime->State != EfiStateCompleted) {
    return EFIRUNTIME_E_NOT_LOADED;
  }

  // Calculate entry point address
  // For simplicity, we assume the binary is loaded at its preferred base
  // In a real implementation, we would need to apply relocations
  EntryPoint = (UINTN (*)(VOID *, VOID *))(
    (UINT8 *)Runtime->ImageBuffer + Runtime->BinaryInfo.EntryPoint
  );

  // Execute the entry point
  Runtime->State = EfiStateRunning;
  Runtime->ExecutionStatus.State = EfiStateRunning;
  StartTime = 0; // In real implementation, get actual timestamp

  // Call the EFI entry point
  Result = EntryPoint(ImageHandle, SystemTable);

  EndTime = 0; // In real implementation, get actual timestamp
  Runtime->ExecutionStatus.ExecutionTime = EndTime - StartTime;
  Runtime->ExecutionStatus.ReturnValue = Result;

  if (Result == 0) {  // EFI_SUCCESS
    Runtime->State = EfiStateCompleted;
    Runtime->ExecutionStatus.State = EfiStateCompleted;
    Runtime->ExecutionStatus.LastError = S_OK;
  } else {
    Runtime->State = EfiStateFailed;
    Runtime->ExecutionStatus.State = EfiStateFailed;
    Runtime->ExecutionStatus.LastError = EFIRUNTIME_E_EXECUTION_FAILED;
  }

  return Runtime->ExecutionStatus.LastError;
}

static HRESULT STDMETHODCALLTYPE
EfiRuntime_GetStatus (
  IEfiRuntime          *This,
  EFI_EXECUTION_STATUS *Status
  )
{
  EFI_RUNTIME_IMPL *Runtime = (EFI_RUNTIME_IMPL *)This;

  if (Status == NULL) {
    return E_INVALIDARG;
  }

  *Status = Runtime->ExecutionStatus;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
EfiRuntime_Reset (
  IEfiRuntime *This
  )
{
  EFI_RUNTIME_IMPL *Runtime = (EFI_RUNTIME_IMPL *)This;

  if (Runtime->ImageBuffer != NULL) {
    free(Runtime->ImageBuffer);
    Runtime->ImageBuffer = NULL;
  }

  Runtime->ImageBufferSize = 0;
  Runtime->State = EfiStateUnloaded;
  memset(&Runtime->BinaryInfo, 0, sizeof(EFI_BINARY_INFO));
  memset(&Runtime->ExecutionStatus, 0, sizeof(EFI_EXECUTION_STATUS));

  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
EfiRuntime_ValidateBinary (
  IEfiRuntime     *This,
  CONST VOID      *BinaryData,
  UINTN           BinarySize,
  EFI_BINARY_INFO *BinaryInfo
  )
{
  HRESULT         Status;
  EFI_BINARY_INFO Info;
  UINT16          *Signature;

  if (BinaryData == NULL || BinarySize < 2) {
    return E_INVALIDARG;
  }

  memset(&Info, 0, sizeof(EFI_BINARY_INFO));

  Signature = (UINT16 *)BinaryData;

  // Try PE format first
  if (*Signature == PE_DOS_SIGNATURE) {
    Status = ParsePeBinary(BinaryData, BinarySize, &Info);
  }
  // Try TE format
  else if (*Signature == TE_IMAGE_SIGNATURE) {
    Status = ParseTeBinary(BinaryData, BinarySize, &Info);
  }
  else {
    return EFIRUNTIME_E_INVALID_BINARY;
  }

  if (FAILED(Status)) {
    return Status;
  }

  // Validate subsystem is EFI-compatible
  if (Info.Subsystem < EfiSubsystemApplication ||
      Info.Subsystem > EfiSubsystemRom) {
    return EFIRUNTIME_E_INVALID_SUBSYSTEM;
  }

  if (BinaryInfo != NULL) {
    *BinaryInfo = Info;
  }

  return S_OK;
}

//
// Factory Function
//

HRESULT
EfiRuntimeCreate (
  OUT IEfiRuntime **ppRuntime
  )
{
  EFI_RUNTIME_IMPL *Runtime;

  if (ppRuntime == NULL) {
    return E_INVALIDARG;
  }

  Runtime = (EFI_RUNTIME_IMPL *)malloc(sizeof(EFI_RUNTIME_IMPL));
  if (Runtime == NULL) {
    return E_OUTOFMEMORY;
  }

  memset(Runtime, 0, sizeof(EFI_RUNTIME_IMPL));
  Runtime->Base.lpVtbl = &gEfiRuntimeVtbl;
  Runtime->RefCount = 1;
  Runtime->State = EfiStateUnloaded;

  *ppRuntime = (IEfiRuntime *)Runtime;
  return S_OK;
}
