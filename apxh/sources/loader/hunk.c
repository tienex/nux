/** @file
  APXH Amiga HUNK Loader

  Provides Amiga HUNK format parsing and loading for Amiga executables.
  HUNK is the executable file format of tools and programs for the Amiga
  Operating System, derived from TRIPOS which formed the basis for AmigaDOS.

  Supports:
  - Executable files (HUNK_HEADER with code/data hunks)
  - 68000 (Motorola 68K) architecture
  - HUNK_CODE, HUNK_DATA, HUNK_BSS sections

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// Amiga HUNK Magic Numbers
//

#define HUNK_HEADER     0x000003F3  ///< Executable file header
#define HUNK_UNIT       0x000003E7  ///< Object file header

//
// Amiga HUNK Types
//

#define HUNK_CODE       0x000003E9  ///< Code hunk
#define HUNK_DATA       0x000003EA  ///< Data hunk
#define HUNK_BSS        0x000003EB  ///< BSS hunk
#define HUNK_RELOC32    0x000003EC  ///< 32-bit relocations
#define HUNK_SYMBOL     0x000003F0  ///< Symbol table
#define HUNK_DEBUG      0x000003F1  ///< Debug info
#define HUNK_END        0x000003F2  ///< End of hunk
#define HUNK_OVERLAY    0x000003F5  ///< Overlay hunk
#define HUNK_BREAK      0x000003F6  ///< Break hunk
#define HUNK_DREL32     0x000003F7  ///< 32-bit data relocations
#define HUNK_DREL16     0x000003F8  ///< 16-bit data relocations
#define HUNK_DREL8      0x000003F9  ///< 8-bit data relocations
#define HUNK_LIB        0x000003FA  ///< Library hunk
#define HUNK_INDEX      0x000003FB  ///< Index hunk

//
// HUNK Flags
//

#define HUNKF_CHIP      0x40000000  ///< Chip memory
#define HUNKF_FAST      0x80000000  ///< Fast memory
#define HUNKF_MASK      0x3FFFFFFF  ///< Mask for size/type

//
// Default Amiga Load Addresses
//

#define AMIGA_BASE_ADDR 0x00000000  ///< Base load address

//
// Helper Macros
//

#define HUNK_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))
#define HUNK_SIZE(_s) ((_s) & HUNKF_MASK)

//
// Internal State
//

typedef struct _HUNK_LOAD_STATE {
  UINT32           NumHunks;
  VIRTUAL_ADDRESS  *HunkAddrs;    ///< Base addresses of loaded hunks
  UINT32           *HunkSizes;    ///< Sizes of loaded hunks
  UINT32           CurrentHunk;
  VIRTUAL_ADDRESS  NextAddr;      ///< Next available address
} HUNK_LOAD_STATE;

//
// Internal Functions
//

/**
  Check if image is Amiga HUNK format.
**/
static
BOOLEAN
ANXAPI
HunkDetect (
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

  return (*Magic == HUNK_HEADER ||
          *Magic == HUNK_UNIT);
}

/**
  Get architecture from HUNK image.
**/
static
ARCH
ANXAPI
HunkGetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  // Amiga HUNK is 68K only, which is not supported by APXH
  return ARCH_UNSUPPORTED;
}

