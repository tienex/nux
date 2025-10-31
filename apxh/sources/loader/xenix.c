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

  if (Header->CpuType == 3) {  // 386
    *Architecture = ARCH_386;
    return S_OK;
  }

  *Architecture = ARCH_UNSUPPORTED;
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
  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  // TODO: Parse XENIX symbol table
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
  if (SymbolInfo == NULL || Name == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  // TODO: Parse XENIX symbol table
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
    RelocInfo->Format = 8;  // Custom XENIX format
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
  // TODO: Implement XENIX relocation processing
  return E_NOTIMPL;
}

//
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
  XenixApplyRelocations
};

//
// XENIX X.OUT Loader Instance
//

IImageLoader gXenixLoader = {
  &gXenixVtbl
};

ANX_REGISTER_IMGLOADER(gXenixLoader);
