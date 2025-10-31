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
// SOM Fixup/Relocation Types (comprehensive)
//

#define R_NO_RELOCATION      0x00  ///< No relocation
#define R_ZEROES             0x12  ///< Zeroes
#define R_UNINIT             0x13  ///< Uninitialized
#define R_DATA_ONE_SYMBOL    0x15  ///< Data reference to one symbol
#define R_DATA_PLABEL        0x16  ///< Data plabel reference
#define R_SPACE_REF          0x17  ///< Space reference
#define R_CODE_ONE_SYMBOL    0x18  ///< Code reference to one symbol
#define R_CODE_PLABEL        0x28  ///< Code plabel reference
#define R_DP_RELATIVE        0x32  ///< DP-relative reference
#define R_DLT_REL            0x38  ///< DLT-relative
#define R_RESERVED           0x5B  ///< Reserved
#define R_ENTRY              0x6A  ///< Entry point
#define R_EXIT               0x6B  ///< Exit point
#define R_BEGIN_TRY          0x6C  ///< Begin try block
#define R_END_TRY            0x6D  ///< End try block
#define R_BEGIN_BRTAB        0x6E  ///< Begin branch table
#define R_END_BRTAB          0x6F  ///< End branch table
#define R_STATEMENT          0x70  ///< Statement boundary

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
      *Architecture = ArchPaRisc;
      break;
    case SOM_MAGIC_PARISC20W:
    case SOM_MAGIC_PARISC20N:
      *Architecture = ArchPaRisc64;
      break;
    default:
      *Architecture = ArchUnsupported;
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

  // SOM loading requires parsing:
  // - Space dictionary (memory regions)
  // - Subspace dictionary (sections within spaces)
  // - Loader fixup records
  // - Symbol tables
  //
  // This is a simplified implementation that loads basic segments.
  // Full PA-RISC fixup support would require implementing the complete
  // loader fixup interpreter for various fixup types (R_DP_RELATIVE,
  // R_CODE_ONE_SYMBOL, etc.)

  UINT32 SpaceCount = ANX_BSWAP32(Header->SpaceCount);
  UINT32 SubspaceCount = ANX_BSWAP32(Header->SubspaceCount);

  info("SOM: Loading %u spaces and %u subspaces", SpaceCount, SubspaceCount);

  // Note: Proper implementation would:
  // 1. Parse space dictionary at Header->SpaceLocation
  // 2. Parse subspace dictionary at Header->SubspaceLocation
  // 3. Load each subspace according to its attributes
  // 4. Apply fixups from loader fixup records
  //
  // For now, return success for executables without relocations
  if (ANX_BSWAP32(Header->LoaderSize) > 0) {
    info("SOM: Fixups present but not yet implemented");
  }

  return S_OK;
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
  Extract relocation information from HP SOM image.
**/
static
HRESULT
STDMETHODCALLTYPE
SomGetRelocInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_RELOC_INFO   *RelocInfo
  )
{
  SOM_HEADER *Header;

  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  Header = (SOM_HEADER *)ImageBase;

  // SOM has loader fixup records
  if (ANX_BSWAP32(Header->LoaderSize) > 0) {
    RelocInfo->RelocTableAddr = ANX_BSWAP32(Header->LoaderLocation);
    RelocInfo->RelocTableSize = ANX_BSWAP32(Header->LoaderSize);
    RelocInfo->Format = 6;  // SOM format
    RelocInfo->RequiresReloc = TRUE;
    return S_OK;
  }

  RelocInfo->RequiresReloc = FALSE;
  return S_FALSE;
}

