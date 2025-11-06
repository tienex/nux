/** @file
  ANANKE EFI Runtime COM Interface

  Provides COM interface for loading and executing EFI bytecode binaries
  (PE/COFF and TE format). Supports EFI applications, boot service drivers,
  and runtime drivers with proper environment setup and execution control.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <ananke/ananke.h>

//
// EFI Runtime HRESULT Codes
//

#define EFIRUNTIME_E_INVALID_BINARY      ((HRESULT)0x800E0001L)  ///< Invalid EFI binary format
#define EFIRUNTIME_E_UNSUPPORTED_ARCH    ((HRESULT)0x800E0002L)  ///< Unsupported architecture
#define EFIRUNTIME_E_LOAD_FAILED         ((HRESULT)0x800E0003L)  ///< Failed to load binary
#define EFIRUNTIME_E_EXECUTION_FAILED    ((HRESULT)0x800E0004L)  ///< Execution failed
#define EFIRUNTIME_E_NOT_LOADED          ((HRESULT)0x800E0005L)  ///< No binary loaded
#define EFIRUNTIME_E_ALREADY_LOADED      ((HRESULT)0x800E0006L)  ///< Binary already loaded
#define EFIRUNTIME_E_INVALID_SUBSYSTEM   ((HRESULT)0x800E0007L)  ///< Invalid EFI subsystem

//
// EFI Binary Type
//

typedef enum _EFI_BINARY_TYPE {
  EfiBinaryTypePe32       = 0,  ///< PE32 (32-bit) EFI binary
  EfiBinaryTypePe32Plus   = 1,  ///< PE32+ (64-bit) EFI binary
  EfiBinaryTypeTe         = 2,  ///< TE (Terse Executable) EFI binary
} EFI_BINARY_TYPE;

//
// EFI Subsystem Type
//

typedef enum _EFI_SUBSYSTEM_TYPE {
  EfiSubsystemApplication        = 10,  ///< UEFI application
  EfiSubsystemBootServiceDriver  = 11,  ///< UEFI boot service driver
  EfiSubsystemRuntimeDriver      = 12,  ///< UEFI runtime driver
  EfiSubsystemRom                = 13,  ///< UEFI ROM image
} EFI_SUBSYSTEM_TYPE;

//
// EFI Execution State
//

typedef enum _EFI_EXECUTION_STATE {
  EfiStateUnloaded     = 0,  ///< No binary loaded
  EfiStateLoaded       = 1,  ///< Binary loaded, ready to execute
  EfiStateRunning      = 2,  ///< Currently executing
  EfiStateCompleted    = 3,  ///< Execution completed successfully
  EfiStateFailed       = 4,  ///< Execution failed with error
} EFI_EXECUTION_STATE;

//
// EFI Binary Information
//

typedef struct _EFI_BINARY_INFO {
  EFI_BINARY_TYPE      BinaryType;      ///< PE32, PE32+, or TE
  EFI_SUBSYSTEM_TYPE   Subsystem;       ///< EFI subsystem type
  ARCH                 Architecture;    ///< Target architecture
  VIRTUAL_ADDRESS      EntryPoint;      ///< Entry point virtual address
  VIRTUAL_ADDRESS      ImageBase;       ///< Preferred image base address
  UINTN                ImageSize;       ///< Total image size in bytes
  UINT16               MajorVersion;    ///< Major subsystem version
  UINT16               MinorVersion;    ///< Minor subsystem version
} EFI_BINARY_INFO, *PEFI_BINARY_INFO;

//
// EFI Execution Status
//

typedef struct _EFI_EXECUTION_STATUS {
  EFI_EXECUTION_STATE  State;           ///< Current execution state
  HRESULT              LastError;       ///< Last error code (if any)
  UINTN                ReturnValue;     ///< Return value from entry point
  UINT64               ExecutionTime;   ///< Execution time in microseconds
} EFI_EXECUTION_STATUS, *PEFI_EXECUTION_STATUS;

//
// Forward declarations
//

typedef struct _IEfiRuntime IEfiRuntime;

//
// IEfiRuntime Interface GUID
// {7C4E3A2B-9F1D-4E8A-B3C5-6D7E8F9A0B1C}
//

#define ANX_IID_IEfiRuntime "7C4E3A2B-9F1D-4E8A-B3C5-6D7E8F9A0B1C"
ANX_DEFINE_GUID(IID_IEfiRuntime, 0x7C4E3A2B,0x9F1D,0x4E8A,0xB3,0xC5,0x6D,0x7E,0x8F,0x9A,0x0B,0x1C);

//
// IEfiRuntime Interface
//

ANX_BEGIN_INTERFACE(IEfiRuntime, IUnknown, IID_IEfiRuntime, ANX_IID_IEfiRuntime)
  /**
    Load an EFI binary (PE/COFF or TE format) into the runtime.

    @param[in] This         Pointer to this interface.
    @param[in] BinaryData   Pointer to EFI binary data in memory.
    @param[in] BinarySize   Size of the binary in bytes.

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, LoadBinary, (
    IN CONST VOID   *BinaryData,
    IN UINTN        BinarySize
    ))

  /**
    Get information about the loaded EFI binary.

    @param[in]  This        Pointer to this interface.
    @param[out] BinaryInfo  Receives binary information.

    @return S_OK on success, EFIRUNTIME_E_NOT_LOADED if no binary loaded.
  **/
  ANX_IFACE_METHOD(HRESULT, GetBinaryInfo, (
    OUT EFI_BINARY_INFO  *BinaryInfo
    ))

  /**
    Execute the loaded EFI binary.

    Executes the entry point of the loaded EFI binary with proper
    environment setup. For applications, this runs the main entry point.
    For drivers, this calls the driver initialization routine.

    @param[in] This         Pointer to this interface.
    @param[in] ImageHandle  EFI image handle (can be NULL for testing).
    @param[in] SystemTable  EFI system table pointer (can be NULL for testing).

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, Execute, (
    IN VOID  *ImageHandle,
    IN VOID  *SystemTable
    ))

  /**
    Get the current execution status.

    @param[in]  This    Pointer to this interface.
    @param[out] Status  Receives execution status information.

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetStatus, (
    OUT EFI_EXECUTION_STATUS  *Status
    ))

  /**
    Reset the runtime environment and unload the binary.

    Frees all resources associated with the loaded binary and
    resets the runtime to the unloaded state.

    @param[in] This  Pointer to this interface.

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, Reset, (
    VOID
    ))

  /**
    Validate an EFI binary without loading it.

    Checks if the binary is a valid EFI PE/COFF or TE format
    and extracts basic information.

    @param[in]  This        Pointer to this interface.
    @param[in]  BinaryData  Pointer to EFI binary data in memory.
    @param[in]  BinarySize  Size of the binary in bytes.
    @param[out] BinaryInfo  Receives binary information (optional).

    @return S_OK if valid, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, ValidateBinary, (
    IN  CONST VOID       *BinaryData,
    IN  UINTN            BinarySize,
    OUT EFI_BINARY_INFO  *BinaryInfo
    ))

ANX_END_INTERFACE(IEfiRuntime)

//
// Factory Function
//

/**
  Create an EFI runtime instance.

  @param[out] ppRuntime  Receives pointer to IEfiRuntime interface.

  @return S_OK on success, error code otherwise.
**/
HRESULT
EfiRuntimeCreate (
  OUT IEfiRuntime  **ppRuntime
  );
