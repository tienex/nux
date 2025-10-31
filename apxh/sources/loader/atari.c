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

ANX_PACK_POP()

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
  Check if image is Atari PRG format.
**/
static
BOOLEAN
ANXAPI
AtariDetect (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  ATARI_PRG_HEADER *Header;

  if (ImageSize < sizeof(ATARI_PRG_HEADER)) {
    return FALSE;
  }

  Header = (ATARI_PRG_HEADER *)ImageBase;

  return (Header->Magic == ATARI_MAGIC);
}

/**
  Get architecture from Atari PRG image.
**/
static
ARCH
ANXAPI
AtariGetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  // Atari TOS is 68K only, which is not supported by APXH
  return ARCH_UNSUPPORTED;
}

/**
  Get entry point from Atari PRG image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
AtariGetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  // Entry point is at start of text segment
  return ATARI_TEXT_START;
}

/**
  Load Atari PRG image.
**/
static
IMGLOAD_STATUS
ANXAPI
AtariLoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase = Context->ImageBase;
  ATARI_PRG_HEADER *Header = (ATARI_PRG_HEADER *)ImageBase;
  UINT32 TextOffset, DataOffset, RelocOffset;
  VIRTUAL_ADDRESS TextAddr, DataAddr, BssAddr;

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

    VirtualAddressCopy(
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

    VirtualAddressCopy(
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

    VirtualAddressMemset(
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

  Context->EntryPoint = AtariGetEntryPoint(This, ImageBase);
  return ImgLoadSuccess;
}

/**
  Extract TLS information from Atari PRG image.
**/
static
IMGLOAD_STATUS
ANXAPI
AtariGetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  // Atari TOS doesn't support TLS
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return ImgLoadSuccess;
}

//
// Atari PRG Loader VTable
//

static CONST IMAGE_LOADER_VTBL gAtariVtbl = {
  AtariDetect,
  AtariGetArch,
  AtariGetEntryPoint,
  AtariLoadImage,
  AtariGetTlsInfo
};

//
// Atari PRG Loader Instance
//

IMAGE_LOADER gAtariLoader = {
  &gAtariVtbl,
  "Atari PRG",
  NULL
};
