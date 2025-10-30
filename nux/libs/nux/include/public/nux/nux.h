/** @file
  NUX Kernel Library Main API

  Provides the main COM-style kernel API including memory management,
  CPU operations, user context management, and kernel entry points.

  This header defines multiple COM interfaces for different subsystems:
  - INuxMemory: Physical memory and PFN operations
  - INuxKva: Kernel virtual address operations
  - INuxKmap: Kernel mapping operations
  - INuxKmem: Kernel memory allocation
  - INuxCpu: CPU management and IPI operations
  - INuxTimer: Timer operations
  - INuxUmap: User address space mapping
  - INuxUaddr: User address validation and copy
  - INuxUctxt: User context manipulation
  - INux: Main aggregator interface

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __nux_nux_h__
#define __nux_nux_h__

#include <config.h>
#include <nux/defs.h>
#include <nux/types.h>
#include <nux/locks.h>

//
// NUX Interface GUIDs
//

#define IID_INUX \
  { 0x7F8E2A3B, 0x94C6, 0x4D5E, { 0xB7, 0x4F, 0xA1, 0x9D, 0x6E, 0x8C, 0x3B, 0x2A } }

#define IID_INUX_MEMORY \
  { 0x9A1B2C3D, 0x4E5F, 0x6708, { 0x90, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x07 } }

#define IID_INUX_KVA \
  { 0x1C2D3E4F, 0x5A6B, 0x7C8D, { 0x9E, 0x0F, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F } }

#define IID_INUX_KMAP \
  { 0x2B3C4D5E, 0x6F7A, 0x8B9C, { 0xAD, 0xBE, 0xCF, 0xD0, 0xE1, 0xF2, 0x03, 0x14 } }

#define IID_INUX_KMEM \
  { 0x3C4D5E6F, 0x7A8B, 0x9CAD, { 0xBE, 0xCF, 0xD0, 0xE1, 0xF2, 0x03, 0x14, 0x25 } }

#define IID_INUX_CPU \
  { 0x4D5E6F7A, 0x8B9C, 0xADBE, { 0xCF, 0xD0, 0xE1, 0xF2, 0x03, 0x14, 0x25, 0x36 } }

#define IID_INUX_TIMER \
  { 0x5E6F7A8B, 0x9CAD, 0xBECF, { 0xD0, 0xE1, 0xF2, 0x03, 0x14, 0x25, 0x36, 0x47 } }

#define IID_INUX_UMAP \
  { 0x6F7A8B9C, 0xADBE, 0xCFD0, { 0xE1, 0xF2, 0x03, 0x14, 0x25, 0x36, 0x47, 0x58 } }

#define IID_INUX_UADDR \
  { 0x7A8B9CAD, 0xBECF, 0xD0E1, { 0xF2, 0x03, 0x14, 0x25, 0x36, 0x47, 0x58, 0x69 } }

#define IID_INUX_UCTXT \
  { 0x8B9CADBE, 0xCFD0, 0xE1F2, { 0x03, 0x14, 0x25, 0x36, 0x47, 0x58, 0x69, 0x7A } }

//
// Forward Declarations
//

INTERFACE_DECL (INux)
INTERFACE_DECL (INuxMemory)
INTERFACE_DECL (INuxKva)
INTERFACE_DECL (INuxKmap)
INTERFACE_DECL (INuxKmem)
INTERFACE_DECL (INuxCpu)
INTERFACE_DECL (INuxTimer)
INTERFACE_DECL (INuxUmap)
INTERFACE_DECL (INuxUaddr)
INTERFACE_DECL (INuxUctxt)

//
// Kernel Memory Trim Modes
//

#define TRIM_NONE  0  ///< No trimming
#define TRIM_BRK   1  ///< Trim break allocation
#define TRIM_HEAP  2  ///< Trim heap allocation

//
// Log Levels
//

#define LOGL_DEBUG  -1  ///< Debug messages
#define LOGL_INFO    0  ///< Informational messages
#define LOGL_WARN    1  ///< Warning messages
#define LOGL_ERROR   2  ///< Error messages
#define LOGL_FATAL   3  ///< Fatal error messages

//
// Exit Values
//

#define EXIT_HALT  0  ///< Halt the current CPU
#define EXIT_IDLE  1  ///< Set the current CPU to idle

//
// Page Fault Handler Callback Type
//

typedef BOOLEAN (*PF_HANDLER)(USER_ADDRESS Va, hal_pfinfo_t Info);

//
// INuxMemory Interface - Physical Memory Operations
//

struct _INuxMemoryVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN INuxMemory *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN INuxMemory *This);
  ULONG   (*Release)(IN INuxMemory *This);

  //
  // INuxMemory Methods
  //

  /**
    Get temporary access to a physical page.

    Obtains a virtual address mapping for a physical page frame for
    temporary access. If the page is in the direct map, returns the
    direct map address. Otherwise, creates a temporary mapping in the
    PFN cache.

    @param[in]  This  Pointer to the INuxMemory instance.
    @param[in]  Pfn   Physical frame number to access.

    @return Virtual address pointer to the physical page.
  **/
  VOID *(*PfnGet)(IN INuxMemory *This, IN PFN Pfn);

  /**
    Release temporary access to a physical page.

    Releases a mapping obtained with PfnGet. The virtual address
    pointer must not be used after this call.

    @param[in]  This  Pointer to the INuxMemory instance.
    @param[in]  Pfn   Physical frame number.
    @param[in]  Va   Virtual address pointer from PfnGet.
  **/
  VOID (*PfnPut)(IN INuxMemory *This, IN PFN Pfn, IN VOID *Va);

  /**
    Allocate a physical page frame.

    Allocates a physical page frame from the system allocator.

    @param[in]  This  Pointer to the INuxMemory instance.
    @param[in]  Low   If non-zero, allocate from low memory.

    @return Physical frame number, or PFN_INVALID on failure.
  **/
  PFN (*PfnAllocate)(IN INuxMemory *This, IN INT32 Low);

  /**
    Free a physical page frame.

    Returns a physical page frame to the system allocator.

    @param[in]  This  Pointer to the INuxMemory instance.
    @param[in]  Pfn   Physical frame number to free.
  **/
  VOID (*PfnFree)(IN INuxMemory *This, IN PFN Pfn);

  /**
    Get count of available physical pages.

    @param[in]  This  Pointer to the INuxMemory instance.

    @return Number of available physical page frames.
  **/
  UINTN (*PfnAvailable)(IN INuxMemory *This);

  /**
    Set custom allocator functions.

    Replaces the default S-tree allocator with custom functions.

    @param[in]  This      Pointer to the INuxMemory instance.
    @param[in]  AllocFn   Custom allocation function.
    @param[in]  FreeFn    Custom free function.
  **/
  VOID (*SetAllocator)(
    IN INuxMemory  *This,
    IN PFN         (*AllocFn)(INT32 Low),
    IN VOID        (*FreeFn)(PFN Pfn)
    );
};

INTERFACE_INHERIT_IUNKNOWN (INuxMemory)

//
// INuxKva Interface - Kernel Virtual Address Operations
//

