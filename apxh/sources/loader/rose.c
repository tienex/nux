/** @file
  APXH OSF/ROSE Loader

  Provides OSF/ROSE (also known as "Mach-O" format on OSF/1) file parsing
  and loading for OSF/1 and Tru64 UNIX on DEC Alpha and MIPS.

  OSF/ROSE is the object file format used by OSF/1, informally called "Mach-O"
  because OSF/1 was based on the Mach kernel. This format is completely
  distinct from Apple's NeXT Mach-O format.

  Format Overview:
  - Fixed-length header (mo_header_t / raw_mo_header_t)
  - Variable-length load command map (array of offsets)
  - Load commands in any order (LDC_REGION, LDC_SYMBOLS, etc.)
  - Object file sections (regions) in any order
  - Big-endian (network byte order) storage on disk
  - Little-endian in-memory representation on Alpha

  Reference Headers (from Tru64 UNIX /usr/include):
  - mach_o_header.h     - Machine-independent header (mo_header_t)
  - mach_o_header_md.h  - Machine-dependent raw header (raw_mo_header_t)
  - mach_o_format.h     - Load command structures
  - mach_o_types.h      - Type definitions
  - mach_o_vals.h       - Constants and magic numbers

  Supported Architectures:
  - DEC Alpha (Tru64 UNIX, Digital UNIX, OSF/1 AXP)
  - MIPS (early OSF/1 ports, DEC workstations)
  - i386, M68000, NS32000 (historical OSF/1 ports)

  Based on OSF/1 Release 1.0 headers.
  Copyright (c) 1990 Open Software Foundation, Inc.
  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// OSF/ROSE Type Definitions (from mach_o_types.h)
//

typedef UINT16  mo_short_t;      ///< Half word (16 bits)
typedef UINT32  mo_long_t;       ///< Whole word (32 bits)
typedef UINT8   mo_byte_t;       ///< Byte
typedef VOID*   mo_ptr_t;        ///< Pointer type
typedef UINT64  mo_offset_t;     ///< File offset
typedef UINT64  mo_vm_addr_t;    ///< VM address
typedef UINT32  mo_cpu_type_t;   ///< CPU type
typedef UINT32  mo_cpu_subtype_t;  ///< CPU subtype
typedef UINT32  mo_vendor_type_t;  ///< Vendor type
typedef mo_long_t mo_lcid_t;     ///< Load command ID

//
// OSF/ROSE Magic Numbers (from mach_o_vals.h)
//

#define MOH_MAGIC_LSB      0xcefaefbe  ///< Little-endian magic
#define MOH_MAGIC_MSB      0xbeefface  ///< Big-endian magic

//
// OSF/ROSE Byte Order (from mach_o_vals.h)
//

#define BO_LSB             1  ///< Little-endian byte order
#define BO_MSB             2  ///< Big-endian byte order

//
// OSF/ROSE CPU Types (from mach_o_vals.h)
//

#define MO_CPU_TYPE_MIPS    1  ///< MIPS
#define MO_CPU_TYPE_NS32000 2  ///< NS32000
#define MO_CPU_TYPE_I386    3  ///< Intel i386
#define MO_CPU_TYPE_M68000  4  ///< Motorola 68000

//
// OSF/ROSE Header Flags (from mach_o_header.h)
//

#define MOH_RELOCATABLE_F  0x1   ///< Has loader relocation
#define MOH_LINKABLE_F     0x2   ///< Has linker relocation
#define MOH_EXECABLE_F     0x4   ///< Can be exec'd (has crt0)
#define MOH_EXECUTABLE_F   0x8   ///< Can be loaded for execution
#define MOH_UNRESOLVED_F   0x10  ///< Has unresolved imports

//
// OSF/ROSE Load Command Types (from mach_o_format.h)
//

#define LDC_UNDEFINED      0   ///< Undefined
#define LDC_CMD_MAP        1   ///< Load command map
#define LDC_INTERPRETER    2   ///< Interpreter path
#define LDC_STRINGS        3   ///< String table
#define LDC_REGION         4   ///< Memory region (section)
#define LDC_RELOC          5   ///< Relocation table
#define LDC_PACKAGE        6   ///< Package info
#define LDC_SYMBOLS        7   ///< Symbol table
#define LDC_ENTRY          8   ///< Entry point
#define LDC_FUNC_TABLE     9   ///< Function table
#define LDC_GEN_INFO       10  ///< General info

//
// OSF/ROSE Region Flags (from mach_o_format.h)
//

#define REG_ABS_ADDR_F     0x1  ///< Use absolute address
#define REG_REL_ADDR_F     0x2  ///< Use relative address

//
// OSF/ROSE Protection Flags (from mach_o_format.h)
//

#define MO_PROT_NONE       0x0  ///< No protection
#define MO_PROT_READ       0x1  ///< Read permission
#define MO_PROT_WRITE      0x2  ///< Write permission
#define MO_PROT_EXECUTE    0x4  ///< Execute permission

//
// OSF/ROSE Structures
//

ANX_PACK_PUSH(1)

/**
  OSF/ROSE main header (from mach_o_header.h).
  This is the decoded, machine-independent form.
**/
typedef struct {
  mo_long_t        moh_magic;            ///< Magic number
  mo_short_t       moh_major_version;    ///< Major version
  mo_short_t       moh_minor_version;    ///< Minor version
  mo_short_t       moh_header_version;   ///< Header version
  mo_short_t       moh_max_page_size;    ///< Max page size
  mo_short_t       moh_byte_order;       ///< Byte order
  mo_short_t       moh_data_rep_id;      ///< Data representation
  mo_cpu_type_t    moh_cpu_type;         ///< CPU type
  mo_cpu_subtype_t moh_cpu_subtype;      ///< CPU subtype
  mo_vendor_type_t moh_vendor_type;      ///< Vendor type
  mo_long_t        moh_flags;            ///< Flags
  mo_offset_t      moh_load_map_cmd_off; ///< Load map command offset
  mo_offset_t      moh_first_cmd_off;    ///< First command offset
  mo_long_t        moh_sizeofcmds;       ///< Size of commands
  mo_long_t        moh_n_load_cmds;      ///< Number of load commands
  mo_long_t        moh_reserved[2];      ///< Reserved
} mo_header_t;

