/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef NUX_NUXPERF_H
#define NUX_NUXPERF_H

#include <string.h>
#include <nux/locks.h>

/*
  Performance Counters.
*/

#define __perf __attribute__((section(".perfctr")))

typedef struct nuxperf {
  const char *name;
  unsigned long val;
} nuxperf_t;


static inline void
nuxperf_inc(nuxperf_t *ctr)
{
  __atomic_fetch_add (&ctr->val, 1, __ATOMIC_RELAXED);
}

static inline void
nuxperf_foreach (void (*fn)(void *opq, nuxperf_t *ctr), void *opq)
{
  extern nuxperf_t _nuxperf_start[];
  extern nuxperf_t _nuxperf_end[];
  nuxperf_t *ptr = _nuxperf_start;

  while (ptr < _nuxperf_end)
    fn (opq, ptr++);
}

static inline void
nuxperf_reset (void)
{
  extern nuxperf_t _nuxperf_start[];
  extern nuxperf_t _nuxperf_end[];
  nuxperf_t *ptr = _nuxperf_start;

  while (ptr < _nuxperf_end)
    {
      *(volatile unsigned long *)&ptr->val = 0;
      ptr++;
    }
}


/*
  Performance measures.

  Measures that can provide average, maximum and minimum.
*/

#define __measure __attribute__((section(".measure"), aligned(1)))

typedef struct nuxmeasure {
  const char *name;
  volatile unsigned long lock;
  uint64_t min;
  uint64_t max;
  uint64_t avg;
  uint64_t count;
} nuxmeasure_t;

static inline void
nuxmeasure_add (nuxmeasure_t *msr, uint64_t data)
{
  while (__sync_lock_test_and_set (&msr->lock, 1))
    hal_cpu_relax ();
  msr->max = data > msr->max ? data : msr->max;
  msr->min = data < msr->min ? data : msr->min;
  msr->count++;
  msr->avg = (msr->avg * (msr->count - 1) + data) / msr->count;
  __sync_lock_release (&msr->lock);
}

static inline void
nuxmeasure_foreach (void (*fn)(void *opq, nuxmeasure_t *msr), void *opq)
{
  extern nuxmeasure_t _nuxmeasure_start[];
  extern nuxmeasure_t _nuxmeasure_end[];
  nuxmeasure_t *ptr = _nuxmeasure_start;

  while (ptr < _nuxmeasure_end)
    {
      while (__sync_lock_test_and_set (&ptr->lock, 1))
	hal_cpu_relax ();
      fn (opq, ptr);
      __sync_lock_release (&ptr->lock);
      ptr++;
    }
}

static inline void
nuxmeasure_reset (void)
{
  extern nuxmeasure_t _nuxmeasure_start[];
  extern nuxmeasure_t _nuxmeasure_end[];
  nuxmeasure_t *ptr = _nuxmeasure_start;

  /*
    Please note: following proceeds unlocked.
    Use with caution.
  */
  while (ptr < _nuxmeasure_end)
    {
      while (__sync_lock_test_and_set (&ptr->lock, 1))
	hal_cpu_relax ();

      ptr->min = -1;
      ptr->avg = 0;
      ptr->max = 0;
      ptr->count = 0;

      __sync_lock_release (&ptr->lock);
      ptr++;
    }
}

static inline void
nuxmeasure_print (void)
{
  extern nuxmeasure_t _nuxmeasure_start[];
  extern nuxmeasure_t _nuxmeasure_end[];
  nuxmeasure_t *ptr = _nuxmeasure_start;

  while (ptr < _nuxmeasure_end)
    {
      while (__sync_lock_test_and_set (&ptr->lock, 1))
	hal_cpu_relax ();

      printf ("Measure \"%s\":\n  ", ptr->name);
      printf ("min %-16d ", ptr->min);
      printf ("avg %-16d ", ptr->avg);
      printf ("max %-16d ", ptr->max);
      printf ("cnt %-16d\n", ptr->count);

      __sync_lock_release (&ptr->lock);
      ptr++;
    }
}

#define DECLARE_MEASURE(_measure)		\
  extern __measure nuxmeasure_t _measure

#define DEFINE_MEASURE(_measure)		\
  __measure nuxmeasure_t _measure = {		\
    .name = #_measure,				\
    .lock = 0,					\
    .min = -1,					\
    .max = 0,					\
    .avg = 0,					\
    .count = 0,					\
  }


/*
  Measured spinlocks.

  These helper function accumulates measures for the number of tries
  required to acquire the lock and the cycles used while holding the
  lock.
*/

typedef struct {
  nuxmeasure_t *waits;
  nuxmeasure_t *heldcy;
} lock_measure_t;

static inline void
spinlock_measured (lock_t *lock, lock_measure_t *lm)
{
  /*
    Measure the number of wait cycles.
  */
  nuxmeasure_add (lm->waits, spinlock_msr(lock));
}

static inline void spinunlock_measured (lock_t *lock, lock_measure_t *lm)
{
  /*
    Measure the number of cycles spent holding the lock.
  */
  nuxmeasure_add (lm->heldcy, spinunlock_msr(lock));
}

#define DECLARE_LOCK_MEASURE(_lock)		\
  extern lock_measure_t _lock

#define DEFINE_LOCK_MEASURE(_lock)		\
  nuxmeasure_t __measure _lock##_waits = {	\
    .name = #_lock "_waits",			\
    .lock = 0,					\
    .min = -1,					\
    .max = 0,					\
    .avg = 0,					\
    .count = 0,					\
  };						\
  nuxmeasure_t __measure _lock##_heldcy = {	\
    .name = #_lock "_heldcy",			\
    .lock = 0,					\
    .min = -1,					\
    .max = 0,					\
    .avg = 0,					\
    .count = 0,					\
  };						\
  lock_measure_t _lock = {			\
    .waits = & _lock##_waits,			\
    .heldcy = & _lock##_heldcy,			\
  }


#endif
