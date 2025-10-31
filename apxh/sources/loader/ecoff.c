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
BOOLEAN
ANXAPI
EcoffDetect (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  ECOFF32_FILEHDR *Header;

  if (ImageSize < sizeof(ECOFF32_FILEHDR)) {
    return FALSE;
  }

  Header = (ECOFF32_FILEHDR *)ImageBase;

  return (Header->Magic == ECOFF_MAGIC_MIPSEL ||
          Header->Magic == ECOFF_MAGIC_MIPSEB ||
          Header->Magic == ECOFF_MAGIC_MIPS64EL ||
          Header->Magic == ECOFF_MAGIC_MIPS64EB ||
          Header->Magic == ECOFF_MAGIC_ALPHA);
}

/**
  Get architecture from ECOFF image.
**/
static
ARCH
ANXAPI
EcoffGetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  ECOFF32_FILEHDR *Header = (ECOFF32_FILEHDR *)ImageBase;

  switch (Header->Magic) {
    case ECOFF_MAGIC_MIPSEL:
    case ECOFF_MAGIC_MIPSEB:
      // MIPS 32-bit is not directly supported by APXH
      // but could be emulated or run in compatibility mode
      return ARCH_UNSUPPORTED;

    case ECOFF_MAGIC_MIPS64EL:
    case ECOFF_MAGIC_MIPS64EB:
      return ARCH_RISCV64;  // Use RISC-V64 as closest match for MIPS64

    case ECOFF_MAGIC_ALPHA:
      return ARCH_AMD64;  // Use AMD64 as closest match for Alpha (both 64-bit RISC)

    default:
      return ARCH_UNSUPPORTED;
  }
}

/**
  Get entry point from ECOFF image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
EcoffGetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  ECOFF32_FILEHDR *FileHdr32 = (ECOFF32_FILEHDR *)ImageBase;
  ECOFF32_AOUTHDR *AoutHdr32;
  ECOFF64_FILEHDR *FileHdr64;
  ECOFF64_AOUTHDR *AoutHdr64;

  if (FileHdr32->Magic == ECOFF_MAGIC_MIPS64EL ||
      FileHdr32->Magic == ECOFF_MAGIC_MIPS64EB) {
    // 64-bit ECOFF
    FileHdr64 = (ECOFF64_FILEHDR *)ImageBase;
    AoutHdr64 = (ECOFF64_AOUTHDR *)(FileHdr64 + 1);
    return AoutHdr64->Entry;
  } else {
    // 32-bit ECOFF
    AoutHdr32 = (ECOFF32_AOUTHDR *)(FileHdr32 + 1);
    return AoutHdr32->Entry;
  }
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
    VirtualAddressMemset(
      Section->VirtAddr,
      0,
      Section->Size,
      IsUserMode,
      TRUE,  // Writable
      FALSE  // Not executable
    );
  } else {
    // Data section
    VirtualAddressCopy(
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
    VirtualAddressMemset(
      Section->VirtAddr,
      0,
      Section->Size,
      IsUserMode,
      TRUE,  // Writable
      FALSE  // Not executable
    );
  } else {
    // Data section
    VirtualAddressCopy(
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
IMGLOAD_STATUS
ANXAPI
EcoffLoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase = Context->ImageBase;
  ECOFF32_FILEHDR *FileHdr32 = (ECOFF32_FILEHDR *)ImageBase;
  UINT32 i;

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

    info("Loading ECOFF 32-bit MIPS executable...");

    Sections = (ECOFF32_SCNHDR *)ECOFF_OFF(
      sizeof(ECOFF32_FILEHDR) + FileHdr32->OptHeaderSize
    );

    for (i = 0; i < FileHdr32->NumSections; i++) {
      EcoffLoadSection32(ImageBase, &Sections[i], Context->IsUserMode);
    }
  }

  Context->EntryPoint = EcoffGetEntryPoint(This, ImageBase);
  return ImgLoadSuccess;
}

/**
  Extract TLS information from ECOFF image.
**/
static
IMGLOAD_STATUS
ANXAPI
EcoffGetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  // ECOFF doesn't have explicit TLS support
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return ImgLoadSuccess;
}

//
// ECOFF Loader VTable
//

static CONST IMAGE_LOADER_VTBL gEcoffVtbl = {
  EcoffDetect,
  EcoffGetArch,
  EcoffGetEntryPoint,
  EcoffLoadImage,
  EcoffGetTlsInfo
};

//
// ECOFF Loader Instance
//

IMAGE_LOADER gEcoffLoader = {
  &gEcoffVtbl,
  "ECOFF",
  NULL
};
