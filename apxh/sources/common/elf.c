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
  UINT32 type;
  UINT32 off;
  UINT32 va;
  UINT32 pa;
  UINT32 fsize;
  UINT32 msize;
  UINT32 flags;
  UINT32 align;
} ELF32_PH;

typedef struct elf64ph
{
  UINT32 type;
  UINT32 flags;
  UINT64 off;
  UINT64 va;
  UINT64 pa;
  UINT64 fsize;
  UINT64 msize;
  UINT64 align;
} ELF64_PH;

typedef struct elf32sh
{
  UINT32 name;
  UINT32 type;
  UINT32 flags;
  UINT32 addr;
  UINT32 off;
  UINT32 size;
  UINT32 lnk;
  UINT32 info;
  UINT32 align;
  UINT32 shent_size;
} ELF32_SH;

typedef struct elf64sh
{
  UINT32 name;
  UINT32 type;
  UINT64 flags;
  UINT64 addr;
  UINT64 off;
  UINT64 size;
  UINT32 lnk;
  UINT32 info;
  UINT64 align;
  UINT64 shent_size;
} ELF64_SH;

typedef struct elf32hdr
{
  UINT8  id[16];
  UINT16 type;
  UINT16 mach;
  UINT32 ver;
  UINT32 entry;
  UINT32 phoff;
  UINT32 shoff;
  UINT32 flags;
  UINT16 eh_size;
  UINT16 phent_size;
  UINT16 phs;
  UINT16 shent_size;
  UINT16 shs;
  UINT16 shstrndx;
} __packed ELF32_HDR;

