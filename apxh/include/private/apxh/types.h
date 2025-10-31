/** @file
  APXH Common Types

  Common type definitions used throughout APXH bootloader.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <ananke/ananke.h>

//
// Basic Types
//

typedef INT64 SSIZE64;
typedef UINT64 SIZE64;
typedef UINT64 VIRTUAL_ADDRESS;

//
// Architecture Types
//

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
  MEMTYPE_WC = 0,  ///< Write Combining
  MEMTYPE_WB = 1,  ///< Write Back (cacheable)
  MEMTYPE_UC = 2   ///< Uncached
} MEMORY_TYPE;

// Legacy aliases for compatibility
#define MemTypeWriteCombining MEMTYPE_WC
#define MemTypeWriteBack      MEMTYPE_WB
#define MemTypeUncached       MEMTYPE_UC

/**
  Generic segment type abstraction for multi-format image loading.

  Maps format-specific segment types (ELF program headers, PE sections,
  Mach-O segments) to a common representation for bootloader operations.
**/
typedef enum _SEGMENT_TYPE {
  SegmentNull           = 0,   ///< Unused/ignored segment
  SegmentLoad           = 1,   ///< Loadable code/data segment
  SegmentTls            = 2,   ///< Thread-local storage
  SegmentDynamic        = 3,   ///< Dynamic linking information
  SegmentInfo           = 4,   ///< Boot information page (APXH)
  SegmentEmpty          = 5,   ///< Empty VA allocation (APXH)
  SegmentPhysicalMap    = 6,   ///< 1:1 physical memory mapping (APXH)
  SegmentPfnMap         = 7,   ///< Page frame number map (APXH)
  SegmentBatree         = 8,   ///< Allocated pages bitmap (APXH)
  SegmentPageTableAlloc = 9,   ///< Page table allocation (APXH)
  SegmentTopPageTableAlloc = 10, ///< Top-level PT allocation (APXH)
  SegmentFramebuffer    = 11,  ///< Framebuffer mapping (APXH)
  SegmentRegions        = 12,  ///< Region list (APXH)
  SegmentLinear         = 13,  ///< Linear/recursive page table mapping (APXH)
  SegmentUniversalResource = 14 ///< APXH Universal Resource (AUR) fork (APXH)
} SEGMENT_TYPE;

/**
  Payload Identifier
**/
typedef enum _PAYLOAD_ID
{
  PayloadKernel = 0,
  PayloadUser = 1
} PAYLOAD_ID;

/**
  CPU Architecture Identifier

  Identifies the target CPU architecture for executable images.
  Used by image loaders to determine which architecture handler to use.
**/
typedef enum _ARCH {
  ArchInvalid = 0,        ///< Invalid/unrecognized architecture
  ArchUnsupported = 1,    ///< Recognized but unsupported architecture

  // x86 family
  Arch386 = 2,            ///< Intel 80386 (32-bit x86)
  ArchX86 = Arch386,      ///< Alias for 386
  ArchAmd64 = 3,          ///< AMD64/x86-64 (64-bit x86)
  ArchX86_64 = ArchAmd64, ///< Alias for AMD64

  // ARM family
  ArchArm = 4,            ///< ARM 32-bit (generic)
  ArchArm32 = ArchArm,    ///< Alias for ARM 32-bit
  ArchArm64 = 5,          ///< ARM 64-bit (AArch64)
  ArchThumb = 6,          ///< ARM Thumb mode

  // PowerPC family
  ArchPpc32 = 7,          ///< PowerPC 32-bit
  ArchPpc64 = 8,          ///< PowerPC 64-bit

  // MIPS family
  ArchMips32 = 9,         ///< MIPS 32-bit
  ArchMips64 = 10,        ///< MIPS 64-bit
  ArchMipsR3000 = 11,     ///< MIPS R3000
  ArchMipsR4000 = 12,     ///< MIPS R4000
  ArchMipsR10000 = 13,    ///< MIPS R10000

  // RISC-V family
  ArchRiscV32 = 14,       ///< RISC-V 32-bit
  ArchRiscV64 = 15,       ///< RISC-V 64-bit
  ArchRiscV128 = 16,      ///< RISC-V 128-bit

  // Motorola 68000 family
  ArchM68k = 17,          ///< Motorola 68000

  // HP PA-RISC
  ArchPaRisc = 18,        ///< HP PA-RISC 32-bit
  ArchPaRisc64 = 19,      ///< HP PA-RISC 64-bit

  // DEC architectures
  ArchAlpha = 20,         ///< DEC Alpha
  ArchVax = 21,           ///< DEC VAX
  ArchPdp10 = 22,         ///< DEC PDP-10
  ArchPdp11 = 23,         ///< DEC PDP-11

  // Intel Itanium
  ArchIa64 = 24,          ///< Intel Itanium (IA-64)

  // SPARC
  ArchSparc = 25,         ///< SPARC

  // Zilog
  ArchZ80 = 26,           ///< Zilog Z80
  ArchZ8000 = 27,         ///< Zilog Z8000

  // SuperH
  ArchSh = 28,            ///< SuperH (generic)
  ArchSh3 = 29,           ///< SuperH SH-3
  ArchSh4 = 30,           ///< SuperH SH-4
  ArchSh5 = 31,           ///< SuperH SH-5

  // National Semiconductor
  ArchNs32k = 32,         ///< NS32000

  // Motorola 88000
  ArchM88k = 33,          ///< Motorola 88000

  // Intel i860
  ArchI860 = 34,          ///< Intel i860

  // AMD Am29000
  ArchAm29000 = 35,       ///< AMD Am29000

  // LoongArch
  ArchLoongArch32 = 36,   ///< LoongArch 32-bit
  ArchLoongArch64 = 37,   ///< LoongArch 64-bit

  // IBM s390x
  ArchS390x = 38,         ///< IBM s390x (64-bit)
  ArchSparc64 = 39,       ///< SPARC 64-bit

  // Hybrid/Compatibility architectures (32-bit on 64-bit hardware)
  ArchAmd64_32 = 40,      ///< x32 ABI (32-bit pointers on x86-64)
  ArchArm64_32 = 41,      ///< ILP32 (32-bit pointers on AArch64)
  ArchLoongArch64_32 = 42, ///< LA32 on LA64 (32-bit mode on LoongArch64)
  ArchMips64_32 = 43,     ///< n32 ABI (32-bit pointers on MIPS64)
  ArchRiscV64_32 = 44,    ///< ILP32 (32-bit pointers on RISC-V 64)
  ArchAlpha32 = 45,       ///< 32-bit mode on Alpha
  ArchPaRisc64_32 = 46,   ///< 32-bit mode on PA-RISC 64
  ArchIa64_32 = 47,       ///< 32-bit compatibility on Itanium
  ArchPpc64_32 = 48,      ///< 32-bit mode on PowerPC 64
  ArchS390x_32 = 49,      ///< 32-bit mode on s390x
  ArchSparc64_32 = 50,    ///< 32-bit mode on SPARC 64

  // Pseudo-endian variants (software emulated byte order)
  ArchM68kPseudoLe = 51,  ///< M68K in pseudo little-endian mode
  Arch386PseudoBe = 52,   ///< x86 in pseudo big-endian mode
  ArchAmd64PseudoBe = 53, ///< x86-64 in pseudo big-endian mode

  // Educational and specialized architectures
  ArchMmix = 54,          ///< Donald Knuth's MMIX (64-bit RISC)
  ArchDlx = 55,           ///< DLX (educational MIPS-like RISC)
  ArchMoxie = 56          ///< Moxie (lightweight RISC)
} ARCH;
