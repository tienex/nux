/** @file
  APXH x86 PAE Paging Shared Definitions

  Common definitions, constants, and function prototypes shared between
  PAE32 and PAE64 paging implementations.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __apxh_x86_pae_h__
#define __apxh_x86_pae_h__

#include <apxh/internal.h>
#include <apxh/x86.h>

//
// Type Definitions
//
typedef UINT64 pte_t;

//
// PTE (Page Table Entry) flags
//
#define PTE_P 1LL
#define PTE_W 2LL
#define PTE_U 4LL
#define PTE_PWT (1L << 3)
#define PTE_PCD (1L << 4)
#define PTE_PAT_4K (1L << 7)	/* This is for 4k leaf */
#define PTE_PS (1L << 7)	/* This is for non-leaf */
#define PTE_PAT_BIG (1 << 12)
#define PTE_NX (gNxEnabled ? 1LL << 63 : 0)

//
// PAT (Page Attribute Table) memory types
//
#define PAT_UC 3
#define PAT_WB 0
#define PAT_WC 7

//
// Externally visible globals
//
extern bool gNxEnabled;

//
// Common functions (implemented in pae_common.c)
//

/**
  Convert memory type to PTE flags.

  @param[in] Type  Memory type (MEMTYPE_*).

  @return PTE flags for the memory type.
**/
UINT64
MemtypeToFlags (
  IN int  Type
  );

/**
  Check if CPU is Intel.

  @retval TRUE   CPU is Intel.
  @retval FALSE  CPU is not Intel.
**/
bool
CpuIsIntel (
  VOID
  );

/**
  Get Intel CPU family.

  @return CPU family number.
**/
UINT32
IntelCpuFamily (
  VOID
  );

/**
  Get Intel CPU model.

  @return CPU model number.
**/
UINT32
IntelCpuModel (
  VOID
  );

/**
  Check if CPU supports PAE.

  @retval TRUE   CPU supports PAE.
  @retval FALSE  CPU does not support PAE.
**/
bool
CpuSupportsPae (
  VOID
  );

/**
  Check if CPU supports long mode (AMD64).

  @retval TRUE   CPU supports long mode.
  @retval FALSE  CPU does not support long mode.
**/
bool
CpuSupportsLongmode (
  VOID
  );

/**
  Check if CPU supports 1GB pages.

  @retval TRUE   CPU supports 1GB pages.
  @retval FALSE  CPU does not support 1GB pages.
**/
bool
CpuSupports1gbPages (
  VOID
  );

/**
  Check if CPU supports NX (No Execute) bit.

  @retval TRUE   CPU supports NX bit.
  @retval FALSE  CPU does not support NX bit.
**/
bool
CpuSupportsNx (
  VOID
  );

/**
  Set PTE (Page Table Entry).

  @param[in] pPte   Pointer to PTE.
  @param[in] pAddr  Physical address.
  @param[in] Flags  PTE flags.
**/
VOID
SetPte (
  IN UINT64   *pPte,
  IN PHYSICAL_ADDRESS  pAddr,
  IN UINT64   Flags
  );

/**
  Get physical address from PTE.

  @param[in] Pte  Page table entry.

  @return Physical address.
**/
PHYSICAL_ADDRESS
PteGetAddr (
  IN UINT64  Pte
  );

/**
  Get flags from PTE.

  @param[in] Pte  Page table entry.

  @return PTE flags.
**/
UINT64
PteGetFlags (
  IN UINT64  Pte
  );

/**
  Merge flags into PTE.

  @param[in] Pte    Original PTE.
  @param[in] Flags  Flags to merge.

  @return Updated PTE.
**/
UINT64
PteMergeFlags (
  IN UINT64  Pte,
  IN UINT64  Flags
  );

#endif // __PAE_H__
