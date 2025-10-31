/** @file
  APXH PalmOS Loader
  
  Provides PalmOS PRC (Palm Resource Code) format parsing and loading for
  Palm OS applications. PRC files contain 68K or ARM code and resources.
  
  Supports:
  - PalmOS PRC format
  - M68K architecture (Palm OS 1.0-4.x)
  - ARM architecture (Palm OS 5.x+)
  
  Copyright (C) 2025 A•NUX Project
  
  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>
#include <ananke/resource.h>
#include "imgresource.h"

// PRC database header
ANX_PACK_PUSH(1)
typedef struct _PRC_HEADER {
  CHAR8   Name[32];         // Database name
  UINT16  Attributes;       // Database attributes
  UINT16  Version;          // Application version
  UINT32  CreationDate;     // Creation date (Mac format)
  UINT32  ModificationDate; // Modification date
  UINT32  LastBackupDate;   // Last backup date
  UINT32  ModificationNumber; // Modification number
  UINT32  AppInfoOffset;    // Application info offset
  UINT32  SortInfoOffset;   // Sort info offset
  UINT32  Type;             // Database type ('appl')
  UINT32  Creator;          // Creator ID
  UINT32  UniqueIDSeed;     // Unique ID seed
  UINT32  NextRecordList;   // Next record list
  UINT16  NumRecords;       // Number of records
} PRC_HEADER;

typedef struct _PRC_RECORD_ENTRY {
  UINT32  Offset;           // Record offset
  UINT8   Attributes;       // Record attributes
  UINT8   UniqueID[3];      // Unique ID (24-bit)
} PRC_RECORD_ENTRY;
ANX_PACK_POP()

#define PRC_TYPE_APPL  0x6170706C  // 'appl'
#define PRC_TYPE_GLIB  0x676C6962  // 'glib' (shared library)

// IUnknown methods
static HRESULT STDMETHODCALLTYPE PalmOsQueryInterface(IN IImageLoader *This, IN CONST GUID *Iid, OUT VOID **Interface) {
  if (Interface == NULL) return E_POINTER;
  if (memcmp(Iid, &IID_IImageLoader, sizeof(GUID)) == 0 || memcmp(Iid, &IID_IUnknown, sizeof(GUID)) == 0) {
    *Interface = This;
    return S_OK;
  }
  *Interface = NULL;
  return E_NOINTERFACE;
}

static UINTN STDMETHODCALLTYPE PalmOsAddRef(IN IImageLoader *This) { return 1; }
static UINTN STDMETHODCALLTYPE PalmOsRelease(IN IImageLoader *This) { return 1; }

// Format detection
static HRESULT STDMETHODCALLTYPE PalmOsDetect(IN IImageLoader *This, IN VOID *ImageBase, IN UINTN ImageSize) {
  PRC_HEADER *Header;
  if (ImageSize < sizeof(PRC_HEADER)) return S_FALSE;
  Header = (PRC_HEADER *)ImageBase;
  return (Header->Type == PRC_TYPE_APPL || Header->Type == PRC_TYPE_GLIB) ? S_OK : S_FALSE;
}

// Architecture detection  
static HRESULT STDMETHODCALLTYPE PalmOsGetArch(IN IImageLoader *This, IN VOID *ImageBase, OUT ARCH *Architecture) {
  if (Architecture == NULL) return E_POINTER;
  // Default to M68K (most common); ARM detection would require code resource inspection
  *Architecture = ArchM68k;
  return S_OK;
}

// Endianness
static HRESULT STDMETHODCALLTYPE PalmOsGetEndianness(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_ENDIAN *Endianness) {
  if (Endianness == NULL) return E_POINTER;
  *Endianness = ImgEndianBig;  // PalmOS PRC is big-endian
  return S_OK;
}

// Entry point
static HRESULT STDMETHODCALLTYPE PalmOsGetEntryPoint(IN IImageLoader *This, IN VOID *ImageBase, OUT VIRTUAL_ADDRESS *EntryPoint) {
  if (EntryPoint == NULL) return E_POINTER;
  *EntryPoint = 0;  // Entry point in code resource #1
  return S_OK;
}

// Load image (stub)
static HRESULT STDMETHODCALLTYPE PalmOsLoadImage(IN OUT IMGLOAD_CONTEXT *Context) {
  if (Context == NULL) return E_POINTER;
  info("Loading PalmOS PRC executable...");
  return IMGLOAD_E_INVALID_FORMAT;  // Not fully implemented
}

// TLS info
static HRESULT STDMETHODCALLTYPE PalmOsGetTlsInfo(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_TLS_INFO *TlsInfo) {
  if (TlsInfo == NULL) return E_POINTER;
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

// Unwind info
static HRESULT STDMETHODCALLTYPE PalmOsGetUnwindInfo(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_UNWIND_INFO *UnwindInfo) {
  if (UnwindInfo == NULL) return E_POINTER;
  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

// Symbol lookup stubs
static HRESULT STDMETHODCALLTYPE PalmOsGetSymbolByAddress(IN IImageLoader *This, IN VOID *ImageBase, IN VIRTUAL_ADDRESS Address, OUT IMGLOAD_SYMBOL_INFO *SymbolInfo) {
  if (SymbolInfo == NULL) return E_POINTER;
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE PalmOsGetSymbolByName(IN IImageLoader *This, IN VOID *ImageBase, IN CONST CHAR8 *Name, OUT IMGLOAD_SYMBOL_INFO *SymbolInfo) {
  if (SymbolInfo == NULL || Name == NULL) return E_POINTER;
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

// Relocation stubs
static HRESULT STDMETHODCALLTYPE PalmOsGetRelocInfo(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_RELOC_INFO *RelocInfo) {
  if (RelocInfo == NULL) return E_POINTER;
  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE PalmOsApplyRelocations(IN IImageLoader *This, IN VOID *ImageBase, IN VIRTUAL_ADDRESS LoadAddress, IN VIRTUAL_ADDRESS PreferredBase) {
  return S_OK;
}

// System info
static HRESULT STDMETHODCALLTYPE PalmOsGetTargetSystem(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_TARGET_SYSTEM *TargetSystem) {
  if (TargetSystem == NULL) return E_POINTER;
  *TargetSystem = ImgSystemPalmOs;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE PalmOsGetMinimumSystemVersion(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_SYSTEM_VERSION *MinimumVersion) {
  if (MinimumVersion == NULL) return E_POINTER;
  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE PalmOsGetTargetSubsystem(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_TARGET_SUBSYSTEM *TargetSubsystem) {
  if (TargetSubsystem == NULL) return E_POINTER;
  *TargetSubsystem = ImgSubsystemPalmOsGui;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE PalmOsGetMinimumSubsystemVersion(IN IImageLoader *This, IN VOID *ImageBase, OUT IMGLOAD_SYSTEM_VERSION *MinimumVersion) {
  if (MinimumVersion == NULL) return E_POINTER;
  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

// Resource methods (PalmOS has native resources)
static HRESULT STDMETHODCALLTYPE PalmOsGetResource(IN IImageLoader *This, IN VOID *ImageBase, IN UINT32 TypeCode, IN UINT32 Id, IN CONST CHAR8 *Name, OUT IImageResource **Resource) {
  if (Resource == NULL) return E_POINTER;
  *Resource = NULL;
  // PalmOS PRC has native resource database but not fully implemented
  return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE PalmOsGetResourceEnumerator(IN IImageLoader *This, IN VOID *ImageBase, IN UINT32 TypeCode, OUT IEnumImageResource **Enumerator) {
  if (Enumerator == NULL) return E_POINTER;
  *Enumerator = NULL;
  return S_FALSE;
}

// VTable
static CONST IImageLoaderVtbl gPalmOsVtbl = {
  PalmOsQueryInterface, PalmOsAddRef, PalmOsRelease,
  PalmOsDetect, PalmOsGetArch, PalmOsGetEndianness, PalmOsGetEntryPoint,
  PalmOsLoadImage, PalmOsGetTlsInfo, PalmOsGetUnwindInfo,
  PalmOsGetSymbolByAddress, PalmOsGetSymbolByName,
  PalmOsGetRelocInfo, PalmOsApplyRelocations,
  PalmOsGetTargetSystem, PalmOsGetMinimumSystemVersion,
  PalmOsGetTargetSubsystem, PalmOsGetMinimumSubsystemVersion,
  PalmOsGetResource, PalmOsGetResourceEnumerator
};

IImageLoader gPalmOsLoader = { &gPalmOsVtbl };
APXH_REGISTER_IMGLOADER(gPalmOsLoader);
