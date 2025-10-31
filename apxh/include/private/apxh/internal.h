/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/
#pragma once

#include <ananke/ananke.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <nux/framebuffer.h>
#include <apxh/apxh.h>

#define BOOTMEM MB(512)		/* We won't be using more than 512Mb to boot. Promise. */

typedef INT64 SSIZE64;
typedef UINT64 SIZE64;
typedef UINT64 VIRTUAL_ADDRESS;

/**
  Boot Info Region Type
**/
typedef enum _BOOTINFO_REGION_TYPE {
  BootInfoRegionUnknown = 0,  ///< Unusable address
  BootInfoRegionRam     = 1,  ///< Available RAM
  BootInfoRegionOther   = 2,  ///< Non-RAM physical address
  BootInfoRegionBusy    = 3   ///< Boot allocated RAM
} BOOTINFO_REGION_TYPE;

/**
  Boot Info Region Descriptor
**/
typedef struct _BOOTINFO_REGION
{
  INT32 Type;
  UINT32 Length;
  UINTN PageFrameNumber;
} BOOTINFO_REGION, *PBOOTINFO_REGION, *PCBOOTINFO_REGION;

/**
  Memory Type for Page Mappings
**/
typedef enum _MEMORY_TYPE
{
  MemTypeWriteCombining = 0,  ///< Write Combining
  MemTypeWriteBack      = 1,  ///< Write Back (cacheable)
  MemTypeUncached       = 2   ///< Uncached
} MEMORY_TYPE;

/* APXH ELF extensions. */
typedef enum _APXH_PROGRAM_HEADER_TYPE {
  ApxhProgramHeaderInfo          = 0xAF100000,  ///< Info Page
  ApxhProgramHeaderEmpty         = 0xAF100001,  ///< Empty (no page tables)
  ApxhProgramHeaderPhysicalMap   = 0xAF100002,  ///< 1:1 Memory Map
  ApxhProgramHeaderPfnMap        = 0xAF100003,  ///< PFN Map
  ApxhProgramHeaderBatree        = 0xAF100004,  ///< Allocated Pages Bitmap
  ApxhProgramHeaderPageTableAlloc = 0xAF100005, ///< Empty (alloc all page tables)
  ApxhProgramHeaderFramebuffer   = 0xAF100006,  ///< Frame Buffer
  ApxhProgramHeaderRegions       = 0xAF100007,  ///< Region List
  ApxhProgramHeaderTopPageTableAlloc = 0xAF100008, ///< Empty (alloc all top-level PTs)
  ApxhProgramHeaderLinear        = 0xAF10FFFF   ///< Linear map
} APXH_PROGRAM_HEADER_TYPE;

#define PFNMAP_ENTRY_SIZE 64

#define PAGE_SHIFT 12
#define PAGE_SIZE (1LL << PAGE_SHIFT)
#define PAGE_MASK ~(PAGE_SIZE - 1)

#define PAGE2M_SHIFT 21
#define PAGE2M_SIZE (1LL << PAGE2M_SHIFT)
#define PAGE2M_MASK ~(PAGE2M_SIZE - 1)

#define MB(_x) ((UINTN)(_x) << 20)
#define BITMAP_SZ(_s) ((_s) >> 3)	// POW2
#define PAGEMAP_SZ(_s) BITMAP_SZ((_s) >> PAGE_SHIFT)	// POW2

#define PAGE_ROUND(_a) (((_a) + (PAGE_SIZE-1)) & PAGE_MASK)
#define PAGE_CEILING(_a) (((_a) + PAGE_SIZE) & PAGE_MASK)