/**
  OSF/ROSE raw header (from mach_o_header_md.h).
  This is the on-disk form in big-endian (network) byte order.
**/
typedef struct {
  UINT32  rmoh_magic;              ///< Magic number (0xBEEFFACE)
  UINT16  rmoh_major_version;      ///< Major version
  UINT16  rmoh_minor_version;      ///< Minor version
  UINT16  rmoh_header_version;     ///< Header version
  UINT16  rmoh_max_page_size;      ///< Max page size
  UINT16  rmoh_byte_order;         ///< Byte order
  UINT16  rmoh_data_rep_id;        ///< Data representation
  UINT32  rmoh_cpu_type;           ///< CPU type
  UINT32  rmoh_cpu_subtype;        ///< CPU subtype
  UINT32  rmoh_vendor_type;        ///< Vendor type
  UINT32  rmoh_flags;              ///< Flags
  UINT32  rmoh_load_map_cmd_off;   ///< Load map command offset
  UINT32  rmoh_first_cmd_off;      ///< First command offset
  UINT32  rmoh_sizeofcmds;         ///< Size of commands
  UINT32  rmoh_n_load_cmds;        ///< Number of load commands
  UINT32  rmoh_reserved[2];        ///< Reserved
} raw_mo_header_t;

/**
  Load command header (from mach_o_format.h).
**/
typedef struct {
  mo_long_t    ldci_cmd_type;      ///< Command type (LDC_*)
  mo_long_t    ldci_cmd_size;      ///< Command size
  mo_offset_t  ldci_section_off;   ///< Section offset
  mo_long_t    ldci_section_len;   ///< Section length
} ldc_header_t;

/**
  Entry command (LDC_ENTRY) - from mach_o_format.h.
**/
typedef struct {
  ldc_header_t  ldc_header;
  mo_short_t    entc_flags;          ///< Flags
  mo_short_t    entc_short_reserved; ///< Reserved
  mo_vm_addr_t  entc_absaddr;        ///< Absolute address
  UINT32        entc_entry_lcid;     ///< Entry point load cmd ID (simplified)
  UINT32        entc_entry_off;      ///< Entry point offset (simplified)
} entry_command_t;

#define ENT_VALID_ABSADDR_F  0x1  ///< Absolute address is valid

ANX_PACK_POP()

//
// Helper Macros
//

#define ROSE_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// IImageLoader Implementation for OSF/ROSE
//

