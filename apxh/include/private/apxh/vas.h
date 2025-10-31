/** @file
  APXH Virtual Address Space API

  Virtual address space management function declarations.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <apxh/types.h>

//
// Virtual Address Space Management
//

VOID VasInitialize (VOID);
UINTN VasGetPhysical (IN VIRTUAL_ADDRESS VirtualAddress);
VOID VasVerify (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VasPopulate (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID VasCopy (IN VIRTUAL_ADDRESS VirtualAddress, IN VOID *SourceAddress, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID VasFill (IN VIRTUAL_ADDRESS VirtualAddress, IN INT32 FillChar, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID VasMapPhysical (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN MEMORY_TYPE Type);
VOID VasMapLinear (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VasMapInfo (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VasMapPageFrameNumbers (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VasMapBatree (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VasAllocTopPageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VasAllocPageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VasMapFramebuffer (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN MEMORY_TYPE Type);
VOID VasMapRegions (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID VasMapKernelTls (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 InitializedSize, IN SIZE64 TotalSize);
VOID VasMapUserTls (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 InitializedSize, IN SIZE64 TotalSize);
VOID VasSetEntry (IN VIRTUAL_ADDRESS EntryPoint, IN ARCH KernelArch);
