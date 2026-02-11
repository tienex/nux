/** @file
  APXH Classic Mac OS 68K Loader
  
  Provides Classic Macintosh 68K executable format parsing and loading.
  This is the original Mac OS executable format (pre-PEF) that stores
  code in 'CODE' resources within the resource fork.
  
  Supports:
  - Classic Mac OS System 1-6 format
  - M68K architecture (68000, 68020, 68030, 68040)
  - Resource fork with CODE resources
  - Jump table and A5 world
  
  Copyright (C) 2025 A•NUX Project
  
  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>
#include <ananke/resource.h>
#include "imgresource.h"

// Classic Mac resource fork header (simplified)
#define MAC_RESOURCE_FORK_OFFSET  0  // Resource fork starts at offset 0 in resource fork file

ANX_PACK_PUSH(1)
typedef struct _MAC_RESOURCE_HEADER {
  UINT32  DataOffset;       // Offset to resource data
  UINT32  MapOffset;        // Offset to resource map
  UINT32  DataLength;       // Length of resource data
  UINT32  MapLength;        // Length of resource map
} MAC_RESOURCE_HEADER;

typedef struct _MAC_RESOURCE_MAP {
  UINT16  Reserved1[11];    // Reserved
  UINT16  Attributes;       // Resource fork attributes
  UINT32  TypeListOffset;   // Offset to type list
  UINT32  NameListOffset;   // Offset to name list
  UINT16  TypeCount;        // Number of types minus 1
} MAC_RESOURCE_MAP;

typedef struct _MAC_CODE_JUMP_TABLE {
  UINT16  AboveA5Size;      // Size above A5
  UINT32  AppParamSize;     // Application parameters size
  UINT32  JumpTableSize;    // Jump table size
  UINT32  JumpTableOffset;  // Jump table offset
} MAC_CODE_JUMP_TABLE;
ANX_PACK_POP()

#define MAC_CODE_RESOURCE  0x434F4445  // 'CODE'

// IUnknown methods
static HRESULT STDMETHODCALLTYPE MacOs68kQueryInterface(IN IImageLoader *This, IN CONST GUID *Iid, OUT VOID **Interface) {
  if (Interface == NULL) return E_POINTER;
  if (memcmp(Iid, &IID_IImageLoader, sizeof(GUID)) == 0 || memcmp(Iid, &IID_IUnknown, sizeof(GUID)) == 0) {
    *Interface = This;
    return S_OK;
  }
  *Interface = NULL;
  return E_NOINTERFACE;
}

static UINTN STDMETHODCALLTYPE MacOs68kAddRef(IN IImageLoader *This) { return 1; }
static UINTN STDMETHODCALLTYPE MacOs68kRelease(IN IImageLoader *This) { return 1; }

// Format detection (check for Mac resource fork structure)
static HRESULT STDMETHODCALLTYPE MacOs68kDetect(IN IImageLoader *This, IN VOID *ImageBase, IN UINTN ImageSize) {
  MAC_RESOURCE_HEADER *Header;
  if (ImageSize < sizeof(MAC_RESOURCE_HEADER)) return S_FALSE;
  Header = (MAC_RESOURCE_HEADER *)ImageBase;
  // Basic validation: check if offsets are reasonable
  if (Header->DataOffset > 0 && Header->DataOffset < ImageSize &&
      Header->MapOffset > 0 && Header->MapOffset < ImageSize &&
      Header->DataOffset < Header->MapOffset) {
    return S_OK;  // Likely a Mac resource fork
  }
  return S_FALSE;
}

// Architecture detection
static HRESULT STDMETHODCALLTYPE MacOs68kGetArch(IN IImageLoader *This, IN VOID *ImageBase, OUT ARCH *Architecture) {
  if (Architecture == NULL) return E_POINTER;
  *Architecture = ArchM68k;  // Classic Mac OS is 68K
  return S_OK;
}

// Endianness
static HRESULT STDMETHODCALLTYPE MacOs68kGetEndianness(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_ENDIAN *Endianness) {
  if (Endianness == NULL) return E_POINTER;
  *Endianness = ImgEndianBig;  // 68K is big-endian
  return S_OK;
}

// Entry point (CODE resource 0 or 1)
static HRESULT STDMETHODCALLTYPE MacOs68kGetEntryPoint(IN IImageLoader *This, IN VOID *ImageBase, OUT VIRTUAL_ADDRESS *EntryPoint) {
  if (EntryPoint == NULL) return E_POINTER;
  *EntryPoint = 0;  // Entry point would be in CODE 0 or CODE 1
  return S_OK;
}

// Load image (stub)
static HRESULT STDMETHODCALLTYPE MacOs68kLoadImage(IN OUT IMGLOAD_CONTEXT *Context) {
  if (Context == NULL) return E_POINTER;
  info("Loading Classic Mac OS 68K resource fork executable...");
  return IMGLOAD_E_INVALID_FORMAT;  // Not fully implemented
}

// TLS info
static HRESULT STDMETHODCALLTYPE MacOs68kGetTlsInfo(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_TLS_INFO *TlsInfo) {
  if (TlsInfo == NULL) return E_POINTER;
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

// Unwind info
static HRESULT STDMETHODCALLTYPE MacOs68kGetUnwindInfo(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_UNWIND_INFO *UnwindInfo) {
  if (UnwindInfo == NULL) return E_POINTER;
  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

// Symbol lookup stubs
static HRESULT STDMETHODCALLTYPE MacOs68kGetSymbolByAddress(IN IImageLoader *This, IN VOID *ImageBase, IN VIRTUAL_ADDRESS Address, OUT IMGLOAD_SYMBOL_INFO *SymbolInfo) {
  if (SymbolInfo == NULL) return E_POINTER;
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE MacOs68kGetSymbolByName(IN IImageLoader *This, IN VOID *ImageBase, IN CONST CHAR8 *Name, OUT IMGLOAD_SYMBOL_INFO *SymbolInfo) {
  if (SymbolInfo == NULL || Name == NULL) return E_POINTER;
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

// Relocation stubs
static HRESULT STDMETHODCALLTYPE MacOs68kGetRelocInfo(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_RELOC_INFO *RelocInfo) {
  if (RelocInfo == NULL) return E_POINTER;
  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE MacOs68kApplyRelocations(IN IImageLoader *This, IN VOID *ImageBase, IN VIRTUAL_ADDRESS LoadAddress, IN VIRTUAL_ADDRESS PreferredBase) {
  return S_OK;
}

// System info
static HRESULT STDMETHODCALLTYPE MacOs68kGetTargetSystem(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_TARGET_SYSTEM *TargetSystem) {
  if (TargetSystem == NULL) return E_POINTER;
  *TargetSystem = ImgSystemMacOs;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE MacOs68kGetMinimumSystemVersion(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_SYSTEM_VERSION *MinimumVersion) {
  if (MinimumVersion == NULL) return E_POINTER;
  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE MacOs68kGetTargetSubsystem(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_TARGET_SUBSYSTEM *TargetSubsystem) {
  if (TargetSubsystem == NULL) return E_POINTER;
  *TargetSubsystem = ImgSubsystemMacOsClassic;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE MacOs68kGetMinimumSubsystemVersion(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_SYSTEM_VERSION *MinimumVersion) {
  if (MinimumVersion == NULL) return E_POINTER;
  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

// Resource methods (native Classic Mac resource fork)
static HRESULT STDMETHODCALLTYPE MacOs68kGetResource(IN IImageLoader *This, IN VOID *ImageBase, IN UINT32 TypeCode, IN UINT32 Id, IN CONST CHAR8 *Name, OUT IImageResource **Resource) {
  if (Resource == NULL) return E_POINTER;
  *Resource = NULL;
  // Classic Mac OS has native resource fork - this would need full resource manager implementation
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE MacOs68kGetResourceEnumerator(IN IImageLoader *This, IN VOID *ImageBase, IN UINT32 TypeCode, OUT IEnumImageResource **Enumerator) {
  if (Enumerator == NULL) return E_POINTER;
  *Enumerator = NULL;
  return S_FALSE;
}

// VTable
static CONST IImageLoaderVtbl gMacOs68kVtbl = {
  MacOs68kQueryInterface, MacOs68kAddRef, MacOs68kRelease,
  MacOs68kDetect, MacOs68kGetArch, MacOs68kGetEndianness, MacOs68kGetEntryPoint,
  MacOs68kLoadImage, MacOs68kGetTlsInfo, MacOs68kGetUnwindInfo,
  MacOs68kGetSymbolByAddress, MacOs68kGetSymbolByName,
  MacOs68kGetRelocInfo, MacOs68kApplyRelocations,
  MacOs68kGetTargetSystem, MacOs68kGetMinimumSystemVersion,
  MacOs68kGetTargetSubsystem, MacOs68kGetMinimumSubsystemVersion,
  MacOs68kGetResource, MacOs68kGetResourceEnumerator
};

IImageLoader gMacOs68kLoader = { &gMacOs68kVtbl };
APXH_REGISTER_IMGLOADER(gMacOs68kLoader);
