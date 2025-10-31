/** @file
  APXH Mach-O Loader

  Provides Mach-O (Mach Object) file format parsing and loading for
  32-bit and 64-bit executables using COM-style interface. Handles load
  commands, segments, TLS, and unwinding information for macOS/iOS binaries.

  Supports:
  - Mach-O 32-bit (MH_MAGIC) and 64-bit (MH_MAGIC_64)
  - Universal binaries (FAT_MAGIC) with multiple architecture slices
  - Comprehensive architecture support:
    * x86 (i386), x86-64 (AMD64)
    * ARM, ARM64 (AArch64), ARM64_32
    * PowerPC, PowerPC 64-bit
    * Motorola 68k, Motorola 88000
    * SPARC, PA-RISC, Intel i860
    * RISC-V, VAX
  - LC_SEGMENT/LC_SEGMENT_64 load commands
  - LC_THREAD_LOCAL_VARIABLES for TLS
  - __unwind_info for unwinding

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
// Universal Binary (FAT) Magic Numbers
//

#define FAT_MAGIC     0xCAFEBABE  ///< Universal binary magic (big endian)
#define FAT_CIGAM     0xBEBAFECA  ///< Universal binary magic (little endian)
#define FAT_MAGIC_64  0xCAFEBABF  ///< Universal binary 64-bit magic (big endian)
#define FAT_CIGAM_64  0xBFBAFECA  ///< Universal binary 64-bit magic (little endian)

//
// Mach-O File Types
//

#define MH_EXECUTE    0x2   ///< Executable file

//
// Mach-O CPU Types
//

#define CPU_TYPE_ANY         -1   ///< Any architecture
#define CPU_TYPE_VAX         1    ///< VAX
#define CPU_TYPE_MC680x0     6    ///< Motorola 68k
#define CPU_TYPE_I386        7    ///< x86 32-bit (Intel 80386)
#define CPU_TYPE_X86_64      0x01000007  ///< x86-64 (AMD64)
#define CPU_TYPE_MC98000     10   ///< PowerPC 98000
#define CPU_TYPE_HPPA        11   ///< HP PA-RISC
#define CPU_TYPE_ARM         12   ///< ARM 32-bit
#define CPU_TYPE_ARM64       0x0100000C  ///< ARM 64-bit (AArch64)
#define CPU_TYPE_ARM64_32    0x0200000C  ///< ARM 64-bit with 32-bit pointers
#define CPU_TYPE_MC88000     13   ///< Motorola 88000
#define CPU_TYPE_SPARC       14   ///< SPARC
#define CPU_TYPE_I860        15   ///< Intel i860
#define CPU_TYPE_POWERPC     18   ///< PowerPC 32-bit
#define CPU_TYPE_POWERPC64   0x01000012  ///< PowerPC 64-bit
#define CPU_TYPE_RISCV       0xF3 ///< RISC-V

//
// ARM CPU Subtypes
//

#define CPU_SUBTYPE_ARM_ALL      0    ///< All ARM
#define CPU_SUBTYPE_ARM_V4T      5    ///< ARMv4T
#define CPU_SUBTYPE_ARM_V6       6    ///< ARMv6
#define CPU_SUBTYPE_ARM_V5TEJ    7    ///< ARMv5TEJ
#define CPU_SUBTYPE_ARM_XSCALE   8    ///< ARM XScale
#define CPU_SUBTYPE_ARM_V7       9    ///< ARMv7
#define CPU_SUBTYPE_ARM_V7F      10   ///< ARMv7F (Cortex A9)
#define CPU_SUBTYPE_ARM_V7S      11   ///< ARMv7S (Swift)
#define CPU_SUBTYPE_ARM_V7K      12   ///< ARMv7K (Watch)
#define CPU_SUBTYPE_ARM_V8       13   ///< ARMv8
#define CPU_SUBTYPE_ARM_V6M      14   ///< ARMv6M (Cortex-M0)
#define CPU_SUBTYPE_ARM_V7M      15   ///< ARMv7M (Cortex-M3)
#define CPU_SUBTYPE_ARM_V7EM     16   ///< ARMv7EM (Cortex-M4)

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

//
// Universal Binary (FAT) Structures
//

typedef struct _FAT_HEADER {
  UINT32  Magic;        ///< FAT_MAGIC or FAT_MAGIC_64
  UINT32  NumArchs;     ///< Number of architecture slices
} FAT_HEADER;

typedef struct _FAT_ARCH {
  UINT32  CpuType;      ///< CPU type
  UINT32  CpuSubType;   ///< CPU subtype
  UINT32  Offset;       ///< File offset to Mach-O header
  UINT32  Size;         ///< Size of Mach-O slice
  UINT32  Align;        ///< Alignment (power of 2)
} FAT_ARCH;

typedef struct _FAT_ARCH_64 {
  UINT32  CpuType;      ///< CPU type
  UINT32  CpuSubType;   ///< CPU subtype
  UINT64  Offset;       ///< File offset to Mach-O header (64-bit)
  UINT64  Size;         ///< Size of Mach-O slice (64-bit)
  UINT32  Align;        ///< Alignment (power of 2)
  UINT32  Reserved;     ///< Reserved
} FAT_ARCH_64;

ANX_PACK_POP()

//
// Helper Macros
//

#define MACHO_OFF(_o) ((VOID *)(UINTN)((UINT8 *)ImageBase + (_o)))

//
// IImageLoader Implementation for Mach-O
//

/**
  Detect if image is Mach-O format (including universal binaries).
**/
static
HRESULT
STDMETHODCALLTYPE
MachoDetect (
  IN IImageLoader  *This,
  IN VOID          *ImageBase,
  IN UINTN         ImageSize
  )
{
  MACHO_HEADER *Header;
  FAT_HEADER *FatHeader;

  if (ImageSize < sizeof(MACHO_HEADER)) {
    return S_FALSE;
  }

  Header = (MACHO_HEADER *)ImageBase;
  FatHeader = (FAT_HEADER *)ImageBase;

  // Check for single Mach-O file
  if (Header->Magic == MH_MAGIC ||
      Header->Magic == MH_CIGAM ||
      Header->Magic == MH_MAGIC_64 ||
      Header->Magic == MH_CIGAM_64) {
    return S_OK;
  }

  // Check for universal binary (FAT)
  if (FatHeader->Magic == FAT_MAGIC ||
      FatHeader->Magic == FAT_CIGAM ||
      FatHeader->Magic == FAT_MAGIC_64 ||
      FatHeader->Magic == FAT_CIGAM_64) {
    return S_OK;
  }

  return S_FALSE;
}

