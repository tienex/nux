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
#include <ananke/resource.h>
#include "imgresource.h"

//
// PE/COFF Magic Numbers
//

#define PE_DOS_SIGNATURE      0x5A4D  ///< "MZ" - DOS header signature
#define PE_NT_SIGNATURE       0x00004550  ///< "PE\0\0" - NT header signature
#define PE_OPT_MAGIC_PE32     0x10B   ///< PE32 optional header
#define PE_OPT_MAGIC_PE32PLUS 0x20B   ///< PE32+ (64-bit) optional header

//
// Machine Types (comprehensive, including historical)
//

#define IMAGE_FILE_MACHINE_UNKNOWN   0x0000  ///< Unknown
#define IMAGE_FILE_MACHINE_I386      0x014C  ///< Intel x86
#define IMAGE_FILE_MACHINE_R3000     0x0162  ///< MIPS R3000 (little endian)
#define IMAGE_FILE_MACHINE_R4000     0x0166  ///< MIPS R4000 (little endian)
#define IMAGE_FILE_MACHINE_R10000    0x0168  ///< MIPS R10000 (little endian)
#define IMAGE_FILE_MACHINE_WCEMIPSV2 0x0169  ///< MIPS WCE v2 (little endian)
#define IMAGE_FILE_MACHINE_ALPHA     0x0184  ///< DEC Alpha AXP
#define IMAGE_FILE_MACHINE_SH3       0x01A2  ///< Hitachi SH3
#define IMAGE_FILE_MACHINE_SH3DSP    0x01A3  ///< Hitachi SH3 DSP
#define IMAGE_FILE_MACHINE_SH3E      0x01A4  ///< Hitachi SH3E
#define IMAGE_FILE_MACHINE_SH4       0x01A6  ///< Hitachi SH4
#define IMAGE_FILE_MACHINE_SH5       0x01A8  ///< Hitachi SH5
#define IMAGE_FILE_MACHINE_ARM       0x01C0  ///< ARM little endian
#define IMAGE_FILE_MACHINE_THUMB     0x01C2  ///< ARM Thumb/Thumb-2 LE
#define IMAGE_FILE_MACHINE_ARMNT     0x01C4  ///< ARM Thumb-2 LE
#define IMAGE_FILE_MACHINE_AM33      0x01D3  ///< Matsushita AM33
#define IMAGE_FILE_MACHINE_POWERPC   0x01F0  ///< PowerPC little endian
#define IMAGE_FILE_MACHINE_POWERPCFP 0x01F1  ///< PowerPC with FP support
#define IMAGE_FILE_MACHINE_POWERPCBE 0x01F2  ///< PowerPC big endian
#define IMAGE_FILE_MACHINE_IA64      0x0200  ///< Intel Itanium
#define IMAGE_FILE_MACHINE_MACPPC    0x01DF  ///< Mac PowerPC (unofficial)
#define IMAGE_FILE_MACHINE_M68K      0x0268  ///< Motorola 68000
#define IMAGE_FILE_MACHINE_MIPS16    0x0266  ///< MIPS16
#define IMAGE_FILE_MACHINE_ALPHA64   0x0284  ///< Alpha AXP 64-bit
#define IMAGE_FILE_MACHINE_MIPSFPU   0x0366  ///< MIPS with FPU
#define IMAGE_FILE_MACHINE_MIPSFPU16 0x0466  ///< MIPS16 with FPU
#define IMAGE_FILE_MACHINE_TRICORE   0x0520  ///< Infineon TriCore
#define IMAGE_FILE_MACHINE_CEF       0x0CEF  ///< CEF
#define IMAGE_FILE_MACHINE_EBC       0x0EBC  ///< EFI Byte Code
#define IMAGE_FILE_MACHINE_AMD64     0x8664  ///< AMD64/x86-64
#define IMAGE_FILE_MACHINE_M32R      0x9041  ///< Mitsubishi M32R LE
#define IMAGE_FILE_MACHINE_ARM64     0xAA64  ///< ARM64/AArch64
#define IMAGE_FILE_MACHINE_CEE       0xC0EE  ///< CLR pure MSIL
#define IMAGE_FILE_MACHINE_RISCV32   0x5032  ///< RISC-V 32-bit
#define IMAGE_FILE_MACHINE_RISCV64   0x5064  ///< RISC-V 64-bit
#define IMAGE_FILE_MACHINE_RISCV128  0x5128  ///< RISC-V 128-bit
#define IMAGE_FILE_MACHINE_LOONGARCH32 0x6232 ///< LoongArch 32-bit
#define IMAGE_FILE_MACHINE_LOONGARCH64 0x6264 ///< LoongArch 64-bit

//
// Section Characteristics
//

#define IMAGE_SCN_MEM_EXECUTE   0x20000000  ///< Executable
#define IMAGE_SCN_MEM_READ      0x40000000  ///< Readable
#define IMAGE_SCN_MEM_WRITE     0x80000000  ///< Writable

//
// Subsystem Types
//

