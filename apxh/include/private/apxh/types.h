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