struct _INuxKvaVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN INuxKva *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN INuxKva *This);
  ULONG   (*Release)(IN INuxKva *This);

  //
  // INuxKva Methods
  //

  /**
    Allocate kernel virtual address space.

    Reserves a range of kernel virtual addresses without mapping them.

    @param[in]  This  Pointer to the INuxKva instance.
    @param[in]  Size  Size in bytes to allocate.

    @return Virtual address, or VADDR_INVALID on failure.
  **/
  VIRTUAL_ADDRESS (*Allocate)(IN INuxKva *This, IN UINTN Size);

  /**
    Free kernel virtual address space.

    Releases a previously allocated range of kernel virtual addresses.

    @param[in]  This  Pointer to the INuxKva instance.
    @param[in]  Va    Virtual address to free.
    @param[in]  Size  Size in bytes.
  **/
  VOID (*Free)(IN INuxKva *This, IN VIRTUAL_ADDRESS Va, IN UINTN Size);

  /**
    Map a physical page into kernel virtual space.

    Allocates KVA and maps a single physical page.

    @param[in]  This  Pointer to the INuxKva instance.
    @param[in]  Pfn   Physical frame number to map.
    @param[in]  Prot  Protection flags (HAL_PTE_*).

    @return Virtual address pointer, or NULL on failure.
  **/
  VOID *(*Map)(IN INuxKva *This, IN PFN Pfn, IN UINTN Prot);

  /**
    Map physical address range into kernel virtual space.

    Allocates KVA and maps a range of physical addresses.

    @param[in]  This   Pointer to the INuxKva instance.
    @param[in]  Paddr  Physical address to map.
    @param[in]  Size   Size in bytes.
    @param[in]  Prot   Protection flags (HAL_PTE_*).

    @return Virtual address pointer, or NULL on failure.
  **/
  VOID *(*PhysMap)(IN INuxKva *This, IN PHYSICAL_ADDRESS Paddr, IN UINTN Size, IN UINTN Prot);

  /**
    Unmap and free kernel virtual address space.

    Unmaps the pages and frees the virtual address range.

    @param[in]  This  Pointer to the INuxKva instance.
    @param[in]  Va   Virtual address pointer.
    @param[in]  Size  Size in bytes.
  **/
  VOID (*Unmap)(IN INuxKva *This, IN VOID *Va, IN UINTN Size);
};

INTERFACE_INHERIT_IUNKNOWN (INuxKva)

//
// INuxKmap Interface - Kernel Mapping Operations
//

struct _INuxKmapVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN INuxKmap *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN INuxKmap *This);
  ULONG   (*Release)(IN INuxKmap *This);

  //
  // INuxKmap Methods
  //

  /**
    Get PFN mapped at virtual address.

    @param[in]  This  Pointer to the INuxKmap instance.
    @param[in]  Va    Virtual address to query.

    @return Physical frame number, or PFN_INVALID if not mapped.
  **/
  PFN (*GetPfn)(IN INuxKmap *This, IN VIRTUAL_ADDRESS Va);

  /**
    Map a physical page at virtual address.

    Maps a PFN at the specified virtual address, allocating page
    tables as needed.

    @param[in]  This  Pointer to the INuxKmap instance.
    @param[in]  Va    Virtual address.
    @param[in]  Pfn   Physical frame number.
    @param[in]  Prot  Protection flags (HAL_PTE_*).

    @return Previous PFN at that address, or PFN_INVALID.
  **/
  PFN (*Map)(IN INuxKmap *This, IN VIRTUAL_ADDRESS Va, IN PFN Pfn, IN UINTN Prot);

  /**
    Map a physical page without allocating page tables.

    Like Map, but fails if page tables don't exist.

    @param[in]  This  Pointer to the INuxKmap instance.
    @param[in]  Va    Virtual address.
    @param[in]  Pfn   Physical frame number.
    @param[in]  Prot  Protection flags (HAL_PTE_*).

    @return Previous PFN at that address, or PFN_INVALID on failure.
  **/
  PFN (*MapNoAlloc)(IN INuxKmap *This, IN VIRTUAL_ADDRESS Va, IN PFN Pfn, IN UINTN Prot);

  /**
    Unmap a virtual address.

    @param[in]  This  Pointer to the INuxKmap instance.
    @param[in]  Va    Virtual address to unmap.

    @return PFN that was unmapped, or PFN_INVALID.
  **/
  PFN (*Unmap)(IN INuxKmap *This, IN VIRTUAL_ADDRESS Va);

  /**
    Check if virtual address is mapped.

    @param[in]  This  Pointer to the INuxKmap instance.
    @param[in]  Va    Virtual address to check.

    @retval TRUE   Address is mapped.
    @retval FALSE  Address is not mapped.
  **/
  BOOLEAN (*IsMapped)(IN INuxKmap *This, IN VIRTUAL_ADDRESS Va);

  /**
    Check if address range is mapped.

    @param[in]  This  Pointer to the INuxKmap instance.
    @param[in]  Va    Starting virtual address.
    @param[in]  Size  Size in bytes.

    @retval TRUE   Entire range is mapped.
    @retval FALSE  Range contains unmapped addresses.
  **/
  BOOLEAN (*IsMappedRange)(IN INuxKmap *This, IN VIRTUAL_ADDRESS Va, IN UINTN Size);

  /**
    Ensure virtual address has required permissions.

    @param[in]  This     Pointer to the INuxKmap instance.
    @param[in]  Va       Virtual address.
    @param[in]  ReqProt  Required protection flags.

    @retval TRUE   Address has required permissions.
    @retval FALSE  Address lacks required permissions.
  **/
  BOOLEAN (*Ensure)(IN INuxKmap *This, IN VIRTUAL_ADDRESS Va, IN UINTN ReqProt);

  /**
    Ensure address range has required permissions.

    @param[in]  This     Pointer to the INuxKmap instance.
    @param[in]  Va       Starting virtual address.
    @param[in]  Size     Size in bytes.
    @param[in]  ReqProt  Required protection flags.

    @retval TRUE   Range has required permissions.
    @retval FALSE  Range lacks required permissions.
  **/
  BOOLEAN (*EnsureRange)(IN INuxKmap *This, IN VIRTUAL_ADDRESS Va, IN UINTN Size, IN UINTN ReqProt);

  /**
    Get current TLB generation.

    @param[in]  This  Pointer to the INuxKmap instance.

    @return Current TLB generation number.
  **/
  TLB_GENERATION (*GetTlbGen)(IN INuxKmap *This);

  /**
    Get global TLB generation.

    @param[in]  This  Pointer to the INuxKmap instance.

    @return Global TLB generation number.
  **/
  TLB_GENERATION (*GetTlbGenGlobal)(IN INuxKmap *This);

  /**
    Commit pending TLB operations.

    Flushes TLB entries for pages modified since last commit.

    @param[in]  This  Pointer to the INuxKmap instance.
  **/
  VOID (*Commit)(IN INuxKmap *This);
};

INTERFACE_INHERIT_IUNKNOWN (INuxKmap)

//
// INuxKmem Interface - Kernel Memory Allocation
//

