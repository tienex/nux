/** @file
  APXH HP SOM Loader

  Provides HP SOM (System Object Model) format recognition for PA-RISC
  executables. SOM is the native 32-bit format for HP-UX and MPE/ix on
  PA-RISC processors.

  Documentation:
  - HP-UX a.out(4) manual page - SOM format specification
  - PA-RISC Runtime Architecture document
  - HP-UX include files: som.h, lst.h

  NOTE: Full implementation requires HP-UX include files and PA-RISC
  Runtime Architecture documentation. This is a stub implementation.

  Supports:
  - PA-RISC architecture
  - Big-endian format
  - Format recognition only (loading not yet implemented)

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// HP SOM Magic Numbers
//

#define SOM_MAGIC_PARISC11  0x02100107  ///< PA-RISC 1.1 executable

//
// Internal Functions
//

/**
  Check if image is HP SOM format.
**/
static
BOOLEAN
ANXAPI
SomDetect (
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

  // Check for PA-RISC magic (big-endian)
  return (*Magic == 0x07011002);  // 0x02100107 in big-endian
}

/**
  Get architecture from HP SOM image.
**/
static
ARCH
ANXAPI
SomGetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  // PA-RISC is not supported by APXH
  return ARCH_UNSUPPORTED;
}

/**
  Get entry point from HP SOM image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
SomGetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  return 0;
}

/**
  Load HP SOM image.
**/
static
IMGLOAD_STATUS
ANXAPI
SomLoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  warn("HP SOM image loading not yet implemented");
  return ImgLoadUnsupportedArch;
}

/**
  Extract TLS information from HP SOM image.
**/
static
IMGLOAD_STATUS
ANXAPI
SomGetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return ImgLoadSuccess;
}

//
// HP SOM Loader VTable
//

static CONST IMAGE_LOADER_VTBL gSomVtbl = {
  SomDetect,
  SomGetArch,
  SomGetEntryPoint,
  SomLoadImage,
  SomGetTlsInfo
};

//
// HP SOM Loader Instance
//

IMAGE_LOADER gSomLoader = {
  &gSomVtbl,
  "SOM",
  NULL
};