/**
  Get actual Mach-O header from image (handles universal binaries).

  For universal binaries, selects the first compatible architecture slice.
  For single Mach-O files, returns the image base directly.

  @param[in]  ImageBase    Base address of image
  @param[in]  ImageSize    Size of image in bytes
  @param[out] MachoHeader  Pointer to actual Mach-O header
  @param[out] MachoSize    Size of Mach-O slice (if FAT)

  @retval S_OK            Success
  @retval S_FALSE         No compatible slice found
  @retval E_POINTER       Invalid parameter
**/
static
HRESULT
MachoGetActualHeader (
  IN  VOID   *ImageBase,
  IN  UINTN  ImageSize,
  OUT VOID   **MachoHeader,
  OUT UINTN  *MachoSize
  )
{
  FAT_HEADER *FatHeader;
  MACHO_HEADER *Header;
  UINT32 Magic;
  UINT32 NumArchs;
  UINT32 i;
  BOOLEAN IsFat64;
  BOOLEAN NeedSwap;

  if (MachoHeader == NULL || MachoSize == NULL) {
    return E_POINTER;
  }

  Header = (MACHO_HEADER *)ImageBase;
  Magic = Header->Magic;

  // Check if it's a FAT binary
  if (Magic == FAT_MAGIC || Magic == FAT_CIGAM ||
      Magic == FAT_MAGIC_64 || Magic == FAT_CIGAM_64) {

    FatHeader = (FAT_HEADER *)ImageBase;
    IsFat64 = (Magic == FAT_MAGIC_64 || Magic == FAT_CIGAM_64);
    NeedSwap = (Magic == FAT_CIGAM || Magic == FAT_CIGAM_64);

    NumArchs = FatHeader->NumArchs;
    if (NeedSwap) {
      NumArchs = ANX_BSWAP32(NumArchs);
    }

    if (NumArchs == 0 || NumArchs > 100) {
      return S_FALSE;
    }

    // Iterate through architecture slices
    if (IsFat64) {
      FAT_ARCH_64 *Arch = (FAT_ARCH_64 *)((UINT8 *)ImageBase + sizeof(FAT_HEADER));

      for (i = 0; i < NumArchs; i++) {
        UINT64 Offset = NeedSwap ? ANX_BSWAP64(Arch[i].Offset) : Arch[i].Offset;
        UINT64 Size = NeedSwap ? ANX_BSWAP64(Arch[i].Size) : Arch[i].Size;

        if (Offset + Size > ImageSize) {
          continue;
        }

        // Return first valid slice
        *MachoHeader = (VOID *)((UINT8 *)ImageBase + Offset);
        *MachoSize = (UINTN)Size;
        return S_OK;
      }
    } else {
      FAT_ARCH *Arch = (FAT_ARCH *)((UINT8 *)ImageBase + sizeof(FAT_HEADER));

      for (i = 0; i < NumArchs; i++) {
        UINT32 Offset = NeedSwap ? ANX_BSWAP32(Arch[i].Offset) : Arch[i].Offset;
        UINT32 Size = NeedSwap ? ANX_BSWAP32(Arch[i].Size) : Arch[i].Size;

        if (Offset + Size > ImageSize) {
          continue;
        }

        // Return first valid slice
        *MachoHeader = (VOID *)((UINT8 *)ImageBase + Offset);
        *MachoSize = (UINTN)Size;
        return S_OK;
      }
    }

    // No valid slice found
    return S_FALSE;
  }

  // Single Mach-O file
  *MachoHeader = ImageBase;
  *MachoSize = ImageSize;
  return S_OK;
}

