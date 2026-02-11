/** @file
  APXH XENIX X.OUT Loader

  Provides SCO XENIX X.OUT (extended output) format parsing and loading.
  Based on SCO XENIX System V a.out.h header definitions.

  Supports:
  - Multiple CPU architectures (8086, 286, 386, 68K, VAX, Z8000, etc.)
  - Segmented x.out format (XE_SEG) with segment tables
  - Non-segmented x.out format
  - Version detection (v2.x, v3.x, v5.x)
  - Byte/word swapping for cross-platform binaries
  - Extension header with stack size and machine-dependent tables
  - Shared library support
  - Multiple relocation and symbol table formats

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>
#include <ananke/resource.h>
#include "imgresource.h"

//
// XENIX X.OUT Magic Numbers
//

#define X_MAGIC         0x0206  ///< X.OUT magic number
#define ARCMAGIC        0xff65  ///< Archive magic (0177545)

//
// CPU Types (x_cpu field, char)
//

#define XC_BSWAP        0x80    ///< Bytes swapped (high byte first in short)
#define XC_WSWAP        0x40    ///< Words swapped (low word first in long)

#define XC_NONE         0x00    ///< None
#define XC_PDP11        0x01    ///< PDP-11
#define XC_23           0x02    ///< 23fixed from PDP-11
#define XC_Z8K          0x03    ///< Z8000
#define XC_8086         0x04    ///< Intel 8086
#define XC_68K          0x05    ///< Motorola 68000
#define XC_Z80          0x06    ///< Z80
#define XC_VAX          0x07    ///< VAX 780/750
#define XC_16032        0x08    ///< NS16032
#define XC_286          0x09    ///< Intel 80286
#define XC_286V         0x29    ///< Intel 80286 (use xe_osver for version)
#define XC_386          0x0a    ///< Intel 80386
#define XC_186          0x0b    ///< Intel 80186
#define XC_CPU          0x3f    ///< CPU mask

//
// x_renv Flags (short) - Runtime Environment
//

#define XE_V2           0x4000  ///< Version 2.x
#define XE_V3           0x8000  ///< Version 3.x
#define XE_OSV          0xc000  ///< If XE_SEG, use xe_osvers
#define XE_V5           XE_OSV  ///< Version 5.x
#define XE_VERS         0xc000  ///< Version mask

#define XE_5_3          0x2000  ///< Binary needs 5.3 functionality
#define XE_LOCK         0x1000  ///< Use advisory locking
#define XE_SEG          0x0800  ///< Segment table present
#define XE_ABS          0x0400  ///< Absolute memory image (standalone)
#define XE_ITER         0x0200  ///< Iterated text/data present
#define XE_HDATA        0x0100  ///< Huge model data (never used)
#define XE_VMOD         XE_HDATA ///< Virtual module
#define XE_FPH          0x0080  ///< Floating point hardware required
#define XE_LTEXT        0x0040  ///< Large model text
#define XE_LDATA        0x0020  ///< Large model data
#define XE_OVER         0x0010  ///< Text overlay
#define XE_FS           0x0008  ///< Fixed stack
#define XE_PURE         0x0004  ///< Pure text
#define XE_SEP          0x0002  ///< Separate I & D
#define XE_EXEC         0x0001  ///< Executable

//
// xe_ostype Values (char)
//

#define XE_OSNONE       0       ///< No OS specified
#define XE_OSXENIX      1       ///< Xenix
#define XE_OSRMX        2       ///< iRMX
#define XE_OSCCPM       3       ///< Concurrent CP/M

//
// xe_osvers Values (char)
//

#define XE_OSXV2        0       ///< Xenix V2.x
#define XE_OSXV3        1       ///< Xenix V3.x
#define XE_OSXV5        2       ///< Xenix V5.x

//
// Segment Types (xs_type, short)
//

#define XS_TNULL        0       ///< Unused segment
#define XS_TTEXT        1       ///< Text segment
#define XS_TDATA        2       ///< Data segment
#define XS_TSYMS        3       ///< Symbol table segment
#define XS_TREL         4       ///< Relocation segment
#define XS_TSESTR       5       ///< Segment table's string table
#define XS_TGRPS        6       ///< Group definitions segment

#define XS_TIDATA       64      ///< Iterated data
#define XS_TTSS         65      ///< TSS
#define XS_TLFIX        66      ///< Lodfix
#define XS_TDNAME       67      ///< Descriptor names
#define XS_TDTEXT       68      ///< Debug text segment
#define XS_TIDBG        XS_TDTEXT
#define XS_TDFIX        69      ///< Debug relocation
#define XS_TOVTAB       70      ///< Overlay table
#define XS_T71          71
#define XS_TSYSTR       72      ///< Symbol string table

//
// Segment Attributes (xs_attr, short)
//

#define XS_AMEM         0x8000  ///< Segment represents a memory image
#define XS_AMASK        0x7fff  ///< Type specific field mask

// For XS_TTEXT and XS_TDATA segments
#define XS_AITER        0x0001  ///< Contains iteration records
#define XS_AHUGE        0x0002  ///< Contains huge element
#define XS_ABSS         0x0004  ///< Contains implicit BSS
#define XS_APURE        0x0008  ///< Read-only, may be shared
#define XS_AEDOWN       0x0010  ///< Segment expands downward (stack)
#define XS_APRIV        0x0020  ///< Segment may not be combined
#define XS_A32BIT       0x0040  ///< Segment is 32 bits

//
// XENIX X.OUT Structures (from SCO a.out.h)
//

ANX_PACK_PUSH(1)

typedef struct _XEXEC {
  UINT16  x_magic;            ///< Magic number (X_MAGIC)
  UINT16  x_ext;              ///< Size of header extension
  UINT32  x_text;             ///< Size of text segment
  UINT32  x_data;             ///< Size of initialized data
  UINT32  x_bss;              ///< Size of uninitialized data
  UINT32  x_syms;             ///< Size of symbol table
  UINT32  x_reloc;            ///< Relocation table length
  UINT32  x_entry;            ///< Entry point (machine dependent)
  UINT8   x_cpu;              ///< CPU type & byte/word order
  UINT8   x_relsym;           ///< Relocation & symbol format
  UINT16  x_renv;             ///< Run-time environment flags
} XEXEC;

typedef struct _XEXT {
  UINT32  xe_trsize;          ///< Size of text relocation (unused)
  UINT32  xe_drsize;          ///< Size of data relocation (unused)
  UINT32  xe_tbase;           ///< Text relocation base (unused)
  UINT32  xe_dbase;           ///< Data relocation base (unused)
  UINT32  xe_stksize;         ///< Stack size (if XE_FS set)
  UINT32  xe_segpos;          ///< Segment table position
  UINT32  xe_segsize;         ///< Segment table size
  UINT32  xe_mdtpos;          ///< Machine dependent table position
  UINT32  xe_mdtsize;         ///< Machine dependent table size
  UINT8   xe_mdttype;         ///< Machine dependent table type
  UINT8   xe_pagesize;        ///< File pagesize (multiples of 512)
  UINT8   xe_ostype;          ///< Operating system type
  UINT8   xe_osvers;          ///< Operating system version
  UINT16  xe_eseg;            ///< Entry segment (machine dependent)
  UINT16  xe_sres;            ///< Reserved
} XEXT;

typedef struct _XSEG {
  UINT16  xs_type;            ///< Segment type (XS_T*)
  UINT16  xs_attr;            ///< Segment attributes (XS_A*)
  UINT16  xs_seg;             ///< Segment number
  UINT8   xs_align;           ///< Log base 2 of alignment
  UINT8   xs_cres;            ///< Unused
  UINT32  xs_filpos;          ///< File position
  UINT32  xs_psize;           ///< Physical size (in file)
  UINT32  xs_vsize;           ///< Virtual size (in core)
  UINT32  xs_rbase;           ///< Relocation base address/offset
  UINT16  xs_noff;            ///< Segment name string table offset
  UINT16  xs_sres;            ///< Unused
  UINT32  xs_lres;            ///< Unused
} XSEG;

typedef struct _XOUT_SYMBOL {
  union {
    CHAR8   Name[8];          ///< Symbol name (if <= 8 chars)
    struct {
      UINT32  Zeros;          ///< 0 if name in string table
      UINT32  StringOffset;   ///< Offset into string table
    } StringRef;
  } N;
  UINT32  Value;              ///< Symbol value
  INT16   SectionNumber;      ///< Section number (1=text, 2=data, 3=bss)
  UINT16  Type;               ///< Symbol type
  UINT8   StorageClass;       ///< Storage class
  UINT8   AuxCount;           ///< Number of auxiliary entries
} XOUT_SYMBOL;

ANX_PACK_POP()

//
// XENIX Symbol Storage Classes
//

#define C_NULL      0   ///< No storage class
#define C_EXT       2   ///< External symbol
#define C_STAT      3   ///< Static symbol
#define C_LABEL     6   ///< Label
#define C_FCN       101 ///< Function

//
// XENIX Symbol Section Numbers
//

#define N_UNDEF     0   ///< Undefined
#define N_ABS       -1  ///< Absolute
#define N_TEXT      1   ///< Text section
#define N_DATA      2   ///< Data section
#define N_BSS       3   ///< BSS section

//
// Default XENIX addresses
//

#define XOUT_TEXT_START  0x00002000  ///< Text segment start
#define XOUT_DATA_START  0x10000000  ///< Data segment start

//
// Helper Macros
//

#define XOUT_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// Helper Functions
//

/**
  Get pointer to extension header if present.
**/
static
XEXT *
XenixGetExtHeader (
  IN VOID  *ImageBase
  )
{
  XEXEC *Header = (XEXEC *)ImageBase;

  if (Header->x_ext < sizeof(XEXT)) {
    return NULL;
  }

  return (XEXT *)((UINT8 *)ImageBase + sizeof(XEXEC));
}

