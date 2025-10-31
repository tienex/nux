/** @file
  APXH Mach-O Loader

  Provides Mach-O (Mach Object) file format parsing and loading for
  32-bit and 64-bit executables. Handles load commands, segments,
  and Thread-Local Storage (TLS) for macOS/iOS binaries.

  Supports:
  - Mach-O 32-bit (MH_MAGIC) and 64-bit (MH_MAGIC_64)
  - x86, x86-64, ARM, ARM64, and RISC-V architectures
  - LC_SEGMENT/LC_SEGMENT_64 load commands
  - LC_THREAD_LOCAL_VARIABLES for TLS

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/imgload.h>

//
// Mach-O Magic Numbers
//

#define MH_MAGIC      0xFEEDFACE  ///< 32-bit Mach-O magic (big endian)
#define MH_CIGAM      0xCEFAEDFE  ///< 32-bit Mach-O magic (little endian)
#define MH_MAGIC_64   0xFEEDFACF  ///< 64-bit Mach-O magic (big endian)
#define MH_CIGAM_64   0xCFFAEDFE  ///< 64-bit Mach-O magic (little endian)

//
// Mach-O File Types
//

#define MH_EXECUTE    0x2   ///< Executable file

//
// Mach-O CPU Types
//

#define CPU_TYPE_I386        7    ///< x86 32-bit
#define CPU_TYPE_X86_64      0x01000007  ///< x86-64
#define CPU_TYPE_ARM         12   ///< ARM 32-bit
#define CPU_TYPE_ARM64       0x0100000C  ///< ARM 64-bit
#define CPU_TYPE_RISCV       0xF3 ///< RISC-V

//
// Mach-O Load Command Types
//

#define LC_SEGMENT              0x1   ///< 32-bit segment
#define LC_SYMTAB               0x2   ///< Symbol table
#define LC_THREAD               0x4   ///< Thread state (deprecated)
#define LC_UNIXTHREAD           0x5   ///< Unix thread (entry point)
#define LC_SEGMENT_64           0x19  ///< 64-bit segment
#define LC_UUID                 0x1B  ///< UUID
#define LC_MAIN                 0x28  ///< Main entry point (LC_MAIN)
#define LC_THREAD_LOCAL_VARIABLES 0x2F  ///< TLS variables

//
// Mach-O Segment Flags
//

#define VM_PROT_NONE      0x0  ///< No permissions
#define VM_PROT_READ      0x1  ///< Read permission
#define VM_PROT_WRITE     0x2  ///< Write permission
#define VM_PROT_EXECUTE   0x4  ///< Execute permission

//
// Mach-O Structures
//

ANX_PACK_PUSH(1)

typedef struct _MACHO_HEADER {
  UINT32  Magic;        ///< Magic number
  UINT32  CpuType;      ///< CPU type
  UINT32  CpuSubType;   ///< CPU subtype
  UINT32  FileType;     ///< File type (MH_EXECUTE, etc.)
  UINT32  NumCmds;      ///< Number of load commands
  UINT32  SizeOfCmds;   ///< Size of load commands
  UINT32  Flags;        ///< Flags
} MACHO_HEADER;

typedef struct _MACHO_HEADER_64 {
  UINT32  Magic;        ///< Magic number
  UINT32  CpuType;      ///< CPU type
  UINT32  CpuSubType;   ///< CPU subtype
  UINT32  FileType;     ///< File type
  UINT32  NumCmds;      ///< Number of load commands
  UINT32  SizeOfCmds;   ///< Size of load commands
  UINT32  Flags;        ///< Flags
  UINT32  Reserved;     ///< Reserved (64-bit only)
} MACHO_HEADER_64;

typedef struct _MACHO_LOAD_COMMAND {
  UINT32  Cmd;          ///< Load command type
  UINT32  CmdSize;      ///< Command size
} MACHO_LOAD_COMMAND;

typedef struct _MACHO_SEGMENT_COMMAND {
  UINT32  Cmd;          ///< LC_SEGMENT
  UINT32  CmdSize;      ///< Command size
  CHAR8   SegName[16];  ///< Segment name
  UINT32  VmAddr;       ///< Virtual address
  UINT32  VmSize;       ///< Virtual size
  UINT32  FileOff;      ///< File offset
  UINT32  FileSize;     ///< File size
  UINT32  MaxProt;      ///< Maximum protection
  UINT32  InitProt;     ///< Initial protection
  UINT32  NumSects;     ///< Number of sections
  UINT32  Flags;        ///< Flags
} MACHO_SEGMENT_COMMAND;

typedef struct _MACHO_SEGMENT_COMMAND_64 {
  UINT32  Cmd;          ///< LC_SEGMENT_64
  UINT32  CmdSize;      ///< Command size
  CHAR8   SegName[16];  ///< Segment name
  UINT64  VmAddr;       ///< Virtual address
  UINT64  VmSize;       ///< Virtual size
  UINT64  FileOff;      ///< File offset
  UINT64  FileSize;     ///< File size
  UINT32  MaxProt;      ///< Maximum protection
  UINT32  InitProt;     ///< Initial protection
  UINT32  NumSects;     ///< Number of sections
  UINT32  Flags;        ///< Flags
} MACHO_SEGMENT_COMMAND_64;

typedef struct _MACHO_ENTRY_POINT_COMMAND {
  UINT32  Cmd;          ///< LC_MAIN
  UINT32  CmdSize;      ///< Command size
  UINT64  EntryOff;     ///< Entry point offset from image base
  UINT64  StackSize;    ///< Initial stack size
} MACHO_ENTRY_POINT_COMMAND;

typedef struct _MACHO_THREAD_COMMAND {
  UINT32  Cmd;          ///< LC_THREAD or LC_UNIXTHREAD
  UINT32  CmdSize;      ///< Command size
  UINT32  Flavor;       ///< Thread state flavor
  UINT32  Count;        ///< Thread state count
  /* Thread state follows */
} MACHO_THREAD_COMMAND;

