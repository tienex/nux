/** @file
  APXH TE (Terse Executable) Loader

  Provides TE (Terse Executable) format parsing and loading for UEFI
  firmware modules. TE is a compressed version of PE/COFF designed for
  space-constrained UEFI environments, primarily used for PEIMs and DXE
  drivers in the early boot phases.

  Supports:
  - TE format (UEFI Terse Executable)
  - x86, x64, ARM, AArch64, RISC-V, LoongArch architectures
  - Stripped PE headers with minimal overhead
  - Relocation fixups

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// TE Image Signature
//

#define TE_IMAGE_SIGNATURE      0x5A56  ///< "VZ" (little-endian)

//
// TE Image Machine Types (same as PE)
//

#define TE_IMAGE_MACHINE_I386         0x014C  ///< Intel 386
#define TE_IMAGE_MACHINE_X64          0x8664  ///< x64 (AMD64/EM64T)
#define TE_IMAGE_MACHINE_ARM          0x01C0  ///< ARM little-endian
#define TE_IMAGE_MACHINE_THUMB        0x01C2  ///< ARM Thumb/Thumb-2
#define TE_IMAGE_MACHINE_ARMNT        0x01C4  ///< ARM Thumb-2 little-endian
#define TE_IMAGE_MACHINE_AARCH64      0xAA64  ///< ARM64 little-endian
#define TE_IMAGE_MACHINE_RISCV32      0x5032  ///< RISC-V 32-bit
#define TE_IMAGE_MACHINE_RISCV64      0x5064  ///< RISC-V 64-bit
#define TE_IMAGE_MACHINE_RISCV128     0x5128  ///< RISC-V 128-bit
#define TE_IMAGE_MACHINE_LOONGARCH32  0x6232  ///< LoongArch 32-bit
#define TE_IMAGE_MACHINE_LOONGARCH64  0x6264  ///< LoongArch 64-bit

//
// TE Image Subsystem
//

#define TE_IMAGE_SUBSYSTEM_EFI_APPLICATION        10
#define TE_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER 11
#define TE_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER     12
#define TE_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER     13

//
// TE Image Data Directory Indices
//

#define TE_IMAGE_DIRECTORY_ENTRY_BASERELOC  0  ///< Base relocation table
#define TE_IMAGE_DIRECTORY_ENTRY_DEBUG      1  ///< Debug directory

//
// TE Image Header Structure
//

ANX_PACK_PUSH(1)

typedef struct _TE_IMAGE_DATA_DIRECTORY {
  UINT32  VirtualAddress;   ///< RVA of the data
  UINT32  Size;             ///< Size of the data
} TE_IMAGE_DATA_DIRECTORY;

typedef struct _TE_IMAGE_HEADER {
  UINT16  Signature;                    ///< TE signature (0x5A56 "VZ")
  UINT16  Machine;                      ///< Machine type
  UINT8   NumberOfSections;             ///< Number of sections
  UINT8   Subsystem;                    ///< Subsystem type
  UINT16  StrippedSize;                 ///< Number of bytes stripped from PE header
  UINT32  AddressOfEntryPoint;          ///< Entry point RVA
  UINT32  BaseOfCode;                   ///< Base of code RVA
  UINT64  ImageBase;                    ///< Image base address
  TE_IMAGE_DATA_DIRECTORY DataDirectory[2];  ///< Base reloc and debug
} TE_IMAGE_HEADER;

typedef struct _TE_IMAGE_SECTION_HEADER {
  UINT8   Name[8];                      ///< Section name
  UINT32  VirtualSize;                  ///< Virtual size
  UINT32  VirtualAddress;               ///< Virtual address (RVA)
  UINT32  SizeOfRawData;                ///< Size of raw data
  UINT32  PointerToRawData;             ///< Pointer to raw data
  UINT32  PointerToRelocations;         ///< Pointer to relocations
  UINT32  PointerToLinenumbers;         ///< Pointer to line numbers
  UINT16  NumberOfRelocations;          ///< Number of relocations
  UINT16  NumberOfLinenumbers;          ///< Number of line numbers
  UINT32  Characteristics;              ///< Section characteristics
} TE_IMAGE_SECTION_HEADER;

typedef struct _TE_IMAGE_BASE_RELOCATION {
  UINT32  VirtualAddress;               ///< Page RVA
  UINT32  SizeOfBlock;                  ///< Size of relocation block
} TE_IMAGE_BASE_RELOCATION;

ANX_PACK_POP()

//
// TE Image Section Characteristics
//

#define TE_IMAGE_SCN_CNT_CODE               0x00000020  ///< Code section
#define TE_IMAGE_SCN_CNT_INITIALIZED_DATA   0x00000040  ///< Initialized data
#define TE_IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080  ///< Uninitialized data
#define TE_IMAGE_SCN_MEM_EXECUTE            0x20000000  ///< Executable
#define TE_IMAGE_SCN_MEM_READ               0x40000000  ///< Readable
#define TE_IMAGE_SCN_MEM_WRITE              0x80000000  ///< Writable

//
// TE Image Base Relocation Types
//

#define TE_IMAGE_REL_BASED_ABSOLUTE       0   ///< No relocation
#define TE_IMAGE_REL_BASED_HIGH           1   ///< High 16 bits
#define TE_IMAGE_REL_BASED_LOW            2   ///< Low 16 bits
#define TE_IMAGE_REL_BASED_HIGHLOW        3   ///< 32-bit relocation
#define TE_IMAGE_REL_BASED_HIGHADJ        4   ///< High 16 bits adjusted
#define TE_IMAGE_REL_BASED_DIR64          10  ///< 64-bit relocation

//
// Helper Macros
//

#define TE_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))
#define TE_SIZEOF_HEADERS (sizeof(TE_IMAGE_HEADER))

//
// Internal Functions
//

/**
  IUnknown: QueryInterface
**/
static
HRESULT
STDMETHODCALLTYPE
TeQueryInterface (
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
TeAddRef (
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
TeRelease (
  IN IImageLoader  *This
  )
{
  return 1;  // Static instance
}

/**
  Check if image is TE format.
**/
static
HRESULT
STDMETHODCALLTYPE
TeDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  TE_IMAGE_HEADER *Header;

  if (ImageSize < sizeof(TE_IMAGE_HEADER)) {
    return S_FALSE;
  }

  Header = (TE_IMAGE_HEADER *)ImageBase;

  return (Header->Signature == TE_IMAGE_SIGNATURE) ? S_OK : S_FALSE;
}

/**
  Get architecture from TE image.
**/
static
HRESULT
STDMETHODCALLTYPE
TeGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  TE_IMAGE_HEADER *Header;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  Header = (TE_IMAGE_HEADER *)ImageBase;

  switch (Header->Machine) {
    case TE_IMAGE_MACHINE_I386:
      *Architecture = Arch386;
      return S_OK;

    case TE_IMAGE_MACHINE_X64:
      *Architecture = ArchAmd64;
      return S_OK;

    case TE_IMAGE_MACHINE_ARM:
    case TE_IMAGE_MACHINE_THUMB:
    case TE_IMAGE_MACHINE_ARMNT:
      *Architecture = ArchArm;
      return S_OK;

    case TE_IMAGE_MACHINE_AARCH64:
      *Architecture = ArchArm64;
      return S_OK;

    case TE_IMAGE_MACHINE_RISCV32:
      *Architecture = ArchRiscV32;
      return S_OK;

    case TE_IMAGE_MACHINE_RISCV64:
      *Architecture = ArchRiscV64;
      return S_OK;

    case TE_IMAGE_MACHINE_RISCV128:
      *Architecture = ArchRiscV128;
      return S_OK;

    case TE_IMAGE_MACHINE_LOONGARCH32:
      *Architecture = ArchLoongArch32;
      return S_OK;

    case TE_IMAGE_MACHINE_LOONGARCH64:
      *Architecture = ArchLoongArch64;
      return S_OK;

    default:
      *Architecture = ArchUnsupported;
      return IMGLOAD_E_UNSUPPORTED_ARCH;
  }
}

