/** @file
  APXH Boot Protocol Definitions

  Defines data structures for the APXH (APX Header) boot protocol used
  to pass boot information from the bootloader to the NUX kernel.

  The APXH protocol provides:
  - Physical memory map (RAM regions, MMIO regions)
  - Platform firmware information (ACPI, Device Tree)
  - Framebuffer configuration
  - Thread-local storage (TLS) information
  - Initial S-tree allocator state

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __apxh_apxh_h__
#define __apxh_apxh_h__

#include <stdint.h>
#include <nux/framebuffer.h>

//
// Platform Type Constants
//

#define PLT_UNKNOWN  0  ///< Unknown platform type
#define PLT_ACPI     1  ///< ACPI-based platform (x86)
#define PLT_DTB      2  ///< Device Tree Blob platform (RISC-V, ARM)

//
// APXH Magic Numbers
//

#define APXH_BOOTINFO_MAGIC  0xAF10B007  ///< Boot info structure magic
#define APXH_STREE_MAGIC     0xAF1057EE  ///< S-tree allocator magic

//
// APXH S-tree Version
//

#define APXH_STREE_VERSION  0  ///< Current S-tree version

//
// Memory Region Types
//

#define APXH_REGION_UNKNOWN  0  ///< Unknown/reserved region
#define APXH_REGION_RAM      1  ///< Available RAM
#define APXH_REGION_MMIO     2  ///< Memory-mapped I/O
#define APXH_REGION_BSY      3  ///< Busy/in-use region

//
// APXH_PLATFORM_DESCRIPTOR - Platform Firmware Information
//

#pragma pack(push, 1)

typedef struct _APXH_PLATFORM_DESCRIPTOR {
  ///
  /// Platform type (PLT_UNKNOWN, PLT_ACPI, PLT_DTB).
  ///
  UINT64  Type;

  ///
  /// Pointer to platform-specific data:
  /// - For PLT_ACPI: Physical address of ACPI RSDP
  /// - For PLT_DTB: Physical address of Device Tree Blob
  ///
  UINT64  PlatformPointer;
} APXH_PLATFORM_DESCRIPTOR;

//
// APXH_TLS_INFO - Thread-Local Storage Information
//

typedef struct _APXH_TLS_INFO {
  ///
  /// Virtual address of initialized TLS data prototype.
  /// This data is copied to each thread's TLS area.
  ///
  UINT64  InitializedDataVaddr;

  ///
  /// Size of initialized TLS data in bytes.
  ///
  UINT64  InitializedDataSize;

  ///
  /// Total TLS size including BSS (uninitialized data).
  ///
  UINT64  TotalSize;
} APXH_TLS_INFO;

//
// APXH_BOOT_INFO - Main Boot Information Structure
//

typedef struct _APXH_BOOT_INFO {
  ///
  /// Magic number (APXH_BOOTINFO_MAGIC = 0xAF10B007).
  /// Used to validate the boot information structure.
  ///
  UINT64  Magic;

  ///
  /// Maximum RAM page frame number.
  /// All RAM pages have PFN <= MaxRamPfn.
  ///
  UINT64  MaxRamPfn;

  ///
  /// Maximum page frame number in physical address space.
  /// Includes RAM, MMIO, and all other regions.
  ///
  UINT64  MaxPfn;

  ///
  /// Number of memory regions in the memory map.
  ///
  UINT64  NumRegions;

  ///
  /// User-mode entry point virtual address.
  /// If non-zero, specifies the initial user-mode program entry point.
  ///
  UINT64  UserEntry;

  ///
  /// Framebuffer descriptor.
  /// Describes the linear framebuffer for console output.
  ///
  struct fbdesc  FramebufferDesc;

  ///
  /// Platform firmware descriptor.
  /// Contains ACPI or Device Tree information.
  ///
  APXH_PLATFORM_DESCRIPTOR  PlatformDesc;

  ///
  /// Kernel thread-local storage information.
  ///
  APXH_TLS_INFO  KernelTls;

  ///
  /// User-mode thread-local storage information.
  ///
  APXH_TLS_INFO  UserTls;
} APXH_BOOT_INFO;

//
// APXH_STREE - S-tree Allocator State
//

typedef struct _APXH_STREE {
  ///
  /// Magic number (APXH_STREE_MAGIC = 0xAF1057EE).
  /// Used to validate the S-tree structure.
  ///
  UINT64  Magic;

  ///
  /// S-tree version (APXH_STREE_VERSION = 0).
  ///
  UINT8   Version;

  ///
  /// S-tree order (power of 2 for tree height).
  ///
  UINT8   Order;

  ///
  /// Offset to S-tree data.
  ///
  UINT16  Offset;

  ///
  /// Size of S-tree data in bytes.
  ///
  UINT32  Size;
} APXH_STREE;

//
// APXH_REGION - Memory Region Descriptor
//

typedef struct _APXH_REGION {
  ///
  /// Region type (APXH_REGION_*).
  /// Stored in low 2 bits, PFN in upper 62 bits.
  ///
  UINT64  Type : 2;

  ///
  /// Starting page frame number.
  /// Upper 62 bits of the first UINT64.
  ///
  UINT64  Pfn : 62;

  ///
  /// Length of region in pages.
  ///
  UINT64  Length;
} APXH_REGION;

#pragma pack(pop)

//
// Legacy Type Aliases (for backward compatibility)
//

/** @deprecated Use APXH_PLATFORM_DESCRIPTOR instead **/
typedef APXH_PLATFORM_DESCRIPTOR apxh_pltdesc;

/** @deprecated Use APXH_TLS_INFO instead **/
typedef APXH_TLS_INFO apxh_tlsinfo;

/** @deprecated Use APXH_BOOT_INFO instead **/
typedef APXH_BOOT_INFO apxh_bootinfo;

/** @deprecated Use APXH_STREE instead **/
typedef APXH_STREE apxh_stree;

/** @deprecated Use APXH_REGION instead **/
typedef APXH_REGION apxh_region;

#endif // _APXH_H
