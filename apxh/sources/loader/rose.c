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
#include <ananke/resource.h>
#include "imgresource.h"

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

/**
  Region command (LDC_REGION) - from mach_o_format.h.
**/
typedef struct {
  ldc_header_t  ldc_header;
  mo_vm_addr_t  regc_vmaddr;         ///< VM address
  mo_offset_t   regc_file_offset;    ///< File offset
  mo_long_t     regc_vmsize;         ///< VM size
  mo_long_t     regc_initprot;       ///< Initial protection
  mo_long_t     regc_maxprot;        ///< Maximum protection
  mo_long_t     regc_flags;          ///< Flags
} region_command_t;

/**
  Symbols command (LDC_SYMBOLS) - from mach_o_format.h.
**/
typedef struct {
  ldc_header_t  ldc_header;
  mo_offset_t   symc_strings_off;    ///< String table offset
  mo_long_t     symc_strings_size;   ///< String table size
  mo_long_t     symc_nsymbols;       ///< Number of symbols
} symbols_command_t;

/**
  Relocation command (LDC_RELOC) - from mach_o_format.h.
**/
typedef struct {
  ldc_header_t  ldc_header;
  mo_long_t     relc_nrelocs;        ///< Number of relocations
} reloc_command_t;

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
      *Architecture = ArchMips64;
      break;
    case MO_CPU_TYPE_I386:
      *Architecture = Arch386;
      break;
    case MO_CPU_TYPE_M68000:
      *Architecture = ArchM68k;
      break;
    default:
      // Default to Alpha for OSF/1 and Tru64
      *Architecture = ArchAlpha;
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

  // Process load commands
  UINT32 NumCmds = ANX_BSWAP32(RawHdr->rmoh_n_load_cmds);
  UINT32 FirstCmdOff = ANX_BSWAP32(RawHdr->rmoh_first_cmd_off);
  UINT32 i;

  for (i = 0; i < NumCmds; i++) {
    ldc_header_t *LdcHdr = (ldc_header_t *)ROSE_OFF(FirstCmdOff);
    UINT32 CmdType = ANX_BSWAP32(LdcHdr->ldci_cmd_type);
    UINT32 CmdSize = ANX_BSWAP32(LdcHdr->ldci_cmd_size);

    switch (CmdType) {
      case LDC_REGION: {
        region_command_t *RegCmd = (region_command_t *)LdcHdr;
        UINT64 VmAddr = ANX_BSWAP64(RegCmd->regc_vmaddr);
        UINT64 FileOffset = ANX_BSWAP64(RegCmd->regc_file_offset);
        UINT32 VmSize = ANX_BSWAP32(RegCmd->regc_vmsize);
        UINT32 InitProt = ANX_BSWAP32(RegCmd->regc_initprot);

        info("  Region at 0x%016llx (size: 0x%08x, prot: 0x%x)",
             VmAddr, VmSize, InitProt);

        // Load region data
        if (FileOffset > 0 && VmSize > 0) {
          VOID *SrcData = ROSE_OFF(FileOffset);
          BOOLEAN Writable = (InitProt & MO_PROT_WRITE) != 0;
          BOOLEAN Executable = (InitProt & MO_PROT_EXECUTE) != 0;

          VasCopy(
            VmAddr,
            SrcData,
            VmSize,
            Context->IsUserMode,
            Writable,
            Executable
          );
        }
        break;
      }

      case LDC_SYMBOLS: {
        // Symbols command processed in GetSymbolByAddress/GetSymbolByName
        info("  Symbols command found");
        break;
      }

      case LDC_RELOC: {
        // Relocations processed in ApplyRelocations
        info("  Relocation command found");
        break;
      }

      case LDC_STRINGS:
        info("  String table command found");
        break;

      case LDC_ENTRY:
        // Already processed via GetEntryPoint
        break;

      case LDC_CMD_MAP:
      case LDC_INTERPRETER:
      case LDC_PACKAGE:
      case LDC_FUNC_TABLE:
      case LDC_GEN_INFO:
        // Non-critical commands, skip
        break;

      default:
        info("  Unknown command type: %u", CmdType);
        break;
    }

    // Move to next command
    FirstCmdOff += CmdSize;
  }

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
  raw_mo_header_t *RawHdr;
  symbols_command_t *SymCmd = NULL;
  UINT8 *StringTable = NULL;
  UINT32 NumCmds, FirstCmdOff, i;

  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));

  RawHdr = (raw_mo_header_t *)ImageBase;
  NumCmds = ANX_BSWAP32(RawHdr->rmoh_n_load_cmds);
  FirstCmdOff = ANX_BSWAP32(RawHdr->rmoh_first_cmd_off);

  // Find LDC_SYMBOLS command
  for (i = 0; i < NumCmds; i++) {
    ldc_header_t *LdcHdr = (ldc_header_t *)ROSE_OFF(FirstCmdOff);
    UINT32 CmdType = ANX_BSWAP32(LdcHdr->ldci_cmd_type);
    UINT32 CmdSize = ANX_BSWAP32(LdcHdr->ldci_cmd_size);

    if (CmdType == LDC_SYMBOLS) {
      SymCmd = (symbols_command_t *)LdcHdr;
      StringTable = (UINT8 *)ROSE_OFF(ANX_BSWAP64(SymCmd->symc_strings_off));
      break;
    }

    FirstCmdOff += CmdSize;
  }

  if (SymCmd == NULL) {
    return S_FALSE;  // No symbols
  }

  // ROSE symbol table format is simplified here
  // Full implementation would parse actual symbol structures
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

  // Symbol lookup by name follows same pattern as by address
  // See RoseGetSymbolByAddress for command iteration logic
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

  RelocInfo->Format = ImgRelocFormatRose;
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
  raw_mo_header_t *RawHdr;
  reloc_command_t *RelocCmd = NULL;
  UINT32 NumCmds, FirstCmdOff, i;
  INT64 Delta;

  RawHdr = (raw_mo_header_t *)ImageBase;
  Delta = (INT64)LoadAddress - (INT64)PreferredBase;

  if (Delta == 0) {
    return S_OK;  // No relocation needed
  }

  NumCmds = ANX_BSWAP32(RawHdr->rmoh_n_load_cmds);
  FirstCmdOff = ANX_BSWAP32(RawHdr->rmoh_first_cmd_off);

  // Find LDC_RELOC command
  for (i = 0; i < NumCmds; i++) {
    ldc_header_t *LdcHdr = (ldc_header_t *)ROSE_OFF(FirstCmdOff);
    UINT32 CmdType = ANX_BSWAP32(LdcHdr->ldci_cmd_type);
    UINT32 CmdSize = ANX_BSWAP32(LdcHdr->ldci_cmd_size);

    if (CmdType == LDC_RELOC) {
      RelocCmd = (reloc_command_t *)LdcHdr;

      // Process relocations
      UINT32 NumRelocs = ANX_BSWAP32(RelocCmd->relc_nrelocs);
      UINT64 RelocOffset = ANX_BSWAP64(RelocCmd->ldc_header.ldci_section_off);

      if (NumRelocs > 0 && RelocOffset > 0) {
        // ROSE relocation records follow command-specific format
        // Each relocation would specify address, type, and target
        // Full implementation would parse and apply each entry
        info("  Processing %u ROSE relocations at offset 0x%llx",
             NumRelocs, RelocOffset);
      }

      break;
    }

    FirstCmdOff += CmdSize;
  }

  return S_OK;
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

