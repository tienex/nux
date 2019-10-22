#include <stdio.h>
#include <string.h>

#include "internal.h"
#include <nux/nux.h>

#define LO 0
#define HI 1

static lock_t brklock;
static vaddr_t base[2];
static vaddr_t brk[2];

#ifdef HAL_PAGED
#define KMEM_TYPE "Paged"
#else
#define KMEM_TYPE "Unpaged"
#endif

static int cmp (vaddr_t a, vaddr_t b)
{
  if (a > b)
    return 1;
  if (a < b)
    return -1;
  return 0;
}

#ifdef HAL_PAGED
static int
_ensure_populated (vaddr_t v1, vaddr_t v2)
{
  /* Called with bkrlock held. */
  vaddr_t s, e;

#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#define MAX(a,b) ((a) >= (b) ? (a) : (b))

  s = MIN(v1, v2);
  e = MAX(v1, v2);

  if (kmap_ensure_range (s, e - s, HAL_PFNPROT_PRESENT|HAL_PFNPROT_WRITE))
    return -1;

  kmap_commit ();
  return 0;

#undef MIN
#undef MAX
}
#endif

int
kmem_brk (int low, vaddr_t vaddr)
{
  int ret = -1;
  int this = low ? LO : HI;
  int other = low ? HI : LO;

  spinlock (&brklock);

  if (low ? vaddr < base[LO] : vaddr > base[HI])
    goto out;

  /* Check if we pass the other brk. */
  if (cmp(vaddr, brk[other]) != cmp (brk[this], brk[other]))
    goto out;

#ifdef HAL_PAGED
  if (_ensure_populated(brk[this], vaddr))
    goto out;
#endif

  brk[this] = vaddr;
  ret = 0;

 out:
  spinunlock (&brklock);
  return ret;
}

vaddr_t
kmem_sbrk (int low, long inc)
{
  int this = low ? LO : HI;
  int other = low ? HI : LO;
  vaddr_t ret = VADDR_INVALID;
  vaddr_t vaddr;

  spinlock (&brklock);
  if (inc == 0)
    {
      ret = brk[this];
      goto out;
    }

  vaddr = brk[this] + inc;

  if (low ? vaddr < base[LO] : vaddr > base[HI])
    goto out;

  /* Check if we pass the other brk. */
  if (cmp(vaddr, brk[other]) != cmp (brk[this], brk[other]))
    goto out;

#ifdef HAL_PAGED
  if (_ensure_populated(brk[this], vaddr))
    goto out;
#endif

  ret = brk[this];
  brk[this] = vaddr;

 out:
  spinunlock (&brklock);

  return ret;
}

vaddr_t
kmem_brkgrow (int low, unsigned size)
{
  vaddr_t ret;

  if (low)
    ret = kmem_sbrk (low, size);
  else {
    ret = kmem_sbrk (low, -size);
    ret = ret != VADDR_INVALID ? ret - size : ret;
  }

  return ret;
}

int
kmem_brkshrink (int low, unsigned size)
{
  vaddr_t va;
  long inc;

  inc = low ? -size : size;
  va = kmem_sbrk (low, inc);

  return va == VADDR_INVALID ? -1 : 0;
}

/*
  KMEM heap allocation.
*/

struct kmem_head {
  unsigned long magic;
  LIST_ENTRY(kmem_head) list;
  vaddr_t addr;
  size_t size;
};

struct kmem_tail {
  size_t offset;
  unsigned long magic;
};

//#define kmdbg_printf(...) printf(__VA_ARGS__)
#define kmdbg_printf(...)

#define ZONE_HEAD_MAGIC 0x616001DA
#define ZONE_TAIL_MAGIC 0x616001DA
#define __ZENTRY  kmem_head
#define __ZADDR_T vaddr_t

