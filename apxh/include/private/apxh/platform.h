/** @file
  APXH Platform API

  Platform abstraction layer function declarations.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <apxh/types.h>
#include <nux/framebuffer.h>
#include <apxh/apxh.h>

//
// Platform Initialization and Information
//

VOID PlatformInit (VOID);
APXH_PLATFORM_DESCRIPTOR *PlatformGetDescriptor (VOID);
FRAMEBUFFER_DESC *PlatformGetFramebuffer (VOID);

//
// Memory Region Management
//

UINT32 PlatformGetMemoryRegionCount (VOID);
BOOTINFO_REGION *PlatformGetMemoryRegion (IN UINT32 Index);
UINT32 PlatformGetMaxPageFrameNumber (VOID);
UINT32 PlatformGetMaxRamPageFrameNumber (VOID);
UINT32 PlatformGetMinRamPageFrameNumber (VOID);

//
// Virtual Address Verification
//

VOID PlatformVerify (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);

//
// Payload Management
//

VOID *GetPayloadStart (IN INT32 ArgumentCount, IN char *ArgumentVector[], IN PAYLOAD_ID PayloadId);
UINTN GetPayloadSize (IN PAYLOAD_ID PayloadId);

//
// Memory Allocation
//

UINTN GetPage (VOID);
UINTN GetPayloadPage (VOID);
