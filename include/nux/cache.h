/*
  cache.h: a simple cache implementation.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

SPDX-License-Identifier:	GPL2.0+
*/

#ifndef _CACHE_H
#define _CACHE_H

/*
  Define:

  CACHE_SLOTS: The number of slots in the cache.

  void ___cache_fill (uintptr_t slot, uintptr_t oldaddr, uintptr_t newaddr);

  The above function will evict from slot SLOT the entry at OLDADDR,
  and fill it with the new at NEWADDR.

*/

#include <assert.h>
#include <stdint.h>
#include <rbtree.h>

struct slot {
  rb_node_t rbn; /* Must be first. */
  TAILQ_ENTRY(slot) lru_entry;

  uintptr_t addr;
  struct {
    uint32_t valid :  1;
    uint32_t ref   : 31;
  };

};

struct cache {
  rb_tree_t map; /* Must be First. */
  TAILQ_HEAD(,slot) freelist;
  lock_t lock;

  void (*fill)(unsigned, uintptr_t, uintptr_t);

  unsigned numslots;
  struct slot *slots;
};

static int
_slotcmp (void *ctx, const void *a, const void *b)
{
  const struct slot *slot1 = (const struct slot *)a;
  const struct slot *slot2 = (const struct slot *)b;

  if (slot1->addr < slot2->addr)
    return -1;
  if (slot1->addr > slot2->addr)
    return 1;
  return 0;
}

static int
_slot_keycmp (void *ctx, const void *a, const void *b)
{
  const struct slot *slot = (const struct slot *)a;
  const uintptr_t addr = (uintptr_t)b;

  if (slot->addr < addr)
    return -1;
  if (slot->addr > addr)
    return 1;
  return 0;
}

static const rb_tree_ops_t cacheops = {
  _slotcmp,
  _slot_keycmp,
  0,
  NULL
};

static inline unsigned
cache_getslotno (struct cache *c, struct slot *s)
{
  return ((uintptr_t)s - (uintptr_t)(c->slots))/sizeof (struct slot);
}

static inline void
cache_init (struct cache *c,
	    struct slot *slots, unsigned numslots,
	    /* Arguments passed to fill:
	       1. Slot allocated
	       2. Old Entry
	       3. New Entry
	    */
	    void (*fill)(unsigned, uintptr_t, uintptr_t))
{
  uintptr_t i;

  spinlock_init (&c->lock);
  rb_tree_init (&c->map, &cacheops);
  TAILQ_INIT (&c->freelist);
  c->numslots = numslots;
  c->slots = slots;
  c->fill = fill;
  for (i = 0; i < numslots; i++)
    {
      slots[i].valid = 0;
      slots[i].ref = 0;
      TAILQ_INSERT_TAIL (&c->freelist, slots + i, lru_entry);
    }
}

static inline unsigned
cache_get (struct cache *c, uintptr_t addr)
{
  struct slot *slot;
  unsigned slotno;

  spinlock (&c->lock);
  slot = (struct slot *)rb_tree_find_node (&c->map, (const void *)addr);
  if (slot != NULL)
    {
      assert (slot->valid);
      if (slot->ref > 0)
	{
	  /* Already present and used, inc refcount and return. */
	  slot->ref++;
	  assert (slot->ref != 0);
	  goto out;
	}
      else
	{
	  /* Currently in cache but unused. inc refcount and return. */
	  TAILQ_REMOVE(&c->freelist, slot, lru_entry);
	  slot->ref = 1;
	  goto out;
	}
    }

  /* Not present in the cache. Evict from free list. */
  slot = TAILQ_FIRST (&c->freelist);
  if (slot != NULL)
    {
      unsigned n = cache_getslotno(c, slot);

      TAILQ_REMOVE(&c->freelist, slot, lru_entry);
      assert (slot->ref == 0);
      c->fill (n, slot->addr, addr);
      slot->addr = addr;
      slot->ref = 1;
      slot->valid = 1;
      rb_tree_insert_node (&c->map, slot);
      goto out;
    }

 out:

  if (slot != NULL)
    slotno = cache_getslotno(c, slot);
  else
    slotno = (unsigned)-1;  

  spinunlock (&c->lock);
  
  return slotno;
}

static void
cache_put (struct cache *c, uintptr_t slotno)
{
  struct slot *slot;

  spinlock (&c->lock);
  assert (slotno < c->numslots);
  slot = c->slots + slotno;

  assert (slot->ref > 0);
  slot->ref--;
  if (slot->ref == 0)
    TAILQ_INSERT_TAIL (&c->freelist, slot, lru_entry);

  spinunlock (&c->lock);
}

#endif /* _CACHE_H */
