/** @file
  Hardware Abstraction Layer Interface

  Defines the COM-style HAL interface for hardware abstraction including
  CPU operations, memory management, virtual memory mapping, interrupt
  handling, and physical CPU management.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __hal_hal_h__
#define __hal_hal_h__

#include <nux/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

//
// HAL module specific definitions
//

#include <hal/config.h>

//
// HAL Interface GUID
//

#define IID_IHAL \
  { 0x6D90F4B1, 0x42D5, 0x4A3E, { 0xA3, 0x5B, 0x8C, 0x7D, 0x9E, 0x2F, 0x1A, 0x4B } }

//
// Forward Declarations
//

INTERFACE_DECL (IHal)
INTERFACE_DECL (IHalCpu)
INTERFACE_DECL (IHalPhysMem)
INTERFACE_DECL (IHalVirtMem)
INTERFACE_DECL (IHalMap)
INTERFACE_DECL (IHalPcpu)
INTERFACE_DECL (IHalFrame)

//
// Page Table Entry Flags
//

#define HAL_PTE_P      (1 << 0)  ///< Page is present
#define HAL_PTE_W      (1 << 1)  ///< Page is writable
#define HAL_PTE_X      (1 << 2)  ///< Page is executable
#define HAL_PTE_U      (1 << 3)  ///< Page is user accessible
#define HAL_PTE_GLOBAL (1 << 4)  ///< Page is global (persists across TLB flush)
#define HAL_PTE_A      (1 << 5)  ///< Page has been accessed
#define HAL_PTE_D      (1 << 6)  ///< Page has been written to (dirty)
#define HAL_PTE_AVL0   (1 << 7)  ///< Available bit 0
#define HAL_PTE_AVL1   (1 << 8)  ///< Available bit 1
#define HAL_PTE_AVL2   (1 << 9)  ///< Available bit 2

//
// Page Fault Information Flags
//

#define HAL_PF_REASON_PROT 0          ///< Protection violation
#define HAL_PF_REASON_NOTP 1          ///< Page not present
#define HAL_PF_REASON_MASK 1          ///< Mask for reason bits
#define HAL_PF_INFO_WRITE  (1 << 4)   ///< Fault was caused by write
#define HAL_PF_INFO_USER   (1 << 5)   ///< Fault occurred in user mode
#define HAL_PF_INFO_EXE    (1 << 6)   ///< Fault was caused by instruction fetch

typedef UINTN hal_pfinfo_t;

//
// Architecture-specific types (opaque handles)
//

typedef UINTN hal_l1p_t;       ///< Level 1 page table pointer
typedef UINTN hal_l1e_t;       ///< Level 1 page table entry

#define L1P_INVALID ((hal_l1p_t)-1)

//
// IHalCpu Interface - CPU Operations
//

struct _IHalCpuVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IHalCpu *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IHalCpu *This);
  ULONG   (*Release)(IN IHalCpu *This);

  //
  // IHalCpu Methods
  //

  /**
    Perform I/O port input operation (for architectures with I/O space).

    @param[in]  This  Pointer to the IHalCpu instance.
    @param[in]  Size  Size of the I/O operation (1, 2, or 4 bytes).
    @param[in]  Port  I/O port number.

    @return Value read from the I/O port.
  **/
  UINTN (*IoIn)(IN IHalCpu *This, IN UINT8 Size, IN UINT32 Port);

  /**
    Perform I/O port output operation (for architectures with I/O space).

    @param[in]  This  Pointer to the IHalCpu instance.
    @param[in]  Size  Size of the I/O operation (1, 2, or 4 bytes).
    @param[in]  Port  I/O port number.
    @param[in]  Value Value to write to the I/O port.
  **/
  VOID (*IoOut)(IN IHalCpu *This, IN UINT8 Size, IN UINT32 Port, IN UINTN Value);

  /**
    Relax the CPU during spin-wait operations.

    @param[in]  This  Pointer to the IHalCpu instance.
  **/
  VOID (*Relax)(IN IHalCpu *This);

  /**
    Generate a CPU trap/exception.

    @param[in]  This  Pointer to the IHalCpu instance.
  **/
  VOID (*Trap)(IN IHalCpu *This);

  /**
    Get the CPU's current cycle counter.

    @param[in]  This  Pointer to the IHalCpu instance.

    @return Current cycle count.
  **/
  UINT64 (*GetCycles)(IN IHalCpu *This);

  /**
    Set the CPU to idle mode (does not return).

    @param[in]  This  Pointer to the IHalCpu instance.
  **/
  VOID (*Idle)(IN IHalCpu *This);

  /**
    Halt the CPU (does not return).

    @param[in]  This  Pointer to the IHalCpu instance.
  **/
  VOID (*Halt)(IN IHalCpu *This);

  /**
    Perform TLB operation on the current CPU.

    @param[in]  This  Pointer to the IHalCpu instance.
    @param[in]  Op    TLB operation to perform.
  **/
  VOID (*TlbOp)(IN IHalCpu *This, IN HAL_TLBOP Op);

  /**
    Set CPU-local data pointer.

    @param[in]  This  Pointer to the IHalCpu instance.
    @param[in]  Data  Pointer to CPU-local data.
  **/
  VOID (*SetData)(IN IHalCpu *This, IN VOID *Data);

  /**
    Get CPU-local data pointer.

    @param[in]  This  Pointer to the IHalCpu instance.

    @return Pointer to CPU-local data.
  **/
  VOID *(*GetData)(IN IHalCpu *This);

  /**
    Begin user memory access (enable SMAP/SMEP workarounds).

    @param[in]  This  Pointer to the IHalCpu instance.
  **/
  VOID (*UserAccessStart)(IN IHalCpu *This);

  /**
    End user memory access (disable SMAP/SMEP workarounds).

    @param[in]  This  Pointer to the IHalCpu instance.
  **/
  VOID (*UserAccessEnd)(IN IHalCpu *This);

  /**
    Get maximum interrupt vector number available.

    @param[in]  This  Pointer to the IHalCpu instance.

    @return Maximum vector number.
  **/
  UINTN (*GetMaxVector)(IN IHalCpu *This);
};

