/** @file
  NUX Kernel Virtual Address (KVA) Allocator

  Provides kernel virtual address space allocation and mapping services.
  Uses NTRTL AVL trees to track free virtual address ranges and integrates
  with the kernel mapping layer for physical page mapping.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <stddef.h>
#include <assert.h>

#include <ananke/ntrtl.h>
#include <nux/internal.h>
#include <nux/nux.h>

//
// VM allocator
//

static RTL_AVL_TREE gVmapAvlTree;
static VIRTUAL_ADDRESS gKvaBase;
static UINTN gKvaSize;
static UINTN gVmapSize;

/**
  Virtual memory entry structure.

  Represents a contiguous range of kernel virtual addresses.
**/
struct vme
{
  RTL_AVL_TREE_NODE AvlNode;   ///< NTRTL AVL tree node
  LIST_ENTRY        ListEntry; ///< List entry
  VIRTUAL_ADDRESS   addr;      ///< Starting virtual address
  UINTN             size;      ///< Size in bytes
};

/**
  Virtual memory map structure.

  Contains an NTRTL AVL tree of virtual memory entries.
**/
struct vmap
{
  RTL_AVL_TREE AvlTree;        ///< NTRTL AVL tree root
  UINTN        size;           ///< Total size managed
};

/**
  Remove virtual memory entry from tree.

  @param[in] Vme  Virtual memory entry to remove.
**/
static VOID
VmapRemove (
  IN struct vme  *Vme
  )
{
  RtlRemoveAvlTreeNode (&gVmapAvlTree, &Vme->AvlNode, TRUE);
  gVmapSize -= Vme->size;
  KmemAlloc (0, sizeof (struct vme));
}

/**
  Find virtual memory entry containing specified address using AVL tree traversal.

  @param[in] Va  Virtual address to search for.

  @return Pointer to virtual memory entry, or NULL if not found.
**/
static struct vme *
VmapFind (
  IN VIRTUAL_ADDRESS  Va
  )
{
  PRTL_AVL_TREE_NODE Current = gVmapAvlTree.Root;

  while (Current != NULL) {
    struct vme *Vme = CONTAINING_RECORD(Current, struct vme, AvlNode);

    if (Va < Vme->addr) {
      Current = Current->Left;
    } else if (Va > Vme->addr + (Vme->size - 1)) {
      Current = Current->Right;
    } else {
      return Vme;
    }
  }

  return NULL;
}

/**
  Insert new virtual memory entry into tree.

  @param[in] Start  Starting virtual address.
  @param[in] Len    Length in bytes.

  @return Pointer to newly created virtual memory entry.
**/
static struct vme *
VmapInsert (
  IN VIRTUAL_ADDRESS  Start,
  IN UINTN   Len
  )
{
  struct vme *Vme;

  Vme = (struct vme *) KmemAlloc (0, sizeof (struct vme));
  Vme->addr = Start;
  Vme->size = Len;
  RtlInitializeAvlTreeNode (&Vme->AvlNode);
  RtlInsertAvlTreeNode (&gVmapAvlTree, &Vme->AvlNode, TRUE);
  gVmapSize += Vme->size;
  return Vme;
}

/**
  Compare two virtual memory entry nodes for NTRTL AVL tree ordering.

  @param[in] Node1    First AVL tree node.
  @param[in] Node2    Second AVL tree node.
  @param[in] Context  Context pointer (unused).

  @retval -1  First node address is less than second.
  @retval  1  First node address is greater than second.
  @retval  0  Addresses are equal (should not happen).
**/
static INTN
ANXAPI
VmapCompareNodes (
  IN PRTL_AVL_TREE_NODE  Node1,
  IN PRTL_AVL_TREE_NODE  Node2,
  IN VOID                *Context OPTIONAL
  )
{
  CONST struct vme *pVmap1 = CONTAINING_RECORD(Node1, struct vme, AvlNode);
  CONST struct vme *pVmap2 = CONTAINING_RECORD(Node2, struct vme, AvlNode);

  /* Assert non overlapping */
  assert (pVmap1->addr < pVmap2->addr
          || pVmap1->addr > (pVmap2->addr + pVmap2->size));
  assert (pVmap2->addr < pVmap1->addr
          || pVmap2->addr > (pVmap1->addr + pVmap1->size));

  if (pVmap1->addr < pVmap2->addr)
    return -1;
  if (pVmap1->addr > pVmap2->addr)
    return 1;
  return 0;
}

//
// VM allocator.
//

#define __ZENTRY vme
#define __ZADDR_T VIRTUAL_ADDRESS

/**
  Get neighboring virtual memory entries.

  @param[in]  Addr  Starting address of range.
  @param[in]  Size  Size of range.
  @param[out] Pv   Pointer to receive previous entry.
  @param[out] Nv   Pointer to receive next entry.
  @param[in]  Opq   Opaque value (unused).
**/
static VOID
___get_neighbors (
  IN  VIRTUAL_ADDRESS      Addr,
  IN  UINTN       Size,
  OUT struct vme   **Pv,
  OUT struct vme   **Nv,
  IN  uintptr_t    Opq
  )
{
  VIRTUAL_ADDRESS End = Addr + Size;
  struct vme *Pvme = NULL, *Nvme = NULL;

  if (Addr == 0)
    goto _next;

  Pvme = VmapFind (Addr - 1);
  if (Pvme != NULL)
    *Pv = Pvme;

_next:
  Nvme = VmapFind (End);
  if (Nvme != NULL)
    *Nv = Nvme;
}

