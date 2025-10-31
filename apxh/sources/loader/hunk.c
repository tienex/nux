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
  IUnknown: QueryInterface
**/
static
HRESULT
STDMETHODCALLTYPE
HunkQueryInterface (
  IN  IImageLoader  *This,
  IN  CONST GUID    *Iid,
  OUT VOID          **Interface
  )
{
  if (Interface == NULL) {
    return E_POINTER;
  }

  if (memcmp(Iid, &IID_IImageLoader, sizeof(GUID)) == 0 ||
      memcmp(Iid, &IID_IUnknown, sizeof(GUID)) == 0) {
    *Interface = This;
    return S_OK;
  }

  *Interface = NULL;
  return E_NOINTERFACE;
}

/**
  IUnknown: AddRef
**/
static
UINTN
STDMETHODCALLTYPE
HunkAddRef (
  IN IImageLoader  *This
  )
{
  return 1;  // Static instance
}

/**
  IUnknown: Release
**/
static
UINTN
STDMETHODCALLTYPE
HunkRelease (
  IN IImageLoader  *This
  )
{
  return 1;  // Static instance
}

/**
  Check if image is Amiga HUNK format.
**/
static
HRESULT
STDMETHODCALLTYPE
HunkDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  UINT32 *Magic;

  if (ImageSize < 4) {
    return S_FALSE;
  }

  Magic = (UINT32 *)ImageBase;

  return (*Magic == HUNK_HEADER || *Magic == HUNK_UNIT) ? S_OK : S_FALSE;
}

/**
  Get architecture from HUNK image.
**/
static
HRESULT
STDMETHODCALLTYPE
HunkGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  if (Architecture == NULL) {
    return E_POINTER;
  }

  // Traditional Amiga HUNK is 68K only, which is not supported by APXH
  // Note: AROS may extend HUNK to host x86 code, but no standard detection method exists
  *Architecture = ArchUnsupported;
  return IMGLOAD_E_UNSUPPORTED_ARCH;
}

/**
  Get endianness from HUNK image.
**/
static
HRESULT
STDMETHODCALLTYPE
HunkGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // Traditional Amiga HUNK is 68K big-endian
  // Note: AROS x86 would be little-endian, but we can't detect that from standard HUNK
  *Endianness = ImgEndianBig;
  return S_OK;
}

/**
  Get entry point from HUNK image.
**/
static
HRESULT
STDMETHODCALLTYPE
HunkGetEntryPoint (
  IN  IImageLoader     *This,
  IN  VOID             *ImageBase,
  OUT VIRTUAL_ADDRESS  *EntryPoint
  )
{
  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  // Entry point is at the start of first code hunk
  *EntryPoint = AMIGA_BASE_ADDR;
  return S_OK;
}

/**
  Parse and load HUNK file.
**/
static
HRESULT
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
    return IMGLOAD_E_INVALID_FORMAT;
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
    return IMGLOAD_E_INVALID_FORMAT;
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
      return IMGLOAD_E_INVALID_FORMAT;
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
          return IMGLOAD_E_INVALID_FORMAT;
        }

        State->HunkAddrs[State->CurrentHunk] = State->NextAddr;

        info("  HUNK_%s at 0x%08x (size: 0x%08x)",
             IsExec ? "CODE" : "DATA", State->NextAddr, Size);

        if (Ptr + (Size / 4) > End) {
          return IMGLOAD_E_INVALID_FORMAT;
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
          return IMGLOAD_E_INVALID_FORMAT;
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
        return IMGLOAD_E_INVALID_FORMAT;
    }
  }

  return S_OK;
}

