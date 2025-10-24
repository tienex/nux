/** @file
  NUX Kernel Memory (KMEM) Allocator

  Provides low-level kernel memory allocation with dual-ended growth
  (low and high allocations from opposite ends). Uses zone allocator for
  efficient memory management and supports paged/unpaged modes.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <stdio.h>
#include <string.h>

#include "internal.h"
#include <nux.h>

#define LO 0
#define HI 1

static lock_t gBrkLock;
static vaddr_t gBase[2];
static vaddr_t gBrk[2];
static vaddr_t gMaxBrk[2];
#ifdef HAL_PAGED
static INT32 gKmemTrim = TRIM_NONE;
#endif

#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#define MAX(a,b) ((a) >= (b) ? (a) : (b))

#ifdef HAL_PAGED
#define KMEM_TYPE "Paged"
#else
#define KMEM_TYPE "Unpaged"
#endif

/**
  Compare two virtual addresses.

  @param[in] A  First virtual address.
  @param[in] B  Second virtual address.

  @retval  1  A is greater than B.
  @retval -1  A is less than B.
  @retval  0  A equals B.
**/
static INT32
Compare (
  IN vaddr_t  A,
  IN vaddr_t  B
  )
{
  if (A > B)
    return 1;
  if (A < B)
    return -1;
  return 0;
}

#ifdef HAL_PAGED
/**
  Ensure address range has specified mapping state.

  Called with gBrkLock held.

  @param[in] V1      Start virtual address.
  @param[in] V2      End virtual address.
  @param[in] Mapped  TRUE to map, FALSE to unmap.

  @retval 0   Success.
  @retval -1  Failure.
**/
static INT32
EnsureRange (
  IN vaddr_t  V1,
  IN vaddr_t  V2,
  IN INT32    Mapped
  )
{
  vaddr_t S, E;
  UINT32 Prot;

  S = MIN (V1, V2);
  E = MAX (V1, V2);
  Prot = Mapped ? HAL_PTE_P | HAL_PTE_W : 0;

  if (kmap_ensure_range (S, E - S, Prot))
    return -1;

  kmap_commit ();
  return 0;
}

/**
  Ensure address range is mapped.

  @param[in] V1  Start virtual address.
  @param[in] V2  End virtual address.

  @retval 0   Success.
  @retval -1  Failure.
**/
static INT32
EnsureRangeMapped (
  IN vaddr_t  V1,
  IN vaddr_t  V2
  )
{
  return EnsureRange (V1, V2, 1);
}

/**
  Ensure address range is unmapped.

  @param[in] V1  Start virtual address.
  @param[in] V2  End virtual address.

  @retval 0   Success.
  @retval -1  Failure.
**/
static INT32
EnsureRangeUnmapped (
  IN vaddr_t  V1,
  IN vaddr_t  V2
  )
{
  return EnsureRange (V1, V2, 0);
}
#endif

/**
  Set kernel memory break point.

  Adjusts the allocation boundary for low or high growth.

  @param[in] Low    TRUE for low growth, FALSE for high growth.
  @param[in] Vaddr  New break point address.

  @retval 0   Success.
  @retval -1  Invalid address or would collide with other break.
**/
INT32
KmemBreak (
  IN INT32    Low,
  IN vaddr_t  Vaddr
  )
{
  INT32 Ret = -1;
  INT32 This = Low ? LO : HI;
  INT32 Other = Low ? HI : LO;

  spinlock (&gBrkLock);

  if (Low ? Vaddr < gBase[LO] : Vaddr > gBase[HI])
    goto out;

  /* Check if we pass the other brk. */
  if (Compare (Vaddr, gBrk[Other]) != Compare (gBrk[This], gBrk[Other]))
    goto out;

#ifdef HAL_PAGED
  if (EnsureRangeMapped (gBrk[This], Vaddr))
    goto out;
#endif

  gBrk[This] = Vaddr;
  Ret = 0;

out:
  spinunlock (&gBrkLock);
  return Ret;
}