INTERFACE_INHERIT_IUNKNOWN (IHalCpu)

//
// IHalPhysMem Interface - Physical Memory Description
//

struct _IHalPhysMemVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IHalPhysMem *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IHalPhysMem *This);
  ULONG   (*Release)(IN IHalPhysMem *This);

  //
  // IHalPhysMem Methods
  //

  /**
    Get the maximum physical page frame number.

    @param[in]  This  Pointer to the IHalPhysMem instance.

    @return Maximum PFN addressable by the hardware.
  **/
  UINTN (*GetMaxPfn)(IN IHalPhysMem *This);

  /**
    Get the maximum RAM physical page frame number.

    @param[in]  This  Pointer to the IHalPhysMem instance.

    @return Maximum PFN containing RAM.
  **/
  UINTN (*GetMaxRamPfn)(IN IHalPhysMem *This);

  /**
    Get the number of memory regions.

    @param[in]  This  Pointer to the IHalPhysMem instance.

    @return Number of physical memory regions.
  **/
  UINTN (*GetNumRegions)(IN IHalPhysMem *This);

  /**
    Get a specific memory region descriptor.

    @param[in]  This   Pointer to the IHalPhysMem instance.
    @param[in]  Index  Region index.

    @return Pointer to the region descriptor, or NULL if invalid index.
  **/
  struct apxh_region *(*GetRegion)(IN IHalPhysMem *This, IN UINTN Index);

  /**
    Get the S-tree allocation bitmap.

    @param[in]  This   Pointer to the IHalPhysMem instance.
    @param[out] Order Receives the order (log2 size) of the bitmap.

    @return Pointer to the S-tree bitmap.
  **/
  VOID *(*GetStree)(IN IHalPhysMem *This, OUT UINTN *Order);
};

INTERFACE_INHERIT_IUNKNOWN (IHalPhysMem)

//
// IHalVirtMem Interface - Virtual Memory Description
//

struct _IHalVirtMemVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IHalVirtMem *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IHalVirtMem *This);
  ULONG   (*Release)(IN IHalVirtMem *This);

  //
  // IHalVirtMem Methods
  //

  /**
    Get user area base address.

    @param[in]  This  Pointer to the IHalVirtMem instance.

    @return Base address of user virtual memory region.
  **/
  VIRTUAL_ADDRESS (*GetUserBase)(IN IHalVirtMem *This);

  /**
    Get user area size.

    @param[in]  This  Pointer to the IHalVirtMem instance.

    @return Size of user virtual memory region in bytes.
  **/
  UINTN (*GetUserSize)(IN IHalVirtMem *This);

  /**
    Get direct physical map base address.

    @param[in]  This  Pointer to the IHalVirtMem instance.

    @return Base address of direct physical mapping.
  **/
  VIRTUAL_ADDRESS (*GetDmapBase)(IN IHalVirtMem *This);

  /**
    Get direct physical map size.

    @param[in]  This  Pointer to the IHalVirtMem instance.

    @return Size of direct physical mapping in bytes.
  **/
  UINTN (*GetDmapSize)(IN IHalVirtMem *This);

  /**
    Get PFN cache area base address.

    @param[in]  This  Pointer to the IHalVirtMem instance.

    @return Base address of PFN cache region.
  **/
  VIRTUAL_ADDRESS (*GetPfnCacheBase)(IN IHalVirtMem *This);

  /**
    Get PFN cache area size.

    @param[in]  This  Pointer to the IHalVirtMem instance.

    @return Size of PFN cache region in bytes.
  **/
  UINTN (*GetPfnCacheSize)(IN IHalVirtMem *This);

  /**
    Get kernel virtual area base address.

    @param[in]  This  Pointer to the IHalVirtMem instance.

    @return Base address of kernel virtual area.
  **/
  VIRTUAL_ADDRESS (*GetKvaBase)(IN IHalVirtMem *This);

  /**
    Get kernel virtual area size.

    @param[in]  This  Pointer to the IHalVirtMem instance.

    @return Size of kernel virtual area in bytes.
  **/
  UINTN (*GetKvaSize)(IN IHalVirtMem *This);

  /**
    Get kernel memory area base address.

    @param[in]  This  Pointer to the IHalVirtMem instance.

    @return Base address of kernel memory region.
  **/
  VIRTUAL_ADDRESS (*GetKmemBase)(IN IHalVirtMem *This);

  /**
    Get kernel memory area size.

    @param[in]  This  Pointer to the IHalVirtMem instance.

    @return Size of kernel memory region in bytes.
  **/
  UINTN (*GetKmemSize)(IN IHalVirtMem *This);

  /**
    Get boot-time user entry point.

    @param[in]  This  Pointer to the IHalVirtMem instance.

    @return Entry point address, or 0 if no boot-time user process.
  **/
  VIRTUAL_ADDRESS (*GetUserEntry)(IN IHalVirtMem *This);
};

