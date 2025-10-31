/** @file
  APXH PE/COFF Loader

  Provides PE (Portable Executable) and COFF (Common Object File Format)
  parsing and loading for Windows executables. Handles sections, imports,
  exports, and Thread-Local Storage (TLS) for Win32/Win64 binaries.

  Supports:
  - PE32 (32-bit Windows executables)
  - PE32+ (64-bit Windows executables)
  - x86, x86-64 architectures
  - TLS callbacks and initialization

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// PE/COFF Magic Numbers
//

#define PE_DOS_SIGNATURE      0x5A4D  ///< "MZ" - DOS header signature
#define PE_NT_SIGNATURE       0x00004550  ///< "PE\0\0" - NT header signature
#define PE_OPT_MAGIC_PE32     0x10B   ///< PE32 optional header
#define PE_OPT_MAGIC_PE32PLUS 0x20B   ///< PE32+ (64-bit) optional header

//
// Machine Types
//

#define IMAGE_FILE_MACHINE_I386   0x014C  ///< x86
#define IMAGE_FILE_MACHINE_AMD64  0x8664  ///< x86-64
#define IMAGE_FILE_MACHINE_ARM    0x01C0  ///< ARM
#define IMAGE_FILE_MACHINE_ARM64  0xAA64  ///< ARM64
#define IMAGE_FILE_MACHINE_RISCV64 0x5064 ///< RISC-V 64-bit

//
// Section Characteristics
//

#define IMAGE_SCN_MEM_EXECUTE   0x20000000  ///< Executable
#define IMAGE_SCN_MEM_READ      0x40000000  ///< Readable
#define IMAGE_SCN_MEM_WRITE     0x80000000  ///< Writable

//
// PE/COFF Structures
//

ANX_PACK_PUSH(1)

typedef struct _DOS_HEADER {
  UINT16  Signature;      ///< "MZ"
  UINT16  LastSize;
  UINT16  NumPages;
  UINT16  Relocations;
  UINT16  HeaderSize;
  UINT16  MinAlloc;
  UINT16  MaxAlloc;
  UINT16  InitSS;
  UINT16  InitSP;
  UINT16  Checksum;
  UINT16  InitIP;
  UINT16  InitCS;
  UINT16  RelocTableOff;
  UINT16  Overlay;
  UINT16  Reserved[4];
  UINT16  OemId;
  UINT16  OemInfo;
  UINT16  Reserved2[10];
  UINT32  NewHeaderOffset;  ///< Offset to PE header
} DOS_HEADER;

typedef struct _COFF_HEADER {
  UINT16  Machine;              ///< Machine type
  UINT16  NumSections;          ///< Number of sections
  UINT32  TimeDateStamp;        ///< Time/date stamp
  UINT32  SymbolTableOffset;    ///< Symbol table offset
  UINT32  NumSymbols;           ///< Number of symbols
  UINT16  OptionalHeaderSize;   ///< Optional header size
  UINT16  Characteristics;      ///< Characteristics
} COFF_HEADER;

typedef struct _PE_DATA_DIRECTORY {
  UINT32  VirtualAddress;  ///< RVA
  UINT32  Size;            ///< Size
} PE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct _PE_OPTIONAL_HEADER32 {
  UINT16  Magic;                    ///< PE32 magic
  UINT8   LinkerMajorVersion;
  UINT8   LinkerMinorVersion;
  UINT32  SizeOfCode;
  UINT32  SizeOfInitializedData;
  UINT32  SizeOfUninitializedData;
  UINT32  EntryPointRVA;            ///< Entry point RVA
  UINT32  BaseOfCode;
  UINT32  BaseOfData;
  UINT32  ImageBase;                ///< Preferred load address
  UINT32  SectionAlignment;
  UINT32  FileAlignment;
  UINT16  OsMajorVersion;
  UINT16  OsMinorVersion;
  UINT16  ImageMajorVersion;
  UINT16  ImageMinorVersion;
  UINT16  SubsystemMajorVersion;
  UINT16  SubsystemMinorVersion;
  UINT32  Win32VersionValue;
  UINT32  SizeOfImage;
  UINT32  SizeOfHeaders;
  UINT32  Checksum;
  UINT16  Subsystem;
  UINT16  DllCharacteristics;
  UINT32  SizeOfStackReserve;
  UINT32  SizeOfStackCommit;
  UINT32  SizeOfHeapReserve;
  UINT32  SizeOfHeapCommit;
  UINT32  LoaderFlags;
  UINT32  NumRvaAndSizes;
  PE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} PE_OPTIONAL_HEADER32;

typedef struct _PE_OPTIONAL_HEADER64 {
  UINT16  Magic;                    ///< PE32+ magic
  UINT8   LinkerMajorVersion;
  UINT8   LinkerMinorVersion;
  UINT32  SizeOfCode;
  UINT32  SizeOfInitializedData;
  UINT32  SizeOfUninitializedData;
  UINT32  EntryPointRVA;            ///< Entry point RVA
  UINT32  BaseOfCode;
  UINT64  ImageBase;                ///< Preferred load address
  UINT32  SectionAlignment;
  UINT32  FileAlignment;
  UINT16  OsMajorVersion;
  UINT16  OsMinorVersion;
  UINT16  ImageMajorVersion;
  UINT16  ImageMinorVersion;
  UINT16  SubsystemMajorVersion;
  UINT16  SubsystemMinorVersion;
  UINT32  Win32VersionValue;
  UINT32  SizeOfImage;
  UINT32  SizeOfHeaders;
  UINT32  Checksum;
  UINT16  Subsystem;
  UINT16  DllCharacteristics;
  UINT64  SizeOfStackReserve;
  UINT64  SizeOfStackCommit;
  UINT64  SizeOfHeapReserve;
  UINT64  SizeOfHeapCommit;
  UINT32  LoaderFlags;
  UINT32  NumRvaAndSizes;
  PE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} PE_OPTIONAL_HEADER64;

typedef struct _PE_NT_HEADERS32 {
  UINT32              Signature;        ///< "PE\0\0"
  COFF_HEADER         FileHeader;
  PE_OPTIONAL_HEADER32 OptionalHeader;
} PE_NT_HEADERS32;

typedef struct _PE_NT_HEADERS64 {
  UINT32              Signature;        ///< "PE\0\0"
  COFF_HEADER         FileHeader;
  PE_OPTIONAL_HEADER64 OptionalHeader;
} PE_NT_HEADERS64;

typedef struct _PE_SECTION_HEADER {
  CHAR8   Name[8];              ///< Section name
  UINT32  VirtualSize;          ///< Virtual size
  UINT32  VirtualAddress;       ///< Virtual address (RVA)
  UINT32  SizeOfRawData;        ///< Size of raw data
  UINT32  PointerToRawData;     ///< File pointer to raw data
  UINT32  PointerToRelocations;
  UINT32  PointerToLinenumbers;
  UINT16  NumberOfRelocations;
  UINT16  NumberOfLinenumbers;
  UINT32  Characteristics;      ///< Section characteristics
} PE_SECTION_HEADER;

// TLS Directory Entry (Data Directory index 9)
#define IMAGE_DIRECTORY_ENTRY_TLS 9

typedef struct _PE_TLS_DIRECTORY32 {
  UINT32  StartAddressOfRawData;  ///< Start of TLS data
  UINT32  EndAddressOfRawData;    ///< End of TLS data
  UINT32  AddressOfIndex;         ///< TLS index address
  UINT32  AddressOfCallBacks;     ///< TLS callback array
  UINT32  SizeOfZeroFill;         ///< BSS size
  UINT32  Characteristics;        ///< Alignment (low 4 bits)
} PE_TLS_DIRECTORY32;

typedef struct _PE_TLS_DIRECTORY64 {
  UINT64  StartAddressOfRawData;  ///< Start of TLS data
  UINT64  EndAddressOfRawData;    ///< End of TLS data
  UINT64  AddressOfIndex;         ///< TLS index address
  UINT64  AddressOfCallBacks;     ///< TLS callback array
  UINT32  SizeOfZeroFill;         ///< BSS size
  UINT32  Characteristics;        ///< Alignment (low 4 bits)
} PE_TLS_DIRECTORY64;

ANX_PACK_POP()

//
// Helper Macros
//

#define PE_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// Internal Functions
//

/**
  Check if image is PE/COFF format.
**/
static
BOOLEAN
ANXAPI
PeDetect (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders;

  if (ImageSize < sizeof(DOS_HEADER)) {
    return FALSE;
  }

  DosHeader = (DOS_HEADER *)ImageBase;
  if (DosHeader->Signature != PE_DOS_SIGNATURE) {
    return FALSE;
  }

  if (DosHeader->NewHeaderOffset >= ImageSize - sizeof(PE_NT_HEADERS32)) {
    return FALSE;
  }

  NtHeaders = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  return (NtHeaders->Signature == PE_NT_SIGNATURE);
}

