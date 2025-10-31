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
  Check if image is NLM format.
**/
static
BOOLEAN
ANXAPI
NlmDetect (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  if (ImageSize < sizeof(NLM_HEADER_V4)) {
    return FALSE;
  }

  return (memcmp(ImageBase, NLM_SIGNATURE, NLM_SIGNATURE_LEN) == 0);
}

/**
  Get architecture from NLM image.
**/
static
ARCH
ANXAPI
NlmGetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  NLM_HEADER_V4 *Header = (NLM_HEADER_V4 *)ImageBase;

  if (Header->Version == NLM_VERSION_5) {
    NLM_HEADER_V5 *HeaderV5 = (NLM_HEADER_V5 *)ImageBase;
    if (HeaderV5->Is64Bit) {
      return ARCH_AMD64;
    }
  }

  // Default to 32-bit x86 for NLM v4 and v5 32-bit
  return ARCH_386;
}

/**
  Get entry point from NLM image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
NlmGetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  NLM_HEADER_V4 *Header = (NLM_HEADER_V4 *)ImageBase;

  if (Header->Version == NLM_VERSION_5) {
    NLM_HEADER_V5 *HeaderV5 = (NLM_HEADER_V5 *)ImageBase;
    return NLM_DEFAULT_CODE_BASE + HeaderV5->CodeStartOffset;
  }

  return NLM_DEFAULT_CODE_BASE + Header->CodeStartOffset;
}

/**
  Load NLM image.
**/
static
IMGLOAD_STATUS
ANXAPI
NlmLoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase = Context->ImageBase;
  NLM_HEADER_V4 *Header = (NLM_HEADER_V4 *)ImageBase;
  UINT64 CodeOffset, CodeSize;
  UINT64 DataOffset, DataSize, BssSize;
  BOOLEAN Is64Bit = FALSE;

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
    return ImgLoadUnsupportedVersion;
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

  Context->EntryPoint = NlmGetEntryPoint(This, ImageBase);
  return ImgLoadSuccess;
}

/**
  Extract TLS information from NLM image.
**/
static
IMGLOAD_STATUS
ANXAPI
NlmGetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  NLM_HEADER_V4 *Header = (NLM_HEADER_V4 *)ImageBase;

  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));

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
    }
  }

  return ImgLoadSuccess;
}

//
// NLM Loader VTable
//

static CONST IMAGE_LOADER_VTBL gNlmVtbl = {
  NlmDetect,
  NlmGetArch,
  NlmGetEntryPoint,
  NlmLoadImage,
  NlmGetTlsInfo
};

//
// NLM Loader Instance
//

IMAGE_LOADER gNlmLoader = {
  &gNlmVtbl,
  "NLM",
  NULL
};
