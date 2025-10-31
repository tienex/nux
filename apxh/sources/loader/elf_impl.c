/** @file
  APXH ELF Loader

  Provides ELF (Executable and Linkable Format) parsing and loading
  for 32-bit and 64-bit executables. Handles program headers for
  kernel and user-space segments, including LOAD, TLS, unwinding info,
  and symbol lookup. Also supports custom APXH segment types.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org
  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

typedef struct elf32ph
{
  UINT32 Type;
  UINT32 Off;
  UINT32 Va;
  UINT32 Pa;
  UINT32 Fsize;
  UINT32 Msize;
  UINT32 Flags;
  UINT32 Align;
} ELF32_PH;

typedef struct elf64ph
{
  UINT32 Type;
  UINT32 Flags;
  UINT64 Off;
  UINT64 Va;
  UINT64 Pa;
  UINT64 Fsize;
  UINT64 Msize;
  UINT64 Align;
} ELF64_PH;

typedef struct elf32sh
{
  UINT32 Name;
  UINT32 Type;
  UINT32 Flags;
  UINT32 Addr;
  UINT32 Off;
  UINT32 Size;
  UINT32 Lnk;
  UINT32 Info;
  UINT32 Align;
  UINT32 ShentSize;
} ELF32_SH;

typedef struct elf64sh
{
  UINT32 Name;
  UINT32 Type;
  UINT64 Flags;
  UINT64 Addr;
  UINT64 Off;
  UINT64 Size;
  UINT32 Lnk;
  UINT32 Info;
  UINT64 Align;
  UINT64 ShentSize;
} ELF64_SH;

typedef struct elf32hdr

ANX_PACK_PUSH(1)
{
  UINT8  Id[16];
  UINT16 Type;
  UINT16 Mach;
  UINT32 Ver;
  UINT32 Entry;
  UINT32 Phoff;
  UINT32 Shoff;
  UINT32 Flags;
  UINT16 EhSize;
  UINT16 PhentSize;
  UINT16 Phs;
  UINT16 ShentSize;
  UINT16 Shs;
  UINT16 Shstrndx;
} ELF32_HDR;
ANX_PACK_POP()

typedef struct elf64hdr

ANX_PACK_PUSH(1)
{
  UINT8  Id[16];
  UINT16 Type;
  UINT16 Mach;
  UINT32 Ver;
  UINT64 Entry;
  UINT64 Phoff;
  UINT64 Shoff;
  UINT32 Flags;
  UINT16 EhSize;
  UINT16 PhentSize;
  UINT16 Phs;
  UINT16 ShentSize;
  UINT16 Shs;
  UINT16 Shstrndx;
} ELF64_HDR;
ANX_PACK_POP()

#define ET_EXEC		2
#define EM_386		3
#define EM_X86_64	62
#define EM_RISCV	0xf3
#define EV_CURRENT	1

#define SHT_PROGBITS 	1
#define SHT_NOBITS	8

#define PHT_NULL	0
#define PHT_LOAD	1
#define PHT_DYNAMIC	2
#define PHT_INTERP	3
#define PHT_NOTE	4
#define PHT_SHLIB	5
#define PHT_PHDR	6
#define PHT_TLS		7

#define PHF_X		1
#define PHF_W		2
#define PHF_R		4

#define ELFOFF(_o) ((VOID *)(UINTN)(ElfImg + (_o)))

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
VOID
PhUload (
  IN VOID    *ElfImage,
  IN UINT32  Type,
  IN UINT32  Flags,
  IN UINT64  Va,
  IN UINT64  MSize,
  IN UINT64  Off,
  IN UINT64  FSize
  )
{
  switch (Type)
    {
    case PHT_LOAD:
      /* Normal load segment. */
      if (Va + MSize < Va)
	{
	  printf ("size of PH too big.");
	  exit (-1);
	}

      if (FSize)
	{
	  /*
	     memcpy() to user and populate on fault.
	   */
	  VirtualAddressCopy (Va, ELFOFF (Off), FSize, 1,
		   !!(Flags & PHF_W), !!(Flags & PHF_X));
	}

      if (MSize - FSize > 0)
	{
	  /*
	     memset() to user and populate on fault.
	   */
	  VirtualAddressMemset (Va + FSize, 0, MSize - FSize, 1,
		     !!(Flags & PHF_W), !!(Flags & PHF_X));
	}
      break;
    case PHT_TLS:
      /* User Thread Local Storage. */
      if (MSize != 0)
	{
	  printf ("USER TLS area at %08" PRIx64 " (initsize: %" PRId64
		  " size: %" PRId64 ").\n", Va, FSize, MSize);

	  if (Va + MSize < Va)
	    {
	      printf ("size of PH too big.");
	      exit (-1);
	    }

	  if (FSize != 0)
	    {
	      VirtualAddressCopy (Va, ELFOFF (Off), FSize, 0,
		       !!(Flags & PHF_W), !!(Flags & PHF_X));
	    }
	  VirtualAddressMapUserTls (Va, FSize, MSize);
	}
    default:
      printf ("Ignored segment type %08lx.\n", Type);
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
VOID
PhKload (
  IN VOID    *ElfImage,
  IN UINT32  Type,
  IN UINT32  Flags,
  IN UINT64  Va,
  IN UINT64  MSize,
  IN UINT64  Off,
  IN UINT64  FSize
  )
{
  switch (Type)
    {
    case PHT_LOAD:
      /* Normal load segment. */
      if (Va + MSize < Va)
	{
	  printf ("size of PH too big.");
	  exit (-1);
	}

      if (FSize)
	{
	  /*
	     memcpy() to user and populate on fault.
	   */
	  VirtualAddressCopy (Va, ELFOFF (Off), FSize, 0,
		   !!(Flags & PHF_W), !!(Flags & PHF_X));
	}

      if (MSize - FSize > 0)
	{
	  /*
	     memset() to user and populate on fault.
	   */
	  VirtualAddressMemset (Va + FSize, 0, MSize - FSize, 0,
		     !!(Flags & PHF_W), !!(Flags & PHF_X));
	}
      break;
    case PHT_TLS:
      /* Thread Local Storage. */
      if (MSize != 0)
	{
	  printf ("TLS area at %08" PRIx64 " (initsize: %" PRId64 " size: %"
		  PRId64 ").\n", Va, FSize, MSize);
	  if (Va + MSize < Va)
	    {
	      printf ("size of PH too big.");
	      exit (-1);
	    }
	  if (FSize != 0)
	    {
	      VirtualAddressCopy (Va, ELFOFF (Off), FSize, 0,
		       !!(Flags & PHF_W), !!(Flags & PHF_X));
	    }
	  VirtualAddressMapKernelTls (Va, FSize, MSize);
	}
      break;

    case ApxhProgramHeaderInfo:
      /* Boot Information segment. */
      printf ("Boot Information area at %" PRIx64 " (size: %" PRId64 "d).\n",
	      Va, MSize);
      VirtualAddressMapInfo (Va, MSize);
      break;
    case ApxhProgramHeaderPhysicalMap:
      /* Direct 1:1 PA mapping. */
      printf ("Physmap VA area at %" PRIx64 " (size: %" PRId64 ").\n", Va,
	      MSize);
      VirtualAddressMapPhysical (Va, MSize, MEMTYPE_WB);
      break;
    case ApxhProgramHeaderEmpty:
      printf ("Empty VA area at %" PRIx64 " (size: %" PRId64 ").\n", Va,
	      MSize);
      /* Just VA allocation. Leave it. */
      break;
    case ApxhProgramHeaderPageTableAlloc:
      printf ("PT Alloc VA area at %" PRIx64 " (size: %" PRId64 ").\n", Va,
	      MSize);
      VirtualAddressAllocatePageTable (Va, MSize);
      break;
    case ApxhProgramHeaderPfnMap:
      printf ("PFN Map at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VirtualAddressMapPageFrameNumbers (Va, MSize);
      break;
    case ApxhProgramHeaderBatree:
      printf ("S-Tree at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VirtualAddressMapBatree (Va, MSize);
      break;
    case ApxhProgramHeaderLinear:
      printf ("Linear Map at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VirtualAddressMapLinear (Va, MSize);
      break;
    case ApxhProgramHeaderFramebuffer:
      printf ("Framebuffer Map at %" PRIx64 " (size: %" PRId64 ").\n", Va,
	      MSize);
      VirtualAddressMapFramebuffer (Va, MSize, MEMTYPE_WC);
      break;
    case ApxhProgramHeaderRegions:
      printf ("Region Map at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VirtualAddressMapRegions (Va, MSize);
      break;
    case ApxhProgramHeaderTopPageTableAlloc:
      printf ("TOP PT Alloc VA area at %" PRIx64 " (size: %" PRId64 ").\n",
	      Va, MSize);
      VirtualAddressAllocateTopPageTable (Va, MSize);
      break;
    default:
      printf ("Ignored segment type %08lx.\n", Type);
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
VIRTUAL_ADDRESS
LoadElf32 (
  IN VOID    *ElfImage,
  IN INT32   User
  )
{
  INT32 i;

  CHAR8 ElfId[] = { 0x7f, 'E', 'L', 'F', };
  ELF32_HDR *ElfHeader = (ELF32_HDR *) ElfImage;
  ELF32_PH *ProgramHeader = (ELF32_PH *) ELFOFF (ElfHeader->Phoff);

  if (memcmp (ElfHeader->Id, ElfId, 4) != 0)
    return (UINTN) - 1;

  if (ElfHeader->Type != ET_EXEC || ElfHeader->Ver != EV_CURRENT)
    return (UINTN) - 1;

  for (i = 0; i < ElfHeader->Phs; i++, ProgramHeader++)
    {
      if (User)
	PhUload (ElfImage, ProgramHeader->Type, ProgramHeader->Flags, ProgramHeader->Va, ProgramHeader->Msize, ProgramHeader->Off,
		  ProgramHeader->Fsize);
      else
	PhKload (ElfImage, ProgramHeader->Type, ProgramHeader->Flags, ProgramHeader->Va, ProgramHeader->Msize, ProgramHeader->Off,
		  ProgramHeader->Fsize);
    }

  return (VIRTUAL_ADDRESS) ElfHeader->Entry;
}

/**
  Load 64-bit ELF image.

  Parses and loads a 64-bit ELF executable, processing all program
  headers for kernel or user-space.

  @param[in] ElfImg  Pointer to ELF image.
  @param[in] User     TRUE for user-space, FALSE for kernel.

  @return Entry point virtual address, or -1 on error.
**/
VIRTUAL_ADDRESS
LoadElf64 (
  IN VOID    *ElfImage,
  IN INT32   User
  )
{
  INT32 i;

  CHAR8 ElfId[] = { 0x7f, 'E', 'L', 'F', };
  ELF64_HDR *ElfHeader = (ELF64_HDR *) ElfImage;
  ELF64_PH *ProgramHeader = (ELF64_PH *) ELFOFF (ElfHeader->Phoff);

  if (memcmp (ElfHeader->Id, ElfId, 4) != 0)
    return (UINTN) - 1;

  if (ElfHeader->Type != ET_EXEC || ElfHeader->Ver != EV_CURRENT)
    return (UINTN) - 1;

  for (i = 0; i < ElfHeader->Phs; i++, ProgramHeader++)
    {
      if (User)
	PhUload (ElfImage, ProgramHeader->Type, ProgramHeader->Flags, ProgramHeader->Va, ProgramHeader->Msize, ProgramHeader->Off,
		  ProgramHeader->Fsize);
      else
	PhKload (ElfImage, ProgramHeader->Type, ProgramHeader->Flags, ProgramHeader->Va, ProgramHeader->Msize, ProgramHeader->Off,
		  ProgramHeader->Fsize);
    }

  return (VIRTUAL_ADDRESS) ElfHeader->Entry;
}

/**
  Get ELF architecture.

  Determines the target architecture from ELF machine type.

  @param[in] ElfImg  Pointer to ELF image.

  @return Architecture type, or ARCH_INVALID/ARCH_UNSUPPORTED.
**/
ARCH
GetElfArch (
  IN VOID  *ElfImage
  )
{
  CHAR8 ElfId[] = { 0x7f, 'E', 'L', 'F', };
  ELF32_HDR *ElfHeader = (ELF32_HDR *) ElfImage;

  if (memcmp (ElfHeader->Id, ElfId, 4) != 0)
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
IMGLOAD_ENDIAN
GetElfEndianness (
  IN VOID  *ElfImage
  )
{
  CHAR8 ElfId[] = { 0x7f, 'E', 'L', 'F', };
  UINT8 *Ident = (UINT8 *) ElfImage;

  // Verify ELF magic
  if (memcmp (Ident, ElfId, 4) != 0)
    return ImgEndianUnknown;

  // ELF identification byte 5 (EI_DATA) specifies endianness
  // 1 = ELFDATA2LSB (little-endian)
  // 2 = ELFDATA2MSB (big-endian)
  switch (Ident[5]) {
    case 1:  // ELFDATA2LSB
      return ImgEndianLittle;
    case 2:  // ELFDATA2MSB
      return ImgEndianBig;
    default:
      return ImgEndianUnknown;
  }
}

//
// Section header types and program header types for unwinding info
//

#define SHT_PROGBITS_EH_FRAME 0x6474e550  ///< PT_GNU_EH_FRAME
#define SHT_EH_FRAME_NAME     ".eh_frame"

/**
  Get unwinding information from 32-bit ELF image.

  @param[in]  ElfImage    Pointer to ELF image.
  @param[out] UnwindInfo  Receives unwinding information.

  @return S_OK on success, S_FALSE if no unwinding info, error code otherwise.
**/
HRESULT
GetElf32UnwindInfo (
  IN  VOID                 *ElfImage,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  ELF32_HDR *ElfHeader = (ELF32_HDR *)ElfImage;
  ELF32_PH *ProgramHeader;
  ELF32_SH *SectionHeader;
  INT32 i;

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  // First, check program headers for PT_GNU_EH_FRAME
  ProgramHeader = (ELF32_PH *)ELFOFF(ElfHeader->Phoff);
  for (i = 0; i < ElfHeader->Phs; i++, ProgramHeader++) {
    if (ProgramHeader->Type == 0x6474e550) {  // PT_GNU_EH_FRAME
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

    for (i = 0; i < ElfHeader->Shs; i++, SectionHeader++) {
      CHAR8 *SectionName = &StringTable[SectionHeader->Name];
      if (strcmp(SectionName, ".eh_frame") == 0) {
        UnwindInfo->UnwindDataAddr = SectionHeader->Addr;
        UnwindInfo->UnwindDataSize = SectionHeader->Size;
        UnwindInfo->Format = 0;  // DWARF eh_frame
        return S_OK;
      }
    }
  }

  return S_FALSE;  // No unwinding info found
}

/**
  Get unwinding information from 64-bit ELF image.

  @param[in]  ElfImage    Pointer to ELF image.
  @param[out] UnwindInfo  Receives unwinding information.

  @return S_OK on success, S_FALSE if no unwinding info, error code otherwise.
**/
HRESULT
GetElf64UnwindInfo (
  IN  VOID                 *ElfImage,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  ELF64_HDR *ElfHeader = (ELF64_HDR *)ElfImage;
  ELF64_PH *ProgramHeader;
  ELF64_SH *SectionHeader;
  INT32 i;

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  // First, check program headers for PT_GNU_EH_FRAME
  ProgramHeader = (ELF64_PH *)ELFOFF(ElfHeader->Phoff);
  for (i = 0; i < ElfHeader->Phs; i++, ProgramHeader++) {
    if (ProgramHeader->Type == 0x6474e550) {  // PT_GNU_EH_FRAME
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

    for (i = 0; i < ElfHeader->Shs; i++, SectionHeader++) {
      CHAR8 *SectionName = &StringTable[SectionHeader->Name];
      if (strcmp(SectionName, ".eh_frame") == 0) {
        UnwindInfo->UnwindDataAddr = SectionHeader->Addr;
        UnwindInfo->UnwindDataSize = SectionHeader->Size;
        UnwindInfo->Format = 0;  // DWARF eh_frame
        return S_OK;
      }
    }
  }

  return S_FALSE;  // No unwinding info found
}

//
// ELF symbol table structures
//

typedef struct elf32sym {
  UINT32 Name;
  UINT32 Value;
  UINT32 Size;
  UINT8  Info;
  UINT8  Other;
  UINT16 Shndx;
} ELF32_SYM;

typedef struct elf64sym {
  UINT32 Name;
  UINT8  Info;
  UINT8  Other;
  UINT16 Shndx;
  UINT64 Value;
  UINT64 Size;
} ELF64_SYM;

#define SHT_SYMTAB  2
#define SHT_DYNSYM  11
#define SHT_STRTAB  3

#define ELF_ST_BIND(i)   ((i)>>4)
#define ELF_ST_TYPE(i)   ((i)&0xf)

#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3

#define STB_LOCAL   0
#define STB_GLOBAL  1
#define STB_WEAK    2

/**
  Get symbol by address from 32-bit ELF image.

  @param[in]  ElfImage    Pointer to ELF image.
  @param[in]  Address     Virtual address to look up.
  @param[out] SymbolInfo  Receives symbol information.

  @return S_OK on success, S_FALSE if not found, error code otherwise.
**/
HRESULT
GetElf32SymbolByAddress (
  IN  VOID                 *ElfImage,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  ELF32_HDR *ElfHeader = (ELF32_HDR *)ElfImage;
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

  @param[in]  ElfImage    Pointer to ELF image.
  @param[in]  Address     Virtual address to look up.
  @param[out] SymbolInfo  Receives symbol information.

  @return S_OK on success, S_FALSE if not found, error code otherwise.
**/
HRESULT
GetElf64SymbolByAddress (
  IN  VOID                 *ElfImage,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  ELF64_HDR *ElfHeader = (ELF64_HDR *)ElfImage;
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

  @param[in]  ElfImage    Pointer to ELF image.
  @param[in]  Name        Symbol name to look up.
  @param[out] SymbolInfo  Receives symbol information.

  @return S_OK on success, S_FALSE if not found, error code otherwise.
**/
HRESULT
GetElf32SymbolByName (
  IN  VOID                 *ElfImage,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  ELF32_HDR *ElfHeader = (ELF32_HDR *)ElfImage;
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

  @param[in]  ElfImage    Pointer to ELF image.
  @param[in]  Name        Symbol name to look up.
  @param[out] SymbolInfo  Receives symbol information.

  @return S_OK on success, S_FALSE if not found, error code otherwise.
**/
HRESULT
GetElf64SymbolByName (
  IN  VOID                 *ElfImage,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  ELF64_HDR *ElfHeader = (ELF64_HDR *)ElfImage;
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
