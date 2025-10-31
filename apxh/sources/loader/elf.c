/** @file
  APXH ELF Loader

  Provides ELF (Executable and Linkable Format) parsing and loading
  for 32-bit and 64-bit executables using COM-style interface.
  Handles program headers for kernel and user-space segments, including
  LOAD, TLS, unwinding info, symbol lookup, and relocations.

  Also supports custom APXH segment types for boot information, physical
  mappings, page table allocation, and other kernel-specific features.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org
  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// ELF Magic Numbers
//

#define ELF_MAGIC  0x7F454C46  ///< "\x7FELF"

//
// ELF Class
//

#define ELFCLASS32  1  ///< 32-bit ELF
#define ELFCLASS64  2  ///< 64-bit ELF

//
// ELF Data Encoding
//

#define ELFDATA2LSB  1  ///< Little-endian
#define ELFDATA2MSB  2  ///< Big-endian

//
// ELF File Types
//

#define ET_EXEC     2  ///< Executable file

//
// ELF Machine Types
//

#define EM_386      3    ///< Intel 80386
#define EM_X86_64   62   ///< AMD x86-64
#define EM_RISCV    0xF3 ///< RISC-V

//
// ELF Version
//

#define EV_CURRENT  1  ///< Current version

//
// ELF Section Header Types
//

#define SHT_PROGBITS  1  ///< Program data
#define SHT_SYMTAB    2  ///< Symbol table
#define SHT_STRTAB    3  ///< String table
#define SHT_RELA      4  ///< Relocation entries with addends
#define SHT_NOBITS    8  ///< BSS section
#define SHT_REL       9  ///< Relocation entries without addends
#define SHT_DYNSYM    11 ///< Dynamic symbol table

//
// ELF Program Header Types
//

#define PHT_NULL     0  ///< Unused entry
#define PHT_LOAD     1  ///< Loadable segment
#define PHT_DYNAMIC  2  ///< Dynamic linking information
#define PHT_INTERP   3  ///< Interpreter path
#define PHT_NOTE     4  ///< Auxiliary information
#define PHT_SHLIB    5  ///< Reserved
#define PHT_PHDR     6  ///< Program header table
#define PHT_TLS      7  ///< Thread-local storage

#define PT_GNU_EH_FRAME  0x6474e550  ///< GCC .eh_frame_hdr segment

//
// ELF Program Header Flags
//

#define PHF_X  1  ///< Execute
#define PHF_W  2  ///< Write
#define PHF_R  4  ///< Read

//
// ELF Relocation Types (x86)
//

#define R_386_NONE        0  ///< No relocation
#define R_386_32          1  ///< Direct 32-bit
#define R_386_PC32        2  ///< PC-relative 32-bit
#define R_386_RELATIVE    8  ///< Relative to load address

//
// ELF Relocation Types (x86-64)
//

#define R_X86_64_NONE      0  ///< No relocation
#define R_X86_64_64        1  ///< Direct 64-bit
#define R_X86_64_PC32      2  ///< PC-relative 32-bit signed
#define R_X86_64_RELATIVE  8  ///< Relative to load address

//
// ELF Relocation Types (RISC-V)
//

#define R_RISCV_NONE      0  ///< No relocation
#define R_RISCV_32        1  ///< Direct 32-bit
#define R_RISCV_64        2  ///< Direct 64-bit
#define R_RISCV_RELATIVE  3  ///< Relative to load address

//
// ELF Symbol Binding and Type
//

#define STB_LOCAL   0  ///< Local symbol
#define STB_GLOBAL  1  ///< Global symbol
#define STB_WEAK    2  ///< Weak symbol

#define STT_NOTYPE   0  ///< No type
#define STT_OBJECT   1  ///< Data object
#define STT_FUNC     2  ///< Function
#define STT_SECTION  3  ///< Section

#define ELF_ST_BIND(i)  ((i) >> 4)
#define ELF_ST_TYPE(i)  ((i) & 0xF)

//
// ELF Structures
//

ANX_PACK_PUSH(1)

typedef struct _ELF32_HDR {
  UINT8   Id[16];      ///< ELF identification
  UINT16  Type;        ///< Object file type
  UINT16  Mach;        ///< Machine type
  UINT32  Ver;         ///< Object file version
  UINT32  Entry;       ///< Entry point address
  UINT32  Phoff;       ///< Program header offset
  UINT32  Shoff;       ///< Section header offset
  UINT32  Flags;       ///< Processor-specific flags
  UINT16  EhSize;      ///< ELF header size
  UINT16  PhentSize;   ///< Program header entry size
  UINT16  Phs;         ///< Number of program headers
  UINT16  ShentSize;   ///< Section header entry size
  UINT16  Shs;         ///< Number of section headers
  UINT16  Shstrndx;    ///< Section header string table index
} ELF32_HDR;

typedef struct _ELF64_HDR {
  UINT8   Id[16];      ///< ELF identification
  UINT16  Type;        ///< Object file type
  UINT16  Mach;        ///< Machine type
  UINT32  Ver;         ///< Object file version
  UINT64  Entry;       ///< Entry point address
  UINT64  Phoff;       ///< Program header offset
  UINT64  Shoff;       ///< Section header offset
  UINT32  Flags;       ///< Processor-specific flags
  UINT16  EhSize;      ///< ELF header size
  UINT16  PhentSize;   ///< Program header entry size
  UINT16  Phs;         ///< Number of program headers
  UINT16  ShentSize;   ///< Section header entry size
  UINT16  Shs;         ///< Number of section headers
  UINT16  Shstrndx;    ///< Section header string table index
} ELF64_HDR;

typedef struct _ELF32_PH {
  UINT32  Type;    ///< Segment type
  UINT32  Off;     ///< Segment file offset
  UINT32  Va;      ///< Segment virtual address
  UINT32  Pa;      ///< Segment physical address
  UINT32  Fsize;   ///< Segment size in file
  UINT32  Msize;   ///< Segment size in memory
  UINT32  Flags;   ///< Segment flags
  UINT32  Align;   ///< Segment alignment
} ELF32_PH;

typedef struct _ELF64_PH {
  UINT32  Type;    ///< Segment type
  UINT32  Flags;   ///< Segment flags
  UINT64  Off;     ///< Segment file offset
  UINT64  Va;      ///< Segment virtual address
  UINT64  Pa;      ///< Segment physical address
  UINT64  Fsize;   ///< Segment size in file
  UINT64  Msize;   ///< Segment size in memory
  UINT64  Align;   ///< Segment alignment
} ELF64_PH;

typedef struct _ELF32_SH {
  UINT32  Name;       ///< Section name (string table index)
  UINT32  Type;       ///< Section type
  UINT32  Flags;      ///< Section flags
  UINT32  Addr;       ///< Section virtual address
  UINT32  Off;        ///< Section file offset
  UINT32  Size;       ///< Section size
  UINT32  Lnk;        ///< Link to another section
  UINT32  Info;       ///< Additional section information
  UINT32  Align;      ///< Section alignment
  UINT32  ShentSize;  ///< Entry size if section holds table
} ELF32_SH;

typedef struct _ELF64_SH {
  UINT32  Name;       ///< Section name (string table index)
  UINT32  Type;       ///< Section type
  UINT64  Flags;      ///< Section flags
  UINT64  Addr;       ///< Section virtual address
  UINT64  Off;        ///< Section file offset
  UINT64  Size;       ///< Section size
  UINT32  Lnk;        ///< Link to another section
  UINT32  Info;       ///< Additional section information
  UINT64  Align;      ///< Section alignment
  UINT64  ShentSize;  ///< Entry size if section holds table
} ELF64_SH;

typedef struct _ELF32_SYM {
  UINT32  Name;   ///< Symbol name (string table index)
  UINT32  Value;  ///< Symbol value
  UINT32  Size;   ///< Symbol size
  UINT8   Info;   ///< Symbol type and binding
  UINT8   Other;  ///< Symbol visibility
  UINT16  Shndx;  ///< Section index
} ELF32_SYM;

typedef struct _ELF64_SYM {
  UINT32  Name;   ///< Symbol name (string table index)
  UINT8   Info;   ///< Symbol type and binding
  UINT8   Other;  ///< Symbol visibility
  UINT16  Shndx;  ///< Section index
  UINT64  Value;  ///< Symbol value
  UINT64  Size;   ///< Symbol size
} ELF64_SYM;

typedef struct _ELF32_REL {
  UINT32  Offset;  ///< Address to apply relocation
  UINT32  Info;    ///< Relocation type and symbol index
} ELF32_REL;

typedef struct _ELF32_RELA {
  UINT32  Offset;  ///< Address to apply relocation
  UINT32  Info;    ///< Relocation type and symbol index
  INT32   Addend;  ///< Addend for relocation
} ELF32_RELA;

typedef struct _ELF64_REL {
  UINT64  Offset;  ///< Address to apply relocation
  UINT64  Info;    ///< Relocation type and symbol index
} ELF64_REL;

typedef struct _ELF64_RELA {
  UINT64  Offset;  ///< Address to apply relocation
  UINT64  Info;    ///< Relocation type and symbol index
  INT64   Addend;  ///< Addend for relocation
} ELF64_RELA;

ANX_PACK_POP()

//
// Helper Macros
//

#define ELFOFF(_o)       ((VOID *)(UINTN)(ElfImg + (_o)))
#define ELF32_R_SYM(i)   ((i) >> 8)
#define ELF32_R_TYPE(i)  ((UINT8)(i))
#define ELF64_R_SYM(i)   ((i) >> 32)
#define ELF64_R_TYPE(i)  ((UINT32)(i))

//
// Internal Functions
//

/**
  Load user-space program header.

  Processes ELF program header for user-space segments (LOAD, TLS).
  Copies or zeros memory as needed and sets up virtual address mappings.

  @param[in] ElfImg  Pointer to ELF image.
  @param[in] Type     Program header type.
  @param[in] Flags    Program header flags (PHF_R/PHF_W/PHF_X).
  @param[in] Va       Virtual address.
  @param[in] MSize    Memory size.
  @param[in] Off      File offset.
  @param[in] FSize    File size.
**/
static
VOID
PhUload (
  IN VOID    *ElfImg,
  IN UINT32  Type,
  IN UINT32  Flags,
  IN UINT64  Va,
  IN UINT64  MSize,
  IN UINT64  Off,
  IN UINT64  FSize
  )
{
  switch (Type) {
    case PHT_LOAD:
      // Normal load segment
      if (Va + MSize < Va) {
        printf("size of PH too big.");
        exit(-1);
      }

      if (FSize) {
        // Copy from file to virtual address (user mode)
        VirtualAddressCopy(Va, ELFOFF(Off), FSize, 1,
                          !!(Flags & PHF_W), !!(Flags & PHF_X));
      }

      if (MSize - FSize > 0) {
        // Zero-fill remainder (BSS)
        VirtualAddressMemset(Va + FSize, 0, MSize - FSize, 1,
                            !!(Flags & PHF_W), !!(Flags & PHF_X));
      }
      break;

    case PHT_TLS:
      // User Thread Local Storage
      if (MSize != 0) {
        printf("USER TLS area at %08" PRIx64 " (initsize: %" PRId64
               " size: %" PRId64 ").\n", Va, FSize, MSize);

        if (Va + MSize < Va) {
          printf("size of PH too big.");
          exit(-1);
        }

        if (FSize != 0) {
          VirtualAddressCopy(Va, ELFOFF(Off), FSize, 0,
                            !!(Flags & PHF_W), !!(Flags & PHF_X));
        }
        VirtualAddressMapUserTls(Va, FSize, MSize);
      }
      break;

    default:
      printf("Ignored segment type %08lx.\n", Type);
      break;
  }
}

