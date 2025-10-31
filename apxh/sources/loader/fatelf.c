/** @file
  APXH FatELF Loader

  Provides FatELF (multi-architecture ELF container) format parsing and
  loading using COM-style interface. FatELF embeds multiple ELF binaries
  for different architectures into one file, similar to macOS Universal
  Binaries.

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

extern IImageLoader gElfLoader;

//
// Internal Functions
//

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

    case ARCH_ARM64:
      *MachineType = EM_AARCH64;
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
  FATELF_HEADER *Header;
  FATELF_RECORD *Records;
  UINT16 TargetMachine;
  UINT8 TargetWordSize;
  UINT32 i;

  Header = (FATELF_HEADER *)ImageBase;
  Records = (FATELF_RECORD *)(Header + 1);

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

//
// IImageLoader Implementation for FatELF
//

/**
  Detect if image is FatELF format.
**/
static
HRESULT
STDMETHODCALLTYPE
FatElfDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  FATELF_HEADER *Header;

  if (ImageSize < sizeof(FATELF_HEADER)) {
    return S_FALSE;
  }

  Header = (FATELF_HEADER *)ImageBase;

  if (Header->Magic == FATELF_MAGIC &&
      Header->Version == FATELF_VERSION &&
      Header->NumRecords > 0) {
    return S_OK;
  }

  return S_FALSE;
}

/**
  Get architecture from FatELF image.

  Note: FatELF contains multiple architectures. We return the first
  supported architecture, with preference for the current platform.
**/
static
HRESULT
STDMETHODCALLTYPE
FatElfGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  FATELF_HEADER *Header;
  FATELF_RECORD *Records;
  FATELF_RECORD *Rec;
  UINT32 i;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  Header = (FATELF_HEADER *)ImageBase;
  Records = (FATELF_RECORD *)(Header + 1);

  // Try to find current architecture first
#if defined(ANANKE_ARCH_X64)
  Rec = FindMatchingRecord(ImageBase, ARCH_AMD64);
  if (Rec != NULL) {
    *Architecture = ARCH_AMD64;
    return S_OK;
  }
#elif defined(ANANKE_ARCH_X86)
  Rec = FindMatchingRecord(ImageBase, ARCH_386);
  if (Rec != NULL) {
    *Architecture = ARCH_386;
    return S_OK;
  }
#elif defined(ANANKE_ARCH_RISCV64)
  Rec = FindMatchingRecord(ImageBase, ARCH_RISCV64);
  if (Rec != NULL) {
    *Architecture = ARCH_RISCV64;
    return S_OK;
  }
#endif

  // Return first supported architecture
  for (i = 0; i < Header->NumRecords; i++) {
    Rec = &Records[i];

    switch (Rec->MachineType) {
      case EM_386:
        *Architecture = ARCH_386;
        return S_OK;
      case EM_X86_64:
        *Architecture = ARCH_AMD64;
        return S_OK;
      case EM_RISCV:
        *Architecture = ARCH_RISCV64;
        return S_OK;
      case EM_AARCH64:
        *Architecture = ARCH_ARM64;
        return S_OK;
      default:
        continue;
    }
  }

  *Architecture = ARCH_UNSUPPORTED;
  return IMGLOAD_E_UNSUPPORTED_ARCH;
}

