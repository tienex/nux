/** @file
  APXH Plan 9 a.out Loader

  Provides Plan 9 a.out format parsing and loading for Plan 9 executables.
  Plan 9 uses a custom a.out format with architecture-specific magic numbers
  calculated using the formula: ((((4*b)+0)*b)+7).

  Supports:
  - Multiple architectures (68020, 386, SPARC, MIPS, ARM)
  - Big-endian header format
  - PC/SP offset table and PC/line number table

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// Plan 9 a.out Magic Number Calculation
//

#define PLAN9_MAGIC(b)  ((((4*(b))+0)*(b))+7)

//
// Plan 9 a.out Magic Numbers
//

#define A_MAGIC     PLAN9_MAGIC(8)   ///< 68020
#define I_MAGIC     PLAN9_MAGIC(11)  ///< Intel 386
#define J_MAGIC     PLAN9_MAGIC(12)  ///< Intel 960
#define K_MAGIC     PLAN9_MAGIC(13)  ///< SPARC
#define V_MAGIC     PLAN9_MAGIC(16)  ///< MIPS 3000
#define X_MAGIC     PLAN9_MAGIC(17)  ///< ATT DSP 3210
#define M_MAGIC     PLAN9_MAGIC(18)  ///< MIPS 4000
#define D_MAGIC     PLAN9_MAGIC(19)  ///< AMD 29000
#define E_MAGIC     PLAN9_MAGIC(20)  ///< ARM
#define Q_MAGIC     PLAN9_MAGIC(21)  ///< PowerPC
#define N_MAGIC     PLAN9_MAGIC(22)  ///< MIPS 4000 BE
#define L_MAGIC     PLAN9_MAGIC(23)  ///< DEC Alpha
#define P_MAGIC     PLAN9_MAGIC(24)  ///< MIPS 3000 BE

//
// Plan 9 Exec Structure
//

ANX_PACK_PUSH(1)

typedef struct _PLAN9_EXEC {
  INT32   Magic;          ///< Magic number
  INT32   TextSize;       ///< Size of text segment
  INT32   DataSize;       ///< Size of initialized data
  INT32   BssSize;        ///< Size of uninitialized data
  INT32   SymbolSize;     ///< Size of symbol table
  INT32   Entry;          ///< Entry point
  INT32   SpszSize;       ///< Size of PC/SP offset table
  INT32   PcszSize;       ///< Size of PC/line number table
} PLAN9_EXEC;

ANX_PACK_POP()

//
// Default Plan 9 Load Addresses
//

#define PLAN9_TEXT_BASE   0x00001000  ///< Text base address
#define PLAN9_HEADER_SIZE sizeof(PLAN9_EXEC)

//
// Plan 9 Symbol Types
//

#define PLAN9_SYM_TEXT       'T'  ///< Text (code) symbol - global
#define PLAN9_SYM_TEXT_LOCAL 't'  ///< Text (code) symbol - local
#define PLAN9_SYM_DATA       'D'  ///< Data symbol - global
#define PLAN9_SYM_DATA_LOCAL 'd'  ///< Data symbol - local
#define PLAN9_SYM_BSS        'B'  ///< BSS symbol - global
#define PLAN9_SYM_BSS_LOCAL  'b'  ///< BSS symbol - local
#define PLAN9_SYM_ABS        'A'  ///< Absolute symbol - global
#define PLAN9_SYM_ABS_LOCAL  'a'  ///< Absolute symbol - local
#define PLAN9_SYM_UNDEF      'U'  ///< Undefined symbol - global
#define PLAN9_SYM_UNDEF_LOCAL 'u' ///< Undefined symbol - local

//
// Helper Macros
//

#define PLAN9_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// Byte swap for big-endian
//

static
UINT32
SwapBE32 (
  UINT32 Value
  )
{
  return ((Value & 0xFF000000) >> 24) |
         ((Value & 0x00FF0000) >> 8) |
         ((Value & 0x0000FF00) << 8) |
         ((Value & 0x000000FF) << 24);
}

//
// Internal Functions
//

/**
  IUnknown: QueryInterface
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9QueryInterface (
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
Plan9AddRef (
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
Plan9Release (
  IN IImageLoader  *This
  )
{
  return 1;  // Static instance
}

/**
  Check if image is Plan 9 a.out format.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9Detect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  PLAN9_EXEC *Header;
  UINT32 Magic;

  if (ImageSize < sizeof(PLAN9_EXEC)) {
    return S_FALSE;
  }

  Header = (PLAN9_EXEC *)ImageBase;
  Magic = SwapBE32((UINT32)Header->Magic);

  if (Magic == A_MAGIC ||
      Magic == I_MAGIC ||
      Magic == J_MAGIC ||
      Magic == K_MAGIC ||
      Magic == V_MAGIC ||
      Magic == X_MAGIC ||
      Magic == M_MAGIC ||
      Magic == D_MAGIC ||
      Magic == E_MAGIC ||
      Magic == Q_MAGIC ||
      Magic == N_MAGIC ||
      Magic == L_MAGIC ||
      Magic == P_MAGIC) {
    return S_OK;
  }

  return S_FALSE;
}

/**
  Get architecture from Plan 9 a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9GetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  PLAN9_EXEC *Header;
  UINT32 Magic;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  Header = (PLAN9_EXEC *)ImageBase;
  Magic = SwapBE32((UINT32)Header->Magic);

  switch (Magic) {
    case I_MAGIC:  // Intel 386
      *Architecture = Arch386;
      return S_OK;

    case E_MAGIC:  // ARM
      *Architecture = ArchUnsupported;
      return IMGLOAD_E_UNSUPPORTED_ARCH;

    case L_MAGIC:  // DEC Alpha
      *Architecture = ArchAlpha;
      return S_OK;

    default:
      *Architecture = ArchUnsupported;
      return IMGLOAD_E_UNSUPPORTED_ARCH;
  }
}

/**
  Get endianness from Plan 9 a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9GetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // Plan 9 a.out headers are always big-endian
  *Endianness = ImgEndianBig;
  return S_OK;
}

/**
  Get entry point from Plan 9 a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9GetEntryPoint (
  IN  IImageLoader     *This,
  IN  VOID             *ImageBase,
  OUT VIRTUAL_ADDRESS  *EntryPoint
  )
{
  PLAN9_EXEC *Header;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  Header = (PLAN9_EXEC *)ImageBase;
  *EntryPoint = SwapBE32((UINT32)Header->Entry);
  return S_OK;
}

/**
  Load Plan 9 a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9LoadImage (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  PLAN9_EXEC *Header;
  UINT32 Magic, TextSize, DataSize, BssSize;
  VIRTUAL_ADDRESS TextAddr, DataAddr, BssAddr;
  UINT32 TextOffset, DataOffset;
  HRESULT Hr;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  Header = (PLAN9_EXEC *)ImageBase;

  Magic = SwapBE32((UINT32)Header->Magic);
  TextSize = SwapBE32((UINT32)Header->TextSize);
  DataSize = SwapBE32((UINT32)Header->DataSize);
  BssSize = SwapBE32((UINT32)Header->BssSize);

  info("Loading Plan 9 a.out executable (magic: 0x%08x)...", Magic);

  TextOffset = PLAN9_HEADER_SIZE;
  DataOffset = TextOffset + TextSize;

  TextAddr = PLAN9_TEXT_BASE;
  DataAddr = TextAddr + TextSize;
  BssAddr = DataAddr + DataSize;

  // Load text segment (executable)
  if (TextSize > 0) {
    info("  Text segment at 0x%08x (size: 0x%08x)", TextAddr, TextSize);

    VasCopy(
      TextAddr,
      PLAN9_OFF(TextOffset),
      TextSize,
      Context->IsUserMode,
      FALSE,  // Not writable
      TRUE    // Executable
    );
  }

  // Load data segment (writable)
  if (DataSize > 0) {
    info("  Data segment at 0x%08x (size: 0x%08x)", DataAddr, DataSize);

    VasCopy(
      DataAddr,
      PLAN9_OFF(DataOffset),
      DataSize,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  // Zero-fill BSS
  if (BssSize > 0) {
    info("  BSS segment at 0x%08x (size: 0x%08x)", BssAddr, BssSize);

    VasFill(
      BssAddr,
      0,
      BssSize,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  Hr = Plan9GetEntryPoint(&gPlan9Loader, ImageBase, &Context->EntryPoint);
  if (FAILED(Hr)) {
    return Hr;
  }

  return S_OK;
}

/**
  Extract TLS information from Plan 9 a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9GetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // Plan 9 a.out doesn't have explicit TLS support
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from Plan 9 a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9GetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  // Plan 9 a.out does not have unwinding information
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9GetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  PLAN9_EXEC *Header;
  UINT8 *SymTable;
  UINT32 SymbolSize, TextSize, DataSize;
  UINT32 SymOffset;
  UINT8 *Ptr, *End;

  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  Header = (PLAN9_EXEC *)ImageBase;
  SymbolSize = SwapBE32((UINT32)Header->SymbolSize);

  if (SymbolSize == 0) {
    return S_FALSE;
  }

  TextSize = SwapBE32((UINT32)Header->TextSize);
  DataSize = SwapBE32((UINT32)Header->DataSize);

  // Symbol table is after text and data segments
  SymOffset = PLAN9_HEADER_SIZE + TextSize + DataSize;
  SymTable = (UINT8 *)PLAN9_OFF(SymOffset);
  End = SymTable + SymbolSize;

  // Parse symbol table entries
  Ptr = SymTable;
  while (Ptr < End) {
    UINT8 Type;
    UINT32 Value;
    CHAR8 *Name;
    UINTN NameLen;
    VIRTUAL_ADDRESS SymAddr;

    if (Ptr + 5 > End) break;  // Need at least type + value

    Type = *Ptr++;
    Value = ((UINT32)Ptr[0] << 24) |
            ((UINT32)Ptr[1] << 16) |
            ((UINT32)Ptr[2] << 8) |
            ((UINT32)Ptr[3]);
    Ptr += 4;

    // Read null-terminated name
    Name = (CHAR8 *)Ptr;
    NameLen = 0;
    while (Ptr < End && *Ptr != '\0') {
      Ptr++;
      NameLen++;
    }
    if (Ptr < End) Ptr++;  // Skip null terminator

    // Calculate symbol address based on type
    switch (Type) {
      case PLAN9_SYM_TEXT:
      case PLAN9_SYM_TEXT_LOCAL:
        SymAddr = PLAN9_TEXT_BASE + Value;
        break;

      case PLAN9_SYM_DATA:
      case PLAN9_SYM_DATA_LOCAL:
        SymAddr = PLAN9_TEXT_BASE + TextSize + Value;
        break;

      case PLAN9_SYM_BSS:
      case PLAN9_SYM_BSS_LOCAL:
        SymAddr = PLAN9_TEXT_BASE + TextSize + DataSize + Value;
        break;

      case PLAN9_SYM_ABS:
      case PLAN9_SYM_ABS_LOCAL:
        SymAddr = Value;
        break;

      default:
        continue;  // Skip undefined/unknown symbols
    }

    // Check for exact match
    if (SymAddr == Address) {
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
Plan9GetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  PLAN9_EXEC *Header;
  UINT8 *SymTable;
  UINT32 SymbolSize, TextSize, DataSize;
  UINT32 SymOffset;
  UINT8 *Ptr, *End;
  UINTN SearchLen;

  if (SymbolInfo == NULL || Name == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  Header = (PLAN9_EXEC *)ImageBase;
  SymbolSize = SwapBE32((UINT32)Header->SymbolSize);

  if (SymbolSize == 0) {
    return S_FALSE;
  }

  SearchLen = strlen(Name);
  TextSize = SwapBE32((UINT32)Header->TextSize);
  DataSize = SwapBE32((UINT32)Header->DataSize);

  // Symbol table is after text and data segments
  SymOffset = PLAN9_HEADER_SIZE + TextSize + DataSize;
  SymTable = (UINT8 *)PLAN9_OFF(SymOffset);
  End = SymTable + SymbolSize;

  // Parse symbol table entries
  Ptr = SymTable;
  while (Ptr < End) {
    UINT8 Type;
    UINT32 Value;
    CHAR8 *SymName;
    UINTN NameLen;
    VIRTUAL_ADDRESS SymAddr;

    if (Ptr + 5 > End) break;  // Need at least type + value

    Type = *Ptr++;
    Value = ((UINT32)Ptr[0] << 24) |
            ((UINT32)Ptr[1] << 16) |
            ((UINT32)Ptr[2] << 8) |
            ((UINT32)Ptr[3]);
    Ptr += 4;

    // Read null-terminated name
    SymName = (CHAR8 *)Ptr;
    NameLen = 0;
    while (Ptr < End && *Ptr != '\0') {
      Ptr++;
      NameLen++;
    }
    if (Ptr < End) Ptr++;  // Skip null terminator

    // Check if name matches
    if (NameLen == SearchLen && memcmp(SymName, Name, SearchLen) == 0) {
      // Calculate symbol address based on type
      switch (Type) {
        case PLAN9_SYM_TEXT:
        case PLAN9_SYM_TEXT_LOCAL:
          SymAddr = PLAN9_TEXT_BASE + Value;
          break;

        case PLAN9_SYM_DATA:
        case PLAN9_SYM_DATA_LOCAL:
          SymAddr = PLAN9_TEXT_BASE + TextSize + Value;
          break;

        case PLAN9_SYM_BSS:
        case PLAN9_SYM_BSS_LOCAL:
          SymAddr = PLAN9_TEXT_BASE + TextSize + DataSize + Value;
          break;

        case PLAN9_SYM_ABS:
        case PLAN9_SYM_ABS_LOCAL:
          SymAddr = Value;
          break;

        default:
          continue;  // Skip undefined/unknown symbols
      }

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
  Extract relocation information from Plan 9 a.out image.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9GetRelocInfo (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  OUT IMGLOAD_RELOC_INFO  *RelocInfo
  )
{
  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  // Plan 9 a.out is position-dependent but doesn't include relocation tables
  RelocInfo->PreferredBase = PLAN9_TEXT_BASE;
  RelocInfo->RequiresReloc = FALSE;

  return S_FALSE;
}

/**
  Apply relocations to Plan 9 a.out image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9ApplyRelocations (
  IN IImageLoader     *This,
  IN VOID             *ImageBase,
  IN VIRTUAL_ADDRESS  LoadAddress,
  IN VIRTUAL_ADDRESS  PreferredBase
  )
{
  // Plan 9 a.out doesn't include relocation information
  return E_NOTIMPL;
}

//

/**
  Get target operating system from Plan9 image.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9GetTargetSystem (
  IN  IImageLoader           *This,
  IN  VOID                   *ImageBase,
  OUT IMGLOAD_TARGET_SYSTEM  *TargetSystem
  )
{
  if (TargetSystem == NULL) {
    return E_POINTER;
  }

  *TargetSystem = ImgSystemPlan9;
  return S_OK;
}

/**
  Get minimum required system version from Plan9 image.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9GetMinimumSystemVersion (
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
  Get target subsystem from Plan9 image.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9GetTargetSubsystem (
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
  Get minimum required subsystem version from Plan9 image.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9GetMinimumSubsystemVersion (
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
// Plan 9 a.out Loader VTable
//


/**
  Get resource from Plan 9 image.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9GetResource (
  IN  IImageLoader   *This,
  IN  VOID           *ImageBase,
  IN  UINT32         TypeCode,
  IN  UINT32         Id,
  IN  CONST CHAR8    *Name,
  OUT IImageResource **Resource
  )
{
  if (Resource == NULL) {
    return E_POINTER;
  }

  *Resource = NULL;

  // Plan 9 format does not have native resources
  return S_FALSE;
}

/**
  Get resource enumerator for Plan 9 image.
**/
static
HRESULT
STDMETHODCALLTYPE
Plan9GetResourceEnumerator (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  IN  UINT32              TypeCode,
  OUT IEnumImageResource  **Enumerator
  )
{
  if (Enumerator == NULL) {
    return E_POINTER;
  }

  *Enumerator = NULL;

  // Plan 9 format does not have native resources
  return S_FALSE;
}

static CONST IImageLoaderVtbl gPlan9Vtbl = {
  // IUnknown
  Plan9QueryInterface,
  Plan9AddRef,
  Plan9Release,
  // IImageLoader
  Plan9Detect,
  Plan9GetArch,
  Plan9GetEndianness,
  Plan9GetEntryPoint,
  Plan9LoadImage,
  Plan9GetTlsInfo,
  Plan9GetUnwindInfo,
  Plan9GetSymbolByAddress,
  Plan9GetSymbolByName,
  Plan9GetRelocInfo,
  Plan9ApplyRelocations,
  Plan9GetTargetSystem,
  Plan9GetMinimumSystemVersion,
  Plan9GetTargetSubsystem,
  Plan9GetMinimumSubsystemVersion
,
  Plan9GetResource,
  Plan9GetResourceEnumerator
};

//
// Plan 9 a.out Loader Instance
//

IImageLoader gPlan9Loader = {
  &gPlan9Vtbl
};

APXH_REGISTER_IMGLOADER(gPlan9Loader);