typedef struct elf64hdr
{
  UINT8  id[16];
  UINT16 type;
  UINT16 mach;
  UINT32 ver;
  UINT64 entry;
  UINT64 phoff;
  UINT64 shoff;
  UINT32 flags;
  UINT16 eh_size;
  UINT16 phent_size;
  UINT16 phs;
  UINT16 shent_size;
  UINT16 shs;
  UINT16 shstrndx;
} __packed ELF64_HDR;

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
  IN VOID    *ElfImg,
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
	  VaCopy (Va, ELFOFF (Off), FSize, 1,
		   !!(Flags & PHF_W), !!(Flags & PHF_X));
	}

      if (MSize - FSize > 0)
	{
	  /*
	     memset() to user and populate on fault.
	   */
	  VaMemset (Va + FSize, 0, MSize - FSize, 1,
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
	      VaCopy (Va, ELFOFF (Off), FSize, 0,
		       !!(Flags & PHF_W), !!(Flags & PHF_X));
	    }
	  VaUtls (Va, FSize, MSize);
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
  IN VOID    *ElfImg,
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
	  VaCopy (Va, ELFOFF (Off), FSize, 0,
		   !!(Flags & PHF_W), !!(Flags & PHF_X));
	}

      if (MSize - FSize > 0)
	{
	  /*
	     memset() to user and populate on fault.
	   */
	  VaMemset (Va + FSize, 0, MSize - FSize, 0,
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
	      VaCopy (Va, ELFOFF (Off), FSize, 0,
		       !!(Flags & PHF_W), !!(Flags & PHF_X));
	    }
	  VaKtls (Va, FSize, MSize);
	}
      break;

    case PHT_APXH_INFO:
      /* Boot Information segment. */
      printf ("Boot Information area at %" PRIx64 " (size: %" PRId64 "d).\n",
	      Va, MSize);
      VaInfo (Va, MSize);
      break;
    case PHT_APXH_PHYSMAP:
      /* Direct 1:1 PA mapping. */
      printf ("Physmap VA area at %" PRIx64 " (size: %" PRId64 ").\n", Va,
	      MSize);
      VaPhysmap (Va, MSize, MEMTYPE_WB);
      break;
    case PHT_APXH_EMPTY:
      printf ("Empty VA area at %" PRIx64 " (size: %" PRId64 ").\n", Va,
	      MSize);
      /* Just VA allocation. Leave it. */
      break;
    case PHT_APXH_PTALLOC:
      printf ("PT Alloc VA area at %" PRIx64 " (size: %" PRId64 ").\n", Va,
	      MSize);
      VaPtalloc (Va, MSize);
      break;
    case PHT_APXH_PFNMAP:
      printf ("PFN Map at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VaPfnmap (Va, MSize);
      break;
    case PHT_APXH_STREE:
      printf ("S-Tree at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VaStree (Va, MSize);
      break;
    case PHT_APXH_LINEAR:
      printf ("Linear Map at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VaLinear (Va, MSize);
      break;
    case PHT_APXH_FRAMEBUF:
      printf ("Framebuffer Map at %" PRIx64 " (size: %" PRId64 ").\n", Va,
	      MSize);
      VaFramebuf (Va, MSize, MEMTYPE_WC);
      break;
    case PHT_APXH_REGIONS:
      printf ("Region Map at %" PRIx64 " (size: %" PRId64 ").\n", Va, MSize);
      VaRegions (Va, MSize);
      break;
    case PHT_APXH_TOPPTALLOC:
      printf ("TOP PT Alloc VA area at %" PRIx64 " (size: %" PRId64 ").\n",
	      Va, MSize);
      VaTopptalloc (Va, MSize);
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
  IN VOID    *ElfImg,
  IN INT32   User
  )
{
  INT32 i;

  CHAR8 ElfId[] = { 0x7f, 'E', 'L', 'F', };
  ELF32_HDR *Hdr = (ELF32_HDR *) ElfImg;
  ELF32_PH *Ph = (ELF32_PH *) ELFOFF (Hdr->phoff);

  if (memcmp (Hdr->id, ElfId, 4) != 0)
    return (UINTN) - 1;

  if (Hdr->type != ET_EXEC || Hdr->ver != EV_CURRENT)
    return (UINTN) - 1;

  for (i = 0; i < Hdr->phs; i++, Ph++)
    {
      if (User)
	PhUload (ElfImg, Ph->type, Ph->flags, Ph->va, Ph->msize, Ph->off,
		  Ph->fsize);
      else
	PhKload (ElfImg, Ph->type, Ph->flags, Ph->va, Ph->msize, Ph->off,
		  Ph->fsize);
    }

  return (VIRTUAL_ADDRESS) Hdr->entry;
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
  IN VOID    *ElfImg,
  IN INT32   User
  )
{
  INT32 i;

  CHAR8 ElfId[] = { 0x7f, 'E', 'L', 'F', };
  ELF64_HDR *Hdr = (ELF64_HDR *) ElfImg;
  ELF64_PH *Ph = (ELF64_PH *) ELFOFF (Hdr->phoff);

  if (memcmp (Hdr->id, ElfId, 4) != 0)
    return (UINTN) - 1;

  if (Hdr->type != ET_EXEC || Hdr->ver != EV_CURRENT)
    return (UINTN) - 1;

  for (i = 0; i < Hdr->phs; i++, Ph++)
    {
      if (User)
	PhUload (ElfImg, Ph->type, Ph->flags, Ph->va, Ph->msize, Ph->off,
		  Ph->fsize);
      else
	PhKload (ElfImg, Ph->type, Ph->flags, Ph->va, Ph->msize, Ph->off,
		  Ph->fsize);
    }

  return (VIRTUAL_ADDRESS) Hdr->entry;
}

/**
  Get ELF architecture.

  Determines the target architecture from ELF machine type.

  @param[in] ElfImg  Pointer to ELF image.

  @return Architecture type, or ARCH_INVALID/ARCH_UNSUPPORTED.
**/
ARCH
GetElfArch (
  IN VOID  *ElfImg
  )
{
  CHAR8 ElfId[] = { 0x7f, 'E', 'L', 'F', };
  ELF32_HDR *Hdr = (ELF32_HDR *) ElfImg;

  if (memcmp (Hdr->id, ElfId, 4) != 0)
    return ARCH_INVALID;

  if (Hdr->mach == EM_386)
    return ARCH_386;

  if (Hdr->mach == EM_X86_64)
    return ARCH_AMD64;

  if (Hdr->mach == EM_RISCV)
    return ARCH_RISCV64;

  return ARCH_UNSUPPORTED;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use PhUload instead **/
void ph_uload (void *elfimg, UINT32 type, UINT32 flags,
	       UINT64 va, UINT64 msize, UINT64 off, UINT64 fsize) {
  PhUload (elfimg, type, flags, va, msize, off, fsize);
}

/** @deprecated Use PhKload instead **/
void ph_kload (void *elfimg, UINT32 type, UINT32 flags,
	       UINT64 va, UINT64 msize, UINT64 off, UINT64 fsize) {
  PhKload (elfimg, type, flags, va, msize, off, fsize);
}

/** @deprecated Use LoadElf32 instead **/
VIRTUAL_ADDRESS load_elf32 (void *elfimg, int u) {
  return LoadElf32 (elfimg, u);
}

/** @deprecated Use LoadElf64 instead **/
VIRTUAL_ADDRESS load_elf64 (void *elfimg, int u) {
  return LoadElf64 (elfimg, u);
}

/** @deprecated Use GetElfArch instead **/
ARCH get_elf_arch (void *elfimg) {
  return GetElfArch (elfimg);
}
