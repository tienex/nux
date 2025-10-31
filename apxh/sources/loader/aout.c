/** @file
  APXH a.out Loader

  Provides a.out (assembler output) format parsing and loading for
  classic Unix executables using COM-style interface. Handles OMAGIC,
  NMAGIC, ZMAGIC, and QMAGIC variants for 32-bit x86 systems.

  Supports:
  - OMAGIC (0407): Text and data not separated
  - NMAGIC (0410): Text read-only, data writable
  - ZMAGIC (0413): Demand-paged (most common)
  - QMAGIC (0314): Demand-paged with 4K offset

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// a.out Magic Numbers
//

#define AOUT_OMAGIC  0407  ///< Old impure format
#define AOUT_NMAGIC  0410  ///< Read-only text
#define AOUT_ZMAGIC  0413  ///< Demand-paged
#define AOUT_QMAGIC  0314  ///< Demand-paged with 4K offset

//
// a.out Machine Types
//

#define AOUT_M_386     100  ///< Intel 386

//
// a.out Structures
//

ANX_PACK_PUSH(1)

typedef struct _AOUT_HEADER {
  UINT32  Magic : 16;      ///< Magic number
  UINT32  MachType : 16;   ///< Machine type
  UINT32  TextSize;        ///< Text segment size
  UINT32  DataSize;        ///< Data segment size
  UINT32  BssSize;         ///< BSS segment size
  UINT32  SymbolSize;      ///< Symbol table size
  UINT32  Entry;           ///< Entry point
  UINT32  TextReloc;       ///< Text relocation size
  UINT32  DataReloc;       ///< Data relocation size
} AOUT_HEADER;

/**
  a.out relocation entry structure.
**/
typedef struct _AOUT_RELOC {
  UINT32  Address;        ///< Offset from segment start
  UINT32  SymNum : 24;    ///< Symbol index or segment number
  UINT32  PcRel  : 1;     ///< PC-relative flag
  UINT32  Length : 2;     ///< 0=byte, 1=word, 2=long, 3=quad
  UINT32  Extern : 1;     ///< External reference flag
  UINT32  Type   : 4;     ///< Relocation type
} AOUT_RELOC;

ANX_PACK_POP()

//
// a.out Relocation Types
//

#define RELOC_8        0  ///< 8-bit relocation
#define RELOC_16       1  ///< 16-bit relocation
#define RELOC_32       2  ///< 32-bit relocation

//
// Default a.out addresses
//

#define AOUT_TEXT_START  0x00001000  ///< Text segment start
#define AOUT_PAGE_SIZE   0x00001000  ///< Page size (4K)

//
// Helper Macros
//

#define AOUT_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))
#define AOUT_ROUND_PAGE(_a) (((_a) + AOUT_PAGE_SIZE - 1) & ~(AOUT_PAGE_SIZE - 1))

//
// IImageLoader Implementation for a.out
//

