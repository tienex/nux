/** @file
  APXH Architecture Abstraction

  Provides COM-based architecture abstraction layer for virtual
  address space management across different CPU architectures.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <apxh/types.h>

//
// Forward declarations
//

typedef struct _IVirtualAddressSpace IVirtualAddressSpace;

//
// Architecture Interface GUID
// {B8F3C4D2-9E1A-4F7B-8C2D-3E5F9A6D8B7C}
//

#define ANX_IID_IVirtualAddressSpace "B8F3C4D2-9E1A-4F7B-8C2D-3E5F9A6D8B7C"
ANX_DEFINE_GUID(IID_IVirtualAddressSpace, 0xB8F3C4D2,0x9E1A,0x4F7B,0x8C,0x2D,0x3E,0x5F,0x9A,0x6D,0x8B,0x7C);

//
// Architecture Auto-Registration Support
//

typedef struct _ARCHITECTURE_REGISTRATION {
  IVirtualAddressSpace  *Architecture;
  ARCH           ArchType;
} ARCHITECTURE_REGISTRATION;

#define APXH_REGISTER_ARCH(ArchVar, ArchTypeValue) \
  ANX_ATTR_SECTION(".architectures") ANX_ATTR_USED \
  static CONST ARCHITECTURE_REGISTRATION _arch_reg_##ArchVar = { \
    &ArchVar, \
    ArchTypeValue \
  }

//
// Architecture Interface (COM-style with ANX macros)
//

ANX_BEGIN_INTERFACE(IVirtualAddressSpace, IUnknown, IID_IVirtualAddressSpace, ANX_IID_IVirtualAddressSpace)
  /**
    Initialize architecture-specific paging structures.

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, Initialize, (
    IN IVirtualAddressSpace  *This
    ))

  /**
    Get physical address from virtual address.

    @param[in]  VirtualAddress  Virtual address to translate.
    @param[out] PhysicalAddress Physical address result.

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, GetPhysical, (
    IN  IVirtualAddressSpace      *This,
    IN  VIRTUAL_ADDRESS    VirtualAddress,
    OUT UINTN              *PhysicalAddress
    ))

  /**
    Verify virtual address range is valid for this architecture.

    @param[in] VirtualAddress Starting virtual address.
    @param[in] Size           Size of region.

    @return S_OK if valid, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, Verify, (
    IN IVirtualAddressSpace     *This,
    IN VIRTUAL_ADDRESS   VirtualAddress,
    IN SIZE64            Size
    ))

  /**
    Populate virtual address range with page mappings.

    @param[in] VirtualAddress Starting virtual address.
    @param[in] Size           Size of region.
    @param[in] IsUserMode     TRUE for user-accessible.
    @param[in] IsWritable     TRUE for writable.
    @param[in] IsExecutable   TRUE for executable.

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, Populate, (
    IN IVirtualAddressSpace     *This,
    IN VIRTUAL_ADDRESS   VirtualAddress,
    IN SIZE64            Size,
    IN INT32             IsUserMode,
    IN INT32             IsWritable,
    IN INT32             IsExecutable
    ))

  /**
    Map physical memory at virtual address.

    @param[in] VirtualAddress  Starting virtual address.
    @param[in] Size            Size of region.
    @param[in] PhysicalAddress Starting physical address.
    @param[in] Type            Memory type (WC, WB, UC).

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, MapPhysical, (
    IN IVirtualAddressSpace     *This,
    IN VIRTUAL_ADDRESS   VirtualAddress,
    IN SIZE64            Size,
    IN UINT64            PhysicalAddress,
    IN MEMORY_TYPE       Type
    ))

  /**
    Allocate page table structures.

    @param[in] VirtualAddress Starting virtual address.
    @param[in] Size           Size of region.

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, AllocatePageTable, (
    IN IVirtualAddressSpace     *This,
    IN VIRTUAL_ADDRESS   VirtualAddress,
    IN SIZE64            Size
    ))

  /**
    Allocate top-level page table structures.

    @param[in] VirtualAddress Starting virtual address.
    @param[in] Size           Size of region.

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, AllocateTopPageTable, (
    IN IVirtualAddressSpace     *This,
    IN VIRTUAL_ADDRESS   VirtualAddress,
    IN SIZE64            Size
    ))

  /**
    Set up linear (recursive) page table mapping.

    @param[in] VirtualAddress Starting virtual address.
    @param[in] Size           Size of region.

    @return S_OK on success, error code otherwise.
  **/
  ANX_IFACE_METHOD(HRESULT, MapLinear, (
    IN IVirtualAddressSpace     *This,
    IN VIRTUAL_ADDRESS   VirtualAddress,
    IN SIZE64            Size
    ))

  /**
    Transfer control to kernel entry point.

    @param[in] EntryPoint Kernel entry point address.

    @return Does not return.
  **/
  ANX_IFACE_METHOD(HRESULT, Entry, (
    IN IVirtualAddressSpace     *This,
    IN VIRTUAL_ADDRESS   EntryPoint
    ))

ANX_END_INTERFACE(IVirtualAddressSpace)

//
// Architecture Management
//

/**
  Register an architecture handler.

  @param[in] Architecture  Pointer to architecture instance.

  @return S_OK on success, error code otherwise.
**/
HRESULT
ArchitectureRegister (
  IN IVirtualAddressSpace  *Architecture
  );

/**
  Get architecture handler for specified architecture.

  @param[in] Arch  Architecture type.

  @return Pointer to architecture instance, or NULL if not found.
**/
IVirtualAddressSpace *
ArchitectureGet (
  IN ARCH  Arch
  );

/**
  Get architecture name string.

  @param[in] Arch  Architecture type.

  @return Pointer to architecture name string.
**/
CONST CHAR *
ArchitectureGetName (
  IN ARCH  Arch
  );

/**
  Initialize all architecture handlers.
**/
VOID
ArchitecturesInit (
  VOID
  );

//
// Specific Architecture Instances
//

#if ANX_ARCH_X86 || ANX_ARCH_X86_64
extern IVirtualAddressSpace gPaeArch;
extern IVirtualAddressSpace gPae64Arch;
#endif

#if ANX_ARCH_RISCV
extern IVirtualAddressSpace gSv48Arch;
#endif
