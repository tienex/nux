/** @file
  APXH Boot Protocol Definitions

  Defines data structures for the APXH (APX Header) boot protocol used
  to pass boot information from the bootloader to the NUX kernel.

  The APXH protocol provides:
  - Physical memory map (RAM regions, MMIO regions)
  - Platform firmware information (ACPI, Device Tree)
  - Framebuffer configuration
  - Thread-local storage (TLS) information
  - Initial BAtree allocator state

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/


#include <ananke/ananke.h>
#include <nux/framebuffer.h>

//
// Platform Type Constants
//

typedef enum _APXH_PLATFORM_TYPE {
  ApxhPlatformUnknown = 0,  ///< Unknown platform type
  ApxhPlatformAcpi    = 1,  ///< ACPI-based platform (x86)
  ApxhPlatformDtb     = 2,  ///< Device Tree Blob platform (RISC-V, ARM)
  ApxhPlatformMps     = 3   ///< Intel MultiProcessor Specification (legacy x86 SMP)
} APXH_PLATFORM_TYPE;

//
// APXH Magic Numbers
//

#define APXH_BOOTINFO_MAGIC  0xAF10B007  ///< Boot info structure magic
#define APXH_BATREE_MAGIC     0xAF1057EE  ///< BAtree allocator magic

//
// APXH BAtree Version
//

#define APXH_BATREE_VERSION  0  ///< Current BAtree version

//
// Memory Region Types
//

typedef enum _APXH_REGION_TYPE {
  ApxhRegionUnknown = 0,  ///< Unknown/reserved region
  ApxhRegionRam     = 1,  ///< Available RAM
  ApxhRegionMmio    = 2,  ///< Memory-mapped I/O
  ApxhRegionBusy    = 3   ///< Busy/in-use region
} APXH_REGION_TYPE;

//
// APXH_PLATFORM_DESCRIPTOR - Platform Firmware Information
//

ANX_PACK_PUSH(1)

typedef struct _APXH_PLATFORM_DESCRIPTOR {
  ///
  /// Platform type (ApxhPlatformUnknown, ApxhPlatformAcpi, ApxhPlatformDtb, ApxhPlatformMps).
  ///
  UINT64  Type;

  ///
  /// Pointer to platform-specific data:
  /// - For ApxhPlatformAcpi: Physical address of ACPI RSDP
  /// - For ApxhPlatformDtb: Physical address of Device Tree Blob
  /// - For ApxhPlatformMps: Physical address of MP Floating Pointer Structure
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

// Mixed-mode execution flags
#define APXH_MIXEDMODE_32ON64     (1 << 0)  ///< 32-bit kernel on 64-bit CPU
#define APXH_MIXEDMODE_64UON32K   (1 << 1)  ///< 64-bit user on 32-bit kernel
#define APXH_MIXEDMODE_32UON64K   (1 << 2)  ///< 32-bit user on 64-bit kernel
#define APXH_MIXEDMODE_MIXED      (1 << 3)  ///< General mixed-mode (different bitness)

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
  FRAMEBUFFER_DESC  FramebufferDesc;

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

  ///
  /// Kernel architecture.
  /// Specifies the instruction set architecture of the kernel.
  ///
  UINT32  KernelArchitecture;

  ///
  /// User architecture.
  /// Specifies the instruction set architecture of user-space.
  /// May be ArchInvalid (0) if no user-space program is loaded.
  ///
  UINT32  UserArchitecture;

  ///
  /// Host CPU architecture.
  /// Specifies the native instruction set of the physical CPU.
  ///
  UINT32  HostArchitecture;

  ///
  /// Mixed-mode execution flags.
  /// Bit 0: 32-bit kernel on 64-bit CPU (32-on-64)
  /// Bit 1: 64-bit user on 32-bit kernel (64U-on-32K)
  /// Bit 2: 32-bit user on 64-bit kernel (32U-on-64K)
  /// Bit 3: General mixed-mode (kernel and user have different bitness)
  ///
  UINT32  MixedModeFlags;
} APXH_BOOT_INFO;

//
// APXH_BATREE - BAtree Allocator State
//

typedef struct _APXH_BATREE {
  ///
  /// Magic number (APXH_BATREE_MAGIC = 0xAF1057EE).
  /// Used to validate the BAtree structure.
  ///
  UINT64  Magic;

  ///
  /// BAtree version (APXH_BATREE_VERSION = 0).
  ///
  UINT8   Version;

  ///
  /// BAtree order (power of 2 for tree height).
  ///
  UINT8   Order;

  ///
  /// Offset to BAtree data.
  ///
  UINT16  Offset;

  ///
  /// Size of BAtree data in bytes.
  ///
  UINT32  Size;
} APXH_BATREE;

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

ANX_PACK_POP()