/**
  Detect if image is OSF/ROSE format.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  raw_mo_header_t *RawHdr;
  UINT32 Magic;

  if (ImageSize < sizeof(raw_mo_header_t)) {
    return S_FALSE;
  }

  RawHdr = (raw_mo_header_t *)ImageBase;
  Magic = ANX_BSWAP32(RawHdr->rmoh_magic);  // Stored big-endian

  if (Magic == MOH_MAGIC_MSB || Magic == MOH_MAGIC_LSB) {
    return S_OK;
  }

  return S_FALSE;
}

/**
  Get architecture from OSF/ROSE image.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  raw_mo_header_t *RawHdr;
  UINT32 CpuType;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  RawHdr = (raw_mo_header_t *)ImageBase;
  CpuType = ANX_BSWAP32(RawHdr->rmoh_cpu_type);

  switch (CpuType) {
    case MO_CPU_TYPE_MIPS:
      *Architecture = ARCH_MIPS64;
      break;
    case MO_CPU_TYPE_I386:
      *Architecture = ARCH_386;
      break;
    case MO_CPU_TYPE_M68000:
      *Architecture = ARCH_M68K;
      break;
    default:
      // Default to Alpha for OSF/1 and Tru64
      *Architecture = ARCH_ALPHA;
      break;
  }

  return S_OK;
}

/**
  Get endianness from OSF/ROSE image.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  if (Endianness == NULL) {
    return E_POINTER;
  }

  // OSF/ROSE is stored big-endian but runs little-endian on Alpha
  *Endianness = ImgEndianLittle;
  return S_OK;
}

/**
  Get entry point from OSF/ROSE image.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  raw_mo_header_t *RawHdr;
  entry_command_t *EntryCmd;
  UINT32 FirstCmdOff;
  UINT32 NumCmds, i;
  ldc_header_t *LdcHdr;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  RawHdr = (raw_mo_header_t *)ImageBase;
  FirstCmdOff = ANX_BSWAP32(RawHdr->rmoh_first_cmd_off);
  NumCmds = ANX_BSWAP32(RawHdr->rmoh_n_load_cmds);

  // Search for LDC_ENTRY command
  for (i = 0; i < NumCmds; i++) {
    LdcHdr = (ldc_header_t *)ROSE_OFF(FirstCmdOff);

    if (ANX_BSWAP32(LdcHdr->ldci_cmd_type) == LDC_ENTRY) {
      EntryCmd = (entry_command_t *)LdcHdr;
      *EntryPoint = ANX_BSWAP64(EntryCmd->entc_absaddr);
      return S_OK;
    }

    FirstCmdOff += ANX_BSWAP32(LdcHdr->ldci_cmd_size);
  }

  *EntryPoint = 0;
  return IMGLOAD_E_INVALID_HEADER;
}

/**
  Load OSF/ROSE image.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseLoadImage (
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  raw_mo_header_t *RawHdr;
  HRESULT Status;
  UINT32 Flags;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;
  RawHdr = (raw_mo_header_t *)ImageBase;

  Flags = ANX_BSWAP32(RawHdr->rmoh_flags);

  info("Loading OSF/ROSE (Mach-O) executable...");
  info("  CPU Type: %u, Flags: 0x%08x",
       ANX_BSWAP32(RawHdr->rmoh_cpu_type), Flags);

  // Populate context
  Status = RoseGetArch(NULL, ImageBase, &Context->Architecture);
  if (FAILED(Status)) {
    return Status;
  }

  Status = RoseGetEndianness(NULL, ImageBase, &Context->Endianness);
  if (FAILED(Status)) {
    return Status;
  }

  Status = RoseGetEntryPoint(NULL, ImageBase, &Context->EntryPoint);
  if (FAILED(Status)) {
    return Status;
  }

  // TODO: Parse and load regions (LDC_REGION commands)
  // TODO: Apply relocations (LDC_RELOC commands)
  // TODO: Load symbols (LDC_SYMBOLS commands)

  info("OSF/ROSE: Entry point at 0x%016llx", Context->EntryPoint);

  return S_OK;
}

/**
  Extract TLS information.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));
  return S_FALSE;
}

/**
  Extract unwinding information.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));
  return S_FALSE;
}

/**
  Look up symbol by address.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  // TODO: Parse LDC_SYMBOLS commands
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  // TODO: Parse LDC_SYMBOLS commands
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Extract relocation information.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetRelocInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_RELOC_INFO   *RelocInfo
  )
{
  raw_mo_header_t *RawHdr;
  UINT32 Flags;

  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  RawHdr = (raw_mo_header_t *)ImageBase;
  Flags = ANX_BSWAP32(RawHdr->rmoh_flags);

  RelocInfo->Format = 11;  // OSF/ROSE format
  RelocInfo->RequiresReloc = !!(Flags & MOH_RELOCATABLE_F);

  return RelocInfo->RequiresReloc ? S_OK : S_FALSE;
}

/**
  Apply relocations.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseApplyRelocations (
  IN VOID              *ImageBase,
  IN VIRTUAL_ADDRESS   LoadAddress,
  IN VIRTUAL_ADDRESS   PreferredBase
  )
{
  // TODO: Parse LDC_RELOC commands and apply relocations
  return E_NOTIMPL;
}

/**
  IUnknown::QueryInterface.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseQueryInterface (
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
  IUnknown::AddRef.
**/
static
UINT32
STDMETHODCALLTYPE
RoseAddRef (
  IN IImageLoader  *This
  )
{
  return 1;
}

/**
  IUnknown::Release.
**/
static
UINT32
STDMETHODCALLTYPE
RoseRelease (
  IN IImageLoader  *This
  )
{
  return 1;
}

//
// OSF/ROSE Loader VTable
//

static CONST IImageLoaderVtbl gRoseVtbl = {
  RoseQueryInterface,
  RoseAddRef,
  RoseRelease,
  RoseDetect,
  RoseGetArch,
  RoseGetEndianness,
  RoseGetEntryPoint,
  RoseLoadImage,
  RoseGetTlsInfo,
  RoseGetUnwindInfo,
  RoseGetSymbolByAddress,
  RoseGetSymbolByName,
  RoseGetRelocInfo,
  RoseApplyRelocations
};

//
// OSF/ROSE Loader Instance
//

IImageLoader gRoseLoader = {
  &gRoseVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gRoseLoader);
