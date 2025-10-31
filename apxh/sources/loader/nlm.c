/** @file
  APXH NLM Loader

  Provides NLM (NetWare Loadable Module) format parsing and loading.
  Handles NetWare executables, modules, and Thread-Local Storage (TLS).

  Supports:
  - NLM version 4 and 5
  - x86 and x86-64 architectures
  - Multiple code and data segments
  - TLS for threaded modules

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// NLM Magic and Signatures
//

#define NLM_SIGNATURE     "NetWare Loadable Module\x1A"
#define NLM_SIGNATURE_LEN 25

//
// NLM Version
//

#define NLM_VERSION_4     4  ///< NLM version 4 (32-bit)
#define NLM_VERSION_5     5  ///< NLM version 5 (can be 64-bit)

//
// NLM Module Types
//

#define NLM_TYPE_GENERIC  0  ///< Generic NLM
#define NLM_TYPE_LAN      1  ///< LAN driver
#define NLM_TYPE_DISK     2  ///< Disk driver
#define NLM_TYPE_NAME     3  ///< Name space

//
// NLM Flags
//

#define NLM_FLAG_REENTRANT     0x00000001  ///< Reentrant module
#define NLM_FLAG_MULTILOAD     0x00000002  ///< Multiple load allowed
#define NLM_FLAG_SYNCHRONIZE   0x00000004  ///< Synchronize start
#define NLM_FLAG_PSEUDO_PREEMPT 0x00000008 ///< Pseudo preemption
#define NLM_FLAG_OS_DOMAIN     0x00000010  ///< OS domain module

//
// NLM Structures
//

ANX_PACK_PUSH(1)

typedef struct _NLM_HEADER_V4 {
  CHAR8   Signature[NLM_SIGNATURE_LEN];  ///< "NetWare Loadable Module\x1A"
  UINT32  Version;                        ///< NLM version (4)
  CHAR8   ModuleName[14];                 ///< Module name
  UINT32  CodeImageOffset;                ///< Code image file offset
  UINT32  CodeImageSize;                  ///< Code image size
  UINT32  DataImageOffset;                ///< Data image file offset
  UINT32  DataImageSize;                  ///< Data image size
  UINT32  UninitDataSize;                 ///< Uninitialized data size (BSS)
  UINT32  CustomDataOffset;               ///< Custom data offset
  UINT32  CustomDataSize;                 ///< Custom data size
  UINT32  ModuleDependencyOffset;         ///< Module dependencies offset
  UINT32  NumModuleDependencies;          ///< Number of dependencies
  UINT32  RelocationFixupOffset;          ///< Relocation fixup offset
  UINT32  NumRelocationFixups;            ///< Number of relocations
  UINT32  ExternalReferenceOffset;        ///< External reference offset
  UINT32  NumExternalReferences;          ///< Number of external references
  UINT32  PublicSymbolOffset;             ///< Public symbol offset
  UINT32  NumPublicSymbols;               ///< Number of public symbols
  UINT32  DebugInfoOffset;                ///< Debug info offset
  UINT32  NumDebugRecords;                ///< Number of debug records
  UINT32  CodeStartOffset;                ///< Code start address (RVA)
  UINT32  ExitProcedureOffset;            ///< Exit procedure address (RVA)
  UINT32  CheckUnloadProcedureOffset;     ///< Check unload address (RVA)
  UINT32  ModuleType;                     ///< Module type
  UINT32  Flags;                          ///< Module flags
} NLM_HEADER_V4;

typedef struct _NLM_HEADER_V5 {
  CHAR8   Signature[NLM_SIGNATURE_LEN];  ///< "NetWare Loadable Module\x1A"
  UINT32  Version;                        ///< NLM version (5)
  CHAR8   ModuleName[14];                 ///< Module name
  UINT32  Is64Bit;                        ///< 1 if 64-bit, 0 if 32-bit
  UINT64  CodeImageOffset;                ///< Code image file offset
  UINT64  CodeImageSize;                  ///< Code image size
  UINT64  DataImageOffset;                ///< Data image file offset
  UINT64  DataImageSize;                  ///< Data image size
  UINT64  UninitDataSize;                 ///< Uninitialized data size (BSS)
  UINT64  CustomDataOffset;               ///< Custom data offset
  UINT64  CustomDataSize;                 ///< Custom data size
  UINT32  ModuleDependencyOffset;         ///< Module dependencies offset
  UINT32  NumModuleDependencies;          ///< Number of dependencies
  UINT32  RelocationFixupOffset;          ///< Relocation fixup offset
  UINT32  NumRelocationFixups;            ///< Number of relocations
  UINT32  ExternalReferenceOffset;        ///< External reference offset
  UINT32  NumExternalReferences;          ///< Number of external references
  UINT32  PublicSymbolOffset;             ///< Public symbol offset
  UINT32  NumPublicSymbols;               ///< Number of public symbols
  UINT64  CodeStartOffset;                ///< Code start address (RVA)
  UINT64  ExitProcedureOffset;            ///< Exit procedure address (RVA)
  UINT64  CheckUnloadProcedureOffset;     ///< Check unload address (RVA)
  UINT32  ModuleType;                     ///< Module type
  UINT32  Flags;                          ///< Module flags
  UINT64  TlsDataOffset;                  ///< TLS data file offset (extension)
  UINT64  TlsDataSize;                    ///< TLS data size
  UINT64  TlsBssSize;                     ///< TLS BSS size
} NLM_HEADER_V5;

ANX_PACK_POP()

//
// Helper Macros
//

#define NLM_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

// Default NLM load address
#define NLM_DEFAULT_CODE_BASE  0x00400000ULL
#define NLM_DEFAULT_DATA_BASE  0x10000000ULL

//
// Internal Functions
//

/**
  IUnknown: QueryInterface
**/
static
HRESULT
STDMETHODCALLTYPE
NlmQueryInterface (
  IN  IImageLoader  *This,
  IN  CONST GUID    *Iid,
  OUT VOID          **Interface
  )
{
  if (Interface == NULL) {
    return E_POINTER;
  }

  if (memcmp(Iid, &IID_IImageLoader, sizeof(GUID)) == 0 ||
      memcmp(Iid, &IID_IUnknown, sizeof(GUID)) == 0) {
    *Interface = This;
    return S_OK;
  }

  *Interface = NULL;
  return E_NOINTERFACE;
}