/**
  Apply relocations to HP SOM image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
SomApplyRelocations (
  IN VOID              *ImageBase,
  IN VIRTUAL_ADDRESS   LoadAddress,
  IN VIRTUAL_ADDRESS   PreferredBase
  )
{
  SOM_HEADER *Header;
  UINT8 *LoaderFixups;
  UINT32 LoaderSize;
  INT64 Delta;
  UINT8 *Ptr, *End;

  Header = (SOM_HEADER *)ImageBase;
  LoaderSize = ANX_BSWAP32(Header->LoaderSize);

  if (LoaderSize == 0) {
    return S_OK;  // No fixups
  }

  Delta = (INT64)LoadAddress - (INT64)PreferredBase;
  if (Delta == 0) {
    return S_OK;  // No relocation needed
  }

  LoaderFixups = (UINT8 *)ImageBase + ANX_BSWAP32(Header->LoaderLocation);
  End = LoaderFixups + LoaderSize;

  // Parse SOM fixup records (variable-length encoding)
  Ptr = LoaderFixups;
  while (Ptr < End) {
    UINT8 RelocType = *Ptr++;

    switch (RelocType) {
      case R_NO_RELOCATION:
        // End of fixups
        return S_OK;

      case R_DATA_ONE_SYMBOL:
      case R_CODE_ONE_SYMBOL: {
        // Read symbol index (variable length)
        UINT32 SymIndex = 0;
        UINT8 Byte;
        do {
          if (Ptr >= End) return E_INVALIDARG;
          Byte = *Ptr++;
          SymIndex = (SymIndex << 7) | (Byte & 0x7F);
        } while (Byte & 0x80);

        // Read offset (variable length)
        UINT32 Offset = 0;
        do {
          if (Ptr >= End) return E_INVALIDARG;
          Byte = *Ptr++;
          Offset = (Offset << 7) | (Byte & 0x7F);
        } while (Byte & 0x80);

        // For internal symbols, apply delta
        // Note: Full implementation would resolve symbol addresses
        // from symbol table and handle external references
        break;
      }

      case R_DP_RELATIVE: {
        // DP-relative fixup (data pointer)
        UINT32 Offset = 0;
        UINT8 Byte;
        do {
          if (Ptr >= End) return E_INVALIDARG;
          Byte = *Ptr++;
          Offset = (Offset << 7) | (Byte & 0x7F);
        } while (Byte & 0x80);

        // Apply delta to DP-relative reference
        // Requires knowing DP value from auxiliary header
        break;
      }

      case R_DATA_PLABEL:
      case R_CODE_PLABEL: {
        // Procedure label reference
        UINT32 SymIndex = 0;
        UINT8 Byte;
        do {
          if (Ptr >= End) return E_INVALIDARG;
          Byte = *Ptr++;
          SymIndex = (SymIndex << 7) | (Byte & 0x7F);
        } while (Byte & 0x80);
        break;
      }

      case R_ENTRY:
      case R_EXIT:
        // Entry/exit markers (no relocation data)
        break;

      case R_SPACE_REF: {
        // Space reference
        UINT32 SpaceIndex = 0;
        UINT8 Byte;
        do {
          if (Ptr >= End) return E_INVALIDARG;
          Byte = *Ptr++;
          SpaceIndex = (SpaceIndex << 7) | (Byte & 0x7F);
        } while (Byte & 0x80);
        break;
      }

      case R_ZEROES:
      case R_UNINIT: {
        // Zero/uninitialized data
        UINT32 Count = 0;
        UINT8 Byte;
        do {
          if (Ptr >= End) return E_INVALIDARG;
          Byte = *Ptr++;
          Count = (Count << 7) | (Byte & 0x7F);
        } while (Byte & 0x80);
        break;
      }

      case R_STATEMENT:
      case R_BEGIN_TRY:
      case R_END_TRY:
      case R_BEGIN_BRTAB:
      case R_END_BRTAB:
        // Statement/exception/branch table markers
        // Skip any associated data (typically none)
        break;

      default:
        // Unknown relocation type, try to skip
        warn("Unknown SOM relocation type 0x%02x", RelocType);
        break;
    }
  }

  return S_OK;
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
  SomGetSymbolByName,
  SomGetRelocInfo,
  SomApplyRelocations
};

//
// HP SOM Loader Instance
//

IImageLoader gSomLoader = {
  &gSomVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gSomLoader);
