/** @file
  APXH ECOFF Loader

  Provides ECOFF (Extended Common Object File Format) parsing and loading
  for MIPS and Alpha executables. ECOFF was used by DEC Ultrix, Tru64
  (Digital Unix, OSF/1), SGI Irix, early Linux/MIPS, and DEC Alpha systems.

  Supports:
  - ECOFF32 (MIPS-I, MIPS-II)
  - ECOFF64 (MIPS-III, MIPS-IV, DEC Alpha)
  - Little-endian and big-endian byte order

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>
#include <ananke/resource.h>
#include "imgresource.h"

//
// ECOFF Magic Numbers
//

#define ECOFF_MAGIC_MIPSEL      0x0162  ///< MIPS little-endian
#define ECOFF_MAGIC_MIPSEB      0x0160  ///< MIPS big-endian
#define ECOFF_MAGIC_MIPS64EL    0x0166  ///< MIPS 64-bit little-endian
#define ECOFF_MAGIC_MIPS64EB    0x0164  ///< MIPS 64-bit big-endian
#define ECOFF_MAGIC_ALPHA       0x0183  ///< DEC Alpha

//
// ECOFF File Header Flags
//

#define F_RELFLG    0x0001  ///< Relocation info stripped
#define F_EXEC      0x0002  ///< File is executable
#define F_LNNO      0x0004  ///< Line numbers stripped
#define F_LSYMS     0x0008  ///< Local symbols stripped
#define F_AR32WR    0x0010  ///< 32-bit little endian
#define F_AR32W     0x0020  ///< 32-bit big endian

//
// ECOFF Section Types
//

#define STYP_TEXT   0x0020  ///< Text section
#define STYP_DATA   0x0040  ///< Data section
#define STYP_BSS    0x0080  ///< BSS section
#define STYP_RDATA  0x0100  ///< Read-only data
#define STYP_SDATA  0x0200  ///< Small data
#define STYP_SBSS   0x0400  ///< Small BSS

//
// ECOFF Structures (32-bit)
//

ANX_PACK_PUSH(1)

typedef struct _ECOFF32_FILEHDR {
  UINT16  Magic;              ///< Magic number
  UINT16  NumSections;        ///< Number of sections
  UINT32  TimeStamp;          ///< Time/date stamp
  UINT32  SymbolTablePtr;     ///< Symbol table file pointer
  UINT32  NumSymbols;         ///< Number of symbol table entries
  UINT16  OptHeaderSize;      ///< Optional header size
  UINT16  Flags;              ///< Flags
} ECOFF32_FILEHDR;

typedef struct _ECOFF32_AOUTHDR {
  UINT16  Magic;              ///< Magic number (0x0107 for OMAGIC)
  UINT16  VStamp;             ///< Version stamp
  UINT32  TextSize;           ///< Text size in bytes
  UINT32  DataSize;           ///< Initialized data size
  UINT32  BssSize;            ///< Uninitialized data size
  UINT32  Entry;              ///< Entry point
  UINT32  TextStart;          ///< Base of text
  UINT32  DataStart;          ///< Base of data
  UINT32  BssStart;           ///< Base of BSS
  UINT32  GpValue;            ///< GP register value
  UINT32  GpMask;             ///< GP mask
  UINT32  CopMask;            ///< Coprocessor masks
} ECOFF32_AOUTHDR;

typedef struct _ECOFF32_SCNHDR {
  CHAR8   Name[8];            ///< Section name
  UINT32  PhysAddr;           ///< Physical address
  UINT32  VirtAddr;           ///< Virtual address
  UINT32  Size;               ///< Section size
  UINT32  DataPtr;            ///< File pointer to raw data
  UINT32  RelocPtr;           ///< File pointer to relocations
  UINT32  LinenoPtr;          ///< File pointer to line numbers
  UINT16  NumReloc;           ///< Number of relocation entries
  UINT16  NumLineno;          ///< Number of line number entries
  UINT32  Flags;              ///< Flags (section type)
} ECOFF32_SCNHDR;

//
// ECOFF Structures (64-bit)
//

typedef struct _ECOFF64_FILEHDR {
  UINT16  Magic;              ///< Magic number
  UINT16  NumSections;        ///< Number of sections
  UINT32  TimeStamp;          ///< Time/date stamp
  UINT64  SymbolTablePtr;     ///< Symbol table file pointer
  UINT32  NumSymbols;         ///< Number of symbol table entries
  UINT16  OptHeaderSize;      ///< Optional header size
  UINT16  Flags;              ///< Flags
} ECOFF64_FILEHDR;

typedef struct _ECOFF64_AOUTHDR {
  UINT16  Magic;              ///< Magic number
  UINT16  VStamp;             ///< Version stamp
  UINT64  TextSize;           ///< Text size in bytes
  UINT64  DataSize;           ///< Initialized data size
  UINT64  BssSize;            ///< Uninitialized data size
  UINT64  Entry;              ///< Entry point
  UINT64  TextStart;          ///< Base of text
  UINT64  DataStart;          ///< Base of data
  UINT64  BssStart;           ///< Base of BSS
  UINT64  GpValue;            ///< GP register value
  UINT32  GpMask;             ///< GP mask
  UINT32  CopMask;            ///< Coprocessor masks
} ECOFF64_AOUTHDR;

typedef struct _ECOFF64_SCNHDR {
  CHAR8   Name[8];            ///< Section name
  UINT64  PhysAddr;           ///< Physical address
  UINT64  VirtAddr;           ///< Virtual address
  UINT64  Size;               ///< Section size
  UINT64  DataPtr;            ///< File pointer to raw data
  UINT64  RelocPtr;           ///< File pointer to relocations
  UINT64  LinenoPtr;          ///< File pointer to line numbers
  UINT32  NumReloc;           ///< Number of relocation entries
  UINT32  NumLineno;          ///< Number of line number entries
  UINT32  Flags;              ///< Flags (section type)
} ECOFF64_SCNHDR;

ANX_PACK_POP()

//
// Helper Macros
//

#define ECOFF_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// Internal Functions
//

/**
  Check if image is ECOFF format.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  ECOFF32_FILEHDR *Header;

  if (ImageSize < sizeof(ECOFF32_FILEHDR)) {
    return S_FALSE;
  }

  Header = (ECOFF32_FILEHDR *)ImageBase;

  if (Header->Magic == ECOFF_MAGIC_MIPSEL ||
      Header->Magic == ECOFF_MAGIC_MIPSEB ||
      Header->Magic == ECOFF_MAGIC_MIPS64EL ||
      Header->Magic == ECOFF_MAGIC_MIPS64EB ||
      Header->Magic == ECOFF_MAGIC_ALPHA) {
    return S_OK;
  }

  return S_FALSE;
}

/**
  Get architecture from ECOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  ECOFF32_FILEHDR *Header;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  Header = (ECOFF32_FILEHDR *)ImageBase;

  switch (Header->Magic) {
    case ECOFF_MAGIC_MIPSEL:
    case ECOFF_MAGIC_MIPSEB:
      *Architecture = ArchMips32;
      break;

    case ECOFF_MAGIC_MIPS64EL:
    case ECOFF_MAGIC_MIPS64EB:
      *Architecture = ArchMips64;
      break;

    case ECOFF_MAGIC_ALPHA:
      *Architecture = ArchAlpha;
      break;

    default:
      *Architecture = ArchUnsupported;
      return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  return S_OK;
}

/**
  Get endianness from ECOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  ECOFF32_FILEHDR *Header;

  if (Endianness == NULL) {
    return E_POINTER;
  }

  Header = (ECOFF32_FILEHDR *)ImageBase;

  switch (Header->Magic) {
    case ECOFF_MAGIC_MIPSEB:
    case ECOFF_MAGIC_MIPS64EB:
      *Endianness = ImgEndianBig;
      break;

    case ECOFF_MAGIC_MIPSEL:
    case ECOFF_MAGIC_MIPS64EL:
    case ECOFF_MAGIC_ALPHA:
      *Endianness = ImgEndianLittle;
      break;

    default:
      *Endianness = ImgEndianUnknown;
      return IMGLOAD_E_INVALID_FORMAT;
  }

  return S_OK;
}

/**
  Get entry point from ECOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  ECOFF32_FILEHDR *FileHdr32;
  ECOFF32_AOUTHDR *AoutHdr32;
  ECOFF64_FILEHDR *FileHdr64;
  ECOFF64_AOUTHDR *AoutHdr64;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  FileHdr32 = (ECOFF32_FILEHDR *)ImageBase;

  if (FileHdr32->OptHeaderSize == 0) {
    *EntryPoint = 0;
    return IMGLOAD_E_INVALID_HEADER;
  }

  if (FileHdr32->Magic == ECOFF_MAGIC_MIPS64EL ||
      FileHdr32->Magic == ECOFF_MAGIC_MIPS64EB) {
    // 64-bit ECOFF
    FileHdr64 = (ECOFF64_FILEHDR *)ImageBase;
    AoutHdr64 = (ECOFF64_AOUTHDR *)(FileHdr64 + 1);
    *EntryPoint = AoutHdr64->Entry;
  } else {
    // 32-bit ECOFF
    AoutHdr32 = (ECOFF32_AOUTHDR *)(FileHdr32 + 1);
    *EntryPoint = AoutHdr32->Entry;
  }

  return S_OK;
}

/**
  Load ECOFF section.
**/
static
VOID
EcoffLoadSection32 (
  IN VOID             *ImageBase,
  IN ECOFF32_SCNHDR   *Section,
  IN BOOLEAN          IsUserMode
  )
{
  BOOLEAN IsWritable = !!(Section->Flags & (STYP_DATA | STYP_BSS | STYP_SDATA | STYP_SBSS));
  BOOLEAN IsExecutable = !!(Section->Flags & STYP_TEXT);

  if (Section->Size == 0) {
    return;
  }

  info("  Section %.8s at 0x%08x (size: 0x%08x, flags: 0x%08x)",
       Section->Name, Section->VirtAddr, Section->Size, Section->Flags);

  if (Section->Flags & (STYP_BSS | STYP_SBSS)) {
    // Zero-filled BSS section
    VasFill(
      Section->VirtAddr,
      0,
      Section->Size,
      IsUserMode,
      TRUE,  // Writable
      FALSE  // Not executable
    );
  } else {
    // Data section
    VasCopy(
      Section->VirtAddr,
      ECOFF_OFF(Section->DataPtr),
      Section->Size,
      IsUserMode,
      IsWritable,
      IsExecutable
    );
  }
}