/**
  Get architecture from PE image.
**/
static
ARCH
ANXAPI
PeGetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  DOS_HEADER *DosHeader = (DOS_HEADER *)ImageBase;
  PE_NT_HEADERS32 *NtHeaders = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);

  switch (NtHeaders->FileHeader.Machine) {
    case IMAGE_FILE_MACHINE_I386:
      return ARCH_386;
    case IMAGE_FILE_MACHINE_AMD64:
      return ARCH_AMD64;
    case IMAGE_FILE_MACHINE_RISCV64:
      return ARCH_RISCV64;
    case IMAGE_FILE_MACHINE_ARM64:
      return ARCH_UNSUPPORTED;
    default:
      return ARCH_UNSUPPORTED;
  }
}

/**
  Get entry point from PE image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
PeGetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  DOS_HEADER *DosHeader = (DOS_HEADER *)ImageBase;
  PE_NT_HEADERS32 *NtHeaders32 = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  PE_NT_HEADERS64 *NtHeaders64 = (PE_NT_HEADERS64 *)NtHeaders32;

  if (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32) {
    return (VIRTUAL_ADDRESS)NtHeaders32->OptionalHeader.ImageBase +
           NtHeaders32->OptionalHeader.EntryPointRVA;
  } else if (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32PLUS) {
    return (VIRTUAL_ADDRESS)NtHeaders64->OptionalHeader.ImageBase +
           NtHeaders64->OptionalHeader.EntryPointRVA;
  }

  return 0;
}

/**
  Load PE section.
**/
static
VOID
PeLoadSection (
  IN VOID                *ImageBase,
  IN UINT64              ImageBaseVA,
  IN PE_SECTION_HEADER   *Section,
  IN BOOLEAN             IsUserMode
  )
{
  UINT64 VirtualAddr = ImageBaseVA + Section->VirtualAddress;
  BOOLEAN IsWritable = !!(Section->Characteristics & IMAGE_SCN_MEM_WRITE);
  BOOLEAN IsExecutable = !!(Section->Characteristics & IMAGE_SCN_MEM_EXECUTE);

  if (Section->SizeOfRawData > 0) {
    // Copy section data from file
    VirtualAddressCopy(
      VirtualAddr,
      PE_OFF(Section->PointerToRawData),
      Section->SizeOfRawData,
      IsUserMode,
      IsWritable,
      IsExecutable
    );
  }

  if (Section->VirtualSize > Section->SizeOfRawData) {
    // Zero-fill remainder
    VirtualAddressMemset(
      VirtualAddr + Section->SizeOfRawData,
      0,
      Section->VirtualSize - Section->SizeOfRawData,
      IsUserMode,
      IsWritable,
      IsExecutable
    );
  }
}