/**
  IUnknown: AddRef
**/
static
UINTN
STDMETHODCALLTYPE
NlmAddRef (
  IN IImageLoader  *This
  )
{
  return 1;  // Static instance
}

/**
  IUnknown: Release
**/
static
UINTN
STDMETHODCALLTYPE
NlmRelease (
  IN IImageLoader  *This
  )
{
  return 1;  // Static instance
}

/**
  Check if image is NLM format.
**/
static
HRESULT
STDMETHODCALLTYPE
NlmDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  if (ImageSize < sizeof(NLM_HEADER_V4)) {
    return S_FALSE;
  }

  return (memcmp(ImageBase, NLM_SIGNATURE, NLM_SIGNATURE_LEN) == 0) ? S_OK : S_FALSE;
}

/**
  Get architecture from NLM image.
**/
static
HRESULT
STDMETHODCALLTYPE
NlmGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  NLM_HEADER_V4 *Header;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  Header = (NLM_HEADER_V4 *)ImageBase;

  if (Header->Version == NLM_VERSION_5) {
    NLM_HEADER_V5 *HeaderV5 = (NLM_HEADER_V5 *)ImageBase;
    if (HeaderV5->Is64Bit) {
      *Architecture = ARCH_AMD64;
      return S_OK;
    }
  }

  // Default to 32-bit x86 for NLM v4 and v5 32-bit
  *Architecture = ARCH_386;
  return S_OK;
}

/**
  Get endianness from NLM image.
**/
static
HRESULT
STDMETHODCALLTYPE
NlmGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // NLM is x86/x86-64 only, always little-endian
  *Endianness = ImgEndianLittle;
  return S_OK;
}