/**
  Load Amiga HUNK image.
**/
static
HRESULT
STDMETHODCALLTYPE
HunkLoadImage (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  HUNK_LOAD_STATE State = {0};
  HRESULT Hr;

  if (Context == NULL) {
    return E_POINTER;
  }

  info("Loading Amiga HUNK executable...");

  Hr = HunkParseFile(
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

  if (FAILED(Hr)) {
    return Hr;
  }

  Hr = HunkGetEntryPoint(&gHunkLoader, Context->ImageBase, &Context->EntryPoint);
  if (FAILED(Hr)) {
    return Hr;
  }

  return S_OK;
}

/**
  Extract TLS information from HUNK image.
**/
static
HRESULT
STDMETHODCALLTYPE
HunkGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // Amiga HUNK doesn't support TLS
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from HUNK image.
**/
static
HRESULT
STDMETHODCALLTYPE
HunkGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  // Amiga HUNK does not have unwinding information
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
HunkGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  UINT32 *Ptr = (UINT32 *)ImageBase;
  UINT32 *End;
  UINTN ImageSize = 0x100000;  // Assume max size for parsing

  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  End = (UINT32 *)((UINT8 *)ImageBase + ImageSize);

  // Skip HUNK_HEADER
  if (Ptr >= End || *Ptr != HUNK_HEADER) {
    return S_FALSE;
  }
  Ptr++;

  // Skip resident library names
  while (Ptr < End && *Ptr != 0) {
    UINT32 NameLen = *Ptr++;
    Ptr += NameLen;
  }
  Ptr++;  // Skip terminating 0

  // Skip hunk table
  if (Ptr + 3 > End) return S_FALSE;
  UINT32 NumHunks = *Ptr++;
  Ptr += 2;  // Skip FirstHunk, LastHunk
  Ptr += NumHunks;  // Skip hunk sizes

  // Parse hunks looking for HUNK_SYMBOL
  UINT32 CurrentHunk = 0;
  VIRTUAL_ADDRESS CurrentBase = AMIGA_BASE_ADDR;

  while (Ptr < End) {
    UINT32 Magic = *Ptr++;

    switch (Magic) {
      case HUNK_CODE:
      case HUNK_DATA: {
        UINT32 Size = HUNK_SIZE(*Ptr++) * 4;
        Ptr += Size / 4;
        break;
      }

      case HUNK_BSS: {
        HUNK_SIZE(*Ptr++);  // Just skip size
        break;
      }

      case HUNK_SYMBOL: {
        // Parse symbol entries for current hunk
        while (Ptr < End && *Ptr != 0) {
          UINT32 NameLen = *Ptr++;
          if (NameLen == 0) break;

          CHAR8 *Name = (CHAR8 *)Ptr;
          Ptr += NameLen;

          UINT32 Value = *Ptr++;
          VIRTUAL_ADDRESS SymAddr = CurrentBase + Value;

          if (SymAddr == Address) {
            // Found matching symbol
            UINTN CopyLen = ((NameLen * 4) < sizeof(SymbolInfo->Name) - 1) ?
                            (NameLen * 4) : (sizeof(SymbolInfo->Name) - 1);
            memcpy(SymbolInfo->Name, Name, CopyLen);
            SymbolInfo->Name[CopyLen] = '\0';
            SymbolInfo->Address = SymAddr;
            SymbolInfo->Size = 0;
            return S_OK;
          }
        }
        break;
      }

      case HUNK_RELOC32:
      case HUNK_DREL32:
      case HUNK_DREL16:
      case HUNK_DREL8:
        while (Ptr < End && *Ptr != 0) {
          UINT32 NumRelocs = *Ptr++;
          if (NumRelocs == 0) break;
          Ptr++;  // Skip hunk number
          Ptr += NumRelocs;
        }
        Ptr++;
        break;

      case HUNK_DEBUG:
        while (Ptr < End && *Ptr != 0) {
          UINT32 NameLen = *Ptr++;
          if (NameLen == 0) break;
          Ptr += NameLen;
          Ptr++;
        }
        break;

      case HUNK_END:
        CurrentHunk++;
        // Update CurrentBase based on hunk tracking (simplified)
        break;

      default:
        return S_FALSE;  // Unknown hunk
    }
  }

  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
HunkGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  UINT32 *Ptr = (UINT32 *)ImageBase;
  UINT32 *End;
  UINTN ImageSize = 0x100000;  // Assume max size for parsing
  UINTN SearchLen;

  if (SymbolInfo == NULL || Name == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  SearchLen = strlen(Name);
  End = (UINT32 *)((UINT8 *)ImageBase + ImageSize);

  // Skip HUNK_HEADER
  if (Ptr >= End || *Ptr != HUNK_HEADER) {
    return S_FALSE;
  }
  Ptr++;

  // Skip resident library names
  while (Ptr < End && *Ptr != 0) {
    UINT32 NameLen = *Ptr++;
    Ptr += NameLen;
  }
  Ptr++;  // Skip terminating 0

  // Skip hunk table
  if (Ptr + 3 > End) return S_FALSE;
  UINT32 NumHunks = *Ptr++;
  Ptr += 2;  // Skip FirstHunk, LastHunk
  Ptr += NumHunks;  // Skip hunk sizes

  // Parse hunks looking for HUNK_SYMBOL
  UINT32 CurrentHunk = 0;
  VIRTUAL_ADDRESS CurrentBase = AMIGA_BASE_ADDR;

  while (Ptr < End) {
    UINT32 Magic = *Ptr++;

    switch (Magic) {
      case HUNK_CODE:
      case HUNK_DATA: {
        UINT32 Size = HUNK_SIZE(*Ptr++) * 4;
        Ptr += Size / 4;
        break;
      }

      case HUNK_BSS: {
        HUNK_SIZE(*Ptr++);  // Just skip size
        break;
      }

      case HUNK_SYMBOL: {
        // Parse symbol entries for current hunk
        while (Ptr < End && *Ptr != 0) {
          UINT32 NameLen = *Ptr++;
          if (NameLen == 0) break;

          CHAR8 *SymName = (CHAR8 *)Ptr;
          UINTN SymNameBytes = NameLen * 4;
          Ptr += NameLen;

          UINT32 Value = *Ptr++;

          // Check if name matches (null-terminated comparison)
          if (SymNameBytes >= SearchLen) {
            if (memcmp(SymName, Name, SearchLen) == 0 &&
                (SymName[SearchLen] == '\0' || SearchLen == SymNameBytes)) {
              // Found matching symbol
              VIRTUAL_ADDRESS SymAddr = CurrentBase + Value;

              UINTN CopyLen = (SymNameBytes < sizeof(SymbolInfo->Name) - 1) ?
                              SymNameBytes : (sizeof(SymbolInfo->Name) - 1);
              memcpy(SymbolInfo->Name, SymName, CopyLen);
              SymbolInfo->Name[CopyLen] = '\0';
              SymbolInfo->Address = SymAddr;
              SymbolInfo->Size = 0;
              return S_OK;
            }
          }
        }
        break;
      }

      case HUNK_RELOC32:
      case HUNK_DREL32:
      case HUNK_DREL16:
      case HUNK_DREL8:
        while (Ptr < End && *Ptr != 0) {
          UINT32 NumRelocs = *Ptr++;
          if (NumRelocs == 0) break;
          Ptr++;  // Skip hunk number
          Ptr += NumRelocs;
        }
        Ptr++;
        break;

      case HUNK_DEBUG:
        while (Ptr < End && *Ptr != 0) {
          UINT32 NameLen = *Ptr++;
          if (NameLen == 0) break;
          Ptr += NameLen;
          Ptr++;
        }
        break;

      case HUNK_END:
        CurrentHunk++;
        // Update CurrentBase based on hunk tracking (simplified)
        break;

      default:
        return S_FALSE;  // Unknown hunk
    }
  }

  return S_FALSE;
}

/**
  Extract relocation information from HUNK image.
**/
static
HRESULT
STDMETHODCALLTYPE
HunkGetRelocInfo (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  OUT IMGLOAD_RELOC_INFO  *RelocInfo
  )
{
  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  // HUNK relocations are implemented in HunkApplyRelocations
  // Detection would require parsing the entire HUNK file which is
  // already done during loading
  RelocInfo->PreferredBase = AMIGA_BASE_ADDR;
  RelocInfo->RequiresReloc = TRUE;  // HUNK files typically have relocations
  RelocInfo->Format = 4;  // Custom HUNK format

  return S_OK;
}

/**
  Apply relocations to HUNK image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
HunkApplyRelocations (
  IN IImageLoader     *This,
  IN VOID             *ImageBase,
  IN VIRTUAL_ADDRESS  LoadAddress,
  IN VIRTUAL_ADDRESS  PreferredBase
  )
{
  UINT32 *Ptr = (UINT32 *)ImageBase;
  UINT32 *End;
  UINTN ImageSize = 0x100000;  // Assume max size
  INT32 Delta;
  VIRTUAL_ADDRESS CurrentBase = AMIGA_BASE_ADDR;
  UINT8 *CurrentHunkData = NULL;

  Delta = (INT32)((INT64)LoadAddress - (INT64)PreferredBase);
  if (Delta == 0) {
    return S_OK;  // No relocation needed
  }

  End = (UINT32 *)((UINT8 *)ImageBase + ImageSize);

  // Skip HUNK_HEADER
  if (Ptr >= End || *Ptr != HUNK_HEADER) {
    return E_INVALIDARG;
  }
  Ptr++;

  // Skip resident library names
  while (Ptr < End && *Ptr != 0) {
    UINT32 NameLen = *Ptr++;
    Ptr += NameLen;
  }
  Ptr++;

  // Skip hunk table
  if (Ptr + 3 > End) return E_INVALIDARG;
  UINT32 NumHunks = *Ptr++;
  Ptr += 2;  // Skip FirstHunk, LastHunk
  Ptr += NumHunks;  // Skip hunk sizes

  // Parse hunks and apply relocations
  while (Ptr < End) {
    UINT32 Magic = *Ptr++;

    switch (Magic) {
      case HUNK_CODE:
      case HUNK_DATA: {
        UINT32 Size = HUNK_SIZE(*Ptr++) * 4;
        CurrentHunkData = (UINT8 *)Ptr;
        Ptr += Size / 4;
        break;
      }

      case HUNK_BSS: {
        HUNK_SIZE(*Ptr++);
        CurrentHunkData = NULL;
        break;
      }

      case HUNK_RELOC32: {
        // Apply 32-bit relocations
        while (Ptr < End && *Ptr != 0) {
          UINT32 NumRelocs = *Ptr++;
          if (NumRelocs == 0) break;

          UINT32 TargetHunk = *Ptr++;  // Which hunk the relocs refer to

          // Apply each relocation
          for (UINT32 i = 0; i < NumRelocs && Ptr < End; i++) {
            UINT32 Offset = *Ptr++;

            if (CurrentHunkData != NULL) {
              UINT32 *RelocTarget = (UINT32 *)(CurrentHunkData + Offset);
              *RelocTarget += Delta;
            }
          }
        }
        Ptr++;  // Skip terminating 0
        break;
      }

      case HUNK_DREL32: {
        // Data-relative 32-bit relocations
        while (Ptr < End && *Ptr != 0) {
          UINT32 NumRelocs = *Ptr++;
          if (NumRelocs == 0) break;

          Ptr++;  // Skip target hunk

          // Apply each relocation
          for (UINT32 i = 0; i < NumRelocs && Ptr < End; i++) {
            UINT32 Offset = *Ptr++;

            if (CurrentHunkData != NULL) {
              UINT32 *RelocTarget = (UINT32 *)(CurrentHunkData + Offset);
              *RelocTarget += Delta;
            }
          }
        }
        Ptr++;
        break;
      }

      case HUNK_DREL16:
      case HUNK_DREL8:
        // Skip these for now (less common)
        while (Ptr < End && *Ptr != 0) {
          UINT32 NumRelocs = *Ptr++;
          if (NumRelocs == 0) break;
          Ptr++;  // Skip hunk number
          Ptr += NumRelocs;
        }
        Ptr++;
        break;

      case HUNK_SYMBOL:
      case HUNK_DEBUG:
        // Skip symbol/debug hunks
        while (Ptr < End && *Ptr != 0) {
          UINT32 NameLen = *Ptr++;
          if (NameLen == 0) break;
          Ptr += NameLen;
          Ptr++;
        }
        break;

      case HUNK_END:
        CurrentHunkData = NULL;
        break;

      default:
        return E_INVALIDARG;
    }
  }

  return S_OK;
}

//
// Amiga HUNK Loader VTable
//

static CONST IImageLoaderVtbl gHunkVtbl = {
  // IUnknown
  HunkQueryInterface,
  HunkAddRef,
  HunkRelease,
  // IImageLoader
  HunkDetect,
  HunkGetArch,
  HunkGetEndianness,
  HunkGetEntryPoint,
  HunkLoadImage,
  HunkGetTlsInfo,
  HunkGetUnwindInfo,
  HunkGetSymbolByAddress,
  HunkGetSymbolByName,
  HunkGetRelocInfo,
  HunkApplyRelocations
};

//
// Amiga HUNK Loader Instance
//

IImageLoader gHunkLoader = {
  &gHunkVtbl
};

APXH_REGISTER_IMGLOADER(gHunkLoader);