/**
  Get endianness from TE image.
**/
static
HRESULT
STDMETHODCALLTYPE
TeGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // TE is little-endian (same as PE/COFF)
  *Endianness = ImgEndianLittle;
  return S_OK;
}

/**
  Get entry point from TE image.
**/
static
HRESULT
STDMETHODCALLTYPE
TeGetEntryPoint (
  IN  IImageLoader     *This,
  IN  VOID             *ImageBase,
  OUT VIRTUAL_ADDRESS  *EntryPoint
  )
{
  TE_IMAGE_HEADER *Header;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  Header = (TE_IMAGE_HEADER *)ImageBase;

  // TE entry point is RVA relative to image base, adjusted for stripped header
  *EntryPoint = Header->ImageBase +
                (Header->AddressOfEntryPoint - Header->StrippedSize);
  return S_OK;
}

/**
  Load TE image.
**/
static
HRESULT
STDMETHODCALLTYPE
TeLoadImage (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  TE_IMAGE_HEADER *Header;
  TE_IMAGE_SECTION_HEADER *Sections;
  UINT32 i;
  HRESULT Hr;
  UINT32 Adjustment;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  Header = (TE_IMAGE_HEADER *)ImageBase;

  info("Loading TE (UEFI Terse Executable) image...");
  info("  Machine: 0x%04x, Subsystem: %d, Stripped: %d bytes",
       Header->Machine, Header->Subsystem, Header->StrippedSize);

  // Calculate adjustment for RVA to file offset conversion
  // TE removes part of PE header, so RVAs need adjustment
  Adjustment = sizeof(TE_IMAGE_HEADER) - Header->StrippedSize;

  // Section headers follow immediately after TE header
  Sections = (TE_IMAGE_SECTION_HEADER *)((UINT8 *)ImageBase + sizeof(TE_IMAGE_HEADER));

  // Load each section
  for (i = 0; i < Header->NumberOfSections; i++) {
    TE_IMAGE_SECTION_HEADER *Section = &Sections[i];
    VIRTUAL_ADDRESS SectionVa;
    BOOLEAN IsWritable;
    BOOLEAN IsExecutable;

    if (Section->SizeOfRawData == 0) {
      continue;  // Skip empty sections
    }

    SectionVa = Header->ImageBase + Section->VirtualAddress;
    IsWritable = !!(Section->Characteristics & TE_IMAGE_SCN_MEM_WRITE);
    IsExecutable = !!(Section->Characteristics & TE_IMAGE_SCN_MEM_EXECUTE);

    info("  Section %.8s at 0x%016llx (size: 0x%08x, flags: 0x%08x)",
         Section->Name, SectionVa, Section->SizeOfRawData, Section->Characteristics);

    // Copy section data
    VirtualAddressCopy(
      SectionVa,
      TE_OFF(Section->PointerToRawData + Adjustment),
      Section->SizeOfRawData,
      Context->IsUserMode,
      IsWritable,
      IsExecutable
    );

    // Zero-fill remainder if VirtualSize > SizeOfRawData
    if (Section->VirtualSize > Section->SizeOfRawData) {
      UINT32 ZeroSize = Section->VirtualSize - Section->SizeOfRawData;

      VirtualAddressMemset(
        SectionVa + Section->SizeOfRawData,
        0,
        ZeroSize,
        Context->IsUserMode,
        IsWritable,
        IsExecutable
      );
    }
  }

  Hr = TeGetEntryPoint(&gTeLoader, ImageBase, &Context->EntryPoint);
  if (FAILED(Hr)) {
    return Hr;
  }

  return S_OK;
}

