/** @file
  APXH a.out Loader

  Provides a.out (assembler output) format parsing and loading for
  classic Unix executables. Handles OMAGIC, NMAGIC, ZMAGIC, and QMAGIC
  variants for 32-bit x86 systems.

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

ANX_PACK_POP()

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
// Internal Functions
//

/**
  Check if image is a.out format.
**/
static
BOOLEAN
ANXAPI
AoutDetect (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  AOUT_HEADER *Header;

  if (ImageSize < sizeof(AOUT_HEADER)) {
    return FALSE;
  }

  Header = (AOUT_HEADER *)ImageBase;

  return (Header->Magic == AOUT_OMAGIC ||
          Header->Magic == AOUT_NMAGIC ||
          Header->Magic == AOUT_ZMAGIC ||
          Header->Magic == AOUT_QMAGIC);
}

/**
  Get architecture from a.out image.
**/
static
ARCH
ANXAPI
AoutGetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  AOUT_HEADER *Header = (AOUT_HEADER *)ImageBase;

  if (Header->MachType == AOUT_M_386) {
    return ARCH_386;
  }

  return ARCH_UNSUPPORTED;
}

/**
  Get entry point from a.out image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
AoutGetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  AOUT_HEADER *Header = (AOUT_HEADER *)ImageBase;
  return Header->Entry;
}

/**
  Load a.out image.
**/
static
IMGLOAD_STATUS
ANXAPI
AoutLoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase = Context->ImageBase;
  AOUT_HEADER *Header = (AOUT_HEADER *)ImageBase;
  UINT32 TextAddr, DataAddr, BssAddr;
  UINT32 TextOffset, DataOffset;

  info("Loading a.out executable (magic: 0%o)...", Header->Magic);

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
      return ImgLoadInvalidFormat;
  }

  // Load text segment (executable)
  if (Header->TextSize > 0) {
    info("  Text segment at 0x%08x (size: 0x%08x)", TextAddr, Header->TextSize);

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
    info("  Data segment at 0x%08x (size: 0x%08x)", DataAddr, Header->DataSize);

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
    info("  BSS segment at 0x%08x (size: 0x%08x)", BssAddr, Header->BssSize);

    VirtualAddressMemset(
      BssAddr,
      0,
      Header->BssSize,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  Context->EntryPoint = AoutGetEntryPoint(This, ImageBase);
  return ImgLoadSuccess;
}

/**
  Extract TLS information from a.out image.
**/
static
IMGLOAD_STATUS
ANXAPI
AoutGetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  // a.out doesn't support TLS
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return ImgLoadSuccess;
}

//
// a.out Loader VTable
//

static CONST IMAGE_LOADER_VTBL gAoutVtbl = {
  AoutDetect,
  AoutGetArch,
  AoutGetEntryPoint,
  AoutLoadImage,
  AoutGetTlsInfo
};

//
// a.out Loader Instance
//

IMAGE_LOADER gAoutLoader = {
  &gAoutVtbl,
  "a.out",
  NULL
};
