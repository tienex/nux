/** @file
  APXH XENIX X.OUT Loader

  Provides XENIX X.OUT (extended output) format parsing and loading
  for XENIX 386 executables. Handles segments, relocations, and
  shared libraries for 32-bit x86 XENIX systems.

  Supports:
  - XENIX 386 X.OUT format
  - Multiple segments (text, data, BSS)
  - Shared library linkage

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// XENIX X.OUT Magic Numbers
//

#define XOUT_MAGIC      0x0206  ///< XENIX 386 X.OUT magic
#define XOUT_SHLIB_VER  2       ///< Shared library version

//
// XENIX X.OUT Flags
//

#define XOUT_F_EXEC     0x0001  ///< Executable
#define XOUT_F_SEP      0x0002  ///< Separate I&D
#define XOUT_F_PURE     0x0004  ///< Pure text (sharable)
#define XOUT_F_NSHD     0x0008  ///< No shared data
#define XOUT_F_SHRLIB   0x0040  ///< Uses shared library
#define XOUT_F_KER      0x0800  ///< Kernel format

//
// XENIX CPU Types
//

#define XOUT_CPU_386    3       ///< Intel 386

//
// XENIX X.OUT Structures
//

ANX_PACK_PUSH(1)

typedef struct _XOUT_HEADER {
  UINT16  Magic;              ///< Magic number (0x0206)
  UINT16  ExtSize;            ///< Size of extended header
  UINT32  TextSize;           ///< Text segment size
  UINT32  DataSize;           ///< Data segment size
  UINT32  BssSize;            ///< BSS segment size
  UINT32  SymbolSize;         ///< Symbol table size
  UINT32  StackSize;          ///< Stack size
  UINT32  Entry;              ///< Entry point offset
  UINT16  CpuType;            ///< CPU type (3 = 386)
  UINT16  Flags;              ///< Flags
  UINT16  MinAlloc;           ///< Minimum allocation
  UINT16  MaxAlloc;           ///< Maximum allocation
  UINT32  Relocations;        ///< Relocation size
  UINT32  ShLibVer;           ///< Shared library version
  UINT32  Reserved[2];        ///< Reserved
} XOUT_HEADER;

typedef struct _XOUT_SEG_HEADER {
  UINT32  VirtualAddr;        ///< Virtual address
  UINT32  PhysicalSize;       ///< Physical (file) size
  UINT32  VirtualSize;        ///< Virtual (memory) size
  UINT32  FileOffset;         ///< File offset
  UINT32  Flags;              ///< Segment flags
} XOUT_SEG_HEADER;

typedef struct _XOUT_SYMBOL {
  union {
    CHAR8   Name[8];          ///< Symbol name (if <= 8 chars)
    struct {
      UINT32  Zeros;          ///< 0 if name in string table
      UINT32  StringOffset;   ///< Offset into string table
    } StringRef;
  } N;
  UINT32  Value;              ///< Symbol value
  INT16   SectionNumber;      ///< Section number (1=text, 2=data, 3=bss)
  UINT16  Type;               ///< Symbol type
  UINT8   StorageClass;       ///< Storage class
  UINT8   AuxCount;           ///< Number of auxiliary entries
} XOUT_SYMBOL;

ANX_PACK_POP()

//
// XENIX Symbol Storage Classes
//

#define C_NULL      0   ///< No storage class
#define C_EXT       2   ///< External symbol
#define C_STAT      3   ///< Static symbol
#define C_LABEL     6   ///< Label
#define C_FCN       101 ///< Function

//
// XENIX Symbol Section Numbers
//

#define N_UNDEF     0   ///< Undefined
#define N_ABS       -1  ///< Absolute
#define N_TEXT      1   ///< Text section
#define N_DATA      2   ///< Data section
#define N_BSS       3   ///< BSS section

//
// Default XENIX addresses
//

#define XOUT_TEXT_START  0x00002000  ///< Text segment start
#define XOUT_DATA_START  0x10000000  ///< Data segment start

//
// Helper Macros
//

#define XOUT_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// Internal Functions
//

/**
  IUnknown: QueryInterface
**/
static
HRESULT
STDMETHODCALLTYPE
XenixQueryInterface (
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
XenixAddRef (
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
XenixRelease (
  IN IImageLoader  *This
  )
{
  return 1;  // Static instance
}

/**
  Check if image is XENIX X.OUT format.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  XOUT_HEADER *Header;

  if (ImageSize < sizeof(XOUT_HEADER)) {
    return S_FALSE;
  }

  Header = (XOUT_HEADER *)ImageBase;

  return (Header->Magic == XOUT_MAGIC &&
          Header->CpuType == 3 &&  // 386
          (Header->Flags & XOUT_F_EXEC)) ? S_OK : S_FALSE;
}

/**
  Get architecture from XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  XOUT_HEADER *Header;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  Header = (XOUT_HEADER *)ImageBase;

  if (Header->CpuType == XOUT_CPU_386) {
    *Architecture = Arch386;
    return S_OK;
  }

  *Architecture = ArchUnsupported;
  return IMGLOAD_E_UNSUPPORTED_ARCH;
}

/**
  Get endianness from XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // XENIX 386 is x86 little-endian
  *Endianness = ImgEndianLittle;
  return S_OK;
}

/**
  Get entry point from XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetEntryPoint (
  IN  IImageLoader     *This,
  IN  VOID             *ImageBase,
  OUT VIRTUAL_ADDRESS  *EntryPoint
  )
{
  XOUT_HEADER *Header;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  Header = (XOUT_HEADER *)ImageBase;
  *EntryPoint = XOUT_TEXT_START + Header->Entry;
  return S_OK;
}

/**
  Load XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixLoadImage (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  XOUT_HEADER *Header;
  UINT32 TextOffset, DataOffset;
  HRESULT Hr;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  Header = (XOUT_HEADER *)ImageBase;

  info("Loading XENIX X.OUT executable...");

  TextOffset = sizeof(XOUT_HEADER) + Header->ExtSize;
  DataOffset = TextOffset + Header->TextSize;

  // Load text segment (executable)
  if (Header->TextSize > 0) {
    info("  Text segment at 0x%08x (size: 0x%08x)",
         XOUT_TEXT_START, Header->TextSize);

    VirtualAddressCopy(
      XOUT_TEXT_START,
      XOUT_OFF(TextOffset),
      Header->TextSize,
      Context->IsUserMode,
      FALSE,  // Not writable
      TRUE    // Executable
    );
  }

  // Load data segment (writable)
  if (Header->DataSize > 0) {
    info("  Data segment at 0x%08x (size: 0x%08x)",
         XOUT_DATA_START, Header->DataSize);

    VirtualAddressCopy(
      XOUT_DATA_START,
      XOUT_OFF(DataOffset),
      Header->DataSize,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  // Zero-fill BSS
  if (Header->BssSize > 0) {
    info("  BSS segment at 0x%08x (size: 0x%08x)",
         XOUT_DATA_START + Header->DataSize, Header->BssSize);

    VirtualAddressMemset(
      XOUT_DATA_START + Header->DataSize,
      0,
      Header->BssSize,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  Hr = XenixGetEntryPoint(&gXenixLoader, ImageBase, &Context->EntryPoint);
  if (FAILED(Hr)) {
    return Hr;
  }

  return S_OK;
}

/**
  Extract TLS information from XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // XENIX doesn't support TLS
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  // XENIX does not have unwinding information
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  XOUT_HEADER *Header;
  XOUT_SYMBOL *Symbols;
  UINT32 NumSymbols, i;
  UINT32 SymbolOffset;
  CHAR8 *StringTable;

  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  Header = (XOUT_HEADER *)ImageBase;
  if (Header->SymbolSize == 0) {
    return S_FALSE;
  }

  // Symbol table is after text, data, and relocations
  SymbolOffset = sizeof(XOUT_HEADER) + Header->ExtSize +
                 Header->TextSize + Header->DataSize + Header->Relocations;
  Symbols = (XOUT_SYMBOL *)XOUT_OFF(SymbolOffset);
  NumSymbols = Header->SymbolSize / sizeof(XOUT_SYMBOL);

  // String table follows symbol table
  StringTable = (CHAR8 *)XOUT_OFF(SymbolOffset + Header->SymbolSize);

  // Search for symbol at address
  for (i = 0; i < NumSymbols; i++) {
    XOUT_SYMBOL *Sym = &Symbols[i];
    VIRTUAL_ADDRESS SymAddr;
    CONST CHAR8 *SymName;

    // Skip auxiliary entries
    if (Sym->AuxCount > 0) {
      i += Sym->AuxCount;
      continue;
    }

    // Calculate symbol address based on section
    switch (Sym->SectionNumber) {
      case N_TEXT:
        SymAddr = XOUT_TEXT_START + Sym->Value;
        break;

      case N_DATA:
        SymAddr = XOUT_DATA_START + Sym->Value;
        break;

      case N_BSS:
        SymAddr = XOUT_DATA_START + Header->DataSize + Sym->Value;
        break;

      case N_ABS:
        SymAddr = Sym->Value;
        break;

      default:
        continue;  // Skip undefined/unknown
    }

    if (SymAddr == Address) {
      // Get symbol name
      if (Sym->N.StringRef.Zeros == 0) {
        // Name in string table
        SymName = StringTable + Sym->N.StringRef.StringOffset;
      } else {
        // Name inline (max 8 chars)
        SymName = Sym->N.Name;
      }

      UINTN NameLen = strlen(SymName);
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
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  XOUT_HEADER *Header;
  XOUT_SYMBOL *Symbols;
  UINT32 NumSymbols, i;
  UINT32 SymbolOffset;
  CHAR8 *StringTable;
  UINTN SearchLen;

  if (SymbolInfo == NULL || Name == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  Header = (XOUT_HEADER *)ImageBase;
  if (Header->SymbolSize == 0) {
    return S_FALSE;
  }

  SearchLen = strlen(Name);

  // Symbol table is after text, data, and relocations
  SymbolOffset = sizeof(XOUT_HEADER) + Header->ExtSize +
                 Header->TextSize + Header->DataSize + Header->Relocations;
  Symbols = (XOUT_SYMBOL *)XOUT_OFF(SymbolOffset);
  NumSymbols = Header->SymbolSize / sizeof(XOUT_SYMBOL);

  // String table follows symbol table
  StringTable = (CHAR8 *)XOUT_OFF(SymbolOffset + Header->SymbolSize);

  // Search for symbol by name
  for (i = 0; i < NumSymbols; i++) {
    XOUT_SYMBOL *Sym = &Symbols[i];
    VIRTUAL_ADDRESS SymAddr;
    CONST CHAR8 *SymName;
    BOOLEAN Match = FALSE;

    // Skip auxiliary entries
    if (Sym->AuxCount > 0) {
      i += Sym->AuxCount;
      continue;
    }

    // Get symbol name
    if (Sym->N.StringRef.Zeros == 0) {
      // Name in string table
      SymName = StringTable + Sym->N.StringRef.StringOffset;
      Match = (strcmp(SymName, Name) == 0);
    } else {
      // Name inline (max 8 chars)
      UINTN InlineLen = 0;
      while (InlineLen < 8 && Sym->N.Name[InlineLen] != '\0') {
        InlineLen++;
      }
      if (InlineLen == SearchLen && memcmp(Sym->N.Name, Name, SearchLen) == 0) {
        Match = TRUE;
        SymName = Sym->N.Name;
      }
    }

    if (!Match) {
      continue;
    }

    // Calculate symbol address based on section
    switch (Sym->SectionNumber) {
      case N_TEXT:
        SymAddr = XOUT_TEXT_START + Sym->Value;
        break;

      case N_DATA:
        SymAddr = XOUT_DATA_START + Sym->Value;
        break;

      case N_BSS:
        SymAddr = XOUT_DATA_START + Header->DataSize + Sym->Value;
        break;

      case N_ABS:
        SymAddr = Sym->Value;
        break;

      default:
        continue;  // Skip undefined/unknown
    }

    UINTN NameLen = strlen(SymName);
    UINTN CopyLen = (NameLen < sizeof(SymbolInfo->Name) - 1) ?
                    NameLen : (sizeof(SymbolInfo->Name) - 1);
    memcpy(SymbolInfo->Name, SymName, CopyLen);
    SymbolInfo->Name[CopyLen] = '\0';
    SymbolInfo->Address = SymAddr;
    SymbolInfo->Size = 0;  // Unknown
    return S_OK;
  }

  return S_FALSE;
}

/**
  Extract relocation information from XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetRelocInfo (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  OUT IMGLOAD_RELOC_INFO  *RelocInfo
  )
{
  XOUT_HEADER *Header;

  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  Header = (XOUT_HEADER *)ImageBase;

  if (Header->Relocations > 0) {
    RelocInfo->PreferredBase = XOUT_TEXT_START;
    RelocInfo->RequiresReloc = TRUE;
    RelocInfo->Format = ImgRelocFormatXenix;
    return S_OK;
  }

  return S_FALSE;
}

/**
  Apply relocations to XENIX image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixApplyRelocations (
  IN IImageLoader     *This,
  IN VOID             *ImageBase,
  IN VIRTUAL_ADDRESS  LoadAddress,
  IN VIRTUAL_ADDRESS  PreferredBase
  )
{
  XOUT_HEADER *Header;
  UINT32 RelocOffset;
  UINT32 *RelocTable;
  UINT32 NumRelocs, i;
  INT32 Delta;

  Header = (XOUT_HEADER *)ImageBase;

  if (Header->Relocations == 0) {
    // No relocations
    return S_FALSE;
  }

  Delta = (INT32)((INT64)LoadAddress - (INT64)PreferredBase);
  if (Delta == 0) {
    // Already at preferred base
    return S_OK;
  }

  // Relocation table is after text and data
  RelocOffset = sizeof(XOUT_HEADER) + Header->ExtSize +
                Header->TextSize + Header->DataSize;
  RelocTable = (UINT32 *)XOUT_OFF(RelocOffset);
  NumRelocs = Header->Relocations / sizeof(UINT32);

  // XENIX relocations are simple: each entry is an offset into text segment
  // that needs to be adjusted by the load delta
  UINT8 *TextBase = (UINT8 *)XOUT_OFF(sizeof(XOUT_HEADER) + Header->ExtSize);

  for (i = 0; i < NumRelocs; i++) {
    UINT32 Offset = RelocTable[i];

    if (Offset >= Header->TextSize) {
      // Invalid offset, skip
      continue;
    }

    // Apply relocation (32-bit little-endian)
    UINT32 *Target = (UINT32 *)(TextBase + Offset);
    *Target += Delta;
  }

  return S_OK;
}

//

/**
  Get target operating system from Xenix image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetTargetSystem (
  IN  IImageLoader           *This,
  IN  VOID                   *ImageBase,
  OUT IMGLOAD_TARGET_SYSTEM  *TargetSystem
  )
{
  if (TargetSystem == NULL) {
    return E_POINTER;
  }

  *TargetSystem = ImgSystemXenix;
  return S_OK;
}

/**
  Get minimum required system version from Xenix image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetMinimumSystemVersion (
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
  Get target subsystem from Xenix image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetTargetSubsystem (
  IN  IImageLoader              *This,
  IN  VOID                      *ImageBase,
  OUT IMGLOAD_TARGET_SUBSYSTEM  *TargetSubsystem
  )
{
  if (TargetSubsystem == NULL) {
    return E_POINTER;
  }

  *TargetSubsystem = ImgSubsystemUnixCli;
  return S_OK;
}

/**
  Get minimum required subsystem version from Xenix image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetMinimumSubsystemVersion (
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
// XENIX X.OUT Loader VTable
//

static CONST IImageLoaderVtbl gXenixVtbl = {
  // IUnknown
  XenixQueryInterface,
  XenixAddRef,
  XenixRelease,
  // IImageLoader
  XenixDetect,
  XenixGetArch,
  XenixGetEndianness,
  XenixGetEntryPoint,
  XenixLoadImage,
  XenixGetTlsInfo,
  XenixGetUnwindInfo,
  XenixGetSymbolByAddress,
  XenixGetSymbolByName,
  XenixGetRelocInfo,
  XenixApplyRelocations,
  XenixGetTargetSystem,
  XenixGetMinimumSystemVersion,
  XenixGetTargetSubsystem,
  XenixGetMinimumSubsystemVersion
};

//
// XENIX X.OUT Loader Instance
//

IImageLoader gXenixLoader = {
  &gXenixVtbl
};

APXH_REGISTER_IMGLOADER(gXenixLoader);
