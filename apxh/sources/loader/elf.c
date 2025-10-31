/** @file
  APXH ELF Loader Implementation

  Provides ELF (Executable and Linkable Format) parsing and loading
  for 32-bit and 64-bit executables using COM-style interface.
  Handles program headers, TLS, and unwinding information extraction.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// External ELF functions (from elf_impl.c)
//

extern ARCH GetElfArch (IN VOID *ElfImage);
extern IMGLOAD_ENDIAN GetElfEndianness (IN VOID *ElfImage);
extern VIRTUAL_ADDRESS LoadElf32 (IN VOID *ElfImage, IN INT32 IsUserMode);
extern VIRTUAL_ADDRESS LoadElf64 (IN VOID *ElfImage, IN INT32 IsUserMode);
extern HRESULT GetElf32UnwindInfo (IN VOID *ElfImage, OUT IMGLOAD_UNWIND_INFO *UnwindInfo);
extern HRESULT GetElf64UnwindInfo (IN VOID *ElfImage, OUT IMGLOAD_UNWIND_INFO *UnwindInfo);
extern HRESULT GetElf32SymbolByAddress (IN VOID *ElfImage, IN VIRTUAL_ADDRESS Address, OUT IMGLOAD_SYMBOL_INFO *SymbolInfo);
extern HRESULT GetElf64SymbolByAddress (IN VOID *ElfImage, IN VIRTUAL_ADDRESS Address, OUT IMGLOAD_SYMBOL_INFO *SymbolInfo);
extern HRESULT GetElf32SymbolByName (IN VOID *ElfImage, IN CONST CHAR8 *Name, OUT IMGLOAD_SYMBOL_INFO *SymbolInfo);
extern HRESULT GetElf64SymbolByName (IN VOID *ElfImage, IN CONST CHAR8 *Name, OUT IMGLOAD_SYMBOL_INFO *SymbolInfo);

//
// ELF Magic Numbers
//

#define ELF_MAGIC  0x7F454C46  ///< "\x7FELF"

//
// ELF Class
//

#define ELFCLASS32  1  ///< 32-bit ELF
#define ELFCLASS64  2  ///< 64-bit ELF

//
// IImageLoader Implementation for ELF
//

/**
  Detect if image is ELF format.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfDetect (
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
  return (*Magic == ELF_MAGIC) ? S_OK : S_FALSE;
}

/**
  Get architecture from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  if (Architecture == NULL) {
    return E_POINTER;
  }

  *Architecture = GetElfArch(ImageBase);

  if (*Architecture == ARCH_INVALID) {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  if (*Architecture == ARCH_UNSUPPORTED) {
    return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  return S_OK;
}

/**
  Get endianness from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  *Endianness = GetElfEndianness(ImageBase);

  if (*Endianness == ImgEndianUnknown) {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  return S_OK;
}

/**
  Get entry point from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  UINT8 *Ident;
  VIRTUAL_ADDRESS Entry;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  Ident = (UINT8 *)ImageBase;

  // Check ELF class (32-bit or 64-bit)
  if (Ident[4] == ELFCLASS32) {
    // For 32-bit ELF, entry point extraction requires full load
    // Return entry from header for now
    typedef struct {
      UINT8  Id[16];
      UINT16 Type;
      UINT16 Mach;
      UINT32 Ver;
      UINT32 Entry;
    } ELF32_HDR_PARTIAL;
    *EntryPoint = ((ELF32_HDR_PARTIAL *)ImageBase)->Entry;
  } else if (Ident[4] == ELFCLASS64) {
    // For 64-bit ELF
    typedef struct {
      UINT8  Id[16];
      UINT16 Type;
      UINT16 Mach;
      UINT32 Ver;
      UINT64 Entry;
    } ELF64_HDR_PARTIAL;
    *EntryPoint = ((ELF64_HDR_PARTIAL *)ImageBase)->Entry;
  } else {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  if (*EntryPoint == 0 || *EntryPoint == (VIRTUAL_ADDRESS)-1) {
    return IMGLOAD_E_INVALID_HEADER;
  }

  return S_OK;
}

/**
  Load ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfLoadImage (
  IN     IImageLoader     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VIRTUAL_ADDRESS EntryPoint;
  UINT8 *Ident;
  HRESULT Status;

  if (Context == NULL) {
    return E_POINTER;
  }

  Ident = (UINT8 *)Context->ImageBase;

  // Populate architecture and endianness
  Status = ElfGetArch(This, Context->ImageBase, &Context->Architecture);
  if (FAILED(Status)) {
    return Status;
  }

  Status = ElfGetEndianness(This, Context->ImageBase, &Context->Endianness);
  if (FAILED(Status)) {
    return Status;
  }

  // Check ELF class (32-bit or 64-bit)
  if (Ident[4] == ELFCLASS32) {
    // 32-bit ELF
    EntryPoint = LoadElf32(Context->ImageBase, Context->IsUserMode);
  } else if (Ident[4] == ELFCLASS64) {
    // 64-bit ELF
    EntryPoint = LoadElf64(Context->ImageBase, Context->IsUserMode);
  } else {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  if (EntryPoint == (VIRTUAL_ADDRESS)-1) {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  Context->EntryPoint = EntryPoint;
  return S_OK;
}

/**
  Extract TLS information from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // TLS is handled internally by LoadElf32/LoadElf64
  // They call VirtualAddressMapKernelTls/VirtualAddressMapUserTls
  // For now, return S_FALSE to indicate no TLS info available via this method
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from ELF image.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  UINT8 *Ident;
  HRESULT Status;

  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  Ident = (UINT8 *)ImageBase;

  // Check ELF class (32-bit or 64-bit)
  if (Ident[4] == ELFCLASS32) {
    Status = GetElf32UnwindInfo(ImageBase, UnwindInfo);
  } else if (Ident[4] == ELFCLASS64) {
    Status = GetElf64UnwindInfo(ImageBase, UnwindInfo);
  } else {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  return Status;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  UINT8 *Ident;
  HRESULT Status;

  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  Ident = (UINT8 *)ImageBase;

  // Check ELF class (32-bit or 64-bit)
  if (Ident[4] == ELFCLASS32) {
    Status = GetElf32SymbolByAddress(ImageBase, Address, SymbolInfo);
  } else if (Ident[4] == ELFCLASS64) {
    Status = GetElf64SymbolByAddress(ImageBase, Address, SymbolInfo);
  } else {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  return Status;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
ElfGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  UINT8 *Ident;
  HRESULT Status;

  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  Ident = (UINT8 *)ImageBase;

  // Check ELF class (32-bit or 64-bit)
  if (Ident[4] == ELFCLASS32) {
    Status = GetElf32SymbolByName(ImageBase, Name, SymbolInfo);
  } else if (Ident[4] == ELFCLASS64) {
    Status = GetElf64SymbolByName(ImageBase, Name, SymbolInfo);
  } else {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  return Status;
}

/**
  IUnknown::QueryInterface implementation (stub).
**/
static
HRESULT
STDMETHODCALLTYPE
ElfQueryInterface (
  IN  IImageLoader  *This,
  IN  REFIID        riid,
  OUT VOID          **ppvObject
  )
{
  // Simple implementation - only support IImageLoader and IUnknown
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  *ppvObject = NULL;

  // Compare GUIDs (simplified)
  if (memcmp(riid, &IID_IImageLoader, sizeof(GUID)) == 0 ||
      memcmp(riid, &IID_IUnknown, sizeof(GUID)) == 0) {
    *ppvObject = This;
    return S_OK;
  }

  return E_NOINTERFACE;
}

/**
  IUnknown::AddRef implementation (stub - static object).
**/
static
UINT32
STDMETHODCALLTYPE
ElfAddRef (
  IN IImageLoader  *This
  )
{
  // Static object, no reference counting
  return 1;
}

/**
  IUnknown::Release implementation (stub - static object).
**/
static
UINT32
STDMETHODCALLTYPE
ElfRelease (
  IN IImageLoader  *This
  )
{
  // Static object, no reference counting
  return 1;
}

//
// ELF Loader VTable
//

#ifdef __cplusplus
// C++ mode not supported in this implementation
#error "C++ mode not implemented"
#else
static CONST IImageLoaderVtbl gElfVtbl = {
  ElfQueryInterface,
  ElfAddRef,
  ElfRelease,
  ElfDetect,
  ElfGetArch,
  ElfGetEndianness,
  ElfGetEntryPoint,
  ElfLoadImage,
  ElfGetTlsInfo,
  ElfGetUnwindInfo,
  ElfGetSymbolByAddress,
  ElfGetSymbolByName
};
#endif

//
// ELF Loader Instance
//

IImageLoader gElfLoader = {
  &gElfVtbl
};

// Auto-register this loader
ANX_REGISTER_IMGLOADER(gElfLoader);
