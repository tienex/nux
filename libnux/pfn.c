/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  See COPYING file for the full license.

  SPDX-License-Identifier:	GPL2.0+
*/

#include <string.h>
#include <nux/hal.h>
#include <nux/locks.h>
#include <nux/types.h>
#include <nux/nux.h>
#include <stree.h>
#include <assert.h>

static lock_t pglock;
static WORD_T *stree;
static unsigned order;

void
pfninit (void)
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


pfn_t
pfn_alloc (int low)
{
  long pg;
  void *va;

  spinlock(&pglock);
  pg = stree_bitsearch(stree, order, low);
  if (pg >= 0)
    stree_clrbit(stree, order, pg);
  spinunlock(&pglock);

  if (pg < 0)
    return PFN_INVALID;

  va = pfn_get (pg);
  memset (va, 0, PAGE_SIZE);
  pfn_put (pg, va);
  
  return (pfn_t)pg;
}

void
pfn_free (pfn_t pfn)
{
  assert (pfn != PFN_INVALID);
  assert (pfn < hal_physmem_maxpfn ());

  spinlock(&pglock);
  stree_setbit(stree, order, pfn);
  spinunlock(&pglock);
}