struct _INuxKmemVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN INuxKmem *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN INuxKmem *This);
  ULONG   (*Release)(IN INuxKmem *This);

  //
  // INuxKmem Methods
  //

  /**
    Set break address.

    Sets the end of the kernel heap to the specified address.

    @param[in]  This   Pointer to the INuxKmem instance.
    @param[in]  Low    Memory region (0 or 1).
    @param[in]  Vaddr  New break address.

    @retval 0   Success.
    @retval -1  Failure.
  **/
  INT32 (*Brk)(IN INuxKmem *This, IN INT32 Low, IN VIRTUAL_ADDRESS Vaddr);

  /**
    Adjust break address by increment.

    @param[in]  This  Pointer to the INuxKmem instance.
    @param[in]  Low   Memory region (0 or 1).
    @param[in]  Inc   Increment (can be negative).

    @return New break address, or VADDR_INVALID on failure.
  **/
  VIRTUAL_ADDRESS (*Sbrk)(IN INuxKmem *This, IN INT32 Low, IN INTN Inc);

  /**
    Grow break allocation by size.

    @param[in]  This  Pointer to the INuxKmem instance.
    @param[in]  Low   Memory region (0 or 1).
    @param[in]  Size  Size to grow in bytes.

    @return New break address, or VADDR_INVALID on failure.
  **/
  VIRTUAL_ADDRESS (*BrkGrow)(IN INuxKmem *This, IN INT32 Low, IN UINTN Size);

  /**
    Shrink break allocation by size.

    @param[in]  This  Pointer to the INuxKmem instance.
    @param[in]  Low   Memory region (0 or 1).
    @param[in]  Size  Size to shrink in bytes.

    @retval 0   Success.
    @retval -1  Failure.
  **/
  INT32 (*BrkShrink)(IN INuxKmem *This, IN INT32 Low, IN UINTN Size);

  /**
    Allocate kernel memory.

    Allocates memory from the kernel heap.

    @param[in]  This  Pointer to the INuxKmem instance.
    @param[in]  Low   Memory region (0 or 1).
    @param[in]  Size  Size in bytes.

    @return Virtual address, or VADDR_INVALID on failure.
  **/
  VIRTUAL_ADDRESS (*Allocate)(IN INuxKmem *This, IN INT32 Low, IN UINTN Size);

  /**
    Free kernel memory.

    @param[in]  This   Pointer to the INuxKmem instance.
    @param[in]  Low    Memory region (0 or 1).
    @param[in]  Vaddr  Address to free.
    @param[in]  Size   Size in bytes.
  **/
  VOID (*Free)(IN INuxKmem *This, IN INT32 Low, IN VIRTUAL_ADDRESS Vaddr, IN UINTN Size);

  /**
    Set memory trim mode.

    @param[in]  This      Pointer to the INuxKmem instance.
    @param[in]  TrimMode  Trim mode (TRIM_*).
  **/
  VOID (*SetTrimMode)(IN INuxKmem *This, IN UINTN TrimMode);

  /**
    Trim memory once.

    Attempts to free unused memory based on trim mode.

    @param[in]  This      Pointer to the INuxKmem instance.
    @param[in]  TrimMode  Trim mode (TRIM_*).
  **/
  VOID (*TrimOne)(IN INuxKmem *This, IN UINTN TrimMode);
};

INTERFACE_INHERIT_IUNKNOWN (INuxKmem)

//
// INuxCpu Interface - CPU Management and Operations
//

