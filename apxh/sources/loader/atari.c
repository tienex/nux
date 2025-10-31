/** @file
  APXH Atari TOS/PRG Loader

  Provides Atari TOS/PRG format parsing and loading for Atari ST/TT/Falcon
  executables. PRG is the standard executable format for Atari TOS (The
  Operating System) on Motorola 68000-based systems.

  Supports:
  - Atari ST/TT/Falcon executables
  - Motorola 68000/68020/68030/68040 architectures
  - Relocatable code with fixup tables

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>
#include <ananke/resource.h>
#include "imgresource.h"

//
// Atari PRG Magic Number
//

#define ATARI_MAGIC     0x601A  ///< Branch instruction (BRA.S)

//
// Atari PRG Flags
//

#define PF_FASTLOAD     0x00000001  ///< Fast load
#define PF_TTRAMLOAD    0x00000002  ///< May be loaded into TT-RAM
#define PF_TTRAMMEM     0x00000004  ///< May allocate TT-RAM
#define PF_SHTEXT       0x00000008  ///< Shared text

//
// Atari PRG Structure
//

ANX_PACK_PUSH(1)

typedef struct _ATARI_PRG_HEADER {
  UINT16  Magic;          ///< Magic number (0x601A)
  UINT32  TextSize;       ///< Size of text segment
  UINT32  DataSize;       ///< Size of data segment
  UINT32  BssSize;        ///< Size of BSS segment
  UINT32  SymbolSize;     ///< Size of symbol table
  UINT32  Reserved;       ///< Reserved (must be 0)
  UINT32  Flags;          ///< Program flags
  UINT16  RelocFlag;      ///< Relocation flag (0 = has relocation info)
} ATARI_PRG_HEADER;

typedef struct _ATARI_SYMBOL {
  CHAR8   Name[8];        ///< Symbol name (space-padded)
  UINT16  Type;           ///< Symbol type
  UINT32  Value;          ///< Symbol value/address
} ATARI_SYMBOL;

ANX_PACK_POP()

//
// Atari Symbol Types
//

#define ATARI_SYM_DEFINED   0x8000  ///< Defined symbol
#define ATARI_SYM_EQUATED   0x4000  ///< Equated symbol
#define ATARI_SYM_GLOBAL    0x2000  ///< Global symbol
#define ATARI_SYM_EQUATED_REG 0x1000  ///< Equated register
#define ATARI_SYM_EXTERNAL  0x0800  ///< External reference
#define ATARI_SYM_DATA      0x0400  ///< Data-based symbol
#define ATARI_SYM_TEXT      0x0200  ///< Text-based symbol
#define ATARI_SYM_BSS       0x0100  ///< BSS-based symbol

//
// Default Atari Load Addresses
//

#define ATARI_TEXT_START  0x00000100  ///< Text segment start
#define ATARI_BASE_PAGE   0x00000000  ///< Base page

//
// Helper Macros
//

#define ATARI_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// Internal Functions
//

/**
  IUnknown: QueryInterface
**/
static
HRESULT
STDMETHODCALLTYPE
AtariQueryInterface (
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
AtariAddRef (
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
AtariRelease (
  IN IImageLoader  *This
  )
{
  return 1;  // Static instance
}

/**
  Check if image is Atari PRG format.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  ATARI_PRG_HEADER *Header;

  if (ImageSize < sizeof(ATARI_PRG_HEADER)) {
    return S_FALSE;
  }

  Header = (ATARI_PRG_HEADER *)ImageBase;

  return (Header->Magic == ATARI_MAGIC) ? S_OK : S_FALSE;
}

/**
  Get architecture from Atari PRG image.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  if (Architecture == NULL) {
    return E_POINTER;
  }

  // Atari TOS is 68K only (Motorola 68000)
  *Architecture = ArchM68k;
  return S_OK;
  // Atari TOS is 68K only (Motorola 68000)
  *Architecture = ArchM68k;
  return S_OK;
  // Atari TOS is 68K only (Motorola 68000)
  *Architecture = ArchM68k;
  return S_OK;
}

/**
  Get endianness from Atari PRG image.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // Atari ST is 68K big-endian
  *Endianness = ImgEndianBig;
  return S_OK;
}

/**
  Get entry point from Atari PRG image.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariGetEntryPoint (
  IN  IImageLoader     *This,
  IN  VOID             *ImageBase,
  OUT VIRTUAL_ADDRESS  *EntryPoint
  )
{
  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  // Entry point is at start of text segment
  *EntryPoint = ATARI_TEXT_START;
  return S_OK;
}

/**
  Load Atari PRG image.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariLoadImage (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  ATARI_PRG_HEADER *Header;
  UINT32 TextOffset, DataOffset, RelocOffset;
  VIRTUAL_ADDRESS TextAddr, DataAddr, BssAddr;
  HRESULT Hr;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  Header = (ATARI_PRG_HEADER *)ImageBase;

  info("Loading Atari TOS/PRG executable...");

  TextOffset = sizeof(ATARI_PRG_HEADER);
  DataOffset = TextOffset + Header->TextSize;
  RelocOffset = DataOffset + Header->DataSize + Header->SymbolSize;

  TextAddr = ATARI_TEXT_START;
  DataAddr = TextAddr + Header->TextSize;
  BssAddr = DataAddr + Header->DataSize;

  // Load text segment (executable)
  if (Header->TextSize > 0) {
    info("  Text segment at 0x%08x (size: 0x%08x)", TextAddr, Header->TextSize);

    VasCopy(
      TextAddr,
      ATARI_OFF(TextOffset),
      Header->TextSize,
      Context->IsUserMode,
      FALSE,  // Not writable
      TRUE    // Executable
    );
  }

  // Load data segment (writable)
  if (Header->DataSize > 0) {
    info("  Data segment at 0x%08x (size: 0x%08x)", DataAddr, Header->DataSize);

    VasCopy(
      DataAddr,
      ATARI_OFF(DataOffset),
      Header->DataSize,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  // Zero-fill BSS
  if (Header->BssSize > 0) {
    info("  BSS segment at 0x%08x (size: 0x%08x)", BssAddr, Header->BssSize);

    VasFill(
      BssAddr,
      0,
      Header->BssSize,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  // Note: Relocation fixups are not implemented
  // They would require processing the fixup table at RelocOffset
  if (Header->RelocFlag == 0) {
    warn("Relocation fixups not implemented");
  }

  Hr = AtariGetEntryPoint(&gAtariLoader, ImageBase, &Context->EntryPoint);
  if (FAILED(Hr)) {
    return Hr;
  }

  return S_OK;
}

/**
  Extract TLS information from Atari PRG image.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // Atari TOS doesn't support TLS
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from Atari PRG image.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  // Atari PRG does not have unwinding information
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  ATARI_PRG_HEADER *Header;
  ATARI_SYMBOL *Symbols;
  UINT32 NumSymbols, i;
  UINT32 SymbolOffset;

  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  Header = (ATARI_PRG_HEADER *)ImageBase;
  if (Header->SymbolSize == 0) {
    return S_FALSE;
  }

  // Symbol table is after data segment
  SymbolOffset = sizeof(ATARI_PRG_HEADER) + Header->TextSize + Header->DataSize;
  Symbols = (ATARI_SYMBOL *)ATARI_OFF(SymbolOffset);
  NumSymbols = Header->SymbolSize / sizeof(ATARI_SYMBOL);

  // Search for closest symbol at or before the address
  for (i = 0; i < NumSymbols; i++) {
    ATARI_SYMBOL *Sym = &Symbols[i];
    UINT32 SymAddr = 0;

    // Calculate symbol address based on type
    if (Sym->Type & ATARI_SYM_TEXT) {
      SymAddr = ATARI_TEXT_START + Sym->Value;
    } else if (Sym->Type & ATARI_SYM_DATA) {
      SymAddr = ATARI_TEXT_START + Header->TextSize + Sym->Value;
    } else if (Sym->Type & ATARI_SYM_BSS) {
      SymAddr = ATARI_TEXT_START + Header->TextSize + Header->DataSize + Sym->Value;
    } else {
      continue;  // Skip non-address symbols
    }

    if (SymAddr == Address) {
      // Exact match
      memcpy(SymbolInfo->Name, Sym->Name, 8);
      SymbolInfo->Name[8] = '\0';
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
AtariGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  ATARI_PRG_HEADER *Header;
  ATARI_SYMBOL *Symbols;
  UINT32 NumSymbols, i;
  UINT32 SymbolOffset;
  UINTN NameLen;

  if (SymbolInfo == NULL || Name == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  Header = (ATARI_PRG_HEADER *)ImageBase;
  if (Header->SymbolSize == 0) {
    return S_FALSE;
  }

  NameLen = strlen(Name);
  if (NameLen > 8) {
    return S_FALSE;  // Atari symbols are max 8 chars
  }

  // Symbol table is after data segment
  SymbolOffset = sizeof(ATARI_PRG_HEADER) + Header->TextSize + Header->DataSize;
  Symbols = (ATARI_SYMBOL *)ATARI_OFF(SymbolOffset);
  NumSymbols = Header->SymbolSize / sizeof(ATARI_SYMBOL);

  // Search for symbol by name
  for (i = 0; i < NumSymbols; i++) {
    ATARI_SYMBOL *Sym = &Symbols[i];
    UINT32 SymAddr = 0;

    // Compare names (space-padded in Atari format)
    if (memcmp(Sym->Name, Name, NameLen) == 0) {
      // Check if rest is spaces (or null terminator matched)
      BOOLEAN Match = TRUE;
      UINT32 j;
      for (j = NameLen; j < 8; j++) {
        if (Sym->Name[j] != ' ' && Sym->Name[j] != '\0') {
          Match = FALSE;
          break;
        }
      }

      if (!Match) {
        continue;
      }

      // Calculate symbol address based on type
      if (Sym->Type & ATARI_SYM_TEXT) {
        SymAddr = ATARI_TEXT_START + Sym->Value;
      } else if (Sym->Type & ATARI_SYM_DATA) {
        SymAddr = ATARI_TEXT_START + Header->TextSize + Sym->Value;
      } else if (Sym->Type & ATARI_SYM_BSS) {
        SymAddr = ATARI_TEXT_START + Header->TextSize + Header->DataSize + Sym->Value;
      } else if (Sym->Type & ATARI_SYM_EQUATED) {
        SymAddr = Sym->Value;  // Equated symbols use absolute values
      }

      memcpy(SymbolInfo->Name, Sym->Name, 8);
      SymbolInfo->Name[8] = '\0';
      SymbolInfo->Address = SymAddr;
      SymbolInfo->Size = 0;  // Unknown
      return S_OK;
    }
  }

  return S_FALSE;
}

/**
  Extract relocation information from Atari PRG image.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariGetRelocInfo (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  OUT IMGLOAD_RELOC_INFO  *RelocInfo
  )
{
  ATARI_PRG_HEADER *Header;

  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  Header = (ATARI_PRG_HEADER *)ImageBase;

  if (Header->RelocFlag == 0) {
    // Has relocation information
    RelocInfo->PreferredBase = ATARI_TEXT_START;
    RelocInfo->RequiresReloc = TRUE;
    RelocInfo->Format = ImgRelocFormatAtari;
    return S_OK;
  }

  return S_FALSE;
}

/**
  Apply relocations to Atari PRG image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariApplyRelocations (
  IN IImageLoader     *This,
  IN VOID             *ImageBase,
  IN VIRTUAL_ADDRESS  LoadAddress,
  IN VIRTUAL_ADDRESS  PreferredBase
  )
{
  ATARI_PRG_HEADER *Header;
  UINT32 RelocOffset;
  UINT8 *RelocTable;
  UINT32 FirstOffset;
  INT32 Delta;
  UINT32 CurrentOffset;
  UINT8 NextByte;

  Header = (ATARI_PRG_HEADER *)ImageBase;

  // Check if relocations are needed
  if (Header->RelocFlag != 0) {
    // No relocation information present
    return S_FALSE;
  }

  Delta = (INT32)((INT64)LoadAddress - (INT64)PreferredBase);
  if (Delta == 0) {
    // Already at preferred base
    return S_OK;
  }

  // Calculate relocation table offset
  RelocOffset = sizeof(ATARI_PRG_HEADER) +
                Header->TextSize +
                Header->DataSize +
                Header->SymbolSize;

  RelocTable = (UINT8 *)ATARI_OFF(RelocOffset);

  // Read first fixup offset (32-bit big-endian)
  FirstOffset = ((UINT32)RelocTable[0] << 24) |
                ((UINT32)RelocTable[1] << 16) |
                ((UINT32)RelocTable[2] << 8) |
                ((UINT32)RelocTable[3]);

  if (FirstOffset == 0) {
    // No relocations needed
    return S_OK;
  }

  RelocTable += 4;
  CurrentOffset = FirstOffset;

  // Apply first relocation
  {
    UINT8 *TextBase = (UINT8 *)ATARI_OFF(sizeof(ATARI_PRG_HEADER));
    UINT32 *RelocPtr = (UINT32 *)(TextBase + CurrentOffset);
    UINT32 OldValue = ((UINT32)((UINT8 *)RelocPtr)[0] << 24) |
                      ((UINT32)((UINT8 *)RelocPtr)[1] << 16) |
                      ((UINT32)((UINT8 *)RelocPtr)[2] << 8) |
                      ((UINT32)((UINT8 *)RelocPtr)[3]);
    UINT32 NewValue = OldValue + Delta;

    ((UINT8 *)RelocPtr)[0] = (NewValue >> 24) & 0xFF;
    ((UINT8 *)RelocPtr)[1] = (NewValue >> 16) & 0xFF;
    ((UINT8 *)RelocPtr)[2] = (NewValue >> 8) & 0xFF;
    ((UINT8 *)RelocPtr)[3] = NewValue & 0xFF;
  }

  // Process relocation chain
  while ((NextByte = *RelocTable++) != 0) {
    if (NextByte == 1) {
      // Special case: add 254 to offset
      CurrentOffset += 254;
    } else {
      // Add byte value to offset
      CurrentOffset += NextByte;

      // Apply relocation at current offset
      UINT8 *TextBase = (UINT8 *)ATARI_OFF(sizeof(ATARI_PRG_HEADER));
      UINT32 *RelocPtr = (UINT32 *)(TextBase + CurrentOffset);
      UINT32 OldValue = ((UINT32)((UINT8 *)RelocPtr)[0] << 24) |
                        ((UINT32)((UINT8 *)RelocPtr)[1] << 16) |
                        ((UINT32)((UINT8 *)RelocPtr)[2] << 8) |
                        ((UINT32)((UINT8 *)RelocPtr)[3]);
      UINT32 NewValue = OldValue + Delta;

      ((UINT8 *)RelocPtr)[0] = (NewValue >> 24) & 0xFF;
      ((UINT8 *)RelocPtr)[1] = (NewValue >> 16) & 0xFF;
      ((UINT8 *)RelocPtr)[2] = (NewValue >> 8) & 0xFF;
      ((UINT8 *)RelocPtr)[3] = NewValue & 0xFF;
    }
  }

  return S_OK;
}

//

/**
  Get target operating system from Atari image.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariGetTargetSystem (
  IN  IImageLoader           *This,
  IN  VOID                   *ImageBase,
  OUT IMGLOAD_TARGET_SYSTEM  *TargetSystem
  )
{
  if (TargetSystem == NULL) {
    return E_POINTER;
  }

  *TargetSystem = ImgSystemAtariTos;
  return S_OK;
}

/**
  Get minimum required system version from Atari image.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariGetMinimumSystemVersion (
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
  Get target subsystem from Atari image.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariGetTargetSubsystem (
  IN  IImageLoader              *This,
  IN  VOID                      *ImageBase,
  OUT IMGLOAD_TARGET_SUBSYSTEM  *TargetSubsystem
  )
{
  if (TargetSubsystem == NULL) {
    return E_POINTER;
  }

  // Atari TOS used GEM (Graphical Environment Manager) as its GUI
  *TargetSubsystem = ImgSubsystemAtariGem;
  return S_OK;
}

/**
  Get minimum required subsystem version from Atari image.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariGetMinimumSubsystemVersion (
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
// Atari PRG Loader VTable
//


/**
  Get resource from Atari TOS image.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariGetResource (
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

  // Atari TOS format does not have native resources
  return S_FALSE;
}

/**
  Get resource enumerator for Atari TOS image.
**/
static
HRESULT
STDMETHODCALLTYPE
AtariGetResourceEnumerator (
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

  // Atari TOS format does not have native resources
  return S_FALSE;
}

static CONST IImageLoaderVtbl gAtariVtbl = {
  // IUnknown
  AtariQueryInterface,
  AtariAddRef,
  AtariRelease,
  // IImageLoader
  AtariDetect,
  AtariGetArch,
  AtariGetEndianness,
  AtariGetEntryPoint,
  AtariLoadImage,
  AtariGetTlsInfo,
  AtariGetUnwindInfo,
  AtariGetSymbolByAddress,
  AtariGetSymbolByName,
  AtariGetRelocInfo,
  AtariApplyRelocations,
  AtariGetTargetSystem,
  AtariGetMinimumSystemVersion,
  AtariGetTargetSubsystem,
  AtariGetMinimumSubsystemVersion
,
  AtariGetResource,
  AtariGetResourceEnumerator
};

//
// Atari PRG Loader Instance
//

IImageLoader gAtariLoader = {
  &gAtariVtbl
};

APXH_REGISTER_IMGLOADER(gAtariLoader);
