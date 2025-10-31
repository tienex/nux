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
#include <apxh/vas.h>
#include <ananke/resource.h>
#include "imgresource.h"

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
// ELF OS/ABI Identification
//

#define EI_OSABI              7    ///< OS/ABI identification index in e_ident
#define ELFOSABI_NONE         0    ///< UNIX System V ABI
#define ELFOSABI_SYSV         0    ///< Alias for NONE
#define ELFOSABI_HPUX         1    ///< HP-UX
#define ELFOSABI_NETBSD       2    ///< NetBSD
#define ELFOSABI_LINUX        3    ///< Linux
#define ELFOSABI_GNU          3    ///< GNU (same as Linux)
#define ELFOSABI_SOLARIS      6    ///< Sun Solaris
#define ELFOSABI_AIX          7    ///< IBM AIX
#define ELFOSABI_IRIX         8    ///< SGI Irix
#define ELFOSABI_FREEBSD      9    ///< FreeBSD
#define ELFOSABI_TRU64        10   ///< Compaq TRU64 UNIX
#define ELFOSABI_MODESTO      11   ///< Novell Modesto
#define ELFOSABI_OPENBSD      12   ///< OpenBSD
#define ELFOSABI_OPENVMS      13   ///< Open VMS
#define ELFOSABI_NSK          14   ///< Hewlett-Packard Non-Stop Kernel
#define ELFOSABI_AROS         15   ///< Amiga Research OS
#define ELFOSABI_FENIXOS      16   ///< The FenixOS
#define ELFOSABI_CLOUDABI     17   ///< Nuxi CloudABI
#define ELFOSABI_OPENVOS      18   ///< Stratus Technologies OpenVOS
#define ELFOSABI_ARM_AEABI    64   ///< ARM EABI
#define ELFOSABI_ARM          97   ///< ARM
#define ELFOSABI_OS2          48   ///< OS/2 (unofficial value used by OS/2 PowerPC)
#define ELFOSABI_STANDALONE   255  ///< Standalone (embedded)

//
// ELF File Types
//

#define ET_EXEC     2  ///< Executable file

//
// ELF Machine Types
//

#define EM_386      3    ///< Intel 80386
#define EM_PPC      20   ///< PowerPC 32-bit
#define EM_PPC64    21   ///< PowerPC 64-bit
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
// OS/2 Presentation Manager Application Types (from NOTE sections)
//

#define OS2_PM_WINDOWED       0x00  ///< PM windowed application
#define OS2_PM_WINDOWCOMPAT   0x01  ///< PM compatible (windowed or console)
#define OS2_PM_WINDOWNOCOMPAT 0x02  ///< Non-PM application (console only)

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
// APXH-Specific ELF Program Header Types (OS-specific range)
//

#define ELF_APXH_INFO              0xAF100000  ///< Boot info page
#define ELF_APXH_EMPTY             0xAF100001  ///< Empty VA allocation
#define ELF_APXH_PHYSICAL_MAP      0xAF100002  ///< 1:1 physical memory map
#define ELF_APXH_PFN_MAP           0xAF100003  ///< Page frame number map
#define ELF_APXH_BATREE            0xAF100004  ///< Allocated pages bitmap
#define ELF_APXH_PT_ALLOC          0xAF100005  ///< Page table allocation
#define ELF_APXH_FRAMEBUFFER       0xAF100006  ///< Framebuffer mapping
#define ELF_APXH_REGIONS           0xAF100007  ///< Region list
#define ELF_APXH_TOP_PT_ALLOC      0xAF100008  ///< Top-level PT allocation
#define ELF_APXH_LINEAR            0xAF100009  ///< Linear page table map

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
  Map ELF program header type to generic segment type.

  @param[in] ElfType  ELF program header type value.

  @return Generic SEGMENT_TYPE.