struct _INuxCpuVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN INuxCpu *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN INuxCpu *This);
  ULONG   (*Release)(IN INuxCpu *This);

  //
  // INuxCpu Methods
  //

  /**
    Start all secondary CPUs.

    @param[in]  This  Pointer to the INuxCpu instance.
  **/
  VOID (*StartAll)(IN INuxCpu *This);

  /**
    Get current CPU ID.

    @param[in]  This  Pointer to the INuxCpu instance.

    @return Current CPU identifier.
  **/
  UINTN (*GetId)(IN INuxCpu *This);

  /**
    Get total number of CPUs.

    @param[in]  This  Pointer to the INuxCpu instance.

    @return Number of CPUs in the system.
  **/
  UINTN (*GetNum)(IN INuxCpu *This);

  /**
    Get active CPU mask.

    @param[in]  This  Pointer to the INuxCpu instance.

    @return Bitmask of active CPUs.
  **/
  CPU_MASK (*GetActiveMask)(IN INuxCpu *This);

  /**
    Set CPU-local data pointer.

    @param[in]  This  Pointer to the INuxCpu instance.
    @param[in]  Ptr  CPU-local data pointer.
  **/
  VOID (*SetData)(IN INuxCpu *This, IN VOID *Ptr);

  /**
    Get CPU-local data pointer.

    @param[in]  This  Pointer to the INuxCpu instance.

    @return CPU-local data pointer.
  **/
  VOID *(*GetData)(IN INuxCpu *This);

  /**
    Idle the current CPU.

    @param[in]  This  Pointer to the INuxCpu instance.
  **/
  VOID (*Idle)(IN INuxCpu *This);

  /**
    Send NMI to a specific CPU.

    @param[in]  This   Pointer to the INuxCpu instance.
    @param[in]  CpuId  Target CPU identifier.
  **/
  VOID (*SendNmi)(IN INuxCpu *This, IN UINTN CpuId);

  /**
    Send NMI to CPUs in mask.

    @param[in]  This     Pointer to the INuxCpu instance.
    @param[in]  CpuMask  Bitmask of target CPUs.
  **/
  VOID (*SendNmiMask)(IN INuxCpu *This, IN CPU_MASK CpuMask);

  /**
    Broadcast NMI to all CPUs except current.

    @param[in]  This  Pointer to the INuxCpu instance.
  **/
  VOID (*BroadcastNmiAllButSelf)(IN INuxCpu *This);

  /**
    Broadcast NMI to all CPUs.

    @param[in]  This  Pointer to the INuxCpu instance.
  **/
  VOID (*BroadcastNmi)(IN INuxCpu *This);

  /**
    Send IPI to a specific CPU.

    @param[in]  This   Pointer to the INuxCpu instance.
    @param[in]  CpuId  Target CPU identifier.
  **/
  VOID (*SendIpi)(IN INuxCpu *This, IN UINTN CpuId);

  /**
    Send IPI to CPUs in mask.

    @param[in]  This     Pointer to the INuxCpu instance.
    @param[in]  CpuMask  Bitmask of target CPUs.
  **/
  VOID (*SendIpiMask)(IN INuxCpu *This, IN CPU_MASK CpuMask);

  /**
    Broadcast IPI to all CPUs.

    @param[in]  This  Pointer to the INuxCpu instance.
  **/
  VOID (*BroadcastIpi)(IN INuxCpu *This);

  /**
    Flush TLB on a specific CPU.

    @param[in]  This   Pointer to the INuxCpu instance.
    @param[in]  CpuId  Target CPU identifier.
  **/
  VOID (*FlushTlb)(IN INuxCpu *This, IN UINTN CpuId);

  /**
    Flush TLB on CPUs in mask.

    @param[in]  This     Pointer to the INuxCpu instance.
    @param[in]  CpuMask  Bitmask of target CPUs.
  **/
  VOID (*FlushTlbMask)(IN INuxCpu *This, IN CPU_MASK CpuMask);

  /**
    Broadcast TLB flush to all CPUs.

    @param[in]  This  Pointer to the INuxCpu instance.
  **/
  VOID (*BroadcastFlushTlb)(IN INuxCpu *This);

  /**
    Broadcast TLB flush and wait for completion.

    @param[in]  This  Pointer to the INuxCpu instance.
  **/
  VOID (*BroadcastFlushTlbSync)(IN INuxCpu *This);

  /**
    Update kernel TLB on current CPU.

    @param[in]  This  Pointer to the INuxCpu instance.
  **/
  VOID (*UpdateKernelTlb)(IN INuxCpu *This);

  /**
    Reach target TLB generation.

    @param[in]  This    Pointer to the INuxCpu instance.
    @param[in]  Target  Target TLB generation.
  **/
  VOID (*ReachKernelTlb)(IN INuxCpu *This, IN TLB_GENERATION Target);

  /**
    Stop a specific CPU.

    @param[in]  This   Pointer to the INuxCpu instance.
    @param[in]  CpuId  Target CPU identifier.
  **/
  VOID (*Stop)(IN INuxCpu *This, IN UINTN CpuId);

  /**
    Stop CPUs in mask.

    @param[in]  This     Pointer to the INuxCpu instance.
    @param[in]  CpuMask  Bitmask of target CPUs.
  **/
  VOID (*StopMask)(IN INuxCpu *This, IN CPU_MASK CpuMask);

  /**
    Broadcast stop to all CPUs.

    @param[in]  This  Pointer to the INuxCpu instance.
  **/
  VOID (*BroadcastStop)(IN INuxCpu *This);

  /**
    Copy from user address space.

    @param[in]  This       Pointer to the INuxCpu instance.
    @param[out] Dst       Kernel destination buffer.
    @param[in]  Src        User source address.
    @param[in]  Size       Number of bytes to copy.
    @param[in]  PfHandler  Page fault handler callback.

    @retval TRUE   Copy succeeded.
    @retval FALSE  Copy failed (page fault or invalid address).
  **/
  BOOLEAN (*UserAccessCopyFrom)(
    IN  INuxCpu        *This,
    OUT VOID           *Dst,
    IN  USER_ADDRESS   Src,
    IN  UINTN          Size,
    IN  PF_HANDLER     PfHandler
    );

  /**
    Copy to user address space.

    @param[in]  This       Pointer to the INuxCpu instance.
    @param[in]  Dst        User destination address.
    @param[in]  Src       Kernel source buffer.
    @param[in]  Size       Number of bytes to copy.
    @param[in]  PfHandler  Page fault handler callback.

    @retval TRUE   Copy succeeded.
    @retval FALSE  Copy failed (page fault or invalid address).
  **/
  BOOLEAN (*UserAccessCopyTo)(
    IN INuxCpu        *This,
    IN USER_ADDRESS   Dst,
    IN VOID           *Src,
    IN UINTN          Size,
    IN PF_HANDLER     PfHandler
    );

  /**
    Set memory in user address space.

    @param[in]  This       Pointer to the INuxCpu instance.
    @param[in]  Dst        User destination address.
    @param[in]  Ch         Byte value to set.
    @param[in]  Size       Number of bytes to set.
    @param[in]  PfHandler  Page fault handler callback.

    @retval TRUE   Operation succeeded.
    @retval FALSE  Operation failed (page fault or invalid address).
  **/
  BOOLEAN (*UserAccessMemset)(
    IN INuxCpu        *This,
    IN USER_ADDRESS   Dst,
    IN INT32          Ch,
    IN UINTN          Size,
    IN PF_HANDLER     PfHandler
    );

  /**
    Get current user address space mapping.

    @param[in]  This  Pointer to the INuxCpu instance.

    @return Pointer to current UMAP, or NULL if none.
  **/
  UMAP *(*GetCurrentUmap)(IN INuxCpu *This);

  /**
    Enter user address space mapping.

    @param[in]  This   Pointer to the INuxCpu instance.
    @param[in]  Umap  User mapping to activate.
  **/
  VOID (*EnterUmap)(IN INuxCpu *This, IN UMAP *Umap);

  /**
    Exit current user address space mapping.

    @param[in]  This  Pointer to the INuxCpu instance.

    @return Pointer to previous UMAP, or NULL.
  **/
  UMAP *(*ExitUmap)(IN INuxCpu *This);
};

INTERFACE_INHERIT_IUNKNOWN (INuxCpu)

//
// INuxTimer Interface - Timer Operations
//

struct _INuxTimerVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN INuxTimer *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN INuxTimer *This);
  ULONG   (*Release)(IN INuxTimer *This);

  //
  // INuxTimer Methods
  //

  /**
    Set timer alarm.

    @param[in]  This    Pointer to the INuxTimer instance.
    @param[in]  TimeNs  Time in nanoseconds until alarm.
  **/
  VOID (*SetAlarm)(IN INuxTimer *This, IN UINT32 TimeNs);

  /**
    Clear timer alarm.

    @param[in]  This  Pointer to the INuxTimer instance.
  **/
  VOID (*ClearAlarm)(IN INuxTimer *This);

  /**
    Get current time.

    @param[in]  This  Pointer to the INuxTimer instance.

    @return Current time value in nanoseconds.
  **/
  UINT64 (*GetTime)(IN INuxTimer *This);
};

INTERFACE_INHERIT_IUNKNOWN (INuxTimer)

//
// INuxUmap Interface - User Address Space Mapping
//

struct _INuxUmapVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN INuxUmap *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN INuxUmap *This);
  ULONG   (*Release)(IN INuxUmap *This);

  //
  // INuxUmap Methods
  //

  /**
    Bootstrap a user mapping.

    Initializes a user mapping structure for the bootstrap process.

    @param[in,out] UserMap  User mapping structure to bootstrap.
  **/
  VOID (*Bootstrap)(IN OUT UMAP *UserMap);

  /**
    Initialize a user mapping.

    @param[in,out] UserMap  User mapping structure to initialize.
  **/
  VOID (*Init)(IN OUT UMAP *UserMap);

  /**
    Free a user mapping.

    @param[in,out] UserMap  User mapping structure to free.
  **/
  VOID (*Free)(IN OUT UMAP *UserMap);

  /**
    Map a physical page in user address space.

    @param[in,out] UserMap  User mapping structure.
    @param[in]     Va        Virtual address.
    @param[in]     Pfn       Physical frame number.
    @param[in]     Prot      Protection flags.
    @param[out]    OldPfn   Previous PFN at that address, or NULL.

    @retval TRUE   Mapping succeeded.
    @retval FALSE  Mapping failed.
  **/
  BOOLEAN (*Map)(
    IN OUT UMAP              *UserMap,
    IN     VIRTUAL_ADDRESS   Va,
    IN     PFN               Pfn,
    IN     UINTN             Prot,
    OUT    PFN               *OldPfn OPTIONAL
    );

  /**
    Change protection flags.

    @param[in,out] UserMap  User mapping structure.
    @param[in]     Va        Virtual address.
    @param[in]     ProtSet   Protection flags to set.
    @param[in]     ProtClr   Protection flags to clear.

    @return Previous protection flags.
  **/
  UINTN (*ChangeFlags)(
    IN OUT UMAP              *UserMap,
    IN     VIRTUAL_ADDRESS   Va,
    IN     UINTN             ProtSet,
    IN     UINTN             ProtClr
    );

  /**
    Unmap a virtual address.

    @param[in,out] UserMap  User mapping structure.
    @param[in]     Va        Virtual address to unmap.

    @return PFN that was unmapped, or PFN_INVALID.
  **/
  PFN (*Unmap)(IN OUT UMAP *UserMap, IN VIRTUAL_ADDRESS Va);

  /**
    Commit pending TLB operations.

    @param[in,out] UserMap  User mapping structure.
  **/
  VOID (*Commit)(IN OUT UMAP *UserMap);
};

