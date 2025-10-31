/** @file
  APXH PEF Loader

  Provides PEF (Preferred Executable Format) parsing and loading for
  classic Mac OS and BeOS PowerPC executables. PEF was developed by
  Apple Computer for the Code Fragment Manager (CFM).

  Supports:
  - PowerPC architecture
  - Motorola 68K architecture (m68k)
  - Instantiated data sections
  - Unpacked data sections

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>
#include <ananke/resource.h>
#include "imgresource.h"

//
// PEF Magic Numbers
//

#define PEF_TAG1    0x4A6F7921  ///< "Joy!" in ASCII
#define PEF_TAG2    0x70656666  ///< "peff" in ASCII

//
// PEF Architecture Tags
//

#define PEF_ARCH_PWPC   0x70777063  ///< "pwpc" - PowerPC
#define PEF_ArchM68k   0x6D36386B  ///< "m68k" - Motorola 68K

//
// PEF Section Types
//

#define kPEFCodeSection             0  ///< Code section
#define kPEFUnpackedDataSection     1  ///< Unpacked data section
#define kPEFPackedDataSection       2  ///< Packed data section
#define kPEFConstantSection         3  ///< Constant (read-only) section
#define kPEFLoaderSection           4  ///< Loader section
#define kPEFDebugSection            5  ///< Debug section
#define kPEFExecutableDataSection   6  ///< Executable data section
#define kPEFExceptionSection        7  ///< Exception section
#define kPEFTracebackSection        8  ///< Traceback section

//
// PEF Section Sharing
//

#define kPEFProcessShare            1  ///< Process share
#define kPEFGlobalShare             4  ///< Global share
#define kPEFProtectedShare          5  ///< Protected share

//
// PEF Structures
//

ANX_PACK_PUSH(1)

typedef struct _PEF_CONTAINER_HEADER {
  UINT32  Tag1;               ///< "Joy!" magic
  UINT32  Tag2;               ///< "peff" magic
  UINT32  Architecture;       ///< CPU architecture ("pwpc" or "m68k")
  UINT32  FormatVersion;      ///< Format version
  UINT32  DateTimeStamp;      ///< Creation timestamp (Mac time)
  UINT32  OldDefVersion;      ///< Old definition version
  UINT32  OldImpVersion;      ///< Old implementation version
  UINT32  CurrentVersion;     ///< Current version
  UINT16  SectionCount;       ///< Number of sections
  UINT16  InstSectionCount;   ///< Number of instantiated sections
  UINT32  Reserved;           ///< Reserved
} PEF_CONTAINER_HEADER;

typedef struct _PEF_SECTION_HEADER {
  INT32   NameOffset;         ///< Section name offset (or -1)
  UINT32  DefaultAddress;     ///< Default address
  UINT32  TotalSize;          ///< Total size in bytes
  UINT32  UnpackedSize;       ///< Unpacked size in bytes
  UINT32  PackedSize;         ///< Packed size in bytes
  UINT32  ContainerOffset;    ///< Offset in container
  UINT8   SectionKind;        ///< Section type
  UINT8   ShareKind;          ///< Share mode
  UINT8   Alignment;          ///< Alignment (power of 2)
  UINT8   Reserved;           ///< Reserved
} PEF_SECTION_HEADER;

typedef struct _PEF_LOADER_INFO_HEADER {
  INT32   MainSection;        ///< Main section index
  UINT32  MainOffset;         ///< Main offset
  INT32   InitSection;        ///< Init section index
  UINT32  InitOffset;         ///< Init offset
  INT32   TermSection;        ///< Term section index
  UINT32  TermOffset;         ///< Term offset
  UINT32  ImportedLibraryCount;   ///< Number of imported libraries
  UINT32  TotalImportedSymbolCount; ///< Total imported symbols
  UINT32  RelocSectionCount;      ///< Number of reloc sections
  UINT32  RelocInstrOffset;       ///< Relocation instructions offset
  UINT32  LoaderStringsOffset;    ///< Loader strings offset
  UINT32  ExportHashOffset;       ///< Export hash offset
  UINT32  ExportHashTablePower;   ///< Export hash table power
  UINT32  ExportedSymbolCount;    ///< Number of exported symbols
} PEF_LOADER_INFO_HEADER;

ANX_PACK_POP()

//
// Helper Macros
//

#define PEF_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// Internal Functions
//

/**
  Check if image is PEF format.
**/
static
HRESULT
STDMETHODCALLTYPE
PefDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  PEF_CONTAINER_HEADER *Header;

  if (ImageSize < sizeof(PEF_CONTAINER_HEADER)) {
    return S_FALSE;
  }

  Header = (PEF_CONTAINER_HEADER *)ImageBase;

  return (Header->Tag1 == PEF_TAG1 &&
          Header->Tag2 == PEF_TAG2) ? S_OK : S_FALSE;
}

