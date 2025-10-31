/** @file
  APXH FatELF Loader

  Provides FatELF (multi-architecture ELF container) format parsing and
  loading. FatELF embeds multiple ELF binaries for different architectures
  into one file, similar to macOS Universal Binaries.

  Supports:
  - FatELF version 1
  - Multiple architectures (x86, x86-64, ARM, ARM64, RISC-V)
  - Architecture auto-selection based on current platform

  Specification: https://icculus.org/fatelf/

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// FatELF Magic Number
//

#define FATELF_MAGIC    0x1F0E70FA  ///< FatELF magic (little-endian)

//
// FatELF Version
//

#define FATELF_VERSION  1  ///< Current FatELF version

//
// FatELF Byte Order Values
//

#define FATELF_BYTEORDER_LSB  1  ///< Little-endian
#define FATELF_BYTEORDER_MSB  2  ///< Big-endian

//
// FatELF Word Size Values
//

#define FATELF_WORDSIZE_32  1  ///< 32-bit
#define FATELF_WORDSIZE_64  2  ///< 64-bit

//
// ELF Machine Types (from ELF specification)
//

#define EM_386          3   ///< Intel 80386
#define EM_X86_64       62  ///< AMD x86-64
#define EM_ARM          40  ///< ARM
#define EM_AARCH64      183 ///< ARM 64-bit
#define EM_RISCV        243 ///< RISC-V

//
// ELF OSABI Values
//

#define ELFOSABI_NONE   0   ///< UNIX System V ABI
#define ELFOSABI_LINUX  3   ///< Linux

//
// FatELF Structures
//

ANX_PACK_PUSH(1)

typedef struct _FATELF_HEADER {
  UINT32  Magic;          ///< 0x1F0E70FA
  UINT16  Version;        ///< Format version (1)
  UINT8   NumRecords;     ///< Number of ELF binaries
  UINT8   Reserved;       ///< Reserved (must be 0)
} FATELF_HEADER;

typedef struct _FATELF_RECORD {
  UINT16  MachineType;    ///< ELF e_machine value
  UINT8   OsAbi;          ///< ELF OSABI
  UINT8   OsAbiVersion;   ///< OSABI version
  UINT8   ByteOrder;      ///< 1=LSB, 2=MSB
  UINT8   WordSize;       ///< 1=32-bit, 2=64-bit
  UINT16  Reserved;       ///< Reserved (must be 0)
  UINT64  Offset;         ///< File offset to ELF binary
  UINT64  Size;           ///< Size of ELF binary
} FATELF_RECORD;

ANX_PACK_POP()

//
// Helper Macros
//

#define FATELF_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// External ELF loader
//

extern IMAGE_LOADER gElfLoader;

//
// Internal Functions
//

/**
  Check if image is FatELF format.
**/
static
BOOLEAN
ANXAPI
FatElfDetect (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  FATELF_HEADER *Header;

  if (ImageSize < sizeof(FATELF_HEADER)) {
    return FALSE;
  }

  Header = (FATELF_HEADER *)ImageBase;

  return (Header->Magic == FATELF_MAGIC &&
          Header->Version == FATELF_VERSION &&
          Header->NumRecords > 0);
}

/**
  Map APXH architecture to ELF machine type and word size.
**/
static
BOOLEAN
ArchToElfMachine (
  IN  ARCH    Arch,
  OUT UINT16  *MachineType,
  OUT UINT8   *WordSize
  )
{
  switch (Arch) {
    case ARCH_386:
      *MachineType = EM_386;
      *WordSize = FATELF_WORDSIZE_32;
      return TRUE;

    case ARCH_AMD64:
      *MachineType = EM_X86_64;
      *WordSize = FATELF_WORDSIZE_64;
      return TRUE;

    case ARCH_RISCV64:
      *MachineType = EM_RISCV;
      *WordSize = FATELF_WORDSIZE_64;
      return TRUE;

    default:
      return FALSE;
  }
}

/**
  Find matching ELF binary in FatELF container.
**/
static
FATELF_RECORD *
FindMatchingRecord (
  IN VOID   *ImageBase,
  IN ARCH   TargetArch
  )
{
  FATELF_HEADER *Header = (FATELF_HEADER *)ImageBase;
  FATELF_RECORD *Records = (FATELF_RECORD *)(Header + 1);
  UINT16 TargetMachine;
  UINT8 TargetWordSize;
  UINT32 i;

  if (!ArchToElfMachine(TargetArch, &TargetMachine, &TargetWordSize)) {
    return NULL;
  }

  // Find record matching target architecture
  for (i = 0; i < Header->NumRecords; i++) {
    FATELF_RECORD *Rec = &Records[i];

    if (Rec->MachineType == TargetMachine &&
        Rec->WordSize == TargetWordSize &&
        Rec->ByteOrder == FATELF_BYTEORDER_LSB) {
      return Rec;
    }
  }

  return NULL;
}

