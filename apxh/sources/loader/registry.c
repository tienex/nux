/** @file
  APXH Image Loader Registry

  Manages registration and selection of image loaders. Provides unified
  interface for loading executables in multiple formats with automatic
  format detection. Supports: ELF, FatELF, ECOFF, Mach-O, PE/COFF, PEF,
  LE/LX, NLM, XCOFF, COFF, a.out, XENIX, Amiga HUNK, Atari PRG, Plan 9,
  HP SOM, OpenVMS, and PDP-10.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// Maximum number of registered loaders
//

#define MAX_LOADERS 32

//
// Loader Registry
//

static IImageLoader *gLoaders[MAX_LOADERS];
static UINT32 gNumLoaders = 0;

/**
  Register an image loader.

  @param[in] Loader  Pointer to loader instance.

  @return S_OK on success, error code otherwise.
**/
HRESULT
ImageLoaderRegister (
  IN IImageLoader  *Loader
  )
{
  HRESULT Status;
  VOID *TestInterface = NULL;

  if (Loader == NULL) {
    return E_POINTER;
  }

  if (gNumLoaders >= MAX_LOADERS) {
    fatal("Too many image loaders registered (max %d)", MAX_LOADERS);
    return E_OUTOFMEMORY;
  }

  // Verify loader implements IImageLoader
  Status = Loader->lpVtbl->QueryInterface(Loader, &IID_IImageLoader, &TestInterface);
  if (FAILED(Status) || TestInterface == NULL) {
    warn("Loader does not implement IImageLoader interface");
    return E_NOINTERFACE;
  }

  gLoaders[gNumLoaders++] = Loader;
  info("Registered image loader #%d", gNumLoaders);

  return S_OK;
}

/**
  Find loader for image format.

  @param[in] ImageBase  Pointer to image in memory.
  @param[in] ImageSize  Size of image.

  @return Pointer to loader instance, or NULL if format not recognized.
**/
IImageLoader *
ImageLoaderFind (
  IN VOID   *ImageBase,
  IN UINTN  ImageSize
  )
{
  UINT32 i;

  for (i = 0; i < gNumLoaders; i++) {
    IImageLoader *Loader = gLoaders[i];
    HRESULT Status;

    Status = Loader->lpVtbl->Detect(Loader, ImageBase, ImageSize);
    if (Status == S_OK) {
      return Loader;
    }
  }

  return NULL;
}

/**
  Load image using appropriate loader.

  @param[in,out] Context  Load context with image information.

  @return S_OK on success, error code otherwise.
**/
HRESULT
ImageLoad (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  IImageLoader *Loader;
  HRESULT Status;

  if (Context == NULL) {
    return E_POINTER;
  }

  // Find appropriate loader
  Loader = ImageLoaderFind(Context->ImageBase, Context->ImageSize);
  if (Loader == NULL) {
    warn("No loader found for image format");
    return IMGLOAD_E_INVALID_FORMAT;
  }

  info("Using loader for image");

  // Get image architecture
  Status = Loader->lpVtbl->GetArch(Loader, Context->ImageBase, &Context->Architecture);
  if (FAILED(Status)) {
    warn("Failed to get image architecture");
    return Status;
  }

  // Get endianness
  Status = Loader->lpVtbl->GetEndianness(Loader, Context->ImageBase, &Context->Endianness);
  if (FAILED(Status)) {
    warn("Failed to get image endianness");
    return Status;
  }

  // Get entry point
  Status = Loader->lpVtbl->GetEntryPoint(Loader, Context->ImageBase, &Context->EntryPoint);
  if (FAILED(Status)) {
    warn("Failed to get entry point");
    return Status;
  }

  // Extract TLS information (optional)
  Status = Loader->lpVtbl->GetTlsInfo(Loader, Context->ImageBase,
                                      Context->IsUserMode ?
                                      &Context->UserTls :
                                      &Context->KernelTls);
  if (FAILED(Status) && Status != S_FALSE) {
    warn("Failed to extract TLS information");
    return Status;
  }

  // Extract unwinding information (optional)
  Status = Loader->lpVtbl->GetUnwindInfo(Loader, Context->ImageBase, &Context->UnwindInfo);
  if (FAILED(Status) && Status != S_FALSE) {
    warn("Failed to extract unwinding information");
    // Continue anyway - unwinding info is optional
  }

  // Load the image
  Status = Loader->lpVtbl->LoadImage(Loader, Context);
  if (FAILED(Status)) {
    warn("Failed to load image");
    return Status;
  }

  return S_OK;
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
  ImageLoaderRegister(&gFatElfLoader);  // Check FatELF before ELF
  ImageLoaderRegister(&gElfLoader);
  ImageLoaderRegister(&gEcoffLoader);   // Extended COFF (MIPS, Alpha)
  ImageLoaderRegister(&gMachoLoader);
  ImageLoaderRegister(&gPeLoader);
  ImageLoaderRegister(&gPefLoader);     // Mac OS Classic
  ImageLoaderRegister(&gLeLoader);
  ImageLoaderRegister(&gNlmLoader);
  ImageLoaderRegister(&gXcoffLoader);   // AIX Extended COFF
  ImageLoaderRegister(&gCoffLoader);    // SCO COFF
  ImageLoaderRegister(&gAoutLoader);    // Unix/MINIX a.out
  ImageLoaderRegister(&gXenixLoader);
  ImageLoaderRegister(&gHunkLoader);    // Amiga
  ImageLoaderRegister(&gAtariLoader);   // Atari TOS
  ImageLoaderRegister(&gPlan9Loader);   // Plan 9
  ImageLoaderRegister(&gSomLoader);     // HP-UX PA-RISC
  ImageLoaderRegister(&gVmsLoader);     // OpenVMS
  ImageLoaderRegister(&gPdp10Loader);   // PDP-10 TOPS-10/TOPS-20

  info("Image loaders initialized (%d loaders)", gNumLoaders);
}
