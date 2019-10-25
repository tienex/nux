#include <nux/hal.h>
#include <nux/types.h>
#include <nux/locks.h>
#include <nux/nux.h>
#include <assert.h>
#include <stree.h>

#define KVA_PAGE_SHIFT (KVA_ALLOC_ORDER + PAGE_SHIFT)
#define KVA_STREE_ORDER (HAL_KVA_SHIFT - KVA_PAGE_SHIFT)

static lock_t lock;
static WORD_T stree[STREE_SIZE(KVA_STREE_ORDER)];
static vaddr_t kvabase;
static size_t kvasize;

void
kvainit(void)
{
  unsigned long i;
  assert (stree_order(HAL_KVA_SIZE >> KVA_PAGE_SHIFT) == KVA_STREE_ORDER);
  assert (hal_virtmem_kvasize () == HAL_KVA_SIZE);

  for (i = 0; i < (1 << KVA_STREE_ORDER); i++)
    stree_setbit(stree, KVA_STREE_ORDER, i);

  spinlock_init (&lock);

  kvabase = hal_virtmem_kvabase ();
  kvasize = hal_virtmem_kvasize ();

  printf ("KVA Area from %lx to %lx\n", kvabase, kvabase + kvasize);
}

vaddr_t
kva_allocva (int low)
{
  long vfn;
  vaddr_t va;


  spinlock (&lock);
  vfn = stree_bitsearch(stree, KVA_STREE_ORDER, low);
  if (vfn >= 0)
    stree_clrbit(stree, KVA_STREE_ORDER, vfn);
  spinunlock (&lock);

  if (vfn < 0)
    va = VADDR_INVALID;
  else
    va = kvabase + (vfn << KVA_PAGE_SHIFT);

  return va;
}

void
kva_freeva (vaddr_t va)
{
  vfn_t vfn;

  assert (va != VADDR_INVALID);
  assert (va >= kvabase && va < kvabase + kvasize); 

  vfn = va >> KVA_PAGE_SHIFT;

  spinlock (&lock);
  stree_setbit(stree, KVA_STREE_ORDER, vfn);
  spinunlock (&lock);
}

void *
kva_map (pfn_t pfn, unsigned no, unsigned prot)
{
  unsigned i;
  vaddr_t va;

  if (no > (1 << KVA_ALLOC_ORDER))
    return NULL;

  va = kva_allocva (0);
  if (va == VADDR_INVALID)
    return NULL;

  for (i = 0; i < no; i++)
    kmap_map (va + i * PAGE_SIZE, pfn + i, prot);
  kmap_commit ();
}

void
kva_unmap (void *vaptr)
{
  unsigned i;
  vaddr_t va = (vaddr_t)vaptr;

  assert (va >= kvabase && va < kvabase + kvasize); 

  for (i = 0; i < (1 << KVA_ALLOC_ORDER); i++)
    kmap_map (va + i * PAGE_SIZE, 0, 0);
  kmap_commit ();

  kva_freeva (va);
}
