/** @file
  APXH HP SOM Loader

  Provides HP SOM (System Object Model) format implementation for PA-RISC
  executables. SOM is the native format for HP-UX and MPE/ix on PA-RISC.

  Documentation:
  - HP-UX a.out(4) manual page - SOM format specification
  - PA-RISC Runtime Architecture document
  - HP-UX include files: som.h, lst.h

  SOM File Structure:
  - File header with magic number and system_id
  - Auxiliary headers (exec_aux_hdr for executables)
  - Space dictionary and subspace dictionary
  - Symbol table and string table
  - Code and data sections

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// HP SOM Magic Numbers and Constants
//

#define SOM_MAGIC_PARISC10  0x02100106  ///< PA-RISC 1.0 executable
#define SOM_MAGIC_PARISC11  0x02100107  ///< PA-RISC 1.1 executable
#define SOM_MAGIC_PARISC20W 0x02100108  ///< PA-RISC 2.0 wide mode
#define SOM_MAGIC_PARISC20N 0x02100203  ///< PA-RISC 2.0 narrow mode

#define SOM_SYSTEM_HPUX     0x020B      ///< HP-UX system ID

//
// SOM File Header (simplified)
//

ANX_PACK_PUSH(1)

typedef struct _SOM_HEADER {
  UINT16  SystemId;          ///< System ID (0x020B for HP-UX)
  UINT16  AHeaderSize;       ///< Auxiliary header size
  UINT32  AHeaderLocation;   ///< Auxiliary header file location
  UINT32  VersionId;         ///< Version/magic
  UINT32  FileTime;          ///< Link time
  UINT32  EntrySpace;        ///< Entry space index
  UINT32  EntrySubspace;     ///< Entry subspace index
  UINT32  EntryOffset;       ///< Entry offset
  UINT32  AuxHeaderLocation; ///< Auxiliary header location
  UINT32  AuxHeaderSize;     ///< Auxiliary header size
  UINT32  SpaceLocation;     ///< Space dictionary location
  UINT32  SpaceCount;        ///< Space count
  UINT32  SubspaceLocation;  ///< Subspace dictionary location
  UINT32  SubspaceCount;     ///< Subspace count
  UINT32  LoaderLocation;    ///< Loader fixup location
  UINT32  LoaderSize;        ///< Loader fixup size
  UINT32  StringsLocation;   ///< String table location
  UINT32  StringsSize;       ///< String table size
  UINT32  SymbolLocation;    ///< Symbol table location
  UINT32  SymbolCount;       ///< Symbol count
  // Additional fields...
} SOM_HEADER;

ANX_PACK_POP()

//
// IImageLoader Implementation for HP SOM
//

/**
  Detect if image is HP SOM format.
**/
static
HRESULT
STDMETHODCALLTYPE
SomDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  SOM_HEADER *Header;

  if (ImageSize < sizeof(SOM_HEADER)) {
    return S_FALSE;
  }

  Header = (SOM_HEADER *)ImageBase;

  // Check system ID and magic (note: SOM is big-endian)
  // We need to byte-swap on little-endian systems
  UINT32 Magic = ANX_BSWAP32(Header->VersionId);
  UINT16 SysId = ANX_BSWAP16(Header->SystemId);

  if (SysId == SOM_SYSTEM_HPUX &&
      (Magic == SOM_MAGIC_PARISC10 || Magic == SOM_MAGIC_PARISC11 ||
       Magic == SOM_MAGIC_PARISC20W || Magic == SOM_MAGIC_PARISC20N)) {
    return S_OK;
  }

  return S_FALSE;
}

/**
  Get architecture from HP SOM image.
**/
static
HRESULT
STDMETHODCALLTYPE
SomGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  SOM_HEADER *Header;
  UINT32 Magic;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  Header = (SOM_HEADER *)ImageBase;
  Magic = ANX_BSWAP32(Header->VersionId);

  // Determine PA-RISC version from magic
  switch (Magic) {
    case SOM_MAGIC_PARISC10:
    case SOM_MAGIC_PARISC11:
      *Architecture = ARCH_PARISC;
      break;
    case SOM_MAGIC_PARISC20W:
    case SOM_MAGIC_PARISC20N:
      *Architecture = ARCH_PARISC64;
      break;
    default:
      *Architecture = ARCH_UNSUPPORTED;
      return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  return S_OK;
}