/**
  Increment kernel memory break point.

  Adjusts the break point by specified increment.

  @param[in] Low  TRUE for low growth, FALSE for high growth.
  @param[in] Inc  Increment value (positive or negative).

  @return Previous break point value, or VADDR_INVALID on failure.
**/
vaddr_t
KmemSbrk (
  IN INT32  Low,
  IN INT64  Inc
  )
{
  CONST INT32 This = Low ? LO : HI;
  CONST INT32 Other = Low ? HI : LO;
  vaddr_t Ret = VADDR_INVALID;
  vaddr_t Vaddr;

  spinlock (&gBrkLock);
  if (Inc == 0)
    {
      Ret = gBrk[This];
      goto out;
    }

  Vaddr = gBrk[This] + Inc;

  if (Low ? Vaddr < gBase[LO] : Vaddr > gBase[HI])
    goto out;

  /* Check if we pass the other brk. */
  if (Compare (Vaddr, gBrk[Other]) != Compare (gBrk[This], gBrk[Other]))
    goto out;

#ifdef HAL_PAGED
  if (EnsureRangeMapped (gBrk[This], Vaddr))
    goto out;
#endif

  gMaxBrk[This] = Low ? MAX (gMaxBrk[This], Vaddr) : MIN (gMaxBrk[This], Vaddr);
  Ret = gBrk[This];
  gBrk[This] = Vaddr;

out:
  spinunlock (&gBrkLock);
  return Ret;
}

/**
  Grow kernel memory allocation.

  Allocates memory by expanding the break point and returns the
  allocated address.

  @param[in] Low   TRUE for low growth, FALSE for high growth.
  @param[in] Size  Size in bytes to allocate.

  @return Allocated address, or VADDR_INVALID on failure.
**/
vaddr_t
KmemBrkGrow (
  IN INT32   Low,
  IN UINT32  Size
  )
{
  vaddr_t Ret;

  if (Low)
    Ret = KmemSbrk (Low, Size);
  else
    {
      Ret = KmemSbrk (Low, -(INT64) Size);
      Ret = Ret != VADDR_INVALID ? Ret - Size : Ret;
    }

  return Ret;
}

/**
  Shrink kernel memory allocation.

  Reduces allocated memory by moving the break point.

  @param[in] Low   TRUE for low growth, FALSE for high growth.
  @param[in] Size  Size in bytes to shrink.

  @retval 0   Success.
  @retval -1  Failure.
**/
INT32
KmemBrkShrink (
  IN INT32   Low,
  IN UINT32  Size
  )
{
  vaddr_t Va;
  INT64 Inc;

  Inc = Low ? -Size : Size;
  Va = KmemSbrk (Low, Inc);

  return Va == VADDR_INVALID ? -1 : 0;
}

/*
  KMEM heap allocation.

  Allocate in batches of 64-bytes. zaddr_t is a 64-byte index into the
  KMEM area.
*/

typedef unsigned long zaddr_t;
#define v_to_z(_v) ((_v) >> 6)
#define z_to_v(_z) ((_z) << 6)
#define zsize(_size) (v_to_z((_size) + 63))
#define size_zalign(_size) z_to_v(zsize(_size))

/**
  KMEM allocation header structure.
**/
struct kmem_head
{
  unsigned long magic;           ///< Magic number for validation
  LIST_ENTRY (kmem_head) list;   ///< List entry
  vaddr_t addr;                  ///< Starting address
  size_t size;                   ///< Allocation size
};

/**
  KMEM allocation tail structure.
**/
struct kmem_tail
{
  size_t offset;                 ///< Offset to header
  unsigned long magic;           ///< Magic number for validation
};

//#define kmdbg_printf(...) printf(__VA_ARGS__)
#define kmdbg_printf(...)

#define ZONE_HEAD_MAGIC 0x616001DA
#define ZONE_TAIL_MAGIC 0x616001DA
#define __ZENTRY  kmem_head
#define __ZADDR_T zaddr_t