#define IMAGE_SUBSYSTEM_UNKNOWN                  0   ///< Unknown subsystem
#define IMAGE_SUBSYSTEM_NATIVE                   1   ///< Native (kernel mode)
#define IMAGE_SUBSYSTEM_WINDOWS_GUI              2   ///< Windows GUI
#define IMAGE_SUBSYSTEM_WINDOWS_CUI              3   ///< Windows console
#define IMAGE_SUBSYSTEM_OS2_GUI                  4   ///< OS/2 GUI (Presentation Manager)
#define IMAGE_SUBSYSTEM_OS2_CUI                  5   ///< OS/2 console
#define IMAGE_SUBSYSTEM_BEOS_GUI                 6   ///< BeOS GUI (x86 version)
#define IMAGE_SUBSYSTEM_POSIX_CUI                7   ///< POSIX console
#define IMAGE_SUBSYSTEM_NATIVE_WINDOWS           8   ///< Native Win9x driver
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI           9   ///< Windows CE GUI
#define IMAGE_SUBSYSTEM_EFI_APPLICATION          10  ///< UEFI application
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER  11  ///< UEFI boot service driver
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER       12  ///< UEFI runtime driver
#define IMAGE_SUBSYSTEM_EFI_ROM                  13  ///< UEFI ROM image
#define IMAGE_SUBSYSTEM_XBOX                     14  ///< Xbox system
#define IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION 16  ///< Windows boot application

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
#define IMAGE_DIRECTORY_ENTRY_EXPORT     0  ///< Export directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT     1  ///< Import directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE   2  ///< Resource directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION  3  ///< Exception (.pdata)
#define IMAGE_DIRECTORY_ENTRY_BASERELOC  5  ///< Base relocations (.reloc)
#define IMAGE_DIRECTORY_ENTRY_TLS        9  ///< TLS

//
// Resource Directory Structures
//

typedef struct _PE_RESOURCE_DIRECTORY {
  UINT32  Characteristics;        ///< Resource flags
  UINT32  TimeDateStamp;          ///< Creation time
  UINT16  MajorVersion;           ///< Major version
  UINT16  MinorVersion;           ///< Minor version
  UINT16  NumberOfNamedEntries;   ///< Number of named entries
  UINT16  NumberOfIdEntries;      ///< Number of ID entries
} PE_RESOURCE_DIRECTORY;

typedef struct _PE_RESOURCE_DIRECTORY_ENTRY {
  UINT32  Name;                   ///< Name offset (high bit set) or ID
  UINT32  OffsetToData;           ///< Subdirectory offset (high bit set) or data RVA
} PE_RESOURCE_DIRECTORY_ENTRY;

typedef struct _PE_RESOURCE_DATA_ENTRY {
  UINT32  OffsetToData;           ///< RVA of resource data
  UINT32  Size;                   ///< Size of resource data
  UINT32  CodePage;               ///< Code page
  UINT32  Reserved;               ///< Reserved (0)
} PE_RESOURCE_DATA_ENTRY;

//
// Resource Type Constants (standard Windows types)
//

#define RT_CURSOR       1   ///< Cursor
#define RT_BITMAP       2   ///< Bitmap
#define RT_ICON         3   ///< Icon
#define RT_MENU         4   ///< Menu
#define RT_DIALOG       5   ///< Dialog
#define RT_STRING       6   ///< String table
#define RT_FONTDIR      7   ///< Font directory
#define RT_FONT         8   ///< Font
#define RT_ACCELERATOR  9   ///< Accelerator table
#define RT_RCDATA       10  ///< Raw data
#define RT_MESSAGETABLE 11  ///< Message table
#define RT_VERSION      16  ///< Version information
#define RT_PLUGPLAY     19  ///< Plug and Play
#define RT_VXD          20  ///< VxD
#define RT_ANICURSOR    21  ///< Animated cursor
#define RT_ANIICON      22  ///< Animated icon
#define RT_HTML         23  ///< HTML
#define RT_MANIFEST     24  ///< Manifest

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

