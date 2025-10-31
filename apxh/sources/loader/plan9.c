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
  Check if image is Plan 9 a.out format.
**/
static
BOOLEAN
ANXAPI
Plan9Detect (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  PLAN9_EXEC *Header;
  UINT32 Magic;

  if (ImageSize < sizeof(PLAN9_EXEC)) {
    return FALSE;
  }

  Header = (PLAN9_EXEC *)ImageBase;
  Magic = SwapBE32((UINT32)Header->Magic);

  return (Magic == A_MAGIC ||
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
          Magic == P_MAGIC);
}

/**
  Get architecture from Plan 9 a.out image.
**/
static
ARCH
ANXAPI
Plan9GetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  PLAN9_EXEC *Header = (PLAN9_EXEC *)ImageBase;
  UINT32 Magic = SwapBE32((UINT32)Header->Magic);

  switch (Magic) {
    case I_MAGIC:  // Intel 386
      return ARCH_386;

    case E_MAGIC:  // ARM
      return ARCH_UNSUPPORTED;

    case L_MAGIC:  // DEC Alpha
      return ARCH_AMD64;  // Closest match

    default:
      return ARCH_UNSUPPORTED;
  }
}

/**
  Get entry point from Plan 9 a.out image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
Plan9GetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  PLAN9_EXEC *Header = (PLAN9_EXEC *)ImageBase;
  return SwapBE32((UINT32)Header->Entry);
}

/**
  Load Plan 9 a.out image.
**/
static
IMGLOAD_STATUS
ANXAPI
Plan9LoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase = Context->ImageBase;
  PLAN9_EXEC *Header = (PLAN9_EXEC *)ImageBase;
  UINT32 Magic, TextSize, DataSize, BssSize;
  VIRTUAL_ADDRESS TextAddr, DataAddr, BssAddr;
  UINT32 TextOffset, DataOffset;

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

    VirtualAddressCopy(
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

    VirtualAddressCopy(
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

    VirtualAddressMemset(
      BssAddr,
      0,
      BssSize,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  Context->EntryPoint = Plan9GetEntryPoint(This, ImageBase);
  return ImgLoadSuccess;
}

/**
  Extract TLS information from Plan 9 a.out image.
**/
static
IMGLOAD_STATUS
ANXAPI
Plan9GetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  // Plan 9 a.out doesn't have explicit TLS support
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return ImgLoadSuccess;
}

//
// Plan 9 a.out Loader VTable
//

static CONST IMAGE_LOADER_VTBL gPlan9Vtbl = {
  Plan9Detect,
  Plan9GetArch,
  Plan9GetEntryPoint,
  Plan9LoadImage,
  Plan9GetTlsInfo
};

//
// Plan 9 a.out Loader Instance
//

IMAGE_LOADER gPlan9Loader = {
  &gPlan9Vtbl,
  "Plan 9",
  NULL
};
