/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef NUX_NUXPERF_H
#define NUX_NUXPERF_H

#define __perf __attribute__((__section__(".perfctr")))

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

#endif