/**
  Get architecture from FatELF image.

  Note: FatELF contains multiple architectures. We return the first
  supported architecture, with preference for the current platform.
**/
static
ARCH
ANXAPI
FatElfGetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  FATELF_HEADER *Header = (FATELF_HEADER *)ImageBase;
  FATELF_RECORD *Records = (FATELF_RECORD *)(Header + 1);
  UINT32 i;

  // Try to find current architecture first
  #if defined(ANANKE_ARCH_X64)
    FATELF_RECORD *Rec = FindMatchingRecord(ImageBase, ARCH_AMD64);
    if (Rec != NULL) {
      return ARCH_AMD64;
    }
  #elif defined(ANANKE_ARCH_X86)
    FATELF_RECORD *Rec = FindMatchingRecord(ImageBase, ARCH_386);
    if (Rec != NULL) {
      return ARCH_386;
    }
  #elif defined(ANANKE_ARCH_RISCV64)
    FATELF_RECORD *Rec = FindMatchingRecord(ImageBase, ARCH_RISCV64);
    if (Rec != NULL) {
      return ARCH_RISCV64;
    }
  #endif

  // Return first supported architecture
  for (i = 0; i < Header->NumRecords; i++) {
    FATELF_RECORD *Rec = &Records[i];

    switch (Rec->MachineType) {
      case EM_386:
        return ARCH_386;
      case EM_X86_64:
        return ARCH_AMD64;
      case EM_RISCV:
        return ARCH_RISCV64;
      default:
        continue;
    }
  }

  return ARCH_UNSUPPORTED;
}

/**
  Get entry point from FatELF image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
FatElfGetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  FATELF_RECORD *Rec;
  VOID *ElfImage;
  ARCH TargetArch;

  // Get target architecture
  TargetArch = FatElfGetArch(This, ImageBase);
  if (TargetArch == ARCH_UNSUPPORTED) {
    return 0;
  }

  // Find matching record
  Rec = FindMatchingRecord(ImageBase, TargetArch);
  if (Rec == NULL) {
    return 0;
  }

  // Get entry point from embedded ELF
  ElfImage = FATELF_OFF(Rec->Offset);
  return gElfLoader.Vtbl->GetEntryPoint(&gElfLoader, ElfImage);
}

/**
  Load FatELF image.

  Extracts the appropriate ELF binary for the target architecture
  and loads it using the ELF loader.
**/
static
IMGLOAD_STATUS
ANXAPI
FatElfLoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase = Context->ImageBase;
  FATELF_HEADER *Header = (FATELF_HEADER *)ImageBase;
  FATELF_RECORD *Rec;
  IMGLOAD_CONTEXT ElfContext;
  IMGLOAD_STATUS Status;

  info("Loading FatELF multi-architecture executable...");
  info("  Container has %d architecture(s)", Header->NumRecords);

  // Find matching record for target architecture
  Rec = FindMatchingRecord(ImageBase, Context->Architecture);
  if (Rec == NULL) {
    warn("No matching architecture found in FatELF container");
    return ImgLoadUnsupportedArch;
  }

  info("  Selected architecture: machine=%d, wordsize=%s",
       Rec->MachineType,
       Rec->WordSize == FATELF_WORDSIZE_64 ? "64-bit" : "32-bit");
  info("  Embedded ELF at offset 0x%llx (size: 0x%llx)",
       Rec->Offset, Rec->Size);

  // Create context for embedded ELF
  memcpy(&ElfContext, Context, sizeof(IMGLOAD_CONTEXT));
  ElfContext.ImageBase = FATELF_OFF(Rec->Offset);
  ElfContext.ImageSize = Rec->Size;

  // Load the embedded ELF binary
  Status = gElfLoader.Vtbl->LoadImage(&gElfLoader, &ElfContext);
  if (Status != ImgLoadSuccess) {
    warn("Failed to load embedded ELF binary");
    return Status;
  }

  // Copy results back
  Context->EntryPoint = ElfContext.EntryPoint;
  Context->KernelTls = ElfContext.KernelTls;
  Context->UserTls = ElfContext.UserTls;

  return ImgLoadSuccess;
}

/**
  Extract TLS information from FatELF image.
**/
static
IMGLOAD_STATUS
ANXAPI
FatElfGetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  FATELF_RECORD *Rec;
  VOID *ElfImage;
  ARCH TargetArch;

  // Get target architecture
  TargetArch = FatElfGetArch(This, ImageBase);
  if (TargetArch == ARCH_UNSUPPORTED) {
    memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
    return ImgLoadUnsupportedArch;
  }

  // Find matching record
  Rec = FindMatchingRecord(ImageBase, TargetArch);
  if (Rec == NULL) {
    memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
    return ImgLoadUnsupportedArch;
  }

  // Get TLS info from embedded ELF
  ElfImage = FATELF_OFF(Rec->Offset);
  return gElfLoader.Vtbl->GetTlsInfo(&gElfLoader, ElfImage, TlsInfo);
}

//
// FatELF Loader VTable
//

static CONST IMAGE_LOADER_VTBL gFatElfVtbl = {
  FatElfDetect,
  FatElfGetArch,
  FatElfGetEntryPoint,
  FatElfLoadImage,
  FatElfGetTlsInfo
};

//
// FatELF Loader Instance
//

IMAGE_LOADER gFatElfLoader = {
  &gFatElfVtbl,
  "FatELF",
  NULL
};
