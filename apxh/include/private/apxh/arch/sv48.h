/** @file
  APXH Sv48 (RISC-V 64-bit) Architecture Support

  Function declarations for RISC-V 64-bit with Sv48 paging.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <apxh/types.h>

#if ANX_ARCH_RISCV

//
// Low-level Sv48 Functions (used by platform code)
//

VOID Sv48DirectMap (IN VOID *PageTable, IN UINT64 PhysicalAddress, IN VIRTUAL_ADDRESS VirtualAddress, IN SIZE64 Size,
                    IN INT32 IsUserMode, IN INT32 IsWritable, IN INT32 IsExecutable, IN MEMORY_TYPE Type);
VOID Sv48MapPage (IN VOID *PageTable, IN VIRTUAL_ADDRESS VirtualAddress, IN UINTN PhysicalAddress, IN INT32 IsPayload, IN INT32 IsWritable,
                  IN INT32 IsExecutable, IN INT32 IsUserMode);

#endif // ANX_ARCH_RISCV
