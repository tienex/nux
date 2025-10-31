/** @file
  APXH ELF Loader

  Provides ELF (Executable and Linkable Format) parsing and loading
  for 32-bit and 64-bit executables. Handles program headers for
  kernel and user-space segments, including LOAD, TLS, and custom
  APXH segment types.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>

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