/**
  Load ECOFF section (64-bit).
**/
static
VOID
EcoffLoadSection64 (
  IN VOID             *ImageBase,
  IN ECOFF64_SCNHDR   *Section,
  IN BOOLEAN          IsUserMode
  )
{
  BOOLEAN IsWritable = !!(Section->Flags & (STYP_DATA | STYP_BSS | STYP_SDATA | STYP_SBSS));
  BOOLEAN IsExecutable = !!(Section->Flags & STYP_TEXT);

  if (Section->Size == 0) {
    return;
  }

  info("  Section %.8s at 0x%016llx (size: 0x%016llx, flags: 0x%08x)",
       Section->Name, Section->VirtAddr, Section->Size, Section->Flags);

  if (Section->Flags & (STYP_BSS | STYP_SBSS)) {
    // Zero-filled BSS section
    VasFill(
      Section->VirtAddr,
      0,
      Section->Size,
      IsUserMode,
      TRUE,  // Writable
      FALSE  // Not executable
    );
  } else {
    // Data section
    VasCopy(
      Section->VirtAddr,
      ECOFF_OFF(Section->DataPtr),
      Section->Size,
      IsUserMode,
      IsWritable,
      IsExecutable
    );
  }
}

/**
  Load ECOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffLoadImage (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  ECOFF32_FILEHDR *FileHdr32;
  UINT32 i;
  HRESULT Status;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  FileHdr32 = (ECOFF32_FILEHDR *)ImageBase;

  // Populate context
  Status = EcoffGetArch(NULL, ImageBase, &Context->Architecture);
  if (FAILED(Status)) {
    return Status;
  }

  Status = EcoffGetEndianness(NULL, ImageBase, &Context->Endianness);
  if (FAILED(Status)) {
    return Status;
  }

  Status = EcoffGetEntryPoint(NULL, ImageBase, &Context->EntryPoint);
  if (FAILED(Status)) {
    return Status;
  }

  if (FileHdr32->Magic == ECOFF_MAGIC_MIPS64EL ||
      FileHdr32->Magic == ECOFF_MAGIC_MIPS64EB) {
    // 64-bit ECOFF
    ECOFF64_FILEHDR *FileHdr64 = (ECOFF64_FILEHDR *)ImageBase;
    ECOFF64_SCNHDR *Sections;

    info("Loading ECOFF 64-bit MIPS executable...");

    Sections = (ECOFF64_SCNHDR *)ECOFF_OFF(
      sizeof(ECOFF64_FILEHDR) + FileHdr64->OptHeaderSize
    );

    for (i = 0; i < FileHdr64->NumSections; i++) {
      EcoffLoadSection64(ImageBase, &Sections[i], Context->IsUserMode);
    }
  } else {
    // 32-bit ECOFF
    ECOFF32_SCNHDR *Sections;

    info("Loading ECOFF 32-bit MIPS/Alpha executable...");

    Sections = (ECOFF32_SCNHDR *)ECOFF_OFF(
      sizeof(ECOFF32_FILEHDR) + FileHdr32->OptHeaderSize
    );

    for (i = 0; i < FileHdr32->NumSections; i++) {
      EcoffLoadSection32(ImageBase, &Sections[i], Context->IsUserMode);
    }
  }

  return S_OK;
}

/**
  Extract TLS information from ECOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // ECOFF doesn't have explicit TLS support
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from ECOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  // ECOFF doesn't have standard unwinding information
  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  // Symbol table parsing would go here
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  // Symbol table parsing would go here
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Extract relocation information from ECOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetRelocInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_RELOC_INFO   *RelocInfo
  )
{
  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  // ECOFF typically doesn't have base relocations for static executables
  RelocInfo->RequiresReloc = FALSE;
  return S_FALSE;
}

/**
  Apply relocations to ECOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffApplyRelocations (
  IN VOID              *ImageBase,
  IN VIRTUAL_ADDRESS   LoadAddress,
  IN VIRTUAL_ADDRESS   PreferredBase
  )
{
  // ECOFF doesn't support relocation for static executables
  return S_OK;
}

/**
  IUnknown::QueryInterface implementation.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffQueryInterface (
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
EcoffAddRef (
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
EcoffRelease (
  IN IImageLoader  *This
  )
{
  return 1;
}

//

/**
  Get target operating system from Ecoff image.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetTargetSystem (
  IN  IImageLoader           *This,
  IN  VOID                   *ImageBase,
  OUT IMGLOAD_TARGET_SYSTEM  *TargetSystem
  )
{
  if (TargetSystem == NULL) {
    return E_POINTER;
  }

  *TargetSystem = ImgSystemUnix;
  return S_OK;
}

/**
  Get minimum required system version from Ecoff image.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetMinimumSystemVersion (
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
  Get target subsystem from Ecoff image.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetTargetSubsystem (
  IN  IImageLoader              *This,
  IN  VOID                      *ImageBase,
  OUT IMGLOAD_TARGET_SUBSYSTEM  *TargetSubsystem
  )
{
  if (TargetSubsystem == NULL) {
    return E_POINTER;
  }

  *TargetSubsystem = ImgSubsystemCli;
  return S_OK;
}

/**
  Get minimum required subsystem version from Ecoff image.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetMinimumSubsystemVersion (
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
  Find section by name in ECOFF image.

  @param[in]  ImageBase    Pointer to ECOFF image.
  @param[in]  SectionName  Section name to find.
  @param[out] Data         Receives pointer to section data.
  @param[out] Size         Receives size of section.

  @return S_OK if found, S_FALSE if not found, error code otherwise.
**/
static
HRESULT
EcoffFindSection (
  IN  VOID         *ImageBase,
  IN  CONST CHAR8  *SectionName,
  OUT VOID         **Data,
  OUT UINT64       *Size
  )
{
  ECOFF_FILE_HEADER *FileHeader;
  ECOFF_SECTION_HEADER *Sections;
  UINT16 NumSections;
  UINT16 i;
  UINTN NameLen;

  if (ImageBase == NULL || SectionName == NULL || Data == NULL || Size == NULL) {
    return E_POINTER;
  }

  FileHeader = (ECOFF_FILE_HEADER *)ImageBase;
  NumSections = FileHeader->NumberOfSections;
  Sections = (ECOFF_SECTION_HEADER *)((UINT8 *)ImageBase +
                                      sizeof(ECOFF_FILE_HEADER) +
                                      FileHeader->SizeOfOptionalHeader);

  NameLen = strlen(SectionName);
  if (NameLen > 8) {
    NameLen = 8;  // ECOFF section names are max 8 characters
  }

  // Search for section
  for (i = 0; i < NumSections; i++) {
    if (memcmp(Sections[i].Name, SectionName, NameLen) == 0) {
      *Data = (UINT8 *)ImageBase + Sections[i].FilePointer;
      *Size = Sections[i].Size;
      return S_OK;
    }
  }

  return S_FALSE;  // Section not found
}

