/** @file
  APXH x86 Address Space Split Implementation

  Implements address space split configuration and management for
  32-bit x86 systems. Supports various kernel/user splits including
  the special 4GB/4GB configuration.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <apxh/internal.h>
#include <apxh/x86/split.h>

//
// Static Split Configuration Table
//

static CONST ADDR_SPACE_SPLIT_CONFIG gSplitConfigs[] = {
  SPLIT_CONFIG_0_5_3_5,
  SPLIT_CONFIG_1_3,
  SPLIT_CONFIG_2_2,
  SPLIT_CONFIG_3_1,
  SPLIT_CONFIG_3_5_0_5,
  SPLIT_CONFIG_4_4
};

//
// Current Split Configuration
//

static ADDR_SPACE_SPLIT gCurrentSplit = DEFAULT_ADDR_SPACE_SPLIT;
static BOOLEAN gSplitInitialized = FALSE;

/**
  Get address space split configuration.

  Returns the configuration structure for a given split type.

  @param[in] SplitType  Split type to query.

  @return Pointer to split configuration structure, or NULL if invalid.
**/
CONST ADDR_SPACE_SPLIT_CONFIG *
GetAddressSpaceSplitConfig (
  IN ADDR_SPACE_SPLIT  SplitType
  )
{
  if (SplitType < 0 || SplitType > AddrSplit_4_4) {
    return NULL;
  }

  return &gSplitConfigs[SplitType];
}

/**
  Get current address space split.

  Returns the currently configured address space split.

  @return Current split type.
**/
ADDR_SPACE_SPLIT
GetCurrentAddressSpaceSplit (
  VOID
  )
{
  return gCurrentSplit;
}

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
  )
{
  CONST ADDR_SPACE_SPLIT_CONFIG *Config;

  // Validate split type
  if (SplitType < 0 || SplitType > AddrSplit_4_4) {
    warn("Invalid address space split type: %d", SplitType);
    return E_INVALIDARG;
  }

  // Get configuration
  Config = GetAddressSpaceSplitConfig(SplitType);
  if (Config == NULL) {
    warn("Failed to get split configuration");
    return E_FAIL;
  }

  // Check if PAE is required for 4GB/4GB split
  if (Config->Requires4G4G) {
    if (!CpuSupportsPae()) {
      warn("4GB/4GB split requires PAE, which is not supported by this CPU");
      return E_NOTIMPL;
    }
    info("4GB/4GB split enabled (requires separate kernel/user page tables)");
  }

  // Warn if changing split after initialization
  if (gSplitInitialized) {
    warn("Changing address space split after paging initialization may cause issues");
  }

  gCurrentSplit = SplitType;
  gSplitInitialized = TRUE;

  info("Address space split configured: %s", Config->Name);
  info("  Kernel: 0x%08llx - 0x%08llx (%llu MB)",
       Config->KernelBase,
       Config->KernelBase + Config->KernelSize - 1,
       Config->KernelSize / (1024 * 1024));
  info("  User:   0x%08llx - 0x%08llx (%llu MB)",
       Config->UserLimit,
       Config->Requires4G4G ? 0xFFFFFFFFULL : (Config->UserLimit + Config->UserSize - 1),
       Config->UserSize / (1024 * 1024));

  return S_OK;
}

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
  )
{
  CONST ADDR_SPACE_SPLIT_CONFIG *Config;

  Config = GetAddressSpaceSplitConfig(SplitType);
  if (Config == NULL) {
    return FALSE;
  }

  // For 4GB/4GB split, context determines user vs kernel
  if (Config->Requires4G4G) {
    // In 4GB/4GB mode, both spaces map full 4GB
    // Actual determination requires page table context
    return TRUE;  // Assume user space by default
  }

  // Standard split: check against user limit
  return (Address >= Config->UserLimit);
}

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
  )
{
  CONST ADDR_SPACE_SPLIT_CONFIG *Config;

  Config = GetAddressSpaceSplitConfig(SplitType);
  if (Config == NULL) {
    return FALSE;
  }

  // For 4GB/4GB split, context determines user vs kernel
  if (Config->Requires4G4G) {
    // In 4GB/4GB mode, both spaces map full 4GB
    // Actual determination requires page table context
    return FALSE;  // Assume user space by default
  }

  // Standard split: check against kernel base and user limit
  return (Address >= Config->KernelBase && Address < Config->UserLimit);
}

/**
  Initialize address space split subsystem.

  Sets up default address space split configuration.
**/
VOID
AddressSpaceSplitInit (
  VOID
  )
{
  // Set default split (1GB/3GB)
  SetAddressSpaceSplit(DEFAULT_ADDR_SPACE_SPLIT);
}