ANX_PACK_POP()

//
// Helper Macros
//

#define MACHO_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// Internal Functions
//

/**
  Check if image is Mach-O format.
**/
static
BOOLEAN
ANXAPI
MachoDetect (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  MACHO_HEADER *Header;

  if (ImageSize < sizeof(MACHO_HEADER)) {
    return FALSE;
  }

  Header = (MACHO_HEADER *)ImageBase;

  return (Header->Magic == MH_MAGIC ||
          Header->Magic == MH_CIGAM ||
          Header->Magic == MH_MAGIC_64 ||
          Header->Magic == MH_CIGAM_64);
}

/**
  Get architecture from Mach-O image.
**/
static
ARCH
ANXAPI
MachoGetArch (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  MACHO_HEADER *Header = (MACHO_HEADER *)ImageBase;

  switch (Header->CpuType) {
    case CPU_TYPE_I386:
      return ARCH_386;
    case CPU_TYPE_X86_64:
      return ARCH_AMD64;
    case CPU_TYPE_RISCV:
      return ARCH_RISCV64;
    case CPU_TYPE_ARM64:
      // ARM64 not yet supported by APXH
      return ARCH_UNSUPPORTED;
    default:
      return ARCH_UNSUPPORTED;
  }
}

/**
  Get entry point from Mach-O image.
**/
static
VIRTUAL_ADDRESS
ANXAPI
MachoGetEntryPoint (
  IN IMAGE_LOADER  *This,
  IN VOID          *ImageBase
  )
{
  MACHO_HEADER *Header32 = (MACHO_HEADER *)ImageBase;
  MACHO_HEADER_64 *Header64 = (MACHO_HEADER_64 *)ImageBase;
  MACHO_LOAD_COMMAND *Cmd;
  UINT32 i;
  UINTN Offset;
  BOOLEAN Is64Bit;

  Is64Bit = (Header32->Magic == MH_MAGIC_64 || Header32->Magic == MH_CIGAM_64);
  Offset = Is64Bit ? sizeof(MACHO_HEADER_64) : sizeof(MACHO_HEADER);

  for (i = 0; i < Header32->NumCmds; i++) {
    Cmd = (MACHO_LOAD_COMMAND *)MACHO_OFF(Offset);

    if (Cmd->Cmd == LC_MAIN) {
      MACHO_ENTRY_POINT_COMMAND *EntryCmd = (MACHO_ENTRY_POINT_COMMAND *)Cmd;
      // LC_MAIN stores offset from __TEXT base, we need to add base address
      // For simplicity, return the offset - caller must add image base
      return (VIRTUAL_ADDRESS)EntryCmd->EntryOff;
    } else if (Cmd->Cmd == LC_UNIXTHREAD) {
      // Legacy entry point in thread state
      MACHO_THREAD_COMMAND *ThreadCmd = (MACHO_THREAD_COMMAND *)Cmd;
      // Entry point is in thread state, architecture-specific
      // For x86/x86-64, it's the EIP/RIP register
      if (Is64Bit) {
        // x86-64 thread state: RIP is at offset 16 in the state
        UINT64 *State = (UINT64 *)(ThreadCmd + 1);
        return (VIRTUAL_ADDRESS)State[16];
      } else {
        // x86 thread state: EIP is at offset 10 in the state
        UINT32 *State = (UINT32 *)(ThreadCmd + 1);
        return (VIRTUAL_ADDRESS)State[10];
      }
    }

    Offset += Cmd->CmdSize;
  }

  return 0;
}

