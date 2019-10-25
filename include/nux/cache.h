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

struct centry {
  rb_node_t rbn; /* Must be first. */
  uintptr_t addr;
  unsigned ref;
  TAILQ_ENTRY(centry) lru_entry;
};


static rb_tree_t cache_locked_map;
static rb_tree_t cache_free_map;
static TAILQ_HEAD(,centry) cache_free_lru;
static struct centry cache[CACHE_SLOTS];

#define cache_entry2slot(_ce) \
  (((uintptr_t)(_ce) - (uintptr_t)cache)/sizeof(struct centry))

static int
centrycmp (void *ctx, const void *a, const void *b)
{
  const struct centry *ce1 = (const struct centry *)a;
  const struct centry *ce2 = (const struct centry *)b;

  if (ce1->addr < ce2->addr)
    return -1;
  if (ce1->addr > ce2->addr)
    return 1;
  return 0;
}

static int
centry_keycmp (void *ctx, const void *a, const void *b)
{
  const struct centry *ce = (const struct centry *)a;
  const uintptr_t addr = (uintptr_t)b;

  if (ce->addr < addr)
    return -1;
  if (ce->addr > addr)
    return 1;
  return 0;
}

static const rb_tree_ops_t cacheops = {
  centrycmp,
  centry_keycmp,
  0,
  NULL
};

static inline void
cache_init (void)
{
  uintptr_t i;

  rb_tree_init (&cache_locked_map, &cacheops);
  rb_tree_init (&cache_free_map, &cacheops);
  TAILQ_INIT (&cache_free_lru);

  for (i = 0; i < CACHE_SLOTS; i++)
    {
      cache[i].addr = (uintptr_t)(-1 -i);
      cache[i].ref = 0;
      rb_tree_insert_node (&cache_free_map, cache + i);
      TAILQ_INSERT_TAIL (&cache_free_lru, cache + i, lru_entry);
    }
}

static inline uintptr_t
cache_get (uintptr_t addr)
{
  struct centry *ce;

  ce = (struct centry *)rb_tree_find_node (&cache_locked_map, (const void *)addr);
  if (ce != NULL)
    {
      /* Already present and locked, inc refcount and return. */
      ce->ref++;
      assert (ce->ref != 0);
      goto out;
    }

  ce = (struct centry *)rb_tree_find_node (&cache_free_map, (const void *)addr);
  if (ce != NULL)
    {
      /* Was currently in cache but unlocked. Lock it and return slot. */
      rb_tree_remove_node(&cache_free_map, ce);
      TAILQ_REMOVE(&cache_free_lru, ce, lru_entry);
      assert (ce->ref == 0);
      ce->ref = 1;
      rb_tree_insert_node (&cache_locked_map, ce);
      goto out;
    }

  /* Not present in the cache. Evict from free list. */
  ce = TAILQ_FIRST (&cache_free_lru);
  if (ce != NULL)
    {
      rb_tree_remove_node(&cache_free_map, ce);
      TAILQ_REMOVE(&cache_free_lru, ce, lru_entry);
      assert (ce->ref == 0);
      uintptr_t slot = cache_entry2slot(ce);
      ___cache_fill (slot, ce->addr, addr);
      ce->addr = addr;
      ce->ref = 1;
      rb_tree_insert_node (&cache_locked_map, ce);
    }

 out:
  if (ce != NULL)
    return cache_entry2slot(ce);
  else
    return (uintptr_t)-1;
}

static void
cache_put (uintptr_t slot)
{
  struct centry *ce;

  assert (slot < CACHE_SLOTS);
  ce = cache + slot;
  assert (ce->ref > 0);

  ce->ref--;
  if (!ce->ref)
    {
      /* Move to free list. */
      rb_tree_remove_node(&cache_locked_map, ce);
      rb_tree_insert_node (&cache_free_map, ce);
      TAILQ_INSERT_TAIL (&cache_free_lru, cache + slot, lru_entry);
    }
}

#undef cache_entrytoslot

#endif /* _CACHE_H */