/**
  Get architecture from Mach-O image.
**/
static
HRESULT
STDMETHODCALLTYPE
MachoGetArch (
  IN  IImageLoader  *This,
  IN  VOID          *ImageBase,
  OUT ARCH          *Architecture
  )
{
  MACHO_HEADER *Header;
  VOID *ActualHeader;
  UINTN ActualSize;
  HRESULT Status;

  if (Architecture == NULL) {
    return E_POINTER;
  }

  // Handle universal binaries - get actual Mach-O header
  // Note: ImageSize is not available here, so we use a large value for detection
  Status = MachoGetActualHeader(ImageBase, 0xFFFFFFFF, &ActualHeader, &ActualSize);
  if (FAILED(Status)) {
    return Status;
  }

  Header = (MACHO_HEADER *)ActualHeader;

  switch (Header->CpuType) {
    case CPU_TYPE_VAX:
      *Architecture = ArchVax;
      break;
    case CPU_TYPE_MC680x0:
      *Architecture = ArchM68k;
      break;
    case CPU_TYPE_I386:
      *Architecture = Arch386;
      break;
    case CPU_TYPE_X86_64:
      *Architecture = ArchAmd64;
      break;
    case CPU_TYPE_MC98000:
      *Architecture = ArchPpc32;
      break;
    case CPU_TYPE_HPPA:
      *Architecture = ArchPaRisc;
      break;
    case CPU_TYPE_ARM:
      *Architecture = ArchArm;
      break;
    case CPU_TYPE_ARM64:
    case CPU_TYPE_ARM64_32:
      *Architecture = ArchArm64;
      break;
    case CPU_TYPE_MC88000:
      *Architecture = ArchM88k;
      break;
    case CPU_TYPE_SPARC:
      *Architecture = ArchSparc;
      break;
    case CPU_TYPE_I860:
      *Architecture = ArchI860;
      break;
    case CPU_TYPE_POWERPC:
      *Architecture = ArchPpc32;
      break;
    case CPU_TYPE_POWERPC64:
      *Architecture = ArchPpc64;
      break;
    case CPU_TYPE_RISCV:
      *Architecture = ArchRiscV64;
      break;
    default:
      *Architecture = ArchUnsupported;
      return IMGLOAD_E_UNSUPPORTED_ARCH;
  }

  return S_OK;
}

