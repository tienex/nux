/** @file
  APXH Image Loader Registry

  Manages registration and selection of image loaders. Provides unified
  interface for loading executables in multiple formats (ELF, Mach-O,
  PE/COFF, NLM) with automatic format detection.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// Maximum number of registered loaders
//

#define MAX_LOADERS 16

//
// Loader Registry
//

static IMAGE_LOADER *gLoaders[MAX_LOADERS];
static UINT32 gNumLoaders = 0;

/**
  Register an image loader.

  @param[in] Loader  Pointer to loader instance.
**/
VOID
ImageLoaderRegister (
  IN IMAGE_LOADER  *Loader
  )
{
  if (gNumLoaders >= MAX_LOADERS) {
    fatal("Too many image loaders registered (max %d)", MAX_LOADERS);
  }

  gLoaders[gNumLoaders++] = Loader;
  info("Registered %s image loader", Loader->Name);
}

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
  )
{
  UINT32 i;

  for (i = 0; i < gNumLoaders; i++) {
    IMAGE_LOADER *Loader = gLoaders[i];

    if (Loader->Vtbl->Detect(Loader, ImageBase, ImageSize)) {
      return Loader;
    }
  }

  return NULL;
}

/**
  Load image using appropriate loader.

  @param[in,out] Context  Load context with image information.

  @return Load status code.
**/
IMGLOAD_STATUS
ImageLoad (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  IMAGE_LOADER *Loader;
  IMGLOAD_STATUS Status;
  ARCH ImageArch;

  // Find appropriate loader
  Loader = ImageLoaderFind(Context->ImageBase, Context->ImageSize);
  if (Loader == NULL) {
    warn("No loader found for image format");
    return ImgLoadInvalidFormat;
  }

  info("Using %s loader", Loader->Name);

  // Get image architecture
  ImageArch = Loader->Vtbl->GetArch(Loader, Context->ImageBase);
  if (ImageArch == ARCH_INVALID) {
    warn("Invalid image format");
    return ImgLoadInvalidFormat;
  }

  if (ImageArch == ARCH_UNSUPPORTED) {
    warn("Unsupported architecture");
    return ImgLoadUnsupportedArch;
  }

  Context->Architecture = ImageArch;

  // Get entry point
  Context->EntryPoint = Loader->Vtbl->GetEntryPoint(Loader, Context->ImageBase);

  // Extract TLS information
  Status = Loader->Vtbl->GetTlsInfo(Loader, Context->ImageBase,
                                    Context->IsUserMode ?
                                    &Context->UserTls :
                                    &Context->KernelTls);
  if (Status != ImgLoadSuccess) {
    warn("Failed to extract TLS information");
    return Status;
  }

  // Load the image
  Status = Loader->Vtbl->LoadImage(Loader, Context);
  if (Status != ImgLoadSuccess) {
    warn("Failed to load image");
    return Status;
  }

  return ImgLoadSuccess;
}

/**
  Initialize all image loaders.
**/
VOID
ImageLoadersInit (
  VOID
  )
{
  info("Initializing image loaders...");

  // Register all supported loaders (order matters for detection)
  ImageLoaderRegister(&gElfLoader);
  ImageLoaderRegister(&gMachoLoader);
  ImageLoaderRegister(&gPeLoader);
  ImageLoaderRegister(&gLeLoader);
  ImageLoaderRegister(&gCoffLoader);
  ImageLoaderRegister(&gAoutLoader);
  ImageLoaderRegister(&gXenixLoader);
  ImageLoaderRegister(&gNlmLoader);

  info("Image loaders initialized (%d loaders)", gNumLoaders);
}
