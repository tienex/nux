/** @file
  APXH PDP-10 SAV Image Loader

  Provides PDP-10 SAV format recognition for TOPS-10/TOPS-20 executables.
  The SAV format is the executable format used on DEC PDP-10 mainframes
  running TOPS-10, TOPS-20, and ITS operating systems.

  Documentation:
  - TOPS-10 Monitor Calls Reference Manual
  - TOPS-20 Monitor Calls Reference Manual
  - DEC PDP-10 System Reference Manual
  - SAV file format: Simple header followed by core image
  - File extension: .SAV (Save file), .EXE (Executable)

  SAV File Structure:
  - Word 0: Entry point address (18-bit address in 36-bit word)
  - Word 1: High segment origin and size
  - Word 2: Low segment size
  - Followed by actual program data (36-bit words)

  NOTE: PDP-10 uses 36-bit words with big-endian byte order.
  This is a stub implementation for format detection only.

  Supports:
  - PDP-10 architecture (36-bit words)
  - Big-endian format
  - Format recognition only (loading not yet implemented)

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// PDP-10 SAV Format Constants
//

#define PDP10_WORD_SIZE    36   ///< PDP-10 word size in bits
#define PDP10_ADDR_MASK    0777777  ///< 18-bit address mask (octal)
#define PDP10_JRST_OPCODE  0254  ///< JRST (Jump and Restore) opcode
#define PDP10_WORD_MASK    0xFFFFFFFFFULL  ///< 36-bit mask

//
// PDP-10 Word Structure (36 bits stored in 64-bit for alignment)
//

typedef struct _PDP10_WORD {
  UINT64  Value;  ///< 36-bit word (upper 28 bits unused)
} PDP10_WORD;

//
// PDP-10 36-bit Word Encoding Formats
//

typedef enum _PDP10_ENCODING {
  Pdp10Encoding5Byte,    ///< 5-byte encoding (36 bits + 4 pad bits)
  Pdp10Encoding4Byte,    ///< 4-byte encoding (32 bits, truncated)
  Pdp10EncodingCore,     ///< Core dump format (binary)
} PDP10_ENCODING;

/**
  Read 36-bit word from PDP-10 5-byte encoding.

  Standard encoding: 5 bytes contain exactly 36 bits with 4 padding bits.
  Byte layout (big-endian):
    Byte 0: bits 35-28 (8 bits)
    Byte 1: bits 27-20 (8 bits)
    Byte 2: bits 19-12 (8 bits)
    Byte 3: bits 11-4  (8 bits)
    Byte 4: bits 3-0 + 4 pad bits (upper 4 bits are word bits)

  @param[in]  Buffer  Pointer to 5-byte buffer
  @param[out] Word    Receives 36-bit word value

  @return S_OK on success, error code otherwise.
**/
static
HRESULT
Pdp10Read36BitWord5Byte (
  IN  CONST UINT8  *Buffer,
  OUT UINT64       *Word
  )
{
  if (Buffer == NULL || Word == NULL) {
    return E_POINTER;
  }

  // Reconstruct 36-bit word from 5 bytes (big-endian)
  *Word = ((UINT64)Buffer[0] << 28) |  // Bits 35-28
          ((UINT64)Buffer[1] << 20) |  // Bits 27-20
          ((UINT64)Buffer[2] << 12) |  // Bits 19-12
          ((UINT64)Buffer[3] << 4)  |  // Bits 11-4
          ((UINT64)(Buffer[4] >> 4));  // Bits 3-0 (upper 4 bits of byte 4)

  *Word &= PDP10_WORD_MASK;  // Ensure only 36 bits
  return S_OK;
}