/**
  Load kernel program header.

  Processes ELF program header for kernel segments (LOAD, TLS, and
  custom APXH types for boot information, physical mappings, etc).

  @param[in] ElfImg  Pointer to ELF image.
  @param[in] Type     Program header type.
  @param[in] Flags    Program header flags (PHF_R/PHF_W/PHF_X).
  @param[in] Va       Virtual address.
  @param[in] MSize    Memory size.
  @param[in] Off      File offset.
  @param[in] FSize    File size.
**/
static
VOID
PhKload (
  IN VOID    *ElfImg,
  IN UINT32  Type,
  IN UINT32  Flags,
  IN UINT64  Va,
  IN UINT64  MSize,
  IN UINT64  Off,
  IN UINT64  FSize
  )
{
  switch (Type) {
    case PHT_LOAD:
      // Normal load segment
      if (Va + MSize < Va) {
        printf("size of PH too big.");
        exit(-1);
      }

      if (FSize) {
        // Copy from file to virtual address (kernel mode)
        VirtualAddressCopy(Va, ELFOFF(Off), FSize, 0,
                          !!(Flags & PHF_W), !!(Flags & PHF_X));
      }

      if (MSize - FSize > 0) {
        // Zero-fill remainder (BSS)
        VirtualAddressMemset(Va + FSize, 0, MSize - FSize, 0,
                            !!(Flags & PHF_W), !!(Flags & PHF_X));
      }
      break;

    case PHT_TLS:
      // Thread Local Storage
      if (MSize != 0) {
        printf("TLS area at %08" PRIx64 " (initsize: %" PRId64 " size: %"
               PRId64 ").\n", Va, FSize, MSize);
        if (Va + MSize < Va) {
          printf("size of PH too big.");
          exit(-1);
        }
        if (FSize != 0) {
          VirtualAddressCopy(Va, ELFOFF(Off), FSize, 0,
                            !!(Flags & PHF_W), !!(Flags & PHF_X));
        }
        VirtualAddressMapKernelTls(Va, FSize, MSize);
      }
      break;

    case ApxhProgramHeaderInfo:
      // Boot Information segment
      printf("Boot Information area at %" PRIx64 " (size: %" PRId64 "d).\n",
             Va, MSize);
      VirtualAddressMapInfo(Va, MSize);
      break;

    case ApxhProgramHeaderPhysicalMap:
      // Direct 1:1 PA mapping
      printf("Physmap VA area at %" PRIx64 " (size: %" PRId64 ").\n", Va,
             MSize);
      VirtualAddressMapPhysical(Va, MSize, MEMTYPE_WB);
      break;

    case ApxhProgramHeaderEmpty:
      printf("Empty VA area at %" PRIx64 " (size: %" PRId64 ").\n", Va,
             MSize);
      // Just VA allocation. Leave it.
      break;

    case ApxhProgramHeaderPageTableAlloc:
      printf("PT Alloc VA area at %" PRIx64 " (size: %" PRId64 ").\n", Va,
             MSize);
      VirtualAddressAllocatePageTable(Va, MSize);
      break;

    case ApxhProgramHeaderPfnMap:
      printf("PFN Map at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VirtualAddressMapPageFrameNumbers(Va, MSize);
      break;

    case ApxhProgramHeaderBatree:
      printf("S-Tree at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VirtualAddressMapBatree(Va, MSize);
      break;

    case ApxhProgramHeaderLinear:
      printf("Linear Map at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VirtualAddressMapLinear(Va, MSize);
      break;

    case ApxhProgramHeaderFramebuffer:
      printf("Framebuffer Map at %" PRIx64 " (size: %" PRId64 ").\n", Va,
             MSize);
      VirtualAddressMapFramebuffer(Va, MSize, MEMTYPE_WC);
      break;

    case ApxhProgramHeaderRegions:
      printf("Region Map at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VirtualAddressMapRegions(Va, MSize);
      break;

    case ApxhProgramHeaderTopPageTableAlloc:
      printf("TOP PT Alloc VA area at %" PRIx64 " (size: %" PRId64 ").\n",
             Va, MSize);
      VirtualAddressAllocateTopPageTable(Va, MSize);
      break;

    default:
      printf("Ignored segment type %08lx.\n", Type);
      break;
  }
}

/**
  Load 32-bit ELF image.

  Parses and loads a 32-bit ELF executable, processing all program
  headers for kernel or user-space.

  @param[in] ElfImg  Pointer to ELF image.
  @param[in] User     TRUE for user-space, FALSE for kernel.

  @return Entry point virtual address, or -1 on error.
**/
static
VIRTUAL_ADDRESS
LoadElf32 (
  IN VOID   *ElfImg,
  IN INT32  User
  )
{
  INT32 i;
  CHAR8 ElfId[] = { 0x7F, 'E', 'L', 'F' };
  ELF32_HDR *ElfHeader = (ELF32_HDR *)ElfImg;
  ELF32_PH *ProgramHeader = (ELF32_PH *)ELFOFF(ElfHeader->Phoff);

  if (memcmp(ElfHeader->Id, ElfId, 4) != 0)
    return (UINTN)-1;

  if (ElfHeader->Type != ET_EXEC || ElfHeader->Ver != EV_CURRENT)
    return (UINTN)-1;

  for (i = 0; i < ElfHeader->Phs; i++, ProgramHeader++) {
    if (User)
      PhUload(ElfImg, ProgramHeader->Type, ProgramHeader->Flags,
              ProgramHeader->Va, ProgramHeader->Msize,
              ProgramHeader->Off, ProgramHeader->Fsize);
    else
      PhKload(ElfImg, ProgramHeader->Type, ProgramHeader->Flags,
              ProgramHeader->Va, ProgramHeader->Msize,
              ProgramHeader->Off, ProgramHeader->Fsize);
  }

  return (VIRTUAL_ADDRESS)ElfHeader->Entry;
}

/**
  Load 64-bit ELF image.

  Parses and loads a 64-bit ELF executable, processing all program
  headers for kernel or user-space.

  @param[in] ElfImg  Pointer to ELF image.
  @param[in] User     TRUE for user-space, FALSE for kernel.

  @return Entry point virtual address, or -1 on error.
**/
static
VIRTUAL_ADDRESS
LoadElf64 (
  IN VOID   *ElfImg,
  IN INT32  User
  )
{
  INT32 i;
  CHAR8 ElfId[] = { 0x7F, 'E', 'L', 'F' };
  ELF64_HDR *ElfHeader = (ELF64_HDR *)ElfImg;
  ELF64_PH *ProgramHeader = (ELF64_PH *)ELFOFF(ElfHeader->Phoff);

  if (memcmp(ElfHeader->Id, ElfId, 4) != 0)
    return (UINTN)-1;

  if (ElfHeader->Type != ET_EXEC || ElfHeader->Ver != EV_CURRENT)
    return (UINTN)-1;

  for (i = 0; i < ElfHeader->Phs; i++, ProgramHeader++) {
    if (User)
      PhUload(ElfImg, ProgramHeader->Type, ProgramHeader->Flags,
              ProgramHeader->Va, ProgramHeader->Msize,
              ProgramHeader->Off, ProgramHeader->Fsize);
    else
      PhKload(ElfImg, ProgramHeader->Type, ProgramHeader->Flags,
              ProgramHeader->Va, ProgramHeader->Msize,
              ProgramHeader->Off, ProgramHeader->Fsize);
  }

  return (VIRTUAL_ADDRESS)ElfHeader->Entry;
}

/**
  Get ELF architecture.

  Determines the target architecture from ELF machine type.

  @param[in] ElfImg  Pointer to ELF image.

  @return Architecture type, or ARCH_INVALID/ARCH_UNSUPPORTED.
**/
static
ARCH
GetElfArch (
  IN VOID  *ElfImg
  )
{
  CHAR8 ElfId[] = { 0x7F, 'E', 'L', 'F' };
  ELF32_HDR *ElfHeader = (ELF32_HDR *)ElfImg;

  if (memcmp(ElfHeader->Id, ElfId, 4) != 0)
    return ARCH_INVALID;

  if (ElfHeader->Mach == EM_386)
    return ARCH_386;

  if (ElfHeader->Mach == EM_X86_64)
    return ARCH_AMD64;

  if (ElfHeader->Mach == EM_RISCV)
    return ARCH_RISCV64;

  return ARCH_UNSUPPORTED;
}

/**
  Get ELF endianness.

  Determines the endianness from ELF identification field.

  @param[in] ElfImg  Pointer to ELF image.

  @return Endianness type (little/big), or ImgEndianUnknown if invalid.
**/
static
IMGLOAD_ENDIAN
GetElfEndianness (
  IN VOID  *ElfImg
  )
{
  CHAR8 ElfId[] = { 0x7F, 'E', 'L', 'F' };
  UINT8 *Ident = (UINT8 *)ElfImg;

  // Verify ELF magic
  if (memcmp(Ident, ElfId, 4) != 0)
    return ImgEndianUnknown;

  // ELF identification byte 5 (EI_DATA) specifies endianness
  switch (Ident[5]) {
    case ELFDATA2LSB:
      return ImgEndianLittle;
    case ELFDATA2MSB:
      return ImgEndianBig;
    default:
      return ImgEndianUnknown;
  }
}

/**
  Get unwinding information from 32-bit ELF image.

  @param[in]  ElfImg      Pointer to ELF image.
  @param[out] UnwindInfo  Receives unwinding information.

  @return S_OK on success, S_FALSE if no unwinding info, error code otherwise.
**/
static
HRESULT
GetElf32UnwindInfo (
  IN  VOID                 *ElfImg,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  ELF32_HDR *ElfHeader = (ELF32_HDR *)ElfImg;
  ELF32_PH *ProgramHeader;
  ELF32_SH *SectionHeader;
  INT32 i;

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  // First, check program headers for PT_GNU_EH_FRAME
  ProgramHeader = (ELF32_PH *)ELFOFF(ElfHeader->Phoff);
  for (i = 0; i < ElfHeader->Phs; i++, ProgramHeader++) {
    if (ProgramHeader->Type == PT_GNU_EH_FRAME) {
      UnwindInfo->UnwindDataAddr = ProgramHeader->Va;
      UnwindInfo->UnwindDataSize = ProgramHeader->Msize;
      UnwindInfo->Format = 0;  // DWARF eh_frame
      return S_OK;
    }
  }

  // If not found in program headers, check section headers
  if (ElfHeader->Shoff != 0 && ElfHeader->Shs > 0) {
    SectionHeader = (ELF32_SH *)ELFOFF(ElfHeader->Shoff);
    ELF32_SH *StringTableSection = &SectionHeader[ElfHeader->Shstrndx];
    CHAR8 *StringTable = (CHAR8 *)ELFOFF(StringTableSection->Off);

    for (i = 0; i < ElfHeader->Shs; i++) {
      CHAR8 *SectionName = &StringTable[SectionHeader[i].Name];
      if (strcmp(SectionName, ".eh_frame") == 0) {
        UnwindInfo->UnwindDataAddr = SectionHeader[i].Addr;
        UnwindInfo->UnwindDataSize = SectionHeader[i].Size;
        UnwindInfo->Format = 0;  // DWARF eh_frame
        return S_OK;
      }
    }
  }

  return S_FALSE;  // No unwinding info found
}

/**
  Get unwinding information from 64-bit ELF image.

  @param[in]  ElfImg      Pointer to ELF image.
  @param[out] UnwindInfo  Receives unwinding information.

  @return S_OK on success, S_FALSE if no unwinding info, error code otherwise.
**/
static
HRESULT
GetElf64UnwindInfo (
  IN  VOID                 *ElfImg,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  ELF64_HDR *ElfHeader = (ELF64_HDR *)ElfImg;
  ELF64_PH *ProgramHeader;
  ELF64_SH *SectionHeader;
  INT32 i;

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  // First, check program headers for PT_GNU_EH_FRAME
  ProgramHeader = (ELF64_PH *)ELFOFF(ElfHeader->Phoff);
  for (i = 0; i < ElfHeader->Phs; i++, ProgramHeader++) {
    if (ProgramHeader->Type == PT_GNU_EH_FRAME) {
      UnwindInfo->UnwindDataAddr = ProgramHeader->Va;
      UnwindInfo->UnwindDataSize = ProgramHeader->Msize;
      UnwindInfo->Format = 0;  // DWARF eh_frame
      return S_OK;
    }
  }

  // If not found in program headers, check section headers
  if (ElfHeader->Shoff != 0 && ElfHeader->Shs > 0) {
    SectionHeader = (ELF64_SH *)ELFOFF(ElfHeader->Shoff);
    ELF64_SH *StringTableSection = &SectionHeader[ElfHeader->Shstrndx];
    CHAR8 *StringTable = (CHAR8 *)ELFOFF(StringTableSection->Off);

    for (i = 0; i < ElfHeader->Shs; i++) {
      CHAR8 *SectionName = &StringTable[SectionHeader[i].Name];
      if (strcmp(SectionName, ".eh_frame") == 0) {
        UnwindInfo->UnwindDataAddr = SectionHeader[i].Addr;
        UnwindInfo->UnwindDataSize = SectionHeader[i].Size;
        UnwindInfo->Format = 0;  // DWARF eh_frame
        return S_OK;
      }
    }
  }

  return S_FALSE;  // No unwinding info found
}

/**
  Get symbol by address from 32-bit ELF image.

  @param[in]  ElfImg      Pointer to ELF image.
  @param[in]  Address     Virtual address to look up.
  @param[out] SymbolInfo  Receives symbol information.

  @return S_OK on success, S_FALSE if not found, error code otherwise.
**/
static
HRESULT
GetElf32SymbolByAddress (
  IN  VOID                 *ElfImg,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  ELF32_HDR *ElfHeader = (ELF32_HDR *)ElfImg;
  ELF32_SH *SectionHeader;
  INT32 i, j;

  if (ElfHeader->Shoff == 0 || ElfHeader->Shs == 0) {
    return S_FALSE;  // No section headers
  }

  SectionHeader = (ELF32_SH *)ELFOFF(ElfHeader->Shoff);

  // Search for symbol table sections
  for (i = 0; i < ElfHeader->Shs; i++) {
    if (SectionHeader[i].Type == SHT_SYMTAB || SectionHeader[i].Type == SHT_DYNSYM) {
      ELF32_SYM *Symbols = (ELF32_SYM *)ELFOFF(SectionHeader[i].Off);
      UINT32 NumSymbols = SectionHeader[i].Size / sizeof(ELF32_SYM);
      ELF32_SH *StringTableSection = &SectionHeader[SectionHeader[i].Lnk];
      CHAR8 *StringTable = (CHAR8 *)ELFOFF(StringTableSection->Off);

      for (j = 0; j < NumSymbols; j++) {
        if (Symbols[j].Value <= Address &&
            Address < Symbols[j].Value + Symbols[j].Size) {
          // Found symbol
          SymbolInfo->Name = &StringTable[Symbols[j].Name];
          SymbolInfo->Address = Symbols[j].Value;
          SymbolInfo->Size = Symbols[j].Size;
          SymbolInfo->Type = ELF_ST_TYPE(Symbols[j].Info);
          SymbolInfo->Binding = ELF_ST_BIND(Symbols[j].Info);
          return S_OK;
        }
      }
    }
  }

  return S_FALSE;  // Symbol not found
}

/**
  Get symbol by address from 64-bit ELF image.

  @param[in]  ElfImg      Pointer to ELF image.
  @param[in]  Address     Virtual address to look up.
  @param[out] SymbolInfo  Receives symbol information.

  @return S_OK on success, S_FALSE if not found, error code otherwise.
**/
static
HRESULT
GetElf64SymbolByAddress (
  IN  VOID                 *ElfImg,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  ELF64_HDR *ElfHeader = (ELF64_HDR *)ElfImg;
  ELF64_SH *SectionHeader;
  INT32 i, j;

  if (ElfHeader->Shoff == 0 || ElfHeader->Shs == 0) {
    return S_FALSE;  // No section headers
  }

  SectionHeader = (ELF64_SH *)ELFOFF(ElfHeader->Shoff);

  // Search for symbol table sections
  for (i = 0; i < ElfHeader->Shs; i++) {
    if (SectionHeader[i].Type == SHT_SYMTAB || SectionHeader[i].Type == SHT_DYNSYM) {
      ELF64_SYM *Symbols = (ELF64_SYM *)ELFOFF(SectionHeader[i].Off);
      UINT64 NumSymbols = SectionHeader[i].Size / sizeof(ELF64_SYM);
      ELF64_SH *StringTableSection = &SectionHeader[SectionHeader[i].Lnk];
      CHAR8 *StringTable = (CHAR8 *)ELFOFF(StringTableSection->Off);

      for (j = 0; j < NumSymbols; j++) {
        if (Symbols[j].Value <= Address &&
            Address < Symbols[j].Value + Symbols[j].Size) {
          // Found symbol
          SymbolInfo->Name = &StringTable[Symbols[j].Name];
          SymbolInfo->Address = Symbols[j].Value;
          SymbolInfo->Size = Symbols[j].Size;
          SymbolInfo->Type = ELF_ST_TYPE(Symbols[j].Info);
          SymbolInfo->Binding = ELF_ST_BIND(Symbols[j].Info);
          return S_OK;
        }
      }
    }
  }

  return S_FALSE;  // Symbol not found
}

/**
  Get symbol by name from 32-bit ELF image.

  @param[in]  ElfImg      Pointer to ELF image.
  @param[in]  Name        Symbol name to look up.
  @param[out] SymbolInfo  Receives symbol information.

  @return S_OK on success, S_FALSE if not found, error code otherwise.
**/
static
HRESULT
GetElf32SymbolByName (
  IN  VOID                 *ElfImg,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  ELF32_HDR *ElfHeader = (ELF32_HDR *)ElfImg;
  ELF32_SH *SectionHeader;
  INT32 i, j;

  if (ElfHeader->Shoff == 0 || ElfHeader->Shs == 0) {
    return S_FALSE;  // No section headers
  }

  SectionHeader = (ELF32_SH *)ELFOFF(ElfHeader->Shoff);

  // Search for symbol table sections
  for (i = 0; i < ElfHeader->Shs; i++) {
    if (SectionHeader[i].Type == SHT_SYMTAB || SectionHeader[i].Type == SHT_DYNSYM) {
      ELF32_SYM *Symbols = (ELF32_SYM *)ELFOFF(SectionHeader[i].Off);
      UINT32 NumSymbols = SectionHeader[i].Size / sizeof(ELF32_SYM);
      ELF32_SH *StringTableSection = &SectionHeader[SectionHeader[i].Lnk];
      CHAR8 *StringTable = (CHAR8 *)ELFOFF(StringTableSection->Off);

      for (j = 0; j < NumSymbols; j++) {
        CHAR8 *SymbolName = &StringTable[Symbols[j].Name];
        if (strcmp(SymbolName, Name) == 0) {
          // Found symbol
          SymbolInfo->Name = SymbolName;
          SymbolInfo->Address = Symbols[j].Value;
          SymbolInfo->Size = Symbols[j].Size;
          SymbolInfo->Type = ELF_ST_TYPE(Symbols[j].Info);
          SymbolInfo->Binding = ELF_ST_BIND(Symbols[j].Info);
          return S_OK;
        }
      }
    }
  }

  return S_FALSE;  // Symbol not found
}

/**
  Get symbol by name from 64-bit ELF image.

  @param[in]  ElfImg      Pointer to ELF image.
  @param[in]  Name        Symbol name to look up.
  @param[out] SymbolInfo  Receives symbol information.

  @return S_OK on success, S_FALSE if not found, error code otherwise.
**/
static
HRESULT
GetElf64SymbolByName (
  IN  VOID                 *ElfImg,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  ELF64_HDR *ElfHeader = (ELF64_HDR *)ElfImg;
  ELF64_SH *SectionHeader;
  INT32 i, j;

  if (ElfHeader->Shoff == 0 || ElfHeader->Shs == 0) {
    return S_FALSE;  // No section headers
  }

  SectionHeader = (ELF64_SH *)ELFOFF(ElfHeader->Shoff);

  // Search for symbol table sections
  for (i = 0; i < ElfHeader->Shs; i++) {
    if (SectionHeader[i].Type == SHT_SYMTAB || SectionHeader[i].Type == SHT_DYNSYM) {
      ELF64_SYM *Symbols = (ELF64_SYM *)ELFOFF(SectionHeader[i].Off);
      UINT64 NumSymbols = SectionHeader[i].Size / sizeof(ELF64_SYM);
      ELF64_SH *StringTableSection = &SectionHeader[SectionHeader[i].Lnk];
      CHAR8 *StringTable = (CHAR8 *)ELFOFF(StringTableSection->Off);

      for (j = 0; j < NumSymbols; j++) {
        CHAR8 *SymbolName = &StringTable[Symbols[j].Name];
        if (strcmp(SymbolName, Name) == 0) {
          // Found symbol
          SymbolInfo->Name = SymbolName;
          SymbolInfo->Address = Symbols[j].Value;
          SymbolInfo->Size = Symbols[j].Size;
          SymbolInfo->Type = ELF_ST_TYPE(Symbols[j].Info);
          SymbolInfo->Binding = ELF_ST_BIND(Symbols[j].Info);
          return S_OK;
        }
      }
    }
  }

  return S_FALSE;  // Symbol not found
}

/**
  Apply 32-bit ELF relocations.

  @param[in] ImageBase  Base address of image.
  @param[in] Delta       Relocation delta.

  @return S_OK on success, error code otherwise.
**/
static
HRESULT
Elf32ApplyRelocations (
  IN VOID   *ImageBase,
  IN INT64  Delta
  )
{
  ELF32_HDR *Ehdr = (ELF32_HDR *)ImageBase;
  ELF32_SH *Shdr;
  UINT16 i, j;

  if (Ehdr->Shoff == 0 || Ehdr->Shs == 0) {
    return S_OK;  // No sections
  }

  for (i = 0; i < Ehdr->Shs; i++) {
    Shdr = (ELF32_SH *)((UINT8 *)ImageBase + Ehdr->Shoff + i * Ehdr->ShentSize);

    if (Shdr->Type == SHT_REL) {
      ELF32_REL *Rel = (ELF32_REL *)((UINT8 *)ImageBase + Shdr->Off);
      UINT32 NumRelocs = Shdr->Size / sizeof(ELF32_REL);

      for (j = 0; j < NumRelocs; j++) {
        UINT32 Type = ELF32_R_TYPE(Rel[j].Info);
        UINT32 *Target = (UINT32 *)((UINT8 *)ImageBase + Rel[j].Offset);

        switch (Type) {
          case R_386_RELATIVE:
          case R_386_32:
            *Target = (UINT32)(*Target + Delta);
            break;
        }
      }
    } else if (Shdr->Type == SHT_RELA) {
      ELF32_RELA *Rela = (ELF32_RELA *)((UINT8 *)ImageBase + Shdr->Off);
      UINT32 NumRelocs = Shdr->Size / sizeof(ELF32_RELA);

      for (j = 0; j < NumRelocs; j++) {
        UINT32 Type = ELF32_R_TYPE(Rela[j].Info);
        UINT32 *Target = (UINT32 *)((UINT8 *)ImageBase + Rela[j].Offset);

        switch (Type) {
          case R_386_RELATIVE:
          case R_386_32:
            *Target = (UINT32)(Delta + Rela[j].Addend);
            break;
        }
      }
    }
  }

  return S_OK;
}

/**
  Apply 64-bit ELF relocations.

  @param[in] ImageBase  Base address of image.
  @param[in] Delta       Relocation delta.

  @return S_OK on success, error code otherwise.
**/
static
HRESULT
Elf64ApplyRelocations (
  IN VOID   *ImageBase,
  IN INT64  Delta
  )
{
  ELF64_HDR *Ehdr = (ELF64_HDR *)ImageBase;
  ELF64_SH *Shdr;
  UINT16 i;
  UINT64 j;

  if (Ehdr->Shoff == 0 || Ehdr->Shs == 0) {
    return S_OK;  // No sections
  }

  for (i = 0; i < Ehdr->Shs; i++) {
    Shdr = (ELF64_SH *)((UINT8 *)ImageBase + Ehdr->Shoff + i * Ehdr->ShentSize);

    if (Shdr->Type == SHT_REL) {
      ELF64_REL *Rel = (ELF64_REL *)((UINT8 *)ImageBase + Shdr->Off);
      UINT64 NumRelocs = Shdr->Size / sizeof(ELF64_REL);

      for (j = 0; j < NumRelocs; j++) {
        UINT32 Type = ELF64_R_TYPE(Rel[j].Info);
        UINT64 *Target = (UINT64 *)((UINT8 *)ImageBase + Rel[j].Offset);

        switch (Type) {
          case R_X86_64_RELATIVE:
          case R_X86_64_64:
          case R_RISCV_RELATIVE:
          case R_RISCV_64:
            *Target = *Target + Delta;
            break;
        }
      }
    } else if (Shdr->Type == SHT_RELA) {
      ELF64_RELA *Rela = (ELF64_RELA *)((UINT8 *)ImageBase + Shdr->Off);
      UINT64 NumRelocs = Shdr->Size / sizeof(ELF64_RELA);

      for (j = 0; j < NumRelocs; j++) {
        UINT32 Type = ELF64_R_TYPE(Rela[j].Info);
        UINT64 *Target = (UINT64 *)((UINT8 *)ImageBase + Rela[j].Offset);

        switch (Type) {
          case R_X86_64_RELATIVE:
          case R_X86_64_64:
          case R_RISCV_RELATIVE:
          case R_RISCV_64:
            *Target = Delta + Rela[j].Addend;
            break;
        }
      }
    }
  }

  return S_OK;
}

//
// IImageLoader Implementation for ELF
//

/**
  Detect if image is ELF format.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  UINT32 *Magic;

  if (ImageSize < 4) {
    return S_FALSE;
  }

  Magic = (UINT32 *)ImageBase;
  return (*Magic == ELF_MAGIC) ? S_OK : S_FALSE;
}

/**
  Get architecture from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  if (Architecture == NULL) {
    return E_POINTER;
  }

  *Architecture = GetElfArch(ImageBase);

  if (*Architecture == ARCH_INVALID) {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  if (*Architecture == ARCH_UNSUPPORTED) {
    return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  return S_OK;
}

/**
  Get endianness from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  *Endianness = GetElfEndianness(ImageBase);

  if (*Endianness == ImgEndianUnknown) {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  return S_OK;
}

/**
  Get entry point from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  UINT8 *Ident;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  Ident = (UINT8 *)ImageBase;

  // Check ELF class (32-bit or 64-bit)
  if (Ident[4] == ELFCLASS32) {
    ELF32_HDR *Hdr = (ELF32_HDR *)ImageBase;
    *EntryPoint = Hdr->Entry;
  } else if (Ident[4] == ELFCLASS64) {
    ELF64_HDR *Hdr = (ELF64_HDR *)ImageBase;
    *EntryPoint = Hdr->Entry;
  } else {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  if (*EntryPoint == 0 || *EntryPoint == (VIRTUAL_ADDRESS)-1) {
    return IMGLOAD_E_INVALID_HEADER;
  }

  return S_OK;
}

/**
  Load ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfLoadImage (
  IN     IImageLoader     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VIRTUAL_ADDRESS EntryPoint;
  UINT8 *Ident;
  HRESULT Status;

  if (Context == NULL) {
    return E_POINTER;
  }

  Ident = (UINT8 *)Context->ImageBase;

  // Populate architecture and endianness
  Status = ElfGetArch(This, Context->ImageBase, &Context->Architecture);
  if (FAILED(Status)) {
    return Status;
  }

  Status = ElfGetEndianness(This, Context->ImageBase, &Context->Endianness);
  if (FAILED(Status)) {
    return Status;
  }

  // Check ELF class (32-bit or 64-bit)
  if (Ident[4] == ELFCLASS32) {
    // 32-bit ELF
    EntryPoint = LoadElf32(Context->ImageBase, Context->IsUserMode);
  } else if (Ident[4] == ELFCLASS64) {
    // 64-bit ELF
    EntryPoint = LoadElf64(Context->ImageBase, Context->IsUserMode);
  } else {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  if (EntryPoint == (VIRTUAL_ADDRESS)-1) {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  Context->EntryPoint = EntryPoint;
  return S_OK;
}

/**
  Extract TLS information from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // TLS is handled internally by LoadElf32/LoadElf64
  // They call VirtualAddressMapKernelTls/VirtualAddressMapUserTls
  // For now, return S_FALSE to indicate no TLS info available via this method
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  UINT8 *Ident;
  HRESULT Status;

  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  Ident = (UINT8 *)ImageBase;

  // Check ELF class (32-bit or 64-bit)
  if (Ident[4] == ELFCLASS32) {
    Status = GetElf32UnwindInfo(ImageBase, UnwindInfo);
  } else if (Ident[4] == ELFCLASS64) {
    Status = GetElf64UnwindInfo(ImageBase, UnwindInfo);
  } else {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  return Status;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  UINT8 *Ident;
  HRESULT Status;

  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  Ident = (UINT8 *)ImageBase;

  // Check ELF class (32-bit or 64-bit)
  if (Ident[4] == ELFCLASS32) {
    Status = GetElf32SymbolByAddress(ImageBase, Address, SymbolInfo);
  } else if (Ident[4] == ELFCLASS64) {
    Status = GetElf64SymbolByAddress(ImageBase, Address, SymbolInfo);
  } else {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  return Status;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  UINT8 *Ident;
  HRESULT Status;

  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  Ident = (UINT8 *)ImageBase;

  // Check ELF class (32-bit or 64-bit)
  if (Ident[4] == ELFCLASS32) {
    Status = GetElf32SymbolByName(ImageBase, Name, SymbolInfo);
  } else if (Ident[4] == ELFCLASS64) {
    Status = GetElf64SymbolByName(ImageBase, Name, SymbolInfo);
  } else {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  return Status;
}

/**
  Extract relocation information from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetRelocInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_RELOC_INFO   *RelocInfo
  )
{
  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  // ELF relocations are stored in .rel/.rela sections
  // For now, return basic info - full implementation would parse dynamic sections
  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  // ELF executables may have relocations in dynamic sections
  // Format: 1=REL, 2=RELA (we'll determine this dynamically)
  RelocInfo->Format = 0;  // Will be set when we parse sections
  RelocInfo->RequiresReloc = FALSE;  // Most ELF executables don't need relocation

  return S_FALSE;  // Relocation support to be implemented
}

/**
  Apply relocations to ELF image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfApplyRelocations (
  IN VOID              *ImageBase,
  IN VIRTUAL_ADDRESS   LoadAddress,
  IN VIRTUAL_ADDRESS   PreferredBase
  )
{
  UINT8 *Ident;
  INT64 Delta;
  HRESULT Status;

  Ident = (UINT8 *)ImageBase;

  // Calculate relocation delta
  Delta = (INT64)LoadAddress - (INT64)PreferredBase;

  if (Delta == 0) {
    return S_OK;  // No relocation needed
  }

  // Check ELF class (32-bit or 64-bit)
  if (Ident[4] == ELFCLASS32) {
    Status = Elf32ApplyRelocations(ImageBase, Delta);
  } else if (Ident[4] == ELFCLASS64) {
    Status = Elf64ApplyRelocations(ImageBase, Delta);
  } else {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  return Status;
}

/**
  IUnknown::QueryInterface implementation.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfQueryInterface (
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
ElfAddRef (
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
ElfRelease (
  IN IImageLoader  *This
  )
{
  return 1;
}

//
// ELF Loader VTable
//

static CONST IImageLoaderVtbl gElfVtbl = {
  ElfQueryInterface,
  ElfAddRef,
  ElfRelease,
  ElfDetect,
  ElfGetArch,
  ElfGetEndianness,
  ElfGetEntryPoint,
  ElfLoadImage,
  ElfGetTlsInfo,
  ElfGetUnwindInfo,
  ElfGetSymbolByAddress,
  ElfGetSymbolByName,
  ElfGetRelocInfo,
  ElfApplyRelocations
};

//
// ELF Loader Instance
//

IImageLoader gElfLoader = {
  &gElfVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gElfLoader);
