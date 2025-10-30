/** @file
  NUX Kernel Virtual Address (KVA) Allocator

  Provides kernel virtual address space allocation and mapping services.
  Uses a red-black tree to track free virtual address ranges and integrates
  with the kernel mapping layer for physical page mapping.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <stddef.h>
#include <rbtree.h>
#include <assert.h>

#include <nux/internal.h>
#include <nux/nux.h>

//
// VM allocator
//

static rb_tree_t gVmapRbTree;
static vaddr_t gKvaBase;
static size_t gKvaSize;
static size_t gVmapSize;

/**
  Virtual memory entry structure.

  Represents a contiguous range of kernel virtual addresses.
**/
struct vme
{
  struct rb_node rb_node;      ///< Red-black tree node
  LIST_ENTRY (vme) list;       ///< List entry
  vaddr_t addr;                ///< Starting virtual address
  size_t size;                 ///< Size in bytes
};

/**
  Virtual memory map structure.

  Contains a red-black tree of virtual memory entries.
**/
struct vmap
{
  rb_tree_t rbtree;            ///< Red-black tree root
  size_t size;                 ///< Total size managed
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
  /* ASSERT ISA(vme) XXX: */
  rb_tree_remove_node (&gVmapRbTree, (VOID *) Vme);
  gVmapSize -= Vme->size;
  kmem_alloc (0, sizeof (struct vme));
}

/**
  Find virtual memory entry containing specified address.

  @param[in] Va  Virtual address to search for.

  @return Pointer to virtual memory entry, or NULL if not found.
**/
static struct vme *
VmapFind (
  IN vaddr_t  Va
  )
{
  struct vme;

  /* ASSERT ISA(vme returned) XXX: */
  return rb_tree_find_node (&gVmapRbTree, (VOID *) &Va);
}

/**
  Insert new virtual memory entry into tree.

  @param[in] Start  Starting virtual address.
  @param[in] Len    Length in bytes.

  @return Pointer to newly created virtual memory entry.
**/
static struct vme *
VmapInsert (
  IN vaddr_t  Start,
  IN size_t   Len
  )
{
  struct vme *Vme;

  Vme = (struct vme *) kmem_alloc (0, sizeof (struct vme));
  Vme->addr = Start;
  Vme->size = Len;
  rb_tree_insert_node (&gVmapRbTree, (VOID *) Vme);
  gVmapSize += Vme->size;
  return Vme;
}

/**
  Compare virtual memory entry with key for red-black tree lookup.

  @param[in] Ctx  Context pointer (unused).
  @param[in] N    Node pointer.
  @param[in] Key  Key pointer (virtual address).

  @retval  1  Key is less than node range.
  @retval -1  Key is greater than node range.
  @retval  0  Key is within node range.
**/
static INT32
VmapCompareKey (
  IN VOID        *Ctx,
  IN CONST VOID  *N,
  IN CONST VOID  *Key
  )
{
  CONST struct vme *Vme = N;
  CONST vaddr_t Va = *(CONST vaddr_t *) Key;

  if (Va < Vme->addr)
    return 1;
  if (Va > Vme->addr + (Vme->size - 1))
    return -1;
  return 0;
}

