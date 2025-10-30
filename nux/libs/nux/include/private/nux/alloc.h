/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef __nux_alloc_h__
#define __nux_alloc_h__

#include <limits.h>
#include <stdint.h>
#include <assert.h>
#include <queue.h>
#include <string.h>

#ifndef ALLOCFUNC
#define ALLOCFUNC(...) zone_##__VA_ARGS__
#endif

#ifdef ALLOCDEBUG
#define dbgprintf(...) printf (__VA_ARGS__)
#else
#define dbgprintf(...)
#endif

#ifndef __ZADDR_T
#error __ZADDR_T not defined
#else
typedef __ZADDR_T zaddr_t;
#endif

/* Basic zentry structure must contain these fields:

   struct zentry {
       LIST_ENTRY(zentry) list;
       zaddr_t addr;
       size_t size;
   };

*/

static INLINE unsigned
lsbit (unsigned long x)
{
  assert (x != 0);
  return __builtin_ffsl (x) - 1;
}

static INLINE unsigned
msbit (unsigned long x)
{
  assert (x != 0);
  return LONG_BIT - __builtin_clzl (x) - 1;
}

#define ORDMAX LONG_BIT

struct __ZENTRY;
LIST_HEAD (zlist, __ZENTRY);

/**
  Zone Allocator

  Manages a zone of memory with free list organized by size.
  Uses bitmap to track available allocation sizes.
**/
typedef struct _ZONE
{
  UINTN      Opq;             ///< Opaque user data
  unsigned long  Bmap;            ///< Bitmap of available sizes
  struct zlist   Zlist[ORDMAX];  ///< Free lists by order
  unsigned       Nfree;           ///< Number of free entries
} ZONE, *PZONE, *PCZONE;

/** Legacy type alias for compatibility **/
#define zone ZONE

static INLINE void
_zone_detachentry (struct zone *z, struct __ZENTRY *ze)
{
  UINT32 msb;

  assert (ze->size != 0);
  msb = msbit (ze->size);
  assert (msb < ORDMAX);

  LIST_REMOVE (ze, list);
  dbgprintf ("LIST_REMOVE: %p (%lx ->", ze, z->Bmap);
  if (LIST_EMPTY (z->Zlist + msb))
    z->Bmap &= ~(1UL << msb);
  dbgprintf (" %lx)", z->Bmap);
  z->Nfree -= ze->size;
  dbgprintf ("D<%p>(%lx,%lx)", ze, ze->addr, ze->size);
}

static INLINE void
_zone_attachentry (struct zone *z, struct __ZENTRY *ze)
{
  UINT32 msb;

  assert (ze->size != 0);
  msb = msbit (ze->size);
  assert (msb < ORDMAX);

  dbgprintf ("LIST_INSERT(%p + %d, %p), bmap (%lx ->", z->Zlist, msb,
	     ze, z->Bmap);
  z->Bmap |= (1UL << msb);
  dbgprintf (" %lx", z->Bmap);


  LIST_INSERT_HEAD (z->Zlist + msb, ze, list);
  z->Nfree += ze->size;
  dbgprintf ("A<%p>(%lx,%lx)", ze, ze->addr, ze->size);
}

static INLINE struct __ZENTRY *
_zone_findfree (struct zone *zn, size_t size)
{
  unsigned long tmp;
  unsigned int minbit;
  struct __ZENTRY *ze = NULL;

  minbit = msbit (size);

  if (size != (1 << minbit))
    minbit += 1;

  if (minbit >= ORDMAX)
    {
      /* Wrong size */
      return NULL;
    }

  tmp = zn->Bmap >> minbit;
  if (tmp)
    {
      tmp = lsbit (tmp);
      ze = LIST_FIRST (zn->Zlist + minbit + tmp);
      dbgprintf ("LIST_FIRST(%p + %d + %d) = %p", zn->Zlist, minbit, tmp, ze);
    }
  return ze;
}

static INLINE void
zone_remove (struct zone *z, struct __ZENTRY *ze)
{
  _zone_detachentry (z, ze);
  ___freeptr (ze, z->Opq);
}

static INLINE void
zone_create (struct zone *z, zaddr_t zaddr, size_t size)
{
  struct __ZENTRY *ze, *pze = NULL, *nze = NULL;
  zaddr_t fprev = zaddr, lnext = zaddr + size;
  dbgprintf ("HHH");
  ___get_neighbors (zaddr, size, &pze, &nze, z->Opq);
  dbgprintf ("HHH");
  if (pze)
    {
      fprev = pze->addr;
      zone_remove (z, pze);
    }
  dbgprintf ("HHH");
  if (nze)
    {
      lnext = nze->addr + nze->size;
      zone_remove (z, nze);
    }
  dbgprintf ("HHH");
  ze = ___mkptr (fprev, lnext - fprev, z->Opq);
  dbgprintf ("MKPTR(%p): %lx,%lx", ze, ze->addr, ze->size);
  _zone_attachentry (z, ze);
}


static INLINE void
zone_free (struct zone *z, zaddr_t zaddr, size_t size)
{

  assert (size != 0);
  dbgprintf ("Freeing %lx", zaddr);
  zone_create (z, zaddr, size);
}

static INLINE zaddr_t
zone_alloc (struct zone *z, size_t size)
{
  struct __ZENTRY *ze;
  zaddr_t addr = (zaddr_t) - 1;
  long diff;

  assert (size != 0);

  ze = _zone_findfree (z, size);
  if (ze == NULL)
    goto out;

  addr = ze->addr;
  diff = ze->size - size;
  assert (diff >= 0);
  zone_remove (z, ze);
  if (diff > 0)
    zone_create (z, addr + size, diff);

out:
  dbgprintf ("Allocating %lx", addr);
  return addr;
}

static INLINE void
zone_init (struct zone *z, UINTN opq)
{
  int i;

  z->Bmap = 0;
  z->Nfree = 0;
  z->Opq = opq;
  for (i = 0; i < ORDMAX; i++)
    LIST_INIT (z->Zlist + i);
}

#endif
