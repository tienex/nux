/** @file
  APXH Image Loader Wrapper

  Provides wrapper functions for legacy image loading interface.
  Uses the COM-based IMGLOADER interface internally for format detection,
  architecture identification, and image loading. Supports all executable
  formats (ELF, PE, LE/LX, Mach-O, etc.) through unified interface.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org
  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

// Minimal ELF header for architecture detection only
ANX_PACK_PUSH(1)
typedef struct _MINIMAL_ELF_HEADER {
  UINT8   Id[16];     ///< ELF identification
  UINT16  Type;       ///< Object file type
  UINT16  Machine;    ///< Machine architecture
} MINIMAL_ELF_HEADER;
ANX_PACK_POP()

/**
  Load executable image using IMGLOADER COM interface.

  Uses the COM-based IMGLOADER interface to load executable images
  in any supported format (ELF, PE, LE/LX, Mach-O, etc). Automatically
  detects format and bitness (32-bit vs 64-bit).

  @param[in] ImageBase  Pointer to image in memory.
  @param[in] IsUserMode TRUE for user-space, FALSE for kernel.

  @return Entry point virtual address, or -1 on error.
**/
VIRTUAL_ADDRESS
LoadExecutable (
  IN VOID     *ImageBase,
  IN BOOLEAN  IsUserMode
  )
{
  IMGLOAD_CONTEXT Context;
  HRESULT Status;
  UINTN ImageSize = 64 * 1024 * 1024; // Assume 64MB max for detection

  // Initialize load context
  memset(&Context, 0, sizeof(Context));
  Context.ImageBase = ImageBase;
  Context.ImageSize = ImageSize;
  Context.IsUserMode = IsUserMode;

  // Use COM interface to load image (handles format/bitness detection)
  Status = ImageLoad(&Context);
  if (FAILED(Status)) {
    printf("Failed to load image: HRESULT 0x%08x\n", Status);
    return (VIRTUAL_ADDRESS)-1;
  }

  return Context.EntryPoint;
}

/**
  Get image architecture.

  Determines the target architecture from any supported executable format
  using the COM-based IMGLOADER interface.

  @param[in] ImageBase  Pointer to image in memory.

  @return Architecture type, or ArchInvalid/ArchUnsupported.
**/
ARCH
GetImageArch (
  IN VOID  *ImageBase
  )
{
  IImageLoader *Loader;
  ARCH Architecture;
  HRESULT Status;
  UINTN ImageSize = 64 * 1024 * 1024; // Assume reasonable max size

  // Find appropriate loader for this image
  Loader = ImageLoaderFind(ImageBase, ImageSize);
  if (Loader == NULL) {
    // Check if it's ELF by looking at magic bytes
    MINIMAL_ELF_HEADER *Header = (MINIMAL_ELF_HEADER *)ImageBase;
    if (Header->Id[0] == 0x7F &&
        Header->Id[1] == 'E' &&
        Header->Id[2] == 'L' &&
        Header->Id[3] == 'F') {
      // Valid ELF but unsupported
      return ArchUnsupported;
    }
    return ArchInvalid;
  }

  // Get architecture from loader
  Status = Loader->lpVtbl->GetArch(Loader, ImageBase, &Architecture);
  if (FAILED(Status)) {
    return ArchUnsupported;
  }

  return Architecture;
}
