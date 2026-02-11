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
#include <ananke/resource.h>
#include "imgresource.h"

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

/**
  LE/LX Resource Table Entry.

  LE/LX uses a resource table similar to NE format.
  Both OS/2 and Windows VxD use the same structure, but with different type values:
  - OS/2 (OsType = 1): OS/2-specific resource types
  - Windows VxD (OsType = 4): Windows resource types (RT_BITMAP, RT_ICON, etc.)

  Resources are organized by type, then by ID/name.
**/
typedef struct _LE_RESOURCE_ENTRY {
  UINT16  TypeId;       ///< Resource type ID (OS/2 or Windows types depending on OsType)
  UINT16  NameId;       ///< Resource name ID
  UINT32  ResourceSize; ///< Size of resource data
  UINT16  ObjectNum;    ///< Object number containing resource (1-based)
  UINT32  Offset;       ///< Offset within object
} LE_RESOURCE_ENTRY;

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
// LE/LX Fixup Source Types
//

#define FIXUP_SRC_BYTE          0x00  ///< Low byte
#define FIXUP_SRC_SEL           0x02  ///< 16-bit selector
#define FIXUP_SRC_PTR16         0x03  ///< 16:16 pointer
#define FIXUP_SRC_OFF16         0x05  ///< 16-bit offset
#define FIXUP_SRC_PTR32         0x06  ///< 16:32 pointer
#define FIXUP_SRC_OFF32         0x07  ///< 32-bit offset
#define FIXUP_SRC_REL32         0x08  ///< 32-bit relative offset

//
// LE/LX Fixup Flags
//

