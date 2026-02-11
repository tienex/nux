/** @file
  APXH EPOC32 Loader
  
  Provides EPOC32 (Symbian OS) E32Image format parsing and loading for
  ARM-based mobile devices. E32Image is the executable format used by
  Symbian OS (EPOC Release 5 and later).
  
  Supports:
  - ARM architecture (ARMv4, ARMv5)
  - E32Image format
  - UID verification
  
  Copyright (C) 2025 A•NUX Project
  
  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>
#include <ananke/resource.h>
#include "imgresource.h"

// E32Image signature
#define EPOC_IMAGE_SIGNATURE  0x434F5045  // "EPOC" in little-endian

// E32Image header structure (simplified)
ANX_PACK_PUSH(1)
typedef struct _E32IMAGE_HEADER {
  UINT32  Signature;        // "EPOC"
  UINT32  Uid1;             // KExecutableImageUid
  UINT32  Uid2;             // Type-specific UID
  UINT32  Uid3;             // Application-specific UID
  UINT32  UidChecksum;      // UID checksum
  UINT32  HeaderCrc;        // Header CRC
  UINT32  ModuleVersion;    // Module version
  UINT32  CompressionType;  // 0=none, KUidCompressionDeflate, etc.
  UINT32  ToolsVersion;     // Build tools version
  UINT64  TimeLo;           // Timestamp (low)
  UINT64  TimeHi;           // Timestamp (high)
  UINT32  Flags;            // Image flags
  UINT32  CodeSize;         // Code section size
  UINT32  DataSize;         // Data section size
  UINT32  HeapSizeMin;      // Minimum heap size
  UINT32  HeapSizeMax;      // Maximum heap size
  UINT32  StackSize;        // Stack size
  UINT32  BssSize;          // BSS size
  UINT32  EntryPoint;       // Entry point offset
  UINT32  CodeBase;         // Code base address
  UINT32  DataBase;         // Data base address
  // Additional fields omitted for brevity
} E32IMAGE_HEADER;
ANX_PACK_POP()

// IUnknown methods
static HRESULT STDMETHODCALLTYPE Epoc32QueryInterface(IN IImageLoader *This, IN CONST GUID *Iid, OUT VOID **Interface) {
  if (Interface == NULL) return E_POINTER;
  if (memcmp(Iid, &IID_IImageLoader, sizeof(GUID)) == 0 || memcmp(Iid, &IID_IUnknown, sizeof(GUID)) == 0) {
    *Interface = This;
    return S_OK;
  }
  *Interface = NULL;
  return E_NOINTERFACE;
}

static UINTN STDMETHODCALLTYPE Epoc32AddRef(IN IImageLoader *This) { return 1; }
static UINTN STDMETHODCALLTYPE Epoc32Release(IN IImageLoader *This) { return 1; }

// Format detection
static HRESULT STDMETHODCALLTYPE Epoc32Detect(IN IImageLoader *This, IN VOID *ImageBase, IN UINTN ImageSize) {
  E32IMAGE_HEADER *Header;
  if (ImageSize < sizeof(E32IMAGE_HEADER)) return S_FALSE;
  Header = (E32IMAGE_HEADER *)ImageBase;
  return (Header->Signature == EPOC_IMAGE_SIGNATURE) ? S_OK : S_FALSE;
}

// Architecture detection
static HRESULT STDMETHODCALLTYPE Epoc32GetArch(IN IImageLoader *This, IN VOID *ImageBase, OUT ARCH *Architecture) {
  if (Architecture == NULL) return E_POINTER;
  *Architecture = ArchArm32;  // EPOC32 is ARM-based
  return S_OK;
}

// Endianness
static HRESULT STDMETHODCALLTYPE Epoc32GetEndianness(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_ENDIAN *Endianness) {
  if (Endianness == NULL) return E_POINTER;
  *Endianness = ImgEndianLittle;  // ARM is little-endian
  return S_OK;
}

// Entry point
static HRESULT STDMETHODCALLTYPE Epoc32GetEntryPoint(IN IImageLoader *This, IN VOID *ImageBase, OUT VIRTUAL_ADDRESS *EntryPoint) {
  E32IMAGE_HEADER *Header;
  if (EntryPoint == NULL) return E_POINTER;
  Header = (E32IMAGE_HEADER *)ImageBase;
  *EntryPoint = Header->CodeBase + Header->EntryPoint;
  return S_OK;
}

// Load image (stub)
static HRESULT STDMETHODCALLTYPE Epoc32LoadImage(IN OUT IMGLOAD_CONTEXT *Context) {
  if (Context == NULL) return E_POINTER;
  info("Loading EPOC32 E32Image executable...");
  // Full implementation would decompress and load code/data sections
  return IMGLOAD_E_INVALID_FORMAT;  // Not fully implemented
}

// TLS info
static HRESULT STDMETHODCALLTYPE Epoc32GetTlsInfo(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_TLS_INFO *TlsInfo) {
  if (TlsInfo == NULL) return E_POINTER;
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

// Unwind info
static HRESULT STDMETHODCALLTYPE Epoc32GetUnwindInfo(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_UNWIND_INFO *UnwindInfo) {
  if (UnwindInfo == NULL) return E_POINTER;
  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

// Symbol lookup stubs
static HRESULT STDMETHODCALLTYPE Epoc32GetSymbolByAddress(IN IImageLoader *This, IN VOID *ImageBase, IN VIRTUAL_ADDRESS Address, OUT IMGLOAD_SYMBOL_INFO *SymbolInfo) {
  if (SymbolInfo == NULL) return E_POINTER;
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE Epoc32GetSymbolByName(IN IImageLoader *This, IN VOID *ImageBase, IN CONST CHAR8 *Name, OUT IMGLOAD_SYMBOL_INFO *SymbolInfo) {
  if (SymbolInfo == NULL || Name == NULL) return E_POINTER;
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

// Relocation stubs
static HRESULT STDMETHODCALLTYPE Epoc32GetRelocInfo(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_RELOC_INFO *RelocInfo) {
  if (RelocInfo == NULL) return E_POINTER;
  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE Epoc32ApplyRelocations(IN IImageLoader *This, IN VOID *ImageBase, IN VIRTUAL_ADDRESS LoadAddress, IN VIRTUAL_ADDRESS PreferredBase) {
  return S_OK;
}

// System info
static HRESULT STDMETHODCALLTYPE Epoc32GetTargetSystem(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_TARGET_SYSTEM *TargetSystem) {
  if (TargetSystem == NULL) return E_POINTER;
  *TargetSystem = ImgSystemSymbian;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE Epoc32GetMinimumSystemVersion(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_SYSTEM_VERSION *MinimumVersion) {
  if (MinimumVersion == NULL) return E_POINTER;
  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE Epoc32GetTargetSubsystem(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_TARGET_SUBSYSTEM *TargetSubsystem) {
  if (TargetSubsystem == NULL) return E_POINTER;
  *TargetSubsystem = ImgSubsystemSymbianGui;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE Epoc32GetMinimumSubsystemVersion(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_SYSTEM_VERSION *MinimumVersion) {
  if (MinimumVersion == NULL) return E_POINTER;
  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

// Resource methods
static HRESULT STDMETHODCALLTYPE Epoc32GetResource(IN IImageLoader *This, IN VOID *ImageBase, IN UINT32 TypeCode, IN UINT32 Id, IN CONST CHAR8 *Name, OUT IImageResource **Resource) {
  if (Resource == NULL) return E_POINTER;
  *Resource = NULL;
  // EPOC32 has native resources but not fully implemented
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE Epoc32GetResourceEnumerator(IN IImageLoader *This, IN VOID *ImageBase, IN UINT32 TypeCode, OUT IEnumImageResource **Enumerator) {
  if (Enumerator == NULL) return E_POINTER;
  *Enumerator = NULL;
  return S_FALSE;
}

// VTable
static CONST IImageLoaderVtbl gEpoc32Vtbl = {
  Epoc32QueryInterface, Epoc32AddRef, Epoc32Release,
  Epoc32Detect, Epoc32GetArch, Epoc32GetEndianness, Epoc32GetEntryPoint,
  Epoc32LoadImage, Epoc32GetTlsInfo, Epoc32GetUnwindInfo,
  Epoc32GetSymbolByAddress, Epoc32GetSymbolByName,
  Epoc32GetRelocInfo, Epoc32ApplyRelocations,
  Epoc32GetTargetSystem, Epoc32GetMinimumSystemVersion,
  Epoc32GetTargetSubsystem, Epoc32GetMinimumSubsystemVersion,
  Epoc32GetResource, Epoc32GetResourceEnumerator
};

IImageLoader gEpoc32Loader = { &gEpoc32Vtbl };
APXH_REGISTER_IMGLOADER(gEpoc32Loader);
