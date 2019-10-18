#include <nux/hal.h>
#include <nux/locks.h>
#include <stree.h>
#include <assert.h>

lock_t pglock;
WORD_T *stree;
unsigned order;

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


long
pgalloc (void)
{
  long pg;

  spinlock(&pglock);
  pg = stree_bitsearch(stree, order, 0);
  if (pg > 0)
    stree_clrbit(stree, order, pg);
  spinunlock(&pglock);

  return pg;
}

unsigned long
pgalloc_low (void)
{
  long pg;

  spinlock(&pglock);
  pg = stree_bitsearch(stree, order, 1);
  if (pg > 0)
    stree_clrbit(stree, order, pg);
  spinunlock(&pglock);

  return pg;
}

void
pgfree (unsigned long pg)
{
  spinlock(&pglock);
  stree_setbit(stree, order, pg);
  spinunlock(&pglock);
}
