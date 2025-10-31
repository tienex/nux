/** @file
  APXH XCOFF Loader

  Provides XCOFF (Extended Common Object File Format) parsing and loading
  for AIX executables. XCOFF is IBM's extended version of COFF used on
  AIX operating systems for PowerPC and POWER architectures.

  Supports:
  - XCOFF32 (32-bit PowerPC)
  - XCOFF64 (64-bit PowerPC/POWER)
  - TOC (Table of Contents) for dynamic linking

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// XCOFF Magic Numbers
//

#define XCOFF32_MAGIC   0x01DF  ///< 32-bit XCOFF
#define XCOFF64_MAGIC   0x01F7  ///< 64-bit XCOFF

//
// XCOFF File Header Flags
//

#define F_RELFLG    0x0001  ///< Relocation info stripped
#define F_EXEC      0x0002  ///< File is executable
#define F_LNNO      0x0004  ///< Line numbers stripped
#define F_LSYMS     0x0008  ///< Local symbols stripped
#define F_FDPR_PROF 0x0010  ///< FDPR profiling
#define F_FDPR_OPTI 0x0020  ///< FDPR optimization
#define F_DSA       0x0040  ///< Large code model
#define F_DEP_SYS   0x0080  ///< System-dependent
#define F_SHROBJ    0x2000  ///< Shared object
#define F_LOADONLY  0x4000  ///< Loadable only
#define F_DYN       0x8000  ///< Dynamic object

//
// XCOFF Section Flags
//

#define STYP_TEXT     0x0020  ///< Text section
#define STYP_DATA     0x0040  ///< Data section
#define STYP_BSS      0x0080  ///< BSS section
#define STYP_EXCEPT   0x0100  ///< Exception section
#define STYP_INFO     0x0200  ///< Comment section
#define STYP_TDATA    0x0400  ///< TLS data
#define STYP_TBSS     0x0800  ///< TLS BSS
#define STYP_LOADER   0x1000  ///< Loader section
#define STYP_DEBUG    0x2000  ///< Debug section
#define STYP_TYPCHK   0x4000  ///< Type check section
#define STYP_OVRFLO   0x8000  ///< Overflow section

//
// XCOFF32 Structures
//

ANX_PACK_PUSH(1)

typedef struct _XCOFF32_FILEHDR {
  UINT16  Magic;              ///< Magic number
  UINT16  NumSections;        ///< Number of sections
  UINT32  TimeStamp;          ///< Time/date stamp
  UINT32  SymbolTablePtr;     ///< Symbol table file pointer
  UINT32  NumSymbols;         ///< Number of symbol table entries
  UINT16  OptHeaderSize;      ///< Optional header size
  UINT16  Flags;              ///< Flags
} XCOFF32_FILEHDR;

typedef struct _XCOFF32_AOUTHDR {
  UINT16  Magic;              ///< Magic number
  UINT16  VStamp;             ///< Version stamp
  UINT32  TextSize;           ///< Text size
  UINT32  DataSize;           ///< Initialized data size
  UINT32  BssSize;            ///< Uninitialized data size
  UINT32  Entry;              ///< Entry point
  UINT32  TextStart;          ///< Base of text
  UINT32  DataStart;          ///< Base of data
  UINT32  TocAddr;            ///< TOC anchor address
  UINT16  SnEntry;            ///< Section number for entry point
  UINT16  SnText;             ///< Section number for .text
  UINT16  SnData;             ///< Section number for .data
  UINT16  SnToc;              ///< Section number for TOC
  UINT16  SnLoader;           ///< Section number for loader
  UINT16  SnBss;              ///< Section number for .bss
  UINT16  AlignText;          ///< Max alignment for .text
  UINT16  AlignData;          ///< Max alignment for .data
  UINT16  ModType;            ///< Module type
  UINT8   CpuType;            ///< CPU type
  UINT8   CpuSubType;         ///< CPU subtype
  UINT32  MaxStack;           ///< Max stack size
  UINT32  MaxData;            ///< Max data size
  UINT32  Reserved[1];        ///< Reserved
} XCOFF32_AOUTHDR;

typedef struct _XCOFF32_SCNHDR {
  CHAR8   Name[8];            ///< Section name
  UINT32  PhysAddr;           ///< Physical address
  UINT32  VirtAddr;           ///< Virtual address
  UINT32  Size;               ///< Section size
  UINT32  DataPtr;            ///< File pointer to raw data
  UINT32  RelocPtr;           ///< File pointer to relocations
  UINT32  LinenoPtr;          ///< File pointer to line numbers
  UINT16  NumReloc;           ///< Number of relocation entries
  UINT16  NumLineno;          ///< Number of line number entries
  UINT32  Flags;              ///< Flags
} XCOFF32_SCNHDR;

//
// XCOFF64 Structures
//

typedef struct _XCOFF64_FILEHDR {
  UINT16  Magic;              ///< Magic number
  UINT16  NumSections;        ///< Number of sections
  UINT32  TimeStamp;          ///< Time/date stamp
  UINT64  SymbolTablePtr;     ///< Symbol table file pointer
  UINT32  OptHeaderSize;      ///< Optional header size
  UINT32  NumSymbols;         ///< Number of symbol table entries
  UINT16  Flags;              ///< Flags
  UINT16  Reserved;           ///< Reserved
} XCOFF64_FILEHDR;

typedef struct _XCOFF64_AOUTHDR {
  UINT16  Magic;              ///< Magic number
  UINT16  VStamp;             ///< Version stamp
  UINT32  Reserved1;          ///< Reserved
  UINT64  TextSize;           ///< Text size
  UINT64  DataSize;           ///< Initialized data size
  UINT64  BssSize;            ///< Uninitialized data size
  UINT64  Entry;              ///< Entry point
  UINT64  TextStart;          ///< Base of text
  UINT64  DataStart;          ///< Base of data
  UINT64  TocAddr;            ///< TOC anchor address
  UINT16  SnEntry;            ///< Section number for entry point
  UINT16  SnText;             ///< Section number for .text
  UINT16  SnData;             ///< Section number for .data
  UINT16  SnToc;              ///< Section number for TOC
  UINT16  SnLoader;           ///< Section number for loader
  UINT16  SnBss;              ///< Section number for .bss
  UINT16  AlignText;          ///< Max alignment for .text
  UINT16  AlignData;          ///< Max alignment for .data
  UINT16  ModType;            ///< Module type
  UINT8   CpuType;            ///< CPU type
  UINT8   CpuSubType;         ///< CPU subtype
  UINT8   TextPageSize;       ///< Text page size
  UINT8   DataPageSize;       ///< Data page size
  UINT8   StackPageSize;      ///< Stack page size
  UINT8   Flags2;             ///< Additional flags
  UINT16  Reserved2;          ///< Reserved
  UINT64  MaxStack;           ///< Max stack size
  UINT64  MaxData;            ///< Max data size
} XCOFF64_AOUTHDR;

typedef struct _XCOFF64_SCNHDR {
  CHAR8   Name[8];            ///< Section name
  UINT64  PhysAddr;           ///< Physical address
  UINT64  VirtAddr;           ///< Virtual address
  UINT64  Size;               ///< Section size
  UINT64  DataPtr;            ///< File pointer to raw data
  UINT64  RelocPtr;           ///< File pointer to relocations
  UINT64  LinenoPtr;          ///< File pointer to line numbers
  UINT32  NumReloc;           ///< Number of relocation entries
  UINT32  NumLineno;          ///< Number of line number entries
  UINT32  Flags;              ///< Flags
  UINT32  Reserved;           ///< Reserved
} XCOFF64_SCNHDR;

ANX_PACK_POP()

//
// Helper Macros
//

#define XCOFF_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// Internal Functions
//

/**
  Check if image is XCOFF format.
**/
static
BOOLEAN
ANXAPI
XcoffDetect (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  XCOFF32_FILEHDR *Header;

  if (ImageSize < sizeof(XCOFF32_FILEHDR)) {
    return FALSE;
  }

  Header = (XCOFF32_FILEHDR *)ImageBase;

  return (Header->Magic == XCOFF32_MAGIC ||
          Header->Magic == XCOFF64_MAGIC);
}

