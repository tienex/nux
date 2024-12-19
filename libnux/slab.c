/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
*/

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <nux/nux.h>
#include <nux/slab.h>

#define SLAB_LOG2SZ 1		/* Must be power of 2 for alignment */
#define SLAB_SIZE ((1L << SLAB_LOG2SZ) * PAGE_SIZE)

#define SLABMAGIC 0x80763141
#define SLABFUNC_NAME "slab cache"
#define SLABFUNC(_s) slab_##_s
#define SLABPRINT(...) info(__VA_ARGS__)
#define SLABFATAL(...) fatal(__VA_ARGS__)

#define ___slabsize() SLAB_SIZE

static const size_t
___slabobjs (const size_t size)
{

  return (___slabsize () - 2 * sizeof (struct slabhdr)) / size;
}

static struct slabhdr *
___slaballoc (struct objhdr **ohptr)
{
  vaddr_t kva_una, kva_una_end, kva_start, kva_end;

  kva_una = kva_alloc (2 * SLAB_SIZE - PAGE_SIZE);
  kva_una_end = kva_una + 2 * SLAB_SIZE - PAGE_SIZE;

  if (kva_una % SLAB_SIZE)
    kva_start = (kva_una + SLAB_SIZE - 1) & ~((vaddr_t) SLAB_SIZE - 1);
  else
    kva_start = kva_una;
  kva_end = kva_start + SLAB_SIZE;

  if (kva_una < kva_start)
    {
      kva_free (kva_una, kva_start - kva_una);
    }

  if (kva_end < kva_una_end)
    {
      kva_free (kva_end, kva_una_end - kva_end);
    }

  assert (!kmap_ensure_range (kva_start, SLAB_SIZE, HAL_PTE_W | HAL_PTE_P));
  kmap_commit ();

  *ohptr = (struct objhdr *) (kva_start + sizeof (struct slabhdr));
  return (struct slabhdr *) kva_start;
}

static struct slabhdr *
___slabgethdr (void *obj)
{
  struct slabhdr *sh;
  uintptr_t addr = (uintptr_t) obj;

  sh = (struct slabhdr *) (addr & ~((uintptr_t) ___slabsize () - 1));
  if (sh->magic != SLABMAGIC)
    return NULL;
  return sh;
}

static void
___slabfree (void *ptr)
{
  kmap_ensure_range ((vaddr_t) ptr, SLAB_SIZE, 0);
  kmap_commit ();
  kva_free ((vaddr_t) ptr, SLAB_SIZE);
}

#define DECLARE_SPIN_LOCK(_x) lock_t _x
#define SPIN_LOCK_INIT(_x) spinlock_init(&_x)
#define SPIN_LOCK(_x) spinlock(&_x)
#define SPIN_UNLOCK(_x) spinunlock(&_x)
#define SPIN_LOCK_FREE(_x)

#include "slabinc.c"
