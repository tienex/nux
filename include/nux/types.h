/** @file
  NUX Type Definitions

  Defines fundamental types used throughout the NUX kernel library,
  including physical and virtual address types, page frame numbers,
  TLB generation tracking, CPU masks, and user context structures.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef NUX_TYPES_H
#define NUX_TYPES_H

#include "defs.h"
#include "combase.h"

#include <stdbool.h>
#include <stdint.h>
#include <queue.h>

//
// Physical Address Type
//

/** Physical Address - 64-bit physical memory address **/
typedef UINT64 PHYSICAL_ADDRESS;

#define PADDR_INVALID ((PHYSICAL_ADDRESS)-1)

/** Legacy type alias for compatibility **/
typedef PHYSICAL_ADDRESS paddr_t;

//
// Virtual Address Types
//

/** Virtual Address - kernel or user virtual address **/
typedef UINTN VIRTUAL_ADDRESS;

#define VADDR_INVALID ((VIRTUAL_ADDRESS)-1)

/** Legacy type alias for compatibility **/
typedef VIRTUAL_ADDRESS vaddr_t;

/** User Virtual Address - user-space virtual address **/
typedef UINTN USER_ADDRESS;

#define UADDR_INVALID ((USER_ADDRESS)-1)

/** Legacy type alias for compatibility **/
typedef USER_ADDRESS uaddr_t;

//
// Page Frame Number Types
//

/**
  Physical Frame Number

  Represents a physical page frame number. The actual physical address
  is calculated as: pfn << PAGE_SHIFT
**/
typedef UINTN PFN;

#define PFN_INVALID ((PFN)-1)

/** Legacy type alias for compatibility **/
typedef PFN pfn_t;

/**
  Virtual Frame Number

  Represents a virtual page frame number.
**/
typedef UINTN VFN;

#define VFN_INVALID ((VFN)-1)

/** Legacy type alias for compatibility **/
typedef VFN vfn_t;

//
// TLB Generation Counter
//

/**
  TLB Generation Counter

  Almost-ordered TLB generation counter with wrap-around protection.

  This type is UINTN and must be atomically accessible. It consists
  of two parts: a WRAP count and a GEN count.

  Incrementing the generation count is equivalent to addition. The higher
  bits serve as a wrap count. Equality comparison is standard integer
  comparison. However, ordering comparison differs:

      A < B if and only if wrap(A) == wrap(B) AND gen(A) < gen(B)

  Comparison between A and B is only valid if their wrap counts match.
  Otherwise, comparison fails and a TLB flush should be performed.

  This design avoids dependency on simple integer comparison and prevents
  issues when the counter wraps. The cost is occasional spurious TLB flushes.

  Trade-off considerations:
  - Generation part should be as large as possible to minimize spurious flushes
  - Wrap count must be large enough to prevent wrap-around between updates
  - A TLB not being flushed while the wrap count itself has wrapped would
    cause a missed TLB flush, which is a serious and potentially fatal error
**/
typedef UINTN TLB_GENERATION;

#define TG_WRAP_SHIFT   6       // Wrap count of 64
#define TG_WRAP(_t)     ((_t) & ((1L << TG_WRAP_SHIFT) - 1))
#define TG_GEN_COUNT(_t) ((_t) >> TG_WRAP_SHIFT)

/** Legacy type alias for compatibility **/
typedef TLB_GENERATION tlbgen_t;

//
// CPU Mask
//

/**
  CPU Mask

  Bit array representing a set of CPUs. Each bit corresponds to one CPU.
  Bit N set indicates CPU N is included in the mask.
**/
typedef UINT64 CPU_MASK;

/** Legacy type alias for compatibility **/
typedef CPU_MASK cpumask_t;

//
// User Context
//

/**
  User Context Structure

  Contains the CPU state of a userspace program, including all register
  values needed to restore user mode context and continue execution.

  The actual structure is architecture-specific (hal_frame).
**/
typedef struct hal_frame UCTXT;

/** Legacy type alias for compatibility **/
typedef UCTXT uctxt_t;

/**
  Special UCTXT value indicating CPU idle state.

  When UCTXT_IDLE is received on entry, the CPU woke up from idle.
  When UCTXT_IDLE is returned from entry, the CPU should enter idle state.
**/
#define UCTXT_IDLE NULL

//
// TLB Operations
//

/**
  TLB Operation Type

  Specifies the type of TLB flush operation to perform.
**/
typedef enum {
  HalTlbOpNone     = 0,          ///< No TLB operation required
  HalTlbOpFlush    = (1 << 0),   ///< Normal TLB flush (non-global entries)
  HalTlbOpFlushAll = (1 << 1)    ///< Global TLB flush (all entries)
} HAL_TLBOP;

/** Legacy type alias for compatibility **/
typedef HAL_TLBOP hal_tlbop_t;

//
// User Mapping Structure
//

/**
  User Address Space Mappings

  Contains a set of user-space page tables and tracking information
  for TLB coherency across multiple CPUs.
**/
typedef struct _UMAP {
  CPU_MASK        CpuMask;    ///< CPUs that have loaded this mapping
  HAL_TLBOP       TlbOp;      ///< Required TLB operation for coherency
  struct hal_umap Hal;        ///< HAL-specific mapping data
} UMAP;

/** Legacy type alias for compatibility **/
typedef UMAP umap_t;

//
// Kernel Symbol Structure
//

/**
  Kernel Symbol Entry

  Represents a kernel symbol for runtime symbol resolution.
  An array of these structures is generated at compile time.
**/
typedef struct _NUX_KSYM {
  UINTN       Address;    ///< Symbol address
  CONST CHAR8 *Name;      ///< Symbol name (null-terminated string)
} NUX_KSYM;

/** Legacy type alias for compatibility **/
struct nux_ksym {
  unsigned long addr;
  const char    *name;
};

#endif // NUX_TYPES_H