/**
  Get architecture from XCOFF image.
**/
static
ARCH
ANXAPI
XcoffGetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  // XCOFF is PowerPC/POWER only, which is not supported by APXH
  return ARCH_UNSUPPORTED;
}

/**
  Get entry point from XCOFF image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
XcoffGetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  XCOFF32_FILEHDR *FileHdr32 = (XCOFF32_FILEHDR *)ImageBase;
  XCOFF32_AOUTHDR *AoutHdr32;
  XCOFF64_FILEHDR *FileHdr64;
  XCOFF64_AOUTHDR *AoutHdr64;

  if (FileHdr32->Magic == XCOFF64_MAGIC) {
    // 64-bit XCOFF
    FileHdr64 = (XCOFF64_FILEHDR *)ImageBase;
    AoutHdr64 = (XCOFF64_AOUTHDR *)(FileHdr64 + 1);
    return AoutHdr64->Entry;
  } else {
    // 32-bit XCOFF
    AoutHdr32 = (XCOFF32_AOUTHDR *)(FileHdr32 + 1);
    return AoutHdr32->Entry;
  }
}

/**
  Load XCOFF section (32-bit).
**/
static
VOID
XcoffLoadSection32 (
  IN VOID             *ImageBase,
  IN XCOFF32_SCNHDR   *Section,
  IN BOOLEAN          IsUserMode
  )
{
  BOOLEAN IsWritable = !!(Section->Flags & (STYP_DATA | STYP_BSS | STYP_TDATA | STYP_TBSS));
  BOOLEAN IsExecutable = !!(Section->Flags & STYP_TEXT);

  if (Section->Size == 0) {
    return;
  }

  info("  Section %.8s at 0x%08x (size: 0x%08x, flags: 0x%08x)",
       Section->Name, Section->VirtAddr, Section->Size, Section->Flags);

  if (Section->Flags & (STYP_BSS | STYP_TBSS)) {
    // Zero-filled BSS section
    VirtualAddressMemset(
      Section->VirtAddr,
      0,
      Section->Size,
      IsUserMode,
      TRUE,  // Writable
      FALSE  // Not executable
    );
  } else if (!(Section->Flags & (STYP_INFO | STYP_DEBUG | STYP_LOADER))) {
    // Data/Text section (skip info/debug/loader sections)
    VirtualAddressCopy(
      Section->VirtAddr,
      XCOFF_OFF(Section->DataPtr),
      Section->Size,
      IsUserMode,
      IsWritable,
      IsExecutable
    );
  }
}

