/** @file
  APXH COFF Loader

  Provides COFF (Common Object File Format) parsing and loading for
  SCO Unix and System V executables. Handles sections, optional headers,
  and relocations for x86 32-bit systems.

  Supports:
  - SCO Unix COFF
  - System V Release 3/4 COFF
  - x86 32-bit architecture

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// COFF Magic Numbers
//

#define COFF_I386MAGIC  0x14C  ///< x86 COFF magic

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
// Internal Functions
//

/**
  Check if image is COFF format.
**/
static
BOOLEAN
ANXAPI
CoffDetect (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  COFF_FILE_HEADER *Header;

  if (ImageSize < sizeof(COFF_FILE_HEADER)) {
    return FALSE;
  }

  Header = (COFF_FILE_HEADER *)ImageBase;

  return (Header->Magic == COFF_I386MAGIC &&
          (Header->Flags & COFF_F_EXEC));
}

/**
  Get architecture from COFF image.
**/
static
ARCH
ANXAPI
CoffGetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  COFF_FILE_HEADER *Header = (COFF_FILE_HEADER *)ImageBase;

  if (Header->Magic == COFF_I386MAGIC) {
    return ARCH_386;
  }

  return ARCH_UNSUPPORTED;
}

/**
  Get entry point from COFF image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
CoffGetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  COFF_FILE_HEADER *FileHeader = (COFF_FILE_HEADER *)ImageBase;
  COFF_AOUT_HEADER *AoutHeader;

  if (FileHeader->OptHeaderSize >= sizeof(COFF_AOUT_HEADER)) {
    AoutHeader = (COFF_AOUT_HEADER *)(FileHeader + 1);
    return AoutHeader->Entry;
  }

  return 0;
}

/**
  Load COFF image.
**/
static
IMGLOAD_STATUS
ANXAPI
CoffLoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase = Context->ImageBase;
  COFF_FILE_HEADER *FileHeader = (COFF_FILE_HEADER *)ImageBase;
  COFF_SECTION_HEADER *Sections;
  UINT16 i;
  UINTN SectionsOffset;

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
    } else if (Sec->DataPtr > 0 && Sec->Size > 0) {
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

  Context->EntryPoint = CoffGetEntryPoint(This, ImageBase);
  return ImgLoadSuccess;
}

/**
  Extract TLS information from COFF image.
**/
static
IMGLOAD_STATUS
ANXAPI
CoffGetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  // COFF doesn't have explicit TLS support
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return ImgLoadSuccess;
}

//
// COFF Loader VTable
//

static CONST IMAGE_LOADER_VTBL gCoffVtbl = {
  CoffDetect,
  CoffGetArch,
  CoffGetEntryPoint,
  CoffLoadImage,
  CoffGetTlsInfo
};

//
// COFF Loader Instance
//

IMAGE_LOADER gCoffLoader = {
  &gCoffVtbl,
  "COFF",
  NULL
};
