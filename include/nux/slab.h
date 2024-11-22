#ifndef NUX_SLAB_H
#define NUX_SLAB_H

#include <nux/locks.h>

#define DECLARE_SPIN_LOCK(_x) lock_t _x
#define SPIN_LOCK_INIT(_x) do { _x = 0; } while (0)
#define SPIN_LOCK(_x) spinlock(&_x)
#define SPIN_UNLOCK(_x) spinunlock(&_x)
#define SPIN_LOCK_FREE(_x)

#include "slabinc.h"

#undef DECLARE_SPIN_LOCK
#undef SPIN_LOCK_INIT
#undef SPIN_LOCK
#undef SPIN_UNLOCK
#undef SPIN_LOCK_FREE

void
slab_register (struct slab *sc, const char *name, size_t objsize,
	       void (*ctr) (void *, void *, int), int cachealign);
void slab_deregister (struct slab *sc);
int slab_shrink (struct slab *sc);
void *slab_alloc_opq (struct slab *sc, void *opq);
void slab_free (void *ptr);
void slab_printstats (void);

static inline void *
slab_alloc (struct slab *sc)
{
  return slab_alloc_opq (sc, NULL);
}

#endif