/**
  Load XCOFF section (64-bit).
**/
static
VOID
XcoffLoadSection64 (
  IN VOID             *ImageBase,
  IN XCOFF64_SCNHDR   *Section,
  IN BOOLEAN          IsUserMode
  )
{
  BOOLEAN IsWritable = !!(Section->Flags & (STYP_DATA | STYP_BSS | STYP_TDATA | STYP_TBSS));
  BOOLEAN IsExecutable = !!(Section->Flags & STYP_TEXT);

  if (Section->Size == 0) {
    return;
  }

  info("  Section %.8s at 0x%016llx (size: 0x%016llx, flags: 0x%08x)",
       Section->Name, Section->VirtAddr, Section->Size, Section->Flags);

  if (Section->Flags & (STYP_BSS | STYP_TBSS)) {
    // Zero-filled BSS section
    VirtualAddressMemset(
      Section->VirtAddr,
      0,
      Section->Size,
      IsUserMode,
      TRUE,  // Writable
      FALSE  // Not executable
    );
  } else if (!(Section->Flags & (STYP_INFO | STYP_DEBUG | STYP_LOADER))) {
    // Data/Text section (skip info/debug/loader sections)
    VirtualAddressCopy(
      Section->VirtAddr,
      XCOFF_OFF(Section->DataPtr),
      Section->Size,
      IsUserMode,
      IsWritable,
      IsExecutable
    );
  }
}