INTERFACE_INHERIT_IUNKNOWN (IHalVirtMem)

//
// IHalMap Interface - Virtual Memory Mapping
//

struct _IHalMapVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IHalMap *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IHalMap *This);
  ULONG   (*Release)(IN IHalMap *This);

  //
  // IHalMap Methods
  //

  /**
    Get Level 1 page table pointer for kernel mapping.

    @param[in]  This   Pointer to the IHalMap instance.
    @param[in]  Va     Virtual address.
    @param[in]  Alloc  If TRUE, allocate page tables as needed.
    @param[out] pL1p   Receives the L1 page table pointer.

    @retval TRUE   L1P was found or allocated.
    @retval FALSE  L1P not found or allocation failed.
  **/
  BOOLEAN (*KmapGetL1p)(IN IHalMap *This, IN UINTN Va, IN BOOLEAN Alloc, OUT hal_l1p_t *pL1p);

  /**
    Initialize an empty user mapping.

    @param[in]  This  Pointer to the IHalMap instance.
    @param[out] Umap Pointer to UMAP structure to initialize.
  **/
  VOID (*UmapInit)(IN IHalMap *This, OUT struct hal_umap *Umap);

  /**
    Save current user mappings (called at boot).

    @param[in]  This  Pointer to the IHalMap instance.
    @param[out] Umap Pointer to UMAP structure to receive mappings.
  **/
  VOID (*UmapBootstrap)(IN IHalMap *This, OUT struct hal_umap *Umap);

  /**
    Load user mappings into current CPU.

    @param[in]  This  Pointer to the IHalMap instance.
    @param[in]  Umap Pointer to UMAP to load, or NULL to unmap user space.

    @return Required TLB operation.
  **/
  HAL_TLBOP (*UmapLoad)(IN IHalMap *This, IN struct hal_umap *Umap OPTIONAL);

  /**
    Get Level 1 page table pointer for user mapping.

    @param[in]  This   Pointer to the IHalMap instance.
    @param[in]  Umap  Pointer to UMAP, or NULL for current CPU mappings.
    @param[in]  Uaddr  User address.
    @param[in]  Alloc  If TRUE, allocate page tables as needed.
    @param[out] pL1p   Receives the L1 page table pointer.

    @retval TRUE   L1P was found or allocated.
    @retval FALSE  L1P not found or allocation failed.
  **/
  BOOLEAN (*UmapGetL1p)(IN IHalMap *This, IN struct hal_umap *Umap OPTIONAL,
                        IN USER_ADDRESS Uaddr, IN BOOLEAN Alloc, OUT hal_l1p_t *pL1p);

  /**
    Find next mapped user address.

    @param[in]  This   Pointer to the IHalMap instance.
    @param[in]  Umap  Pointer to UMAP, or NULL for current CPU mappings.
    @param[in]  Uaddr  Starting user address.
    @param[out] pL1p   Optional, receives L1P for the address.
    @param[out] pL1e   Optional, receives L1E for the address.

    @return Next mapped user address, or UADDR_INVALID if none.
  **/
  USER_ADDRESS (*UmapNext)(IN IHalMap *This, IN struct hal_umap *Umap OPTIONAL,
                           IN USER_ADDRESS Uaddr, OUT hal_l1p_t *pL1p OPTIONAL,
                           OUT hal_l1e_t *pL1e OPTIONAL);

  /**
    Free all memory associated with user mapping.

    @param[in]  This  Pointer to the IHalMap instance.
    @param[in]  Umap Pointer to UMAP to free.
  **/
  VOID (*UmapFree)(IN IHalMap *This, IN struct hal_umap *Umap);

  /**
    Create a page table entry.

    @param[in]  This  Pointer to the IHalMap instance.
    @param[in]  Pfn   Physical frame number.
    @param[in]  Flags Protection and attribute flags.

    @return Encoded page table entry.
  **/
  hal_l1e_t (*L1eBox)(IN IHalMap *This, IN UINTN Pfn, IN UINTN Flags);

  /**
    Decode a page table entry.

    @param[in]  This  Pointer to the IHalMap instance.
    @param[in]  L1e   Page table entry to decode.
    @param[out] Pfn  Receives the physical frame number.
    @param[out] Prot Receives the protection flags.
  **/
  VOID (*L1eUnbox)(IN IHalMap *This, IN hal_l1e_t L1e, OUT UINTN *Pfn, OUT UINTN *Prot);

  /**
    Determine required TLB operation for page table change.

    @param[in]  This   Pointer to the IHalMap instance.
    @param[in]  OldL1e Previous page table entry.
    @param[in]  NewL1e New page table entry.

    @return Required TLB operation.
  **/
  HAL_TLBOP (*L1eTlbOp)(IN IHalMap *This, IN hal_l1e_t OldL1e, IN hal_l1e_t NewL1e);

  /**
    Get page table entry from L1 pointer.

    @param[in]  This  Pointer to the IHalMap instance.
    @param[in]  L1p   Level 1 page table pointer.

    @return Current page table entry.
  **/
  hal_l1e_t (*L1eGet)(IN IHalMap *This, IN hal_l1p_t L1p);

  /**
    Set page table entry at L1 pointer.

    @param[in]  This   Pointer to the IHalMap instance.
    @param[in]  L1p    Level 1 page table pointer.
    @param[in]  NewL1e New page table entry.

    @return Previous page table entry.
  **/
  hal_l1e_t (*L1eSet)(IN IHalMap *This, IN hal_l1p_t L1p, IN hal_l1e_t NewL1e);
};