INTERFACE_INHERIT_IUNKNOWN (INuxUmap)

//
// INuxUaddr Interface - User Address Validation and Copy
//

struct _INuxUaddrVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN INuxUaddr *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN INuxUaddr *This);
  ULONG   (*Release)(IN INuxUaddr *This);

  //
  // INuxUaddr Methods
  //

  /**
    Validate user address.

    @param[in]  This  Pointer to the INuxUaddr instance.
    @param[in]  Addr  User address to validate.

    @retval TRUE   Address is valid.
    @retval FALSE  Address is invalid.
  **/
  BOOLEAN (*Valid)(IN INuxUaddr *This, IN USER_ADDRESS Addr);

  /**
    Validate user address range.

    @param[in]  This  Pointer to the INuxUaddr instance.
    @param[in]  Addr  Starting user address.
    @param[in]  Size  Size in bytes.

    @retval TRUE   Range is valid.
    @retval FALSE  Range is invalid.
  **/
  BOOLEAN (*ValidRange)(IN INuxUaddr *This, IN USER_ADDRESS Addr, IN UINTN Size);

  /**
    Copy from user address space.

    @param[in]  This       Pointer to the INuxUaddr instance.
    @param[out] Dst       Kernel destination buffer.
    @param[in]  Src        User source address.
    @param[in]  Size       Number of bytes to copy.
    @param[in]  PfHandler  Page fault handler callback.

    @retval TRUE   Copy succeeded.
    @retval FALSE  Copy failed.
  **/
  BOOLEAN (*CopyFrom)(
    IN  INuxUaddr      *This,
    OUT VOID           *Dst,
    IN  USER_ADDRESS   Src,
    IN  UINTN          Size,
    IN  PF_HANDLER     PfHandler
    );

  /**
    Copy to user address space.

    @param[in]  This       Pointer to the INuxUaddr instance.
    @param[in]  Dst        User destination address.
    @param[in]  Src       Kernel source buffer.
    @param[in]  Size       Number of bytes to copy.
    @param[in]  PfHandler  Page fault handler callback.

    @retval TRUE   Copy succeeded.
    @retval FALSE  Copy failed.
  **/
  BOOLEAN (*CopyTo)(
    IN INuxUaddr      *This,
    IN USER_ADDRESS   Dst,
    IN VOID           *Src,
    IN UINTN          Size,
    IN PF_HANDLER     PfHandler
    );

  /**
    Set memory in user address space.

    @param[in]  This       Pointer to the INuxUaddr instance.
    @param[in]  Dst        User destination address.
    @param[in]  Ch         Byte value to set.
    @param[in]  Size       Number of bytes to set.
    @param[in]  PfHandler  Page fault handler callback.

    @retval TRUE   Operation succeeded.
    @retval FALSE  Operation failed.
  **/
  BOOLEAN (*Memset)(
    IN INuxUaddr      *This,
    IN USER_ADDRESS   Dst,
    IN INT32          Ch,
    IN UINTN          Size,
    IN PF_HANDLER     PfHandler
    );
};

INTERFACE_INHERIT_IUNKNOWN (INuxUaddr)

//
// INuxUctxt Interface - User Context Manipulation
//

struct _INuxUctxtVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN INuxUctxt *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN INuxUctxt *This);
  ULONG   (*Release)(IN INuxUctxt *This);

  //
  // INuxUctxt Methods
  //

  /**
    Bootstrap a user context.

    @param[in,out] Uctxt  User context to bootstrap.

    @retval TRUE   Bootstrap succeeded.
    @retval FALSE  Bootstrap failed.
  **/
  BOOLEAN (*Bootstrap)(IN OUT UCTXT *Uctxt);

  /**
    Initialize a user context.

    @param[in,out] Uctxt  User context to initialize.
    @param[in]     Ip      Initial instruction pointer.
    @param[in]     Sp      Initial stack pointer.
    @param[in]     Gp      Initial global pointer.
  **/
  VOID (*Init)(
    IN OUT UCTXT             *Uctxt,
    IN     VIRTUAL_ADDRESS   Ip,
    IN     VIRTUAL_ADDRESS   Sp,
    IN     VIRTUAL_ADDRESS   Gp
    );

  /**
    Set instruction pointer.

    @param[in,out] Uctxt  User context.
    @param[in]     Ip      New instruction pointer.
  **/
  VOID (*SetIp)(IN OUT UCTXT *Uctxt, IN VIRTUAL_ADDRESS Ip);

  /**
    Get instruction pointer.

    @param[in] Uctxt  User context.

    @return Current instruction pointer.
  **/
  VIRTUAL_ADDRESS (*GetIp)(IN UCTXT *Uctxt);

  /**
    Set stack pointer.

    @param[in,out] Uctxt  User context.
    @param[in]     Sp      New stack pointer.
  **/
  VOID (*SetSp)(IN OUT UCTXT *Uctxt, IN VIRTUAL_ADDRESS Sp);

  /**
    Get stack pointer.

    @param[in] Uctxt  User context.

    @return Current stack pointer.
  **/
  VIRTUAL_ADDRESS (*GetSp)(IN UCTXT *Uctxt);

  /**
    Set global pointer.

    @param[in,out] Uctxt  User context.
    @param[in]     Gp      New global pointer.
  **/
  VOID (*SetGp)(IN OUT UCTXT *Uctxt, IN VIRTUAL_ADDRESS Gp);

  /**
    Get global pointer.

    @param[in] Uctxt  User context.

    @return Current global pointer.
  **/
  VIRTUAL_ADDRESS (*GetGp)(IN UCTXT *Uctxt);

  /**
    Set return value.

    @param[in,out] Uctxt  User context.
    @param[in]     Ret     Return value.
  **/
  VOID (*SetRet)(IN OUT UCTXT *Uctxt, IN UINTN Ret);

  /**
    Set argument register A0.

    @param[in,out] Uctxt  User context.
    @param[in]     A0      Argument value.
  **/
  VOID (*SetA0)(IN OUT UCTXT *Uctxt, IN UINTN A0);

  /**
    Set argument register A1.

    @param[in,out] Uctxt  User context.
    @param[in]     A1      Argument value.
  **/
  VOID (*SetA1)(IN OUT UCTXT *Uctxt, IN UINTN A1);

  /**
    Set argument register A2.

    @param[in,out] Uctxt  User context.
    @param[in]     A2      Argument value.
  **/
  VOID (*SetA2)(IN OUT UCTXT *Uctxt, IN UINTN A2);

  /**
    Set TLS pointer.

    @param[in,out] Uctxt  User context.
    @param[in]     Tls     TLS pointer value.
  **/
  VOID (*SetTls)(IN OUT UCTXT *Uctxt, IN UINTN Tls);

  /**
    Print user context for debugging.

    @param[in] Uctxt  User context to print.
  **/
  VOID (*Print)(IN UCTXT *Uctxt);
};