/**
  Load XCOFF image.
**/
static
IMGLOAD_STATUS
ANXAPI
XcoffLoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase = Context->ImageBase;
  XCOFF32_FILEHDR *FileHdr32 = (XCOFF32_FILEHDR *)ImageBase;
  UINT32 i;

  if (FileHdr32->Magic == XCOFF64_MAGIC) {
    // 64-bit XCOFF
    XCOFF64_FILEHDR *FileHdr64 = (XCOFF64_FILEHDR *)ImageBase;
    XCOFF64_SCNHDR *Sections;

    info("Loading XCOFF 64-bit PowerPC executable...");

    Sections = (XCOFF64_SCNHDR *)XCOFF_OFF(
      sizeof(XCOFF64_FILEHDR) + FileHdr64->OptHeaderSize
    );

    for (i = 0; i < FileHdr64->NumSections; i++) {
      XcoffLoadSection64(ImageBase, &Sections[i], Context->IsUserMode);
    }
  } else {
    // 32-bit XCOFF
    XCOFF32_SCNHDR *Sections;

    info("Loading XCOFF 32-bit PowerPC executable...");

    Sections = (XCOFF32_SCNHDR *)XCOFF_OFF(
      sizeof(XCOFF32_FILEHDR) + FileHdr32->OptHeaderSize
    );

    for (i = 0; i < FileHdr32->NumSections; i++) {
      XcoffLoadSection32(ImageBase, &Sections[i], Context->IsUserMode);
    }
  }

  Context->EntryPoint = XcoffGetEntryPoint(This, ImageBase);
  return ImgLoadSuccess;
}

/**
  Extract TLS information from XCOFF image.
**/
static
IMGLOAD_STATUS
ANXAPI
XcoffGetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  XCOFF32_FILEHDR *FileHdr32 = (XCOFF32_FILEHDR *)ImageBase;
  UINT32 i;

  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));

  if (FileHdr32->Magic == XCOFF64_MAGIC) {
    // 64-bit XCOFF
    XCOFF64_FILEHDR *FileHdr64 = (XCOFF64_FILEHDR *)ImageBase;
    XCOFF64_SCNHDR *Sections = (XCOFF64_SCNHDR *)XCOFF_OFF(
      sizeof(XCOFF64_FILEHDR) + FileHdr64->OptHeaderSize
    );

    for (i = 0; i < FileHdr64->NumSections; i++) {
      if (Sections[i].Flags & STYP_TDATA) {
        TlsInfo->InitDataAddr = Sections[i].VirtAddr;
        TlsInfo->InitDataSize = Sections[i].Size;
        TlsInfo->Alignment = 16;  // Default alignment
      }
      if (Sections[i].Flags & STYP_TBSS) {
        TlsInfo->TotalSize = TlsInfo->InitDataSize + Sections[i].Size;
      }
    }
  } else {
    // 32-bit XCOFF
    XCOFF32_SCNHDR *Sections = (XCOFF32_SCNHDR *)XCOFF_OFF(
      sizeof(XCOFF32_FILEHDR) + FileHdr32->OptHeaderSize
    );

    for (i = 0; i < FileHdr32->NumSections; i++) {
      if (Sections[i].Flags & STYP_TDATA) {
        TlsInfo->InitDataAddr = Sections[i].VirtAddr;
        TlsInfo->InitDataSize = Sections[i].Size;
        TlsInfo->Alignment = 16;  // Default alignment
      }
      if (Sections[i].Flags & STYP_TBSS) {
        TlsInfo->TotalSize = TlsInfo->InitDataSize + Sections[i].Size;
      }
    }
  }

  return ImgLoadSuccess;
}

//
// XCOFF Loader VTable
//

static CONST IMAGE_LOADER_VTBL gXcoffVtbl = {
  XcoffDetect,
  XcoffGetArch,
  XcoffGetEntryPoint,
  XcoffLoadImage,
  XcoffGetTlsInfo
};

//
// XCOFF Loader Instance
//

IMAGE_LOADER gXcoffLoader = {
  &gXcoffVtbl,
  "XCOFF",
  NULL
};