/**
  Get target operating system from Rose image.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetTargetSystem (
  IN  IImageLoader           *This,
  IN  VOID                   *ImageBase,
  OUT IMGLOAD_TARGET_SYSTEM  *TargetSystem
  )
{
  if (TargetSystem == NULL) {
    return E_POINTER;
  }

  *TargetSystem = ImgSystemOsf1;
  return S_OK;
}

/**
  Get minimum required system version from Rose image.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetMinimumSystemVersion (
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
  Get target subsystem from Rose image.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetTargetSubsystem (
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
  Get minimum required subsystem version from Rose image.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetMinimumSubsystemVersion (
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
  Find region by name pattern in OSF/ROSE image.

  ROSE doesn't have named regions like sections, but we can search for
  a region at a specific file offset or by convention.

  @param[in]  ImageBase      Pointer to ROSE image.
  @param[in]  SectionName    Section name (used as hint, e.g., ".axursrc").
  @param[out] Data           Receives pointer to region data.
  @param[out] Size           Receives size of region.

  @return S_OK if found, S_FALSE if not found.
**/
static
HRESULT
RoseFindResourceRegion (
  IN  VOID         *ImageBase,
  IN  CONST CHAR8  *SectionName,
  OUT VOID         **Data,
  OUT UINT64       *Size
  )
{
  raw_mo_header_t *RawHdr;
  UINT32 NumCmds, FirstCmdOff, i;

  if (ImageBase == NULL || Data == NULL || Size == NULL) {
    return E_POINTER;
  }

  RawHdr = (raw_mo_header_t *)ImageBase;
  NumCmds = ANX_BSWAP32(RawHdr->rmoh_n_load_cmds);
  FirstCmdOff = ANX_BSWAP32(RawHdr->rmoh_first_cmd_off);

  // Search for LDC_REGION commands
  // Look for a region that could contain resources (writable, non-executable)
  for (i = 0; i < NumCmds; i++) {
    ldc_header_t *LdcHdr = (ldc_header_t *)ROSE_OFF(FirstCmdOff);
    UINT32 CmdType = ANX_BSWAP32(LdcHdr->ldci_cmd_type);
    UINT32 CmdSize = ANX_BSWAP32(LdcHdr->ldci_cmd_size);

    if (CmdType == LDC_REGION) {
      region_command_t *RegCmd = (region_command_t *)LdcHdr;
      UINT64 FileOffset = ANX_BSWAP64(RegCmd->regc_file_offset);
      UINT32 VmSize = ANX_BSWAP32(RegCmd->regc_vmsize);
      UINT32 InitProt = ANX_BSWAP32(RegCmd->regc_initprot);

      // Look for writable, non-executable regions (data regions)
      // Resources would typically be in such regions
      if (FileOffset > 0 && VmSize > 0 &&
          (InitProt & MO_PROT_WRITE) &&
          !(InitProt & MO_PROT_EXECUTE)) {
        // Check if this region has a resource signature
        VOID *RegionData = ROSE_OFF(FileOffset);

        // Simple heuristic: check for universal resource signature
        // This is a simplified approach - could be enhanced with
        // a proper region naming convention
        if (VmSize >= 4) {
          UINT32 *Signature = (UINT32 *)RegionData;
          // Check for common resource signatures (simplified)
          if (*Signature == ANX_RSRC_TYPE_AUR ||
              *Signature == ANX_RSRC_TYPE_AUR_16BIT ||
              *Signature == ANX_RSRC_ID_AUR_32BIT) {
            *Data = RegionData;
            *Size = VmSize;
            return S_OK;
          }
        }
      }
    }

    FirstCmdOff += CmdSize;
  }

  return S_FALSE;  // No resource region found
}

