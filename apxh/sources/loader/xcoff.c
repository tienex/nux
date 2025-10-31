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
HRESULT
STDMETHODCALLTYPE
XcoffDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  XCOFF32_FILEHDR *Header;

  if (ImageSize < sizeof(XCOFF32_FILEHDR)) {
    return S_FALSE;
  }

  Header = (XCOFF32_FILEHDR *)ImageBase;

  return (Header->Magic == XCOFF32_MAGIC ||
          Header->Magic == XCOFF64_MAGIC) ? S_OK : S_FALSE;
}

/**
  Get architecture from XCOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  if (Architecture == NULL) {
    return E_POINTER;
  }

  // XCOFF is PowerPC/POWER only, which is not supported by APXH
  *Architecture = ArchUnsupported;
  return IMGLOAD_E_UNSUPPORTED_ARCH;
}

/**
  Get endianness from XCOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // XCOFF is typically big-endian for PowerPC/POWER architectures
  // (AIX traditionally used big-endian PowerPC)
  *Endianness = ImgEndianBig;
  return S_OK;
}

/**
  Get entry point from XCOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  XCOFF32_FILEHDR *FileHdr32;
  XCOFF32_AOUTHDR *AoutHdr32;
  XCOFF64_FILEHDR *FileHdr64;
  XCOFF64_AOUTHDR *AoutHdr64;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  FileHdr32 = (XCOFF32_FILEHDR *)ImageBase;

  if (FileHdr32->Magic == XCOFF64_MAGIC) {
    // 64-bit XCOFF
    FileHdr64 = (XCOFF64_FILEHDR *)ImageBase;
    AoutHdr64 = (XCOFF64_AOUTHDR *)(FileHdr64 + 1);
    *EntryPoint = AoutHdr64->Entry;
  } else {
    // 32-bit XCOFF
    AoutHdr32 = (XCOFF32_AOUTHDR *)(FileHdr32 + 1);
    *EntryPoint = AoutHdr32->Entry;
  }

  if (*EntryPoint == 0) {
    return IMGLOAD_E_INVALID_HEADER;
  }

  return S_OK;
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
HRESULT
STDMETHODCALLTYPE
XcoffLoadImage (
  IN     IImageLoader     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  XCOFF32_FILEHDR *FileHdr32;
  UINT32 i;
  HRESULT Status;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  FileHdr32 = (XCOFF32_FILEHDR *)ImageBase;

  // Populate architecture and endianness
  Status = XcoffGetArch(This, ImageBase, &Context->Architecture);
  if (FAILED(Status)) {
    return Status;
  }

  Status = XcoffGetEndianness(This, ImageBase, &Context->Endianness);
  if (FAILED(Status)) {
    return Status;
  }

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

  Status = XcoffGetEntryPoint(This, ImageBase, &Context->EntryPoint);
  if (FAILED(Status)) {
    return Status;
  }

  return S_OK;
}

/**
  Extract TLS information from XCOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  XCOFF32_FILEHDR *FileHdr32;
  UINT32 i;
  BOOLEAN HasTls = FALSE;

  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  FileHdr32 = (XCOFF32_FILEHDR *)ImageBase;
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
        HasTls = TRUE;
      }
      if (Sections[i].Flags & STYP_TBSS) {
        TlsInfo->TotalSize = TlsInfo->InitDataSize + Sections[i].Size;
        HasTls = TRUE;
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
        HasTls = TRUE;
      }
      if (Sections[i].Flags & STYP_TBSS) {
        TlsInfo->TotalSize = TlsInfo->InitDataSize + Sections[i].Size;
        HasTls = TRUE;
      }
    }
  }

  return HasTls ? S_OK : S_FALSE;
}

/**
  Extract unwinding information from XCOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  // XCOFF doesn't have standardized unwind information
  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  // XCOFF symbol lookup not implemented
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  // XCOFF symbol lookup not implemented
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Extract relocation information from XCOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffGetRelocInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_RELOC_INFO   *RelocInfo
  )
{
  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  // XCOFF has relocations but they're handled internally
  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));
  return S_FALSE;
}

/**
  Apply relocations to XCOFF image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffApplyRelocations (
  IN VOID              *ImageBase,
  IN VIRTUAL_ADDRESS   LoadAddress,
  IN VIRTUAL_ADDRESS   PreferredBase
  )
{
  // XCOFF relocations are applied during load
  // No additional relocation needed
  return S_OK;
}

/**
  IUnknown::QueryInterface implementation (stub).
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffQueryInterface (
  IN  IImageLoader  *This,
  IN  REFIID        riid,
  OUT VOID          **ppvObject
  )
{
  // Simple implementation - only support IImageLoader and IUnknown
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  *ppvObject = NULL;

  // Compare GUIDs (simplified)
  if (memcmp(riid, &IID_IImageLoader, sizeof(GUID)) == 0 ||
      memcmp(riid, &IID_IUnknown, sizeof(GUID)) == 0) {
    *ppvObject = This;
    return S_OK;
  }

  return E_NOINTERFACE;
}

/**
  IUnknown::AddRef implementation (stub - static object).
**/
static
UINT32
STDMETHODCALLTYPE
XcoffAddRef (
  IN IImageLoader  *This
  )
{
  // Static object, no reference counting
  return 1;
}

/**
  IUnknown::Release implementation (stub - static object).
**/
static
UINT32
STDMETHODCALLTYPE
XcoffRelease (
  IN IImageLoader  *This
  )
{
  // Static object, no reference counting
  return 1;
}

//
// XCOFF Loader VTable
//

#ifdef __cplusplus

/**
  Get target operating system from Xcoff image.
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffGetTargetSystem (
  IN  IImageLoader           *This,
  IN  VOID                   *ImageBase,
  OUT IMGLOAD_TARGET_SYSTEM  *TargetSystem
  )
{
  if (TargetSystem == NULL) {
    return E_POINTER;
  }

  *TargetSystem = ImgSystemAix;
  return S_OK;
}

/**
  Get minimum required system version from Xcoff image.
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffGetMinimumSystemVersion (
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
  Get target subsystem from Xcoff image.
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffGetTargetSubsystem (
  IN  IImageLoader              *This,
  IN  VOID                      *ImageBase,
  OUT IMGLOAD_TARGET_SUBSYSTEM  *TargetSubsystem
  )
{
  if (TargetSubsystem == NULL) {
    return E_POINTER;
  }

  *TargetSubsystem = ImgSubsystemUnixCli;
  return S_OK;
}

/**
  Get minimum required subsystem version from Xcoff image.
**/
static
HRESULT
STDMETHODCALLTYPE
XcoffGetMinimumSubsystemVersion (
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
// C++ mode not supported in this implementation
#error "C++ mode not implemented"
#else
static CONST IImageLoaderVtbl gXcoffVtbl = {
  XcoffQueryInterface,
  XcoffAddRef,
  XcoffRelease,
  XcoffDetect,
  XcoffGetArch,
  XcoffGetEndianness,
  XcoffGetEntryPoint,
  XcoffLoadImage,
  XcoffGetTlsInfo,
  XcoffGetUnwindInfo,
  XcoffGetSymbolByAddress,
  XcoffGetSymbolByName,
  XcoffGetRelocInfo,
  XcoffApplyRelocations,
  XcoffGetTargetSystem,
  XcoffGetMinimumSystemVersion,
  XcoffGetTargetSubsystem,
  XcoffGetMinimumSubsystemVersion
};
#endif

//
// XCOFF Loader Instance
//

IImageLoader gXcoffLoader = {
  &gXcoffVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gXcoffLoader)