/**
  Get architecture from PEF image.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  PEF_CONTAINER_HEADER *Header;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  Header = (PEF_CONTAINER_HEADER *)ImageBase;

  switch (Header->Architecture) {
    case PEF_ARCH_PWPC:
      *Architecture = ArchPpc32;
      return S_OK;

    case PEF_ArchM68k:
      *Architecture = ArchM68k;
      return S_OK;

    default:
      *Architecture = ArchUnsupported;
      return IMGLOAD_E_UNSUPPORTED_ARCH;
  }
}

/**
  Get endianness from PEF image.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // PEF is always big-endian (PowerPC and 68K are big-endian architectures)
  *Endianness = ImgEndianBig;
  return S_OK;
}

/**
  Get entry point from PEF image.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  PEF_CONTAINER_HEADER *Header;
  PEF_SECTION_HEADER *Sections;
  PEF_LOADER_INFO_HEADER *LoaderInfo = NULL;
  UINT32 i;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  Header = (PEF_CONTAINER_HEADER *)ImageBase;
  Sections = (PEF_SECTION_HEADER *)(Header + 1);

  // Find loader section
  for (i = 0; i < Header->SectionCount; i++) {
    if (Sections[i].SectionKind == kPEFLoaderSection) {
      LoaderInfo = (PEF_LOADER_INFO_HEADER *)PEF_OFF(Sections[i].ContainerOffset);
      break;
    }
  }

  if (LoaderInfo == NULL || LoaderInfo->MainSection < 0) {
    *EntryPoint = 0;
    return IMGLOAD_E_INVALID_HEADER;
  }

  // Calculate entry point from main section and offset
  *EntryPoint = Sections[LoaderInfo->MainSection].DefaultAddress + LoaderInfo->MainOffset;
  return S_OK;
}

/**
  Load PEF section.
**/
static
VOID
PefLoadSection (
  IN VOID                *ImageBase,
  IN PEF_SECTION_HEADER  *Section,
  IN BOOLEAN             IsUserMode
  )
{
  BOOLEAN IsWritable = FALSE;
  BOOLEAN IsExecutable = FALSE;

  if (Section->TotalSize == 0) {
    return;
  }

  // Determine section permissions
  switch (Section->SectionKind) {
    case kPEFCodeSection:
      IsExecutable = TRUE;
      break;

    case kPEFUnpackedDataSection:
    case kPEFPackedDataSection:
    case kPEFExecutableDataSection:
      IsWritable = TRUE;
      if (Section->SectionKind == kPEFExecutableDataSection) {
        IsExecutable = TRUE;
      }
      break;

    case kPEFConstantSection:
      // Read-only
      break;

    default:
      // Skip loader, debug, and other sections
      return;
  }

  info("  Section type %d at 0x%08x (size: 0x%08x, alignment: %d)",
       Section->SectionKind, Section->DefaultAddress,
       Section->TotalSize, 1 << Section->Alignment);

  if (Section->SectionKind == kPEFPackedDataSection) {
    // Packed data needs decompression (not implemented)
    warn("Packed PEF sections not supported");
    return;
  }

  if (Section->UnpackedSize > 0 && Section->ContainerOffset != 0) {
    // Copy section data
    VirtualAddressCopy(
      Section->DefaultAddress,
      PEF_OFF(Section->ContainerOffset),
      Section->UnpackedSize,
      IsUserMode,
      IsWritable,
      IsExecutable
    );
  }

  if (Section->TotalSize > Section->UnpackedSize) {
    // Zero-fill remainder
    VirtualAddressMemset(
      Section->DefaultAddress + Section->UnpackedSize,
      0,
      Section->TotalSize - Section->UnpackedSize,
      IsUserMode,
      IsWritable,
      IsExecutable
    );
  }
}

/**
  Load PEF image.
**/
static
HRESULT
STDMETHODCALLTYPE
PefLoadImage (
  IN     IImageLoader     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  PEF_CONTAINER_HEADER *Header;
  PEF_SECTION_HEADER *Sections;
  UINT32 i;
  HRESULT Status;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  Header = (PEF_CONTAINER_HEADER *)ImageBase;

  info("Loading PEF %s executable...",
       Header->Architecture == PEF_ARCH_PWPC ? "PowerPC" : "68K");

  // Populate architecture and endianness
  Status = PefGetArch(This, ImageBase, &Context->Architecture);
  if (FAILED(Status)) {
    return Status;
  }

  Status = PefGetEndianness(This, ImageBase, &Context->Endianness);
  if (FAILED(Status)) {
    return Status;
  }

  Sections = (PEF_SECTION_HEADER *)(Header + 1);

  // Load instantiated sections only
  for (i = 0; i < Header->InstSectionCount; i++) {
    PefLoadSection(ImageBase, &Sections[i], Context->IsUserMode);
  }

  Status = PefGetEntryPoint(This, ImageBase, &Context->EntryPoint);
  if (FAILED(Status)) {
    return Status;
  }

  return S_OK;
}