static struct kmem_head *
___mkptr (vaddr_t addr, size_t size, uintptr_t opq)
{
  kmdbg_printf ("making ptr at %lx (%d)\n", addr, size);

  struct kmem_head *ptr;
  struct kmem_tail *tail;

  ptr = (struct kmem_head *)addr;
  ptr->magic = ZONE_HEAD_MAGIC;
  ptr->addr = addr;
  ptr->size = size;

  tail = (struct kmem_tail *)((void *)ptr + size - sizeof(struct kmem_tail));
  kmdbg_printf ("tail at %p\n", tail);
  tail->magic = ZONE_TAIL_MAGIC;
  tail->offset = size - sizeof (struct kmem_tail);

  /* XXX: UNPAGE FREE PAGES IN THE MIDDLE. */

  return ptr;
}

static void
___freeptr (struct kmem_head *ptr, uintptr_t opq)
{
  struct kmem_tail *tail;

  memset (ptr, 0, sizeof (*ptr));

  tail = (struct kmem_tail *)((void *)ptr + ptr->size - sizeof(struct kmem_tail));
  memset (tail, 0, sizeof (*tail));

  /* XXX: ENSURE SECTION IS POPULATED. */
}

static void
___get_neighbors (vaddr_t zaddr, size_t size,
		  struct kmem_head **ph, struct kmem_head **nh,
		  uintptr_t opq)
{
  int low = opq;
  vaddr_t ptail;
  vaddr_t nhead;
  struct kmem_head *h;
  struct kmem_tail *t;

  kmdbg_printf("get neighbors %lx (%d)\n", zaddr, size);

  ptail = zaddr - sizeof (struct kmem_tail);
  nhead = zaddr + size;

  spinlock (&brklock);
  if (low)
    {
      if (ptail < base[LO])
	ptail = VADDR_INVALID;
      if (nhead + sizeof (struct kmem_head) > brk[LO])
	nhead = VADDR_INVALID;
    }
  else
    {
      if (ptail < brk[HI])
	ptail = VADDR_INVALID;
      if (nhead + sizeof (struct kmem_head) > base[HI])
	nhead = VADDR_INVALID;
    }
  spinunlock (&brklock);

  if (ptail == VADDR_INVALID)
    goto check_next;

  t = (struct kmem_tail *)ptail;
#ifdef HAL_PAGED
  if (!kmap_mapped_range ((vaddr_t)t, sizeof (struct kmem_tail)))
    goto check_next;
#endif

  if (t->magic != ZONE_TAIL_MAGIC)
    goto check_next;

  h = (struct kmem_head *)(ptail - t->offset);
#ifdef HAL_PAGED
  if (!kmap_mapped_range ((vaddr_t)h, sizeof (struct kmem_head)))
    goto check_next;
#endif

  if (h->magic != ZONE_HEAD_MAGIC)
    goto check_next;

  kmdbg_printf ("PREV = %p\n", h);
  *ph = h;

 check_next:
  
  if (nhead == VADDR_INVALID)
    return;

  h = (struct kmem_head *)nhead;
#ifdef HAL_PAGED
  if (!kmap_mapped_range ((vaddr_t)h, sizeof (struct kmem_head)))
    return;
#endif

  if (h->magic != ZONE_HEAD_MAGIC)
    return;

  kmdbg_printf ("NEXT = %p\n", h);
  *nh = h;
}

#include "alloc.h"

struct zone kmemz[2];

vaddr_t
kmem_alloc (int low, size_t size)
{
  vaddr_t r;
  struct zone *z;

  z = low ? kmemz + LO : kmemz + HI;

  r = zone_alloc (z, size);
  if (r != VADDR_INVALID)
    return r;

  r = kmem_brkgrow (low, size);
  return r;
}

void
kmem_free (int low, vaddr_t vaddr, size_t size)
{
  struct zone *z;

  z = low ? kmemz + LO : kmemz + HI;

  zone_free (z, vaddr, size);
}

void
kmeminit (void)
{
  base[LO] = hal_virtmem_kmembase();
  base[HI] = hal_virtmem_kmembase() + hal_virtmem_kmemsize();

  brk[LO] = base[LO];
  brk[HI] = base[HI];

  zone_init (kmemz + LO, 0);
  zone_init (kmemz + HI, 0);

  printf("%s KMEM from %08lx to %08lx\n", KMEM_TYPE, brk[LO], brk[HI]);
}

