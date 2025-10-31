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
  APXH-specific program header types (ELF extension).

  Custom segment types for boot-time information that extend the
  standard ELF PT_* types for kernel loading requirements.
**/
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
  ApxhProgramHeaderLinear        = 0xAF100009   ///< Linear (recursive) page table map
} APXH_PROGRAM_HEADER_TYPE;

/**
  Payload Identifier
**/
typedef enum _PAYLOAD_ID
{
  PayloadKernel = 0,
  PayloadUser = 1
} PAYLOAD_ID;