/**
  Load PE image.
**/
static
IMGLOAD_STATUS
ANXAPI
PeLoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase = Context->ImageBase;
  DOS_HEADER *DosHeader = (DOS_HEADER *)ImageBase;
  PE_NT_HEADERS32 *NtHeaders32 = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  PE_NT_HEADERS64 *NtHeaders64 = (PE_NT_HEADERS64 *)NtHeaders32;
  PE_SECTION_HEADER *Sections;
  UINT16 NumSections;
  UINT64 ImageBaseVA;
  BOOLEAN Is64Bit;
  UINT16 i;

  Is64Bit = (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32PLUS);
  NumSections = NtHeaders32->FileHeader.NumSections;

  ImageBaseVA = Is64Bit ?
    NtHeaders64->OptionalHeader.ImageBase :
    (UINT64)NtHeaders32->OptionalHeader.ImageBase;

  info("Loading PE%s executable...", Is64Bit ? "32+" : "32");

  // Section headers follow the optional header
  Sections = (PE_SECTION_HEADER *)((UINT8 *)&NtHeaders32->OptionalHeader +
                                   NtHeaders32->FileHeader.OptionalHeaderSize);

  // Load all sections
  for (i = 0; i < NumSections; i++) {
    PE_SECTION_HEADER *Sec = &Sections[i];
    info("  Section %.8s at 0x%08x (size: 0x%08x, chars: 0x%08x)",
         Sec->Name, Sec->VirtualAddress, Sec->VirtualSize, Sec->Characteristics);

    PeLoadSection(ImageBase, ImageBaseVA, Sec, Context->IsUserMode);
  }

  Context->EntryPoint = PeGetEntryPoint(This, ImageBase);
  return ImgLoadSuccess;
}