/**
  Read 36-bit word from PDP-10 4-byte encoding.

  Truncated encoding: Only 32 bits stored, upper 4 bits lost.
  Used in some transfer formats where full precision not needed.

  @param[in]  Buffer  Pointer to 4-byte buffer
  @param[out] Word    Receives 32-bit value (bits 35-32 = 0)

  @return S_OK on success, error code otherwise.
**/
static
HRESULT
Pdp10Read36BitWord4Byte (
  IN  CONST UINT8  *Buffer,
  OUT UINT64       *Word
  )
{
  if (Buffer == NULL || Word == NULL) {
    return E_POINTER;
  }

  // Reconstruct 32-bit value (upper 4 bits = 0)
  *Word = ((UINT64)Buffer[0] << 24) |
          ((UINT64)Buffer[1] << 16) |
          ((UINT64)Buffer[2] << 8)  |
          ((UINT64)Buffer[3]);

  return S_OK;
}

/**
  Read multiple 36-bit words from buffer.

  @param[in]  Buffer    Source buffer
  @param[in]  Count     Number of words to read
  @param[in]  Encoding  Word encoding format
  @param[out] Words     Output array (must hold Count elements)

  @return S_OK on success, error code otherwise.
**/
static
HRESULT
Pdp10ReadWords (
  IN  CONST UINT8       *Buffer,
  IN  UINTN             Count,
  IN  PDP10_ENCODING    Encoding,
  OUT UINT64            *Words
  )
{
  UINTN i;
  UINTN BytesPerWord;
  HRESULT Status;

  if (Buffer == NULL || Words == NULL) {
    return E_POINTER;
  }

  switch (Encoding) {
    case Pdp10Encoding5Byte:
      BytesPerWord = 5;
      break;
    case Pdp10Encoding4Byte:
      BytesPerWord = 4;
      break;
    default:
      return E_INVALIDARG;
  }

  for (i = 0; i < Count; i++) {
    if (Encoding == Pdp10Encoding5Byte) {
      Status = Pdp10Read36BitWord5Byte(&Buffer[i * BytesPerWord], &Words[i]);
    } else {
      Status = Pdp10Read36BitWord4Byte(&Buffer[i * BytesPerWord], &Words[i]);
    }

    if (FAILED(Status)) {
      return Status;
    }
  }

  return S_OK;
}

/**
  Extract field from 36-bit word.

  PDP-10 instructions and data use various field formats:
  - Opcode: bits 35-27 (9 bits)
  - AC (accumulator): bits 26-23 (4 bits)
  - Index: bits 22-18 (5 bits)
  - Indirect bit: bit 17 (1 bit)
  - Address: bits 17-0 or 17-0 depending on format

  @param[in] Word    36-bit word value
  @param[in] Start   Starting bit position (0-35, MSB = 35)
  @param[in] Length  Number of bits to extract

  @return Extracted field value.
**/
static
UINT64
Pdp10ExtractField (
  IN UINT64  Word,
  IN UINT32  Start,
  IN UINT32  Length
  )
{
  UINT64 Mask;
  UINT32 Shift;

  if (Length == 0 || Length > 36 || Start >= 36) {
    return 0;
  }

  Mask = (1ULL << Length) - 1;
  Shift = (35 - Start - Length + 1);

  return (Word >> Shift) & Mask;
}

//
// IImageLoader Implementation for PDP-10 SAV
//

/**
  Detect if image is PDP-10 SAV format.

  SAV files have limited magic number detection. We check:
  - File size is reasonable (at least 3 words = 18 bytes for 5-byte encoding)
  - Entry point looks valid (JRST instruction or reasonable address)
**/
static
HRESULT
STDMETHODCALLTYPE
Pdp10Detect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  UINT8 *Data;

  // Minimum size for SAV header (3 words encoded)
  if (ImageSize < 18) {
    return S_FALSE;
  }

  Data = (UINT8 *)ImageBase;

  // SAV format detection is heuristic-based
  // Check if first bytes could be a valid PDP-10 instruction or address
  // This is a weak detection - improve with better heuristics

  // For now, we'll return S_FALSE since we can't reliably detect without
  // more format information or a specific magic number
  return S_FALSE;
}

/**
  Get architecture from PDP-10 SAV image.
**/
static
HRESULT
STDMETHODCALLTYPE
Pdp10GetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  if (Architecture == NULL) {
    return E_POINTER;
  }

  *Architecture = ARCH_PDP10;
  return S_OK;
}