/**
  Extract TLS information from PEF image.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // PEF doesn't have explicit TLS support
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from PEF image.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  // PEF doesn't have standardized unwind information
  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  // PEF symbol lookup not implemented
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  // PEF symbol lookup not implemented
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Extract relocation information from PEF image.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetRelocInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_RELOC_INFO   *RelocInfo
  )
{
  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  // PEF has relocations but they're handled internally
  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));
  return S_FALSE;
}

/**
  Apply relocations to PEF image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
PefApplyRelocations (
  IN VOID              *ImageBase,
  IN VIRTUAL_ADDRESS   LoadAddress,
  IN VIRTUAL_ADDRESS   PreferredBase
  )
{
  // PEF relocations are applied during load
  // No additional relocation needed
  return S_OK;
}

/**
  Find section by name in PEF image.
**/
static
HRESULT
PefFindSection (
  IN  VOID         *ImageBase,
  IN  CONST CHAR8  *Name,
  OUT VOID         **Data,
  OUT UINT64       *Size
  )
{
  PEF_CONTAINER_HEADER *Header;
  PEF_SECTION_HEADER *Sections;
  PEF_LOADER_INFO_HEADER *LoaderInfo;
  UINT32 i;
  CHAR8 *SectionName;
  UINT8 *LoaderSection;

  if (Name == NULL || Data == NULL || Size == NULL) {
    return E_POINTER;
  }

  Header = (PEF_CONTAINER_HEADER *)ImageBase;
  Sections = (PEF_SECTION_HEADER *)(Header + 1);

  // Find loader section for string table
  LoaderSection = NULL;
  for (i = 0; i < Header->SectionCount; i++) {
    if (Sections[i].SectionKind == kPEFLoaderSection) {
      LoaderSection = (UINT8 *)ImageBase + Sections[i].ContainerOffset;
      break;
    }
  }

  if (LoaderSection == NULL) {
    return S_FALSE;
  }

  LoaderInfo = (PEF_LOADER_INFO_HEADER *)LoaderSection;

  // Search sections
  for (i = 0; i < Header->SectionCount; i++) {
    if (Sections[i].NameOffset >= 0) {
      // Section has a name - look it up in loader strings
      SectionName = (CHAR8 *)LoaderSection + LoaderInfo->LoaderStringsOffset + Sections[i].NameOffset;

      if (strcmp(SectionName, Name) == 0) {
        *Data = (UINT8 *)ImageBase + Sections[i].ContainerOffset;
        *Size = Sections[i].PackedSize;
        return S_OK;
      }
    }
  }

  return S_FALSE;
}

/**
  Extract initialization and termination function addresses from PEF image.
**/
static
HRESULT
PefGetInitFini (
  IN  VOID     *ImageBase,
  OUT UINT64   *InitAddress,
  OUT UINT64   *TermAddress
  )
{
  PEF_CONTAINER_HEADER *Header;
  PEF_SECTION_HEADER *Sections;
  PEF_LOADER_INFO_HEADER *LoaderInfo;
  UINT32 i;
  UINT8 *LoaderSection;

  if (InitAddress == NULL || TermAddress == NULL) {
    return E_POINTER;
  }

  *InitAddress = 0;
  *TermAddress = 0;

  Header = (PEF_CONTAINER_HEADER *)ImageBase;
  Sections = (PEF_SECTION_HEADER *)(Header + 1);

  // Find loader section
  LoaderSection = NULL;
  for (i = 0; i < Header->SectionCount; i++) {
    if (Sections[i].SectionKind == kPEFLoaderSection) {
      LoaderSection = (UINT8 *)ImageBase + Sections[i].ContainerOffset;
      break;
    }
  }

  if (LoaderSection == NULL) {
    return S_FALSE;
  }

  LoaderInfo = (PEF_LOADER_INFO_HEADER *)LoaderSection;

  // Extract init function address
  if (LoaderInfo->InitSection >= 0 && (UINT32)LoaderInfo->InitSection < Header->SectionCount) {
    *InitAddress = Sections[LoaderInfo->InitSection].DefaultAddress + LoaderInfo->InitOffset;
  }

  // Extract term function address
  if (LoaderInfo->TermSection >= 0 && (UINT32)LoaderInfo->TermSection < Header->SectionCount) {
    *TermAddress = Sections[LoaderInfo->TermSection].DefaultAddress + LoaderInfo->TermOffset;
  }

  return (*InitAddress != 0 || *TermAddress != 0) ? S_OK : S_FALSE;
}