typedef struct _PE_EXPORT_DIRECTORY {
  UINT32  Characteristics;        ///< Reserved, must be 0
  UINT32  TimeDateStamp;          ///< Time/date stamp
  UINT16  MajorVersion;           ///< Major version
  UINT16  MinorVersion;           ///< Minor version
  UINT32  Name;                   ///< RVA of DLL name
  UINT32  Base;                   ///< Starting ordinal number
  UINT32  NumberOfFunctions;      ///< Number of entries in EAT
  UINT32  NumberOfNames;          ///< Number of entries in name pointer table
  UINT32  AddressOfFunctions;     ///< RVA of export address table (EAT)
  UINT32  AddressOfNames;         ///< RVA of export name pointer table
  UINT32  AddressOfNameOrdinals;  ///< RVA of ordinal table
} PE_EXPORT_DIRECTORY;

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
    case IMAGE_FILE_MACHINE_ARM:
    case IMAGE_FILE_MACHINE_ARMNT:
      *Architecture = ArchArm;
      break;
    case IMAGE_FILE_MACHINE_THUMB:
      *Architecture = ArchThumb;
      break;
    case IMAGE_FILE_MACHINE_ARM64:
      *Architecture = ArchArm64;
      break;
    case IMAGE_FILE_MACHINE_RISCV32:
      *Architecture = ArchRiscV32;
      break;
    case IMAGE_FILE_MACHINE_RISCV64:
      *Architecture = ArchRiscV64;
      break;
    case IMAGE_FILE_MACHINE_RISCV128:
      *Architecture = ArchRiscV128;
      break;
    case IMAGE_FILE_MACHINE_LOONGARCH32:
      *Architecture = ArchLoongArch32;
      break;
    case IMAGE_FILE_MACHINE_LOONGARCH64:
      *Architecture = ArchLoongArch64;
      break;
    case IMAGE_FILE_MACHINE_POWERPC:
    case IMAGE_FILE_MACHINE_POWERPCFP:
    case IMAGE_FILE_MACHINE_POWERPCBE:
    case IMAGE_FILE_MACHINE_MACPPC:
      *Architecture = ArchPpc32;
      break;
    case IMAGE_FILE_MACHINE_M68K:
      *Architecture = ArchM68k;
      break;
    case IMAGE_FILE_MACHINE_R3000:
      *Architecture = ArchMipsR3000;
      break;
    case IMAGE_FILE_MACHINE_R4000:
      *Architecture = ArchMipsR4000;
      break;
    case IMAGE_FILE_MACHINE_R10000:
      *Architecture = ArchMipsR10000;
      break;
    case IMAGE_FILE_MACHINE_WCEMIPSV2:
    case IMAGE_FILE_MACHINE_MIPS16:
    case IMAGE_FILE_MACHINE_MIPSFPU:
    case IMAGE_FILE_MACHINE_MIPSFPU16:
      *Architecture = ArchMips32;
      break;
    case IMAGE_FILE_MACHINE_ALPHA:
    case IMAGE_FILE_MACHINE_ALPHA64:
      *Architecture = ArchAlpha;
      break;
    case IMAGE_FILE_MACHINE_SH3:
      *Architecture = ArchSh3;
      break;
    case IMAGE_FILE_MACHINE_SH3DSP:
    case IMAGE_FILE_MACHINE_SH3E:
      *Architecture = ArchSh3;
      break;
    case IMAGE_FILE_MACHINE_SH4:
      *Architecture = ArchSh4;
      break;
    case IMAGE_FILE_MACHINE_SH5:
      *Architecture = ArchSh5;
      break;
    case IMAGE_FILE_MACHINE_IA64:
      *Architecture = ArchIa64;
      break;
    case IMAGE_FILE_MACHINE_AM33:
      *Architecture = ArchAm29000;
      break;
    case IMAGE_FILE_MACHINE_M32R:
      *Architecture = ArchUnsupported; // No specific M32R arch defined yet
      break;
    case IMAGE_FILE_MACHINE_EBC:
      *Architecture = ArchUnsupported; // EFI Byte Code is virtual
      break;
    case IMAGE_FILE_MACHINE_CEE:
      *Architecture = ArchUnsupported; // CLR MSIL is virtual
      break;
    case IMAGE_FILE_MACHINE_TRICORE:
    case IMAGE_FILE_MACHINE_CEF:
    case IMAGE_FILE_MACHINE_UNKNOWN:
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
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders;

  if (Endianness == NULL) {
    return E_POINTER;
  }

  DosHeader = (DOS_HEADER *)ImageBase;
  NtHeaders = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);

  // Check for big-endian architectures
  switch (NtHeaders->FileHeader.Machine) {
    case 0x0160:  // MIPS R3000 big endian (unofficial)
    case IMAGE_FILE_MACHINE_POWERPCBE:
    case IMAGE_FILE_MACHINE_MACPPC:
    case IMAGE_FILE_MACHINE_M68K:
      *Endianness = ImgEndianBig;
      return S_OK;
    default:
      break;
  }

  // All other Windows PE architectures are little-endian
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
    VasCopy(
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
    VasFill(
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
  UnwindInfo->Format = ImgUnwindFormatPeExceptionTable;

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
  Look up symbol by name in PE export directory.
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
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders32;
  PE_NT_HEADERS64 *NtHeaders64;
  PE_DATA_DIRECTORY *ExportDir;
  PE_EXPORT_DIRECTORY *ExportDirectory;
  UINT32 *NamePointerTable;
  UINT16 *OrdinalTable;
  UINT32 *AddressTable;
  UINT32 i;
  BOOLEAN Is64Bit;
  UINT64 ImageBaseVA;
  UINTN NameLen;

  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  DosHeader = (DOS_HEADER *)ImageBase;
  NtHeaders32 = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  NtHeaders64 = (PE_NT_HEADERS64 *)NtHeaders32;

  Is64Bit = (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32PLUS);

  // Get export data directory
  ExportDir = Is64Bit ?
    &NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] :
    &NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

  if (ExportDir->VirtualAddress == 0 || ExportDir->Size == 0) {
    // No exports
    return S_FALSE;
  }

  ExportDirectory = (PE_EXPORT_DIRECTORY *)PE_OFF(ExportDir->VirtualAddress);

  // Get export tables
  NamePointerTable = (UINT32 *)PE_OFF(ExportDirectory->AddressOfNames);
  OrdinalTable = (UINT16 *)PE_OFF(ExportDirectory->AddressOfNameOrdinals);
  AddressTable = (UINT32 *)PE_OFF(ExportDirectory->AddressOfFunctions);

  ImageBaseVA = Is64Bit ?
    NtHeaders64->OptionalHeader.ImageBase :
    (UINT64)NtHeaders32->OptionalHeader.ImageBase;

  NameLen = strlen(Name);

  // Search for the export by name
  for (i = 0; i < ExportDirectory->NumberOfNames; i++) {
    CHAR8 *ExportName = (CHAR8 *)PE_OFF(NamePointerTable[i]);
    UINTN ExportNameLen = strlen(ExportName);

    if (ExportNameLen == NameLen && memcmp(ExportName, Name, NameLen) == 0) {
      // Found the export
      UINT16 Ordinal = OrdinalTable[i];
      UINT32 FunctionRVA = AddressTable[Ordinal];

      // Copy name
      UINTN CopyLen = (NameLen < sizeof(SymbolInfo->Name) - 1) ?
                      NameLen : (sizeof(SymbolInfo->Name) - 1);
      memcpy(SymbolInfo->Name, Name, CopyLen);
      SymbolInfo->Name[CopyLen] = '\0';

      // Set address
      SymbolInfo->Address = ImageBaseVA + FunctionRVA;
      SymbolInfo->Size = 0;  // PE doesn't store symbol size in exports

      return S_OK;
    }
  }

  // Not found
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

/**
  Get target operating system from PE image.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetTargetSystem (
  IN  IImageLoader           *This,
  IN  VOID                   *ImageBase,
  OUT IMGLOAD_TARGET_SYSTEM  *TargetSystem
  )
{
  DOS_HEADER *DosHeader;
  PE_OPTIONAL_HEADER32 *OptHeader32;
  PE_OPTIONAL_HEADER64 *OptHeader64;
  COFF_HEADER *CoffHeader;
  UINT16 Subsystem;
  BOOLEAN Is64Bit;

  if (TargetSystem == NULL) {
    return E_POINTER;
  }

  DosHeader = (DOS_HEADER *)ImageBase;
  CoffHeader = (COFF_HEADER *)((UINT8 *)ImageBase + DosHeader->NewHeaderOffset + 4);
  OptHeader32 = (PE_OPTIONAL_HEADER32 *)(CoffHeader + 1);
  OptHeader64 = (PE_OPTIONAL_HEADER64 *)(CoffHeader + 1);

  Is64Bit = (OptHeader32->Magic == PE_OPT_MAGIC_PE32PLUS);
  Subsystem = Is64Bit ? OptHeader64->Subsystem : OptHeader32->Subsystem;

  // Check for BeOS (x86 version used PE format with subsystem 6)
  if (Subsystem == IMAGE_SUBSYSTEM_BEOS_GUI) {
    *TargetSystem = ImgSystemBeOs;
    return S_OK;
  }

  // PE format is primarily used by Windows NT family
  *TargetSystem = ImgSystemWindowsNt;
  return S_OK;
}

/**
  Get minimum required system version from PE image.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetMinimumSystemVersion (
  IN  IImageLoader            *This,
  IN  VOID                    *ImageBase,
  OUT IMGLOAD_SYSTEM_VERSION  *MinimumVersion
  )
{
  DOS_HEADER *DosHeader;
  PE_OPTIONAL_HEADER32 *OptHeader32;
  PE_OPTIONAL_HEADER64 *OptHeader64;
  COFF_HEADER *CoffHeader;
  BOOLEAN Is64Bit;

  if (MinimumVersion == NULL) {
    return E_POINTER;
  }

  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));

  DosHeader = (DOS_HEADER *)ImageBase;
  CoffHeader = (COFF_HEADER *)((UINT8 *)ImageBase + DosHeader->NewHeaderOffset + 4);
  OptHeader32 = (PE_OPTIONAL_HEADER32 *)(CoffHeader + 1);
  OptHeader64 = (PE_OPTIONAL_HEADER64 *)(CoffHeader + 1);

  Is64Bit = (OptHeader32->Magic == PE_OPT_MAGIC_PE32PLUS);

  if (Is64Bit) {
    MinimumVersion->Major = OptHeader64->MajorOperatingSystemVersion;
    MinimumVersion->Minor = OptHeader64->MinorOperatingSystemVersion;
  } else {
    MinimumVersion->Major = OptHeader32->MajorOperatingSystemVersion;
    MinimumVersion->Minor = OptHeader32->MinorOperatingSystemVersion;
  }

  return (MinimumVersion->Major > 0) ? S_OK : S_FALSE;
}

/**
  Get target subsystem from PE image.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetTargetSubsystem (
  IN  IImageLoader              *This,
  IN  VOID                      *ImageBase,
  OUT IMGLOAD_TARGET_SUBSYSTEM  *TargetSubsystem
  )
{
  DOS_HEADER *DosHeader;
  PE_OPTIONAL_HEADER32 *OptHeader32;
  PE_OPTIONAL_HEADER64 *OptHeader64;
  COFF_HEADER *CoffHeader;
  UINT16 Subsystem;
  BOOLEAN Is64Bit;

  if (TargetSubsystem == NULL) {
    return E_POINTER;
  }

  DosHeader = (DOS_HEADER *)ImageBase;
  CoffHeader = (COFF_HEADER *)((UINT8 *)ImageBase + DosHeader->NewHeaderOffset + 4);
  OptHeader32 = (PE_OPTIONAL_HEADER32 *)(CoffHeader + 1);
  OptHeader64 = (PE_OPTIONAL_HEADER64 *)(CoffHeader + 1);

  Is64Bit = (OptHeader32->Magic == PE_OPT_MAGIC_PE32PLUS);
  Subsystem = Is64Bit ? OptHeader64->Subsystem : OptHeader32->Subsystem;

  switch (Subsystem) {
    case IMAGE_SUBSYSTEM_NATIVE:
      *TargetSubsystem = ImgSubsystemNative;
      break;
    case IMAGE_SUBSYSTEM_WINDOWS_GUI:
      *TargetSubsystem = ImgSubsystemWindowsGui;
      break;
    case IMAGE_SUBSYSTEM_WINDOWS_CUI:
      *TargetSubsystem = ImgSubsystemWindowsCui;
      break;
    case IMAGE_SUBSYSTEM_OS2_GUI:
      *TargetSubsystem = ImgSubsystemOs2Gui;
      break;
    case IMAGE_SUBSYSTEM_OS2_CUI:
      *TargetSubsystem = ImgSubsystemOs2Cui;
      break;
    case IMAGE_SUBSYSTEM_BEOS_GUI:
      *TargetSubsystem = ImgSubsystemBeOsGui;
      break;
    case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
      *TargetSubsystem = ImgSubsystemWindowsCeGui;
      break;
    case IMAGE_SUBSYSTEM_EFI_APPLICATION:
      *TargetSubsystem = ImgSubsystemEfiApplication;
      break;
    case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
      *TargetSubsystem = ImgSubsystemEfiBootDriver;
      break;
    case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
      *TargetSubsystem = ImgSubsystemEfiRuntimeDriver;
      break;
    case IMAGE_SUBSYSTEM_EFI_ROM:
      *TargetSubsystem = ImgSubsystemEfiRom;
      break;
    case IMAGE_SUBSYSTEM_POSIX_CUI:
      *TargetSubsystem = ImgSubsystemPosix;
      break;
    case IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION:
      *TargetSubsystem = ImgSubsystemWindowsBootApp;
      break;
    case IMAGE_SUBSYSTEM_XBOX:
      *TargetSubsystem = ImgSubsystemXbox;
      break;
    default:
      *TargetSubsystem = ImgSubsystemUnknown;
      break;
  }

  return S_OK;
}

/**
  Get minimum required subsystem version from PE image.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetMinimumSubsystemVersion (
  IN  IImageLoader            *This,
  IN  VOID                    *ImageBase,
  OUT IMGLOAD_SYSTEM_VERSION  *MinimumVersion
  )
{
  DOS_HEADER *DosHeader;
  PE_OPTIONAL_HEADER32 *OptHeader32;
  PE_OPTIONAL_HEADER64 *OptHeader64;
  COFF_HEADER *CoffHeader;
  BOOLEAN Is64Bit;

  if (MinimumVersion == NULL) {
    return E_POINTER;
  }

  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));

  DosHeader = (DOS_HEADER *)ImageBase;
  CoffHeader = (COFF_HEADER *)((UINT8 *)ImageBase + DosHeader->NewHeaderOffset + 4);
  OptHeader32 = (PE_OPTIONAL_HEADER32 *)(CoffHeader + 1);
  OptHeader64 = (PE_OPTIONAL_HEADER64 *)(CoffHeader + 1);

  Is64Bit = (OptHeader32->Magic == PE_OPT_MAGIC_PE32PLUS);

  if (Is64Bit) {
    MinimumVersion->Major = OptHeader64->SubsystemMajorVersion;
    MinimumVersion->Minor = OptHeader64->SubsystemMinorVersion;
  } else {
    MinimumVersion->Major = OptHeader32->SubsystemMajorVersion;
    MinimumVersion->Minor = OptHeader32->SubsystemMinorVersion;
  }

  return (MinimumVersion->Major > 0) ? S_OK : S_FALSE;
}

/**
  Convert RVA to file pointer by searching section headers.
**/
static
VOID *
PeRvaToPointer (
  IN VOID    *ImageBase,
  IN UINT32  Rva
  )
{
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders;
  PE_SECTION_HEADER *Sections;
  UINT16 NumSections;
  UINT16 i;

  if (Rva == 0) {
    return NULL;
  }

  DosHeader = (DOS_HEADER *)ImageBase;
  NtHeaders = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  NumSections = NtHeaders->FileHeader.NumSections;

  Sections = (PE_SECTION_HEADER *)((UINT8 *)&NtHeaders->OptionalHeader +
                                   NtHeaders->FileHeader.OptionalHeaderSize);

  // Find section containing this RVA
  for (i = 0; i < NumSections; i++) {
    UINT32 SectionStart = Sections[i].VirtualAddress;
    UINT32 SectionEnd = SectionStart + Sections[i].VirtualSize;

    if (Rva >= SectionStart && Rva < SectionEnd) {
      // Calculate offset within section
      UINT32 SectionOffset = Rva - SectionStart;
      return PE_OFF(Sections[i].PointerToRawData + SectionOffset);
    }
  }

  // RVA not found in any section
  return NULL;
}

/**
  Search for resource in PE resource directory tree.

  PE resource directory has 3 levels:
  - Level 0: Resource Type (RT_BITMAP, RT_ICON, or custom types like AUR)
  - Level 1: Resource Name/ID
  - Level 2: Language

  @param[in]  ResourceDir    Pointer to resource directory root.
  @param[in]  TypeCode       Resource type to search for.
  @param[in]  Id             Resource ID (0 if using name).
  @param[in]  Name           Resource name (NULL if using ID).
  @param[out] Data           Receives pointer to resource data.
  @param[out] Size           Receives size of resource data.

  @return S_OK if found, S_FALSE if not found, error code otherwise.
**/
static
HRESULT
PeSearchResourceDirectory (
  IN  VOID         *ResourceDir,
  IN  UINT32       TypeCode,
  IN  UINT32       Id,
  IN  CONST CHAR8  *Name,
  OUT VOID         **Data,
  OUT UINT64       *Size
  )
{
  PE_RESOURCE_DIRECTORY *Dir;
  PE_RESOURCE_DIRECTORY_ENTRY *Entries;
  UINT32 NumEntries;
  UINT32 i;
  VOID *TypeDir;
  PE_RESOURCE_DIRECTORY *NameDir;
  PE_RESOURCE_DIRECTORY_ENTRY *NameEntries;
  UINT32 NameNumEntries;
  UINT32 j;
  VOID *LangDir;
  PE_RESOURCE_DIRECTORY *LangResDir;
  PE_RESOURCE_DIRECTORY_ENTRY *LangEntries;
  PE_RESOURCE_DATA_ENTRY *DataEntry;

  if (ResourceDir == NULL || Data == NULL || Size == NULL) {
    return E_POINTER;
  }

  // Level 0: Search for type
  Dir = (PE_RESOURCE_DIRECTORY *)ResourceDir;
  NumEntries = Dir->NumberOfNamedEntries + Dir->NumberOfIdEntries;
  Entries = (PE_RESOURCE_DIRECTORY_ENTRY *)((UINT8 *)Dir + sizeof(PE_RESOURCE_DIRECTORY));

  TypeDir = NULL;
  for (i = 0; i < NumEntries; i++) {
    UINT32 EntryName = Entries[i].Name;
    BOOLEAN IsNameEntry = !!(EntryName & 0x80000000);

    if (IsNameEntry) {
      // Named entry - check if Name parameter matches
      if (Name != NULL) {
        UINT32 NameOffset = EntryName & 0x7FFFFFFF;
        UINT16 *NameData = (UINT16 *)((UINT8 *)ResourceDir + NameOffset);
        UINT16 NameLength = NameData[0];
        CHAR8 *NameString = (CHAR8 *)&NameData[1];
        UINTN NameLen = strlen(Name);

        // Compare Unicode name with ASCII name (simple conversion)
        BOOLEAN Match = TRUE;
        if (NameLength != NameLen) {
          Match = FALSE;
        } else {
          for (UINT32 k = 0; k < NameLength; k++) {
            if ((CHAR8)NameString[k * 2] != Name[k]) {
              Match = FALSE;
              break;
            }
          }
        }

        if (Match) {
          TypeDir = (UINT8 *)ResourceDir + (Entries[i].OffsetToData & 0x7FFFFFFF);
          break;
        }
      }
    } else {
      // ID entry
      if (EntryName == TypeCode) {
        TypeDir = (UINT8 *)ResourceDir + (Entries[i].OffsetToData & 0x7FFFFFFF);
        break;
      }
    }
  }

  if (TypeDir == NULL) {
    return S_FALSE;  // Type not found
  }

  // Level 1: Get first name/ID entry (we don't filter by specific name/ID at this level)
  NameDir = (PE_RESOURCE_DIRECTORY *)TypeDir;
  NameNumEntries = NameDir->NumberOfNamedEntries + NameDir->NumberOfIdEntries;
  NameEntries = (PE_RESOURCE_DIRECTORY_ENTRY *)((UINT8 *)NameDir + sizeof(PE_RESOURCE_DIRECTORY));

  if (NameNumEntries == 0) {
    return S_FALSE;  // No names/IDs under this type
  }

  // Just take the first entry
  LangDir = (UINT8 *)ResourceDir + (NameEntries[0].OffsetToData & 0x7FFFFFFF);

  // Level 2: Get first language entry
  LangResDir = (PE_RESOURCE_DIRECTORY *)LangDir;
  LangEntries = (PE_RESOURCE_DIRECTORY_ENTRY *)((UINT8 *)LangResDir + sizeof(PE_RESOURCE_DIRECTORY));

  if ((LangResDir->NumberOfNamedEntries + LangResDir->NumberOfIdEntries) == 0) {
    return S_FALSE;  // No language entries
  }

  // Get data entry
  DataEntry = (PE_RESOURCE_DATA_ENTRY *)((UINT8 *)ResourceDir + (LangEntries[0].OffsetToData & 0x7FFFFFFF));

  *Data = (VOID *)(UINTN)DataEntry->OffsetToData;  // This is an RVA, caller must convert
  *Size = DataEntry->Size;

  return S_OK;
}

/**
  Find native resource in PE resource directory.

  Searches the PE resource tree for a specific resource by type and name/ID.

  @param[in]  ImageBase      Pointer to PE image.
  @param[in]  TypeCode       Resource type (4-char code or RT_* constant).
  @param[in]  Id             Resource ID (0 if using name).
  @param[in]  Name           Resource name (NULL if using ID).
  @param[out] Data           Receives pointer to resource data.
  @param[out] Size           Receives size of resource data.

  @return S_OK if found, S_FALSE if not found, error code otherwise.
**/
static
HRESULT
PeFindNativeResource (
  IN  VOID         *ImageBase,
  IN  UINT32       TypeCode,
  IN  UINT32       Id,
  IN  CONST CHAR8  *Name,
  OUT VOID         **Data,
  OUT UINT64       *Size
  )
{
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders32;
  PE_NT_HEADERS64 *NtHeaders64;
  PE_DATA_DIRECTORY *ResourceDir;
  VOID *ResourceDirBase;
  BOOLEAN Is64Bit;
  HRESULT Status;
  VOID *ResourceRva;
  UINT64 ResourceSize;

  if (ImageBase == NULL || Data == NULL || Size == NULL) {
    return E_POINTER;
  }

  DosHeader = (DOS_HEADER *)ImageBase;
  NtHeaders32 = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  NtHeaders64 = (PE_NT_HEADERS64 *)NtHeaders32;

  Is64Bit = (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32PLUS);

  // Get resource data directory
  ResourceDir = Is64Bit ?
    &NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE] :
    &NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];

  if (ResourceDir->VirtualAddress == 0 || ResourceDir->Size == 0) {
    return S_FALSE;  // No resources
  }

  ResourceDirBase = PeRvaToPointer(ImageBase, ResourceDir->VirtualAddress);
  if (ResourceDirBase == NULL) {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  // Search resource directory
  Status = PeSearchResourceDirectory(
    ResourceDirBase,
    TypeCode,
    Id,
    Name,
    &ResourceRva,
    &ResourceSize
  );

  if (FAILED(Status) || Status == S_FALSE) {
    return Status;
  }

  // Convert RVA to pointer
  *Data = PeRvaToPointer(ImageBase, (UINT32)(UINTN)ResourceRva);
  *Size = ResourceSize;

  if (*Data == NULL) {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  return S_OK;
}

/**
  Find section by name in PE image.

  @param[in]  ImageBase      Pointer to PE image.
  @param[in]  SectionName    Name of section to find (e.g., ".axursrc").
  @param[out] Data           Receives pointer to section data.
  @param[out] Size           Receives size of section.

  @return S_OK if found, S_FALSE if not found, error code otherwise.
**/
static
HRESULT
PeFindSection (
  IN  VOID         *ImageBase,
  IN  CONST CHAR8  *SectionName,
  OUT VOID         **Data,
  OUT UINT64       *Size
  )
{
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders;
  PE_SECTION_HEADER *Sections;
  UINT16 NumSections;
  UINT16 i;
  UINTN NameLen;

  if (ImageBase == NULL || SectionName == NULL || Data == NULL || Size == NULL) {
    return E_POINTER;
  }

  DosHeader = (DOS_HEADER *)ImageBase;
  NtHeaders = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  NumSections = NtHeaders->FileHeader.NumSections;

  Sections = (PE_SECTION_HEADER *)((UINT8 *)&NtHeaders->OptionalHeader +
                                   NtHeaders->FileHeader.OptionalHeaderSize);

  NameLen = strlen(SectionName);
  if (NameLen > 8) {
    NameLen = 8;  // Section names are max 8 characters
  }

  // Search for section
  for (i = 0; i < NumSections; i++) {
    if (memcmp(Sections[i].Name, SectionName, NameLen) == 0) {
      *Data = PE_OFF(Sections[i].PointerToRawData);
      *Size = Sections[i].SizeOfRawData;
      return S_OK;
    }
  }

  return S_FALSE;  // Section not found
}

/**
  Get resource from PE image.

  Hybrid strategy: Combines universal resources (from AUR or .axursrc)
  with native PE resources. Tries universal fork first, then native.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetResource (
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
  UINT32 PeTypeId;
  UINT16 Id;
  CONST CHAR8 *Name;
  VOID *NativeData;
  UINT64 NativeSize;

  if (ImageBase == NULL || Resource == NULL) {
    return E_POINTER;
  }

  *Resource = NULL;

  // Extract type code from ResourceType
  if (ResourceType != NULL) {
    if (ResourceType->IsNumeric) {
      TypeCode = ResourceType->Id;
      PeTypeId = ResourceType->Id;
    } else {
      // Convert name to 4-char type code (take first 4 chars)
      TypeCode = ANX_MAKE_TYPE(ResourceType->Name);
      PeTypeId = 0;  // Use name for PE lookup
    }
  } else {
    TypeCode = 0;  // All types
    PeTypeId = 0;
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

  // First, try to get universal resource fork
  Status = FindUniversalResourceFork(
    ImageBase,
    ResourceStrategyBoth,
    PeFindNativeResource,
    PeFindSection,
    ".axursrc",
    &ResourceFork,
    &Size,
    &NeedsFree
  );

  if (Status == S_OK) {
    // Try to find resource in universal fork
    Status = CreateImageResource(
      ResourceFork,
      TypeCode,
      Id,
      Name,
      Resource
    );

    if (Status == S_OK) {
      // Found in universal fork
      if (NeedsFree && ResourceFork != NULL) {
        free(ResourceFork);
      }
      return S_OK;
    }

    // Not found in universal fork, will try native PE resources
    if (NeedsFree && ResourceFork != NULL) {
      free(ResourceFork);
    }
  }

  // Try native PE resources (excluding AUR types which are for universal fork)
  if (PeTypeId != ANX_RSRC_TYPE_AUR &&
      PeTypeId != ANX_RSRC_TYPE_AUR_16BIT &&
      PeTypeId != ANX_RSRC_ID_AUR_32BIT) {

    Status = PeFindNativeResource(
      ImageBase,
      PeTypeId,
      Id,
      (ResourceType != NULL && !ResourceType->IsNumeric) ? ResourceType->Name : Name,
      &NativeData,
      &NativeSize
    );

    if (Status == S_OK) {
      // Found in native resources - wrap it as IImageResource
      Status = CreateNativeImageResource(
        TypeCode,
        Id,
        (ResourceType != NULL && !ResourceType->IsNumeric) ? ResourceType->Name : Name,
        NativeData,
        NativeSize,
        Resource
      );
      return Status;
    }
  }

  return S_FALSE;  // Not found in either source
}

/**
  Get initialization and termination functions from PE image.

  PE uses TLS callbacks as initialization functions. These are executed
  before the entry point. PE does not have standard termination functions.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetInitFini (
  IN  IImageLoader          *This,
  IN  VOID                  *ImageBase,
  OUT IMGLOAD_INITFINI_INFO *InitFiniInfo
  )
{
  DOS_HEADER *DosHeader;
  PE_NT_HEADERS32 *NtHeaders32;
  PE_NT_HEADERS64 *NtHeaders64;
  PE_DATA_DIRECTORY *TlsDir;
  BOOLEAN Is64Bit;
  UINT64 ImageBaseVA;

  if (InitFiniInfo == NULL) {
    return E_POINTER;
  }

  memset(InitFiniInfo, 0, sizeof(IMGLOAD_INITFINI_INFO));

  DosHeader = (DOS_HEADER *)ImageBase;
  NtHeaders32 = (PE_NT_HEADERS32 *)PE_OFF(DosHeader->NewHeaderOffset);
  NtHeaders64 = (PE_NT_HEADERS64 *)NtHeaders32;

  Is64Bit = (NtHeaders32->OptionalHeader.Magic == PE_OPT_MAGIC_PE32PLUS);

  // Get image base for converting absolute addresses
  ImageBaseVA = Is64Bit ?
    NtHeaders64->OptionalHeader.ImageBase :
    (UINT64)NtHeaders32->OptionalHeader.ImageBase;

  // Get TLS data directory
  TlsDir = Is64Bit ?
    &NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS] :
    &NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];

  if (TlsDir->VirtualAddress == 0 || TlsDir->Size == 0) {
    // No TLS callbacks
    return S_FALSE;
  }

  // Get TLS directory and extract callback array address
  if (Is64Bit) {
    PE_TLS_DIRECTORY64 *TlsDirectory = (PE_TLS_DIRECTORY64 *)PeRvaToPointer(ImageBase, TlsDir->VirtualAddress);

    if (TlsDirectory == NULL) {
      return S_FALSE;
    }

    if (TlsDirectory->AddressOfCallBacks != 0) {
      // TLS callbacks exist - use the callback array address as init
      // The array itself is a NULL-terminated array of function pointers
      InitFiniInfo->InitAddress = TlsDirectory->AddressOfCallBacks;
      InitFiniInfo->HasInit = TRUE;
    }
  } else {
    PE_TLS_DIRECTORY32 *TlsDirectory = (PE_TLS_DIRECTORY32 *)PeRvaToPointer(ImageBase, TlsDir->VirtualAddress);

    if (TlsDirectory == NULL) {
      return S_FALSE;
    }

    if (TlsDirectory->AddressOfCallBacks != 0) {
      // TLS callbacks exist - use the callback array address as init
      InitFiniInfo->InitAddress = TlsDirectory->AddressOfCallBacks;
      InitFiniInfo->HasInit = TRUE;
    }
  }

  // PE doesn't have standard termination functions
  InitFiniInfo->HasFini = FALSE;
  InitFiniInfo->FiniAddress = 0;
  InitFiniInfo->Priority = 0;

  return InitFiniInfo->HasInit ? S_OK : S_FALSE;
}

/**
  Get resource enumerator for PE image.

  Enumerates all resources of a given type from the universal resource fork.
**/
static
HRESULT
STDMETHODCALLTYPE
PeGetResourceEnumerator (
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
      // Convert name to 4-char type code
      TypeCode = ANX_MAKE_TYPE(ResourceType->Name);
    }
  } else {
    TypeCode = 0;  // All types
  }

  // Use FindUniversalResourceFork with hybrid strategy
  Status = FindUniversalResourceFork(
    ImageBase,
    ResourceStrategyBoth,
    PeFindNativeResource,
    PeFindSection,
    ".axursrc",
    &ResourceFork,
    &Size,
    &NeedsFree
  );

  if (FAILED(Status) || Status == S_FALSE) {
    return Status;
  }

  // Create enumerator
  Status = CreateImageResourceEnumerator(
    ResourceFork,
    TypeCode,
    Enumerator
  );

  if (NeedsFree && ResourceFork != NULL) {
    free(ResourceFork);
  }

  return Status;
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
  PeApplyRelocations,
  PeGetTargetSystem,
  PeGetMinimumSystemVersion,
  PeGetTargetSubsystem,
  PeGetMinimumSubsystemVersion,
  PeGetResource,
  PeGetResourceEnumerator,
  PeGetInitFini
};

//
// PE Loader Instance
//

IImageLoader gPeLoader = {
  &gPeVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gPeLoader);