/**
  Get ECOFF init/fini function arrays.

  ECOFF (DEC Ultrix/SGI Irix) uses special sections for initialization and termination:
  - .init: Initialization code section (executed before main)
  - .fini: Finalization code section (executed after main)
  - .ctors: Array of constructor function pointers (C++)
  - .dtors: Array of destructor function pointers (C++)

  @param[in]  ImageBase       Pointer to ECOFF image.
  @param[out] InitSection     Receives pointer to .init section (NULL if none).
  @param[out] InitSize        Receives size of .init section.
  @param[out] FiniSection     Receives pointer to .fini section (NULL if none).
  @param[out] FiniSize        Receives size of .fini section.
  @param[out] CtorsArray      Receives pointer to .ctors array (NULL if none).
  @param[out] NumCtors        Receives number of constructors.
  @param[out] DtorsArray      Receives pointer to .dtors array (NULL if none).
  @param[out] NumDtors        Receives number of destructors.

  @return S_OK if any init/fini found, S_FALSE if none found.
**/
static
HRESULT
EcoffGetInitFini (
  IN  VOID     *ImageBase,
  OUT VOID     **InitSection,
  OUT UINT64   *InitSize,
  OUT VOID     **FiniSection,
  OUT UINT64   *FiniSize,
  OUT VOID     **CtorsArray,
  OUT UINT32   *NumCtors,
  OUT VOID     **DtorsArray,
  OUT UINT32   *NumDtors
  )
{
  VOID     *InitData;
  VOID     *FiniData;
  VOID     *CtorsData;
  VOID     *DtorsData;
  UINT64   InitSz;
  UINT64   FiniSz;
  UINT64   CtorsSz;
  UINT64   DtorsSz;
  HRESULT  Status;
  BOOLEAN  FoundAny;

  if (ImageBase == NULL) {
    return E_POINTER;
  }

  // Initialize outputs
  if (InitSection != NULL) *InitSection = NULL;
  if (InitSize != NULL) *InitSize = 0;
  if (FiniSection != NULL) *FiniSection = NULL;
  if (FiniSize != NULL) *FiniSize = 0;
  if (CtorsArray != NULL) *CtorsArray = NULL;
  if (NumCtors != NULL) *NumCtors = 0;
  if (DtorsArray != NULL) *DtorsArray = NULL;
  if (NumDtors != NULL) *NumDtors = 0;

  FoundAny = FALSE;

  // Find .init section
  Status = EcoffFindSection(ImageBase, ".init", &InitData, &InitSz);
  if (Status == S_OK) {
    if (InitSection != NULL) *InitSection = InitData;
    if (InitSize != NULL) *InitSize = InitSz;
    FoundAny = TRUE;
  }

  // Find .fini section
  Status = EcoffFindSection(ImageBase, ".fini", &FiniData, &FiniSz);
  if (Status == S_OK) {
    if (FiniSection != NULL) *FiniSection = FiniData;
    if (FiniSize != NULL) *FiniSize = FiniSz;
    FoundAny = TRUE;
  }

  // Find .ctors section (array of function pointers)
  Status = EcoffFindSection(ImageBase, ".ctors", &CtorsData, &CtorsSz);
  if (Status == S_OK) {
    if (CtorsArray != NULL) *CtorsArray = CtorsData;
    if (NumCtors != NULL) *NumCtors = (UINT32)(CtorsSz / sizeof(UINT32));  // Assume 32-bit pointers
    FoundAny = TRUE;
  }

  // Find .dtors section (array of function pointers)
  Status = EcoffFindSection(ImageBase, ".dtors", &DtorsData, &DtorsSz);
  if (Status == S_OK) {
    if (DtorsArray != NULL) *DtorsArray = DtorsData;
    if (NumDtors != NULL) *NumDtors = (UINT32)(DtorsSz / sizeof(UINT32));  // Assume 32-bit pointers
    FoundAny = TRUE;
  }

  return FoundAny ? S_OK : S_FALSE;
}