/**
  Detect if image is a.out format.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  AOUT_HEADER *Header;

  if (ImageSize < sizeof(AOUT_HEADER)) {
    return S_FALSE;
  }

  Header = (AOUT_HEADER *)ImageBase;

  if (Header->Magic == AOUT_OMAGIC ||
      Header->Magic == AOUT_NMAGIC ||
      Header->Magic == AOUT_ZMAGIC ||
      Header->Magic == AOUT_QMAGIC) {
    return S_OK;
  }

  return S_FALSE;
}

/**
  Get architecture from a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  AOUT_HEADER *Header;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  Header = (AOUT_HEADER *)ImageBase;

  if (Header->MachType == AOUT_M_386) {
    *Architecture = Arch386;
  } else {
    *Architecture = ArchUnsupported;
    return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  return S_OK;
}

/**
  Get endianness from a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // a.out is little-endian (x86)
  *Endianness = ImgEndianLittle;
  return S_OK;
}

/**
  Get entry point from a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  AOUT_HEADER *Header;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  Header = (AOUT_HEADER *)ImageBase;
  *EntryPoint = Header->Entry;

  if (*EntryPoint == 0 || *EntryPoint == (VIRTUAL_ADDRESS)-1) {
    return IMGLOAD_E_INVALID_HEADER;
  }

  return S_OK;
}

/**
  Load a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutLoadImage (
  IN     IImageLoader     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  AOUT_HEADER *Header;
  UINT32 TextAddr, DataAddr, BssAddr;
  UINT32 TextOffset, DataOffset;
  HRESULT Status;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  Header = (AOUT_HEADER *)ImageBase;

  // Populate context
  Status = AoutGetArch(This, ImageBase, &Context->Architecture);
  if (FAILED(Status)) {
    return Status;
  }

  Status = AoutGetEndianness(This, ImageBase, &Context->Endianness);
  if (FAILED(Status)) {
    return Status;
  }

  Status = AoutGetEntryPoint(This, ImageBase, &Context->EntryPoint);
  if (FAILED(Status)) {
    return Status;
  }

  // Calculate addresses and offsets based on magic
  switch (Header->Magic) {
    case AOUT_OMAGIC:
      // OMAGIC: Text and data contiguous, no page alignment
      TextAddr = AOUT_TEXT_START;
      DataAddr = TextAddr + Header->TextSize;
      BssAddr = DataAddr + Header->DataSize;
      TextOffset = sizeof(AOUT_HEADER);
      DataOffset = TextOffset + Header->TextSize;
      break;

    case AOUT_NMAGIC:
      // NMAGIC: Data page-aligned after text
      TextAddr = AOUT_TEXT_START;
      DataAddr = AOUT_ROUND_PAGE(TextAddr + Header->TextSize);
      BssAddr = DataAddr + Header->DataSize;
      TextOffset = sizeof(AOUT_HEADER);
      DataOffset = TextOffset + Header->TextSize;
      break;

    case AOUT_ZMAGIC:
      // ZMAGIC: Page-aligned text and data
      TextAddr = AOUT_TEXT_START;
      DataAddr = AOUT_ROUND_PAGE(TextAddr + Header->TextSize);
      BssAddr = DataAddr + Header->DataSize;
      TextOffset = AOUT_PAGE_SIZE;  // Header in first page
      DataOffset = AOUT_ROUND_PAGE(TextOffset + Header->TextSize);
      break;

    case AOUT_QMAGIC:
      // QMAGIC: Like ZMAGIC but header not mapped
      TextAddr = AOUT_TEXT_START;
      DataAddr = AOUT_ROUND_PAGE(TextAddr + Header->TextSize);
      BssAddr = DataAddr + Header->DataSize;
      TextOffset = 0;  // No header offset
      DataOffset = AOUT_ROUND_PAGE(Header->TextSize);
      break;

    default:
      return IMGLOAD_E_INVALID_FORMAT;
  }

  // Load text segment (executable)
  if (Header->TextSize > 0) {
    VirtualAddressCopy(
      TextAddr,
      AOUT_OFF(TextOffset),
      Header->TextSize,
      Context->IsUserMode,
      FALSE,  // Not writable
      TRUE    // Executable
    );
  }

  // Load data segment (writable)
  if (Header->DataSize > 0) {
    VirtualAddressCopy(
      DataAddr,
      AOUT_OFF(DataOffset),
      Header->DataSize,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  // Zero-fill BSS
  if (Header->BssSize > 0) {
    VirtualAddressMemset(
      BssAddr,
      0,
      Header->BssSize,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  return S_OK;
}

/**
  Extract TLS information from a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // a.out doesn't support TLS
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  // a.out doesn't have unwinding information
  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  // a.out symbol table parsing would go here
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  // a.out symbol table parsing would go here
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Extract relocation information from a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutGetRelocInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_RELOC_INFO   *RelocInfo
  )
{
  AOUT_HEADER *Header;

  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  Header = (AOUT_HEADER *)ImageBase;

  // a.out has text and data relocation tables
  if (Header->TextReloc > 0 || Header->DataReloc > 0) {
    RelocInfo->PreferredBase = AOUT_TEXT_START;
    RelocInfo->RelocTableSize = Header->TextReloc + Header->DataReloc;
    RelocInfo->Format = ImgRelocFormatAout;
    RelocInfo->RequiresReloc = TRUE;
    return S_OK;
  }

  RelocInfo->RequiresReloc = FALSE;
  return S_FALSE;
}

/**
  Apply relocations to a.out image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutApplyRelocations (
  IN VOID              *ImageBase,
  IN VIRTUAL_ADDRESS   LoadAddress,
  IN VIRTUAL_ADDRESS   PreferredBase
  )
{
  AOUT_HEADER *Header;
  AOUT_RELOC *TextRelocs, *DataRelocs;
  INT64 Delta;
  UINT32 NumTextRelocs, NumDataRelocs;
  UINT32 i;
  UINT32 TextStart, DataStart;
  UINT8 *TextBase, *DataBase;

  Header = (AOUT_HEADER *)ImageBase;

  // Calculate relocation delta
  Delta = (INT64)LoadAddress - (INT64)PreferredBase;

  if (Delta == 0) {
    return S_OK;  // No relocation needed
  }

  // Check if there are any relocations
  if (Header->TextReloc == 0 && Header->DataReloc == 0) {
    return S_OK;  // No relocations to apply
  }

  // Calculate segment addresses
  TextStart = AOUT_TEXT_START;
  DataStart = TextStart + AOUT_ROUND_PAGE(Header->TextSize);

  TextBase = (UINT8 *)LoadAddress;
  DataBase = TextBase + Header->TextSize;

  // Get relocation tables
  // They are located after: header + text + data + symbols
  UINT32 RelocOffset = sizeof(AOUT_HEADER) + Header->TextSize +
                       Header->DataSize + Header->SymbolSize;

  TextRelocs = (AOUT_RELOC *)AOUT_OFF(RelocOffset);
  DataRelocs = (AOUT_RELOC *)AOUT_OFF(RelocOffset + Header->TextReloc);

  NumTextRelocs = Header->TextReloc / sizeof(AOUT_RELOC);
  NumDataRelocs = Header->DataReloc / sizeof(AOUT_RELOC);

  // Apply text relocations
  for (i = 0; i < NumTextRelocs; i++) {
    AOUT_RELOC *Reloc = &TextRelocs[i];

    // Only apply non-external relocations (internal position-dependent code)
    if (!Reloc->Extern) {
      UINT8 *Target = TextBase + Reloc->Address;

      switch (Reloc->Length) {
        case 0:  // Byte
          *(UINT8 *)Target += (UINT8)Delta;
          break;
        case 1:  // Word
          *(UINT16 *)Target += (UINT16)Delta;
          break;
        case 2:  // Long
          *(UINT32 *)Target += (UINT32)Delta;
          break;
      }
    }
  }

  // Apply data relocations
  for (i = 0; i < NumDataRelocs; i++) {
    AOUT_RELOC *Reloc = &DataRelocs[i];

    // Only apply non-external relocations
    if (!Reloc->Extern) {
      UINT8 *Target = DataBase + Reloc->Address;

      switch (Reloc->Length) {
        case 0:  // Byte
          *(UINT8 *)Target += (UINT8)Delta;
          break;
        case 1:  // Word
          *(UINT16 *)Target += (UINT16)Delta;
          break;
        case 2:  // Long
          *(UINT32 *)Target += (UINT32)Delta;
          break;
      }
    }
  }

  return S_OK;
}

/**
  IUnknown::QueryInterface implementation.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutQueryInterface (
  IN  IImageLoader  *This,
  IN  REFIID        riid,
  OUT VOID          **ppvObject
  )
{
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  *ppvObject = NULL;

  if (memcmp(riid, &IID_IImageLoader, sizeof(GUID)) == 0 ||
      memcmp(riid, &IID_IUnknown, sizeof(GUID)) == 0) {
    *ppvObject = This;
    return S_OK;
  }

  return E_NOINTERFACE;
}

/**
  IUnknown::AddRef implementation.
**/
static
UINT32
STDMETHODCALLTYPE
AoutAddRef (
  IN IImageLoader  *This
  )
{
  return 1;
}