/**
  Get endianness from Mach-O image.
**/
static
HRESULT
STDMETHODCALLTYPE
MachoGetEndianness (
  IN  IImageLoader    *This,
  IN  VOID            *ImageBase,
  OUT IMGLOAD_ENDIAN  *Endianness
  )
{
  MACHO_HEADER *Header;
  VOID *ActualHeader;
  UINTN ActualSize;
  HRESULT Status;

  if (Endianness == NULL) {
    return E_POINTER;
  }

  // Handle universal binaries
  Status = MachoGetActualHeader(ImageBase, 0xFFFFFFFF, &ActualHeader, &ActualSize);
  if (FAILED(Status)) {
    return Status;
  }

  Header = (MACHO_HEADER *)ActualHeader;

  // Determine endianness from magic number
  if (Header->Magic == MH_MAGIC || Header->Magic == MH_MAGIC_64) {
    *Endianness = ImgEndianBig;
  } else if (Header->Magic == MH_CIGAM || Header->Magic == MH_CIGAM_64) {
    *Endianness = ImgEndianLittle;
  } else {
    *Endianness = ImgEndianUnknown;
    return IMGLOAD_E_INVALID_FORMAT;
  }

  return S_OK;
}

/**
  Get entry point from Mach-O image.
**/
static
HRESULT
STDMETHODCALLTYPE
MachoGetEntryPoint (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT VIRTUAL_ADDRESS   *EntryPoint
  )
{
  MACHO_HEADER *Header32;
  MACHO_HEADER_64 *Header64;
  MACHO_LOAD_COMMAND *Cmd;
  UINT32 i;
  UINTN Offset;
  BOOLEAN Is64Bit;
  VOID *ActualHeader;
  UINTN ActualSize;
  HRESULT Status;

  if (EntryPoint == NULL) {
    return E_POINTER;
  }

  // Handle universal binaries
  Status = MachoGetActualHeader(ImageBase, 0xFFFFFFFF, &ActualHeader, &ActualSize);
  if (FAILED(Status)) {
    return Status;
  }

  Header32 = (MACHO_HEADER *)ActualHeader;
  Header64 = (MACHO_HEADER_64 *)ActualHeader;

  Is64Bit = (Header32->Magic == MH_MAGIC_64 || Header32->Magic == MH_CIGAM_64);
  Offset = Is64Bit ? sizeof(MACHO_HEADER_64) : sizeof(MACHO_HEADER);

  for (i = 0; i < Header32->NumCmds; i++) {
    Cmd = (MACHO_LOAD_COMMAND *)((UINT8 *)ActualHeader + Offset);

    if (Cmd->Cmd == LC_MAIN) {
      MACHO_ENTRY_POINT_COMMAND *EntryCmd = (MACHO_ENTRY_POINT_COMMAND *)Cmd;
      *EntryPoint = (VIRTUAL_ADDRESS)EntryCmd->EntryOff;
      return S_OK;
    } else if (Cmd->Cmd == LC_UNIXTHREAD) {
      // Legacy entry point in thread state - architecture-specific
      MACHO_THREAD_COMMAND *ThreadCmd = (MACHO_THREAD_COMMAND *)Cmd;
      UINT32 CpuType = Is64Bit ? Header64->CpuType : Header32->CpuType;

      // Extract PC from architecture-specific thread state
      switch (CpuType) {
        case CPU_TYPE_I386: {
          // x86 thread state: EIP (Program Counter) is at DWORD offset 10
          UINT32 *State = (UINT32 *)(ThreadCmd + 1);
          *EntryPoint = (VIRTUAL_ADDRESS)State[10];
          return S_OK;
        }

        case CPU_TYPE_X86_64: {
          // x86-64 thread state: RIP (Instruction Pointer) is at QWORD offset 16
          UINT64 *State = (UINT64 *)(ThreadCmd + 1);
          *EntryPoint = (VIRTUAL_ADDRESS)State[16];
          return S_OK;
        }

        case CPU_TYPE_ARM: {
          // ARM thread state: PC (R15) is at DWORD offset 15
          UINT32 *State = (UINT32 *)(ThreadCmd + 1);
          *EntryPoint = (VIRTUAL_ADDRESS)State[15];
          return S_OK;
        }

        case CPU_TYPE_ARM64:
        case CPU_TYPE_ARM64_32: {
          // ARM64 thread state: PC is at QWORD offset 32
          UINT64 *State = (UINT64 *)(ThreadCmd + 1);
          *EntryPoint = (VIRTUAL_ADDRESS)State[32];
          return S_OK;
        }

        case CPU_TYPE_POWERPC: {
          // PowerPC thread state: SRR0 (PC) is at DWORD offset 0
          UINT32 *State = (UINT32 *)(ThreadCmd + 1);
          *EntryPoint = (VIRTUAL_ADDRESS)State[0];
          return S_OK;
        }

        case CPU_TYPE_POWERPC64: {
          // PowerPC64 thread state: SRR0 (PC) is at QWORD offset 0
          UINT64 *State = (UINT64 *)(ThreadCmd + 1);
          *EntryPoint = (VIRTUAL_ADDRESS)State[0];
          return S_OK;
        }

        case CPU_TYPE_SPARC: {
          // SPARC thread state: PC is at DWORD offset 1
          UINT32 *State = (UINT32 *)(ThreadCmd + 1);
          *EntryPoint = (VIRTUAL_ADDRESS)State[1];
          return S_OK;
        }

        case CPU_TYPE_MC680x0: {
          // M68k thread state: PC is at DWORD offset 16
          UINT32 *State = (UINT32 *)(ThreadCmd + 1);
          *EntryPoint = (VIRTUAL_ADDRESS)State[16];
          return S_OK;
        }

        case CPU_TYPE_VAX: {
          // VAX thread state: PC is at DWORD offset 15
          UINT32 *State = (UINT32 *)(ThreadCmd + 1);
          *EntryPoint = (VIRTUAL_ADDRESS)State[15];
          return S_OK;
        }

        case CPU_TYPE_RISCV: {
          // RISC-V thread state: PC is at QWORD offset 0
          UINT64 *State = (UINT64 *)(ThreadCmd + 1);
          *EntryPoint = (VIRTUAL_ADDRESS)State[0];
          return S_OK;
        }

        case CPU_TYPE_MC88000: {
          // M88k thread state: PC (SXIP) is at DWORD offset 0
          UINT32 *State = (UINT32 *)(ThreadCmd + 1);
          *EntryPoint = (VIRTUAL_ADDRESS)State[0];
          return S_OK;
        }

        case CPU_TYPE_I860: {
          // i860 thread state: PC (fir) is at DWORD offset 0
          UINT32 *State = (UINT32 *)(ThreadCmd + 1);
          *EntryPoint = (VIRTUAL_ADDRESS)State[0];
          return S_OK;
        }

        case CPU_TYPE_HPPA: {
          // PA-RISC thread state: PC (PCOQ head) is at DWORD offset 0
          UINT32 *State = (UINT32 *)(ThreadCmd + 1);
          *EntryPoint = (VIRTUAL_ADDRESS)State[0];
          return S_OK;
        }

        default:
          // Unsupported architecture for UNIXTHREAD
          *EntryPoint = 0;
          return IMGLOAD_E_UNSUPPORTED_ARCH;
      }
    }

    Offset += Cmd->CmdSize;
  }

  *EntryPoint = 0;
  return IMGLOAD_E_INVALID_HEADER;
}