**/
static
SEGMENT_TYPE
MapElfTypeToSegmentType (
  IN UINT32  ElfType
  )
{
  switch (ElfType) {
    case PHT_NULL:
      return SegmentNull;
    case PHT_LOAD:
      return SegmentLoad;
    case PHT_DYNAMIC:
      return SegmentDynamic;
    case PHT_TLS:
      return SegmentTls;
    case ELF_APXH_INFO:
      return SegmentInfo;
    case ELF_APXH_EMPTY:
      return SegmentEmpty;
    case ELF_APXH_PHYSICAL_MAP:
      return SegmentPhysicalMap;
    case ELF_APXH_PFN_MAP:
      return SegmentPfnMap;
    case ELF_APXH_BATREE:
      return SegmentBatree;
    case ELF_APXH_PT_ALLOC:
      return SegmentPageTableAlloc;
    case ELF_APXH_TOP_PT_ALLOC:
      return SegmentTopPageTableAlloc;
    case ELF_APXH_FRAMEBUFFER:
      return SegmentFramebuffer;
    case ELF_APXH_REGIONS:
      return SegmentRegions;
    case ELF_APXH_LINEAR:
      return SegmentLinear;
    default:
      return SegmentNull;  // Ignore unknown types
  }
}

/**
  Load user-space program header.

  Processes program header for user-space segments (LOAD, TLS).
  Copies or zeros memory as needed and sets up virtual address mappings.

  @param[in] ElfImg  Pointer to ELF image.
  @param[in] Type     Generic segment type.
  @param[in] Flags    Segment flags (PHF_R/PHF_W/PHF_X).
  @param[in] Va       Virtual address.
  @param[in] MSize    Memory size.
  @param[in] Off      File offset.
  @param[in] FSize    File size.
**/
static
VOID
PhUload (
  IN VOID          *ElfImg,
  IN SEGMENT_TYPE  Type,
  IN UINT32        Flags,
  IN UINT64        Va,
  IN UINT64        MSize,
  IN UINT64        Off,
  IN UINT64        FSize
  )
{
  switch (Type) {
    case SegmentLoad:
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

    case SegmentTls:
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

  Processes program header for kernel segments (LOAD, TLS, and
  custom APXH types for boot information, physical mappings, etc).

  @param[in] ElfImg  Pointer to ELF image.
  @param[in] Type     Generic segment type.
  @param[in] Flags    Segment flags (PHF_R/PHF_W/PHF_X).
  @param[in] Va       Virtual address.
  @param[in] MSize    Memory size.
  @param[in] Off      File offset.
  @param[in] FSize    File size.
**/
static
VOID
PhKload (
  IN VOID          *ElfImg,
  IN SEGMENT_TYPE  Type,
  IN UINT32        Flags,
  IN UINT64        Va,
  IN UINT64        MSize,
  IN UINT64        Off,
  IN UINT64        FSize
  )
{
  switch (Type) {
    case SegmentLoad:
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

    case SegmentTls:
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

    case SegmentInfo:
      // Boot Information segment
      printf("Boot Information area at %" PRIx64 " (size: %" PRId64 "d).\n",
             Va, MSize);
      VirtualAddressMapInfo(Va, MSize);
      break;

    case SegmentPhysicalMap:
      // Direct 1:1 PA mapping
      printf("Physmap VA area at %" PRIx64 " (size: %" PRId64 ").\n", Va,
             MSize);
      VirtualAddressMapPhysical(Va, MSize, MEMTYPE_WB);
      break;

    case SegmentEmpty:
      printf("Empty VA area at %" PRIx64 " (size: %" PRId64 ").\n", Va,
             MSize);
      // Just VA allocation. Leave it.
      break;

    case SegmentPageTableAlloc:
      printf("PT Alloc VA area at %" PRIx64 " (size: %" PRId64 ").\n", Va,
             MSize);
      VirtualAddressAllocatePageTable(Va, MSize);
      break;

    case SegmentPfnMap:
      printf("PFN Map at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VirtualAddressMapPageFrameNumbers(Va, MSize);
      break;

    case SegmentBatree:
      printf("S-Tree at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VirtualAddressMapBatree(Va, MSize);
      break;

    case SegmentLinear:
      printf("Linear Map at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VirtualAddressMapLinear(Va, MSize);
      break;

    case SegmentFramebuffer:
      printf("Framebuffer Map at %" PRIx64 " (size: %" PRId64 ").\n", Va,
             MSize);
      VirtualAddressMapFramebuffer(Va, MSize, MEMTYPE_WC);
      break;

    case SegmentRegions:
      printf("Region Map at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VirtualAddressMapRegions(Va, MSize);
      break;

    case SegmentTopPageTableAlloc:
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
    SEGMENT_TYPE SegType = MapElfTypeToSegmentType(ProgramHeader->Type);

    if (User)
      PhUload(ElfImg, SegType, ProgramHeader->Flags,
              ProgramHeader->Va, ProgramHeader->Msize,
              ProgramHeader->Off, ProgramHeader->Fsize);
    else
      PhKload(ElfImg, SegType, ProgramHeader->Flags,
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
    SEGMENT_TYPE SegType = MapElfTypeToSegmentType(ProgramHeader->Type);

    if (User)
      PhUload(ElfImg, SegType, ProgramHeader->Flags,
              ProgramHeader->Va, ProgramHeader->Msize,
              ProgramHeader->Off, ProgramHeader->Fsize);
    else
      PhKload(ElfImg, SegType, ProgramHeader->Flags,
              ProgramHeader->Va, ProgramHeader->Msize,
              ProgramHeader->Off, ProgramHeader->Fsize);
  }

  return (VIRTUAL_ADDRESS)ElfHeader->Entry;
}

/**
  Get ELF architecture.

  Determines the target architecture from ELF machine type.

  @param[in] ElfImg  Pointer to ELF image.

  @return Architecture type, or ArchInvalid/ArchUnsupported.
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
    return ArchInvalid;

  if (ElfHeader->Mach == EM_386)
    return Arch386;

  if (ElfHeader->Mach == EM_X86_64)
    return ArchAmd64;

  if (ElfHeader->Mach == EM_PPC)
    return ArchPpc32;

  if (ElfHeader->Mach == EM_PPC64)
    return ArchPpc64;

  if (ElfHeader->Mach == EM_RISCV)
    return ArchRiscV64;

  return ArchUnsupported;
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
      UnwindInfo->Format = ImgUnwindFormatDwarfEhFrame;
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
        UnwindInfo->Format = ImgUnwindFormatDwarfEhFrame;
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
      UnwindInfo->Format = ImgUnwindFormatDwarfEhFrame;
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
        UnwindInfo->Format = ImgUnwindFormatDwarfEhFrame;
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
          // Found symbol - map ELF types to IMGLOAD types
          UINT8 ElfType = ELF_ST_TYPE(Symbols[j].Info);
          UINT8 ElfBind = ELF_ST_BIND(Symbols[j].Info);

          SymbolInfo->Name = &StringTable[Symbols[j].Name];
          SymbolInfo->Address = Symbols[j].Value;
          SymbolInfo->Size = Symbols[j].Size;

          // Map ELF symbol type to IMGLOAD_SYMBOL_TYPE
          switch (ElfType) {
            case STT_OBJECT:  SymbolInfo->Type = ImgSymbolTypeObject; break;
            case STT_FUNC:    SymbolInfo->Type = ImgSymbolTypeFunc; break;
            case STT_SECTION: SymbolInfo->Type = ImgSymbolTypeSection; break;
            default:          SymbolInfo->Type = ImgSymbolTypeNone; break;
          }

          // Map ELF symbol binding to IMGLOAD_SYMBOL_BINDING
          switch (ElfBind) {
            case STB_LOCAL:  SymbolInfo->Binding = ImgSymbolBindLocal; break;
            case STB_GLOBAL: SymbolInfo->Binding = ImgSymbolBindGlobal; break;
            case STB_WEAK:   SymbolInfo->Binding = ImgSymbolBindWeak; break;
            default:         SymbolInfo->Binding = ImgSymbolBindLocal; break;
          }

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
          // Found symbol - map ELF types to IMGLOAD types
          UINT8 ElfType = ELF_ST_TYPE(Symbols[j].Info);
          UINT8 ElfBind = ELF_ST_BIND(Symbols[j].Info);

          SymbolInfo->Name = &StringTable[Symbols[j].Name];
          SymbolInfo->Address = Symbols[j].Value;
          SymbolInfo->Size = Symbols[j].Size;

          // Map ELF symbol type to IMGLOAD_SYMBOL_TYPE
          switch (ElfType) {
            case STT_OBJECT:  SymbolInfo->Type = ImgSymbolTypeObject; break;
            case STT_FUNC:    SymbolInfo->Type = ImgSymbolTypeFunc; break;
            case STT_SECTION: SymbolInfo->Type = ImgSymbolTypeSection; break;
            default:          SymbolInfo->Type = ImgSymbolTypeNone; break;
          }

          // Map ELF symbol binding to IMGLOAD_SYMBOL_BINDING
          switch (ElfBind) {
            case STB_LOCAL:  SymbolInfo->Binding = ImgSymbolBindLocal; break;
            case STB_GLOBAL: SymbolInfo->Binding = ImgSymbolBindGlobal; break;
            case STB_WEAK:   SymbolInfo->Binding = ImgSymbolBindWeak; break;
            default:         SymbolInfo->Binding = ImgSymbolBindLocal; break;
          }

          return S_OK;
        }
      }
    }
  }

  return S_FALSE;  // Symbol not found
}

/**
  OS/2 PowerPC ELF Export Entry (from .export section)
**/
ANX_PACK_PUSH(1)
typedef struct _OS2_EXPORT_ENTRY {
  UINT32  Address;     ///< Symbol address/offset
  UINT32  NameOffset;  ///< Offset to symbol name in .export string table
} OS2_EXPORT_ENTRY;
ANX_PACK_POP()

/**
  Get symbol by name from OS/2 PowerPC ELF .export section.

  OS/2 PowerPC ELF uses a custom .export section for exported symbols,
  similar to PE export directory but with ELF-style layout.

  @param[in]  ElfImg      Pointer to ELF image.
  @param[in]  Name        Symbol name to look up.
  @param[out] SymbolInfo  Receives symbol information.

  @return S_OK on success, S_FALSE if not found, error code otherwise.
**/
static
HRESULT
GetOs2ExportSymbolByName (
  IN  VOID                 *ElfImg,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  ELF32_HDR *ElfHeader = (ELF32_HDR *)ElfImg;
  ELF32_SH *SectionHeaders;
  ELF32_SH *StrTab;
  INT32 i;
  UINTN NameLen;

  if (ElfHeader->Shoff == 0 || ElfHeader->Shs == 0) {
    return S_FALSE;
  }

  SectionHeaders = (ELF32_SH *)ELFOFF(ElfHeader->Shoff);

  // Get section name string table
  if (ElfHeader->Shstrndx >= ElfHeader->Shs) {
    return S_FALSE;
  }
  StrTab = &SectionHeaders[ElfHeader->Shstrndx];

  NameLen = strlen(Name);

  // Search for .export section
  for (i = 0; i < ElfHeader->Shs; i++) {
    CHAR8 *SectionName = (CHAR8 *)ELFOFF(StrTab->Off + SectionHeaders[i].Name);

    if (memcmp(SectionName, ".export", 7) == 0) {
      // Found .export section
      OS2_EXPORT_ENTRY *Exports = (OS2_EXPORT_ENTRY *)ELFOFF(SectionHeaders[i].Off);
      UINT32 NumExports = SectionHeaders[i].Size / sizeof(OS2_EXPORT_ENTRY);
      UINT32 j;

      // Find associated string table (usually next section)
      ELF32_SH *ExportStrTab = NULL;
      if (i + 1 < ElfHeader->Shs &&
          SectionHeaders[i + 1].Type == SHT_STRTAB) {
        ExportStrTab = &SectionHeaders[i + 1];
      }

      if (ExportStrTab == NULL) {
        return S_FALSE;
      }

      // Search exports
      for (j = 0; j < NumExports; j++) {
        CHAR8 *ExportName = (CHAR8 *)ELFOFF(ExportStrTab->Off + Exports[j].NameOffset);
        UINTN ExportNameLen = strlen(ExportName);

        if (ExportNameLen == NameLen && memcmp(ExportName, Name, NameLen) == 0) {
          // Found export
          UINTN CopyLen = (NameLen < sizeof(SymbolInfo->Name) - 1) ?
                          NameLen : (sizeof(SymbolInfo->Name) - 1);
          memcpy(SymbolInfo->Name, Name, CopyLen);
          SymbolInfo->Name[CopyLen] = '\0';
          SymbolInfo->Address = Exports[j].Address;
          SymbolInfo->Size = 0;
          SymbolInfo->Type = ImgSymbolTypeFunction;
          SymbolInfo->Binding = ImgSymbolBindGlobal;
          return S_OK;
        }
      }

      return S_FALSE;  // Export section found but symbol not in it
    }
  }

  return S_FALSE;  // No .export section
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
  HRESULT Status;

  if (ElfHeader->Shoff == 0 || ElfHeader->Shs == 0) {
    return S_FALSE;  // No section headers
  }

  // For OS/2 PowerPC ELF, try .export section first
  if (ElfHeader->Id[EI_OSABI] == ELFOSABI_OS2) {
    Status = GetOs2ExportSymbolByName(ElfImg, Name, SymbolInfo);
    if (SUCCEEDED(Status)) {
      return Status;
    }
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
          // Found symbol - map ELF types to IMGLOAD types
          UINT8 ElfType = ELF_ST_TYPE(Symbols[j].Info);
          UINT8 ElfBind = ELF_ST_BIND(Symbols[j].Info);

          SymbolInfo->Name = SymbolName;
          SymbolInfo->Address = Symbols[j].Value;
          SymbolInfo->Size = Symbols[j].Size;

          // Map ELF symbol type to IMGLOAD_SYMBOL_TYPE
          switch (ElfType) {
            case STT_OBJECT:  SymbolInfo->Type = ImgSymbolTypeObject; break;
            case STT_FUNC:    SymbolInfo->Type = ImgSymbolTypeFunc; break;
            case STT_SECTION: SymbolInfo->Type = ImgSymbolTypeSection; break;
            default:          SymbolInfo->Type = ImgSymbolTypeNone; break;
          }

          // Map ELF symbol binding to IMGLOAD_SYMBOL_BINDING
          switch (ElfBind) {
            case STB_LOCAL:  SymbolInfo->Binding = ImgSymbolBindLocal; break;
            case STB_GLOBAL: SymbolInfo->Binding = ImgSymbolBindGlobal; break;
            case STB_WEAK:   SymbolInfo->Binding = ImgSymbolBindWeak; break;
            default:         SymbolInfo->Binding = ImgSymbolBindLocal; break;
          }

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
          // Found symbol - map ELF types to IMGLOAD types
          UINT8 ElfType = ELF_ST_TYPE(Symbols[j].Info);
          UINT8 ElfBind = ELF_ST_BIND(Symbols[j].Info);

          SymbolInfo->Name = SymbolName;
          SymbolInfo->Address = Symbols[j].Value;
          SymbolInfo->Size = Symbols[j].Size;

          // Map ELF symbol type to IMGLOAD_SYMBOL_TYPE
          switch (ElfType) {
            case STT_OBJECT:  SymbolInfo->Type = ImgSymbolTypeObject; break;
            case STT_FUNC:    SymbolInfo->Type = ImgSymbolTypeFunc; break;
            case STT_SECTION: SymbolInfo->Type = ImgSymbolTypeSection; break;
            default:          SymbolInfo->Type = ImgSymbolTypeNone; break;
          }

          // Map ELF symbol binding to IMGLOAD_SYMBOL_BINDING
          switch (ElfBind) {
            case STB_LOCAL:  SymbolInfo->Binding = ImgSymbolBindLocal; break;
            case STB_GLOBAL: SymbolInfo->Binding = ImgSymbolBindGlobal; break;
            case STB_WEAK:   SymbolInfo->Binding = ImgSymbolBindWeak; break;
            default:         SymbolInfo->Binding = ImgSymbolBindLocal; break;
          }

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

  if (*Architecture == ArchInvalid) {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  if (*Architecture == ArchUnsupported) {
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
  RelocInfo->Format = ImgRelocFormatNone;
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

/**
  Get target operating system from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetTargetSystem (
  IN  IImageLoader           *This,
  IN  VOID                   *ImageBase,
  OUT IMGLOAD_TARGET_SYSTEM  *TargetSystem
  )
{
  ELF32_HDR *Elf32;
  ELF64_HDR *Elf64;
  UINT8 OsAbi;
  UINT8 ElfClass;

  if (TargetSystem == NULL) {
    return E_POINTER;
  }

  Elf32 = (ELF32_HDR *)ImageBase;
  Elf64 = (ELF64_HDR *)ImageBase;

  ElfClass = Elf32->Id[4];  // EI_CLASS
  OsAbi = Elf32->Id[EI_OSABI];

  // Determine OS from OSABI
  switch (OsAbi) {
    case ELFOSABI_HPUX:
      *TargetSystem = ImgSystemHpux;
      break;
    case ELFOSABI_NETBSD:
      *TargetSystem = ImgSystemNetBsd;
      break;
    case ELFOSABI_LINUX:
      *TargetSystem = ImgSystemLinux;
      break;
    case ELFOSABI_SOLARIS:
      *TargetSystem = ImgSystemSolaris;
      break;
    case ELFOSABI_AIX:
      *TargetSystem = ImgSystemAix;
      break;
    case ELFOSABI_IRIX:
      *TargetSystem = ImgSystemIrix;
      break;
    case ELFOSABI_FREEBSD:
      *TargetSystem = ImgSystemFreeBsd;
      break;
    case ELFOSABI_TRU64:
      *TargetSystem = ImgSystemOsf1;  // TRU64 is OSF/1
      break;
    case ELFOSABI_MODESTO:
      *TargetSystem = ImgSystemUnix;  // Novell Modesto (rare)
      break;
    case ELFOSABI_OPENBSD:
      *TargetSystem = ImgSystemOpenBsd;
      break;
    case ELFOSABI_OPENVMS:
      *TargetSystem = ImgSystemOpenVms;
      break;
    case ELFOSABI_NSK:
      *TargetSystem = ImgSystemUnix;  // HP Non-Stop Kernel
      break;
    case ELFOSABI_AROS:
      *TargetSystem = ImgSystemAmigaOs;  // AROS is Amiga-like
      break;
    case ELFOSABI_FENIXOS:
      *TargetSystem = ImgSystemUnknown;  // FenixOS (rare)
      break;
    case ELFOSABI_CLOUDABI:
      *TargetSystem = ImgSystemUnix;  // CloudABI (capability-based Unix)
      break;
    case ELFOSABI_OPENVOS:
      *TargetSystem = ImgSystemUnix;  // Stratus OpenVOS
      break;
    case ELFOSABI_ARM_AEABI:
    case ELFOSABI_ARM:
      *TargetSystem = ImgSystemEmbedded;  // ARM EABI (typically embedded)
      break;
    case ELFOSABI_OS2:
      *TargetSystem = ImgSystemOs2;
      break;
    case ELFOSABI_STANDALONE:
      *TargetSystem = ImgSystemBaremetal;  // Standalone/bare-metal
      break;
    case ELFOSABI_NONE:
    default:
      // Generic Unix for SYSV ABI
      *TargetSystem = ImgSystemUnix;
      break;
  }

  return S_OK;
}

/**
  Get minimum required system version from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetMinimumSystemVersion (
  IN  IImageLoader            *This,
  IN  VOID                    *ImageBase,
  OUT IMGLOAD_SYSTEM_VERSION  *MinimumVersion
  )
{
  if (MinimumVersion == NULL) {
    return E_POINTER;
  }

  // ELF doesn't typically encode minimum system version
  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

/**
  Get PM application type from OS/2 ELF NOTE section.

  OS/2 PowerPC ELF uses a NOTE section to indicate PM (Presentation Manager)
  compatibility, similar to the LE/LX module flags:
  - PM incompatible: Console/text mode application
  - PM compatible: Can run in PM (typically GUI)
  - PM only: Requires PM (GUI required)
**/
static
IMGLOAD_TARGET_SUBSYSTEM
ElfGetOs2PmType (
  IN VOID  *ImageBase
  )
{
  ELF32_HDR *Elf32;
  ELF32_SHDR *Sections;
  ELF32_SHDR *StrTab;
  UINT16 i;
  CHAR8 *SectionName;

  Elf32 = (ELF32_HDR *)ImageBase;
  Sections = (ELF32_SHDR *)((UINT8 *)ImageBase + Elf32->Shoff);

  // Get section header string table
  if (Elf32->Shstrndx >= Elf32->Shnum) {
    return ImgSubsystemOs2Cui;  // Default to console
  }

  StrTab = &Sections[Elf32->Shstrndx];

  // Search for OS/2-specific NOTE sections or .note.OS2 section
  for (i = 0; i < Elf32->Shnum; i++) {
    if (Sections[i].Type == SHT_NOTE) {
      // Get section name
      SectionName = (CHAR8 *)((UINT8 *)ImageBase + StrTab->Offset + Sections[i].Name);

      // Check if this is an OS/2 NOTE section
      if (memcmp(SectionName, ".note.OS2", 9) == 0 ||
          memcmp(SectionName, ".note", 5) == 0) {
        // Parse NOTE section for PM type
        UINT8 *NoteData = (UINT8 *)ImageBase + Sections[i].Offset;
        UINT32 NoteSize = Sections[i].Size;

        // NOTE format: namesz, descsz, type, name, desc
        if (NoteSize >= 12) {
          UINT32 *Note = (UINT32 *)NoteData;
          UINT32 NameSz = Note[0];
          UINT32 DescSz = Note[1];
          UINT32 Type = Note[2];

          // Align sizes to 4 bytes
          NameSz = (NameSz + 3) & ~3;

          // Check if this is an OS/2 PM note
          if (DescSz >= 4 && NameSz <= NoteSize - 12) {
            UINT32 *Desc = (UINT32 *)(NoteData + 12 + NameSz);
            UINT32 PmFlags = *Desc;

            // PM flags (similar to LE/LX module flags):
            // 0x00000100 = PM incompatible
            // 0x00000200 = PM compatible
            // 0x00000300 = PM only
            switch (PmFlags & 0x00000300) {
              case 0x00000300:  // PM only
                return ImgSubsystemOs2Gui;
              case 0x00000200:  // PM compatible
                return ImgSubsystemOs2Gui;
              case 0x00000100:  // PM incompatible
              default:
                return ImgSubsystemOs2Cui;
            }
          }
        }
      }
    }
  }

  // Default to console/CLI for OS/2
  return ImgSubsystemOs2Cui;
}

/**
  Get target subsystem from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetTargetSubsystem (
  IN  IImageLoader              *This,
  IN  VOID                      *ImageBase,
  OUT IMGLOAD_TARGET_SUBSYSTEM  *TargetSubsystem
  )
{
  ELF32_HDR *Elf32;
  UINT8 OsAbi;

  if (TargetSubsystem == NULL) {
    return E_POINTER;
  }

  Elf32 = (ELF32_HDR *)ImageBase;
  OsAbi = Elf32->Id[EI_OSABI];

  // For OS/2, check PM application type
  if (OsAbi == ELFOSABI_OS2) {
    *TargetSubsystem = ElfGetOs2PmType(ImageBase);
    return S_OK;
  }

  // For other Unix systems, default to CLI
  *TargetSubsystem = ImgSubsystemCli;
  return S_OK;
}

/**
  Get minimum required subsystem version from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetMinimumSubsystemVersion (
  IN  IImageLoader            *This,
  IN  VOID                    *ImageBase,
  OUT IMGLOAD_SYSTEM_VERSION  *MinimumVersion
  )
{
  if (MinimumVersion == NULL) {
    return E_POINTER;
  }

  // ELF doesn't typically encode minimum subsystem version
  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

/**
  Find .rsrc section in ELF image.
**/
static
HRESULT
ElfFindResourceFork (
  IN  VOID    *ImageBase,
  OUT VOID    **ResourceFork,
  OUT UINT64  *Size
  )
{
  UINT8  *Ident;
  UINT8  ElfClass;
  INT32  i;

  if (ImageBase == NULL || ResourceFork == NULL || Size == NULL) {
    return E_POINTER;
  }

  Ident = (UINT8 *)ImageBase;
  ElfClass = Ident[4];  // EI_CLASS

  if (ElfClass == ELFCLASS32) {
    ELF32_HDR  *Hdr = (ELF32_HDR *)ImageBase;
    ELF32_SH   *Sections;
    ELF32_SH   *StrTabSec;
    CHAR8      *StrTab;

    if (Hdr->Shoff == 0 || Hdr->Shs == 0) {
      return S_FALSE;  // No sections
    }

    Sections = (ELF32_SH *)((UINT8 *)ImageBase + Hdr->Shoff);
    StrTabSec = &Sections[Hdr->Shstrndx];
    StrTab = (CHAR8 *)((UINT8 *)ImageBase + StrTabSec->Off);

    for (i = 0; i < Hdr->Shs; i++) {
      CHAR8 *SecName = &StrTab[Sections[i].Name];

      if (strcmp(SecName, ".rsrc") == 0) {
        *ResourceFork = (VOID *)((UINT8 *)ImageBase + Sections[i].Off);
        *Size = Sections[i].Size;
        return S_OK;
      }
    }
  } else if (ElfClass == ELFCLASS64) {
    ELF64_HDR  *Hdr = (ELF64_HDR *)ImageBase;
    ELF64_SH   *Sections;
    ELF64_SH   *StrTabSec;
    CHAR8      *StrTab;

    if (Hdr->Shoff == 0 || Hdr->Shs == 0) {
      return S_FALSE;  // No sections
    }

    Sections = (ELF64_SH *)((UINT8 *)ImageBase + Hdr->Shoff);
    StrTabSec = &Sections[Hdr->Shstrndx];
    StrTab = (CHAR8 *)((UINT8 *)ImageBase + StrTabSec->Off);

    for (i = 0; i < Hdr->Shs; i++) {
      CHAR8 *SecName = &StrTab[Sections[i].Name];

      if (strcmp(SecName, ".rsrc") == 0) {
        *ResourceFork = (VOID *)((UINT8 *)ImageBase + Sections[i].Off);
        *Size = Sections[i].Size;
        return S_OK;
      }
    }
  } else {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  return S_FALSE;  // No .rsrc section
}

/**
  Get resource from ELF image.

  Uses Classic Mac resource fork in .rsrc section.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetResource (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  IN  IMGLOAD_RESOURCE_ID *ResourceId,
  IN  IMGLOAD_RESOURCE_ID *ResourceType,
  OUT IImageResource      **Resource
  )
{
  VOID     *ResourceFork;
  UINT64   Size;
  UINT32   TypeCode;
  UINT16   Id;
  CONST CHAR8  *Name;
  HRESULT  Status;

  if (ImageBase == NULL || ResourceType == NULL || Resource == NULL) {
    return E_POINTER;
  }

  // Find .rsrc section
  Status = ElfFindResourceFork(ImageBase, &ResourceFork, &Size);
  if (FAILED(Status) || Status == S_FALSE) {
    return Status;
  }

  // Validate resource fork
  Status = AnxResourceValidate(ResourceFork, Size);
  if (FAILED(Status)) {
    return Status;
  }

  // Get type code
  if (ResourceType->IsNumeric) {
    TypeCode = ResourceType->Id;
  } else {
    TypeCode = ANX_MAKE_TYPE(ResourceType->Name);
  }

  // Get ID/Name
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

  // Create resource object
  return CreateImageResource(ResourceFork, TypeCode, Id, Name, Resource);
}

/**
  Get resource enumerator for ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetResourceEnumerator (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  IN  IMGLOAD_RESOURCE_ID *ResourceType,
  OUT IEnumImageResource  **Enumerator
  )
{
  VOID     *ResourceFork;
  UINT64   Size;
  UINT32   TypeCode;
  HRESULT  Status;

  if (ImageBase == NULL || Enumerator == NULL) {
    return E_POINTER;
  }

  // Find .rsrc section
  Status = ElfFindResourceFork(ImageBase, &ResourceFork, &Size);
  if (FAILED(Status) || Status == S_FALSE) {
    return Status;
  }

  // Validate resource fork
  Status = AnxResourceValidate(ResourceFork, Size);
  if (FAILED(Status)) {
    return Status;
  }

  // Get type code
  if (ResourceType != NULL) {
    if (ResourceType->IsNumeric) {
      TypeCode = ResourceType->Id;
    } else {
      TypeCode = ANX_MAKE_TYPE(ResourceType->Name);
    }
  } else {
    TypeCode = 0;  // All types
  }

  // Create enumerator
  return CreateImageResourceEnumerator(ResourceFork, TypeCode, Enumerator);
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
  ElfApplyRelocations,
  ElfGetTargetSystem,
  ElfGetMinimumSystemVersion,
  ElfGetTargetSubsystem,
  ElfGetMinimumSubsystemVersion,
  ElfGetResource,
  ElfGetResourceEnumerator
};

//
// ELF Loader Instance
//

IImageLoader gElfLoader = {
  &gElfVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gElfLoader);