/**
  Get resource from PEF image.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetResource (
  IN  IImageLoader   *This,
  IN  VOID           *ImageBase,
  IN  UINT32         TypeCode,
  IN  UINT32         Id,
  IN  CONST CHAR8    *Name,
  OUT IImageResource **Resource
  )
{
  VOID *ResourceFork;
  UINT64 Size;
  BOOLEAN NeedsFree;
  HRESULT Status;

  if (Resource == NULL) {
    return E_POINTER;
  }

  *Resource = NULL;

  // PEF doesn't have native resources, use universal resource fork
  Status = FindUniversalResourceFork(
    ImageBase,
    ResourceStrategyDirect,
    NULL,
    PefFindSection,
    ".axursrc",
    &ResourceFork,
    &Size,
    &NeedsFree
  );

  if (Status != S_OK) {
    return Status;
  }

  Status = CreateImageResource(ResourceFork, TypeCode, (UINT16)Id, Name, Resource);

  if (NeedsFree) {
    free(ResourceFork);
  }

  return Status;
}

/**
  Get resource enumerator for PEF image.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetResourceEnumerator (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  IN  UINT32              TypeCode,
  OUT IEnumImageResource  **Enumerator
  )
{
  VOID *ResourceFork;
  UINT64 Size;
  BOOLEAN NeedsFree;
  HRESULT Status;

  if (Enumerator == NULL) {
    return E_POINTER;
  }

  *Enumerator = NULL;

  // PEF doesn't have native resources, use universal resource fork
  Status = FindUniversalResourceFork(
    ImageBase,
    ResourceStrategyDirect,
    NULL,
    PefFindSection,
    ".axursrc",
    &ResourceFork,
    &Size,
    &NeedsFree
  );

  if (Status != S_OK) {
    return Status;
  }

  Status = CreateImageResourceEnumerator(ResourceFork, TypeCode, Enumerator);

  if (NeedsFree) {
    free(ResourceFork);
  }

  return Status;
}

/**
  IUnknown::QueryInterface implementation (stub).
**/
static
HRESULT
STDMETHODCALLTYPE
PefQueryInterface (
  IN  IImageLoader  *This,
  IN  REFIID        riid,
  OUT VOID          **ppvObject
  )
{
  // Simple implementation - only support IImageLoader and IUnknown
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  *ppvObject = NULL;

  // Compare GUIDs (simplified)
  if (memcmp(riid, &IID_IImageLoader, sizeof(GUID)) == 0 ||
      memcmp(riid, &IID_IUnknown, sizeof(GUID)) == 0) {
    *ppvObject = This;
    return S_OK;
  }

  return E_NOINTERFACE;
}

/**
  IUnknown::AddRef implementation (stub - static object).
**/
static
UINT32
STDMETHODCALLTYPE
PefAddRef (
  IN IImageLoader  *This
  )
{
  // Static object, no reference counting
  return 1;
}

/**
  IUnknown::Release implementation (stub - static object).
**/
static
UINT32
STDMETHODCALLTYPE
PefRelease (
  IN IImageLoader  *This
  )
{
  // Static object, no reference counting
  return 1;
}

