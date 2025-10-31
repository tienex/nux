/** @file
  APXH PE/COFF Loader

  Provides PE (Portable Executable) and COFF (Common Object File Format)
  parsing and loading for Windows executables using COM-style interface.
  Handles sections, imports, exports, TLS, and unwinding information for
  Win32/Win64 binaries.

  Supports:
  - PE32 (32-bit Windows executables)
  - PE32+ (64-bit Windows executables)
  - x86, x86-64, ARM, ARM64, RISC-V architectures
  - TLS callbacks and initialization
  - .pdata unwinding information

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

// Data Directory Indices
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION  3  ///< Exception (.pdata)
#define IMAGE_DIRECTORY_ENTRY_BASERELOC  5  ///< Base relocations (.reloc)
#define IMAGE_DIRECTORY_ENTRY_TLS        9  ///< TLS

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

typedef struct _PE_BASE_RELOCATION {
  UINT32  VirtualAddress;  ///< Page RVA
  UINT32  SizeOfBlock;     ///< Block size including this header
} PE_BASE_RELOCATION;

// Relocation types
#define IMAGE_REL_BASED_ABSOLUTE  0  ///< No-op
#define IMAGE_REL_BASED_HIGHLOW   3  ///< 32-bit fixup
#define IMAGE_REL_BASED_DIR64    10  ///< 64-bit fixup

ANX_PACK_POP()

//
// Helper Macros
//

#define PE_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// IImageLoader Implementation for PE/COFF
//

/**
  Detect if image is PE/COFF format.
**/
static
HRESULT
STDMETHODCALLTYPE
PeDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders;

  if (ImageSize < sizeof(DOS_HEADER)) {
    return S_FALSE;
  }

  DosHeader = (DOS_HEADER *)ImageBase;
  if (DosHeader->Signature != PE_DOS_SIGNATURE) {
    return S_FALSE;
  }

  if (DosHeader->NewHeaderOffset >= ImageSize - sizeof(PE_NT_HEADERS32)) {
    return S_FALSE;
  }

  NtHeaders = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  return (NtHeaders->Signature == PE_NT_SIGNATURE) ? S_OK : S_FALSE;
}