/**
  Get resource from ECOFF image.

  ECOFF has no native resource system, so we only check .axursrc section.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetResource (
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

  // Find universal resource fork in .axursrc section
  Status = FindUniversalResourceFork(
    ImageBase,
    ResourceStrategyDirect,
    NULL,  // No native resources in ECOFF
    EcoffFindSection,
    ".axursrc",
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
  Get resource enumerator for ECOFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
EcoffGetResourceEnumerator (
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

  // Find universal resource fork
  Status = FindUniversalResourceFork(
    ImageBase,
    ResourceStrategyDirect,
    NULL,
    EcoffFindSection,
    ".axursrc",
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

// ECOFF Loader VTable
//

static CONST IImageLoaderVtbl gEcoffVtbl = {
  EcoffQueryInterface,
  EcoffAddRef,
  EcoffRelease,
  EcoffDetect,
  EcoffGetArch,
  EcoffGetEndianness,
  EcoffGetEntryPoint,
  EcoffLoadImage,
  EcoffGetTlsInfo,
  EcoffGetUnwindInfo,
  EcoffGetSymbolByAddress,
  EcoffGetSymbolByName,
  EcoffGetRelocInfo,
  EcoffApplyRelocations,
  EcoffGetTargetSystem,
  EcoffGetMinimumSystemVersion,
  EcoffGetTargetSubsystem,
  EcoffGetMinimumSubsystemVersion,
  EcoffGetResource,
  EcoffGetResourceEnumerator
};

//
// ECOFF Loader Instance
//

IImageLoader gEcoffLoader = {
  &gEcoffVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gEcoffLoader);