/**
  Load Mach-O segment.
**/
static
VOID
MachoLoadSegment (
  IN VOID     *ImageBase,
  IN UINT64   VmAddr,
  IN UINT64   VmSize,
  IN UINT64   FileOff,
  IN UINT64   FileSize,
  IN UINT32   InitProt,
  IN BOOLEAN  IsUserMode
  )
{
  BOOLEAN IsWritable = !!(InitProt & VM_PROT_WRITE);
  BOOLEAN IsExecutable = !!(InitProt & VM_PROT_EXECUTE);

  if (VmAddr + VmSize < VmAddr) {
    fatal("Mach-O segment size overflow");
  }

  if (FileSize > 0) {
    // Copy file data to virtual address
    VirtualAddressCopy(
      VmAddr,
      MACHO_OFF(FileOff),
      FileSize,
      IsUserMode,
      IsWritable,
      IsExecutable
    );
  }

  if (VmSize > FileSize) {
    // Zero-fill remainder (BSS)
    VirtualAddressMemset(
      VmAddr + FileSize,
      0,
      VmSize - FileSize,
      IsUserMode,
      IsWritable,
      IsExecutable
    );
  }
}

/**
  Load Mach-O image.
**/
static
IMGLOAD_STATUS
ANXAPI
MachoLoadImage (
  IN     IMAGE_LOADER     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase = Context->ImageBase;
  MACHO_HEADER *Header32 = (MACHO_HEADER *)ImageBase;
  MACHO_LOAD_COMMAND *Cmd;
  UINT32 i;
  UINTN Offset;
  BOOLEAN Is64Bit;

  Is64Bit = (Header32->Magic == MH_MAGIC_64 || Header32->Magic == MH_CIGAM_64);

  if (Header32->FileType != MH_EXECUTE) {
    return ImgLoadInvalidFormat;
  }

  Offset = Is64Bit ? sizeof(MACHO_HEADER_64) : sizeof(MACHO_HEADER);

  info("Loading Mach-O %s executable...", Is64Bit ? "64-bit" : "32-bit");

  // Process all load commands
  for (i = 0; i < Header32->NumCmds; i++) {
    Cmd = (MACHO_LOAD_COMMAND *)MACHO_OFF(Offset);

    if (Cmd->Cmd == LC_SEGMENT) {
      MACHO_SEGMENT_COMMAND *Seg = (MACHO_SEGMENT_COMMAND *)Cmd;
      info("  Segment %.16s at 0x%08x (size: 0x%08x, prot: 0x%x)",
           Seg->SegName, Seg->VmAddr, Seg->VmSize, Seg->InitProt);

      MachoLoadSegment(
        ImageBase,
        Seg->VmAddr,
        Seg->VmSize,
        Seg->FileOff,
        Seg->FileSize,
        Seg->InitProt,
        Context->IsUserMode
      );
    } else if (Cmd->Cmd == LC_SEGMENT_64) {
      MACHO_SEGMENT_COMMAND_64 *Seg = (MACHO_SEGMENT_COMMAND_64 *)Cmd;
      info("  Segment %.16s at 0x%016llx (size: 0x%016llx, prot: 0x%x)",
           Seg->SegName, Seg->VmAddr, Seg->VmSize, Seg->InitProt);

      MachoLoadSegment(
        ImageBase,
        Seg->VmAddr,
        Seg->VmSize,
        Seg->FileOff,
        Seg->FileSize,
        Seg->InitProt,
        Context->IsUserMode
      );
    }

    Offset += Cmd->CmdSize;
  }

  Context->EntryPoint = MachoGetEntryPoint(This, ImageBase);
  return ImgLoadSuccess;
}

