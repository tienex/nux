#include <nux/hal.h>
#include <nux/locks.h>
#include <nux/types.h>
#include <stree.h>
#include <assert.h>

static lock_t pglock;
static WORD_T *stree;
static unsigned order;

void
pginit (void)
{
  long first, last;

  stree = hal_physmem_stree(&order);
  assert (stree);

  first = stree_bitsearch(stree, order, 1);
  last = stree_bitsearch(stree, order, 0);
  assert (first >= 0);
  assert (last >= 0);
  printf("Lowest physical page free:  %08lx.\n", first);
  printf("Highest physical page free: %08lx.\n", last);

  spinlock_init(&pglock);
}


static pfn_t
_pfn_alloc (int low)
{
  long pg;

  spinlock(&pglock);
  pg = stree_bitsearch(stree, order, low);
  if (pg > 0)
    stree_clrbit(stree, order, pg);
  spinunlock(&pglock);

  if (pg < 0)
    return PFN_INVALID;
  else
    return (pfn_t)pg;
}

pfn_t
pfn_alloc (void)
{
  return _pfn_alloc(0);
}

pfn_t
pfn_alloc_low (void)
{
  return _pfn_alloc(1);
}

void
pfn_free (pfn_t pfn)
{
  assert (pfn != PFN_INVALID);

  spinlock(&pglock);
  stree_setbit(stree, order, pfn);
  spinunlock(&pglock);
}
