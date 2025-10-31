/** @file
  APXH PAE64 (x86-64) Architecture Support

  Function declarations for x86-64 with 4-level paging.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <apxh/types.h>

#if EC_MACHINE_I386 || EC_MACHINE_AMD64

//
// PAE64 Initialization and Management
//

VOID Pae64Initialize (VOID);
UINTN Pae64GetPhysical (IN VIRTUAL_ADDRESS VirtualAddress);
VOID Pae64Verify (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Pae64Populate (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable);
VOID Pae64MapPhysical (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size, IN UINT64 PhysicalAddress, IN MEMORY_TYPE Type);
VOID Pae64AllocatePageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Pae64AllocateTopPageTable (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Pae64MapLinear (IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size);
VOID Pae64Entry (IN VIRTUAL_ADDRESS EntryPoint);

//
// Low-level PAE64 Functions
//

VOID Pae64DirectMap (IN VOID *PageTable, IN UINT64 PhysicalAddress, IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size,
                     IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable, IN MEMORY_TYPE Type);
VOID Pae64MapPage (IN VOID *PageTable, IN VIRTUAL_ADDRESS VirtualAddress, IN UINTN PhysicalAddress, IN INT32 IsPayload, IN INT32 IsWritable,
                   IN INT32 IsExecutable, IN INT32 IsUserMode);

#endif // EC_MACHINE_I386 || EC_MACHINE_AMD64
