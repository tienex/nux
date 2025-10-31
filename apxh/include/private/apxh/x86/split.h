/** @file
  APXH x86 Address Space Split Configurations

  Defines various kernel/user address space split configurations for
  32-bit x86 systems. Different splits affect how the 4GB virtual
  address space is divided between kernel and user space.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <apxh/types.h>

//
// Address Space Split Types
//

/**
  Address space split enumeration.

  Defines how the 32-bit 4GB virtual address space is divided
  between kernel and user space.
**/
typedef enum _ADDR_SPACE_SPLIT {
  //
  // Standard splits (kernel/user)
  //
  AddrSplit_0_5_3_5 = 0,  ///< 0.5GB kernel / 3.5GB user (0x20000000)
  AddrSplit_1_3     = 1,  ///< 1GB kernel / 3GB user (0x40000000) - Linux default
  AddrSplit_2_2     = 2,  ///< 2GB kernel / 2GB user (0x80000000)
  AddrSplit_3_1     = 3,  ///< 3GB kernel / 1GB user (0xC0000000)
  AddrSplit_3_5_0_5 = 4,  ///< 3.5GB kernel / 0.5GB user (0xE0000000)

  //
  // Special splits
  //
  AddrSplit_4_4     = 5   ///< 4GB/4GB split (requires PAE page table tricks)
} ADDR_SPACE_SPLIT;

//
// Address Space Split Configuration Structure
//

/**
  Address space split configuration.

  Contains the virtual address boundaries and metadata for a
  given kernel/user split configuration.
**/
typedef struct _ADDR_SPACE_SPLIT_CONFIG {
  ADDR_SPACE_SPLIT  SplitType;        ///< Split type enumeration
  UINT64            KernelBase;       ///< Kernel base virtual address
  UINT64            UserLimit;        ///< User space limit (exclusive)
  UINT64            KernelSize;       ///< Kernel address space size in bytes
  UINT64            UserSize;         ///< User address space size in bytes
  CONST CHAR8       *Name;            ///< Human-readable name (e.g., "1/3 split")
  BOOLEAN           Requires4G4G;     ///< TRUE if requires 4GB/4GB tricks
} ADDR_SPACE_SPLIT_CONFIG;

//
// Pre-defined Split Configurations
//

/**
  0.5GB/3.5GB split configuration.
  Kernel: 0x00000000 - 0x1FFFFFFF (512 MB)
  User:   0x20000000 - 0xFFFFFFFF (3.5 GB)
**/
#define SPLIT_CONFIG_0_5_3_5 { \
  AddrSplit_0_5_3_5, \
  0x00000000ULL, \
  0x20000000ULL, \
  0x20000000ULL, \
  0xE0000000ULL, \
  "0.5GB/3.5GB", \
  FALSE \
}

/**
  1GB/3GB split configuration (Linux default).
  Kernel: 0x00000000 - 0x3FFFFFFF (1 GB)
  User:   0x40000000 - 0xFFFFFFFF (3 GB)
**/
#define SPLIT_CONFIG_1_3 { \
  AddrSplit_1_3, \
  0x00000000ULL, \
  0x40000000ULL, \
  0x40000000ULL, \
  0xC0000000ULL, \
  "1GB/3GB (Linux default)", \
  FALSE \
}

/**
  2GB/2GB split configuration.
  Kernel: 0x00000000 - 0x7FFFFFFF (2 GB)
  User:   0x80000000 - 0xFFFFFFFF (2 GB)
**/
#define SPLIT_CONFIG_2_2 { \
  AddrSplit_2_2, \
  0x00000000ULL, \
  0x80000000ULL, \
  0x80000000ULL, \
  0x80000000ULL, \
  "2GB/2GB", \
  FALSE \
}

/**
  3GB/1GB split configuration.
  Kernel: 0x00000000 - 0xBFFFFFFF (3 GB)
  User:   0xC0000000 - 0xFFFFFFFF (1 GB)
**/
#define SPLIT_CONFIG_3_1 { \
  AddrSplit_3_1, \
  0x00000000ULL, \
  0xC0000000ULL, \
  0xC0000000ULL, \
  0x40000000ULL, \
  "3GB/1GB", \
  FALSE \
}

/**
  3.5GB/0.5GB split configuration.
  Kernel: 0x00000000 - 0xDFFFFFFF (3.5 GB)
  User:   0xE0000000 - 0xFFFFFFFF (512 MB)
**/
#define SPLIT_CONFIG_3_5_0_5 { \
  AddrSplit_3_5_0_5, \
  0x00000000ULL, \
  0xE0000000ULL, \
  0xE0000000ULL, \
  0x20000000ULL, \
  "3.5GB/0.5GB", \
  FALSE \
}

/**
  4GB/4GB split configuration.

  This is a special configuration where both kernel and user space
  have access to the full 4GB address space. This requires:

  1. PAE (Physical Address Extension)
  2. Separate page table hierarchies for kernel and user
  3. Page table switching on kernel/user transitions

  Implementation:
  - User space: 0x00000000 - 0xFFFFFFFF (full 4GB)
  - Kernel space: 0x00000000 - 0xFFFFFFFF (full 4GB, different mapping)
  - Requires maintaining two sets of page tables
  - Context switch must reload CR3
**/
#define SPLIT_CONFIG_4_4 { \
  AddrSplit_4_4, \
  0x00000000ULL, \
  0x100000000ULL, \
  0x100000000ULL, \
  0x100000000ULL, \
  "4GB/4GB (PAE required)", \
  TRUE \
}

//
// Default Split Configuration
//

/**
  Default address space split (1GB/3GB).
  This is the most common configuration used by Linux and other systems.
**/
#define DEFAULT_ADDR_SPACE_SPLIT AddrSplit_1_3

//
// Function Prototypes
//

/**
  Get address space split configuration.

  Returns the configuration structure for a given split type.

  @param[in] SplitType  Split type to query.

  @return Pointer to split configuration structure, or NULL if invalid.
**/
CONST ADDR_SPACE_SPLIT_CONFIG *
GetAddressSpaceSplitConfig (
  IN ADDR_SPACE_SPLIT  SplitType
  );

/**
  Get current address space split.

  Returns the currently configured address space split.

  @return Current split type.
**/
ADDR_SPACE_SPLIT
GetCurrentAddressSpaceSplit (
  VOID
  );

/**
  Set address space split.

  Configures the address space split to use. Must be called before
  initializing paging.

  @param[in] SplitType  Split type to configure.

  @return S_OK on success, error code otherwise.
**/
HRESULT
SetAddressSpaceSplit (
  IN ADDR_SPACE_SPLIT  SplitType
  );

/**
  Check if address is in user space.

  @param[in] Address   Virtual address to check.
  @param[in] SplitType Split type to use.

  @retval TRUE   Address is in user space.
  @retval FALSE  Address is in kernel space.
**/
BOOLEAN
IsUserSpaceAddress (
  IN VIRTUAL_ADDRESS   Address,
  IN ADDR_SPACE_SPLIT  SplitType
  );

/**
  Check if address is in kernel space.

  @param[in] Address   Virtual address to check.
  @param[in] SplitType Split type to use.

  @retval TRUE   Address is in kernel space.
  @retval FALSE  Address is in user space.
**/
BOOLEAN
IsKernelSpaceAddress (
  IN VIRTUAL_ADDRESS   Address,
  IN ADDR_SPACE_SPLIT  SplitType
  );

/**
  Initialize address space split subsystem.

  Sets up default address space split configuration (1GB/3GB).
  Called during PAE initialization.
**/
VOID
AddressSpaceSplitInit (
  VOID
  );
