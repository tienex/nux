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

vaddr_t
kva_allocva (int low)
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

void
kva_freeva (vaddr_t va)
{
  vfn_t vfn;

  assert (va != VADDR_INVALID);

  vfn = va >> PAGE_SHIFT;

  spinlock (&lock);
  stree_setbit(stree, HAL_KVA_ORDER, vfn);
  spinunlock (&lock);
}