/**
  Get entry point from NLM image.
**/
static
HRESULT
STDMETHODCALLTYPE
NlmGetEntryPoint (
  IN  IImageLoader     *This,
  IN  VOID             *ImageBase,
  OUT VIRTUAL_ADDRESS  *EntryPoint
  )
{
  NLM_HEADER_V4 *Header;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  Header = (NLM_HEADER_V4 *)ImageBase;

  if (Header->Version == NLM_VERSION_5) {
    NLM_HEADER_V5 *HeaderV5 = (NLM_HEADER_V5 *)ImageBase;
    *EntryPoint = NLM_DEFAULT_CODE_BASE + HeaderV5->CodeStartOffset;
    return S_OK;
  }

  *EntryPoint = NLM_DEFAULT_CODE_BASE + Header->CodeStartOffset;
  return S_OK;
}

/**
  Load NLM image.
**/
static
HRESULT
STDMETHODCALLTYPE
NlmLoadImage (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  NLM_HEADER_V4 *Header;
  UINT64 CodeOffset, CodeSize;
  UINT64 DataOffset, DataSize, BssSize;
  BOOLEAN Is64Bit = FALSE;
  HRESULT Hr;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  Header = (NLM_HEADER_V4 *)ImageBase;

  if (Header->Version == NLM_VERSION_5) {
    NLM_HEADER_V5 *HeaderV5 = (NLM_HEADER_V5 *)ImageBase;
    Is64Bit = (HeaderV5->Is64Bit != 0);
    CodeOffset = HeaderV5->CodeImageOffset;
    CodeSize = HeaderV5->CodeImageSize;
    DataOffset = HeaderV5->DataImageOffset;
    DataSize = HeaderV5->DataImageSize;
    BssSize = HeaderV5->UninitDataSize;

    info("Loading NLM v5 %s module: %.14s",
         Is64Bit ? "64-bit" : "32-bit", HeaderV5->ModuleName);
  } else if (Header->Version == NLM_VERSION_4) {
    CodeOffset = Header->CodeImageOffset;
    CodeSize = Header->CodeImageSize;
    DataOffset = Header->DataImageOffset;
    DataSize = Header->DataImageSize;
    BssSize = Header->UninitDataSize;

    info("Loading NLM v4 32-bit module: %.14s", Header->ModuleName);
  } else {
    return IMGLOAD_E_UNSUPPORTED_VERSION;
  }

  // Load code segment
  if (CodeSize > 0) {
    info("  Code segment at 0x%016llx (size: 0x%llx)",
         NLM_DEFAULT_CODE_BASE, CodeSize);

    VirtualAddressCopy(
      NLM_DEFAULT_CODE_BASE,
      NLM_OFF(CodeOffset),
      CodeSize,
      Context->IsUserMode,
      FALSE,  // Not writable
      TRUE    // Executable
    );
  }

  // Load data segment
  if (DataSize > 0) {
    info("  Data segment at 0x%016llx (size: 0x%llx)",
         NLM_DEFAULT_DATA_BASE, DataSize);

    VirtualAddressCopy(
      NLM_DEFAULT_DATA_BASE,
      NLM_OFF(DataOffset),
      DataSize,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  // Zero-fill BSS
  if (BssSize > 0) {
    info("  BSS segment at 0x%016llx (size: 0x%llx)",
         NLM_DEFAULT_DATA_BASE + DataSize, BssSize);

    VirtualAddressMemset(
      NLM_DEFAULT_DATA_BASE + DataSize,
      0,
      BssSize,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  Hr = NlmGetEntryPoint(&gNlmLoader, ImageBase, &Context->EntryPoint);
  if (FAILED(Hr)) {
    return Hr;
  }

  return S_OK;
}

/**
  Extract TLS information from NLM image.
**/
static
HRESULT
STDMETHODCALLTYPE
NlmGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  NLM_HEADER_V4 *Header;

  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));

  Header = (NLM_HEADER_V4 *)ImageBase;

  // Only NLM v5 supports TLS
  if (Header->Version == NLM_VERSION_5) {
    NLM_HEADER_V5 *HeaderV5 = (NLM_HEADER_V5 *)ImageBase;

    if (HeaderV5->TlsDataSize > 0 || HeaderV5->TlsBssSize > 0) {
      TlsInfo->InitDataAddr = NLM_DEFAULT_DATA_BASE +
                              HeaderV5->DataImageSize +
                              HeaderV5->UninitDataSize;
      TlsInfo->InitDataSize = HeaderV5->TlsDataSize;
      TlsInfo->TotalSize = HeaderV5->TlsDataSize + HeaderV5->TlsBssSize;
      TlsInfo->Alignment = 16;  // Default TLS alignment

      info("  TLS segment at 0x%016llx (init: 0x%llx, total: 0x%llx)",
           TlsInfo->InitDataAddr, TlsInfo->InitDataSize, TlsInfo->TotalSize);

      // Copy TLS initialization data if present
      if (HeaderV5->TlsDataSize > 0) {
        VirtualAddressCopy(
          TlsInfo->InitDataAddr,
          NLM_OFF(HeaderV5->TlsDataOffset),
          HeaderV5->TlsDataSize,
          FALSE,  // Kernel TLS
          TRUE,   // Writable
          FALSE   // Not executable
        );
      }

      return S_OK;
    }
  }

  return S_FALSE;  // No TLS information
}

/**
  Extract unwinding information from NLM image.
**/
static
HRESULT
STDMETHODCALLTYPE
NlmGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  // NLM does not have standard unwinding information in the format
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
NlmGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  NLM_HEADER_V4 *Header;
  UINT8 *SymTable;
  UINT8 *Ptr, *End;
  UINT32 i;

  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  Header = (NLM_HEADER_V4 *)ImageBase;
  if (Header->NumPublicSymbols == 0) {
    return S_FALSE;
  }

  SymTable = (UINT8 *)NLM_OFF(Header->PublicSymbolOffset);
  Ptr = SymTable;
  End = Ptr + (Header->NumPublicSymbols * 256);  // Generous upper bound

  // Parse public symbol table
  for (i = 0; i < Header->NumPublicSymbols; i++) {
    UINT8 NameLen;
    CHAR8 *Name;
    UINT32 Offset;
    VIRTUAL_ADDRESS SymAddr;

    if (Ptr >= End) break;

    NameLen = *Ptr++;
    if (NameLen == 0 || Ptr + NameLen + 4 > End) {
      break;
    }

    Name = (CHAR8 *)Ptr;
    Ptr += NameLen;

    Offset = *(UINT32 *)Ptr;
    Ptr += 4;

    // Symbol offset is RVA from code base
    SymAddr = NLM_DEFAULT_CODE_BASE + Offset;

    if (SymAddr == Address) {
      // Found matching symbol
      UINTN CopyLen = (NameLen < sizeof(SymbolInfo->Name) - 1) ?
                      NameLen : (sizeof(SymbolInfo->Name) - 1);
      memcpy(SymbolInfo->Name, Name, CopyLen);
      SymbolInfo->Name[CopyLen] = '\0';
      SymbolInfo->Address = SymAddr;
      SymbolInfo->Size = 0;  // Unknown
      return S_OK;
    }
  }

  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
NlmGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  NLM_HEADER_V4 *Header;
  UINT8 *SymTable;
  UINT8 *Ptr, *End;
  UINT32 i;
  UINTN SearchLen;

  if (SymbolInfo == NULL || Name == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  SearchLen = strlen(Name);
  Header = (NLM_HEADER_V4 *)ImageBase;
  if (Header->NumPublicSymbols == 0) {
    return S_FALSE;
  }

  SymTable = (UINT8 *)NLM_OFF(Header->PublicSymbolOffset);
  Ptr = SymTable;
  End = Ptr + (Header->NumPublicSymbols * 256);  // Generous upper bound

  // Parse public symbol table
  for (i = 0; i < Header->NumPublicSymbols; i++) {
    UINT8 NameLen;
    CHAR8 *SymName;
    UINT32 Offset;
    VIRTUAL_ADDRESS SymAddr;

    if (Ptr >= End) break;

    NameLen = *Ptr++;
    if (NameLen == 0 || Ptr + NameLen + 4 > End) {
      break;
    }

    SymName = (CHAR8 *)Ptr;
    Ptr += NameLen;

    Offset = *(UINT32 *)Ptr;
    Ptr += 4;

    // Check if name matches
    if (NameLen == SearchLen && memcmp(SymName, Name, SearchLen) == 0) {
      // Found matching symbol
      SymAddr = NLM_DEFAULT_CODE_BASE + Offset;

      UINTN CopyLen = (NameLen < sizeof(SymbolInfo->Name) - 1) ?
                      NameLen : (sizeof(SymbolInfo->Name) - 1);
      memcpy(SymbolInfo->Name, SymName, CopyLen);
      SymbolInfo->Name[CopyLen] = '\0';
      SymbolInfo->Address = SymAddr;
      SymbolInfo->Size = 0;  // Unknown
      return S_OK;
    }
  }

  return S_FALSE;
}

/**
  Extract relocation information from NLM image.
**/
static
HRESULT
STDMETHODCALLTYPE
NlmGetRelocInfo (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  OUT IMGLOAD_RELOC_INFO  *RelocInfo
  )
{
  NLM_HEADER_V4 *Header;

  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  Header = (NLM_HEADER_V4 *)ImageBase;

  if (Header->NumRelocationFixups > 0) {
    RelocInfo->PreferredBase = NLM_DEFAULT_CODE_BASE;
    RelocInfo->RelocTableAddr = Header->RelocationFixupOffset;
    RelocInfo->RelocTableSize = Header->NumRelocationFixups * sizeof(UINT32);  // Approximate
    RelocInfo->Format = 5;  // Custom NLM format
    RelocInfo->RequiresReloc = TRUE;
    return S_OK;
  }

  return S_FALSE;
}

/**
  Apply relocations to NLM image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
NlmApplyRelocations (
  IN IImageLoader     *This,
  IN VOID             *ImageBase,
  IN VIRTUAL_ADDRESS  LoadAddress,
  IN VIRTUAL_ADDRESS  PreferredBase
  )
{
  NLM_HEADER_V4 *Header;
  UINT8 *FixupTable;
  UINT8 *CodeBase;
  UINT32 i;
  INT32 Delta;

  Header = (NLM_HEADER_V4 *)ImageBase;

  if (Header->NumRelocationFixups == 0) {
    return S_FALSE;  // No relocations
  }

  Delta = (INT32)((INT64)LoadAddress - (INT64)PreferredBase);
  if (Delta == 0) {
    return S_OK;  // Already at preferred base
  }

  FixupTable = (UINT8 *)NLM_OFF(Header->RelocationFixupOffset);
  CodeBase = (UINT8 *)NLM_OFF(Header->CodeImageOffset);

  // Process fixup records
  UINT8 *Ptr = FixupTable;
  for (i = 0; i < Header->NumRelocationFixups; i++) {
    UINT8 FixupType = *Ptr++;
    UINT32 Offset;

    switch (FixupType) {
      case 0x00:  // Internal reference (most common)
        // Read 4-byte offset
        Offset = *(UINT32 *)Ptr;
        Ptr += 4;

        // Apply fixup to code segment
        if (Offset < Header->CodeImageSize) {
          UINT32 *Target = (UINT32 *)(CodeBase + Offset);
          *Target += Delta;
        }
        break;

      case 0x01:  // External reference
        // Skip: 4-byte offset + 4-byte external symbol index
        Ptr += 8;
        break;

      case 0x02:  // Far call fixup
        // Read 4-byte offset
        Offset = *(UINT32 *)Ptr;
        Ptr += 4;

        if (Offset < Header->CodeImageSize) {
          UINT32 *Target = (UINT32 *)(CodeBase + Offset);
          *Target += Delta;
        }
        break;

      case 0x03:  // Segment fixup
        // Skip: 4-byte offset + 2-byte segment
        Ptr += 6;
        break;

      default:
        // Unknown fixup type, skip 4 bytes as default
        Ptr += 4;
        break;
    }
  }

  return S_OK;
}

//
// NLM Loader VTable
//

static CONST IImageLoaderVtbl gNlmVtbl = {
  // IUnknown
  NlmQueryInterface,
  NlmAddRef,
  NlmRelease,
  // IImageLoader
  NlmDetect,
  NlmGetArch,
  NlmGetEndianness,
  NlmGetEntryPoint,
  NlmLoadImage,
  NlmGetTlsInfo,
  NlmGetUnwindInfo,
  NlmGetSymbolByAddress,
  NlmGetSymbolByName,
  NlmGetRelocInfo,
  NlmApplyRelocations
};

//
// NLM Loader Instance
//

IImageLoader gNlmLoader = {
  &gNlmVtbl
};

APXH_REGISTER_IMGLOADER(gNlmLoader);
