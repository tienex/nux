#include <nux/hal.h>
#include <nux/types.h>
#include <nux/locks.h>
#include <nux/nux.h>
#include <assert.h>
#include <stree.h>

static lock_t lock;
static WORD_T stree[STREE_SIZE(HAL_KVA_ORDER)];
static vaddr_t kvabase;

void
kvainit(void)
{
  unsigned long i;
  assert (stree_order(HAL_KVA_SIZE >> PAGE_SHIFT) == HAL_KVA_ORDER);

  for (i = 0; i < (1 << HAL_KVA_ORDER); i++)
    stree_setbit(stree, HAL_KVA_ORDER, i);

  spinlock_init (&lock);

  kvabase = hal_virtmem_kvabase ();
}

static vaddr_t
_kva_alloc (int low)
{
  long vfn;
  vaddr_t va;


  spinlock (&lock);
  vfn = stree_bitsearch(stree, HAL_KVA_ORDER, low);
  if (vfn >= 0)
    stree_clrbit(stree, HAL_KVA_ORDER, vfn);
  spinunlock (&lock);

  if (vfn < 0)
    va = VADDR_INVALID;
  else
    va = kvabase + (vfn << PAGE_SHIFT);

  return va;
}

vaddr_t
kva_alloc (void)
{
  return _kva_alloc(0);
}

vaddr_t
kva_alloc_low (void)
{
  return _kva_alloc(1);
}

void
kva_free (vaddr_t va)
{
  vfn_t vfn;

  assert (va != VADDR_INVALID);

  vfn = va >> PAGE_SHIFT;

  spinlock (&lock);
  stree_setbit(stree, HAL_KVA_ORDER, vfn);
  spinunlock (&lock);
}