INTERFACE_INHERIT_IUNKNOWN (IHalMap)

//
// IHalPcpu Interface - Physical CPU Management
//

struct _IHalPcpuVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IHalPcpu *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IHalPcpu *This);
  ULONG   (*Release)(IN IHalPcpu *This);

  //
  // IHalPcpu Methods
  //

  /**
    Initialize the PCPU subsystem.

    @param[in]  This  Pointer to the IHalPcpu instance.
  **/
  VOID (*Init)(IN IHalPcpu *This);

  /**
    Add a physical CPU.

    @param[in]  This     Pointer to the IHalPcpu instance.
    @param[in]  PcpuId   Physical CPU identifier.
    @param[in]  HalData HAL-specific CPU data structure.
  **/
  VOID (*Add)(IN IHalPcpu *This, IN UINTN PcpuId, IN struct hal_cpu *HalData);

  /**
    Load HAL-specific state for current CPU.

    @param[in]  This   Pointer to the IHalPcpu instance.
    @param[in]  PcpuId Physical CPU identifier.
  **/
  VOID (*Enter)(IN IHalPcpu *This, IN UINTN PcpuId);

  /**
    Get bootstrap start address for a physical CPU.

    @param[in]  This   Pointer to the IHalPcpu instance.
    @param[in]  PcpuId Physical CPU identifier.

    @return Start address, or PADDR_INVALID if CPU cannot boot.
  **/
  PHYSICAL_ADDRESS (*GetStartAddr)(IN IHalPcpu *This, IN UINTN PcpuId);
};

INTERFACE_INHERIT_IUNKNOWN (IHalPcpu)

//
// IHalFrame Interface - Interrupt Frame Management
//

struct _IHalFrameVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IHalFrame *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IHalFrame *This);
  ULONG   (*Release)(IN IHalFrame *This);

  //
  // IHalFrame Methods
  //

  /**
    Initialize a HAL frame.

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[out] Frame Frame structure to initialize.
  **/
  VOID (*Init)(IN IHalFrame *This, OUT struct hal_frame *Frame);

  /**
    Check if frame originated from user space.

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[in]  Frame Frame to check.

    @retval TRUE   Frame is from user mode.
    @retval FALSE  Frame is from kernel mode.
  **/
  BOOLEAN (*IsUser)(IN IHalFrame *This, IN struct hal_frame *Frame);

  /**
    Set instruction pointer in frame.

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[in]  Frame Frame to modify.
    @param[in]  Ip     New instruction pointer value.
  **/
  VOID (*SetIp)(IN IHalFrame *This, IN struct hal_frame *Frame, IN UINTN Ip);

  /**
    Get instruction pointer from frame.

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[in]  Frame Frame to read.

    @return Instruction pointer value.
  **/
  UINTN (*GetIp)(IN IHalFrame *This, IN struct hal_frame *Frame);

  /**
    Set stack pointer in frame.

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[in]  Frame Frame to modify.
    @param[in]  Sp     New stack pointer value.
  **/
  VOID (*SetSp)(IN IHalFrame *This, IN struct hal_frame *Frame, IN UINTN Sp);

  /**
    Get stack pointer from frame.

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[in]  Frame Frame to read.

    @return Stack pointer value.
  **/
  UINTN (*GetSp)(IN IHalFrame *This, IN struct hal_frame *Frame);

  /**
    Set global pointer in frame (RISC-V specific).

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[in]  Frame Frame to modify.
    @param[in]  Gp     New global pointer value.
  **/
  VOID (*SetGp)(IN IHalFrame *This, IN struct hal_frame *Frame, IN UINTN Gp);

  /**
    Get global pointer from frame (RISC-V specific).

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[in]  Frame Frame to read.

    @return Global pointer value.
  **/
  UINTN (*GetGp)(IN IHalFrame *This, IN struct hal_frame *Frame);

  /**
    Set argument register A0 in frame.

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[in]  Frame Frame to modify.
    @param[in]  A0     New A0 value.
  **/
  VOID (*SetA0)(IN IHalFrame *This, IN struct hal_frame *Frame, IN UINTN A0);

  /**
    Set argument register A1 in frame.

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[in]  Frame Frame to modify.
    @param[in]  A1     New A1 value.
  **/
  VOID (*SetA1)(IN IHalFrame *This, IN struct hal_frame *Frame, IN UINTN A1);

  /**
    Set argument register A2 in frame.

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[in]  Frame Frame to modify.
    @param[in]  A2     New A2 value.
  **/
  VOID (*SetA2)(IN IHalFrame *This, IN struct hal_frame *Frame, IN UINTN A2);

  /**
    Set return value register in frame.

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[in]  Frame Frame to modify.
    @param[in]  Ret    New return value.
  **/
  VOID (*SetRet)(IN IHalFrame *This, IN struct hal_frame *Frame, IN UINTN Ret);

  /**
    Set TLS pointer in frame.

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[in]  Frame Frame to modify.
    @param[in]  Tls    New TLS pointer value.
  **/
  VOID (*SetTls)(IN IHalFrame *This, IN struct hal_frame *Frame, IN UINTN Tls);

  /**
    Print frame information to log.

    @param[in]  This   Pointer to the IHalFrame instance.
    @param[in]  Frame Frame to print.
  **/
  VOID (*Print)(IN IHalFrame *This, IN struct hal_frame *Frame);
};