/**
  Get entry point from HUNK image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
HunkGetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  // Entry point is at the start of first code hunk
  return AMIGA_BASE_ADDR;
}

/**
  Parse and load HUNK file.
**/
static
IMGLOAD_STATUS
HunkParseFile (
  IN     VOID                *ImageBase,
  IN     UINTN               ImageSize,
  IN     BOOLEAN             IsUserMode,
  IN OUT HUNK_LOAD_STATE     *State
  )
{
  UINT32 *Ptr = (UINT32 *)ImageBase;
  UINT32 *End = (UINT32 *)((UINT8 *)ImageBase + ImageSize);
  UINT32 Magic, NumHunks, FirstHunk, LastHunk;
  UINT32 i;

  // Read HUNK_HEADER
  if (Ptr >= End || *Ptr != HUNK_HEADER) {
    return ImgLoadInvalidFormat;
  }
  Ptr++;

  // Skip resident library names (if any)
  while (Ptr < End && *Ptr != 0) {
    UINT32 NameLen = *Ptr++;
    Ptr += NameLen;
  }
  Ptr++;  // Skip terminating 0

  // Read hunk table
  if (Ptr + 3 > End) {
    return ImgLoadInvalidFormat;
  }

  NumHunks = *Ptr++;
  FirstHunk = *Ptr++;
  LastHunk = *Ptr++;

  State->NumHunks = LastHunk - FirstHunk + 1;
  State->CurrentHunk = 0;
  State->NextAddr = AMIGA_BASE_ADDR;

  info("  %d hunks (first: %d, last: %d)", State->NumHunks, FirstHunk, LastHunk);

  // Allocate hunk tracking arrays
  State->HunkAddrs = (VIRTUAL_ADDRESS *)malloc(State->NumHunks * sizeof(VIRTUAL_ADDRESS));
  State->HunkSizes = (UINT32 *)malloc(State->NumHunks * sizeof(UINT32));

  // Read hunk sizes
  for (i = 0; i < State->NumHunks; i++) {
    if (Ptr >= End) {
      return ImgLoadInvalidFormat;
    }
    State->HunkSizes[i] = HUNK_SIZE(*Ptr++) * 4;  // Convert longwords to bytes
  }

  // Process hunks
  while (Ptr < End) {
    Magic = *Ptr++;

    switch (Magic) {
      case HUNK_CODE:
      case HUNK_DATA: {
        UINT32 Size = HUNK_SIZE(*Ptr++) * 4;
        BOOLEAN IsExec = (Magic == HUNK_CODE);

        if (State->CurrentHunk >= State->NumHunks) {
          warn("Too many hunks in file");
          return ImgLoadInvalidFormat;
        }

        State->HunkAddrs[State->CurrentHunk] = State->NextAddr;

        info("  HUNK_%s at 0x%08x (size: 0x%08x)",
             IsExec ? "CODE" : "DATA", State->NextAddr, Size);

        if (Ptr + (Size / 4) > End) {
          return ImgLoadInvalidFormat;
        }

        VirtualAddressCopy(
          State->NextAddr,
          Ptr,
          Size,
          IsUserMode,
          !IsExec,  // Data is writable
          IsExec    // Code is executable
        );

        Ptr += Size / 4;
        State->NextAddr += Size;
        State->CurrentHunk++;
        break;
      }

      case HUNK_BSS: {
        UINT32 Size = HUNK_SIZE(*Ptr++) * 4;

        if (State->CurrentHunk >= State->NumHunks) {
          warn("Too many hunks in file");
          return ImgLoadInvalidFormat;
        }

        State->HunkAddrs[State->CurrentHunk] = State->NextAddr;

        info("  HUNK_BSS at 0x%08x (size: 0x%08x)", State->NextAddr, Size);

        VirtualAddressMemset(
          State->NextAddr,
          0,
          Size,
          IsUserMode,
          TRUE,   // Writable
          FALSE   // Not executable
        );

        State->NextAddr += Size;
        State->CurrentHunk++;
        break;
      }

      case HUNK_RELOC32:
      case HUNK_DREL32:
      case HUNK_DREL16:
      case HUNK_DREL8:
        // Skip relocations (not implemented)
        while (Ptr < End && *Ptr != 0) {
          UINT32 NumRelocs = *Ptr++;
          if (NumRelocs == 0) break;
          Ptr++;  // Skip hunk number
          Ptr += NumRelocs;  // Skip offsets
        }
        Ptr++;  // Skip terminating 0
        break;

      case HUNK_SYMBOL:
      case HUNK_DEBUG:
        // Skip symbols/debug info
        while (Ptr < End && *Ptr != 0) {
          UINT32 NameLen = *Ptr++;
          if (NameLen == 0) break;
          Ptr += NameLen;  // Skip name
          Ptr++;  // Skip value/offset
        }
        break;

      case HUNK_END:
        // End of current hunk
        break;

      default:
        warn("Unknown hunk type 0x%08x", Magic);
        return ImgLoadInvalidFormat;
    }
  }

  return ImgLoadSuccess;
}

/**
  Load Amiga HUNK image.
**/
static
IMGLOAD_STATUS
ANXAPI
HunkLoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  HUNK_LOAD_STATE State = {0};
  IMGLOAD_STATUS Status;

  info("Loading Amiga HUNK executable...");

  Status = HunkParseFile(
    Context->ImageBase,
    Context->ImageSize,
    Context->IsUserMode,
    &State
  );

  if (State.HunkAddrs != NULL) {
    free(State.HunkAddrs);
  }
  if (State.HunkSizes != NULL) {
    free(State.HunkSizes);
  }

  if (Status != ImgLoadSuccess) {
    return Status;
  }

  Context->EntryPoint = HunkGetEntryPoint(This, Context->ImageBase);
  return ImgLoadSuccess;
}

/**
  Extract TLS information from HUNK image.
**/
static
IMGLOAD_STATUS
ANXAPI
HunkGetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  // Amiga HUNK doesn't support TLS
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return ImgLoadSuccess;
}

//
// Amiga HUNK Loader VTable
//

static CONST IMAGE_LOADER_VTBL gHunkVtbl = {
  HunkDetect,
  HunkGetArch,
  HunkGetEntryPoint,
  HunkLoadImage,
  HunkGetTlsInfo
};

//
// Amiga HUNK Loader Instance
//

IMAGE_LOADER gHunkLoader = {
  &gHunkVtbl,
  "HUNK",
  NULL
};