/**
  Get endianness from PDP-10 SAV image.
**/
static
HRESULT
STDMETHODCALLTYPE
Pdp10GetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // PDP-10 uses big-endian byte order
  *Endianness = ImgEndianBig;
  return S_OK;
}

/**
  Get entry point from PDP-10 SAV image.
**/
static
HRESULT
STDMETHODCALLTYPE
Pdp10GetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  // Entry point extraction requires understanding 36-bit word encoding
  // Stub implementation
  *EntryPoint = 0;
  return E_NOTIMPL;
}

/**
  Load PDP-10 SAV image.
**/
static
HRESULT
STDMETHODCALLTYPE
Pdp10LoadImage (
  IN     IImageLoader     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  if (Context == NULL) {
    return E_POINTER;
  }

  // Loading PDP-10 images requires:
  // - 36-bit word decoding from various encodings (5-byte, 4-byte)
  // - Understanding high/low segment layout
  // - PDP-10 virtual address space mapping
  // This is beyond current implementation scope

  return E_NOTIMPL;
}

/**
  Extract TLS information from PDP-10 SAV image.
**/
static
HRESULT
STDMETHODCALLTYPE
Pdp10GetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // PDP-10 did not have thread-local storage
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from PDP-10 SAV image.
**/
static
HRESULT
STDMETHODCALLTYPE
Pdp10GetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  // PDP-10 did not have structured exception handling
  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
Pdp10GetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  // SAV format does not include symbol tables
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
Pdp10GetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  // SAV format does not include symbol tables
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Extract relocation information from PDP-10 image.
**/
static
HRESULT
STDMETHODCALLTYPE
Pdp10GetRelocInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_RELOC_INFO   *RelocInfo
  )
{
  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  // PDP-10 SAV format typically does not contain relocations
  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));
  RelocInfo->Format = 8;  // PDP-10 format
  RelocInfo->RequiresReloc = FALSE;

  return S_FALSE;
}

/**
  Apply relocations to PDP-10 image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
Pdp10ApplyRelocations (
  IN VOID              *ImageBase,
  IN VIRTUAL_ADDRESS   LoadAddress,
  IN VIRTUAL_ADDRESS   PreferredBase
  )
{
  INT64 Delta;

  // Calculate relocation delta
  Delta = (INT64)LoadAddress - (INT64)PreferredBase;

  if (Delta == 0) {
    return S_OK;  // No relocation needed
  }

  // PDP-10 SAV format does not support relocation
  return E_NOTIMPL;
}

/**
  IUnknown::QueryInterface implementation (stub).
**/
static
HRESULT
STDMETHODCALLTYPE
Pdp10QueryInterface (
  IN  IImageLoader  *This,
  IN  REFIID        riid,
  OUT VOID          **ppvObject
  )
{
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  *ppvObject = NULL;

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
Pdp10AddRef (
  IN IImageLoader  *This
  )
{
  return 1;
}

/**
  IUnknown::Release implementation (stub - static object).
**/
static
UINT32
STDMETHODCALLTYPE
Pdp10Release (
  IN IImageLoader  *This
  )
{
  return 1;
}

//
// PDP-10 Loader VTable
//

static CONST IImageLoaderVtbl gPdp10Vtbl = {
  Pdp10QueryInterface,
  Pdp10AddRef,
  Pdp10Release,
  Pdp10Detect,
  Pdp10GetArch,
  Pdp10GetEndianness,
  Pdp10GetEntryPoint,
  Pdp10LoadImage,
  Pdp10GetTlsInfo,
  Pdp10GetUnwindInfo,
  Pdp10GetSymbolByAddress,
  Pdp10GetSymbolByName,
  Pdp10GetRelocInfo,
  Pdp10ApplyRelocations
};

//
// PDP-10 Loader Instance
//

IImageLoader gPdp10Loader = {
  &gPdp10Vtbl
};

// Auto-register this loader
ANX_REGISTER_IMGLOADER(gPdp10Loader);