INTERFACE_INHERIT_IUNKNOWN (IHalFrame)

//
// Main HAL Interface - Aggregates all HAL functionality
//

struct _IHalVtbl {
  //
  // IUnknown Methods
  //
  HRESULT (*QueryInterface)(IN IHal *This, IN IID *riid, OUT VOID **ppvObject);
  ULONG   (*AddRef)(IN IHal *This);
  ULONG   (*Release)(IN IHal *This);

  //
  // IHal Methods
  //

  /**
    Get the CPU operations interface.

    @param[in]  This    Pointer to the IHal instance.
    @param[out] ppCpu   Receives the IHalCpu interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppCpu is NULL.
  **/
  HRESULT (*GetCpuInterface)(IN IHal *This, OUT IHalCpu **ppCpu);

  /**
    Get the physical memory interface.

    @param[in]  This      Pointer to the IHal instance.
    @param[out] ppPhysMem Receives the IHalPhysMem interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppPhysMem is NULL.
  **/
  HRESULT (*GetPhysMemInterface)(IN IHal *This, OUT IHalPhysMem **ppPhysMem);

  /**
    Get the virtual memory interface.

    @param[in]  This      Pointer to the IHal instance.
    @param[out] ppVirtMem Receives the IHalVirtMem interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppVirtMem is NULL.
  **/
  HRESULT (*GetVirtMemInterface)(IN IHal *This, OUT IHalVirtMem **ppVirtMem);

  /**
    Get the memory mapping interface.

    @param[in]  This   Pointer to the IHal instance.
    @param[out] ppMap  Receives the IHalMap interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppMap is NULL.
  **/
  HRESULT (*GetMapInterface)(IN IHal *This, OUT IHalMap **ppMap);

  /**
    Get the physical CPU management interface.

    @param[in]  This    Pointer to the IHal instance.
    @param[out] ppPcpu  Receives the IHalPcpu interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppPcpu is NULL.
  **/
  HRESULT (*GetPcpuInterface)(IN IHal *This, OUT IHalPcpu **ppPcpu);

  /**
    Get the frame management interface.

    @param[in]  This    Pointer to the IHal instance.
    @param[out] ppFrame Receives the IHalFrame interface pointer.

    @retval S_OK        Interface retrieved successfully.
    @retval E_POINTER   ppFrame is NULL.
  **/
  HRESULT (*GetFrameInterface)(IN IHal *This, OUT IHalFrame **ppFrame);

  /**
    Indicate that initialization is complete.

    @param[in]  This  Pointer to the IHal instance.
  **/
  VOID (*InitDone)(IN IHal *This);

  /**
    Output a character to the boot-time console.

    @param[in]  This  Pointer to the IHal instance.
    @param[in]  Char  Character to output.

    @return Number of characters written (0 or 1).
  **/
  INT32 (*PutChar)(IN IHal *This, IN INT32 Char);

  /**
    Get platform information from bootloader.

    @param[in]  This  Pointer to the IHal instance.

    @return Pointer to platform descriptor.
  **/
  CONST struct apxh_platformdesc *(*GetPlatformInfo)(IN IHal *This);

  /**
    Stop all CPUs and panic.

    @param[in]  This   Pointer to the IHal instance.
    @param[in]  Cpu    CPU number initiating panic.
    @param[in]  Error Error message.
    @param[in]  Frame Frame at time of panic, or NULL.
  **/
  VOID (*Panic)(IN IHal *This, IN UINTN Cpu, IN CONST CHAR8 *Error,
                IN struct hal_frame *Frame OPTIONAL);
};

INTERFACE_INHERIT_IUNKNOWN (IHal)

//
// Legacy C Function Wrappers (for backward compatibility)
//

extern IHal *gpHal;

static INLINE UINTN hal_cpu_in (UINT8 size, UINT32 port) {
  IHalCpu *Cpu; gpHal->lpVtbl->GetCpuInterface(gpHal, &Cpu);
  return Cpu->lpVtbl->IoIn(Cpu, size, port);
}

static INLINE void hal_cpu_out (UINT8 size, UINT32 port, UINTN val) {
  IHalCpu *Cpu; gpHal->lpVtbl->GetCpuInterface(gpHal, &Cpu);
  Cpu->lpVtbl->IoOut(Cpu, size, port, val);
}

static INLINE void hal_cpu_relax (void) {
  IHalCpu *Cpu; gpHal->lpVtbl->GetCpuInterface(gpHal, &Cpu);
  Cpu->lpVtbl->Relax(Cpu);
}