/**
  Get endianness from HP SOM image.
**/
static
HRESULT
STDMETHODCALLTYPE
SomGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // PA-RISC is big-endian
  *Endianness = ImgEndianBig;
  return S_OK;
}

/**
  Get entry point from HP SOM image.
**/
static
HRESULT
STDMETHODCALLTYPE
SomGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  SOM_HEADER *Header;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  Header = (SOM_HEADER *)ImageBase;

  // Entry point is EntryOffset within EntrySubspace
  *EntryPoint = ANX_BSWAP32(Header->EntryOffset);

  if (*EntryPoint == 0) {
    return IMGLOAD_E_INVALID_HEADER;
  }

  return S_OK;
}

/**
  Load HP SOM image.
**/
static
HRESULT
STDMETHODCALLTYPE
SomLoadImage (
  IN     IImageLoader     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  SOM_HEADER *Header;
  HRESULT Status;

  if (Context == NULL) {
    return E_POINTER;
  }

  Header = (SOM_HEADER *)Context->ImageBase;

  // Populate context
  Status = SomGetArch(This, Context->ImageBase, &Context->Architecture);
  if (FAILED(Status)) {
    return Status;
  }

  Status = SomGetEndianness(This, Context->ImageBase, &Context->Endianness);
  if (FAILED(Status)) {
    return Status;
  }

  Status = SomGetEntryPoint(This, Context->ImageBase, &Context->EntryPoint);
  if (FAILED(Status)) {
    return Status;
  }

  // TODO: Load spaces, subspaces, apply fixups
  // Full loading requires parsing:
  // - Space dictionary (memory regions)
  // - Subspace dictionary (sections within spaces)
  // - Loader fixup records
  // - Symbol tables

  return E_NOTIMPL;
}

/**
  Extract TLS information from HP SOM image.
**/
static
HRESULT
STDMETHODCALLTYPE
SomGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // HP-UX on PA-RISC has thread-local storage but format is complex
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from HP SOM image.
**/
static
HRESULT
STDMETHODCALLTYPE
SomGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  // PA-RISC has unwind descriptors for exception handling
  // Located in special subspace, format documented in PA-RISC Runtime Architecture
  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
SomGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  SOM_HEADER *Header;

  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  Header = (SOM_HEADER *)ImageBase;

  // Would need to parse symbol table at Header->SymbolLocation
  // SOM symbol format is documented in som.h
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
SomGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  // Would need to parse symbol and string tables
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  IUnknown::QueryInterface implementation.
**/
static
HRESULT
STDMETHODCALLTYPE
SomQueryInterface (
  IN  IImageLoader  *This,
  IN  REFIID        riid,
  OUT VOID          **ppvObject
  )
{
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  *ppvObject = NULL;

  if (memcmp(riid, &IID_IImageLoader, sizeof(GUID)) == 0 ||
      memcmp(riid, &IID_IUnknown, sizeof(GUID)) == 0) {
    *ppvObject = This;
    return S_OK;
  }

  return E_NOINTERFACE;
}

/**
  IUnknown::AddRef implementation.
**/
static
UINT32
STDMETHODCALLTYPE
SomAddRef (
  IN IImageLoader  *This
  )
{
  return 1;
}

/**
  IUnknown::Release implementation.
**/
static
UINT32
STDMETHODCALLTYPE
SomRelease (
  IN IImageLoader  *This
  )
{
  return 1;
}

//
// HP SOM Loader VTable
//

static CONST IImageLoaderVtbl gSomVtbl = {
  SomQueryInterface,
  SomAddRef,
  SomRelease,
  SomDetect,
  SomGetArch,
  SomGetEndianness,
  SomGetEntryPoint,
  SomLoadImage,
  SomGetTlsInfo,
  SomGetUnwindInfo,
  SomGetSymbolByAddress,
  SomGetSymbolByName
};

//
// HP SOM Loader Instance
//

IImageLoader gSomLoader = {
  &gSomVtbl
};

// Auto-register this loader
ANX_REGISTER_IMGLOADER(gSomLoader);