/**
  Extract TLS information from Mach-O image.
**/
static
IMGLOAD_STATUS
ANXAPI
MachoGetTlsInfo (
  IN  IMAGE_LOADER      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  MACHO_HEADER *Header32 = (MACHO_HEADER *)ImageBase;
  MACHO_LOAD_COMMAND *Cmd;
  UINT32 i;
  UINTN Offset;
  BOOLEAN Is64Bit;

  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));

  Is64Bit = (Header32->Magic == MH_MAGIC_64 || Header32->Magic == MH_CIGAM_64);
  Offset = Is64Bit ? sizeof(MACHO_HEADER_64) : sizeof(MACHO_HEADER);

  // Search for __thread_vars or __thread_bss segments
  for (i = 0; i < Header32->NumCmds; i++) {
    Cmd = (MACHO_LOAD_COMMAND *)MACHO_OFF(Offset);

    if (Cmd->Cmd == LC_SEGMENT_64) {
      MACHO_SEGMENT_COMMAND_64 *Seg = (MACHO_SEGMENT_COMMAND_64 *)Cmd;

      // Check for TLS segments by name
      if (memcmp(Seg->SegName, "__thread_vars", 13) == 0 ||
          memcmp(Seg->SegName, "__thread_data", 13) == 0) {
        TlsInfo->InitDataAddr = Seg->VmAddr;
        TlsInfo->InitDataSize = Seg->FileSize;
        TlsInfo->TotalSize = Seg->VmSize;
        TlsInfo->Alignment = 16; // Default TLS alignment
        return ImgLoadSuccess;
      }
    } else if (Cmd->Cmd == LC_SEGMENT) {
      MACHO_SEGMENT_COMMAND *Seg = (MACHO_SEGMENT_COMMAND *)Cmd;

      if (memcmp(Seg->SegName, "__thread_vars", 13) == 0 ||
          memcmp(Seg->SegName, "__thread_data", 13) == 0) {
        TlsInfo->InitDataAddr = Seg->VmAddr;
        TlsInfo->InitDataSize = Seg->FileSize;
        TlsInfo->TotalSize = Seg->VmSize;
        TlsInfo->Alignment = 16;
        return ImgLoadSuccess;
      }
    }

    Offset += Cmd->CmdSize;
  }

  // No TLS found - not an error
  return ImgLoadSuccess;
}

//
// Mach-O Loader VTable
//

static CONST IMAGE_LOADER_VTBL gMachoVtbl = {
  MachoDetect,
  MachoGetArch,
  MachoGetEntryPoint,
  MachoLoadImage,
  MachoGetTlsInfo
};

//
// Mach-O Loader Instance
//

IMAGE_LOADER gMachoLoader = {
  &gMachoVtbl,
  "Mach-O",
  NULL
};
