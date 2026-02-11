/** @file
  APXH RISC OS Loader
  
  Provides RISC OS AIF (Acorn Image Format) and AOF (Acorn Object Format)
  parsing and loading for ARM-based Acorn/RISC OS systems.
  
  Supports:
  - RISC OS AIF (Application Image Format)
  - ARM architecture (ARM2, ARM3, StrongARM)
  
  Copyright (C) 2025 A•NUX Project
  
  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>
#include <ananke/resource.h>
#include "imgresource.h"

// AIF header (simplified)
#define AIF_MAGIC  0xE1A00000  // NOP instruction (MOV R0, R0)

ANX_PACK_PUSH(1)
typedef struct _AIF_HEADER {
  UINT32  BranchInstruction;  // Branch to start of code (or NOP)
  UINT32  BranchToExit;       // Branch to exit code  
  UINT32  RODataSize;         // Read-only data size
  UINT32  RWDataSize;         // Read-write data size
  UINT32  DebugSize;          // Debug data size
  UINT32  ZeroInitSize;       // Zero-init data size
  UINT32  DebugType;          // Debug type
  UINT32  ImageBase;          // Image base address
  UINT32  WorkSpace;          // Workspace size
  UINT32  AddressingMode;     // 26-bit or 32-bit addressing
  UINT32  DataBase;           // Data base address
  UINT32  Reserved[2];        // Reserved
} AIF_HEADER;
ANX_PACK_POP()

// IUnknown methods
static HRESULT STDMETHODCALLTYPE AcornQueryInterface(IN IImageLoader *This, IN CONST GUID *Iid, OUT VOID **Interface) {
  if (Interface == NULL) return E_POINTER;
  if (memcmp(Iid, &IID_IImageLoader, sizeof(GUID)) == 0 || memcmp(Iid, &IID_IUnknown, sizeof(GUID)) == 0) {
    *Interface = This;
    return S_OK;
  }
  *Interface = NULL;
  return E_NOINTERFACE;
}

static UINTN STDMETHODCALLTYPE AcornAddRef(IN IImageLoader *This) { return 1; }
static UINTN STDMETHODCALLTYPE AcornRelease(IN IImageLoader *This) { return 1; }

// Format detection
static HRESULT STDMETHODCALLTYPE AcornDetect(IN IImageLoader *This, IN VOID *ImageBase, IN UINTN ImageSize) {
  AIF_HEADER *Header;
  if (ImageSize < sizeof(AIF_HEADER)) return S_FALSE;
  Header = (AIF_HEADER *)ImageBase;
  // Check for NOP or branch instruction
  return ((Header->BranchInstruction == AIF_MAGIC) || 
          ((Header->BranchInstruction & 0x0F000000) == 0x0A000000)) ? S_OK : S_FALSE;
}

// Architecture detection
static HRESULT STDMETHODCALLTYPE AcornGetArch(IN IImageLoader *This, IN VOID *ImageBase, OUT ARCH *Architecture) {
  if (Architecture == NULL) return E_POINTER;
  *Architecture = ArchArm32;  // RISC OS is ARM-based
  return S_OK;
}

// Endianness
static HRESULT STDMETHODCALLTYPE AcornGetEndianness(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_ENDIAN *Endianness) {
  if (Endianness == NULL) return E_POINTER;
  *Endianness = ImgEndianLittle;  // ARM is little-endian
  return S_OK;
}

// Entry point
static HRESULT STDMETHODCALLTYPE AcornGetEntryPoint(IN IImageLoader *This, IN VOID *ImageBase, OUT VIRTUAL_ADDRESS *EntryPoint) {
  AIF_HEADER *Header;
  if (EntryPoint == NULL) return E_POINTER;
  Header = (AIF_HEADER *)ImageBase;
  *EntryPoint = Header->ImageBase;
  return S_OK;
}

// Load image (stub)
static HRESULT STDMETHODCALLTYPE AcornLoadImage(IN OUT IMGLOAD_CONTEXT *Context) {
  if (Context == NULL) return E_POINTER;
  info("Loading RISC OS AIF executable...");
  return IMGLOAD_E_INVALID_FORMAT;  // Not fully implemented
}

// TLS info
static HRESULT STDMETHODCALLTYPE AcornGetTlsInfo(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_TLS_INFO *TlsInfo) {
  if (TlsInfo == NULL) return E_POINTER;
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

// Unwind info
static HRESULT STDMETHODCALLTYPE AcornGetUnwindInfo(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_UNWIND_INFO *UnwindInfo) {
  if (UnwindInfo == NULL) return E_POINTER;
  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

// Symbol lookup stubs
static HRESULT STDMETHODCALLTYPE AcornGetSymbolByAddress(IN IImageLoader *This, IN VOID *ImageBase, IN VIRTUAL_ADDRESS Address, OUT IMGLOAD_SYMBOL_INFO *SymbolInfo) {
  if (SymbolInfo == NULL) return E_POINTER;
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE AcornGetSymbolByName(IN IImageLoader *This, IN VOID *ImageBase, IN CONST CHAR8 *Name, OUT IMGLOAD_SYMBOL_INFO *SymbolInfo) {
  if (SymbolInfo == NULL || Name == NULL) return E_POINTER;
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

// Relocation stubs
static HRESULT STDMETHODCALLTYPE AcornGetRelocInfo(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_RELOC_INFO *RelocInfo) {
  if (RelocInfo == NULL) return E_POINTER;
  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE AcornApplyRelocations(IN IImageLoader *This, IN VOID *ImageBase, IN VIRTUAL_ADDRESS LoadAddress, IN VIRTUAL_ADDRESS PreferredBase) {
  return S_OK;
}

// System info
static HRESULT STDMETHODCALLTYPE AcornGetTargetSystem(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_TARGET_SYSTEM *TargetSystem) {
  if (TargetSystem == NULL) return E_POINTER;
  *TargetSystem = ImgSystemRiscOs;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE AcornGetMinimumSystemVersion(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_SYSTEM_VERSION *MinimumVersion) {
  if (MinimumVersion == NULL) return E_POINTER;
  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE AcornGetTargetSubsystem(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_TARGET_SUBSYSTEM *TargetSubsystem) {
  if (TargetSubsystem == NULL) return E_POINTER;
  *TargetSubsystem = ImgSubsystemRiscOsWimp;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE AcornGetMinimumSubsystemVersion(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_SYSTEM_VERSION *MinimumVersion) {
  if (MinimumVersion == NULL) return E_POINTER;
  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

// Resource methods
static HRESULT STDMETHODCALLTYPE AcornGetResource(IN IImageLoader *This, IN VOID *ImageBase, IN UINT32 TypeCode, IN UINT32 Id, IN CONST CHAR8 *Name, OUT IImageResource **Resource) {
  if (Resource == NULL) return E_POINTER;
  *Resource = NULL;
  // RISC OS AIF doesn't have native resources
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE AcornGetResourceEnumerator(IN IImageLoader *This, IN VOID *ImageBase, IN UINT32 TypeCode, OUT IEnumImageResource **Enumerator) {
  if (Enumerator == NULL) return E_POINTER;
  *Enumerator = NULL;
  return S_FALSE;
}

// VTable
static CONST IImageLoaderVtbl gAcornVtbl = {
  AcornQueryInterface, AcornAddRef, AcornRelease,
  AcornDetect, AcornGetArch, AcornGetEndianness, AcornGetEntryPoint,
  AcornLoadImage, AcornGetTlsInfo, AcornGetUnwindInfo,
  AcornGetSymbolByAddress, AcornGetSymbolByName,
  AcornGetRelocInfo, AcornApplyRelocations,
  AcornGetTargetSystem, AcornGetMinimumSystemVersion,
  AcornGetTargetSubsystem, AcornGetMinimumSubsystemVersion,
  AcornGetResource, AcornGetResourceEnumerator
};

IImageLoader gAcornLoader = { &gAcornVtbl };
APXH_REGISTER_IMGLOADER(gAcornLoader);
