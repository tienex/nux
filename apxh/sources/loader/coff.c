/** @file
  APXH COFF Loader Implementation

  Provides COFF (Common Object File Format) parsing and loading for
  Unix System V executables and object files using COM-style interface.
  Handles sections, optional headers, and relocations for multiple
  architectures with proper endianness detection.

  COFF is the predecessor of ELF and can be either big-endian or little-endian
  depending on the target architecture.

  Supported Architectures:
  - x86: i386, i860, AMD64/x86-64
  - ARM: ARMv4, ARMv4T, ARMv7, ARM64 (multiple ABIs)
  - MIPS: R3000, R4000, R10000, MIPS16, WCE (BE and LE variants)
  - PowerPC: 32-bit LE/BE, 64-bit BE
  - Alpha: 32-bit and 64-bit
  - Motorola: 68000
  - Hitachi SH: SH, SH3, SH4, SH5 (BE and LE variants)
  - Itanium (IA-64)
  - PA-RISC
  - RISC-V: 32-bit, 64-bit, 128-bit
  - LoongArch: 32-bit, 64-bit

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// COFF Magic Numbers (Common Object File Format)
// Sourced from binutils/libbfd and file magic database
//

// x86/x64 Architectures
#define COFF_MAGIC_I386      0x014C  ///< Intel 386
#define COFF_MAGIC_I860      0x014D  ///< Intel 860
#define COFF_MAGIC_AMD64     0x8664  ///< AMD64/x86-64

// ARM Architectures
#define COFF_MAGIC_ARMV4     0x01C0  ///< ARMv4 little-endian
#define COFF_MAGIC_ARMV4T    0x01C2  ///< ARMv4T (Thumb)
#define COFF_MAGIC_ARMV7     0x01C4  ///< ARMv7
#define COFF_MAGIC_ARM64     0xAA64  ///< ARM64
#define COFF_MAGIC_ARM64X64  0xA641  ///< ARM64 (x86-64 ABI)
#define COFF_MAGIC_ARM64CLS  0xA64E  ///< ARM64 (classic + x86-64 ABI)
#define COFF_MAGIC_ARM64I386 0x3A64  ///< ARM64 (i386 ABI)

// MIPS Architectures
#define COFF_MAGIC_MIPS_R3000_BE  0x0160  ///< MIPS R3000 big-endian
#define COFF_MAGIC_MIPS_R3000     0x0162  ///< MIPS R3000 little-endian
#define COFF_MAGIC_MIPS_R4000     0x0166  ///< MIPS R4000
#define COFF_MAGIC_MIPS_R10000    0x0168  ///< MIPS R10000
#define COFF_MAGIC_MIPS_WCE       0x0169  ///< MIPS WCE v2
#define COFF_MAGIC_MIPS16         0x0266  ///< MIPS16
#define COFF_MAGIC_MIPS_FPU       0x0366  ///< MIPS with FPU
#define COFF_MAGIC_MIPS16_FPU     0x0466  ///< MIPS16 with FPU

// Alpha Architectures
#define COFF_MAGIC_ALPHA32   0x0184  ///< Alpha AXP 32-bit
#define COFF_MAGIC_ALPHA64   0x0284  ///< Alpha AXP 64-bit

// PowerPC Architectures
#define COFF_MAGIC_POWERPC_LE    0x01F0  ///< PowerPC 32-bit little-endian
#define COFF_MAGIC_POWERPC_FPU   0x01F1  ///< PowerPC 32-bit with FPU (LE)
#define COFF_MAGIC_POWERPC64_BE  0x01F2  ///< PowerPC 64-bit big-endian
#define COFF_MAGIC_POWERPC_BE    0x0601  ///< PowerPC 32-bit big-endian

// Motorola Architectures
#define COFF_MAGIC_M68K      0x0268  ///< Motorola 68000

// Hitachi SH Architectures
#define COFF_MAGIC_SH        0x01A2  ///< Hitachi SH
#define COFF_MAGIC_SH3       0x01A3  ///< Hitachi SH3
#define COFF_MAGIC_SH3DSP    0x01A4  ///< Hitachi SH3 DSP
#define COFF_MAGIC_SH4       0x01A6  ///< Hitachi SH4
#define COFF_MAGIC_SH5       0x01A8  ///< Hitachi SH5
#define COFF_MAGIC_SH_BE     0x0500  ///< Hitachi SH big-endian
#define COFF_MAGIC_SH_LE     0x0550  ///< Hitachi SH little-endian

// Other Architectures
#define COFF_MAGIC_ITANIUM   0x0200  ///< Intel Itanium
#define COFF_MAGIC_PARISC    0x0290  ///< HP PA-RISC

// RISC-V Architectures
#define COFF_MAGIC_RISCV32   0x5032  ///< RISC-V 32-bit
#define COFF_MAGIC_RISCV64   0x5064  ///< RISC-V 64-bit
#define COFF_MAGIC_RISCV128  0x5128  ///< RISC-V 128-bit

// LoongArch Architectures
#define COFF_MAGIC_LOONGARCH32  0x6232  ///< LoongArch 32-bit
#define COFF_MAGIC_LOONGARCH64  0x6264  ///< LoongArch 64-bit

//
// COFF File Header Flags
//

#define COFF_F_RELFLG   0x0001  ///< Relocation info stripped
#define COFF_F_EXEC     0x0002  ///< File is executable
#define COFF_F_LNNO     0x0004  ///< Line numbers stripped
#define COFF_F_LSYMS    0x0008  ///< Local symbols stripped

//
// COFF Section Flags
//

#define COFF_STYP_TEXT   0x0020  ///< Text (executable)
#define COFF_STYP_DATA   0x0040  ///< Data (initialized)
#define COFF_STYP_BSS    0x0080  ///< BSS (uninitialized)

//
// COFF Structures
//

ANX_PACK_PUSH(1)

typedef struct _COFF_FILE_HEADER {
  UINT16  Magic;              ///< Magic number
  UINT16  NumSections;        ///< Number of sections
  UINT32  TimeStamp;          ///< Time & date stamp
  UINT32  SymbolTablePtr;     ///< Symbol table file pointer
  UINT32  NumSymbols;         ///< Number of symbol table entries
  UINT16  OptHeaderSize;      ///< Size of optional header
  UINT16  Flags;              ///< Flags
} COFF_FILE_HEADER;

typedef struct _COFF_AOUT_HEADER {
  UINT16  Magic;              ///< Magic number (0407, 0410, 0413)
  UINT16  Version;            ///< Version stamp
  UINT32  TextSize;           ///< Text size in bytes
  UINT32  DataSize;           ///< Initialized data size
  UINT32  BssSize;            ///< Uninitialized data size
  UINT32  Entry;              ///< Entry point
  UINT32  TextStart;          ///< Base of text
  UINT32  DataStart;          ///< Base of data
} COFF_AOUT_HEADER;

typedef struct _COFF_SECTION_HEADER {
  CHAR8   Name[8];            ///< Section name
  UINT32  PhysicalAddr;       ///< Physical address
  UINT32  VirtualAddr;        ///< Virtual address
  UINT32  Size;               ///< Section size
  UINT32  DataPtr;            ///< File pointer to raw data
  UINT32  RelocPtr;           ///< File pointer to relocation
  UINT32  LinenoPtr;          ///< File pointer to line numbers
  UINT16  NumRelocs;          ///< Number of relocation entries
  UINT16  NumLinenos;         ///< Number of line number entries
  UINT32  Flags;              ///< Section flags
} COFF_SECTION_HEADER;

typedef struct _COFF_RELOC {
  UINT32  VirtualAddress;     ///< Address to apply relocation
  UINT32  SymbolTableIndex;   ///< Symbol table index
  UINT16  Type;               ///< Relocation type
} COFF_RELOC;

ANX_PACK_POP()

//
// COFF Relocation Types
//

// Generic/Common
#define COFF_RELOC_ABSOLUTE      0x0000  ///< No relocation

// x86 (i386)
#define COFF_RELOC_I386_DIR32    0x0006  ///< Direct 32-bit reference
#define COFF_RELOC_I386_DIR32NB  0x0007  ///< Direct 32-bit ref (no base)
#define COFF_RELOC_I386_SEG12    0x0009  ///< 16-bit segment reference
#define COFF_RELOC_I386_SECTION  0x000A  ///< Section index
#define COFF_RELOC_I386_SECREL   0x000B  ///< Section-relative
#define COFF_RELOC_I386_REL32    0x0014  ///< PC-relative 32-bit reference

// AMD64 (x86-64)
#define COFF_RELOC_AMD64_ADDR64  0x0001  ///< Direct 64-bit reference
#define COFF_RELOC_AMD64_ADDR32  0x0002  ///< Direct 32-bit reference
#define COFF_RELOC_AMD64_ADDR32NB 0x0003 ///< Direct 32-bit ref (no base)
#define COFF_RELOC_AMD64_REL32   0x0004  ///< PC-relative 32-bit reference
#define COFF_RELOC_AMD64_REL32_1 0x0005  ///< PC-relative 32-bit +1
#define COFF_RELOC_AMD64_REL32_2 0x0006  ///< PC-relative 32-bit +2
#define COFF_RELOC_AMD64_REL32_3 0x0007  ///< PC-relative 32-bit +3
#define COFF_RELOC_AMD64_REL32_4 0x0008  ///< PC-relative 32-bit +4
#define COFF_RELOC_AMD64_REL32_5 0x0009  ///< PC-relative 32-bit +5
#define COFF_RELOC_AMD64_SECTION 0x000A  ///< Section index
#define COFF_RELOC_AMD64_SECREL  0x000B  ///< Section-relative
#define COFF_RELOC_AMD64_SECREL7 0x000C  ///< 7-bit section offset
#define COFF_RELOC_AMD64_TOKEN   0x000D  ///< CLR token
#define COFF_RELOC_AMD64_SREL32  0x000E  ///< 32-bit signed span-dependent
#define COFF_RELOC_AMD64_PAIR    0x000F  ///< Relocation pair
#define COFF_RELOC_AMD64_SSPAN32 0x0010  ///< 32-bit signed span

// ARM
#define COFF_RELOC_ARM_ABSOLUTE  0x0000  ///< No relocation
#define COFF_RELOC_ARM_ADDR32    0x0001  ///< 32-bit address
#define COFF_RELOC_ARM_ADDR32NB  0x0002  ///< 32-bit address (no base)
#define COFF_RELOC_ARM_BRANCH24  0x0003  ///< 24-bit relative branch
#define COFF_RELOC_ARM_BRANCH11  0x0004  ///< Thumb 11-bit branch
#define COFF_RELOC_ARM_SECTION   0x000E  ///< Section index
#define COFF_RELOC_ARM_SECREL    0x000F  ///< Section-relative

// ARM64
#define COFF_RELOC_ARM64_ABSOLUTE 0x0000 ///< No relocation
#define COFF_RELOC_ARM64_ADDR32   0x0001 ///< 32-bit address
#define COFF_RELOC_ARM64_ADDR32NB 0x0002 ///< 32-bit address (no base)
#define COFF_RELOC_ARM64_BRANCH26 0x0003 ///< 26-bit relative branch
#define COFF_RELOC_ARM64_PAGEBASE_REL21 0x0004 ///< Page base + 21-bit offset
#define COFF_RELOC_ARM64_REL21    0x0005 ///< 21-bit relative
#define COFF_RELOC_ARM64_PAGEOFFSET_12A 0x0006 ///< 12-bit page offset (ADD)
#define COFF_RELOC_ARM64_PAGEOFFSET_12L 0x0007 ///< 12-bit page offset (LD/ST)
#define COFF_RELOC_ARM64_SECREL   0x0008 ///< Section-relative
#define COFF_RELOC_ARM64_SECREL_LOW12A 0x0009 ///< Low 12 bits of section offset (ADD)
#define COFF_RELOC_ARM64_SECREL_HIGH12A 0x000A ///< High 12 bits of section offset (ADD)
#define COFF_RELOC_ARM64_SECREL_LOW12L 0x000B ///< Low 12 bits of section offset (LD/ST)
#define COFF_RELOC_ARM64_ADDR64   0x000E ///< 64-bit address

// MIPS
#define COFF_RELOC_MIPS_ABSOLUTE  0x0000 ///< No relocation
#define COFF_RELOC_MIPS_REFHALF   0x0001 ///< 16-bit reference
#define COFF_RELOC_MIPS_REFWORD   0x0002 ///< 32-bit reference
#define COFF_RELOC_MIPS_JMPADDR   0x0003 ///< 26-bit jump address
#define COFF_RELOC_MIPS_REFHI     0x0004 ///< High 16 bits
#define COFF_RELOC_MIPS_REFLO     0x0005 ///< Low 16 bits
#define COFF_RELOC_MIPS_GPREL     0x0006 ///< GP-relative
#define COFF_RELOC_MIPS_LITERAL   0x0007 ///< Literal reference
#define COFF_RELOC_MIPS_SECTION   0x000A ///< Section index
#define COFF_RELOC_MIPS_SECREL    0x000B ///< Section-relative
#define COFF_RELOC_MIPS_REFWORDNB 0x0022 ///< 32-bit ref (no base)
#define COFF_RELOC_MIPS_PAIR      0x0025 ///< Relocation pair

// Alpha
#define COFF_RELOC_ALPHA_ABSOLUTE 0x0000 ///< No relocation
#define COFF_RELOC_ALPHA_REFLONG  0x0001 ///< 32-bit reference
#define COFF_RELOC_ALPHA_REFQUAD  0x0002 ///< 64-bit reference
#define COFF_RELOC_ALPHA_GPREL32  0x0003 ///< 32-bit GP-relative
#define COFF_RELOC_ALPHA_LITERAL  0x0004 ///< 16-bit literal
#define COFF_RELOC_ALPHA_LITUSE   0x0005 ///< Literal usage hint
#define COFF_RELOC_ALPHA_GPDISP   0x0006 ///< GP displacement
#define COFF_RELOC_ALPHA_BRADDR   0x0007 ///< 21-bit branch address
#define COFF_RELOC_ALPHA_HINT     0x0008 ///< Branch hint
#define COFF_RELOC_ALPHA_INLINE_RELOC_1 0x0009 ///< Inline reloc 1
#define COFF_RELOC_ALPHA_INLINE_RELOC_2 0x000A ///< Inline reloc 2
#define COFF_RELOC_ALPHA_INLINE_RELOC_3 0x000B ///< Inline reloc 3
#define COFF_RELOC_ALPHA_SECTION  0x000C ///< Section index
#define COFF_RELOC_ALPHA_SECREL   0x000D ///< Section-relative
#define COFF_RELOC_ALPHA_REFLONGNB 0x0010 ///< 32-bit ref (no base)
#define COFF_RELOC_ALPHA_PAIR     0x0012 ///< Relocation pair

// PowerPC
#define COFF_RELOC_PPC_ABSOLUTE   0x0000 ///< No relocation
#define COFF_RELOC_PPC_ADDR64     0x0001 ///< 64-bit address
#define COFF_RELOC_PPC_ADDR32     0x0002 ///< 32-bit address
#define COFF_RELOC_PPC_ADDR24     0x0003 ///< 24-bit address
#define COFF_RELOC_PPC_ADDR16     0x0004 ///< 16-bit address
#define COFF_RELOC_PPC_ADDR14     0x0005 ///< 14-bit address
#define COFF_RELOC_PPC_REL24      0x0006 ///< 24-bit relative branch
#define COFF_RELOC_PPC_REL14      0x0007 ///< 14-bit relative branch
#define COFF_RELOC_PPC_ADDR32NB   0x000A ///< 32-bit address (no base)
#define COFF_RELOC_PPC_SECREL     0x000B ///< Section-relative
#define COFF_RELOC_PPC_SECTION    0x000C ///< Section index
#define COFF_RELOC_PPC_SECREL16   0x000F ///< 16-bit section offset
#define COFF_RELOC_PPC_PAIR       0x0012 ///< Relocation pair

// Hitachi SH
#define COFF_RELOC_SH_ABSOLUTE    0x0000 ///< No relocation
#define COFF_RELOC_SH_DIRECT32    0x0001 ///< Direct 32-bit
#define COFF_RELOC_SH_PCREL8      0x0002 ///< PC-relative 8-bit
#define COFF_RELOC_SH_PCREL16     0x0003 ///< PC-relative 16-bit
#define COFF_RELOC_SH_HIGH16      0x0004 ///< High 16 bits
#define COFF_RELOC_SH_LOW16       0x0005 ///< Low 16 bits
#define COFF_RELOC_SH_PCREL32     0x0006 ///< PC-relative 32-bit
#define COFF_RELOC_SH_SECTION     0x000A ///< Section index
#define COFF_RELOC_SH_SECREL      0x000B ///< Section-relative
#define COFF_RELOC_SH_DIRECT32NB  0x0010 ///< Direct 32-bit (no base)

// Itanium (IA-64)
#define COFF_RELOC_IA64_ABSOLUTE  0x0000 ///< No relocation
#define COFF_RELOC_IA64_IMM14     0x0001 ///< 14-bit immediate
#define COFF_RELOC_IA64_IMM22     0x0002 ///< 22-bit immediate
#define COFF_RELOC_IA64_IMM64     0x0003 ///< 64-bit immediate
#define COFF_RELOC_IA64_DIR32     0x0004 ///< Direct 32-bit
#define COFF_RELOC_IA64_DIR64     0x0005 ///< Direct 64-bit
#define COFF_RELOC_IA64_PCREL21B  0x0006 ///< PC-relative 21-bit branch
#define COFF_RELOC_IA64_PCREL21M  0x0007 ///< PC-relative 21-bit move
#define COFF_RELOC_IA64_PCREL21F  0x0008 ///< PC-relative 21-bit float
#define COFF_RELOC_IA64_GPREL22   0x0009 ///< GP-relative 22-bit
#define COFF_RELOC_IA64_LTOFF22   0x000A ///< LT-relative 22-bit
#define COFF_RELOC_IA64_SECTION   0x000B ///< Section index
#define COFF_RELOC_IA64_SECREL22  0x000C ///< Section-relative 22-bit
#define COFF_RELOC_IA64_SECREL64I 0x000D ///< Section-relative 64-bit (immediate)
#define COFF_RELOC_IA64_SECREL32  0x000E ///< Section-relative 32-bit
#define COFF_RELOC_IA64_DIR32NB   0x0010 ///< Direct 32-bit (no base)
#define COFF_RELOC_IA64_ADDEND    0x001F ///< Addend relocation

//
// Helper Macros
//

#define COFF_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// IImageLoader Implementation for COFF
//

/**
  Detect if image is COFF format.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  COFF_FILE_HEADER *Header;
  UINT16 Magic;

  if (ImageSize < sizeof(COFF_FILE_HEADER)) {
    return S_FALSE;
  }

  Header = (COFF_FILE_HEADER *)ImageBase;
  Magic = Header->Magic;

  // Try both endianness interpretations
  UINT16 MagicSwapped = ANX_BSWAP16(Magic);

  // Check for known COFF machine types in both endianness
  switch (Magic) {
    // x86/x64
    case COFF_MAGIC_I386:
    case COFF_MAGIC_I860:
    case COFF_MAGIC_AMD64:
    // ARM
    case COFF_MAGIC_ARMV4:
    case COFF_MAGIC_ARMV4T:
    case COFF_MAGIC_ARMV7:
    case COFF_MAGIC_ARM64:
    case COFF_MAGIC_ARM64X64:
    case COFF_MAGIC_ARM64CLS:
    case COFF_MAGIC_ARM64I386:
    // MIPS
    case COFF_MAGIC_MIPS_R3000_BE:
    case COFF_MAGIC_MIPS_R3000:
    case COFF_MAGIC_MIPS_R4000:
    case COFF_MAGIC_MIPS_R10000:
    case COFF_MAGIC_MIPS_WCE:
    case COFF_MAGIC_MIPS16:
    case COFF_MAGIC_MIPS_FPU:
    case COFF_MAGIC_MIPS16_FPU:
    // Alpha
    case COFF_MAGIC_ALPHA32:
    case COFF_MAGIC_ALPHA64:
    // PowerPC
    case COFF_MAGIC_POWERPC_LE:
    case COFF_MAGIC_POWERPC_FPU:
    case COFF_MAGIC_POWERPC64_BE:
    case COFF_MAGIC_POWERPC_BE:
    // Motorola
    case COFF_MAGIC_M68K:
    // Hitachi SH
    case COFF_MAGIC_SH:
    case COFF_MAGIC_SH3:
    case COFF_MAGIC_SH3DSP:
    case COFF_MAGIC_SH4:
    case COFF_MAGIC_SH5:
    case COFF_MAGIC_SH_BE:
    case COFF_MAGIC_SH_LE:
    // Other
    case COFF_MAGIC_ITANIUM:
    case COFF_MAGIC_PARISC:
    // RISC-V
    case COFF_MAGIC_RISCV32:
    case COFF_MAGIC_RISCV64:
    case COFF_MAGIC_RISCV128:
    // LoongArch
    case COFF_MAGIC_LOONGARCH32:
    case COFF_MAGIC_LOONGARCH64:
      if (Header->Flags & COFF_F_EXEC) {
        return S_OK;
      }
      break;
  }

  // Try swapped endianness
  switch (MagicSwapped) {
    // x86/x64
    case COFF_MAGIC_I386:
    case COFF_MAGIC_I860:
    case COFF_MAGIC_AMD64:
    // ARM
    case COFF_MAGIC_ARMV4:
    case COFF_MAGIC_ARMV4T:
    case COFF_MAGIC_ARMV7:
    case COFF_MAGIC_ARM64:
    case COFF_MAGIC_ARM64X64:
    case COFF_MAGIC_ARM64CLS:
    case COFF_MAGIC_ARM64I386:
    // MIPS
    case COFF_MAGIC_MIPS_R3000_BE:
    case COFF_MAGIC_MIPS_R3000:
    case COFF_MAGIC_MIPS_R4000:
    case COFF_MAGIC_MIPS_R10000:
    case COFF_MAGIC_MIPS_WCE:
    case COFF_MAGIC_MIPS16:
    case COFF_MAGIC_MIPS_FPU:
    case COFF_MAGIC_MIPS16_FPU:
    // Alpha
    case COFF_MAGIC_ALPHA32:
    case COFF_MAGIC_ALPHA64:
    // PowerPC
    case COFF_MAGIC_POWERPC_LE:
    case COFF_MAGIC_POWERPC_FPU:
    case COFF_MAGIC_POWERPC64_BE:
    case COFF_MAGIC_POWERPC_BE:
    // Motorola
    case COFF_MAGIC_M68K:
    // Hitachi SH
    case COFF_MAGIC_SH:
    case COFF_MAGIC_SH3:
    case COFF_MAGIC_SH3DSP:
    case COFF_MAGIC_SH4:
    case COFF_MAGIC_SH5:
    case COFF_MAGIC_SH_BE:
    case COFF_MAGIC_SH_LE:
    // Other
    case COFF_MAGIC_ITANIUM:
    case COFF_MAGIC_PARISC:
    // RISC-V
    case COFF_MAGIC_RISCV32:
    case COFF_MAGIC_RISCV64:
    case COFF_MAGIC_RISCV128:
    // LoongArch
    case COFF_MAGIC_LOONGARCH32:
    case COFF_MAGIC_LOONGARCH64:
      if (ANX_BSWAP16(Header->Flags) & COFF_F_EXEC) {
        return S_OK;
      }
      break;
  }

  return S_FALSE;
}

/**
  Determine if COFF image is byte-swapped.
  Returns TRUE if the file is in a different endianness than the host.
**/
static
BOOLEAN
CoffIsSwapped (
  IN VOID  *ImageBase
  )
{
  COFF_FILE_HEADER *Header = (COFF_FILE_HEADER *)ImageBase;
  UINT16 Magic = Header->Magic;

  // Big-endian architectures (on LE host, these need swap)
  switch (Magic) {
    case COFF_MAGIC_M68K:
    case COFF_MAGIC_MIPS_R3000_BE:
    case COFF_MAGIC_POWERPC64_BE:
    case COFF_MAGIC_POWERPC_BE:
    case COFF_MAGIC_SH_BE:
      return FALSE;  // Native big-endian on BE host
  }

  // Check if we need to swap (magic appears in wrong endianness)
  UINT16 MagicSwapped = ANX_BSWAP16(Magic);
  switch (MagicSwapped) {
    case COFF_MAGIC_M68K:
    case COFF_MAGIC_MIPS_R3000_BE:
    case COFF_MAGIC_POWERPC64_BE:
    case COFF_MAGIC_POWERPC_BE:
    case COFF_MAGIC_SH_BE:
      return TRUE;  // Needs byte swap
  }

  return FALSE;  // Little-endian or matches host
}