/**
  Create virtual memory entry pointer.

  @param[in] Addr  Starting address.
  @param[in] Size  Size in bytes.
  @param[in] Opq   Opaque value (unused).

  @return Pointer to newly created entry.
**/
static struct vme *
___mkptr (
  IN VIRTUAL_ADDRESS     Addr,
  IN UINTN      Size,
  IN uintptr_t   Opq
  )
{
  return VmapInsert (Addr, Size);
}

/**
  Free virtual memory entry pointer.

  @param[in] Vme  Virtual memory entry to free.
  @param[in] Opq   Opaque value (unused).
**/
static VOID
___freeptr (
  IN struct vme  *Vme,
  IN uintptr_t   Opq
  )
{
  VmapRemove (Vme);
}

#include <nux/alloc.h>

static SPINLOCK gVmapLock;
static struct zone gVmapZone;

/**
  Allocate kernel virtual address range.

  Allocates a contiguous range of kernel virtual addresses without
  backing physical pages.

  @param[in] Size  Size in bytes to allocate (rounded up to page size).

  @return Starting virtual address, or VADDR_INVALID on failure.
**/
VIRTUAL_ADDRESS
KvaAllocate (
  IN UINTN  Size
  )
{
  UINTN PgSz;
  VIRTUAL_ADDRESS Va;

  PgSz = round_page (Size);
  spinlock (&gVmapLock);
  Va = zone_alloc (&gVmapZone, PgSz);
  spinunlock (&gVmapLock);
  if (Va == 0)
    return VADDR_INVALID;

  return Va;
}

/**
  Free kernel virtual address range.

  Returns previously allocated virtual address range to the allocator.
  Does not unmap any backing physical pages.

  @param[in] Va    Starting virtual address (aligned to page boundary).
  @param[in] Size  Size in bytes to free (rounded up to page size).
**/
VOID
KvaFree (
  IN VIRTUAL_ADDRESS  Va,
  IN UINTN   Size
  )
{
  Va = trunc_page (Va);
  Size = round_page (Size);
  spinlock (&gVmapLock);
  zone_free (&gVmapZone, Va, Size);
  spinunlock (&gVmapLock);
}

/**
  Map single physical page to kernel virtual address.

  Allocates kernel virtual address and maps specified physical page.

  @param[in] Pfn   Page frame number to map.
  @param[in] Prot  Protection flags (HAL_PTE_*).

  @return Pointer to mapped page, or NULL on failure.
**/
VOID *
KvaMap (
  IN PFN    Pfn,
  IN UINT32   Prot
  )
{
  VIRTUAL_ADDRESS Va;

  Va = KvaAllocate (PAGE_SIZE);
  if (Va == VADDR_INVALID)
    return NULL;

  KmapMap (Va, Pfn, Prot);
  KmapCommit ();
  return (VOID *) Va;
}

/**
  Map physical memory region to kernel virtual address.

  Allocates kernel virtual address range and maps specified physical
  address range. Handles page-unaligned addresses.

  @param[in] Paddr  Physical address to map.
  @param[in] Size   Size in bytes to map.
  @param[in] Prot   Protection flags (HAL_PTE_*).

  @return Pointer to mapped memory with offset preserved, or NULL on failure.
**/
VOID *
KvaMapPhysical (
  IN PHYSICAL_ADDRESS  Paddr,
  IN UINTN   Size,
  IN UINT32   Prot
  )
{
  VIRTUAL_ADDRESS Va;
  PFN Pfn;
  UINT32 No, i;

  Pfn = Paddr >> PAGE_SHIFT;
  No = round_page ((Paddr & PAGE_MASK) + Size) >> PAGE_SHIFT;

  Va = KvaAllocate (No * PAGE_SIZE);
  if (Va == VADDR_INVALID)
    return NULL;

  for (i = 0; i < No; i++)
    KmapMap (Va + i * PAGE_SIZE, Pfn + i, Prot);
  KmapCommit ();

  return (VOID *) (uintptr_t) (Va + (Paddr & PAGE_MASK));
}

/**
  Unmap kernel virtual address range.

  Unmaps backing physical pages and frees virtual address range.

  @param[in] Ptr  Pointer to mapped memory.
  @param[in] Size  Size in bytes to unmap.
**/
VOID
KvaUnmap (
  IN VOID    *Ptr,
  IN UINTN  Size
  )
{
  UINT32 No, i;
  VIRTUAL_ADDRESS Vaddr;

  Vaddr = trunc_page ((uintptr_t) Ptr);
  Size = round_page (Size);
  No = round_page ((Vaddr & PAGE_MASK) + Size) >> PAGE_SHIFT;

  for (i = 0; i < No; i++)
    KmapUnmap (Vaddr + i * PAGE_SIZE);
  KmapCommit ();

  KvaFree (Vaddr, Size);
}

/**
  Initialize kernel virtual address allocator.

  Sets up NTRTL AVL tree, zone allocator, and initializes with
  available kernel virtual address range from HAL.
**/
VOID
KvaInitialize (
  VOID
  )
{
  RtlInitializeAvlTree (&gVmapAvlTree, VmapCompareNodes, NULL, NULL, NULL);
  zone_init (&gVmapZone, 0);
  spinlock_init (&gVmapLock);
  gVmapSize = 0;

  gKvaBase = hal_virtmem_kvabase ();
  gKvaSize = hal_virtmem_kvasize ();
  KvaFree (gKvaBase, gKvaSize);
  info ("KVA Area from %lx to %lx", gKvaBase, gKvaBase + gKvaSize);
}