/**
  Extract TLS information from PE image.
**/
static
IMGLOAD_STATUS
ANXAPI
PeGetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  DOS_HEADER *DosHeader = (DOS_HEADER *)ImageBase;
  PE_NT_HEADERS32 *NtHeaders32 = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  PE_NT_HEADERS64 *NtHeaders64 = (PE_NT_HEADERS64 *)NtHeaders32;
  PE_DATA_DIRECTORY *TlsDir;
  BOOLEAN Is64Bit;

  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));

  Is64Bit = (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32PLUS);

  // Get TLS data directory
  TlsDir = Is64Bit ?
    &NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS] :
    &NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];

  if (TlsDir->VirtualAddress == 0 || TlsDir->Size == 0) {
    // No TLS - not an error
    return ImgLoadSuccess;
  }

  if (Is64Bit) {
    PE_TLS_DIRECTORY64 *TlsDirectory;
    UINT64 ImageBaseVA = NtHeaders64->OptionalHeader.ImageBase;

    TlsDirectory = (PE_TLS_DIRECTORY64 *)PE_OFF(TlsDir->VirtualAddress);

    TlsInfo->InitDataAddr = TlsDirectory->StartAddressOfRawData;
    TlsInfo->InitDataSize = TlsDirectory->EndAddressOfRawData -
                            TlsDirectory->StartAddressOfRawData;
    TlsInfo->TotalSize = TlsInfo->InitDataSize + TlsDirectory->SizeOfZeroFill;
    TlsInfo->IndexAddr = TlsDirectory->AddressOfIndex;
    TlsInfo->CallbacksAddr = TlsDirectory->AddressOfCallBacks;
    TlsInfo->Alignment = 1 << (TlsDirectory->Characteristics & 0xF);
  } else {
    PE_TLS_DIRECTORY32 *TlsDirectory;
    UINT32 ImageBaseVA = NtHeaders32->OptionalHeader.ImageBase;

    TlsDirectory = (PE_TLS_DIRECTORY32 *)PE_OFF(TlsDir->VirtualAddress);

    TlsInfo->InitDataAddr = TlsDirectory->StartAddressOfRawData;
    TlsInfo->InitDataSize = TlsDirectory->EndAddressOfRawData -
                            TlsDirectory->StartAddressOfRawData;
    TlsInfo->TotalSize = TlsInfo->InitDataSize + TlsDirectory->SizeOfZeroFill;
    TlsInfo->IndexAddr = TlsDirectory->AddressOfIndex;
    TlsInfo->CallbacksAddr = TlsDirectory->AddressOfCallBacks;
    TlsInfo->Alignment = 1 << (TlsDirectory->Characteristics & 0xF);
  }

  return ImgLoadSuccess;
}

//
// PE Loader VTable
//

static CONST IMAGE_LOADER_VTBL gPeVtbl = {
  PeDetect,
  PeGetArch,
  PeGetEntryPoint,
  PeLoadImage,
  PeGetTlsInfo
};

//
// PE Loader Instance
//

IMAGE_LOADER gPeLoader = {
  &gPeVtbl,
  "PE/COFF",
  NULL
};