/**
  IUnknown::Release implementation.
**/
static
UINT32
STDMETHODCALLTYPE
AoutRelease (
  IN IImageLoader  *This
  )
{
  return 1;
}

//

/**
  Get target operating system from Aout image.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutGetTargetSystem (
  IN  IImageLoader           *This,
  IN  VOID                   *ImageBase,
  OUT IMGLOAD_TARGET_SYSTEM  *TargetSystem
  )
{
  if (TargetSystem == NULL) {
    return E_POINTER;
  }

  *TargetSystem = ImgSystemUnix;
  return S_OK;
}

/**
  Get minimum required system version from Aout image.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutGetMinimumSystemVersion (
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
  Get target subsystem from Aout image.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutGetTargetSubsystem (
  IN  IImageLoader              *This,
  IN  VOID                      *ImageBase,
  OUT IMGLOAD_TARGET_SUBSYSTEM  *TargetSubsystem
  )
{
  if (TargetSubsystem == NULL) {
    return E_POINTER;
  }

  *TargetSubsystem = ImgSubsystemCli;
  return S_OK;
}

/**
  Get minimum required subsystem version from Aout image.
**/
static
HRESULT
STDMETHODCALLTYPE
AoutGetMinimumSubsystemVersion (
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
// a.out Loader VTable
//

static CONST IImageLoaderVtbl gAoutVtbl = {
  AoutQueryInterface,
  AoutAddRef,
  AoutRelease,
  AoutDetect,
  AoutGetArch,
  AoutGetEndianness,
  AoutGetEntryPoint,
  AoutLoadImage,
  AoutGetTlsInfo,
  AoutGetUnwindInfo,
  AoutGetSymbolByAddress,
  AoutGetSymbolByName,
  AoutGetRelocInfo,
  AoutApplyRelocations,
  AoutGetTargetSystem,
  AoutGetMinimumSystemVersion,
  AoutGetTargetSubsystem,
  AoutGetMinimumSubsystemVersion
};

//
// a.out Loader Instance
//

IImageLoader gAoutLoader = {
  &gAoutVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gAoutLoader);