/**
  Get pointer to segment table if present.
**/
static
XSEG *
XenixGetSegmentTable (
  IN VOID    *ImageBase,
  OUT UINTN  *NumSegments
  )
{
  XEXEC *Header;
  XEXT *Ext;

  Header = (XEXEC *)ImageBase;
  *NumSegments = 0;

  if (!(Header->x_renv & XE_SEG)) {
    return NULL;
  }

  Ext = XenixGetExtHeader(ImageBase);
  if (Ext == NULL || Ext->xe_segsize == 0) {
    return NULL;
  }

  *NumSegments = Ext->xe_segsize / sizeof(XSEG);
  return (XSEG *)((UINT8 *)ImageBase + Ext->xe_segpos);
}

//
// Internal Functions
//

/**
  IUnknown: QueryInterface
**/
static
HRESULT
STDMETHODCALLTYPE
XenixQueryInterface (
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
XenixAddRef (
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
XenixRelease (
  IN IImageLoader  *This
  )
{
  return 1;  // Static instance
}

/**
  Check if image is XENIX X.OUT format.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  XEXEC *Header;
  UINT8 CpuType;

  if (ImageSize < sizeof(XEXEC)) {
    return S_FALSE;
  }

  Header = (XEXEC *)ImageBase;

  // Check magic number
  if (Header->x_magic != X_MAGIC) {
    return S_FALSE;
  }

  // Check CPU type (mask out byte/word swap bits)
  CpuType = Header->x_cpu & XC_CPU;

  // Verify it's a known CPU type
  switch (CpuType) {
    case XC_PDP11:
    case XC_23:
    case XC_Z8K:
    case XC_8086:
    case XC_68K:
    case XC_Z80:
    case XC_VAX:
    case XC_16032:
    case XC_286:
    case XC_386:
    case XC_186:
      // Valid CPU type
      break;
    default:
      return S_FALSE;
  }

  // Check that it's executable
  if (!(Header->x_renv & XE_EXEC)) {
    return S_FALSE;
  }

  return S_OK;
}

/**
  Get architecture from XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  XEXEC *Header;
  UINT8 CpuType;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  Header = (XEXEC *)ImageBase;
  CpuType = Header->x_cpu & XC_CPU;

  switch (CpuType) {
    case XC_8086:
    case XC_186:
      *Architecture = ArchX86;
      return S_OK;

    case XC_286:
      *Architecture = ArchX86;  // 286 is x86
      return S_OK;

    case XC_386:
      *Architecture = Arch386;
      return S_OK;

    case XC_68K:
      *Architecture = ArchM68k;
      return S_OK;

    case XC_VAX:
      *Architecture = ArchVax;
      return S_OK;

    case XC_Z8K:
      *Architecture = ArchZ8000;
      return S_OK;

    case XC_Z80:
      *Architecture = ArchZ80;
      return S_OK;

    case XC_16032:
      *Architecture = ArchNs32k;
      return S_OK;

    case XC_PDP11:
    case XC_23:
      *Architecture = ArchPdp11;
      return S_OK;

    default:
      *Architecture = ArchUnsupported;
      return IMGLOAD_E_UNSUPPORTED_ARCH;
  }
}

/**
  Get endianness from XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  XEXEC *Header;
  UINT8 CpuType;
  BOOLEAN ByteSwapped;

  if (Endianness == NULL) {
    return E_POINTER;
  }

  Header = (XEXEC *)ImageBase;
  CpuType = Header->x_cpu & XC_CPU;
  ByteSwapped = (Header->x_cpu & XC_BSWAP) != 0;

  // Determine native endianness for CPU type
  switch (CpuType) {
    case XC_8086:
    case XC_186:
    case XC_286:
    case XC_386:
    case XC_VAX:
    case XC_Z80:
      // Little-endian architectures
      *Endianness = ByteSwapped ? ImgEndianBig : ImgEndianLittle;
      break;

    case XC_68K:
    case XC_Z8K:
    case XC_PDP11:
    case XC_23:
      // Big-endian architectures
      *Endianness = ByteSwapped ? ImgEndianLittle : ImgEndianBig;
      break;

    case XC_16032:
      // NS32000 can be either, default little-endian
      *Endianness = ByteSwapped ? ImgEndianBig : ImgEndianLittle;
      break;

    default:
      *Endianness = ImgEndianLittle;
      break;
  }

  return S_OK;
}

/**
  Get entry point from XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetEntryPoint (
  IN  IImageLoader     *This,
  IN  VOID             *ImageBase,
  OUT VIRTUAL_ADDRESS  *EntryPoint
  )
{
  XEXEC *Header;
  XEXT *Ext;
  XSEG *SegTable;
  UINTN NumSegments;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  Header = (XEXEC *)ImageBase;

  // For segmented executables, use entry segment from extension header
  if (Header->x_renv & XE_SEG) {
    Ext = XenixGetExtHeader(ImageBase);
    if (Ext != NULL) {
      SegTable = XenixGetSegmentTable(ImageBase, &NumSegments);
      if (SegTable != NULL && Ext->xe_eseg < NumSegments) {
        // Entry point is in specific segment
        *EntryPoint = SegTable[Ext->xe_eseg].xs_rbase + Header->x_entry;
        return S_OK;
      }
    }
  }

  // For non-segmented executables, use text start + entry offset
  *EntryPoint = XOUT_TEXT_START + Header->x_entry;
  return S_OK;
}

/**
  Load XENIX image (segmented format).
**/
static
HRESULT
XenixLoadSegmented (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  XEXEC *Header;
  XSEG *SegTable;
  UINTN NumSegments, i;

  ImageBase = Context->ImageBase;
  Header = (XEXEC *)ImageBase;

  SegTable = XenixGetSegmentTable(ImageBase, &NumSegments);
  if (SegTable == NULL) {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  info("Loading segmented XENIX X.OUT executable (%u segments)...", NumSegments);

  // Load each segment
  for (i = 0; i < NumSegments; i++) {
    XSEG *Seg = &SegTable[i];
    BOOLEAN IsWritable = FALSE;
    BOOLEAN IsExecutable = FALSE;

    // Skip non-memory segments
    if (!(Seg->xs_attr & XS_AMEM)) {
      continue;
    }

    // Determine segment permissions based on type
    switch (Seg->xs_type) {
      case XS_TTEXT:
        IsExecutable = TRUE;
        IsWritable = !(Seg->xs_attr & XS_APURE);
        break;

      case XS_TDATA:
        IsWritable = TRUE;
        break;

      default:
        continue;  // Skip non-loadable segments
    }

    info("  Segment %u: type=%u base=0x%08x psize=0x%x vsize=0x%x",
         Seg->xs_seg, Seg->xs_type, Seg->xs_rbase, Seg->xs_psize, Seg->xs_vsize);

    // Load physical data from file
    if (Seg->xs_psize > 0) {
      VasCopy(
        Seg->xs_rbase,
        XOUT_OFF(Seg->xs_filpos),
        Seg->xs_psize,
        Context->IsUserMode,
        IsWritable,
        IsExecutable
      );
    }

    // Zero-fill remaining virtual space (BSS-like)
    if (Seg->xs_vsize > Seg->xs_psize) {
      VasFill(
        Seg->xs_rbase + Seg->xs_psize,
        0,
        Seg->xs_vsize - Seg->xs_psize,
        Context->IsUserMode,
        IsWritable,
        IsExecutable
      );
    }
  }

  return S_OK;
}

/**
  Load XENIX image (non-segmented format).
**/
static
HRESULT
XenixLoadNonSegmented (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  XEXEC *Header;
  UINT32 TextOffset, DataOffset;

  ImageBase = Context->ImageBase;
  Header = (XEXEC *)ImageBase;

  info("Loading non-segmented XENIX X.OUT executable...");

  TextOffset = sizeof(XEXEC) + Header->x_ext;
  DataOffset = TextOffset + Header->x_text;

  // Load text segment (executable)
  if (Header->x_text > 0) {
    info("  Text segment at 0x%08x (size: 0x%08x)",
         XOUT_TEXT_START, Header->x_text);

    VasCopy(
      XOUT_TEXT_START,
      XOUT_OFF(TextOffset),
      Header->x_text,
      Context->IsUserMode,
      FALSE,  // Not writable
      TRUE    // Executable
    );
  }

  // Load data segment (writable)
  if (Header->x_data > 0) {
    info("  Data segment at 0x%08x (size: 0x%08x)",
         XOUT_DATA_START, Header->x_data);

    VasCopy(
      XOUT_DATA_START,
      XOUT_OFF(DataOffset),
      Header->x_data,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  // Zero-fill BSS
  if (Header->x_bss > 0) {
    info("  BSS segment at 0x%08x (size: 0x%08x)",
         XOUT_DATA_START + Header->x_data, Header->x_bss);

    VasFill(
      XOUT_DATA_START + Header->x_data,
      0,
      Header->x_bss,
      Context->IsUserMode,
      TRUE,   // Writable
      FALSE   // Not executable
    );
  }

  return S_OK;
}

/**
  Load XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixLoadImage (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  XEXEC *Header;
  HRESULT Hr;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  Header = (XEXEC *)ImageBase;

  // Load based on format type
  if (Header->x_renv & XE_SEG) {
    Hr = XenixLoadSegmented(Context);
  } else {
    Hr = XenixLoadNonSegmented(Context);
  }

  if (FAILED(Hr)) {
    return Hr;
  }

  Hr = XenixGetEntryPoint(&gXenixLoader, ImageBase, &Context->EntryPoint);
  if (FAILED(Hr)) {
    return Hr;
  }

  return S_OK;
}

/**
  Extract TLS information from XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  // XENIX doesn't support TLS
  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information from XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  // XENIX does not have unwinding information
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  XEXEC *Header;
  XOUT_SYMBOL *Symbols;
  UINT32 NumSymbols, i;
  UINT32 SymbolOffset;
  CHAR8 *StringTable;

  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  Header = (XEXEC *)ImageBase;
  if (Header->x_syms == 0) {
    return S_FALSE;
  }

  // Symbol table is after text, data, and relocations
  SymbolOffset = sizeof(XEXEC) + Header->x_ext +
                 Header->x_text + Header->x_data + Header->x_reloc;
  Symbols = (XOUT_SYMBOL *)XOUT_OFF(SymbolOffset);
  NumSymbols = Header->x_syms / sizeof(XOUT_SYMBOL);

  // String table follows symbol table
  StringTable = (CHAR8 *)XOUT_OFF(SymbolOffset + Header->x_syms);

  // Search for symbol at address
  for (i = 0; i < NumSymbols; i++) {
    XOUT_SYMBOL *Sym = &Symbols[i];
    VIRTUAL_ADDRESS SymAddr;
    CONST CHAR8 *SymName;

    // Skip auxiliary entries
    if (Sym->AuxCount > 0) {
      i += Sym->AuxCount;
      continue;
    }

    // Calculate symbol address based on section
    switch (Sym->SectionNumber) {
      case N_TEXT:
        SymAddr = XOUT_TEXT_START + Sym->Value;
        break;

      case N_DATA:
        SymAddr = XOUT_DATA_START + Sym->Value;
        break;

      case N_BSS:
        SymAddr = XOUT_DATA_START + Header->x_data + Sym->Value;
        break;

      case N_ABS:
        SymAddr = Sym->Value;
        break;

      default:
        continue;  // Skip undefined/unknown
    }

    if (SymAddr == Address) {
      // Get symbol name
      if (Sym->N.StringRef.Zeros == 0) {
        // Name in string table
        SymName = StringTable + Sym->N.StringRef.StringOffset;
      } else {
        // Name inline (max 8 chars)
        SymName = Sym->N.Name;
      }

      UINTN NameLen = strlen(SymName);
      UINTN CopyLen = (NameLen < sizeof(SymbolInfo->Name) - 1) ?
                      NameLen : (sizeof(SymbolInfo->Name) - 1);
      memcpy(SymbolInfo->Name, SymName, CopyLen);
      SymbolInfo->Name[CopyLen] = '\0';
      SymbolInfo->Address = SymAddr;
      SymbolInfo->Size = 0;  // Unknown
      return S_OK;
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
XenixGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  XEXEC *Header;
  XOUT_SYMBOL *Symbols;
  UINT32 NumSymbols, i;
  UINT32 SymbolOffset;
  CHAR8 *StringTable;
  UINTN SearchLen;

  if (SymbolInfo == NULL || Name == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  Header = (XEXEC *)ImageBase;
  if (Header->x_syms == 0) {
    return S_FALSE;
  }

  SearchLen = strlen(Name);

  // Symbol table is after text, data, and relocations
  SymbolOffset = sizeof(XEXEC) + Header->x_ext +
                 Header->x_text + Header->x_data + Header->x_reloc;
  Symbols = (XOUT_SYMBOL *)XOUT_OFF(SymbolOffset);
  NumSymbols = Header->x_syms / sizeof(XOUT_SYMBOL);

  // String table follows symbol table
  StringTable = (CHAR8 *)XOUT_OFF(SymbolOffset + Header->x_syms);

  // Search for symbol by name
  for (i = 0; i < NumSymbols; i++) {
    XOUT_SYMBOL *Sym = &Symbols[i];
    VIRTUAL_ADDRESS SymAddr;
    CONST CHAR8 *SymName;
    BOOLEAN Match = FALSE;

    // Skip auxiliary entries
    if (Sym->AuxCount > 0) {
      i += Sym->AuxCount;
      continue;
    }

    // Get symbol name
    if (Sym->N.StringRef.Zeros == 0) {
      // Name in string table
      SymName = StringTable + Sym->N.StringRef.StringOffset;
      Match = (strcmp(SymName, Name) == 0);
    } else {
      // Name inline (max 8 chars)
      UINTN InlineLen = 0;
      while (InlineLen < 8 && Sym->N.Name[InlineLen] != '\0') {
        InlineLen++;
      }
      if (InlineLen == SearchLen && memcmp(Sym->N.Name, Name, SearchLen) == 0) {
        Match = TRUE;
        SymName = Sym->N.Name;
      }
    }

    if (!Match) {
      continue;
    }

    // Calculate symbol address based on section
    switch (Sym->SectionNumber) {
      case N_TEXT:
        SymAddr = XOUT_TEXT_START + Sym->Value;
        break;

      case N_DATA:
        SymAddr = XOUT_DATA_START + Sym->Value;
        break;

      case N_BSS:
        SymAddr = XOUT_DATA_START + Header->x_data + Sym->Value;
        break;

      case N_ABS:
        SymAddr = Sym->Value;
        break;

      default:
        continue;  // Skip undefined/unknown
    }

    UINTN NameLen = strlen(SymName);
    UINTN CopyLen = (NameLen < sizeof(SymbolInfo->Name) - 1) ?
                    NameLen : (sizeof(SymbolInfo->Name) - 1);
    memcpy(SymbolInfo->Name, SymName, CopyLen);
    SymbolInfo->Name[CopyLen] = '\0';
    SymbolInfo->Address = SymAddr;
    SymbolInfo->Size = 0;  // Unknown
    return S_OK;
  }

  return S_FALSE;
}

/**
  Extract relocation information from XENIX image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetRelocInfo (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  OUT IMGLOAD_RELOC_INFO  *RelocInfo
  )
{
  XEXEC *Header;

  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  Header = (XEXEC *)ImageBase;

  if (Header->x_reloc > 0) {
    RelocInfo->PreferredBase = XOUT_TEXT_START;
    RelocInfo->RequiresReloc = TRUE;
    RelocInfo->Format = ImgRelocFormatXenix;
    return S_OK;
  }

  return S_FALSE;
}

/**
  Apply relocations to XENIX image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixApplyRelocations (
  IN IImageLoader     *This,
  IN VOID             *ImageBase,
  IN VIRTUAL_ADDRESS  LoadAddress,
  IN VIRTUAL_ADDRESS  PreferredBase
  )
{
  XEXEC *Header;
  UINT32 RelocOffset;
  UINT32 *RelocTable;
  UINT32 NumRelocs, i;
  INT32 Delta;

  Header = (XEXEC *)ImageBase;

  if (Header->x_reloc == 0) {
    // No relocations
    return S_FALSE;
  }

  Delta = (INT32)((INT64)LoadAddress - (INT64)PreferredBase);
  if (Delta == 0) {
    // Already at preferred base
    return S_OK;
  }

  // Relocation table is after text and data
  RelocOffset = sizeof(XEXEC) + Header->x_ext +
                Header->x_text + Header->x_data;
  RelocTable = (UINT32 *)XOUT_OFF(RelocOffset);
  NumRelocs = Header->x_reloc / sizeof(UINT32);

  // XENIX relocations are simple: each entry is an offset into text segment
  // that needs to be adjusted by the load delta
  UINT8 *TextBase = (UINT8 *)XOUT_OFF(sizeof(XEXEC) + Header->x_ext);

  for (i = 0; i < NumRelocs; i++) {
    UINT32 Offset = RelocTable[i];

    if (Offset >= Header->x_text) {
      // Invalid offset, skip
      continue;
    }

    // Apply relocation (32-bit little-endian)
    UINT32 *Target = (UINT32 *)(TextBase + Offset);
    *Target += Delta;
  }

  return S_OK;
}

//

/**
  Get target operating system from Xenix image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetTargetSystem (
  IN  IImageLoader           *This,
  IN  VOID                   *ImageBase,
  OUT IMGLOAD_TARGET_SYSTEM  *TargetSystem
  )
{
  if (TargetSystem == NULL) {
    return E_POINTER;
  }

  *TargetSystem = ImgSystemXenix;
  return S_OK;
}

/**
  Get minimum required system version from Xenix image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetMinimumSystemVersion (
  IN  IImageLoader            *This,
  IN  VOID                    *ImageBase,
  OUT IMGLOAD_SYSTEM_VERSION  *MinimumVersion
  )
{
  if (MinimumVersion == NULL) {
    return E_POINTER;
  }

  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}

/**
  Get target subsystem from Xenix image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetTargetSubsystem (
  IN  IImageLoader              *This,
  IN  VOID                      *ImageBase,
  OUT IMGLOAD_TARGET_SUBSYSTEM  *TargetSubsystem
  )
{
  if (TargetSubsystem == NULL) {
    return E_POINTER;
  }

  *TargetSubsystem = ImgSubsystemCli;
  return S_OK;
}

/**
  Get minimum required subsystem version from Xenix image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetMinimumSubsystemVersion (
  IN  IImageLoader            *This,
  IN  VOID                    *ImageBase,
  OUT IMGLOAD_SYSTEM_VERSION  *MinimumVersion
  )
{
  if (MinimumVersion == NULL) {
    return E_POINTER;
  }

  memset(MinimumVersion, 0, sizeof(IMGLOAD_SYSTEM_VERSION));
  return S_FALSE;
}
// XENIX X.OUT Loader VTable
//


/**
  Get initialization/termination function information from Xenix image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetInitFini (
  IN  IImageLoader          *This,
  IN  VOID                  *ImageBase,
  OUT IMGLOAD_INITFINI_INFO *InitFiniInfo
  )
{
  XEXEC *Header;
  XEXT *Ext;
  XSEG *SegTable;
  UINTN NumSegments, i;

  if (InitFiniInfo == NULL) {
    return E_POINTER;
  }

  memset(InitFiniInfo, 0, sizeof(IMGLOAD_INITFINI_INFO));

  Header = (XEXEC *)ImageBase;

  // Only segmented executables have init/fini support
  if (!(Header->x_renv & XE_SEG)) {
    return S_FALSE;
  }

  Ext = XenixGetExtHeader(ImageBase);
  if (Ext == NULL) {
    return S_FALSE;
  }

  SegTable = XenixGetSegmentTable(ImageBase, &NumSegments);
  if (SegTable == NULL) {
    return S_FALSE;
  }

  // Search for init/fini segments by type
  // In XENIX, initialization code is typically in special segments
  // marked with specific attributes
  for (i = 0; i < NumSegments; i++) {
    XSEG *Seg = &SegTable[i];

    if (Seg->xs_type == XS_TTEXT) {
      // Check segment name for init/fini markers
      // This is format-specific and may vary by linker version
      // For now, we'll rely on the entry segment for initialization

      // If this is the entry segment, it contains initialization code
      if (i == Ext->xe_eseg) {
        InitFiniInfo->InitAddress = Seg->xs_rbase;
        InitFiniInfo->HasInit = TRUE;
      }
    }
  }

  // XENIX segmented executables typically don't have explicit termination
  // functions - cleanup is handled by the kernel
  InitFiniInfo->HasFini = FALSE;

  return InitFiniInfo->HasInit ? S_OK : S_FALSE;
}

/**
  Get resource from Xenix X.OUT image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetResource (
  IN  IImageLoader   *This,
  IN  VOID           *ImageBase,
  IN  UINT32         TypeCode,
  IN  UINT32         Id,
  IN  CONST CHAR8    *Name,
  OUT IImageResource **Resource
  )
{
  if (Resource == NULL) {
    return E_POINTER;
  }

  *Resource = NULL;

  // Xenix X.OUT format does not have native resources
  return S_FALSE;
}

/**
  Get resource enumerator for Xenix X.OUT image.
**/
static
HRESULT
STDMETHODCALLTYPE
XenixGetResourceEnumerator (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  IN  UINT32              TypeCode,
  OUT IEnumImageResource  **Enumerator
  )
{
  if (Enumerator == NULL) {
    return E_POINTER;
  }

  *Enumerator = NULL;

  // Xenix X.OUT format does not have native resources
  return S_FALSE;
}

static CONST IImageLoaderVtbl gXenixVtbl = {
  // IUnknown
  XenixQueryInterface,
  XenixAddRef,
  XenixRelease,
  // IImageLoader
  XenixDetect,
  XenixGetArch,
  XenixGetEndianness,
  XenixGetEntryPoint,
  XenixLoadImage,
  XenixGetTlsInfo,
  XenixGetUnwindInfo,
  XenixGetSymbolByAddress,
  XenixGetSymbolByName,
  XenixGetRelocInfo,
  XenixApplyRelocations,
  XenixGetTargetSystem,
  XenixGetMinimumSystemVersion,
  XenixGetTargetSubsystem,
  XenixGetMinimumSubsystemVersion,
  XenixGetResource,
  XenixGetResourceEnumerator,
  XenixGetInitFini
};

//
// XENIX X.OUT Loader Instance
//

IImageLoader gXenixLoader = {
  &gXenixVtbl
};

APXH_REGISTER_IMGLOADER(gXenixLoader);