INTERFACE_INHERIT_IUNKNOWN (INuxUctxt)

//
// INux Interface - Main Aggregator Interface
//

struct _INuxVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN INux *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN INux *This);
  ULONG   (*Release)(IN INux *This);

  //
  // INux Methods
  //

  /**
    Get memory management interface.

    @param[in]  This      Pointer to the INux instance.
    @param[out] ppMemory  Receives INuxMemory interface pointer.

    @retval S_OK  Success.
  **/
  HRESULT (*GetMemoryInterface)(IN INux *This, OUT INuxMemory **ppMemory);

  /**
    Get kernel virtual address interface.

    @param[in]  This   Pointer to the INux instance.
    @param[out] ppKva  Receives INuxKva interface pointer.

    @retval S_OK  Success.
  **/
  HRESULT (*GetKvaInterface)(IN INux *This, OUT INuxKva **ppKva);

  /**
    Get kernel mapping interface.

    @param[in]  This    Pointer to the INux instance.
    @param[out] ppKmap  Receives INuxKmap interface pointer.

    @retval S_OK  Success.
  **/
  HRESULT (*GetKmapInterface)(IN INux *This, OUT INuxKmap **ppKmap);

  /**
    Get kernel memory allocation interface.

    @param[in]  This    Pointer to the INux instance.
    @param[out] ppKmem  Receives INuxKmem interface pointer.

    @retval S_OK  Success.
  **/
  HRESULT (*GetKmemInterface)(IN INux *This, OUT INuxKmem **ppKmem);

  /**
    Get CPU management interface.

    @param[in]  This   Pointer to the INux instance.
    @param[out] ppCpu  Receives INuxCpu interface pointer.

    @retval S_OK  Success.
  **/
  HRESULT (*GetCpuInterface)(IN INux *This, OUT INuxCpu **ppCpu);

  /**
    Get timer interface.

    @param[in]  This     Pointer to the INux instance.
    @param[out] ppTimer  Receives INuxTimer interface pointer.

    @retval S_OK  Success.
  **/
  HRESULT (*GetTimerInterface)(IN INux *This, OUT INuxTimer **ppTimer);

  /**
    Get user mapping interface.

    @param[in]  This    Pointer to the INux instance.
    @param[out] ppUmap  Receives INuxUmap interface pointer.

    @retval S_OK  Success.
  **/
  HRESULT (*GetUmapInterface)(IN INux *This, OUT INuxUmap **ppUmap);

  /**
    Get user address interface.

    @param[in]  This     Pointer to the INux instance.
    @param[out] ppUaddr  Receives INuxUaddr interface pointer.

    @retval S_OK  Success.
  **/
  HRESULT (*GetUaddrInterface)(IN INux *This, OUT INuxUaddr **ppUaddr);

  /**
    Get user context interface.

    @param[in]  This     Pointer to the INux instance.
    @param[out] ppUctxt  Receives INuxUctxt interface pointer.

    @retval S_OK  Success.
  **/
  HRESULT (*GetUctxtInterface)(IN INux *This, OUT INuxUctxt **ppUctxt);
};

INTERFACE_INHERIT_IUNKNOWN (INux)

//
// Global NUX Interface Pointer
//

extern INux *gpNux;

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

//
// Logging Helper Function
//

static inline
VOID
__printflike (2, 3)
__log (
  IN CONST INT32  Level,
  IN CONST CHAR8  *Format,
  ...
  )
{
  va_list  Args;

  if (Level == LOGL_WARN) {
    printf ("Warning: ");
  } else if (Level == LOGL_FATAL) {
    printf ("Fatal: ");
  } else if (Level == LOGL_ERROR) {
    printf ("ERROR: ");
  }

  va_start (Args, Format);
  vprintf (Format, Args);
  va_end (Args);

  putchar ('\n');

  if (Level == LOGL_FATAL) {
    exit (-1);
  }
}

//
// Logging Macros
//

#ifdef DEBUG
#define debug(...)  __log(LOGL_DEBUG, __VA_ARGS__)
#else
#define debug(...)
#endif

#define info(...)   __log(LOGL_INFO, __VA_ARGS__)
#define warn(...)   __log(LOGL_WARN, __VA_ARGS__)
#define error(...)  __log(LOGL_ERROR, __VA_ARGS__)
#define fatal(...)  __log(LOGL_FATAL, __VA_ARGS__)

//
// System Panic
//

/**
  Stop all CPUs and panic.

  @param[in] Message  Error message.
  @param[in] Frame    CPU frame at time of panic, or NULL.
**/
VOID __dead NuxPanic (IN CONST CHAR8 *Message, IN struct hal_frame *Frame);

//
// Main Entry Points
//

/**
  Primary CPU initialization entry point.

  Called at startup after HAL and PLT have been initialized.

  @param[in] argc  Argument count.
  @param[in] argv  Argument vector.

  @return Exit code (EXIT_HALT or EXIT_IDLE).
**/
INT32 main (IN INT32 argc, IN CHAR8 *argv[]);

/**
  Secondary CPU initialization entry point.

  Called when a secondary processor starts.

  @return Exit code (EXIT_HALT or EXIT_IDLE).
**/
INT32 main_ap (VOID);

//
// Kernel Entry Points (Event Handlers)
//

/**
  System call entry point.

  @param[in] Uctxt  User context, or NULL if CPU was idle.
  @param[in] Arg1-7  System call arguments.

  @return User context to restore, or NULL to idle CPU.
**/
UCTXT *entry_sysc (
  IN UCTXT        *Uctxt OPTIONAL,
  IN UINTN        Arg1,
  IN UINTN        Arg2,
  IN UINTN        Arg3,
  IN UINTN        Arg4,
  IN UINTN        Arg5,
  IN UINTN        Arg6,
  IN UINTN        Arg7
  );

/**
  Page fault entry point.

  @param[in] Uctxt  User context, or NULL if CPU was idle.
  @param[in] Va      Faulting virtual address.
  @param[in] PfInfo  Page fault information.

  @return User context to restore, or NULL to idle CPU.
**/
UCTXT *entry_pf (
  IN UCTXT              *Uctxt OPTIONAL,
  IN VIRTUAL_ADDRESS    Va,
  IN hal_pfinfo_t       PfInfo
  );

/**
  Generic exception entry point.

  @param[in] Uctxt  User context, or NULL if CPU was idle.
  @param[in] ExNum   Exception number.

  @return User context to restore, or NULL to idle CPU.
**/
UCTXT *entry_ex (
  IN UCTXT  *Uctxt OPTIONAL,
  IN UINTN  ExNum
  );