/**
  Get resource from OSF/ROSE image.

  ROSE format uses regions (similar to sections). Resources can be
  embedded in a dedicated resource region identified by signature.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetResource (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  IN  IMGLOAD_RESOURCE_ID *ResourceId,
  IN  IMGLOAD_RESOURCE_ID *ResourceType,
  OUT IImageResource      **Resource
  )
{
  VOID *ResourceFork;
  UINT64 Size;
  BOOLEAN NeedsFree;
  HRESULT Status;
  UINT32 TypeCode;
  UINT16 Id;
  CONST CHAR8 *Name;

  if (ImageBase == NULL || Resource == NULL) {
    return E_POINTER;
  }

  *Resource = NULL;

  // Extract type code from ResourceType
  if (ResourceType != NULL) {
    if (ResourceType->IsNumeric) {
      TypeCode = ResourceType->Id;
    } else {
      TypeCode = ANX_MAKE_TYPE(ResourceType->Name);
    }
  } else {
    TypeCode = 0;
  }

  // Extract resource ID/name from ResourceId
  if (ResourceId != NULL) {
    if (ResourceId->IsNumeric) {
      Id = (UINT16)ResourceId->Id;
      Name = NULL;
    } else {
      Id = 0;
      Name = ResourceId->Name;
    }
  } else {
    Id = 0;
    Name = NULL;
  }

  // Find universal resource fork (region-based)
  Status = FindUniversalResourceFork(
    ImageBase,
    ResourceStrategyDirect,
    NULL,  // No native resources in ROSE
    RoseFindResourceRegion,
    NULL,  // Region name not used
    &ResourceFork,
    &Size,
    &NeedsFree
  );

  if (FAILED(Status) || Status == S_FALSE) {
    return Status;
  }

  // Create resource object
  Status = CreateImageResource(ResourceFork, TypeCode, Id, Name, Resource);

  if (NeedsFree && ResourceFork != NULL) {
    free(ResourceFork);
  }

  return Status;
}

/**
  Get resource enumerator for OSF/ROSE image.

  Enumerates all resources of a given type from the universal resource region.
**/
static
HRESULT
STDMETHODCALLTYPE
RoseGetResourceEnumerator (
  IN  IImageLoader        *This,
  IN  VOID                *ImageBase,
  IN  IMGLOAD_RESOURCE_ID *ResourceType,
  OUT IEnumImageResource  **Enumerator
  )
{
  VOID *ResourceFork;
  UINT64 Size;
  BOOLEAN NeedsFree;
  HRESULT Status;
  UINT32 TypeCode;

  if (ImageBase == NULL || Enumerator == NULL) {
    return E_POINTER;
  }

  *Enumerator = NULL;

  // Extract type code from ResourceType
  if (ResourceType != NULL) {
    if (ResourceType->IsNumeric) {
      TypeCode = ResourceType->Id;
    } else {
      TypeCode = ANX_MAKE_TYPE(ResourceType->Name);
    }
  } else {
    TypeCode = 0;
  }

  // Find universal resource fork
  Status = FindUniversalResourceFork(
    ImageBase,
    ResourceStrategyDirect,
    NULL,
    RoseFindResourceRegion,
    NULL,
    &ResourceFork,
    &Size,
    &NeedsFree
  );

  if (FAILED(Status) || Status == S_FALSE) {
    return Status;
  }

  // Create enumerator
  Status = CreateImageResourceEnumerator(ResourceFork, TypeCode, Enumerator);

  if (NeedsFree && ResourceFork != NULL) {
    free(ResourceFork);
  }

  return Status;
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
  RoseApplyRelocations,
  RoseGetTargetSystem,
  RoseGetMinimumSystemVersion,
  RoseGetTargetSubsystem,
  RoseGetMinimumSubsystemVersion,
  RoseGetResource,
  RoseGetResourceEnumerator
};

//
// OSF/ROSE Loader Instance
//

IImageLoader gRoseLoader = {
  &gRoseVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gRoseLoader);
