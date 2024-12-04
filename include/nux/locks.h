/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef NUX_LOCKS_H
#define NUX_LOCKS_H

#include <nux/hal.h>

typedef volatile int lock_t;

typedef struct
{
  unsigned r;
  lock_t lr, lg;
}
rwlock_t;

static inline void
spinlock_init (lock_t * l)
{
  *l = 0;
}

static inline void
spinlock (lock_t * l)
{
  while (__sync_lock_test_and_set (l, 1))
    hal_cpu_relax ();
}

static inline void
spinunlock (lock_t * l)
{
  __sync_lock_release (l);
}

static inline void
rwlock_init (rwlock_t * rw)
{
  rw->r = 0;
  spinlock_init (&rw->lr);
  spinlock_init (&rw->lg);
}

static inline void
readlock (rwlock_t * rw)
{
  spinlock (&rw->lr);
  if (rw->r++ == 0)
    spinlock (&rw->lg);
  spinunlock (&rw->lr);
}

static inline void
readunlock (rwlock_t * rw)
{
  spinlock (&rw->lr);
  if (--rw->r == 0)
    spinunlock (&rw->lg);
  spinunlock (&rw->lr);
}

static inline void
writelock (rwlock_t * rw)
{
  spinlock (&rw->lg);
}

static inline void
writeunlock (rwlock_t * rw)
{
  spinunlock (&rw->lg);
}

#endif /* NUX_LOCKS_H */