static INLINE void hal_cpu_trap (void) {
  IHalCpu *Cpu; gpHal->lpVtbl->GetCpuInterface(gpHal, &Cpu);
  Cpu->lpVtbl->Trap(Cpu);
}

static INLINE UINT64 hal_cpu_cycles (void) {
  IHalCpu *Cpu; gpHal->lpVtbl->GetCpuInterface(gpHal, &Cpu);
  return Cpu->lpVtbl->GetCycles(Cpu);
}

static INLINE void __dead hal_cpu_idle (void) {
  IHalCpu *Cpu; gpHal->lpVtbl->GetCpuInterface(gpHal, &Cpu);
  Cpu->lpVtbl->Idle(Cpu);
  __builtin_unreachable();
}

static INLINE void __dead hal_cpu_halt (void) {
  IHalCpu *Cpu; gpHal->lpVtbl->GetCpuInterface(gpHal, &Cpu);
  Cpu->lpVtbl->Halt(Cpu);
  __builtin_unreachable();
}

static INLINE void hal_cpu_tlbop (hal_tlbop_t op) {
  IHalCpu *Cpu; gpHal->lpVtbl->GetCpuInterface(gpHal, &Cpu);
  Cpu->lpVtbl->TlbOp(Cpu, op);
}

static INLINE void hal_cpu_setdata (void *data) {
  IHalCpu *Cpu; gpHal->lpVtbl->GetCpuInterface(gpHal, &Cpu);
  Cpu->lpVtbl->SetData(Cpu, data);
}

static INLINE void *hal_cpu_getdata (void) {
  IHalCpu *Cpu; gpHal->lpVtbl->GetCpuInterface(gpHal, &Cpu);
  return Cpu->lpVtbl->GetData(Cpu);
}

static INLINE void hal_useraccess_start (void) {
  IHalCpu *Cpu; gpHal->lpVtbl->GetCpuInterface(gpHal, &Cpu);
  Cpu->lpVtbl->UserAccessStart(Cpu);
}

static INLINE void hal_useraccess_end (void) {
  IHalCpu *Cpu; gpHal->lpVtbl->GetCpuInterface(gpHal, &Cpu);
  Cpu->lpVtbl->UserAccessEnd(Cpu);
}

static INLINE UINTN hal_vect_max (void) {
  IHalCpu *Cpu; gpHal->lpVtbl->GetCpuInterface(gpHal, &Cpu);
  return Cpu->lpVtbl->GetMaxVector(Cpu);
}

static INLINE unsigned long hal_physmem_maxpfn (void) {
  IHalPhysMem *pPhysMem; gpHal->lpVtbl->GetPhysMemInterface(gpHal, &pPhysMem);
  return pPhysMem->lpVtbl->GetMaxPfn(pPhysMem);
}

static INLINE unsigned long hal_physmem_maxrampfn (void) {
  IHalPhysMem *pPhysMem; gpHal->lpVtbl->GetPhysMemInterface(gpHal, &pPhysMem);
  return pPhysMem->lpVtbl->GetMaxRamPfn(pPhysMem);
}

static INLINE unsigned hal_physmem_numregions (void) {
  IHalPhysMem *pPhysMem; gpHal->lpVtbl->GetPhysMemInterface(gpHal, &pPhysMem);
  return pPhysMem->lpVtbl->GetNumRegions(pPhysMem);
}

static INLINE struct apxh_region *hal_physmem_region (unsigned i) {
  IHalPhysMem *pPhysMem; gpHal->lpVtbl->GetPhysMemInterface(gpHal, &pPhysMem);
  return pPhysMem->lpVtbl->GetRegion(pPhysMem, i);
}

static INLINE void *hal_physmem_stree (unsigned *order) {
  IHalPhysMem *pPhysMem; gpHal->lpVtbl->GetPhysMemInterface(gpHal, &pPhysMem);
  return pPhysMem->lpVtbl->GetStree(pPhysMem, order);
}

static INLINE VIRTUAL_ADDRESS hal_virtmem_userbase (void) {
  IHalVirtMem *pVirtMem; gpHal->lpVtbl->GetVirtMemInterface(gpHal, &pVirtMem);
  return pVirtMem->lpVtbl->GetUserBase(pVirtMem);
}

static INLINE CONST size_t hal_virtmem_usersize (void) {
  IHalVirtMem *pVirtMem; gpHal->lpVtbl->GetVirtMemInterface(gpHal, &pVirtMem);
  return pVirtMem->lpVtbl->GetUserSize(pVirtMem);
}

static INLINE VIRTUAL_ADDRESS hal_virtmem_dmapbase (void) {
  IHalVirtMem *pVirtMem; gpHal->lpVtbl->GetVirtMemInterface(gpHal, &pVirtMem);
  return pVirtMem->lpVtbl->GetDmapBase(pVirtMem);
}

static INLINE CONST size_t hal_virtmem_dmapsize (void) {
  IHalVirtMem *pVirtMem; gpHal->lpVtbl->GetVirtMemInterface(gpHal, &pVirtMem);
  return pVirtMem->lpVtbl->GetDmapSize(pVirtMem);
}

static INLINE VIRTUAL_ADDRESS hal_virtmem_pfn$base (void) {
  IHalVirtMem *pVirtMem; gpHal->lpVtbl->GetVirtMemInterface(gpHal, &pVirtMem);
  return pVirtMem->lpVtbl->GetPfnCacheBase(pVirtMem);
}