/**
  Get architecture from COFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  COFF_FILE_HEADER *Header;
  UINT16 Magic;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  Header = (COFF_FILE_HEADER *)ImageBase;
  Magic = Header->Magic;

  // Handle byte-swapped images
  if (CoffIsSwapped(ImageBase)) {
    Magic = ANX_BSWAP16(Magic);
  }

  switch (Magic) {
    // x86/x64
    case COFF_MAGIC_I386:
      *Architecture = ARCH_386;
      break;
    case COFF_MAGIC_I860:
      *Architecture = ARCH_I860;
      break;
    case COFF_MAGIC_AMD64:
      *Architecture = ARCH_AMD64;
      break;

    // ARM
    case COFF_MAGIC_ARMV4:
    case COFF_MAGIC_ARMV4T:
    case COFF_MAGIC_ARMV7:
      *Architecture = ARCH_ARM;
      break;
    case COFF_MAGIC_ARM64:
    case COFF_MAGIC_ARM64X64:
    case COFF_MAGIC_ARM64CLS:
    case COFF_MAGIC_ARM64I386:
      *Architecture = ARCH_ARM64;
      break;

    // MIPS
    case COFF_MAGIC_MIPS_R3000_BE:
    case COFF_MAGIC_MIPS_R3000:
    case COFF_MAGIC_MIPS16:
      *Architecture = ARCH_MIPS32;
      break;
    case COFF_MAGIC_MIPS_R4000:
    case COFF_MAGIC_MIPS_R10000:
    case COFF_MAGIC_MIPS_WCE:
    case COFF_MAGIC_MIPS_FPU:
    case COFF_MAGIC_MIPS16_FPU:
      *Architecture = ARCH_MIPS64;
      break;

    // Alpha
    case COFF_MAGIC_ALPHA32:
    case COFF_MAGIC_ALPHA64:
      *Architecture = ARCH_ALPHA;
      break;

    // PowerPC
    case COFF_MAGIC_POWERPC_LE:
    case COFF_MAGIC_POWERPC_FPU:
    case COFF_MAGIC_POWERPC_BE:
      *Architecture = ARCH_PPC32;
      break;
    case COFF_MAGIC_POWERPC64_BE:
      *Architecture = ARCH_PPC64;
      break;

    // Motorola
    case COFF_MAGIC_M68K:
      *Architecture = ARCH_M68K;
      break;

    // Hitachi SH
    case COFF_MAGIC_SH:
    case COFF_MAGIC_SH3:
    case COFF_MAGIC_SH3DSP:
    case COFF_MAGIC_SH4:
    case COFF_MAGIC_SH5:
    case COFF_MAGIC_SH_BE:
    case COFF_MAGIC_SH_LE:
      *Architecture = ARCH_SH;
      break;

    // Other
    case COFF_MAGIC_ITANIUM:
      *Architecture = ARCH_IA64;
      break;
    case COFF_MAGIC_PARISC:
      *Architecture = ARCH_PARISC;
      break;

    // RISC-V
    case COFF_MAGIC_RISCV32:
      *Architecture = ARCH_RISCV32;
      break;
    case COFF_MAGIC_RISCV64:
      *Architecture = ARCH_RISCV64;
      break;
    case COFF_MAGIC_RISCV128:
      *Architecture = ARCH_RISCV128;
      break;

    // LoongArch
    case COFF_MAGIC_LOONGARCH32:
      *Architecture = ARCH_LOONGARCH32;
      break;
    case COFF_MAGIC_LOONGARCH64:
      *Architecture = ARCH_LOONGARCH64;
      break;

    default:
      *Architecture = ARCH_UNSUPPORTED;
      return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  return S_OK;
}

/**
  Get endianness from COFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  COFF_FILE_HEADER *Header;
  UINT16 Magic;

  if (Endianness == NULL) {
    return E_POINTER;
  }

  Header = (COFF_FILE_HEADER *)ImageBase;
  Magic = Header->Magic;

  // Handle byte-swapped images
  if (CoffIsSwapped(ImageBase)) {
    Magic = ANX_BSWAP16(Magic);
  }

  // Determine endianness based on architecture
  switch (Magic) {
    // Big-endian architectures
    case COFF_MAGIC_M68K:
    case COFF_MAGIC_MIPS_R3000_BE:
    case COFF_MAGIC_POWERPC64_BE:
    case COFF_MAGIC_POWERPC_BE:
    case COFF_MAGIC_SH_BE:
      *Endianness = ImgEndianBig;
      break;

    // Little-endian architectures
    case COFF_MAGIC_I386:
    case COFF_MAGIC_I860:
    case COFF_MAGIC_AMD64:
    case COFF_MAGIC_ARMV4:
    case COFF_MAGIC_ARMV4T:
    case COFF_MAGIC_ARMV7:
    case COFF_MAGIC_ARM64:
    case COFF_MAGIC_ARM64X64:
    case COFF_MAGIC_ARM64CLS:
    case COFF_MAGIC_ARM64I386:
    case COFF_MAGIC_MIPS_R3000:
    case COFF_MAGIC_MIPS_R4000:
    case COFF_MAGIC_MIPS_R10000:
    case COFF_MAGIC_MIPS_WCE:
    case COFF_MAGIC_MIPS16:
    case COFF_MAGIC_MIPS_FPU:
    case COFF_MAGIC_MIPS16_FPU:
    case COFF_MAGIC_ALPHA32:
    case COFF_MAGIC_ALPHA64:
    case COFF_MAGIC_POWERPC_LE:
    case COFF_MAGIC_POWERPC_FPU:
    case COFF_MAGIC_SH:
    case COFF_MAGIC_SH3:
    case COFF_MAGIC_SH3DSP:
    case COFF_MAGIC_SH4:
    case COFF_MAGIC_SH5:
    case COFF_MAGIC_SH_LE:
    case COFF_MAGIC_ITANIUM:
    case COFF_MAGIC_PARISC:
    case COFF_MAGIC_RISCV32:
    case COFF_MAGIC_RISCV64:
    case COFF_MAGIC_RISCV128:
    case COFF_MAGIC_LOONGARCH32:
    case COFF_MAGIC_LOONGARCH64:
      *Endianness = ImgEndianLittle;
      break;

    default:
      *Endianness = ImgEndianUnknown;
      return IMGLOAD_E_INVALID_FORMAT;
  }

  return S_OK;
}

/**
  Get entry point from COFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  COFF_FILE_HEADER *FileHeader;
  COFF_AOUT_HEADER *AoutHeader;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  FileHeader = (COFF_FILE_HEADER *)ImageBase;

  if (FileHeader->OptHeaderSize >= sizeof(COFF_AOUT_HEADER)) {
    AoutHeader = (COFF_AOUT_HEADER *)(FileHeader + 1);
    *EntryPoint = AoutHeader->Entry;
    return S_OK;
  }

  *EntryPoint = 0;
  return IMGLOAD_E_INVALID_HEADER;
}

/**
  Load COFF image segments.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffLoadImage (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  COFF_FILE_HEADER *FileHeader;
  COFF_SECTION_HEADER *Sections;
  UINT16 i;
  UINTN SectionsOffset;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  FileHeader = (COFF_FILE_HEADER *)ImageBase;

  info("Loading COFF executable...");

  // Sections follow the optional header
  SectionsOffset = sizeof(COFF_FILE_HEADER) + FileHeader->OptHeaderSize;
  Sections = (COFF_SECTION_HEADER *)COFF_OFF(SectionsOffset);

  // Load all sections
  for (i = 0; i < FileHeader->NumSections; i++) {
    COFF_SECTION_HEADER *Sec = &Sections[i];
    BOOLEAN IsText = !!(Sec->Flags & COFF_STYP_TEXT);
    BOOLEAN IsData = !!(Sec->Flags & COFF_STYP_DATA);
    BOOLEAN IsBss = !!(Sec->Flags & COFF_STYP_BSS);

    info("  Section %.8s at 0x%08x (size: 0x%08x, flags: 0x%08x)",
         Sec->Name, Sec->VirtualAddr, Sec->Size, Sec->Flags);

    if (Sec->Size == 0) {
      continue;
    }

    if (IsBss) {
      // BSS: zero-filled, writable
      VirtualAddressMemset(
        Sec->VirtualAddr,
        0,
        Sec->Size,
        Context->IsUserMode,
        TRUE,   // Writable
        FALSE   // Not executable
      );
    } else if (Sec->DataPtr > 0) {
      // Normal section with data
      VirtualAddressCopy(
        Sec->VirtualAddr,
        COFF_OFF(Sec->DataPtr),
        Sec->Size,
        Context->IsUserMode,
        !IsText,  // Writable if not text
        IsText    // Executable if text
      );
    }
  }

  return S_OK;
}

/**
  Extract TLS information from COFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // COFF doesn't have explicit TLS support
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from COFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  // COFF doesn't have standard unwinding information
  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetSymbolByAddress (
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
CoffGetSymbolByName (
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
  Extract relocation information from COFF image.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffGetRelocInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_RELOC_INFO   *RelocInfo
  )
{
  COFF_FILE_HEADER *Header;

  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  Header = (COFF_FILE_HEADER *)ImageBase;

  // Check if relocations are stripped
  if (Header->Flags & COFF_F_RELFLG) {
    RelocInfo->RequiresReloc = FALSE;
    return S_FALSE;
  }

  // COFF has per-section relocations
  RelocInfo->Format = 9;  // COFF format
  RelocInfo->RequiresReloc = TRUE;

  return S_OK;
}

/**
  Apply relocations to COFF image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffApplyRelocations (
  IN VOID              *ImageBase,
  IN VIRTUAL_ADDRESS   LoadAddress,
  IN VIRTUAL_ADDRESS   PreferredBase
  )
{
  COFF_FILE_HEADER *Header;
  COFF_SECTION_HEADER *Sections;
  COFF_RELOC *Relocs;
  INT64 Delta;
  UINT16 i, j;
  UINTN SectionsOffset;
  BOOLEAN NeedSwap;
  ARCH Arch;
  HRESULT Status;

  Header = (COFF_FILE_HEADER *)ImageBase;
  NeedSwap = CoffIsSwapped(ImageBase);

  // Calculate relocation delta
  Delta = (INT64)LoadAddress - (INT64)PreferredBase;

  if (Delta == 0) {
    return S_OK;  // No relocation needed
  }

  // Check if relocations are stripped
  UINT16 Flags = NeedSwap ? ANX_BSWAP16(Header->Flags) : Header->Flags;
  if (Flags & COFF_F_RELFLG) {
    return S_OK;  // No relocations available
  }

  // Get architecture to determine relocation types
  Status = CoffGetArch(NULL, ImageBase, &Arch);
  if (FAILED(Status)) {
    return Status;
  }

  // Get sections
  UINT16 OptHeaderSize = NeedSwap ? ANX_BSWAP16(Header->OptHeaderSize) : Header->OptHeaderSize;
  UINT16 NumSections = NeedSwap ? ANX_BSWAP16(Header->NumSections) : Header->NumSections;

  SectionsOffset = sizeof(COFF_FILE_HEADER) + OptHeaderSize;
  Sections = (COFF_SECTION_HEADER *)COFF_OFF(SectionsOffset);

  // Process relocations for each section
  for (i = 0; i < NumSections; i++) {
    COFF_SECTION_HEADER *Sec = &Sections[i];
    UINT16 NumRelocs = NeedSwap ? ANX_BSWAP16(Sec->NumRelocs) : Sec->NumRelocs;
    UINT32 RelocPtr = NeedSwap ? ANX_BSWAP32(Sec->RelocPtr) : Sec->RelocPtr;

    if (NumRelocs == 0 || RelocPtr == 0) {
      continue;
    }

    Relocs = (COFF_RELOC *)COFF_OFF(RelocPtr);

    for (j = 0; j < NumRelocs; j++) {
      UINT32 RelocVa = NeedSwap ? ANX_BSWAP32(Relocs[j].VirtualAddress) : Relocs[j].VirtualAddress;
      UINT16 RelocType = NeedSwap ? ANX_BSWAP16(Relocs[j].Type) : Relocs[j].Type;

      // Apply relocation based on type and architecture
      switch (Arch) {
        case ARCH_386:
          switch (RelocType) {
            case COFF_RELOC_I386_DIR32:
            case COFF_RELOC_I386_DIR32NB: {
              UINT32 *Target = (UINT32 *)COFF_OFF(RelocVa);
              *Target = (UINT32)(*Target + Delta);
              break;
            }
            case COFF_RELOC_ABSOLUTE:
              // No operation
              break;
          }
          break;

        case ARCH_AMD64:
          switch (RelocType) {
            case COFF_RELOC_AMD64_ADDR64: {
              UINT64 *Target = (UINT64 *)COFF_OFF(RelocVa);
              *Target = *Target + Delta;
              break;
            }
            case COFF_RELOC_AMD64_ADDR32:
            case COFF_RELOC_AMD64_ADDR32NB: {
              UINT32 *Target = (UINT32 *)COFF_OFF(RelocVa);
              *Target = (UINT32)(*Target + Delta);
              break;
            }
            case COFF_RELOC_ABSOLUTE:
              // No operation
              break;
          }
          break;

        case ARCH_ARM:
          switch (RelocType) {
            case COFF_RELOC_ARM_ADDR32:
            case COFF_RELOC_ARM_ADDR32NB: {
              UINT32 *Target = (UINT32 *)COFF_OFF(RelocVa);
              *Target = (UINT32)(*Target + Delta);
              break;
            }
            case COFF_RELOC_ARM_ABSOLUTE:
              // No operation
              break;
          }
          break;

        case ARCH_ARM64:
          switch (RelocType) {
            case COFF_RELOC_ARM64_ADDR64: {
              UINT64 *Target = (UINT64 *)COFF_OFF(RelocVa);
              *Target = *Target + Delta;
              break;
            }
            case COFF_RELOC_ARM64_ADDR32:
            case COFF_RELOC_ARM64_ADDR32NB: {
              UINT32 *Target = (UINT32 *)COFF_OFF(RelocVa);
              *Target = (UINT32)(*Target + Delta);
              break;
            }
            case COFF_RELOC_ARM64_ABSOLUTE:
              // No operation
              break;
          }
          break;

        case ARCH_MIPS32:
        case ARCH_MIPS64:
          switch (RelocType) {
            case COFF_RELOC_MIPS_REFWORD:
            case COFF_RELOC_MIPS_REFWORDNB: {
              UINT32 *Target = (UINT32 *)COFF_OFF(RelocVa);
              *Target = (UINT32)(*Target + Delta);
              break;
            }
            case COFF_RELOC_MIPS_REFHALF: {
              UINT16 *Target = (UINT16 *)COFF_OFF(RelocVa);
              *Target = (UINT16)(*Target + Delta);
              break;
            }
            case COFF_RELOC_MIPS_ABSOLUTE:
              // No operation
              break;
          }
          break;

        case ARCH_ALPHA:
          switch (RelocType) {
            case COFF_RELOC_ALPHA_REFQUAD: {
              UINT64 *Target = (UINT64 *)COFF_OFF(RelocVa);
              *Target = *Target + Delta;
              break;
            }
            case COFF_RELOC_ALPHA_REFLONG:
            case COFF_RELOC_ALPHA_REFLONGNB: {
              UINT32 *Target = (UINT32 *)COFF_OFF(RelocVa);
              *Target = (UINT32)(*Target + Delta);
              break;
            }
            case COFF_RELOC_ALPHA_ABSOLUTE:
              // No operation
              break;
          }
          break;

        case ARCH_PPC32:
          switch (RelocType) {
            case COFF_RELOC_PPC_ADDR32:
            case COFF_RELOC_PPC_ADDR32NB: {
              UINT32 *Target = (UINT32 *)COFF_OFF(RelocVa);
              *Target = (UINT32)(*Target + Delta);
              break;
            }
            case COFF_RELOC_PPC_ADDR24: {
              // 24-bit address relocation
              UINT32 *Target = (UINT32 *)COFF_OFF(RelocVa);
              UINT32 Addr = (*Target & 0x03FFFFFC) + (UINT32)Delta;
              *Target = (*Target & 0xFC000003) | (Addr & 0x03FFFFFC);
              break;
            }
            case COFF_RELOC_PPC_ADDR16: {
              UINT16 *Target = (UINT16 *)COFF_OFF(RelocVa);
              *Target = (UINT16)(*Target + Delta);
              break;
            }
            case COFF_RELOC_PPC_ABSOLUTE:
              // No operation
              break;
          }
          break;

        case ARCH_PPC64:
          switch (RelocType) {
            case COFF_RELOC_PPC_ADDR64: {
              UINT64 *Target = (UINT64 *)COFF_OFF(RelocVa);
              *Target = *Target + Delta;
              break;
            }
            case COFF_RELOC_PPC_ADDR32:
            case COFF_RELOC_PPC_ADDR32NB: {
              UINT32 *Target = (UINT32 *)COFF_OFF(RelocVa);
              *Target = (UINT32)(*Target + Delta);
              break;
            }
            case COFF_RELOC_PPC_ABSOLUTE:
              // No operation
              break;
          }
          break;

        case ARCH_SH:
          switch (RelocType) {
            case COFF_RELOC_SH_DIRECT32:
            case COFF_RELOC_SH_DIRECT32NB: {
              UINT32 *Target = (UINT32 *)COFF_OFF(RelocVa);
              *Target = (UINT32)(*Target + Delta);
              break;
            }
            case COFF_RELOC_SH_ABSOLUTE:
              // No operation
              break;
          }
          break;

        case ARCH_IA64:
          switch (RelocType) {
            case COFF_RELOC_IA64_DIR64: {
              UINT64 *Target = (UINT64 *)COFF_OFF(RelocVa);
              *Target = *Target + Delta;
              break;
            }
            case COFF_RELOC_IA64_DIR32:
            case COFF_RELOC_IA64_DIR32NB: {
              UINT32 *Target = (UINT32 *)COFF_OFF(RelocVa);
              *Target = (UINT32)(*Target + Delta);
              break;
            }
            case COFF_RELOC_IA64_ABSOLUTE:
              // No operation
              break;
          }
          break;

        default:
          // Unsupported architecture for relocations
          break;
      }
    }
  }

  return S_OK;
}

/**
  IUnknown::QueryInterface implementation.
**/
static
HRESULT
STDMETHODCALLTYPE
CoffQueryInterface (
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
CoffAddRef (
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
CoffRelease (
  IN IImageLoader  *This
  )
{
  return 1;
}

//
// COFF Loader VTable
//

static CONST IImageLoaderVtbl gCoffVtbl = {
  CoffQueryInterface,
  CoffAddRef,
  CoffRelease,
  CoffDetect,
  CoffGetArch,
  CoffGetEndianness,
  CoffGetEntryPoint,
  CoffLoadImage,
  CoffGetTlsInfo,
  CoffGetUnwindInfo,
  CoffGetSymbolByAddress,
  CoffGetSymbolByName,
  CoffGetRelocInfo,
  CoffApplyRelocations
};

//
// COFF Loader Instance
//

IImageLoader gCoffLoader = {
  &gCoffVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gCoffLoader);