/**
  Create allocation header and tail.

  @param[in] Zaddr  Zone address.
  @param[in] Size   Allocation size.
  @param[in] Opq    Opaque value (unused).

  @return Pointer to allocation header.
**/
static struct kmem_head *
___mkptr (
  IN zaddr_t    Zaddr,
  IN size_t     Size,
  IN uintptr_t  Opq
  )
{
  struct kmem_head *pPtr;
  struct kmem_tail *pTail;
  vaddr_t Addr = z_to_v (Zaddr);

  pPtr = (struct kmem_head *) Addr;
  pPtr->magic = ZONE_HEAD_MAGIC;
  pPtr->addr = Zaddr;
  pPtr->size = Size;

  pTail =
    (struct kmem_tail *) ((VOID *) pPtr + Size - sizeof (struct kmem_tail));
  pTail->magic = ZONE_TAIL_MAGIC;
  pTail->offset = Size - sizeof (struct kmem_tail);

  /* XXX: UNPAGE FREE PAGES IN THE MIDDLE. */

  return pPtr;
}

/**
  Free allocation header and tail.

  @param[in] pPtr  Pointer to allocation header.
  @param[in] Opq   Opaque value (unused).
**/
static VOID
___freeptr (
  IN struct kmem_head  *pPtr,
  IN uintptr_t         Opq
  )
{
  struct kmem_tail *pTail;

  pTail =
    (struct kmem_tail *) ((VOID *) pPtr + pPtr->size -
                          sizeof (struct kmem_tail));
  memset (pPtr, 0, sizeof (*pPtr));
  memset (pTail, 0, sizeof (*pTail));

  /* XXX: ENSURE SECTION IS POPULATED. */
}

/**
  Get neighboring allocations.

  @param[in]  Zaddr  Zone address.
  @param[in]  Size   Allocation size.
  @param[out] pPh    Pointer to receive previous allocation header.
  @param[out] pNh    Pointer to receive next allocation header.
  @param[in]  Opq    Opaque value (Low flag).
**/
static VOID
___get_neighbors (
  IN  zaddr_t           Zaddr,
  IN  size_t            Size,
  OUT struct kmem_head  **pPh,
  OUT struct kmem_head  **pNh,
  IN  uintptr_t         Opq
  )
{
  INT32 Low = Opq;
  vaddr_t Vaddr;
  vaddr_t Ptail;
  vaddr_t Nhead;
  struct kmem_head *pH;
  struct kmem_tail *pT;

  Vaddr = z_to_v (Zaddr);
  Ptail = Vaddr - sizeof (struct kmem_tail);
  Nhead = Vaddr + Size;

  spinlock (&gBrkLock);
  if (Low)
    {
      if (Ptail < gBase[LO])
        Ptail = VADDR_INVALID;
      if (Nhead + sizeof (struct kmem_head) > gBrk[LO])
        Nhead = VADDR_INVALID;
    }
  else
    {
      if (Ptail < gBrk[HI])
        Ptail = VADDR_INVALID;
      if (Nhead + sizeof (struct kmem_head) > gBase[HI])
        Nhead = VADDR_INVALID;
    }
  spinunlock (&gBrkLock);

  if (Ptail == VADDR_INVALID)
    goto check_next;

  pT = (struct kmem_tail *) Ptail;
#ifdef HAL_PAGED
  if (!kmap_mapped_range ((vaddr_t) pT, sizeof (struct kmem_tail)))
    goto check_next;
#endif

  if (pT->magic != ZONE_TAIL_MAGIC)
    goto check_next;

  pH = (struct kmem_head *) (Ptail - pT->offset);
#ifdef HAL_PAGED
  if (!kmap_mapped_range ((vaddr_t) pH, sizeof (struct kmem_head)))
    goto check_next;
#endif

  if (pH->magic != ZONE_HEAD_MAGIC)
    goto check_next;

  *pPh = pH;

check_next:

  if (Nhead == VADDR_INVALID)
    return;

  pH = (struct kmem_head *) Nhead;
#ifdef HAL_PAGED
  if (!kmap_mapped_range ((vaddr_t) pH, sizeof (struct kmem_head)))
    return;
#endif

  if (pH->magic != ZONE_HEAD_MAGIC)
    return;

  *pNh = pH;
}