static INLINE CONST size_t hal_virtmem_pfn$size (void) {
  IHalVirtMem *pVirtMem; gpHal->lpVtbl->GetVirtMemInterface(gpHal, &pVirtMem);
  return pVirtMem->lpVtbl->GetPfnCacheSize(pVirtMem);
}

static INLINE VIRTUAL_ADDRESS hal_virtmem_kvabase (void) {
  IHalVirtMem *pVirtMem; gpHal->lpVtbl->GetVirtMemInterface(gpHal, &pVirtMem);
  return pVirtMem->lpVtbl->GetKvaBase(pVirtMem);
}

static INLINE CONST size_t hal_virtmem_kvasize (void) {
  IHalVirtMem *pVirtMem; gpHal->lpVtbl->GetVirtMemInterface(gpHal, &pVirtMem);
  return pVirtMem->lpVtbl->GetKvaSize(pVirtMem);
}

static INLINE VIRTUAL_ADDRESS hal_virtmem_kmembase (void) {
  IHalVirtMem *pVirtMem; gpHal->lpVtbl->GetVirtMemInterface(gpHal, &pVirtMem);
  return pVirtMem->lpVtbl->GetKmemBase(pVirtMem);
}

static INLINE CONST size_t hal_virtmem_kmemsize (void) {
  IHalVirtMem *pVirtMem; gpHal->lpVtbl->GetVirtMemInterface(gpHal, &pVirtMem);
  return pVirtMem->lpVtbl->GetKmemSize(pVirtMem);
}

static INLINE CONST VIRTUAL_ADDRESS hal_virtmem_userentry (void) {
  IHalVirtMem *pVirtMem; gpHal->lpVtbl->GetVirtMemInterface(gpHal, &pVirtMem);
  return pVirtMem->lpVtbl->GetUserEntry(pVirtMem);
}

static INLINE bool hal_kmap_getl1p (unsigned long va, bool alloc, hal_l1p_t *l1p) {
  IHalMap *pMap; gpHal->lpVtbl->GetMapInterface(gpHal, &pMap);
  return pMap->lpVtbl->KmapGetL1p(pMap, va, alloc, l1p);
}

static INLINE void hal_umap_init (struct hal_umap *umap) {
  IHalMap *pMap; gpHal->lpVtbl->GetMapInterface(gpHal, &pMap);
  pMap->lpVtbl->UmapInit(pMap, umap);
}

static INLINE void hal_umap_bootstrap (struct hal_umap *umap) {
  IHalMap *pMap; gpHal->lpVtbl->GetMapInterface(gpHal, &pMap);
  pMap->lpVtbl->UmapBootstrap(pMap, umap);
}

static INLINE hal_tlbop_t hal_umap_load (struct hal_umap *umap) {
  IHalMap *pMap; gpHal->lpVtbl->GetMapInterface(gpHal, &pMap);
  return pMap->lpVtbl->UmapLoad(pMap, umap);
}

static INLINE BOOLEAN hal_umap_getl1p (struct hal_umap *umap, USER_ADDRESS uaddr, BOOLEAN alloc, hal_l1p_t *l1p) {
  IHalMap *pMap; gpHal->lpVtbl->GetMapInterface(gpHal, &pMap);
  return pMap->lpVtbl->UmapGetL1p(pMap, umap, uaddr, alloc, l1p);
}

static INLINE USER_ADDRESS hal_umap_next (struct hal_umap *umap, USER_ADDRESS uaddr, hal_l1p_t *l1p, hal_l1e_t *l1e) {
  IHalMap *pMap; gpHal->lpVtbl->GetMapInterface(gpHal, &pMap);
  return pMap->lpVtbl->UmapNext(pMap, umap, uaddr, l1p, l1e);
}

static INLINE void hal_umap_free (struct hal_umap *umap) {
  IHalMap *pMap; gpHal->lpVtbl->GetMapInterface(gpHal, &pMap);
  pMap->lpVtbl->UmapFree(pMap, umap);
}

static INLINE hal_l1e_t hal_l1e_box (unsigned long pfn, unsigned flags) {
  IHalMap *pMap; gpHal->lpVtbl->GetMapInterface(gpHal, &pMap);
  return pMap->lpVtbl->L1eBox(pMap, pfn, flags);
}

static INLINE void hal_l1e_unbox (hal_l1e_t l1e, unsigned long *pfn, unsigned *prot) {
  IHalMap *pMap; gpHal->lpVtbl->GetMapInterface(gpHal, &pMap);
  pMap->lpVtbl->L1eUnbox(pMap, l1e, pfn, prot);
}

static INLINE hal_tlbop_t hal_l1e_tlbop (hal_l1e_t old, hal_l1e_t new) {
  IHalMap *pMap; gpHal->lpVtbl->GetMapInterface(gpHal, &pMap);
  return pMap->lpVtbl->L1eTlbOp(pMap, old, new);
}

static INLINE hal_l1e_t hal_l1e_get (hal_l1p_t l1p) {
  IHalMap *pMap; gpHal->lpVtbl->GetMapInterface(gpHal, &pMap);
  return pMap->lpVtbl->L1eGet(pMap, l1p);
}