/**
  Timer alarm entry point.

  @param[in] Uctxt  User context, or NULL if CPU was idle.

  @return User context to restore, or NULL to idle CPU.
**/
UCTXT *entry_alarm (
  IN UCTXT  *Uctxt OPTIONAL
  );

/**
  Inter-processor interrupt entry point.

  @param[in] Uctxt  User context, or NULL if CPU was idle.

  @return User context to restore, or NULL to idle CPU.
**/
UCTXT *entry_ipi (
  IN UCTXT  *Uctxt OPTIONAL
  );

/**
  IRQ entry point.

  @param[in] Uctxt  User context, or NULL if CPU was idle.
  @param[in] IrqNum  IRQ number.
  @param[in] Level   TRUE if level-triggered, FALSE if edge-triggered.

  @return User context to restore, or NULL to idle CPU.
**/
UCTXT *entry_irq (
  IN UCTXT    *Uctxt OPTIONAL,
  IN UINTN    IrqNum,
  IN BOOLEAN  Level
  );

//
// Physical Memory Operations
//

VOID *PfnGet (IN PFN Pfn);
VOID PfnPut (IN PFN Pfn, IN VOID *Va);
VOID NuxSetAllocator (IN PFN (*Alloc) (INT32), IN VOID (*Free) (PFN));
PFN PfnAlloc (IN INT32 Low);
VOID PfnFree (IN PFN Pfn);
UINTN PfnAvail (VOID);
PFN StreePfnAlloc (IN INT32 Low);
VOID StreePfnFree (IN PFN Pfn);

/** Legacy compatibility **/
#define pfn_get PfnGet
#define pfn_put PfnPut
#define nux_set_allocator NuxSetAllocator
#define pfn_alloc PfnAlloc
#define pfn_free PfnFree
#define pfn_avail PfnAvail
#define stree_pfnalloc StreePfnAlloc
#define stree_pfnfree StreePfnFree

//
// Kernel Virtual Address Operations
//

VIRTUAL_ADDRESS KvaAlloc (IN UINTN Size);
VOID KvaFree (IN VIRTUAL_ADDRESS Va, IN UINTN Size);
VOID *KvaMap (IN PFN Pfn, IN UINT32 Prot);
VOID *KvaPhysMap (IN PHYSICAL_ADDRESS Paddr, IN UINTN Size, IN UINT32 Prot);
VOID KvaUnmap (IN VOID *Va, IN UINTN Size);

/** Legacy compatibility **/
#define kva_alloc KvaAlloc
#define kva_free KvaFree
#define kva_map KvaMap
#define kva_physmap KvaPhysMap
#define kva_unmap KvaUnmap

//
// Kernel Mapping Operations
//

PFN KmapGetPfn (IN VIRTUAL_ADDRESS Va);
PFN KmapMap (IN VIRTUAL_ADDRESS Va, IN PFN Pfn, IN UINT32 Prot);
PFN KmapMapNoAlloc (IN VIRTUAL_ADDRESS Va, IN PFN Pfn, IN UINT32 Prot);
PFN KmapUnmap (IN VIRTUAL_ADDRESS Va);
INT32 KmapMapped (IN VIRTUAL_ADDRESS Va);
INT32 KmapMappedRange (IN VIRTUAL_ADDRESS Va, IN UINTN Size);
INT32 KmapEnsure (IN VIRTUAL_ADDRESS Va, IN UINT32 ReqProt);
INT32 KmapEnsureRange (IN VIRTUAL_ADDRESS Va, IN UINTN Size, IN UINT32 ReqProt);
volatile TLB_GENERATION KmapTlbGen (VOID);
volatile TLB_GENERATION KmapTlbGenGlobal (VOID);
VOID KmapCommit (VOID);

/** Legacy compatibility **/
#define kmap_getpfn KmapGetPfn
#define kmap_map KmapMap
#define kmap_map_noalloc KmapMapNoAlloc
#define kmap_unmap KmapUnmap
#define kmap_mapped KmapMapped
#define kmap_mapped_range KmapMappedRange
#define kmap_ensure KmapEnsure
#define kmap_ensure_range KmapEnsureRange
#define kmap_tlbgen KmapTlbGen
#define kmap_tlbgen_global KmapTlbGenGlobal
#define kmap_commit KmapCommit

//
// Kernel Memory Operations
//

INT32 KmemBrk (IN INT32 Low, IN VIRTUAL_ADDRESS Vaddr);
VIRTUAL_ADDRESS KmemSbrk (IN INT32 Low, IN INTN Inc);
VIRTUAL_ADDRESS KmemBrkGrow (IN INT32 Low, IN UINT32 Size);
INT32 KmemBrkShrink (IN INT32 Low, IN UINT32 Size);
VIRTUAL_ADDRESS KmemAlloc (IN INT32 Low, IN UINTN Size);
VOID KmemFree (IN INT32 Low, IN VIRTUAL_ADDRESS Vaddr, IN UINTN Size);
VOID KmemTrimSetMode (IN UINT32 TrimMode);
VOID KmemTrimOne (IN UINT32 TrimMode);

/** Legacy compatibility **/
#define kmem_brk KmemBrk
#define kmem_sbrk KmemSbrk
#define kmem_brkgrow KmemBrkGrow
#define kmem_brkshrink KmemBrkShrink
#define kmem_alloc KmemAlloc
#define kmem_free KmemFree
#define kmem_trim_setmode KmemTrimSetMode
#define kmem_trim_one KmemTrimOne

//
// CPU Operations
//

VOID CpuStartAll (VOID);
UINT32 CpuId (VOID);
UINT32 CpuNum (VOID);
CPU_MASK CpuActiveMask (VOID);
VOID CpuSetData (IN VOID *Ptr);
VOID *CpuGetData (VOID);
VOID CpuIdle (VOID);
VOID CpuNmi (IN INT32 Cpu);
VOID CpuNmiMask (IN CPU_MASK Map);
VOID CpuNmiAllButSelf (VOID);
VOID CpuNmiBroadcast (VOID);
VOID CpuIpi (IN INT32 Cpu);
VOID CpuIpiMask (IN CPU_MASK Map);
VOID CpuIpiBroadcast (VOID);
VOID CpuTlbFlush (IN INT32 Cpu);
VOID CpuTlbFlushMask (IN CPU_MASK Mask);
VOID CpuTlbFlushBroadcast (VOID);
VOID CpuTlbFlushBroadcastSync (VOID);
VOID CpuKtlbUpdate (VOID);
VOID CpuKtlbReach (IN TLB_GENERATION Target);
VOID CpuStop (IN INT32 Cpu);
VOID CpuStopMask (IN CPU_MASK Mask);
VOID CpuStopBroadcast (VOID);
BOOLEAN CpuUserAccessCopyFrom (OUT VOID *Dst, IN USER_ADDRESS Src, IN UINTN Size, IN BOOLEAN (*PfHandler)(USER_ADDRESS Va, UINTN Info) OPTIONAL);
BOOLEAN CpuUserAccessCopyTo (IN USER_ADDRESS Dst, IN VOID *Src, IN UINTN Size, IN BOOLEAN (*PfHandler)(USER_ADDRESS Va, UINTN Info) OPTIONAL);
BOOLEAN CpuUserAccessMemset (IN USER_ADDRESS Dst, IN INT32 Ch, IN UINTN Size, IN BOOLEAN (*PfHandler)(USER_ADDRESS Va, UINTN Info) OPTIONAL);
UMAP *CpuUmapCurrent (VOID);
VOID CpuUmapEnter (IN UMAP *Umap);
UMAP *CpuUmapExit (VOID);