/**
  Get architecture from PE image.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  DosHeader = (DOS_HEADER *)ImageBase;
  NtHeaders = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);

  switch (NtHeaders->FileHeader.Machine) {
    case IMAGE_FILE_MACHINE_I386:
      *Architecture = Arch386;
      break;
    case IMAGE_FILE_MACHINE_AMD64:
      *Architecture = ArchAmd64;
      break;
    case IMAGE_FILE_MACHINE_RISCV64:
      *Architecture = ArchRiscV64;
      break;
    case IMAGE_FILE_MACHINE_ARM64:
      *Architecture = ArchArm64;
      break;
    default:
      *Architecture = ArchUnsupported;
      return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  return S_OK;
}

/**
  Get endianness from PE image.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // All Windows architectures are little-endian
  *Endianness = ImgEndianLittle;
  return S_OK;
}

/**
  Get entry point from PE image.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders32;
  PE_NT_HEADERS64 *NtHeaders64;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  DosHeader = (DOS_HEADER *)ImageBase;
  NtHeaders32 = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  NtHeaders64 = (PE_NT_HEADERS64 *)NtHeaders32;

  if (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32) {
    *EntryPoint = (VIRTUAL_ADDRESS)NtHeaders32->OptionalHeader.ImageBase +
                  NtHeaders32->OptionalHeader.EntryPointRVA;
  } else if (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32PLUS) {
    *EntryPoint = (VIRTUAL_ADDRESS)NtHeaders64->OptionalHeader.ImageBase +
                  NtHeaders64->OptionalHeader.EntryPointRVA;
  } else {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  if (*EntryPoint == 0 || *EntryPoint == (VIRTUAL_ADDRESS)-1) {
    return IMGLOAD_E_INVALID_HEADER;
  }

  return S_OK;
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
HRESULT
STDMETHODCALLTYPE
PeLoadImage (
  IN     IImageLoader     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders32;
  PE_NT_HEADERS64 *NtHeaders64;
  PE_SECTION_HEADER *Sections;
  UINT16 NumSections;
  UINT64 ImageBaseVA;
  BOOLEAN Is64Bit;
  UINT16 i;
  HRESULT Status;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  DosHeader = (DOS_HEADER *)ImageBase;
  NtHeaders32 = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  NtHeaders64 = (PE_NT_HEADERS64 *)NtHeaders32;

  Is64Bit = (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32PLUS);
  NumSections = NtHeaders32->FileHeader.NumSections;

  ImageBaseVA = Is64Bit ?
    NtHeaders64->OptionalHeader.ImageBase :
    (UINT64)NtHeaders32->OptionalHeader.ImageBase;

  // Populate context
  Status = PeGetArch(This, ImageBase, &Context->Architecture);
  if (FAILED(Status)) {
    return Status;
  }

  Status = PeGetEndianness(This, ImageBase, &Context->Endianness);
  if (FAILED(Status)) {
    return Status;
  }

  Status = PeGetEntryPoint(This, ImageBase, &Context->EntryPoint);
  if (FAILED(Status)) {
    return Status;
  }

  // Section headers follow the optional header
  Sections = (PE_SECTION_HEADER *)((UINT8 *)&NtHeaders32->OptionalHeader +
                                   NtHeaders32->FileHeader.OptionalHeaderSize);

  // Load all sections
  for (i = 0; i < NumSections; i++) {
    PE_SECTION_HEADER *Sec = &Sections[i];
    PeLoadSection(ImageBase, ImageBaseVA, Sec, Context->IsUserMode);
  }

  return S_OK;
}

/**
  Extract TLS information from PE image.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders32;
  PE_NT_HEADERS64 *NtHeaders64;
  PE_DATA_DIRECTORY *TlsDir;
  BOOLEAN Is64Bit;

  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));

  DosHeader = (DOS_HEADER *)ImageBase;
  NtHeaders32 = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  NtHeaders64 = (PE_NT_HEADERS64 *)NtHeaders32;

  Is64Bit = (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32PLUS);

  // Get TLS data directory
  TlsDir = Is64Bit ?
    &NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS] :
    &NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];

  if (TlsDir->VirtualAddress == 0 || TlsDir->Size == 0) {
    // No TLS - not an error
    return S_FALSE;
  }

  if (Is64Bit) {
    PE_TLS_DIRECTORY64 *TlsDirectory = (PE_TLS_DIRECTORY64 *)PE_OFF(TlsDir->VirtualAddress);

    TlsInfo->InitDataAddr = TlsDirectory->StartAddressOfRawData;
    TlsInfo->InitDataSize = TlsDirectory->EndAddressOfRawData -
                            TlsDirectory->StartAddressOfRawData;
    TlsInfo->TotalSize = TlsInfo->InitDataSize + TlsDirectory->SizeOfZeroFill;
    TlsInfo->IndexAddr = TlsDirectory->AddressOfIndex;
    TlsInfo->CallbacksAddr = TlsDirectory->AddressOfCallBacks;
    TlsInfo->Alignment = 1 << (TlsDirectory->Characteristics & 0xF);
  } else {
    PE_TLS_DIRECTORY32 *TlsDirectory = (PE_TLS_DIRECTORY32 *)PE_OFF(TlsDir->VirtualAddress);

    TlsInfo->InitDataAddr = TlsDirectory->StartAddressOfRawData;
    TlsInfo->InitDataSize = TlsDirectory->EndAddressOfRawData -
                            TlsDirectory->StartAddressOfRawData;
    TlsInfo->TotalSize = TlsInfo->InitDataSize + TlsDirectory->SizeOfZeroFill;
    TlsInfo->IndexAddr = TlsDirectory->AddressOfIndex;
    TlsInfo->CallbacksAddr = TlsDirectory->AddressOfCallBacks;
    TlsInfo->Alignment = 1 << (TlsDirectory->Characteristics & 0xF);
  }

  return S_OK;
}

/**
  Extract unwinding information from PE image.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders32;
  PE_NT_HEADERS64 *NtHeaders64;
  PE_DATA_DIRECTORY *ExceptionDir;
  BOOLEAN Is64Bit;

  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  DosHeader = (DOS_HEADER *)ImageBase;
  NtHeaders32 = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  NtHeaders64 = (PE_NT_HEADERS64 *)NtHeaders32;

  Is64Bit = (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32PLUS);

  // Get Exception data directory (.pdata)
  ExceptionDir = Is64Bit ?
    &NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION] :
    &NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];

  if (ExceptionDir->VirtualAddress == 0 || ExceptionDir->Size == 0) {
    // No unwinding info - not an error
    return S_FALSE;
  }

  UnwindInfo->UnwindDataAddr = ExceptionDir->VirtualAddress;
  UnwindInfo->UnwindDataSize = ExceptionDir->Size;
  UnwindInfo->Format = 1;  // PE .pdata format

  return S_OK;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  // PE symbol table parsing would go here
  // For now, return S_FALSE (not found)
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  // PE symbol table parsing would go here
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Extract relocation information from PE image.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetRelocInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_RELOC_INFO   *RelocInfo
  )
{
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders32;
  PE_NT_HEADERS64 *NtHeaders64;
  PE_DATA_DIRECTORY *RelocDir;
  BOOLEAN Is64Bit;

  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  DosHeader = (DOS_HEADER *)ImageBase;
  NtHeaders32 = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  NtHeaders64 = (PE_NT_HEADERS64 *)NtHeaders32;

  Is64Bit = (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32PLUS);

  // Get preferred base address
  if (Is64Bit) {
    RelocInfo->PreferredBase = NtHeaders64->OptionalHeader.ImageBase;
  } else {
    RelocInfo->PreferredBase = NtHeaders32->OptionalHeader.ImageBase;
  }

  // Get base relocation directory
  RelocDir = Is64Bit ?
    &NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC] :
    &NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

  if (RelocDir->VirtualAddress == 0 || RelocDir->Size == 0) {
    RelocInfo->RequiresReloc = FALSE;
    return S_FALSE;  // No relocations
  }

  RelocInfo->RelocTableAddr = RelocDir->VirtualAddress;
  RelocInfo->RelocTableSize = RelocDir->Size;
  RelocInfo->Format = ImgRelocFormatPe;
  RelocInfo->RequiresReloc = TRUE;

  return S_OK;
}

/**
  Apply relocations to PE image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
PeApplyRelocations (
  IN VOID              *ImageBase,
  IN VIRTUAL_ADDRESS   LoadAddress,
  IN VIRTUAL_ADDRESS   PreferredBase
  )
{
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders32;
  PE_NT_HEADERS64 *NtHeaders64;
  PE_DATA_DIRECTORY *RelocDir;
  PE_BASE_RELOCATION *RelocBlock;
  UINT16 *RelocEntry;
  UINTN RelocOffset;
  UINTN BlockSize;
  UINTN NumEntries;
  UINTN i;
  INT64 Delta;
  BOOLEAN Is64Bit;
  UINT32 *Fixup32;
  UINT64 *Fixup64;
  UINT16 Type;
  UINT16 Offset;

  DosHeader = (DOS_HEADER *)ImageBase;
  NtHeaders32 = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  NtHeaders64 = (PE_NT_HEADERS64 *)NtHeaders32;

  Is64Bit = (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32PLUS);

  // Calculate relocation delta
  Delta = (INT64)LoadAddress - (INT64)PreferredBase;

  if (Delta == 0) {
    return S_OK;  // No relocation needed
  }

  // Get base relocation directory
  RelocDir = Is64Bit ?
    &NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC] :
    &NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

  if (RelocDir->VirtualAddress == 0 || RelocDir->Size == 0) {
    return S_OK;  // No relocations to apply
  }

  // Process each relocation block
  RelocOffset = 0;
  while (RelocOffset < RelocDir->Size) {
    RelocBlock = (PE_BASE_RELOCATION *)PE_OFF(RelocDir->VirtualAddress + RelocOffset);

    if (RelocBlock->SizeOfBlock == 0) {
      break;  // End of relocation data
    }

    BlockSize = RelocBlock->SizeOfBlock - sizeof(PE_BASE_RELOCATION);
    NumEntries = BlockSize / sizeof(UINT16);
    RelocEntry = (UINT16 *)((UINT8 *)RelocBlock + sizeof(PE_BASE_RELOCATION));

    for (i = 0; i < NumEntries; i++) {
      Type = (RelocEntry[i] >> 12) & 0xF;
      Offset = RelocEntry[i] & 0xFFF;

      switch (Type) {
        case IMAGE_REL_BASED_ABSOLUTE:
          // No-op, used for padding
          break;

        case IMAGE_REL_BASED_HIGHLOW:
          // 32-bit fixup
          Fixup32 = (UINT32 *)PE_OFF(RelocBlock->VirtualAddress + Offset);
          *Fixup32 = (UINT32)(*Fixup32 + Delta);
          break;

        case IMAGE_REL_BASED_DIR64:
          // 64-bit fixup
          Fixup64 = (UINT64 *)PE_OFF(RelocBlock->VirtualAddress + Offset);
          *Fixup64 = *Fixup64 + Delta;
          break;

        default:
          // Unknown relocation type, skip
          break;
      }
    }

    RelocOffset += RelocBlock->SizeOfBlock;
  }

  return S_OK;
}

/**
  IUnknown::QueryInterface implementation.
**/
static
HRESULT
STDMETHODCALLTYPE
PeQueryInterface (
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
PeAddRef (
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
PeRelease (
  IN IImageLoader  *This
  )
{
  return 1;
}

//
// PE Loader VTable
//

static CONST IImageLoaderVtbl gPeVtbl = {
  PeQueryInterface,
  PeAddRef,
  PeRelease,
  PeDetect,
  PeGetArch,
  PeGetEndianness,
  PeGetEntryPoint,
  PeLoadImage,
  PeGetTlsInfo,
  PeGetUnwindInfo,
  PeGetSymbolByAddress,
  PeGetSymbolByName,
  PeGetRelocInfo,
  PeApplyRelocations
};

//
// PE Loader Instance
//

IImageLoader gPeLoader = {
  &gPeVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gPeLoader);