/**
  Extract TLS information from TE image.
**/
static
HRESULT
STDMETHODCALLTYPE
TeGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // TE format doesn't include TLS directory (stripped from PE)
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from TE image.
**/
static
HRESULT
STDMETHODCALLTYPE
TeGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  // TE format doesn't include exception directory (stripped from PE)
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
TeGetSymbolByAddress (
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

  // TE format doesn't include export directory (stripped from PE)
  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
TeGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (SymbolInfo == NULL || Name == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  // TE format doesn't include export directory (stripped from PE)
  return S_FALSE;
}

/**
  Extract relocation information from TE image.
**/
static
HRESULT
STDMETHODCALLTYPE
TeGetRelocInfo (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  OUT IMGLOAD_RELOC_INFO  *RelocInfo
  )
{
  TE_IMAGE_HEADER *Header;

  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  Header = (TE_IMAGE_HEADER *)ImageBase;

  if (Header->DataDirectory[TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > 0) {
    RelocInfo->PreferredBase = Header->ImageBase;
    RelocInfo->RelocTableAddr = Header->DataDirectory[TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
    RelocInfo->RelocTableSize = Header->DataDirectory[TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
    RelocInfo->Format = ImgRelocFormatPe;
    RelocInfo->RequiresReloc = TRUE;
    return S_OK;
  }

  return S_FALSE;
}

/**
  Apply relocations to TE image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
TeApplyRelocations (
  IN IImageLoader     *This,
  IN VOID             *ImageBase,
  IN VIRTUAL_ADDRESS  LoadAddress,
  IN VIRTUAL_ADDRESS  PreferredBase
  )
{
  TE_IMAGE_HEADER *Header;
  TE_IMAGE_BASE_RELOCATION *RelocBase;
  INT64 Delta;
  UINT32 RelocSize;
  UINT8 *RelocEnd;
  UINT32 Adjustment;

  Header = (TE_IMAGE_HEADER *)ImageBase;

  // Calculate delta
  Delta = (INT64)LoadAddress - (INT64)PreferredBase;
  if (Delta == 0) {
    return S_OK;  // No relocation needed
  }

  RelocSize = Header->DataDirectory[TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
  if (RelocSize == 0) {
    return S_OK;  // No relocations
  }

  // Calculate adjustment for RVA to file offset
  Adjustment = sizeof(TE_IMAGE_HEADER) - Header->StrippedSize;

  // Get relocation table
  RelocBase = (TE_IMAGE_BASE_RELOCATION *)TE_OFF(
    Header->DataDirectory[TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + Adjustment
  );
  RelocEnd = (UINT8 *)RelocBase + RelocSize;

  info("  Applying TE relocations (delta: 0x%llx)...", Delta);

  // Process relocation blocks
  while ((UINT8 *)RelocBase < RelocEnd && RelocBase->SizeOfBlock > 0) {
    UINT16 *RelocEntry = (UINT16 *)((UINT8 *)RelocBase + sizeof(TE_IMAGE_BASE_RELOCATION));
    UINT32 NumEntries = (RelocBase->SizeOfBlock - sizeof(TE_IMAGE_BASE_RELOCATION)) / sizeof(UINT16);
    UINT32 i;

    for (i = 0; i < NumEntries; i++) {
      UINT16 Type = RelocEntry[i] >> 12;
      UINT16 Offset = RelocEntry[i] & 0x0FFF;
      UINT8 *Target = (UINT8 *)ImageBase + RelocBase->VirtualAddress + Offset + Adjustment;

      switch (Type) {
        case TE_IMAGE_REL_BASED_ABSOLUTE:
          // Skip, used for padding
          break;

        case TE_IMAGE_REL_BASED_HIGHLOW:
          // 32-bit relocation
          *(UINT32 *)Target += (UINT32)Delta;
          break;

        case TE_IMAGE_REL_BASED_DIR64:
          // 64-bit relocation
          *(UINT64 *)Target += Delta;
          break;

        case TE_IMAGE_REL_BASED_HIGH:
          // High 16 bits
          *(UINT16 *)Target += (UINT16)((Delta >> 16) & 0xFFFF);
          break;

        case TE_IMAGE_REL_BASED_LOW:
          // Low 16 bits
          *(UINT16 *)Target += (UINT16)(Delta & 0xFFFF);
          break;

        default:
          warn("Unknown TE relocation type %d", Type);
          break;
      }
    }

    // Move to next relocation block
    RelocBase = (TE_IMAGE_BASE_RELOCATION *)((UINT8 *)RelocBase + RelocBase->SizeOfBlock);
  }

  return S_OK;
}

//
// TE Loader VTable
//

static CONST IImageLoaderVtbl gTeVtbl = {
  // IUnknown
  TeQueryInterface,
  TeAddRef,
  TeRelease,
  // IImageLoader
  TeDetect,
  TeGetArch,
  TeGetEndianness,
  TeGetEntryPoint,
  TeLoadImage,
  TeGetTlsInfo,
  TeGetUnwindInfo,
  TeGetSymbolByAddress,
  TeGetSymbolByName,
  TeGetRelocInfo,
  TeApplyRelocations
};

//
// TE Loader Instance
//

IImageLoader gTeLoader = {
  &gTeVtbl
};

APXH_REGISTER_IMGLOADER(gTeLoader);