#include "alloc.h"

static lock_t gLockZ[2];
static struct zone gKmemZ[2];

/**
  Allocate kernel memory.

  Allocates memory from low or high end of kernel memory region.

  @param[in] Low   TRUE for low growth, FALSE for high growth.
  @param[in] Size  Size in bytes to allocate.

  @return Allocated virtual address, or error indicator.
**/
vaddr_t
KmemAllocate (
  IN INT32   Low,
  IN size_t  Size
  )
{
  zaddr_t Zr;
  vaddr_t R;
  lock_t *pL;
  struct zone *pZ;
  size_t Size64b;

  Size64b = size_zalign (Size);

  pZ = Low ? gKmemZ + LO : gKmemZ + HI;
  pL = Low ? gLockZ + LO : gLockZ + HI;

  spinlock (pL);
  Zr = zone_alloc (pZ, zsize (Size64b));
  spinunlock (pL);
  if (Zr != (zaddr_t) - 1)
    return z_to_v (Zr);

  R = KmemBrkGrow (Low, Size64b);
  return R;
}

/**
  Free kernel memory.

  Returns allocated memory to the kernel memory allocator.

  @param[in] Low    TRUE for low growth, FALSE for high growth.
  @param[in] Vaddr  Virtual address to free.
  @param[in] Size   Size in bytes to free.
**/
VOID
KmemFree (
  IN INT32    Low,
  IN vaddr_t  Vaddr,
  IN size_t   Size
  )
{
  UINT32 This;
  vaddr_t Base;
  vaddr_t Limit;
  struct zone *pZ;
  lock_t *pL;
  size_t Size64b;

  Size64b = size_zalign (Size);

  This = Low ? LO : HI;
  Base = Low ? Vaddr : Vaddr + Size64b;
  Limit = Low ? Vaddr + Size64b : Vaddr;

  /*
     If we're freeing up to the BRK, reduce BRK allocation.
   */
  spinlock (&gBrkLock);
  kmdbg_printf ("(BRK) %lx == %lx ? ", gBrk[This], Limit);
  if (gBrk[This] == Limit)
    {
      kmdbg_printf ("BRK set to %lx\n", Base);
      gBrk[This] = Base;

      if (gKmemTrim >= TRIM_BRK)
        {
          /*
             If in TRIM mode, unmap and free unneeded pages.
           */
          vaddr_t V1 = Low ? round_page (Base) : trunc_page (Base);
          vaddr_t V2 = Low ? round_page (Limit) : trunc_page (Limit);

          kmdbg_printf ("Unmapping from [%lx-%lx] ", V1, V2);
          EnsureRangeUnmapped (V1, V2);
          kmdbg_printf (" done\n");
        }
      spinunlock (&gBrkLock);
      goto out;
    }
  spinunlock (&gBrkLock);

  /*
     Free using allocator.
   */
  pZ = gKmemZ + This;
  pL = gLockZ + This;
  spinlock (pL);
  zone_free (pZ, v_to_z (Vaddr), zsize (Size64b));
  spinunlock (pL);

out:
  return;
}

/**
  Trim kernel memory once.

  Unmaps pages between break points if trim mode is enabled.

  @param[in] TrimMode  Trim mode setting.
**/
VOID
KmemTrimOnce (
  IN UINT32  TrimMode
  )
{
  spinlock (&gBrkLock);
  if (TrimMode >= TRIM_BRK)
    {
      /* Unmap all pages between the BRKs. */
      EnsureRangeUnmapped (round_page (gBrk[LO]), round_page (gMaxBrk[LO]));
      gMaxBrk[LO] = gBrk[LO];
      EnsureRangeUnmapped (trunc_page (gBrk[HI]), trunc_page (gMaxBrk[HI]));
      gMaxBrk[HI] = gBrk[HI];
    }
  spinunlock (&gBrkLock);
}

