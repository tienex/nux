/** @file
  APXH COFF Loader Implementation

  Provides COFF (Common Object File Format) parsing and loading for
  Unix System V and early Windows executables using COM-style interface.
  Handles sections, optional headers, and relocations for x86 and other
  architectures.

  Supports:
  - SCO Unix COFF
  - System V Release 3/4 COFF
  - x86 (32-bit and 64-bit)
  - MIPS, ARM, Alpha architectures

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// COFF Magic Numbers
//

#define COFF_MAGIC_I386    0x014C  ///< Intel 386
#define COFF_MAGIC_I860    0x014D  ///< Intel 860
#define COFF_MAGIC_R3000   0x0162  ///< MIPS R3000 (little-endian)
#define COFF_MAGIC_R4000   0x0166  ///< MIPS R4000 (little-endian)
#define COFF_MAGIC_ALPHA   0x0184  ///< Alpha AXP
#define COFF_MAGIC_ARM     0x01C0  ///< ARM
#define COFF_MAGIC_AMD64   0x8664  ///< AMD64

//
// COFF File Header Flags
//

#define COFF_F_RELFLG   0x0001  ///< Relocation info stripped
#define COFF_F_EXEC     0x0002  ///< File is executable
#define COFF_F_LNNO     0x0004  ///< Line numbers stripped
#define COFF_F_LSYMS    0x0008  ///< Local symbols stripped

//
// COFF Section Flags
//

#define COFF_STYP_TEXT   0x0020  ///< Text (executable)
#define COFF_STYP_DATA   0x0040  ///< Data (initialized)
#define COFF_STYP_BSS    0x0080  ///< BSS (uninitialized)

//
// COFF Structures
//

ANX_PACK_PUSH(1)

typedef struct _COFF_FILE_HEADER {
  UINT16  Magic;              ///< Magic number
  UINT16  NumSections;        ///< Number of sections
  UINT32  TimeStamp;          ///< Time & date stamp
  UINT32  SymbolTablePtr;     ///< Symbol table file pointer
  UINT32  NumSymbols;         ///< Number of symbol table entries
  UINT16  OptHeaderSize;      ///< Size of optional header
  UINT16  Flags;              ///< Flags
} COFF_FILE_HEADER;

typedef struct _COFF_AOUT_HEADER {
  UINT16  Magic;              ///< Magic number (0407, 0410, 0413)
  UINT16  Version;            ///< Version stamp
  UINT32  TextSize;           ///< Text size in bytes
  UINT32  DataSize;           ///< Initialized data size
  UINT32  BssSize;            ///< Uninitialized data size
  UINT32  Entry;              ///< Entry point
  UINT32  TextStart;          ///< Base of text
  UINT32  DataStart;          ///< Base of data
} COFF_AOUT_HEADER;

typedef struct _COFF_SECTION_HEADER {
  CHAR8   Name[8];            ///< Section name
  UINT32  PhysicalAddr;       ///< Physical address
  UINT32  VirtualAddr;        ///< Virtual address
  UINT32  Size;               ///< Section size
  UINT32  DataPtr;            ///< File pointer to raw data
  UINT32  RelocPtr;           ///< File pointer to relocation
  UINT32  LinenoPtr;          ///< File pointer to line numbers
  UINT16  NumRelocs;          ///< Number of relocation entries
  UINT16  NumLinenos;         ///< Number of line number entries
  UINT32  Flags;              ///< Section flags
} COFF_SECTION_HEADER;

ANX_PACK_POP()

//
// Helper Macros
//

#define COFF_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// IImageLoader Implementation for COFF
//

/**
  Detect if image is COFF format.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  COFF_FILE_HEADER *Header;

  if (ImageSize < sizeof(COFF_FILE_HEADER)) {
    return S_FALSE;
  }

  Header = (COFF_FILE_HEADER *)ImageBase;

  // Check for known COFF machine types
  switch (Header->Magic) {
    case COFF_MAGIC_I386:
    case COFF_MAGIC_AMD64:
    case COFF_MAGIC_R3000:
    case COFF_MAGIC_R4000:
    case COFF_MAGIC_ALPHA:
    case COFF_MAGIC_ARM:
      // Must have executable flag set
      if (Header->Flags & COFF_F_EXEC) {
        return S_OK;
      }
      break;

    default:
      return S_FALSE;
  }

  return S_FALSE;
}

/**
  Get architecture from COFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  COFF_FILE_HEADER *Header;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  Header = (COFF_FILE_HEADER *)ImageBase;

  switch (Header->Magic) {
    case COFF_MAGIC_I386:
      *Architecture = ARCH_386;
      break;

    case COFF_MAGIC_AMD64:
      *Architecture = ARCH_AMD64;
      break;

    case COFF_MAGIC_R3000:
    case COFF_MAGIC_R4000:
      *Architecture = ARCH_MIPS;
      break;

    case COFF_MAGIC_ALPHA:
      *Architecture = ARCH_ALPHA;
      break;

    case COFF_MAGIC_ARM:
      *Architecture = ARCH_ARM;
      break;

    default:
      *Architecture = ARCH_UNSUPPORTED;
      return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  return S_OK;
}

/**
  Get endianness from COFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // COFF is always little-endian (MIPS R3000/R4000 variants are little-endian)
  *Endianness = ImgEndianLittle;

  return S_OK;
}

/**
  Get entry point from COFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  COFF_FILE_HEADER *FileHeader;
  COFF_AOUT_HEADER *AoutHeader;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  FileHeader = (COFF_FILE_HEADER *)ImageBase;

  if (FileHeader->OptHeaderSize >= sizeof(COFF_AOUT_HEADER)) {
    AoutHeader = (COFF_AOUT_HEADER *)(FileHeader + 1);
    *EntryPoint = AoutHeader->Entry;
    return S_OK;
  }

  *EntryPoint = 0;
  return IMGLOAD_E_INVALID_HEADER;
}

/**
  Load COFF image segments.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffLoadImage (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  COFF_FILE_HEADER *FileHeader;
  COFF_SECTION_HEADER *Sections;
  UINT16 i;
  UINTN SectionsOffset;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  FileHeader = (COFF_FILE_HEADER *)ImageBase;

  info("Loading COFF executable...");

  // Sections follow the optional header
  SectionsOffset = sizeof(COFF_FILE_HEADER) + FileHeader->OptHeaderSize;
  Sections = (COFF_SECTION_HEADER *)COFF_OFF(SectionsOffset);

  // Load all sections
  for (i = 0; i < FileHeader->NumSections; i++) {
    COFF_SECTION_HEADER *Sec = &Sections[i];
    BOOLEAN IsText = !!(Sec->Flags & COFF_STYP_TEXT);
    BOOLEAN IsData = !!(Sec->Flags & COFF_STYP_DATA);
    BOOLEAN IsBss = !!(Sec->Flags & COFF_STYP_BSS);

    info("  Section %.8s at 0x%08x (size: 0x%08x, flags: 0x%08x)",
         Sec->Name, Sec->VirtualAddr, Sec->Size, Sec->Flags);

    if (Sec->Size == 0) {
      continue;
    }

    if (IsBss) {
      // BSS: zero-filled, writable
      VirtualAddressMemset(
        Sec->VirtualAddr,
        0,
        Sec->Size,
        Context->IsUserMode,
        TRUE,   // Writable
        FALSE   // Not executable
      );
    } else if (Sec->DataPtr > 0) {
      // Normal section with data
      VirtualAddressCopy(
        Sec->VirtualAddr,
        COFF_OFF(Sec->DataPtr),
        Sec->Size,
        Context->IsUserMode,
        !IsText,  // Writable if not text
        IsText    // Executable if text
      );
    }
  }

  return S_OK;
}

/**
  Extract TLS information from COFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // COFF doesn't have explicit TLS support
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from COFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  // COFF doesn't have standard unwinding information
  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  // Symbol table parsing would go here
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  // Symbol table parsing would go here
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Extract relocation information from COFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetRelocInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_RELOC_INFO   *RelocInfo
  )
{
  COFF_FILE_HEADER *Header;

  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  Header = (COFF_FILE_HEADER *)ImageBase;

  // Check if relocations are stripped
  if (Header->Flags & COFF_F_RELFLG) {
    RelocInfo->RequiresReloc = FALSE;
    return S_FALSE;
  }

  // COFF has per-section relocations
  RelocInfo->Format = 9;  // COFF format
  RelocInfo->RequiresReloc = TRUE;

  return S_OK;
}

/**
  Apply relocations to COFF image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffApplyRelocations (
  IN VOID              *ImageBase,
  IN VIRTUAL_ADDRESS   LoadAddress,
  IN VIRTUAL_ADDRESS   PreferredBase
  )
{
  COFF_FILE_HEADER *Header;
  INT64 Delta;

  Header = (COFF_FILE_HEADER *)ImageBase;

  // Calculate relocation delta
  Delta = (INT64)LoadAddress - (INT64)PreferredBase;

  if (Delta == 0) {
    return S_OK;  // No relocation needed
  }

  // Check if relocations are stripped
  if (Header->Flags & COFF_F_RELFLG) {
    return E_NOTIMPL;  // No relocations available
  }

  // COFF relocation format varies by architecture
  // Full implementation would parse per-section relocation tables
  return E_NOTIMPL;
}

/**
  IUnknown::QueryInterface implementation.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffQueryInterface (
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
CoffAddRef (
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
CoffRelease (
  IN IImageLoader  *This
  )
{
  return 1;
}

//
// COFF Loader VTable
//

static CONST IImageLoaderVtbl gCoffVtbl = {
  CoffQueryInterface,
  CoffAddRef,
  CoffRelease,
  CoffDetect,
  CoffGetArch,
  CoffGetEndianness,
  CoffGetEntryPoint,
  CoffLoadImage,
  CoffGetTlsInfo,
  CoffGetUnwindInfo,
  CoffGetSymbolByAddress,
  CoffGetSymbolByName,
  CoffGetRelocInfo,
  CoffApplyRelocations
};

//
// COFF Loader Instance
//

IImageLoader gCoffLoader = {
  &gCoffVtbl
};

// Auto-register this loader
ANX_REGISTER_IMGLOADER(gCoffLoader);
