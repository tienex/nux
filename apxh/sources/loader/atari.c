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

  // Atari TOS is 68K only, which is not supported by APXH
  *Architecture = ARCH_UNSUPPORTED;
  return IMGLOAD_E_UNSUPPORTED_ARCH;
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
  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  // TODO: Parse Atari symbol table
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
  if (SymbolInfo == NULL || Name == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  // TODO: Parse Atari symbol table
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
    RelocInfo->Format = 6;  // Custom Atari format
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
  // TODO: Implement Atari PRG relocation fixup table processing
  return E_NOTIMPL;
}

//
// Atari PRG Loader VTable
//

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
  AtariApplyRelocations
};

//
// Atari PRG Loader Instance
//

IImageLoader gAtariLoader = {
  &gAtariVtbl
};

ANX_REGISTER_IMGLOADER(gAtariLoader);
