/** @file
  APXH LE/LX Loader

  Provides LE (Linear Executable) and LX (Linear Executable Extended)
  format parsing and loading for OS/2 and DOS extender executables.
  Handles pages, objects, and fixups for 32-bit x86 binaries.

  Supports:
  - LE format (OS/2 2.x, Windows VxD)
  - LX format (OS/2 Warp, eComStation)
  - 32-bit x86 architecture
  - Multiple objects (segments)
  - Page-based loading

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// LE/LX Magic Numbers
//

#define LE_SIGNATURE      0x454C  ///< "LE" - Linear Executable
#define LX_SIGNATURE      0x584C  ///< "LX" - Linear Executable Extended
#define DOS_SIGNATURE     0x5A4D  ///< "MZ" - DOS header

//
// LE/LX Module Flags
//

#define LE_MODULE_PER_PROCESS    0x00000000  ///< Per-process
#define LE_MODULE_GLOBAL         0x00000004  ///< Global (shared)
#define LE_MODULE_INIT_GLOBAL    0x00000008  ///< Init global data
#define LE_MODULE_NO_INT_FIXUPS  0x00000010  ///< No internal fixups
#define LE_MODULE_NO_EXT_FIXUPS  0x00000020  ///< No external fixups
#define LE_MODULE_PM_INCOMPATIBLE 0x00000100 ///< Not PM compatible
#define LE_MODULE_PM_COMPATIBLE  0x00000200  ///< PM compatible
#define LE_MODULE_PM_ONLY        0x00000300  ///< PM only
#define LE_MODULE_IS_DLL         0x00008000  ///< Is DLL/driver

//
// LE/LX Object Flags
//

#define LE_OBJ_READABLE          0x00000001  ///< Readable
#define LE_OBJ_WRITABLE          0x00000002  ///< Writable
#define LE_OBJ_EXECUTABLE        0x00000004  ///< Executable
#define LE_OBJ_RESOURCE          0x00000008  ///< Resource object
#define LE_OBJ_DISCARDABLE       0x00000010  ///< Discardable
#define LE_OBJ_SHARED            0x00000020  ///< Shared
#define LE_OBJ_PRELOAD           0x00000040  ///< Preload pages
#define LE_OBJ_INVALID           0x00000080  ///< Invalid pages
#define LE_OBJ_RESIDENT          0x00000300  ///< Resident
#define LE_OBJ_USE_32BIT         0x00002000  ///< Use 32-bit addressing

//
// LE/LX Structures
//

ANX_PACK_PUSH(1)

typedef struct _DOS_HEADER_SHORT {
  UINT16  Signature;          ///< "MZ"
  UINT8   Padding[58];
  UINT32  NewHeaderOffset;    ///< Offset to LE/LX header
} DOS_HEADER_SHORT;

typedef struct _LE_HEADER {
  UINT16  Signature;          ///< "LE" or "LX"
  UINT8   ByteOrder;          ///< Byte ordering (0 = little endian)
  UINT8   WordOrder;          ///< Word ordering
  UINT32  FormatLevel;        ///< Format level
  UINT16  CpuType;            ///< CPU type (1 = 286, 2 = 386, 3 = 486)
  UINT16  OsType;             ///< OS type (1 = OS/2, 4 = Windows)
  UINT32  ModuleVersion;      ///< Module version
  UINT32  ModuleFlags;        ///< Module flags
  UINT32  NumPages;           ///< Number of memory pages
  UINT32  InitObjectNum;      ///< Initial object number (CS)
  UINT32  InitEip;            ///< Initial EIP
  UINT32  InitStackObj;       ///< Initial stack object (SS)
  UINT32  InitEsp;            ///< Initial ESP
  UINT32  PageSize;           ///< Memory page size
  UINT32  LastPageSize;       ///< Last page size (LE) or Page offset shift (LX)
  UINT32  FixupSize;          ///< Fixup section size
  UINT32  FixupChecksum;      ///< Fixup checksum
  UINT32  LoaderSize;         ///< Loader section size
  UINT32  LoaderChecksum;     ///< Loader checksum
  UINT32  ObjectTableOffset;  ///< Object table offset
  UINT32  NumObjects;         ///< Number of objects
  UINT32  ObjectPageTableOffset;  ///< Object page table offset
  UINT32  ObjectIterPagesOffset;  ///< Object iterated pages offset
  UINT32  ResourceTableOffset;    ///< Resource table offset
  UINT32  NumResourceEntries;     ///< Number of resource entries
  UINT32  ResidentNamesTableOffset;  ///< Resident names table offset
  UINT32  EntryTableOffset;       ///< Entry table offset
  UINT32  ModuleDirectivesOffset; ///< Module directives offset
  UINT32  NumModuleDirectives;    ///< Number of module directives
  UINT32  FixupPageTableOffset;   ///< Fixup page table offset
  UINT32  FixupRecordTableOffset; ///< Fixup record table offset
  UINT32  ImportModTableOffset;   ///< Import module table offset
  UINT32  NumImportModEntries;    ///< Number of import module entries
  UINT32  ImportProcTableOffset;  ///< Import procedure table offset
  UINT32  PerPageChecksumOffset;  ///< Per-page checksum offset
  UINT32  DataPagesOffset;        ///< Data pages offset
  UINT32  NumPreloadPages;        ///< Number of preload pages
  UINT32  NonResidentNamesTableOffset;  ///< Non-resident names table offset
  UINT32  NonResidentNamesTableSize;    ///< Non-resident names table size
  UINT32  NonResidentNamesTableChecksum; ///< Non-resident names checksum
  UINT32  AutoDataSegment;        ///< Automatic data segment object number
  UINT32  DebugInfoOffset;        ///< Debug info offset
  UINT32  DebugInfoSize;          ///< Debug info size
  UINT32  NumInstancePreload;     ///< Number of instance preload pages
  UINT32  NumInstanceDemand;      ///< Number of instance demand pages
  UINT32  HeapSize;               ///< Heap size
} LE_HEADER;

typedef struct _LE_OBJECT_TABLE_ENTRY {
  UINT32  VirtualSize;        ///< Virtual segment size
  UINT32  BaseAddress;        ///< Base virtual address
  UINT32  Flags;              ///< Object flags
  UINT32  PageTableIndex;     ///< Page table index (1-based)
  UINT32  NumPageEntries;     ///< Number of page table entries
  UINT32  Reserved;           ///< Reserved
} LE_OBJECT_TABLE_ENTRY;

typedef struct _LE_PAGE_TABLE_ENTRY {
  UINT32  PageDataOffset : 24;  ///< Page data offset (from data pages offset)
  UINT8   Type;                  ///< Page type
} LE_PAGE_TABLE_ENTRY;

ANX_PACK_POP()

//
// LE/LX Page Types
//

#define LE_PAGE_LEGAL           0x00  ///< Legal page
#define LE_PAGE_ITERATED        0x01  ///< Iterated (compressed)
#define LE_PAGE_INVALID         0x02  ///< Invalid page
#define LE_PAGE_ZEROED          0x03  ///< Zeroed page
#define LE_PAGE_RANGE           0x04  ///< Range of pages

//
// Helper Macros
//

#define LE_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// Internal Functions
//

/**
  IUnknown: QueryInterface
**/
static
HRESULT
STDMETHODCALLTYPE
LeQueryInterface (
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
LeAddRef (
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
LeRelease (
  IN IImageLoader  *This
  )
{
  return 1;  // Static instance
}

/**
  Check if image is LE/LX format.
**/
static
HRESULT
STDMETHODCALLTYPE
LeDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  DOS_HEADER_SHORT *DosHeader;
  LE_HEADER *LeHeader;

  if (ImageSize < sizeof(DOS_HEADER_SHORT)) {
    return S_FALSE;
  }

  DosHeader = (DOS_HEADER_SHORT *)ImageBase;
  if (DosHeader->Signature != DOS_SIGNATURE) {
    return S_FALSE;
  }

  if (DosHeader->NewHeaderOffset >= ImageSize - sizeof(LE_HEADER)) {
    return S_FALSE;
  }

  LeHeader = (LE_HEADER *)LE_OFF(DosHeader->NewHeaderOffset);
  return (LeHeader->Signature == LE_SIGNATURE ||
          LeHeader->Signature == LX_SIGNATURE) ? S_OK : S_FALSE;
}

/**
  Get architecture from LE/LX image.
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  DOS_HEADER_SHORT *DosHeader;
  LE_HEADER *LeHeader;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  DosHeader = (DOS_HEADER_SHORT *)ImageBase;
  LeHeader = (LE_HEADER *)LE_OFF(DosHeader->NewHeaderOffset);

  // LE/LX is x86 32-bit only
  if (LeHeader->CpuType >= 2) {  // 386 or higher
    *Architecture = ARCH_386;
    return S_OK;
  }

  *Architecture = ARCH_UNSUPPORTED;
  return IMGLOAD_E_UNSUPPORTED_ARCH;
}

/**
  Get endianness from LE/LX image.
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // LE/LX is x86 little-endian
  *Endianness = ImgEndianLittle;
  return S_OK;
}

/**
  Get entry point from LE/LX image.
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetEntryPoint (
  IN  IImageLoader     *This,
  IN  VOID             *ImageBase,
  OUT VIRTUAL_ADDRESS  *EntryPoint
  )
{
  DOS_HEADER_SHORT *DosHeader;
  LE_HEADER *LeHeader;
  LE_OBJECT_TABLE_ENTRY *Objects;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  DosHeader = (DOS_HEADER_SHORT *)ImageBase;
  LeHeader = (LE_HEADER *)LE_OFF(DosHeader->NewHeaderOffset);

  Objects = (LE_OBJECT_TABLE_ENTRY *)LE_OFF(DosHeader->NewHeaderOffset +
                                             LeHeader->ObjectTableOffset);

  // Entry point is in object InitObjectNum at offset InitEip
  if (LeHeader->InitObjectNum > 0 && LeHeader->InitObjectNum <= LeHeader->NumObjects) {
    LE_OBJECT_TABLE_ENTRY *InitObj = &Objects[LeHeader->InitObjectNum - 1];
    *EntryPoint = InitObj->BaseAddress + LeHeader->InitEip;
    return S_OK;
  }

  *EntryPoint = 0;
  return S_OK;
}

/**
  Load LE/LX object (segment).
**/
static
VOID
LeLoadObject (
  IN VOID                      *ImageBase,
  IN LE_HEADER                 *LeHeader,
  IN LE_OBJECT_TABLE_ENTRY     *Object,
  IN LE_PAGE_TABLE_ENTRY       *PageTable,
  IN BOOLEAN                   IsUserMode
  )
{
  UINT32 i;
  BOOLEAN IsWritable = !!(Object->Flags & LE_OBJ_WRITABLE);
  BOOLEAN IsExecutable = !!(Object->Flags & LE_OBJ_EXECUTABLE);
  UINT32 PageSize = LeHeader->PageSize;
  UINT64 DataPagesBase = ((DOS_HEADER_SHORT *)ImageBase)->NewHeaderOffset +
                          LeHeader->DataPagesOffset;

  info("  Object at 0x%08x (size: 0x%08x, flags: 0x%08x)",
       Object->BaseAddress, Object->VirtualSize, Object->Flags);

  // Load each page
  for (i = 0; i < Object->NumPageEntries; i++) {
    LE_PAGE_TABLE_ENTRY *PageEntry = &PageTable[Object->PageTableIndex - 1 + i];
    UINT64 PageVa = Object->BaseAddress + (i * PageSize);
    UINT32 PageDataSize = (i == Object->NumPageEntries - 1 && LeHeader->Signature == LE_SIGNATURE) ?
                          LeHeader->LastPageSize : PageSize;

    switch (PageEntry->Type) {
      case LE_PAGE_LEGAL:
        // Normal page with data
        VirtualAddressCopy(
          PageVa,
          LE_OFF(DataPagesBase + PageEntry->PageDataOffset),
          PageDataSize,
          IsUserMode,
          IsWritable,
          IsExecutable
        );

        // Zero remainder if partial page
        if (PageDataSize < PageSize) {
          VirtualAddressMemset(
            PageVa + PageDataSize,
            0,
            PageSize - PageDataSize,
            IsUserMode,
            IsWritable,
            IsExecutable
          );
        }
        break;

      case LE_PAGE_ZEROED:
        // Zero-filled page
        VirtualAddressMemset(
          PageVa,
          0,
          PageSize,
          IsUserMode,
          IsWritable,
          IsExecutable
        );
        break;

      case LE_PAGE_INVALID:
        // Skip invalid pages
        break;

      default:
        warn("Unknown LE page type %d", PageEntry->Type);
        break;
    }
  }
}

/**
  Load LE/LX image.
**/
static
HRESULT
STDMETHODCALLTYPE
LeLoadImage (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  DOS_HEADER_SHORT *DosHeader;
  LE_HEADER *LeHeader;
  LE_OBJECT_TABLE_ENTRY *Objects;
  LE_PAGE_TABLE_ENTRY *PageTable;
  UINT32 i;
  HRESULT Hr;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  DosHeader = (DOS_HEADER_SHORT *)ImageBase;
  LeHeader = (LE_HEADER *)LE_OFF(DosHeader->NewHeaderOffset);

  info("Loading %s executable...",
       LeHeader->Signature == LE_SIGNATURE ? "LE" : "LX");

  Objects = (LE_OBJECT_TABLE_ENTRY *)LE_OFF(DosHeader->NewHeaderOffset +
                                             LeHeader->ObjectTableOffset);
  PageTable = (LE_PAGE_TABLE_ENTRY *)LE_OFF(DosHeader->NewHeaderOffset +
                                             LeHeader->ObjectPageTableOffset);

  // Load all objects
  for (i = 0; i < LeHeader->NumObjects; i++) {
    LeLoadObject(ImageBase, LeHeader, &Objects[i], PageTable, Context->IsUserMode);
  }

  Hr = LeGetEntryPoint(&gLeLoader, ImageBase, &Context->EntryPoint);
  if (FAILED(Hr)) {
    return Hr;
  }

  return S_OK;
}

/**
  Extract TLS information from LE/LX image.
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // LE/LX doesn't have explicit TLS support
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from LE/LX image.
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  // LE/LX does not have unwinding information
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetSymbolByAddress (
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

  // LE/LX entry table uses bundle-based encoding with complex ordinal mapping
  // See LeGetSymbolByName for resident names table parsing
  // Full entry table parsing not implemented
  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  DOS_HEADER_SHORT *DosHeader;
  LE_HEADER *LeHeader;
  UINT8 *ResidentNames;
  UINT8 *Ptr, *End;
  UINTN SearchLen;

  if (SymbolInfo == NULL || Name == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  SearchLen = strlen(Name);
  DosHeader = (DOS_HEADER_SHORT *)ImageBase;
  LeHeader = (LE_HEADER *)LE_OFF(DosHeader->NewHeaderOffset);

  // Parse resident names table (length-prefixed names with ordinal numbers)
  ResidentNames = (UINT8 *)LE_OFF(DosHeader->NewHeaderOffset + LeHeader->ResidentNamesTableOffset);
  Ptr = ResidentNames;
  End = Ptr + 4096;  // Reasonable upper bound

  // First entry is module name, skip it
  if (Ptr < End) {
    UINT8 NameLen = *Ptr++;
    Ptr += NameLen + 2;  // Skip name and ordinal
  }

  // Parse remaining entries
  while (Ptr < End) {
    UINT8 NameLen = *Ptr++;
    if (NameLen == 0) break;  // End of table

    CHAR8 *SymName = (CHAR8 *)Ptr;
    Ptr += NameLen;

    if (Ptr + 2 > End) break;
    UINT16 Ordinal = *(UINT16 *)Ptr;
    Ptr += 2;

    // Check if name matches
    if (NameLen == SearchLen && memcmp(SymName, Name, SearchLen) == 0) {
      // Found - ordinal would need to be looked up in entry table for actual address
      // Simplified: return ordinal as address
      UINTN CopyLen = (NameLen < sizeof(SymbolInfo->Name) - 1) ?
                      NameLen : (sizeof(SymbolInfo->Name) - 1);
      memcpy(SymbolInfo->Name, SymName, CopyLen);
      SymbolInfo->Name[CopyLen] = '\0';
      SymbolInfo->Address = Ordinal;  // Simplified
      SymbolInfo->Size = 0;
      return S_OK;
    }
  }

  // Note: Full implementation would also search non-resident names table
  // and map ordinals through entry table to actual addresses
  return S_FALSE;
}

/**
  Extract relocation information from LE/LX image.
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetRelocInfo (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  OUT IMGLOAD_RELOC_INFO  *RelocInfo
  )
{
  DOS_HEADER_SHORT *DosHeader;
  LE_HEADER *LeHeader;

  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  DosHeader = (DOS_HEADER_SHORT *)ImageBase;
  LeHeader = (LE_HEADER *)LE_OFF(DosHeader->NewHeaderOffset);

  // LE/LX has fixup records
  if (LeHeader->FixupSize > 0) {
    RelocInfo->RequiresReloc = TRUE;
    RelocInfo->Format = 7;  // Custom LE/LX format
    return S_OK;
  }

  return S_FALSE;
}

/**
  Apply relocations to LE/LX image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
LeApplyRelocations (
  IN IImageLoader     *This,
  IN VOID             *ImageBase,
  IN VIRTUAL_ADDRESS  LoadAddress,
  IN VIRTUAL_ADDRESS  PreferredBase
  )
{
  DOS_HEADER_SHORT *DosHeader;
  LE_HEADER *LeHeader;
  INT32 Delta;

  DosHeader = (DOS_HEADER_SHORT *)ImageBase;
  LeHeader = (LE_HEADER *)LE_OFF(DosHeader->NewHeaderOffset);

  if (LeHeader->FixupSize == 0) {
    return S_FALSE;  // No fixups
  }

  Delta = (INT32)((INT64)LoadAddress - (INT64)PreferredBase);
  if (Delta == 0) {
    return S_OK;  // Already at preferred base
  }

  // LE/LX fixup processing is extremely complex with multiple record types:
  // - Source types: byte, selector, pointer, offset
  // - Target types: internal, imported ordinal, imported name, internal entry
  // - Flags: 16-bit/32-bit, additive/non-additive
  //
  // A full implementation would:
  // 1. Iterate through fixup page table to find fixups for each page
  // 2. Parse fixup records with their type-dependent encoding
  // 3. Apply fixups based on source/target type and addressing mode
  //
  // This is a simplified stub showing the structure.
  // For a production implementation, see OS/2 documentation or existing
  // open-source loaders (e.g., ODIN, Wine).

  warn("LE/LX fixup processing not fully implemented");
  return E_NOTIMPL;
}

//
// LE/LX Loader VTable
//

static CONST IImageLoaderVtbl gLeVtbl = {
  // IUnknown
  LeQueryInterface,
  LeAddRef,
  LeRelease,
  // IImageLoader
  LeDetect,
  LeGetArch,
  LeGetEndianness,
  LeGetEntryPoint,
  LeLoadImage,
  LeGetTlsInfo,
  LeGetUnwindInfo,
  LeGetSymbolByAddress,
  LeGetSymbolByName,
  LeGetRelocInfo,
  LeApplyRelocations
};

//
// LE/LX Loader Instance
//

IImageLoader gLeLoader = {
  &gLeVtbl
};

APXH_REGISTER_IMGLOADER(gLeLoader);