/**
  Set kernel memory trim mode.

  Controls whether unused pages are unmapped.

  @param[in] TrimMode  New trim mode (TRIM_NONE, TRIM_BRK, etc.).
**/
VOID
KmemTrimSetMode (
  IN UINT32  TrimMode
  )
{
  kmdbg_printf ("Setting TRIM mode to %d (%s).\n",
                TrimMode,
                TrimMode == TRIM_NONE ? "off" :
                TrimMode == TRIM_BRK ? "BRK" : "unknown");

  spinlock (&gBrkLock);
  gKmemTrim = TrimMode;
  spinunlock (&gBrkLock);
}

/**
  Initialize kernel memory allocator.

  Sets up base addresses, break points, and zone allocators.
**/
VOID
KmemInitialize (
  VOID
  )
{
  gBase[LO] = hal_virtmem_kmembase ();
  gBase[HI] = hal_virtmem_kmembase () + hal_virtmem_kmemsize ();

  gBrk[LO] = gBase[LO];
  gBrk[HI] = gBase[HI];

  gMaxBrk[LO] = gBase[LO];
  gMaxBrk[HI] = gBase[HI];

  zone_init (gKmemZ + LO, 0);
  spinlock_init (gLockZ + LO);
  zone_init (gKmemZ + HI, 0);
  spinlock_init (gLockZ + HI);

  kmdbg_printf ("%s KMEM from %08lx to %08lx\n", KMEM_TYPE, gBrk[LO], gBrk[HI]);
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use Compare instead **/
static int cmp (vaddr_t a, vaddr_t b) {
  return Compare (a, b);
}

#ifdef HAL_PAGED
/** @deprecated Use EnsureRange instead **/
static int _ensure_range (vaddr_t v1, vaddr_t v2, int mapped) {
  return EnsureRange (v1, v2, mapped);
}

/** @deprecated Use EnsureRangeMapped instead **/
static int _ensure_range_mapped (vaddr_t v1, vaddr_t v2) {
  return EnsureRangeMapped (v1, v2);
}

/** @deprecated Use EnsureRangeUnmapped instead **/
static int _ensure_range_unmapped (vaddr_t v1, vaddr_t v2) {
  return EnsureRangeUnmapped (v1, v2);
}
#endif

/** @deprecated Use KmemBreak instead **/
int kmem_brk (int low, vaddr_t vaddr) {
  return KmemBreak (low, vaddr);
}

/** @deprecated Use KmemSbrk instead **/
vaddr_t kmem_sbrk (int low, long inc) {
  return KmemSbrk (low, inc);
}

/** @deprecated Use KmemBrkGrow instead **/
vaddr_t kmem_brkgrow (int low, unsigned size) {
  return KmemBrkGrow (low, size);
}

/** @deprecated Use KmemBrkShrink instead **/
int kmem_brkshrink (int low, unsigned size) {
  return KmemBrkShrink (low, size);
}

/** @deprecated Use KmemAllocate instead **/
vaddr_t kmem_alloc (int low, size_t size) {
  return KmemAllocate (low, size);
}

/** @deprecated Use KmemFree instead **/
void kmem_free (int low, vaddr_t vaddr, size_t size) {
  KmemFree (low, vaddr, size);
}

/** @deprecated Use KmemTrimOnce instead **/
void kmem_trim_one (unsigned trim_mode) {
  KmemTrimOnce (trim_mode);
}

/** @deprecated Use KmemTrimSetMode instead **/
void kmem_trim_setmode (unsigned trim_mode) {
  KmemTrimSetMode (trim_mode);
}

/** @deprecated Use KmemInitialize instead **/
void kmeminit (void) {
  KmemInitialize ();
}

// Legacy global variable aliases
static lock_t brklock __attribute__((alias("gBrkLock")));
static vaddr_t base[2] __attribute__((alias("gBase")));
static vaddr_t brk[2] __attribute__((alias("gBrk")));
static vaddr_t maxbrk[2] __attribute__((alias("gMaxBrk")));
#ifdef HAL_PAGED
static int kmem_trim __attribute__((alias("gKmemTrim")));
#endif
static lock_t lockz[2] __attribute__((alias("gLockZ")));
static struct zone kmemz[2] __attribute__((alias("gKmemZ")));