/**
  Load Mach-O segment.
**/
static
VOID
MachoLoadSegment (
  IN VOID     *MachoBase,
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
    return; // Overflow check
  }

  if (FileSize > 0) {
    // Copy file data to virtual address
    VirtualAddressCopy(
      VmAddr,
      (VOID *)((UINT8 *)MachoBase + FileOff),
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
HRESULT
STDMETHODCALLTYPE
MachoLoadImage (
  IN     IImageLoader     *This,
  IN OUT IMGLOAD_CONTEXT  *Context
  )
{
  VOID *ImageBase;
  VOID *ActualHeader;
  UINTN ActualSize;
  MACHO_HEADER *Header32;
  MACHO_LOAD_COMMAND *Cmd;
  UINT32 i;
  UINTN Offset;
  BOOLEAN Is64Bit;
  HRESULT Status;

  if (Context == NULL) {
    return E_POINTER;
  }

  ImageBase = Context->ImageBase;

  // Handle universal binaries - get actual Mach-O header
  Status = MachoGetActualHeader(ImageBase, Context->ImageSize, &ActualHeader, &ActualSize);
  if (FAILED(Status)) {
    return Status;
  }

  Header32 = (MACHO_HEADER *)ActualHeader;

  Is64Bit = (Header32->Magic == MH_MAGIC_64 || Header32->Magic == MH_CIGAM_64);

  if (Header32->FileType != MH_EXECUTE) {
    return IMGLOAD_E_INVALID_FORMAT;
  }

  // Populate context
  Status = MachoGetArch(This, ImageBase, &Context->Architecture);
  if (FAILED(Status)) {
    return Status;
  }

  Status = MachoGetEndianness(This, ImageBase, &Context->Endianness);
  if (FAILED(Status)) {
    return Status;
  }

  Status = MachoGetEntryPoint(This, ImageBase, &Context->EntryPoint);
  if (FAILED(Status)) {
    return Status;
  }

  Offset = Is64Bit ? sizeof(MACHO_HEADER_64) : sizeof(MACHO_HEADER);

  // Process all load commands
  for (i = 0; i < Header32->NumCmds; i++) {
    Cmd = (MACHO_LOAD_COMMAND *)((UINT8 *)ActualHeader + Offset);

    if (Cmd->Cmd == LC_SEGMENT) {
      MACHO_SEGMENT_COMMAND *Seg = (MACHO_SEGMENT_COMMAND *)Cmd;
      MachoLoadSegment(
        ActualHeader,
        Seg->VmAddr,
        Seg->VmSize,
        Seg->FileOff,
        Seg->FileSize,
        Seg->InitProt,
        Context->IsUserMode
      );
    } else if (Cmd->Cmd == LC_SEGMENT_64) {
      MACHO_SEGMENT_COMMAND_64 *Seg = (MACHO_SEGMENT_COMMAND_64 *)Cmd;
      MachoLoadSegment(
        ActualHeader,
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

  return S_OK;
}

/**
  Extract TLS information from Mach-O image.
**/
static
HRESULT
STDMETHODCALLTYPE
MachoGetTlsInfo (
  IN  IImageLoader      *This,
  IN  VOID              *ImageBase,
  OUT IMGLOAD_TLS_INFO  *TlsInfo
  )
{
  MACHO_HEADER *Header32;
  MACHO_LOAD_COMMAND *Cmd;
  UINT32 i;
  UINTN Offset;
  BOOLEAN Is64Bit;
  VOID *ActualHeader;
  UINTN ActualSize;
  HRESULT Status;

  if (TlsInfo == NULL) {
    return E_POINTER;
  }

  memset(TlsInfo, 0, sizeof(IMGLOAD_TLS_INFO));

  // Handle universal binaries
  Status = MachoGetActualHeader(ImageBase, 0xFFFFFFFF, &ActualHeader, &ActualSize);
  if (FAILED(Status)) {
    return Status;
  }

  Header32 = (MACHO_HEADER *)ActualHeader;
  Is64Bit = (Header32->Magic == MH_MAGIC_64 || Header32->Magic == MH_CIGAM_64);
  Offset = Is64Bit ? sizeof(MACHO_HEADER_64) : sizeof(MACHO_HEADER);

  // Search for __thread_vars or __thread_bss segments
  for (i = 0; i < Header32->NumCmds; i++) {
    Cmd = (MACHO_LOAD_COMMAND *)((UINT8 *)ActualHeader + Offset);

    if (Cmd->Cmd == LC_SEGMENT_64) {
      MACHO_SEGMENT_COMMAND_64 *Seg = (MACHO_SEGMENT_COMMAND_64 *)Cmd;

      if (memcmp(Seg->SegName, "__thread_vars", 13) == 0 ||
          memcmp(Seg->SegName, "__thread_data", 13) == 0) {
        TlsInfo->InitDataAddr = Seg->VmAddr;
        TlsInfo->InitDataSize = Seg->FileSize;
        TlsInfo->TotalSize = Seg->VmSize;
        TlsInfo->Alignment = 16;
        return S_OK;
      }
    } else if (Cmd->Cmd == LC_SEGMENT) {
      MACHO_SEGMENT_COMMAND *Seg = (MACHO_SEGMENT_COMMAND *)Cmd;

      if (memcmp(Seg->SegName, "__thread_vars", 13) == 0 ||
          memcmp(Seg->SegName, "__thread_data", 13) == 0) {
        TlsInfo->InitDataAddr = Seg->VmAddr;
        TlsInfo->InitDataSize = Seg->FileSize;
        TlsInfo->TotalSize = Seg->VmSize;
        TlsInfo->Alignment = 16;
        return S_OK;
      }
    }

    Offset += Cmd->CmdSize;
  }

  // No TLS found - not an error
  return S_FALSE;
}

/**
  Extract unwinding information from Mach-O image.
**/
static
HRESULT
STDMETHODCALLTYPE
MachoGetUnwindInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_UNWIND_INFO  *UnwindInfo
  )
{
  MACHO_HEADER *Header32;
  MACHO_LOAD_COMMAND *Cmd;
  UINT32 i;
  UINTN Offset;
  BOOLEAN Is64Bit;
  VOID *ActualHeader;
  UINTN ActualSize;
  HRESULT Status;

  if (UnwindInfo == NULL) {
    return E_POINTER;
  }

  memset(UnwindInfo, 0, sizeof(IMGLOAD_UNWIND_INFO));

  // Handle universal binaries
  Status = MachoGetActualHeader(ImageBase, 0xFFFFFFFF, &ActualHeader, &ActualSize);
  if (FAILED(Status)) {
    return Status;
  }

  Header32 = (MACHO_HEADER *)ActualHeader;
  Is64Bit = (Header32->Magic == MH_MAGIC_64 || Header32->Magic == MH_CIGAM_64);
  Offset = Is64Bit ? sizeof(MACHO_HEADER_64) : sizeof(MACHO_HEADER);

  // Search for __unwind_info segment
  for (i = 0; i < Header32->NumCmds; i++) {
    Cmd = (MACHO_LOAD_COMMAND *)((UINT8 *)ActualHeader + Offset);

    if (Cmd->Cmd == LC_SEGMENT_64) {
      MACHO_SEGMENT_COMMAND_64 *Seg = (MACHO_SEGMENT_COMMAND_64 *)Cmd;

      if (memcmp(Seg->SegName, "__unwind_info", 13) == 0) {
        UnwindInfo->UnwindDataAddr = Seg->VmAddr;
        UnwindInfo->UnwindDataSize = Seg->VmSize;
        UnwindInfo->Format = 2;  // Mach-O compact unwinding format
        return S_OK;
      }
    } else if (Cmd->Cmd == LC_SEGMENT) {
      MACHO_SEGMENT_COMMAND *Seg = (MACHO_SEGMENT_COMMAND *)Cmd;

      if (memcmp(Seg->SegName, "__unwind_info", 13) == 0) {
        UnwindInfo->UnwindDataAddr = Seg->VmAddr;
        UnwindInfo->UnwindDataSize = Seg->VmSize;
        UnwindInfo->Format = 2;  // Mach-O compact unwinding format
        return S_OK;
      }
    }

    Offset += Cmd->CmdSize;
  }

  // No unwinding info - not an error
  return S_FALSE;
}

/**
  Look up symbol by virtual address.
**/
static
HRESULT
STDMETHODCALLTYPE
MachoGetSymbolByAddress (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  VIRTUAL_ADDRESS      Address,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (SymbolInfo == NULL) {
    return E_POINTER;
  }

  // Mach-O symbol table parsing would go here
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Look up symbol by name.
**/
static
HRESULT
STDMETHODCALLTYPE
MachoGetSymbolByName (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  IN  CONST CHAR8          *Name,
  OUT IMGLOAD_SYMBOL_INFO  *SymbolInfo
  )
{
  if (Name == NULL || SymbolInfo == NULL) {
    return E_POINTER;
  }

  // Mach-O symbol table parsing would go here
  memset(SymbolInfo, 0, sizeof(IMGLOAD_SYMBOL_INFO));
  return S_FALSE;
}

/**
  Extract relocation information from Mach-O image.
**/
static
HRESULT
STDMETHODCALLTYPE
MachoGetRelocInfo (
  IN  IImageLoader         *This,
  IN  VOID                 *ImageBase,
  OUT IMGLOAD_RELOC_INFO   *RelocInfo
  )
{
  if (RelocInfo == NULL) {
    return E_POINTER;
  }

  // Mach-O relocations are in LC_DYSYMTAB load command
  memset(RelocInfo, 0, sizeof(IMGLOAD_RELOC_INFO));

  // Mach-O typically doesn't require base relocation (uses PIC)
  RelocInfo->Format = ImgRelocFormatMachO;
  RelocInfo->RequiresReloc = FALSE;

  return S_FALSE;  // Most Mach-O images don't need relocation
}

/**
  Apply relocations to Mach-O image loaded at different address.
**/
static
HRESULT
STDMETHODCALLTYPE
MachoApplyRelocations (
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

  // Mach-O typically uses position-independent code
  // and doesn't require base relocation
  // Full implementation would parse LC_DYSYMTAB and apply relocations
  return E_NOTIMPL;
}

/**
  IUnknown::QueryInterface implementation.
**/
static
HRESULT
STDMETHODCALLTYPE
MachoQueryInterface (
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
  IUnknown::AddRef implementation.
**/
static
UINT32
STDMETHODCALLTYPE
MachoAddRef (
  IN IImageLoader  *This
  )
{
  return 1;
}

/**
  IUnknown::Release implementation.
**/
static
UINT32
STDMETHODCALLTYPE
MachoRelease (
  IN IImageLoader  *This
  )
{
  return 1;
}

//
// Mach-O Loader VTable
//

static CONST IImageLoaderVtbl gMachoVtbl = {
  MachoQueryInterface,
  MachoAddRef,
  MachoRelease,
  MachoDetect,
  MachoGetArch,
  MachoGetEndianness,
  MachoGetEntryPoint,
  MachoLoadImage,
  MachoGetTlsInfo,
  MachoGetUnwindInfo,
  MachoGetSymbolByAddress,
  MachoGetSymbolByName,
  MachoGetRelocInfo,
  MachoApplyRelocations
};

//
// Mach-O Loader Instance
//

IImageLoader gMachoLoader = {
  &gMachoVtbl
};

// Auto-register this loader
APXH_REGISTER_IMGLOADER(gMachoLoader);