#define FIXUP_FLAGS_TARGET_MASK 0x03  ///< Target type mask
#define FIXUP_FLAGS_ADDITIVE    0x04  ///< Additive fixup
#define FIXUP_FLAGS_32BIT       0x10  ///< 32-bit target
#define FIXUP_FLAGS_16BIT       0x20  ///< 16-bit target
#define FIXUP_FLAGS_8BIT        0x00  ///< 8-bit target
#define FIXUP_FLAGS_ALIAS       0x10  ///< Alias flag

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
    *Architecture = Arch386;
    return S_OK;
  }

  *Architecture = ArchUnsupported;
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
        VasCopy(
          PageVa,
          LE_OFF(DataPagesBase + PageEntry->PageDataOffset),
          PageDataSize,
          IsUserMode,
          IsWritable,
          IsExecutable
        );

        // Zero remainder if partial page
        if (PageDataSize < PageSize) {
          VasFill(
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
        VasFill(
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
    RelocInfo->Format = ImgRelocFormatLe;
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
  LE_OBJECT_TABLE_ENTRY *Objects;
  UINT32 *FixupPageTable;
  UINT8 *FixupRecords;
  INT32 Delta;
  UINT32 i;

  DosHeader = (DOS_HEADER_SHORT *)ImageBase;
  LeHeader = (LE_HEADER *)LE_OFF(DosHeader->NewHeaderOffset);

  if (LeHeader->FixupSize == 0) {
    return S_FALSE;  // No fixups
  }

  Delta = (INT32)((INT64)LoadAddress - (INT64)PreferredBase);
  if (Delta == 0) {
    return S_OK;  // Already at preferred base
  }

  // Get object table for address calculations
  Objects = (LE_OBJECT_TABLE_ENTRY *)LE_OFF(
    DosHeader->NewHeaderOffset + LeHeader->ObjectTableOffset
  );

  // Get fixup page table and records
  FixupPageTable = (UINT32 *)LE_OFF(
    DosHeader->NewHeaderOffset + LeHeader->FixupPageTableOffset
  );
  FixupRecords = (UINT8 *)LE_OFF(
    DosHeader->NewHeaderOffset + LeHeader->FixupRecordTableOffset
  );

  // Process fixups for each page
  for (i = 0; i < LeHeader->NumPages; i++) {
    UINT32 FixupStart = FixupPageTable[i];
    UINT32 FixupEnd = FixupPageTable[i + 1];
    UINT8 *Fixup = FixupRecords + FixupStart;

    if (FixupStart == FixupEnd) {
      continue;  // No fixups for this page
    }

    // Parse fixup records for this page
    while (Fixup < FixupRecords + FixupEnd) {
      UINT8 SourceType = *Fixup++;
      UINT8 Flags = *Fixup++;
      UINT16 SourceOffset = *(UINT16 *)Fixup;
      Fixup += 2;

      UINT8 TargetType = Flags & FIXUP_FLAGS_TARGET_MASK;
      UINT32 TargetAddr = 0;
      BOOLEAN ApplyFixup = FALSE;

      // Parse target based on type
      switch (TargetType) {
        case 0: {  // Internal reference
          UINT8 ObjectNum = *Fixup++;
          UINT32 TargetOffset;

          if (!(Flags & FIXUP_FLAGS_16BIT)) {
            TargetOffset = *(UINT32 *)Fixup;
            Fixup += 4;
          } else {
            TargetOffset = *(UINT16 *)Fixup;
            Fixup += 2;
          }

          // Calculate target address from object table
          if (ObjectNum > 0 && ObjectNum <= LeHeader->NumObjects) {
            LE_OBJECT_TABLE_ENTRY *TargetObj = &Objects[ObjectNum - 1];
            TargetAddr = TargetObj->BaseAddress + TargetOffset;
            ApplyFixup = TRUE;
          }
          break;
        }

        case 1: {  // Imported by ordinal
          Fixup++;  // Module ordinal
          if (Flags & FIXUP_FLAGS_16BIT) {
            Fixup += 2;
          } else {
            Fixup++;
          }
          if (Flags & FIXUP_FLAGS_ADDITIVE) {
            Fixup += 4;
          }
          // Skip import fixups (require external resolution)
          break;
        }

        case 2:  // Imported by name
          Fixup++;  // Module ordinal
          Fixup += 4;  // Procedure name offset
          if (Flags & FIXUP_FLAGS_ADDITIVE) {
            Fixup += 4;
          }
          break;

        case 3: {  // Internal entry table
          UINT8 ObjectNum = *Fixup++;
          if (Flags & FIXUP_FLAGS_16BIT) {
            Fixup += 2;  // Ordinal
          } else {
            Fixup++;
          }
          if (Flags & FIXUP_FLAGS_ADDITIVE) {
            Fixup += 4;
          }
          // Could resolve through entry table, skip for now
          break;
        }
      }

      // Apply fixup if we have a valid target
      if (ApplyFixup) {
        // Calculate source address in loaded image
        UINT32 PageAddr = i * LeHeader->PageSize;
        UINT8 *SourcePtr = (UINT8 *)ImageBase + PageAddr + SourceOffset;

        // Apply fixup based on source type
        switch (SourceType) {
          case FIXUP_SRC_OFF32: {
            UINT32 *Ptr = (UINT32 *)SourcePtr;
            if (Flags & FIXUP_FLAGS_ADDITIVE) {
              *Ptr += TargetAddr + Delta;
            } else {
              *Ptr = TargetAddr + Delta;
            }
            break;
          }

          case FIXUP_SRC_OFF16: {
            UINT16 *Ptr = (UINT16 *)SourcePtr;
            if (Flags & FIXUP_FLAGS_ADDITIVE) {
              *Ptr += (UINT16)(TargetAddr + Delta);
            } else {
              *Ptr = (UINT16)(TargetAddr + Delta);
            }
            break;
          }

          case FIXUP_SRC_BYTE: {
            if (Flags & FIXUP_FLAGS_ADDITIVE) {
              *SourcePtr += (UINT8)(TargetAddr + Delta);
            } else {
              *SourcePtr = (UINT8)(TargetAddr + Delta);
            }
            break;
          }

          case FIXUP_SRC_SEL: {
            // Selector fixup (16-bit segment selector)
            UINT16 *Ptr = (UINT16 *)SourcePtr;
            *Ptr = (UINT16)((TargetAddr + Delta) >> 16);
            break;
          }

          case FIXUP_SRC_PTR32: {
            // 16:32 pointer fixup
            UINT16 *Sel = (UINT16 *)SourcePtr;
            UINT32 *Off = (UINT32 *)(SourcePtr + 2);
            *Sel = (UINT16)((TargetAddr + Delta) >> 16);
            *Off = (TargetAddr + Delta) & 0xFFFFFFFF;
            break;
          }

          case FIXUP_SRC_REL32: {
            // 32-bit relative offset
            UINT32 *Ptr = (UINT32 *)SourcePtr;
            UINT32 SourceAddr = PageAddr + SourceOffset;
            *Ptr = (TargetAddr + Delta) - (SourceAddr + 4);
            break;
          }
        }
      }
    }
  }

  return S_OK;
}

//

/**
  Get target operating system from Le image.

  LE/LX format was used by multiple operating systems:
  - OS/2 (OsType = 1)
  - Windows VxD (OsType = 4) - Virtual Device Drivers for Windows 3.x/9x
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetTargetSystem (
  IN  IImageLoader           *This,
  IN  VOID                   *ImageBase,
  OUT IMGLOAD_TARGET_SYSTEM  *TargetSystem
  )
{
  DOS_HEADER_SHORT *DosHeader;
  LE_HEADER *LeHeader;

  if (TargetSystem == NULL) {
    return E_POINTER;
  }

  DosHeader = (DOS_HEADER_SHORT *)ImageBase;
  LeHeader = (LE_HEADER *)LE_OFF(DosHeader->NewHeaderOffset);

  // Determine target OS based on OsType field
  switch (LeHeader->OsType) {
    case 1:
      *TargetSystem = ImgSystemOs2;
      break;
    case 4:
      *TargetSystem = ImgSystemWindows;
      break;
    default:
      // Unknown OS type, default to OS/2
      *TargetSystem = ImgSystemOs2;
      break;
  }

  return S_OK;
}

/**
  Get minimum required system version from Le image.
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetMinimumSystemVersion (
  IN  IImageLoader            *This,
  IN  VOID                    *ImageBase,
  OUT IMGLOAD_SYSTEM_VERSION  *MinimumVersion
  )
{
  if (MinimumVersion == NULL) {
    return E_POINTER;
  }

  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

/**
  Get target subsystem from Le image.

  Distinguishes between OS/2 applications and Windows VxD drivers.
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetTargetSubsystem (
  IN  IImageLoader              *This,
  IN  VOID                      *ImageBase,
  OUT IMGLOAD_TARGET_SUBSYSTEM  *TargetSubsystem
  )
{
  DOS_HEADER_SHORT *DosHeader;
  LE_HEADER *Header;
  UINT32 PmFlags;

  if (TargetSubsystem == NULL) {
    return E_POINTER;
  }

  DosHeader = (DOS_HEADER_SHORT *)ImageBase;
  Header = (LE_HEADER *)LE_OFF(DosHeader->NewHeaderOffset);

  // Check OS type first
  if (Header->OsType == 4) {
    // Windows VxD (Virtual Device Driver)
    *TargetSubsystem = ImgSubsystemNative;  // Kernel-mode driver
    return S_OK;
  }

  // OS/2: Check PM (Presentation Manager) compatibility flags
  PmFlags = Header->ModuleFlags & 0x00000300;

  switch (PmFlags) {
    case LE_MODULE_PM_ONLY:
      // PM-only application (GUI required)
      *TargetSubsystem = ImgSubsystemOs2Gui;
      break;

    case LE_MODULE_PM_COMPATIBLE:
      // PM-compatible application (can run in PM, typically GUI)
      *TargetSubsystem = ImgSubsystemOs2Gui;
      break;

    case LE_MODULE_PM_INCOMPATIBLE:
    default:
      // PM-incompatible or no PM flags (console/text mode)
      *TargetSubsystem = ImgSubsystemOs2Cui;
      break;
  }

  return S_OK;
}

/**
  Get minimum required subsystem version from Le image.
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetMinimumSubsystemVersion (
  IN  IImageLoader            *This,
  IN  VOID                    *ImageBase,
  OUT IMGLOAD_SYSTEM_VERSION  *MinimumVersion
  )
{
  if (MinimumVersion == NULL) {
    return E_POINTER;
  }

  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

/**
  Get LE/LX init/fini information.

  OS/2 LE/LX uses entry point and module flags for initialization:
  - InitObjectNum/InitEip: Entry point for DLL initialization
  - LE_MODULE_INIT_GLOBAL: Global initialization flag
  - LE_MODULE_IS_DLL: Indicates library vs executable

  @param[in]  ImageBase       Pointer to LE/LX image.
  @param[out] InitAddress     Receives init entry point address (0 if none).
  @param[out] FiniAddress     Receives fini address (0 if none - not supported in LE/LX).
  @param[out] IsGlobalInit    Receives TRUE if global initialization.
  @param[out] IsDll           Receives TRUE if DLL/driver.

  @return S_OK if init info found, S_FALSE if not found.
**/
static
HRESULT
LeGetInitFini (
  IN  VOID     *ImageBase,
  OUT UINT64   *InitAddress,
  OUT UINT64   *FiniAddress,
  OUT BOOLEAN  *IsGlobalInit,
  OUT BOOLEAN  *IsDll
  )
{
  DOS_HEADER_SHORT       *DosHeader;
  LE_HEADER              *LeHeader;
  LE_OBJECT_TABLE_ENTRY  *Objects;

  if (ImageBase == NULL) {
    return E_POINTER;
  }

  // Initialize outputs
  if (InitAddress != NULL) *InitAddress = 0;
  if (FiniAddress != NULL) *FiniAddress = 0;  // LE/LX doesn't have explicit fini
  if (IsGlobalInit != NULL) *IsGlobalInit = FALSE;
  if (IsDll != NULL) *IsDll = FALSE;

  DosHeader = (DOS_HEADER_SHORT *)ImageBase;
  LeHeader = (LE_HEADER *)LE_OFF(DosHeader->NewHeaderOffset);

  // Check if this is a DLL
  if (IsDll != NULL) {
    *IsDll = (LeHeader->ModuleFlags & LE_MODULE_IS_DLL) != 0;
  }

  // Check if global initialization is required
  if (IsGlobalInit != NULL) {
    *IsGlobalInit = (LeHeader->ModuleFlags & LE_MODULE_INIT_GLOBAL) != 0;
  }

  // Get initialization entry point (for DLLs)
  if (InitAddress != NULL && LeHeader->InitObjectNum > 0 &&
      LeHeader->InitObjectNum <= LeHeader->NumObjects) {
    Objects = (LE_OBJECT_TABLE_ENTRY *)
      LE_OFF(DosHeader->NewHeaderOffset + LeHeader->ObjectTableOffset);

    LE_OBJECT_TABLE_ENTRY *InitObj = &Objects[LeHeader->InitObjectNum - 1];
    *InitAddress = InitObj->BaseAddress + LeHeader->InitEip;
  }

  return S_OK;
}

/**
  Find native resource in LE/LX image.

  Searches the LE/LX resource table for the specified resource.

  LE/LX format was used by both OS/2 and Windows:
  - OS/2 (OsType = 1): Uses OS/2 resource format with OS/2-specific types
  - Windows VxD (OsType = 4): Uses Windows resource format with Windows-specific types (RT_BITMAP, RT_ICON, etc.)

  Both use the same LE_RESOURCE_ENTRY table structure, but resource type
  values and interpretation differ based on the target OS.

  @param[in]  ImageBase    Pointer to LE/LX image.
  @param[in]  TypeCode     Resource type code (4-char or numeric).
  @param[in]  Id           Resource ID (0 if using name).
  @param[in]  Name         Resource name (NULL if using ID).
  @param[out] Data         Receives pointer to resource data.
  @param[out] Size         Receives size of resource data.

  @return S_OK if found, S_FALSE if not found, error code otherwise.
**/
static
HRESULT
LeFindNativeResource (
  IN  VOID         *ImageBase,
  IN  UINT32       TypeCode,
  IN  UINT32       Id,
  IN  CONST CHAR8  *Name,
  OUT VOID         **Data,
  OUT UINT64       *Size
  )
{
  DOS_HEADER_SHORT  *DosHeader;
  LE_HEADER         *LeHeader;
  LE_RESOURCE_ENTRY *Resources;
  UINT32            i;
  UINT16            TypeId;

  if (ImageBase == NULL || Data == NULL || Size == NULL) {
    return E_POINTER;
  }

  *Data = NULL;
  *Size = 0;

  DosHeader = (DOS_HEADER_SHORT *)ImageBase;
  LeHeader = (LE_HEADER *)LE_OFF(DosHeader->NewHeaderOffset);

  if (LeHeader->ResourceTableOffset == 0 || LeHeader->NumResourceEntries == 0) {
    return S_FALSE;  // No resources
  }

  // Note: Resource table format is the same for OS/2 (OsType=1) and Windows (OsType=4),
  // but resource type values differ (OS/2 types vs Windows RT_* types)
  Resources = (LE_RESOURCE_ENTRY *)LE_OFF(DosHeader->NewHeaderOffset + LeHeader->ResourceTableOffset);

  // Convert type code to type ID (for simplicity, use lower 16 bits)
  TypeId = (UINT16)(TypeCode & 0xFFFF);

  // Search for matching resource
  for (i = 0; i < LeHeader->NumResourceEntries; i++) {
    if (Resources[i].TypeId == TypeId) {
      // Type matches, now check ID/name
      if (Name != NULL) {
        // Name-based lookup not yet implemented for LE/LX
        continue;
      } else if (Resources[i].NameId == (UINT16)Id) {
        // Found matching resource by ID
        LE_OBJECT_TABLE_ENTRY *Objects = (LE_OBJECT_TABLE_ENTRY *)
          LE_OFF(DosHeader->NewHeaderOffset + LeHeader->ObjectTableOffset);

        if (Resources[i].ObjectNum > 0 && Resources[i].ObjectNum <= LeHeader->NumObjects) {
          LE_OBJECT_TABLE_ENTRY *Obj = &Objects[Resources[i].ObjectNum - 1];

          *Data = (UINT8 *)ImageBase + Obj->BaseAddress + Resources[i].Offset;
          *Size = Resources[i].ResourceSize;
          return S_OK;
        }
      }
    }
  }

  return S_FALSE;  // Resource not found
}

/**
  Get resource from LE/LX image.

  Uses hybrid strategy: tries native OS/2 resources first, then .axursrc section.
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetResource (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  IN  IMGLOAD_RESOURCE_ID *ResourceId,
  IN  IMGLOAD_RESOURCE_ID *ResourceType,
  OUT IImageResource      **Resource
  )
{
  VOID *ResourceFork;
  UINT64 Size;
  BOOLEAN NeedsFree;
  HRESULT Status;
  UINT32 TypeCode;
  UINT16 Id;
  CONST CHAR8 *Name;

  if (ImageBase == NULL || Resource == NULL) {
    return E_POINTER;
  }

  *Resource = NULL;

  // Extract type code from ResourceType
  if (ResourceType != NULL) {
    if (ResourceType->IsNumeric) {
      TypeCode = ResourceType->Id;
    } else {
      TypeCode = ANX_MAKE_TYPE(ResourceType->Name);
    }
  } else {
    TypeCode = 0;
  }

  // Extract resource ID/name from ResourceId
  if (ResourceId != NULL) {
    if (ResourceId->IsNumeric) {
      Id = (UINT16)ResourceId->Id;
      Name = NULL;
    } else {
      Id = 0;
      Name = ResourceId->Name;
    }
  } else {
    Id = 0;
    Name = NULL;
  }

  // Try hybrid strategy: native OS/2 resources first, then .axursrc
  // Note: LE/LX doesn't have sections like ELF/COFF, so .axursrc would need
  // to be in a resource object. For now, we only support native resources.
  Status = FindUniversalResourceFork(
    ImageBase,
    ResourceStrategyBoth,
    LeFindNativeResource,
    NULL,  // No section-based search for LE/LX
    NULL,
    &ResourceFork,
    &Size,
    &NeedsFree
  );

  if (FAILED(Status) || Status == S_FALSE) {
    return Status;
  }

  // Create resource object
  Status = CreateImageResource(ResourceFork, TypeCode, Id, Name, Resource);

  if (NeedsFree && ResourceFork != NULL) {
    free(ResourceFork);
  }

  return Status;
}

/**
  Get resource enumerator for LE/LX image.
**/
static
HRESULT
STDMETHODCALLTYPE
LeGetResourceEnumerator (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  IN  IMGLOAD_RESOURCE_ID *ResourceType,
  OUT IEnumImageResource  **Enumerator
  )
{
  VOID *ResourceFork;
  UINT64 Size;
  BOOLEAN NeedsFree;
  HRESULT Status;
  UINT32 TypeCode;

  if (ImageBase == NULL || Enumerator == NULL) {
    return E_POINTER;
  }

  *Enumerator = NULL;

  // Extract type code
  if (ResourceType != NULL) {
    if (ResourceType->IsNumeric) {
      TypeCode = ResourceType->Id;
    } else {
      TypeCode = ANX_MAKE_TYPE(ResourceType->Name);
    }
  } else {
    TypeCode = 0;  // All types
  }

  // Try to find universal resource fork (hybrid strategy)
  Status = FindUniversalResourceFork(
    ImageBase,
    ResourceStrategyBoth,
    LeFindNativeResource,
    NULL,
    NULL,
    &ResourceFork,
    &Size,
    &NeedsFree
  );

  if (FAILED(Status) || Status == S_FALSE) {
    return Status;
  }

  // Create enumerator
  Status = CreateImageResourceEnumerator(ResourceFork, TypeCode, Enumerator);

  if (NeedsFree && ResourceFork != NULL) {
    free(ResourceFork);
  }

  return Status;
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
  LeApplyRelocations,
  LeGetTargetSystem,
  LeGetMinimumSystemVersion,
  LeGetTargetSubsystem,
  LeGetMinimumSubsystemVersion,
  LeGetResource,
  LeGetResourceEnumerator,
  LeGetInitFini
};

//
// LE/LX Loader Instance
//

IImageLoader gLeLoader = {
  &gLeVtbl
};

APXH_REGISTER_IMGLOADER(gLeLoader);
