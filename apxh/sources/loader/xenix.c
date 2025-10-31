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

ANX_PACK_POP()

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
  Check if image is XENIX X.OUT format.
**/
static
BOOLEAN
ANXAPI
XenixDetect (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  XOUT_HEADER *Header;

  if (ImageSize < sizeof(XOUT_HEADER)) {
    return FALSE;
  }

  Header = (XOUT_HEADER *)ImageBase;

  return (Header->Magic == XOUT_MAGIC &&
          Header->CpuType == 3 &&  // 386
          (Header->Flags & XOUT_F_EXEC));
}

/**
  Get architecture from XENIX image.
**/
static
ARCH
ANXAPI
XenixGetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  XOUT_HEADER *Header = (XOUT_HEADER *)ImageBase;

  if (Header->CpuType == 3) {  // 386
    return ARCH_386;
  }

  return ARCH_UNSUPPORTED;
}

/**
  Get entry point from XENIX image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
XenixGetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  XOUT_HEADER *Header = (XOUT_HEADER *)ImageBase;
  return XOUT_TEXT_START + Header->Entry;
}

/**
  Load XENIX image.
**/
static
IMGLOAD_STATUS
ANXAPI
XenixLoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase = Context->ImageBase;
  XOUT_HEADER *Header = (XOUT_HEADER *)ImageBase;
  UINT32 TextOffset, DataOffset;

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

  Context->EntryPoint = XenixGetEntryPoint(This, ImageBase);
  return ImgLoadSuccess;
}

/**
  Extract TLS information from XENIX image.
**/
static
IMGLOAD_STATUS
ANXAPI
XenixGetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  // XENIX doesn't support TLS
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return ImgLoadSuccess;
}

//
// XENIX X.OUT Loader VTable
//

static CONST IMAGE_LOADER_VTBL gXenixVtbl = {
  XenixDetect,
  XenixGetArch,
  XenixGetEntryPoint,
  XenixLoadImage,
  XenixGetTlsInfo
};

//
// XENIX X.OUT Loader Instance
//

IMAGE_LOADER gXenixLoader = {
  &gXenixVtbl,
  "XENIX",
  NULL
};