/**
  Compare two virtual memory entry nodes for red-black tree ordering.

  @param[in] Ctx  Context pointer (unused).
  @param[in] pN1   First node pointer.
  @param[in] pN2   Second node pointer.

  @retval -1  First node address is less than second.
  @retval  1  First node address is greater than second.
  @retval  0  Addresses are equal (should not happen).
**/
static INT32
VmapCompareNodes (
  IN VOID        *Ctx,
  IN CONST VOID  *pN1,
  IN CONST VOID  *pN2
  )
{
  CONST struct vme *pVmap1 = pN1;
  CONST struct vme *pVmap2 = pN2;

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

static const rb_tree_ops_t gVmapTreeOps = {
  .rbto_compare_nodes = VmapCompareNodes,
  .rbto_compare_key = VmapCompareKey,
  .rbto_node_offset = offsetof (struct vme, rb_node),
  .rbto_context = NULL
};

//
// VM allocator.
//

#define __ZENTRY vme
#define __ZADDR_T vaddr_t

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
  IN  vaddr_t      Addr,
  IN  size_t       Size,
  OUT struct vme   **Pv,
  OUT struct vme   **Nv,
  IN  uintptr_t    Opq
  )
{
  vaddr_t End = Addr + Size;
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
  IN vaddr_t     Addr,
  IN size_t      Size,
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

static lock_t gVmapLock;
static struct zone gVmapZone;

/**
  Allocate kernel virtual address range.

  Allocates a contiguous range of kernel virtual addresses without
  backing physical pages.

  @param[in] Size  Size in bytes to allocate (rounded up to page size).

  @return Starting virtual address, or VADDR_INVALID on failure.
**/
vaddr_t
KvaAllocate (
  IN size_t  Size
  )
{
  size_t PgSz;
  vaddr_t Va;

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
  IN vaddr_t  Va,
  IN size_t   Size
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
  IN pfn_t    Pfn,
  IN UINT32   Prot
  )
{
  vaddr_t Va;

  Va = KvaAllocate (PAGE_SIZE);
  if (Va == VADDR_INVALID)
    return NULL;

  kmap_map (Va, Pfn, Prot);
  kmap_commit ();
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
  IN paddr_t  Paddr,
  IN size_t   Size,
  IN UINT32   Prot
  )
{
  vaddr_t Va;
  pfn_t Pfn;
  UINT32 No, i;

  Pfn = Paddr >> PAGE_SHIFT;
  No = round_page ((Paddr & PAGE_MASK) + Size) >> PAGE_SHIFT;

  Va = KvaAllocate (No * PAGE_SIZE);
  if (Va == VADDR_INVALID)
    return NULL;

  for (i = 0; i < No; i++)
    kmap_map (Va + i * PAGE_SIZE, Pfn + i, Prot);
  kmap_commit ();

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
  IN size_t  Size
  )
{
  UINT32 No, i;
  vaddr_t Vaddr;

  Vaddr = trunc_page ((uintptr_t) Ptr);
  Size = round_page (Size);
  No = round_page ((Vaddr & PAGE_MASK) + Size) >> PAGE_SHIFT;

  for (i = 0; i < No; i++)
    kmap_unmap (Vaddr + i * PAGE_SIZE);
  kmap_commit ();

  KvaFree (Vaddr, Size);
}

/**
  Initialize kernel virtual address allocator.

  Sets up red-black tree, zone allocator, and initializes with
  available kernel virtual address range from HAL.
**/
VOID
KvaInitialize (
  VOID
  )
{
  rb_tree_init (&gVmapRbTree, &gVmapTreeOps);
  zone_init (&gVmapZone, 0);
  spinlock_init (&gVmapLock);
  gVmapSize = 0;

  gKvaBase = hal_virtmem_kvabase ();
  gKvaSize = hal_virtmem_kvasize ();
  KvaFree (gKvaBase, gKvaSize);
  info ("KVA Area from %lx to %lx", gKvaBase, gKvaBase + gKvaSize);
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use VmapRemove instead **/
static void vmap_remove (struct vme *vme) {
  VmapRemove (vme);
}

/** @deprecated Use VmapFind instead **/
static struct vme *vmap_find (vaddr_t va) {
  return VmapFind (va);
}

/** @deprecated Use VmapInsert instead **/
static struct vme *vmap_insert (vaddr_t start, size_t len) {
  return VmapInsert (start, len);
}

/** @deprecated Use VmapCompareKey instead **/
static int vmap_compare_key (void *ctx, const void *n, const void *key) {
  return VmapCompareKey (ctx, n, key);
}

/** @deprecated Use VmapCompareNodes instead **/
static int vmap_compare_nodes (void *ctx, const void *n1, const void *n2) {
  return VmapCompareNodes (ctx, n1, n2);
}

/** @deprecated Use KvaAllocate instead **/
vaddr_t kva_alloc (size_t size) {
  return KvaAllocate (size);
}

/** @deprecated Use KvaFree instead **/
void kva_free (vaddr_t va, size_t size) {
  KvaFree (va, size);
}

/** @deprecated Use KvaMap instead **/
void *kva_map (pfn_t pfn, unsigned prot) {
  return KvaMap (pfn, prot);
}

/** @deprecated Use KvaMapPhysical instead **/
void *kva_physmap (paddr_t paddr, size_t size, unsigned prot) {
  return KvaMapPhysical (paddr, size, prot);
}

/** @deprecated Use KvaUnmap instead **/
void kva_unmap (void *ptr, size_t size) {
  KvaUnmap (ptr, size);
}

/** @deprecated Use KvaInitialize instead **/
void kvainit (void) {
  KvaInitialize ();
}

// Legacy global variable aliases
static rb_tree_t vmap_rbtree __attribute__((alias("gVmapRbTree")));
static vaddr_t kvabase __attribute__((alias("gKvaBase")));
static size_t kvasize __attribute__((alias("gKvaSize")));
static size_t vmap_size __attribute__((alias("gVmapSize")));
static lock_t vmap_lock __attribute__((alias("gVmapLock")));
static struct zone vmap_zone __attribute__((alias("gVmapZone")));
