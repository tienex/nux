#include <stdio.h>

#include "internal.h"
#include <nux/nux.h>


#define LO 0
#define HI 1

static lock_t brklock;
static vaddr_t base[2];
static vaddr_t brk[2];

void
kmeminit (void)
{
  base[LO] = hal_virtmem_kmembase();
  base[HI] = hal_virtmem_kmembase() + hal_virtmem_kmemsize();

  brk[LO] = base[LO];
  brk[HI] = base[HI];

  printf("KMEM from %08lx to %08lx\n", brk[LO], brk[HI]);
#ifdef HAL_PAGED
  printf("KMEM is paged.\n");
#endif
}

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

vaddr_t
kmem_alloc (int low, size_t size)
{
  return VADDR_INVALID;
}

void
kmem_free (int low, vaddr_t vaddr, size_t size)
{
}