static INLINE hal_l1e_t hal_l1e_set (hal_l1p_t l1p, hal_l1e_t new) {
  IHalMap *pMap; gpHal->lpVtbl->GetMapInterface(gpHal, &pMap);
  return pMap->lpVtbl->L1eSet(pMap, l1p, new);
}

static INLINE void hal_pcpu_init (void) {
  IHalPcpu *pPcpu; gpHal->lpVtbl->GetPcpuInterface(gpHal, &pPcpu);
  pPcpu->lpVtbl->Init(pPcpu);
}

static INLINE void hal_pcpu_add (unsigned pcpuid, struct hal_cpu *haldata) {
  IHalPcpu *pPcpu; gpHal->lpVtbl->GetPcpuInterface(gpHal, &pPcpu);
  pPcpu->lpVtbl->Add(pPcpu, pcpuid, haldata);
}

static INLINE void hal_pcpu_enter (unsigned pcpuid) {
  IHalPcpu *pPcpu; gpHal->lpVtbl->GetPcpuInterface(gpHal, &pPcpu);
  pPcpu->lpVtbl->Enter(pPcpu, pcpuid);
}

static INLINE PHYSICAL_ADDRESS hal_pcpu_startaddr (unsigned pcpu) {
  IHalPcpu *pPcpu; gpHal->lpVtbl->GetPcpuInterface(gpHal, &pPcpu);
  return pPcpu->lpVtbl->GetStartAddr(pPcpu, pcpu);
}

static INLINE void hal_frame_init (struct hal_frame *f) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  Frame->lpVtbl->Init(Frame, f);
}

static INLINE bool hal_frame_isuser (struct hal_frame *f) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  return Frame->lpVtbl->IsUser(Frame, f);
}

static INLINE void hal_frame_setip (struct hal_frame *f, unsigned long ip) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  Frame->lpVtbl->SetIp(Frame, f, ip);
}

static INLINE unsigned long hal_frame_getip (struct hal_frame *f) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  return Frame->lpVtbl->GetIp(Frame, f);
}

static INLINE void hal_frame_setsp (struct hal_frame *f, unsigned long sp) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  Frame->lpVtbl->SetSp(Frame, f, sp);
}

static INLINE unsigned long hal_frame_getsp (struct hal_frame *f) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  return Frame->lpVtbl->GetSp(Frame, f);
}

static INLINE void hal_frame_setgp (struct hal_frame *f, unsigned long gp) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  Frame->lpVtbl->SetGp(Frame, f, gp);
}

static INLINE unsigned long hal_frame_getgp (struct hal_frame *f) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  return Frame->lpVtbl->GetGp(Frame, f);
}

static INLINE void hal_frame_seta0 (struct hal_frame *f, unsigned long a0) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  Frame->lpVtbl->SetA0(Frame, f, a0);
}

static INLINE void hal_frame_seta1 (struct hal_frame *f, unsigned long a1) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  Frame->lpVtbl->SetA1(Frame, f, a1);
}

static INLINE void hal_frame_seta2 (struct hal_frame *f, unsigned long a2) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  Frame->lpVtbl->SetA2(Frame, f, a2);
}

static INLINE void hal_frame_setret (struct hal_frame *f, unsigned long r) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  Frame->lpVtbl->SetRet(Frame, f, r);
}

static INLINE void hal_frame_settls (struct hal_frame *f, unsigned long r) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  Frame->lpVtbl->SetTls(Frame, f, r);
}

static INLINE void hal_frame_print (struct hal_frame *f) {
  IHalFrame *Frame; gpHal->lpVtbl->GetFrameInterface(gpHal, &Frame);
  Frame->lpVtbl->Print(Frame, f);
}

static INLINE void hal_init_done (void) {
  gpHal->lpVtbl->InitDone(gpHal);
}

static INLINE int hal_putchar (int c) {
  return gpHal->lpVtbl->PutChar(gpHal, c);
}

static INLINE CONST struct apxh_platformdesc *hal_pltinfo (void) {
  return gpHal->lpVtbl->GetPlatformInfo(gpHal);
}

static INLINE __dead void hal_panic (unsigned cpu, CONST char *error, struct hal_frame *frame) {
  gpHal->lpVtbl->Panic(gpHal, cpu, error, frame);
  __builtin_unreachable();
}

//
// HAL Entry Points (callbacks from HAL to kernel)
//

struct hal_frame *HalEntryPageFault (IN struct hal_frame *Frame, IN UINTN Va, IN hal_pfinfo_t PfInfo);
struct hal_frame *HalEntryException (IN struct hal_frame *Frame, IN UINT32 ExceptionNumber);
struct hal_frame *HalEntryIrq (IN struct hal_frame *Frame, IN UINT32 IrqNumber, IN BOOLEAN Level);
struct hal_frame *HalEntryTimer (IN struct hal_frame *Frame);
struct hal_frame *HalEntryIpi (IN struct hal_frame *Frame);
struct hal_frame *HalEntrySyscall (IN struct hal_frame *Frame, IN UINTN Arg1,
                                   IN UINTN Arg2, IN UINTN Arg3,
                                   IN UINTN Arg4, IN UINTN Arg5,
                                   IN UINTN Arg6, IN UINTN Arg7);
VOID HalEntryNmi (IN struct hal_frame *Frame);
VOID HalMainAp (VOID);

#endif // _HAL_H