typedef enum
{
  ARCH_INVALID,
  ARCH_UNSUPPORTED,

  // x86 family (Intel/AMD)
  ARCH_8086,           ///< Intel 8086
  ARCH_80186,          ///< Intel 80186
  ARCH_80286,          ///< Intel 80286
  ARCH_386,            ///< Intel 80386 and compatibles
  ARCH_486,            ///< Intel 80486 and compatibles
  ARCH_PENTIUM,        ///< Intel Pentium
  ARCH_AMD64,          ///< AMD64/x86-64/EM64T
  ARCH_I860,           ///< Intel i860 RISC
  ARCH_I960,           ///< Intel i960 RISC

  // RISC-V
  ARCH_RISCV32,        ///< RISC-V 32-bit
  ARCH_RISCV64,        ///< RISC-V 64-bit
  ARCH_RISCV128,       ///< RISC-V 128-bit

  // ARM family (all variants)
  ARCH_ARM,            ///< ARM (generic 32-bit)
  ARCH_ARMV4,          ///< ARMv4
  ARCH_ARMV4T,         ///< ARMv4T (Thumb)
  ARCH_ARMV5,          ///< ARMv5
  ARCH_ARMV5T,         ///< ARMv5T (Thumb)
  ARCH_ARMV6,          ///< ARMv6
  ARCH_ARMV7,          ///< ARMv7
  ARCH_ARM64,          ///< ARM64/AArch64
  ARCH_THUMB,          ///< ARM Thumb mode

  // PowerPC family
  ARCH_PPC32,          ///< PowerPC 32-bit
  ARCH_PPC64,          ///< PowerPC 64-bit
  ARCH_PPC601,         ///< PowerPC 601
  ARCH_PPC603,         ///< PowerPC 603
  ARCH_PPC604,         ///< PowerPC 604
  ARCH_PPC750,         ///< PowerPC 750 (G3)
  ARCH_PPC7400,        ///< PowerPC 7400 (G4)
  ARCH_PPC970,         ///< PowerPC 970 (G5)

  // MIPS family
  ARCH_MIPS32,         ///< MIPS 32-bit (generic)
  ARCH_MIPS64,         ///< MIPS 64-bit (generic)
  ARCH_MIPS_R3000,     ///< MIPS R3000
  ARCH_MIPS_R4000,     ///< MIPS R4000
  ARCH_MIPS_R5000,     ///< MIPS R5000
  ARCH_MIPS_R6000,     ///< MIPS R6000
  ARCH_MIPS_R8000,     ///< MIPS R8000
  ARCH_MIPS_R10000,    ///< MIPS R10000
  ARCH_MIPS16,         ///< MIPS16

  // Alpha (DEC/Compaq/HP)
  ARCH_ALPHA,          ///< DEC Alpha (generic)
  ARCH_ALPHA_21064,    ///< Alpha 21064 (EV4)
  ARCH_ALPHA_21164,    ///< Alpha 21164 (EV5)
  ARCH_ALPHA_21264,    ///< Alpha 21264 (EV6)

  // PA-RISC (HP)
  ARCH_PARISC,         ///< PA-RISC 32-bit
  ARCH_PARISC64,       ///< PA-RISC 64-bit (PA-RISC 2.0)
  ARCH_PARISC1_0,      ///< PA-RISC 1.0
  ARCH_PARISC1_1,      ///< PA-RISC 1.1
  ARCH_PARISC2_0,      ///< PA-RISC 2.0

  // SPARC (Sun/Oracle)
  ARCH_SPARC32,        ///< SPARC 32-bit (V8)
  ARCH_SPARC64,        ///< SPARC 64-bit (V9)
  ARCH_SPARCV7,        ///< SPARC V7
  ARCH_SPARCV8,        ///< SPARC V8
  ARCH_SPARCV9,        ///< SPARC V9

  // Itanium (Intel)
  ARCH_IA64,           ///< Intel Itanium (IA-64)

  // Motorola 68k family
  ARCH_M68K,           ///< Motorola 68000 (generic)
  ARCH_M68000,         ///< Motorola 68000
  ARCH_M68010,         ///< Motorola 68010
  ARCH_M68020,         ///< Motorola 68020
  ARCH_M68030,         ///< Motorola 68030
  ARCH_M68040,         ///< Motorola 68040
  ARCH_M68060,         ///< Motorola 68060
  ARCH_COLDFIRE,       ///< Motorola/Freescale ColdFire

  // Motorola 88k family
  ARCH_M88K,           ///< Motorola 88000 (generic)
  ARCH_M88100,         ///< Motorola 88100
  ARCH_M88110,         ///< Motorola 88110

  // LoongArch (Loongson)
  ARCH_LOONGARCH32,    ///< LoongArch 32-bit
  ARCH_LOONGARCH64,    ///< LoongArch 64-bit

  // VAX (DEC)
  ARCH_VAX,            ///< DEC VAX

  // PDP (DEC)
  ARCH_PDP7,           ///< DEC PDP-7
  ARCH_PDP8,           ///< DEC PDP-8
  ARCH_PDP9,           ///< DEC PDP-9
  ARCH_PDP10,          ///< DEC PDP-10
  ARCH_PDP11,          ///< DEC PDP-11
  ARCH_PDP15,          ///< DEC PDP-15

  // IBM mainframe
  ARCH_S360,           ///< IBM System/360
  ARCH_S370,           ///< IBM System/370
  ARCH_S390,           ///< IBM S/390 (31-bit)
  ARCH_S390X,          ///< IBM z/Architecture (64-bit)

  // POWER (IBM)
  ARCH_POWER,          ///< IBM POWER (generic)
  ARCH_POWER1,         ///< IBM POWER1
  ARCH_POWER2,         ///< IBM POWER2
  ARCH_POWER3,         ///< IBM POWER3
  ARCH_POWER4,         ///< IBM POWER4
  ARCH_POWER5,         ///< IBM POWER5
  ARCH_POWER6,         ///< IBM POWER6
  ARCH_POWER7,         ///< IBM POWER7
  ARCH_POWER8,         ///< IBM POWER8
  ARCH_POWER9,         ///< IBM POWER9
  ARCH_POWER10,        ///< IBM POWER10

  // SuperH (Hitachi/Renesas)
  ARCH_SH,             ///< SuperH (generic)
  ARCH_SH1,            ///< SuperH SH-1
  ARCH_SH2,            ///< SuperH SH-2
  ARCH_SH3,            ///< SuperH SH-3
  ARCH_SH4,            ///< SuperH SH-4
  ARCH_SH5,            ///< SuperH SH-5

  // AT&T/Bellmac/WE
  ARCH_WE32000,        ///< AT&T WE32000 (3B series)
  ARCH_WE32100,        ///< AT&T WE32100
  ARCH_WE32200,        ///< AT&T WE32200

  // National Semiconductor
  ARCH_NS32016,        ///< NS32016
  ARCH_NS32032,        ///< NS32032
  ARCH_NS32332,        ///< NS32332
  ARCH_NS32532,        ///< NS32532

  // Clipper (Fairchild/Intergraph)
  ARCH_CLIPPER,        ///< Clipper (generic)
  ARCH_CLIPPER_C100,   ///< Clipper C100
  ARCH_CLIPPER_C300,   ///< Clipper C300
  ARCH_CLIPPER_C400,   ///< Clipper C400

  // ROMP (IBM RT PC)
  ARCH_ROMP,           ///< IBM ROMP

  // AMD 29000
  ARCH_AM29000,        ///< AMD Am29000

  // Pyramid
  ARCH_PYRAMID,        ///< Pyramid 90x

  // Convex
  ARCH_CONVEX,         ///< Convex

  // Cray
  ARCH_CRAY1,          ///< Cray-1
  ARCH_CRAY2,          ///< Cray-2
  ARCH_CRAY_X_MP,      ///< Cray X-MP
  ARCH_CRAY_Y_MP,      ///< Cray Y-MP
  ARCH_CRAY_C90,       ///< Cray C90
  ARCH_CRAY_T3D,       ///< Cray T3D
  ARCH_CRAY_T3E,       ///< Cray T3E
  ARCH_CRAY_T90,       ///< Cray T90

  // Embedded/Microcontrollers
  ARCH_AVR,            ///< Atmel AVR
  ARCH_AVR32,          ///< Atmel AVR32
  ARCH_MSP430,         ///< TI MSP430
  ARCH_8051,           ///< Intel 8051
  ARCH_6502,           ///< MOS 6502
  ARCH_6800,           ///< Motorola 6800
  ARCH_6809,           ///< Motorola 6809
  ARCH_Z80,            ///< Zilog Z80
  ARCH_Z8000,          ///< Zilog Z8000
  ARCH_PIC,            ///< Microchip PIC

  // Other modern architectures
  ARCH_ARC,            ///< Synopsys ARC
  ARCH_XTENSA,         ///< Tensilica Xtensa
  ARCH_OPENRISC,       ///< OpenRISC
  ARCH_MICROBLAZE,     ///< Xilinx MicroBlaze
  ARCH_NIOS,           ///< Altera Nios
  ARCH_NIOS2,          ///< Altera Nios II
  ARCH_BLACKFIN,       ///< Analog Devices Blackfin
  ARCH_C6X,            ///< TI C6x DSP
  ARCH_HEXAGON,        ///< Qualcomm Hexagon
  ARCH_EPIPHANY,       ///< Adapteva Epiphany
  ARCH_TILE,           ///< Tilera TILE
  ARCH_TILEGX,         ///< Tilera TILE-Gx
  ARCH_TILEPRO,        ///< Tilera TILEPro

  // Acorn/ARM Ltd historical
  ARCH_ARM2,           ///< Acorn ARM2
  ARCH_ARM3,           ///< Acorn ARM3

  // Historical minicomputers
  ARCH_NOVA,           ///< Data General Nova
  ARCH_ECLIPSE,        ///< Data General Eclipse

  // Game consoles
  ARCH_N64_VR4300,     ///< Nintendo 64 VR4300
  ARCH_GAMECUBE_GEKKO, ///< GameCube Gekko
  ARCH_WII_BROADWAY,   ///< Wii Broadway
  ARCH_PS2_EE,         ///< PlayStation 2 Emotion Engine
  ARCH_PS3_CELL,       ///< PlayStation 3 Cell

  // Transputer
  ARCH_TRANSPUTER,     ///< Inmos Transputer

  // Misc
  ARCH_UNICORE,        ///< PKUnity UniCore
  ARCH_ALPHA_MAX,      ///< For range checking
} ARCH;

