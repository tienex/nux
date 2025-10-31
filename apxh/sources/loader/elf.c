/** @file
  APXH ELF Loader COM Wrapper

  Provides COM interface wrapper for the existing ELF loader.
  Exposes ELF loading functionality through the standardized
  image loader interface.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// External ELF functions (from elf_impl.c)
//

extern ARCH GetElfArch (IN VOID *ElfImage);
extern VIRTUAL_ADDRESS LoadElf32 (IN VOID *ElfImage, IN INT32 IsUserMode);
extern VIRTUAL_ADDRESS LoadElf64 (IN VOID *ElfImage, IN INT32 IsUserMode);

//
// ELF Magic Numbers
//

#define ELF_MAGIC  0x7F454C46  ///< "\x7FELF"

//
// Internal Functions
//

/**
  Check if image is ELF format.
**/
static
BOOLEAN
ANXAPI
ElfDetect (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  UINT32 *Magic;

  if (ImageSize < 4) {
    return FALSE;
  }

  Magic = (UINT32 *)ImageBase;
  return (*Magic == ELF_MAGIC);
}

/**
  Get architecture from ELF image.
**/
static
ARCH
ANXAPI
ElfGetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  return GetElfArch(ImageBase);
}

/**
  Get entry point from ELF image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
ElfGetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  // Entry point is extracted during LoadElf32/64
  // For now, return 0 - actual entry point is set by LoadImage
  return 0;
}

/**
  Load ELF image.
**/
static
IMGLOAD_STATUS
ANXAPI
ElfLoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VIRTUAL_ADDRESS EntryPoint;
  UINT8 *Ident = (UINT8 *)Context->ImageBase;

  // Check ELF class (32-bit or 64-bit)
  if (Ident[4] == 1) {
    // 32-bit ELF
    EntryPoint = LoadElf32(Context->ImageBase, Context->IsUserMode);
  } else if (Ident[4] == 2) {
    // 64-bit ELF
    EntryPoint = LoadElf64(Context->ImageBase, Context->IsUserMode);
  } else {
    return ImgLoadInvalidFormat;
  }

  if (EntryPoint == (VIRTUAL_ADDRESS)-1) {
    return ImgLoadInvalidFormat;
  }

  Context->EntryPoint = EntryPoint;
  return ImgLoadSuccess;
}

/**
  Extract TLS information from ELF image.
**/
static
IMGLOAD_STATUS
ANXAPI
ElfGetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  // TLS is handled internally by LoadElf32/LoadElf64
  // They call VirtualAddressMapKernelTls/VirtualAddressMapUserTls
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return ImgLoadSuccess;
}

//
// ELF Loader VTable
//

static CONST IMAGE_LOADER_VTBL gElfVtbl = {
  ElfDetect,
  ElfGetArch,
  ElfGetEntryPoint,
  ElfLoadImage,
  ElfGetTlsInfo
};

//
// ELF Loader Instance
//

IMAGE_LOADER gElfLoader = {
  &gElfVtbl,
  "ELF",
  NULL
};
