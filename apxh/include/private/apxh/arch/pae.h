/** @file
  APXH PAE (x86 32-bit) Architecture Support

  Function declarations for x86 32-bit with PAE paging.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <apxh/types.h>

#if ANX_ARCH_X86 || ANX_ARCH_X86_64

//
// Low-level PAE Functions (used by platform code)
//

VOID PaeDirectMap (IN VOID *PageTable, IN UINT64 PhysicalAddress, IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size,
                   IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable, IN MEMORY_TYPE Type);
VOID PaeMapPage (IN VOID *PageTable, IN VIRTUAL_ADDRESS VirtualAddress, IN UINTN PhysicalAddress, IN INT32 IsPayload, IN INT32 IsWritable,
                 IN INT32 IsExecutable, IN INT32 IsUserMode);

#endif // ANX_ARCH_X86 || ANX_ARCH_X86_64