VOID PlatformInit (VOID);
UINT64 PlatformGetMaxPageFrameNumber (VOID);
UINT64 PlatformGetMinRamPageFrameNumber (VOID);
UINT64 PlatformGetMaxRamPageFrameNumber (VOID);
UINT32 PlatformGetMemoryRegionCount (VOID);
BOOTINFO_REGION *PlatformGetMemoryRegion (IN UINT32 Index);
FRAMEBUFFER_DESC *PlatformGetFramebuffer (VOID);
APXH_PLATFORM_DESCRIPTOR *PlatformGetDescriptor (VOID);
VOID PlatformVerify (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID PlatformEntry (IN ARCH Architecture, IN VIRTUAL_ADDRESS PageTable, IN VIRTUAL_ADDRESS EntryPoint);

VOID *PayloadGet (IN UINT32 Index, OUT OPTIONAL UINTN *Size);

typedef enum _PAYLOAD_ID
{
  PayloadKernel,
  PayloadUser,
} PAYLOAD_ID;

VOID *GetPayloadStart (IN INT32 ArgumentCount, IN char *ArgumentVector[], IN PAYLOAD_ID PayloadId);
UINTN GetPayloadSize (IN PAYLOAD_ID PayloadId);

ARCH GetElfArch (IN VOID *ElfImage);
VIRTUAL_ADDRESS LoadElf32 (IN VOID *ElfImage, IN INT32 IsUserMode);
VIRTUAL_ADDRESS LoadElf64 (IN VOID *ElfImage, IN INT32 IsUserMode);

UINTN GetPage (VOID);
UINTN GetPayloadPage (VOID);

VOID VirtualAddressInit (VOID);
UINTN VirtualAddressGetPhysical (IN VIRTUAL_ADDRESS VirtualAddress);
VOID VirtualAddressVerify (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressPopulate (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID VirtualAddressCopy (IN VIRTUAL_ADDRESS VirtualAddress, IN VOID *SourceAddress, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID VirtualAddressMemset (IN VIRTUAL_ADDRESS VirtualAddress, IN INT32 FillChar, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID VirtualAddressMapPhysical (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN MEMORY_TYPE Type);
VOID VirtualAddressMapLinear (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressMapInfo (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressMapPageFrameNumbers (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressMapBatree (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressAllocateTopPageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressAllocatePageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressMapFramebuffer (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN MEMORY_TYPE Type);
VOID VirtualAddressMapRegions (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VirtualAddressMapKernelTls (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 InitializedSize, IN SIZE64 TotalSize);
VOID VirtualAddressMapUserTls (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 InitializedSize, IN SIZE64 TotalSize);
VOID VirtualAddressSetEntry (IN VIRTUAL_ADDRESS EntryPoint);

VOID PaeInit (VOID);
UINTN PaeGetPhysical (IN VIRTUAL_ADDRESS VirtualAddress);
VOID PaeVerify (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID PaePopulate (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID PaeMapPhysical (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN UINT64 PhysicalAddress, IN MEMORY_TYPE Type);
VOID PaeAllocatePageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID PaeAllocateTopPageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID PaeMapLinear (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID PaeEntry (IN VIRTUAL_ADDRESS EntryPoint);

/* Internal PAE functions. */
VOID PaeDirectMap (IN VOID *PageTable, IN UINT64 PhysicalAddress, IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size,
		    IN MEMORY_TYPE Type, IN INT32 IsPayload, IN INT32 IsExecutable);
VOID PaeMapPage (IN VOID *PageTable, IN VIRTUAL_ADDRESS VirtualAddress, IN UINTN PhysicalAddress, IN INT32 IsPayload, IN INT32 IsWritable,
		   IN INT32 IsExecutable);

VOID Pae64Init (VOID);
UINTN Pae64GetPhysical (IN VIRTUAL_ADDRESS VirtualAddress);
VOID Pae64Verify (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Pae64Populate (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID Pae64MapPhysical (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN UINT64 PhysicalAddress, IN MEMORY_TYPE Type);
VOID Pae64AllocatePageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Pae64AllocateTopPageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Pae64MapLinear (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Pae64Entry (IN VIRTUAL_ADDRESS EntryPoint);

/* Internal PAE64 functions. */
VOID Pae64DirectMap (IN VOID *PageTable, IN UINT64 PhysicalAddress, IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size,
		      IN MEMORY_TYPE Type, IN INT32 IsPayload, IN INT32 IsExecutable);
VOID Pae64MapPage (IN VOID *PageTable, IN VIRTUAL_ADDRESS VirtualAddress, IN UINTN PhysicalAddress, IN INT32 IsPayload, IN INT32 IsWritable,
		     IN INT32 IsExecutable);

VOID Sv48Init (VOID);
UINTN Sv48GetPhysical (IN VIRTUAL_ADDRESS VirtualAddress);
VOID Sv48Verify (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Sv48Populate (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID Sv48MapPhysical (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN UINT64 PhysicalAddress, IN MEMORY_TYPE Type);
VOID Sv48AllocatePageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Sv48AllocateTopPageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Sv48MapLinear (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Sv48Entry (IN VIRTUAL_ADDRESS EntryPoint);

/* Internal SV48 functions. */
VOID Sv48DirectMap (IN VOID *PageTable, IN UINT64 PhysicalAddress, IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size,
		     IN MEMORY_TYPE Type, IN INT32 IsPayload, IN INT32 IsExecutable);
VOID Sv48MapPage (IN VOID *PageTable, IN VIRTUAL_ADDRESS VirtualAddress, IN UINTN PhysicalAddress, IN INT32 IsPayload, IN INT32 IsWritable,
		    IN INT32 IsExecutable);


#define info(...) do { printf (__VA_ARGS__); putchar('\n'); } while (0)
#define debug(...) do { printf (__VA_ARGS__); putchar('\n'); } while (0)
#define warn(...) do { printf ("Warning: "); printf (__VA_ARGS__); putchar('\n'); } while (0)
#define fatal(...) do { printf ("Fatal: "); printf (__VA_ARGS__); putchar('\n'); exit(-1); } while (0)