/** Legacy compatibility **/
#define cpu_startall CpuStartAll
#define cpu_id CpuId
#define cpu_num CpuNum
#define cpu_activemask CpuActiveMask
#define cpu_setdata CpuSetData
#define cpu_getdata CpuGetData
#define cpu_idle CpuIdle
#define cpu_nmi CpuNmi
#define cpu_nmi_mask CpuNmiMask
#define cpu_nmi_allbutself CpuNmiAllButSelf
#define cpu_nmi_broadcast CpuNmiBroadcast
#define cpu_ipi CpuIpi
#define cpu_ipi_mask CpuIpiMask
#define cpu_ipi_broadcast CpuIpiBroadcast
#define cpu_tlbflush CpuTlbFlush
#define cpu_tlbflush_mask CpuTlbFlushMask
#define cpu_tlbflush_broadcast CpuTlbFlushBroadcast
#define cpu_tlbflush_broadcast_sync CpuTlbFlushBroadcastSync
#define cpu_ktlb_update CpuKtlbUpdate
#define cpu_ktlb_reach CpuKtlbReach
#define cpu_stop CpuStop
#define cpu_stop_mask CpuStopMask
#define cpu_stop_broadcast CpuStopBroadcast
#define cpu_useraccess_copyfrom CpuUserAccessCopyFrom
#define cpu_useraccess_copyto CpuUserAccessCopyTo
#define cpu_useraccess_memset CpuUserAccessMemset
#define cpu_umap_current CpuUmapCurrent
#define cpu_umap_enter CpuUmapEnter
#define cpu_umap_exit CpuUmapExit

//
// Timer Operations
//

VOID TimerAlarm (IN UINT32 TimeNs);
VOID TimerClear (VOID);
UINT64 TimerGetTime (VOID);

/** Legacy compatibility **/
#define timer_alarm TimerAlarm
#define timer_clear TimerClear
#define timer_gettime TimerGetTime

//
// User Mapping Operations
//

VOID UmapBootstrap (IN OUT UMAP *Umap);
VOID UmapInit (IN OUT UMAP *Umap);
VOID UmapFree (IN OUT UMAP *Umap);
BOOLEAN UmapMap (IN OUT UMAP *Umap, IN VIRTUAL_ADDRESS Va, IN PFN Pfn, IN UINT32 Prot, OUT PFN *OPfn OPTIONAL);
UINT32 UmapChFlags (IN OUT UMAP *Umap, IN VIRTUAL_ADDRESS Va, IN UINT32 ProtSet, IN UINT32 ProtClr);
PFN UmapUnmap (IN OUT UMAP *Umap, IN VIRTUAL_ADDRESS Va);
VOID UmapCommit (IN OUT UMAP *Umap);

/** Legacy compatibility **/
#define umap_bootstrap UmapBootstrap
#define umap_init UmapInit
#define umap_free UmapFree
#define umap_map UmapMap
#define umap_chflags UmapChFlags
#define umap_unmap UmapUnmap
#define umap_commit UmapCommit

//
// User Address Operations
//

BOOLEAN UaddrValid (IN USER_ADDRESS Addr);
BOOLEAN UaddrValidRange (IN USER_ADDRESS Addr, IN UINTN Size);
BOOLEAN UaddrCopyFrom (OUT VOID *Dst, IN USER_ADDRESS Src, IN UINTN Size, IN BOOLEAN (*PfHandler)(USER_ADDRESS Va, UINTN Info) OPTIONAL);
BOOLEAN UaddrCopyTo (IN USER_ADDRESS Dst, IN VOID *Src, IN UINTN Size, IN BOOLEAN (*PfHandler)(USER_ADDRESS Va, UINTN Info) OPTIONAL);
BOOLEAN UaddrMemset (IN USER_ADDRESS Dst, IN INT32 Ch, IN UINTN Size, IN BOOLEAN (*PfHandler)(USER_ADDRESS Va, UINTN Info) OPTIONAL);

/** Legacy compatibility **/
#define uaddr_valid UaddrValid
#define uaddr_validrange UaddrValidRange
#define uaddr_copyfrom UaddrCopyFrom
#define uaddr_copyto UaddrCopyTo
#define uaddr_memset UaddrMemset

//
// User Context Operations
//

BOOLEAN UctxtBootstrap (IN OUT UCTXT *Uctxt);
VOID UctxtInit (IN OUT UCTXT *Uctxt, IN VIRTUAL_ADDRESS Ip, IN VIRTUAL_ADDRESS Sp, IN VIRTUAL_ADDRESS Gp);
VOID UctxtSetIp (IN OUT UCTXT *Uctxt, IN VIRTUAL_ADDRESS Ip);
VIRTUAL_ADDRESS UctxtGetIp (IN UCTXT *Uctxt);
VOID UctxtSetSp (IN OUT UCTXT *Uctxt, IN VIRTUAL_ADDRESS Sp);
VIRTUAL_ADDRESS UctxtGetSp (IN UCTXT *Uctxt);
VOID UctxtSetGp (IN OUT UCTXT *Uctxt, IN VIRTUAL_ADDRESS Gp);
VIRTUAL_ADDRESS UctxtGetGp (IN UCTXT *Uctxt);
VOID UctxtSetRet (IN OUT UCTXT *Uctxt, IN UINTN Ret);
VOID UctxtSetA0 (IN OUT UCTXT *Uctxt, IN UINTN A0);
VOID UctxtSetA1 (IN OUT UCTXT *Uctxt, IN UINTN A1);
VOID UctxtSetA2 (IN OUT UCTXT *Uctxt, IN UINTN A2);
VOID UctxtSetTls (IN OUT UCTXT *Uctxt, IN UINTN Tls);
VOID UctxtPrint (IN UCTXT *Uctxt);

/** Legacy compatibility **/
#define uctxt_bootstrap UctxtBootstrap
#define uctxt_init UctxtInit
#define uctxt_setip UctxtSetIp
#define uctxt_getip UctxtGetIp
#define uctxt_setsp UctxtSetSp
#define uctxt_getsp UctxtGetSp
#define uctxt_setgp UctxtSetGp
#define uctxt_getgp UctxtGetGp
#define uctxt_setret UctxtSetRet
#define uctxt_seta0 UctxtSetA0
#define uctxt_seta1 UctxtSetA1
#define uctxt_seta2 UctxtSetA2
#define uctxt_settls UctxtSetTls
#define uctxt_print UctxtPrint

//
// Legacy Wrapper Functions
//

/** @deprecated Use NuxPanic instead **/
static inline void __dead nux_panic (const char *message, struct hal_frame *f) {
  NuxPanic (message, f);
}

#endif // _NUX_H
