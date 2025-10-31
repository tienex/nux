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

typedef enum _ARCH
{
  ArchInvalid,
  ArchUnsupported,

  // x86 family (Intel/AMD)
  Arch8086,           ///< Intel 8086
  Arch80186,          ///< Intel 80186
  Arch80286,          ///< Intel 80286
  Arch386,            ///< Intel 80386 and compatibles
  Arch486,            ///< Intel 80486 and compatibles
  ArchPentium,        ///< Intel Pentium
  ArchAmd64,          ///< AMD64/x86-64/EM64T
  ArchI860,           ///< Intel i860 RISC
  ArchI960,           ///< Intel i960 RISC

  // RISC-V
  ArchRiscV32,        ///< RISC-V 32-bit
  ArchRiscV64,        ///< RISC-V 64-bit
  ArchRiscV128,       ///< RISC-V 128-bit

  // ARM family (all variants)
  ArchArm,            ///< ARM (generic 32-bit)
  ArchArmV4,          ///< ARMv4
  ArchArmV4T,         ///< ARMv4T (Thumb)
  ArchArmV5,          ///< ARMv5
  ArchArmV5T,         ///< ARMv5T (Thumb)
  ArchArmV6,          ///< ARMv6
  ArchArmV7,          ///< ARMv7
  ArchArm64,          ///< ARM64/AArch64
  ArchThumb,          ///< ARM Thumb mode

  // PowerPC family
  ArchPpc32,          ///< PowerPC 32-bit
  ArchPpc64,          ///< PowerPC 64-bit
  ArchPpc601,         ///< PowerPC 601
  ArchPpc603,         ///< PowerPC 603
  ArchPpc604,         ///< PowerPC 604
  ArchPpc750,         ///< PowerPC 750 (G3)
  ArchPpc7400,        ///< PowerPC 7400 (G4)
  ArchPpc970,         ///< PowerPC 970 (G5)

  // MIPS family
  ArchMips32,         ///< MIPS 32-bit (generic)
  ArchMips64,         ///< MIPS 64-bit (generic)
  ArchMipsR3000,     ///< MIPS R3000
  ArchMipsR4000,     ///< MIPS R4000
  ArchMipsR5000,     ///< MIPS R5000
  ArchMipsR6000,     ///< MIPS R6000
  ArchMipsR8000,     ///< MIPS R8000
  ArchMipsR10000,    ///< MIPS R10000
  ArchMips16,         ///< MIPS16

  // Alpha (DEC/Compaq/HP)
  ArchAlpha,          ///< DEC Alpha (generic)
  ArchAlpha21064,    ///< Alpha 21064 (EV4)
  ArchAlpha21164,    ///< Alpha 21164 (EV5)
  ArchAlpha21264,    ///< Alpha 21264 (EV6)

  // PA-RISC (HP)
  ArchPaRisc,         ///< PA-RISC 32-bit
  ArchPaRisc64,       ///< PA-RISC 64-bit (PA-RISC 2.0)
  ArchPaRisc1_0,      ///< PA-RISC 1.0
  ArchPaRisc1_1,      ///< PA-RISC 1.1
  ArchPaRisc2_0,      ///< PA-RISC 2.0

  // SPARC (Sun/Oracle)
  ArchSparc32,        ///< SPARC 32-bit (V8)
  ArchSparc64,        ///< SPARC 64-bit (V9)
  ArchSparcV7,        ///< SPARC V7
  ArchSparcV8,        ///< SPARC V8
  ArchSparcV9,        ///< SPARC V9

  // Itanium (Intel)
  ArchIa64,           ///< Intel Itanium (IA-64)

  // Motorola 68k family
  ArchM68k,           ///< Motorola 68000 (generic)
  ArchM68000,         ///< Motorola 68000
  ArchM68010,         ///< Motorola 68010
  ArchM68020,         ///< Motorola 68020
  ArchM68030,         ///< Motorola 68030
  ArchM68040,         ///< Motorola 68040
  ArchM68060,         ///< Motorola 68060
  ArchColdFire,       ///< Motorola/Freescale ColdFire

  // Motorola 88k family
  ArchM88k,           ///< Motorola 88000 (generic)
  ArchM88100,         ///< Motorola 88100
  ArchM88110,         ///< Motorola 88110

  // LoongArch (Loongson)
  ArchLoongArch32,    ///< LoongArch 32-bit
  ArchLoongArch64,    ///< LoongArch 64-bit

  // VAX (DEC)
  ArchVax,            ///< DEC VAX

  // PDP (DEC)
  ArchPdp7,           ///< DEC PDP-7
  ArchPdp8,           ///< DEC PDP-8
  ArchPdp9,           ///< DEC PDP-9
  ArchPdp10,          ///< DEC PDP-10
  ArchPdp11,          ///< DEC PDP-11
  ArchPdp15,          ///< DEC PDP-15

  // IBM mainframe
  ArchS360,           ///< IBM System/360
  ArchS370,           ///< IBM System/370
  ArchS390,           ///< IBM S/390 (31-bit)
  ArchS390x,          ///< IBM z/Architecture (64-bit)

  // POWER (IBM)
  ArchPower,          ///< IBM POWER (generic)
  ArchPower1,         ///< IBM POWER1
  ArchPower2,         ///< IBM POWER2
  ArchPower3,         ///< IBM POWER3
  ArchPower4,         ///< IBM POWER4
  ArchPower5,         ///< IBM POWER5
  ArchPower6,         ///< IBM POWER6
  ArchPower7,         ///< IBM POWER7
  ArchPower8,         ///< IBM POWER8
  ArchPower9,         ///< IBM POWER9
  ArchPower10,        ///< IBM POWER10

  // SuperH (Hitachi/Renesas)
  ArchSh,             ///< SuperH (generic)
  ArchSh1,            ///< SuperH SH-1
  ArchSh2,            ///< SuperH SH-2
  ArchSh3,            ///< SuperH SH-3
  ArchSh4,            ///< SuperH SH-4
  ArchSh5,            ///< SuperH SH-5

  // AT&T/Bellmac/WE
  ArchWe32000,        ///< AT&T WE32000 (3B series)
  ArchWe32100,        ///< AT&T WE32100
  ArchWe32200,        ///< AT&T WE32200

  // National Semiconductor
  ArchNs32016,        ///< NS32016
  ArchNs32032,        ///< NS32032
  ArchNs32332,        ///< NS32332
  ArchNs32532,        ///< NS32532

  // Clipper (Fairchild/Intergraph)
  ArchClipper,        ///< Clipper (generic)
  ArchClipperC100,   ///< Clipper C100
  ArchClipperC300,   ///< Clipper C300
  ArchClipperC400,   ///< Clipper C400

  // ROMP (IBM RT PC)
  ArchRomp,           ///< IBM ROMP

  // AMD 29000
  ArchAm29000,        ///< AMD Am29000

  // Pyramid
  ArchPyramid,        ///< Pyramid 90x

  // Convex
  ArchConvex,         ///< Convex

  // Cray
  ArchCray1,          ///< Cray-1
  ArchCray2,          ///< Cray-2
  ArchCrayXMp,      ///< Cray X-MP
  ArchCrayYMp,      ///< Cray Y-MP
  ArchCrayC90,       ///< Cray C90
  ArchCrayT3D,       ///< Cray T3D
  ArchCrayT3E,       ///< Cray T3E
  ArchCrayT90,       ///< Cray T90

  // Embedded/Microcontrollers
  ArchAvr,            ///< Atmel AVR
  ArchAvr32,          ///< Atmel AVR32
  ArchMsp430,         ///< TI MSP430
  Arch8051,           ///< Intel 8051
  Arch6502,           ///< MOS 6502
  Arch6800,           ///< Motorola 6800
  Arch6809,           ///< Motorola 6809
  ArchZ80,            ///< Zilog Z80
  ArchZ8000,          ///< Zilog Z8000
  ArchPic,            ///< Microchip PIC

  // Other modern architectures
  ArchArc,            ///< Synopsys ARC
  ArchXtensa,         ///< Tensilica Xtensa
  ArchOpenRisc,       ///< OpenRISC
  ArchMicroBlaze,     ///< Xilinx MicroBlaze
  ArchNios,           ///< Altera Nios
  ArchNios2,          ///< Altera Nios II
  ArchBlackfin,       ///< Analog Devices Blackfin
  ArchC6x,            ///< TI C6x DSP
  ArchHexagon,        ///< Qualcomm Hexagon
  ArchEpiphany,       ///< Adapteva Epiphany
  ArchTile,           ///< Tilera TILE
  ArchTileGx,         ///< Tilera TILE-Gx
  ArchTilePro,        ///< Tilera TILEPro

  // Acorn/ARM Ltd historical
  ArchArm2,           ///< Acorn ARM2
  ArchArm3,           ///< Acorn ARM3

  // Historical minicomputers
  ArchNova,           ///< Data General Nova
  ArchEclipse,        ///< Data General Eclipse

  // Game consoles
  ArchN64Vr4300,     ///< Nintendo 64 VR4300
  ArchGameCubeGekko, ///< GameCube Gekko
  ArchWiiBroadway,   ///< Wii Broadway
  ArchPs2Ee,         ///< PlayStation 2 Emotion Engine
  ArchPs3Cell,       ///< PlayStation 3 Cell

  // Transputer
  ArchTransputer,     ///< Inmos Transputer

  // Misc
  ArchUniCore,        ///< PKUnity UniCore
  ArchAlphaMax,      ///< For range checking
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