/**
  Get endianness from FatELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
FatElfGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  FATELF_RECORD *Rec;
  VOID *ElfImage;
  ARCH TargetArch;
  HRESULT Status;

  if (Endianness == NULL) {
    return E_POINTER;
  }

  // Get target architecture
  Status = FatElfGetArch(This, ImageBase, &TargetArch);
  if (FAILED(Status)) {
    return Status;
  }

  // Find matching record
  Rec = FindMatchingRecord(ImageBase, TargetArch);
  if (Rec == NULL) {
    return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  // Get endianness from embedded ELF
  ElfImage = FATELF_OFF(Rec->Offset);
  return gElfLoader.lpVtbl->GetEndianness(&gElfLoader, ElfImage, Endianness);
}

/**
  Get entry point from FatELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
FatElfGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  FATELF_RECORD *Rec;
  VOID *ElfImage;
  ARCH TargetArch;
  HRESULT Status;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  // Get target architecture
  Status = FatElfGetArch(This, ImageBase, &TargetArch);
  if (FAILED(Status)) {
    return Status;
  }

  // Find matching record
  Rec = FindMatchingRecord(ImageBase, TargetArch);
  if (Rec == NULL) {
    return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  // Get entry point from embedded ELF
  ElfImage = FATELF_OFF(Rec->Offset);
  return gElfLoader.lpVtbl->GetEntryPoint(&gElfLoader, ElfImage, EntryPoint);
}

/**
  Load FatELF image.

  Extracts the appropriate ELF binary for the target architecture
  and loads it using the ELF loader.
**/
static
HRESULT
STDMETHODCALLTYPE
FatElfLoadImage (
  IN     IImageLoader     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  FATELF_RECORD *Rec;
  IMGLOAD_CONTEXT ElfContext;
  HRESULT Status;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;

  // Find matching record for target architecture
  Rec = FindMatchingRecord(ImageBase, Context->Architecture);
  if (Rec == NULL) {
    return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  // Create context for embedded ELF
  memcpy(&ElfContext, Context, sizeof(IMGLOAD_CONTEXT));
  ElfContext.ImageBase = FATELF_OFF(Rec->Offset);
  ElfContext.ImageSize = Rec->Size;

  // Load the embedded ELF binary
  Status = gElfLoader.lpVtbl->LoadImage(&gElfLoader, &ElfContext);
  if (FAILED(Status)) {
    return Status;
  }

  // Copy results back
  Context->Architecture = ElfContext.Architecture;
  Context->Endianness = ElfContext.Endianness;
  Context->EntryPoint = ElfContext.EntryPoint;

  return S_OK;
}

/**
  Extract TLS information from FatELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
FatElfGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  FATELF_RECORD *Rec;
  VOID *ElfImage;
  ARCH TargetArch;
  HRESULT Status;

  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // Get target architecture
  Status = FatElfGetArch(This, ImageBase, &TargetArch);
  if (FAILED(Status)) {
    memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
    return Status;
  }

  // Find matching record
  Rec = FindMatchingRecord(ImageBase, TargetArch);
  if (Rec == NULL) {
    memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
    return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  // Get TLS info from embedded ELF
  ElfImage = FATELF_OFF(Rec->Offset);
  return gElfLoader.lpVtbl->GetTlsInfo(&gElfLoader, ElfImage, TlsInfo);
}

/**
  Extract unwinding information from FatELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
FatElfGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  FATELF_RECORD *Rec;
  VOID *ElfImage;
  ARCH TargetArch;
  HRESULT Status;

  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  // Get target architecture
  Status = FatElfGetArch(This, ImageBase, &TargetArch);
  if (FAILED(Status)) {
    memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
    return Status;
  }

  // Find matching record
  Rec = FindMatchingRecord(ImageBase, TargetArch);
  if (Rec == NULL) {
    memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
    return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  // Get unwinding info from embedded ELF
  ElfImage = FATELF_OFF(Rec->Offset);
  return gElfLoader.lpVtbl->GetUnwindInfo(&gElfLoader, ElfImage, UnwindInfo);
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
FatElfGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  FATELF_RECORD *Rec;
  VOID *ElfImage;
  ARCH TargetArch;
  HRESULT Status;

  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  // Get target architecture
  Status = FatElfGetArch(This, ImageBase, &TargetArch);
  if (FAILED(Status)) {
    memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
    return Status;
  }

  // Find matching record
  Rec = FindMatchingRecord(ImageBase, TargetArch);
  if (Rec == NULL) {
    memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
    return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  // Get symbol from embedded ELF
  ElfImage = FATELF_OFF(Rec->Offset);
  return gElfLoader.lpVtbl->GetSymbolByAddress(&gElfLoader, ElfImage, Address, SymbolInfo);
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
FatElfGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  FATELF_RECORD *Rec;
  VOID *ElfImage;
  ARCH TargetArch;
  HRESULT Status;

  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  // Get target architecture
  Status = FatElfGetArch(This, ImageBase, &TargetArch);
  if (FAILED(Status)) {
    memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
    return Status;
  }

  // Find matching record
  Rec = FindMatchingRecord(ImageBase, TargetArch);
  if (Rec == NULL) {
    memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
    return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  // Get symbol from embedded ELF
  ElfImage = FATELF_OFF(Rec->Offset);
  return gElfLoader.lpVtbl->GetSymbolByName(&gElfLoader, ElfImage, Name, SymbolInfo);
}

/**
  Extract relocation information from FatELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
FatElfGetRelocInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_RELOC_INFO   *RelocInfo
  )
{
  FATELF_RECORD *Rec;
  VOID *ElfImage;
  ARCH TargetArch;
  HRESULT Status;

  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  // Get target architecture
  Status = FatElfGetArch(This, ImageBase, &TargetArch);
  if (FAILED(Status)) {
    memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));
    return Status;
  }

  // Find matching record
  Rec = FindMatchingRecord(ImageBase, TargetArch);
  if (Rec == NULL) {
    memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));
    return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  // Delegate to embedded ELF loader
  ElfImage = FATELF_OFF(Rec->Offset);
  return gElfLoader.lpVtbl->GetRelocInfo(&gElfLoader, ElfImage, RelocInfo);
}

/**
  Apply relocations to FatELF image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
FatElfApplyRelocations (
  IN VOID              *ImageBase,
  IN VIRTUAL_ADDRESS   LoadAddress,
  IN VIRTUAL_ADDRESS   PreferredBase
  )
{
  FATELF_HEADER *Header;
  FATELF_RECORD *Records;
  FATELF_RECORD *Rec;
  VOID *ElfImage;
  ARCH TargetArch;
  HRESULT Status;
  UINTN i;

  Header = (FATELF_HEADER *)ImageBase;
  Records = (FATELF_RECORD *)FATELF_OFF(sizeof(FATELF_HEADER));

  // Determine target architecture
  TargetArch = ARCH_INVALID;
#if defined(ANANKE_ARCH_X64)
  TargetArch = ARCH_AMD64;
#elif defined(ANANKE_ARCH_ARM64)
  TargetArch = ARCH_AARCH64;
#elif defined(ANANKE_ARCH_RISCV64)
  TargetArch = ARCH_RISCV64;
#elif defined(ANANKE_ARCH_IA32)
  TargetArch = ARCH_386;
#endif

  // Find matching record
  Rec = NULL;
  for (i = 0; i < ANX_BSWAP16(Header->NumRecords); i++) {
    if (GetArchFromMachine(ANX_BSWAP16(Records[i].MachineType)) == TargetArch) {
      Rec = &Records[i];
      break;
    }
  }

  if (Rec == NULL) {
    return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  // Delegate to embedded ELF loader
  ElfImage = FATELF_OFF(ANX_BSWAP64(Rec->Offset));
  return gElfLoader.lpVtbl->ApplyRelocations(ElfImage, LoadAddress, PreferredBase);
}

/**
  IUnknown::QueryInterface implementation.
**/
static
HRESULT
STDMETHODCALLTYPE
FatElfQueryInterface (
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
FatElfAddRef (
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
FatElfRelease (
  IN IImageLoader  *This
  )
{
  return 1;
}

//
// FatELF Loader VTable
//

static CONST IImageLoaderVtbl gFatElfVtbl = {
  FatElfQueryInterface,
  FatElfAddRef,
  FatElfRelease,
  FatElfDetect,
  FatElfGetArch,
  FatElfGetEndianness,
  FatElfGetEntryPoint,
  FatElfLoadImage,
  FatElfGetTlsInfo,
  FatElfGetUnwindInfo,
  FatElfGetSymbolByAddress,
  FatElfGetSymbolByName,
  FatElfGetRelocInfo,
  FatElfApplyRelocations
};

//
// FatELF Loader Instance
//

IImageLoader gFatElfLoader = {
  &gFatElfVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gFatElfLoader);
