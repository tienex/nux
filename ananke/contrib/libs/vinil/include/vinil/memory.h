/** @file
  VINIL Memory Management

  Pool-based memory allocator COM interface for IL programs.

  Copyright (C) 2003-2007 Hans-Martin Will.
  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#ifndef VINIL_MEMORY_H
#define VINIL_MEMORY_H 1

#include <vinil/base.h>
#include <ananke/com.h>
#include <setjmp.h>

//
// Constants
//

#define VINIL_DEFAULT_PAGE_SIZE     8192

//
// GUID
//

ANX_DEFINE_GUID(IID_IVinilMemoryPool, 0x34567890, 0x3456, 0x3456, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF, 0x12);

//
// IVinilMemoryPool Interface
//

ANX_BEGIN_INTERFACE(IVinilMemoryPool, IUnknown, IID_IVinilMemoryPool, "34567890-3456-3456-3456-7890ABCDEF12")
    /**
      Allocate memory from the pool.

      @param[in]  Size   Number of bytes to allocate.
      @param[out] Memory Pointer to allocated memory.

      @retval  S_OK           Success.
      @retval  E_OUTOFMEMORY  Allocation failed.
    **/
    ANX_IFACE_METHOD(HRESULT, Allocate, (UINTN Size, VOID **Memory))

    /**
      Clear all allocations and reset to initial state.

      @retval  S_OK  Success.
    **/
    ANX_IFACE_METHOD(HRESULT, Clear, (VOID))

    /**
      Set allocation error handler.

      @param[in]  Handler  Jump buffer for allocation failures.

      @retval  S_OK  Success.
    **/
    ANX_IFACE_METHOD(HRESULT, SetHandler, (jmp_buf *Handler))
ANX_END_INTERFACE(IVinilMemoryPool)

//
// Factory Function
//

/**
  Create a new memory pool.

  @param[in]  DefaultPageSize  Default size for memory pages.
  @param[out] MemoryPool       Created memory pool interface.

  @retval  S_OK           Success.
  @retval  E_POINTER      Invalid pointer.
  @retval  E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
VinilCreateMemoryPool (
    UINTN               DefaultPageSize,
    IVinilMemoryPool    **MemoryPool
    );

#endif /* VINIL_MEMORY_H */
