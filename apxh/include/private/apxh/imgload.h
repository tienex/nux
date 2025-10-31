/** @file
  APXH Image Loader COM Interface

  Defines COM-style interface for executable image loaders supporting
  multiple formats: ELF, Mach-O, PE/COFF, and NLM. Provides standardized
  loading, TLS handling, and entry point discovery for kernel and user
  executables.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <ananke/ananke.h>
#include <apxh/internal.h>

//
// Image Loader Result Codes
//

typedef enum _IMGLOAD_STATUS {
  ImgLoadSuccess            = 0,  ///< Load succeeded
  ImgLoadInvalidFormat      = 1,  ///< Invalid or corrupted image format
  ImgLoadUnsupportedArch    = 2,  ///< Unsupported architecture
  ImgLoadUnsupportedVersion = 3,  ///< Unsupported format version
  ImgLoadMemoryError        = 4,  ///< Memory allocation failed
  ImgLoadInvalidHeader      = 5,  ///< Invalid header structure
  ImgLoadTlsError           = 6   ///< TLS setup failed
} IMGLOAD_STATUS;

//
// TLS Information Structure
//

typedef struct _IMGLOAD_TLS_INFO {
  VIRTUAL_ADDRESS  InitDataAddr;     ///< Address of initialized TLS data
  UINT64           InitDataSize;     ///< Size of initialized TLS data
  UINT64           TotalSize;        ///< Total TLS size (init + BSS)
  VIRTUAL_ADDRESS  IndexAddr;        ///< TLS index address (optional)
  VIRTUAL_ADDRESS  CallbacksAddr;    ///< TLS callback array address (optional)
  UINT32           Alignment;        ///< TLS alignment requirement
} IMGLOAD_TLS_INFO, *PIMGLOAD_TLS_INFO;

//
// Image Load Context
//

typedef struct _IMGLOAD_CONTEXT {
  VOID             *ImageBase;       ///< Base address of image in memory
  UINTN            ImageSize;        ///< Total image size
  BOOLEAN          IsUserMode;       ///< TRUE for user-space, FALSE for kernel
  ARCH             Architecture;     ///< Target architecture
  VIRTUAL_ADDRESS  EntryPoint;      ///< Entry point virtual address
  IMGLOAD_TLS_INFO KernelTls;       ///< Kernel TLS information
  IMGLOAD_TLS_INFO UserTls;         ///< User TLS information
} IMGLOAD_CONTEXT, *PIMGLOAD_CONTEXT;

//
// Forward declarations
//

typedef struct _IMAGE_LOADER IMAGE_LOADER;
typedef struct _IMAGE_LOADER_VTBL IMAGE_LOADER_VTBL;

//
// Image Loader Interface (COM-style)
//

struct _IMAGE_LOADER_VTBL {
  /**
    Detect if image format is supported by this loader.

    @param[in] This       Pointer to loader instance.
    @param[in] ImageBase  Pointer to image in memory.
    @param[in] ImageSize  Size of image.

    @return TRUE if format is recognized, FALSE otherwise.
  **/
  BOOLEAN
  (ANXAPI *Detect) (
    IN IMAGE_LOADER  *This,
    IN VOID          *ImageBase,
    IN UINTN         ImageSize
    );

  /**
    Get target architecture from image.

    @param[in] This       Pointer to loader instance.
    @param[in] ImageBase  Pointer to image in memory.

    @return Architecture type (ARCH_*).
  **/
  ARCH
  (ANXAPI *GetArch) (
    IN IMAGE_LOADER  *This,
    IN VOID          *ImageBase
    );

  /**
    Get entry point virtual address from image.

    @param[in] This       Pointer to loader instance.
    @param[in] ImageBase  Pointer to image in memory.

    @return Entry point virtual address.
  **/
  VIRTUAL_ADDRESS
  (ANXAPI *GetEntryPoint) (
    IN IMAGE_LOADER  *This,
    IN VOID          *ImageBase
    );

  /**
    Load image segments and set up address space.

    @param[in]     This     Pointer to loader instance.
    @param[in,out] Context  Load context with image information.

    @return Load status code.
  **/
  IMGLOAD_STATUS
  (ANXAPI *LoadImage) (
    IN     IMAGE_LOADER      *This,
    IN OUT IMGLOAD_CONTEXT   *Context
    );

  /**
    Extract TLS information from image.

    @param[in]  This       Pointer to loader instance.
    @param[in]  ImageBase  Pointer to image in memory.
    @param[out] TlsInfo    Receives TLS information.

    @return Load status code.
  **/
  IMGLOAD_STATUS
  (ANXAPI *GetTlsInfo) (
    IN  IMAGE_LOADER       *This,
    IN  VOID               *ImageBase,
    OUT IMGLOAD_TLS_INFO   *TlsInfo
    );
};

struct _IMAGE_LOADER {
  CONST IMAGE_LOADER_VTBL  *Vtbl;      ///< Virtual function table
  CONST CHAR8              *Name;      ///< Loader name (e.g., "ELF", "Mach-O")
  VOID                     *Private;   ///< Private loader data
};

//
// Loader Registration
//

/**
  Register an image loader.

  @param[in] Loader  Pointer to loader instance.
**/
VOID
ImageLoaderRegister (
  IN IMAGE_LOADER  *Loader
  );

/**
  Find loader for image format.

  @param[in] ImageBase  Pointer to image in memory.
  @param[in] ImageSize  Size of image.

  @return Pointer to loader instance, or NULL if format not recognized.
**/
IMAGE_LOADER *
ImageLoaderFind (
  IN VOID   *ImageBase,
  IN UINTN  ImageSize
  );

/**
  Load image using appropriate loader.

  @param[in,out] Context  Load context with image information.

  @return Load status code.
**/
IMGLOAD_STATUS
ImageLoad (
  IN OUT IMGLOAD_CONTEXT  *Context
  );

//
// Specific Loader Instances
//

extern IMAGE_LOADER gElfLoader;
extern IMAGE_LOADER gMachoLoader;
extern IMAGE_LOADER gPeLoader;
extern IMAGE_LOADER gLeLoader;
extern IMAGE_LOADER gNlmLoader;
extern IMAGE_LOADER gAoutLoader;
extern IMAGE_LOADER gCoffLoader;
extern IMAGE_LOADER gXenixLoader;

//
// Initialization
//

/**
  Initialize all image loaders.
**/
VOID
ImageLoadersInit (
  VOID
  );