/**
  Detect if a PEF binary is for BeOS by checking for BeOS-specific characteristics.

  Checks for BeOS-specific library imports in the loader section.
  BeOS libraries typically include: libroot.so, libbe.so, libstdc++.r4.so

  @param[in]  ImageBase  Base address of the PEF image.

  @retval TRUE   Binary appears to be for BeOS.
  @retval FALSE  Binary appears to be for Mac OS or cannot determine.
**/
static
BOOLEAN
PefIsBeOsBinary (
  IN VOID  *ImageBase
  )
{
  PEF_CONTAINER_HEADER *Container;
  PEF_SECTION_HEADER *Sections;
  PEF_LOADER_INFO_HEADER *LoaderInfo;
  UINT32 I;
  UINT8 *LoaderSection;
  CHAR8 *LibName;
  UINT32 *ImportLibOffsets;

  Container = (PEF_CONTAINER_HEADER *)ImageBase;
  Sections = (PEF_SECTION_HEADER *)((UINT8 *)ImageBase + sizeof(PEF_CONTAINER_HEADER));

  // Look for loader section
  LoaderSection = NULL;
  for (I = 0; I < Container->SectionCount; I++) {
    if (Sections[I].SectionKind == kPEFLoaderSection) {
      LoaderSection = (UINT8 *)ImageBase + Sections[I].ContainerOffset;
      break;
    }
  }

  if (LoaderSection == NULL) {
    return FALSE;  // No loader section, default to Mac OS
  }

  LoaderInfo = (PEF_LOADER_INFO_HEADER *)LoaderSection;
  if (LoaderInfo->ImportedLibraryCount == 0) {
    return FALSE;  // No imports, default to Mac OS
  }

  // Get pointer to imported library name offsets
  // They're stored after the loader info header
  ImportLibOffsets = (UINT32 *)(LoaderInfo + 1);

  // Check first few library names for BeOS-specific libraries
  for (I = 0; I < LoaderInfo->ImportedLibraryCount && I < 5; I++) {
    // Library names are stored in the loader strings section
    LibName = (CHAR8 *)LoaderSection + LoaderInfo->LoaderStringsOffset + ImportLibOffsets[I];

    // Check for BeOS-specific library names
    if (memcmp(LibName, "libroot.so", 10) == 0 ||
        memcmp(LibName, "libbe.so", 8) == 0 ||
        memcmp(LibName, "libstdc++.r4.so", 15) == 0 ||
        memcmp(LibName, "libnet.so", 9) == 0) {
      return TRUE;  // Found BeOS library
    }
  }

  return FALSE;  // No BeOS libraries found, assume Mac OS
}

//
// PEF Loader VTable
//

#ifdef __cplusplus

/**
  Get target operating system from Pef image.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetTargetSystem (
  IN  IImageLoader           *This,
  IN  VOID                   *ImageBase,
  OUT IMGLOAD_TARGET_SYSTEM  *TargetSystem
  )
{
  if (TargetSystem == NULL) {
    return E_POINTER;
  }

  // PEF was used on both Mac OS and BeOS PowerPC
  // Detect based on imported libraries
  if (PefIsBeOsBinary(ImageBase)) {
    *TargetSystem = ImgSystemBeOs;
  } else {
    *TargetSystem = ImgSystemMacOs;
  }

  return S_OK;
}

/**
  Get minimum required system version from Pef image.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetMinimumSystemVersion (
  IN  IImageLoader            *This,
  IN  VOID                    *ImageBase,
  OUT IMGLOAD_SYSTEM_VERSION  *MinimumVersion
  )
{
  if (MinimumVersion == NULL) {
    return E_POINTER;
  }

  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

/**
  Get target subsystem from Pef image.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetTargetSubsystem (
  IN  IImageLoader              *This,
  IN  VOID                      *ImageBase,
  OUT IMGLOAD_TARGET_SUBSYSTEM  *TargetSubsystem
  )
{
  if (TargetSubsystem == NULL) {
    return E_POINTER;
  }

  // Detect BeOS vs Mac OS
  if (PefIsBeOsBinary(ImageBase)) {
    // BeOS PEF binaries are typically GUI applications
    *TargetSubsystem = ImgSubsystemBeOsGui;
  } else {
    // Mac OS classic applications (GUI-based operating system)
    *TargetSubsystem = ImgSubsystemMacOsClassic;
  }

  return S_OK;
}

/**
  Get minimum required subsystem version from Pef image.
**/
static
HRESULT
STDMETHODCALLTYPE
PefGetMinimumSubsystemVersion (
  IN  IImageLoader            *This,
  IN  VOID                    *ImageBase,
  OUT IMGLOAD_SYSTEM_VERSION  *MinimumVersion
  )
{
  if (MinimumVersion == NULL) {
    return E_POINTER;
  }

  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}
// C++ mode not supported in this implementation
#error "C++ mode not implemented"
#else
static CONST IImageLoaderVtbl gPefVtbl = {
  PefQueryInterface,
  PefAddRef,
  PefRelease,
  PefDetect,
  PefGetArch,
  PefGetEndianness,
  PefGetEntryPoint,
  PefLoadImage,
  PefGetTlsInfo,
  PefGetUnwindInfo,
  PefGetSymbolByAddress,
  PefGetSymbolByName,
  PefGetRelocInfo,
  PefApplyRelocations,
  PefGetTargetSystem,
  PefGetMinimumSystemVersion,
  PefGetTargetSubsystem,
  PefGetMinimumSubsystemVersion,
  PefGetResource,
  PefGetResourceEnumerator
};
#endif

//
// PEF Loader Instance
//

IImageLoader gPefLoader = {
  &gPefVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gPefLoader)
