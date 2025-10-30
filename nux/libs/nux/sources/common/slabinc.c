/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
*/

//
// Generic, simple and portable slab allocator.
//
/* *INDENT-OFF* */

#include <nux/slabinc.h>
#include <string.h>

#ifndef SLABFUNC_NAME
#define SLABFUNC_NAME "default name"
#endif
#ifndef SLABFUNC
#define SLABFUNC(_s) slab##_s
#endif
#ifndef SLABPRINT
#define SLABPRINT(...)
#endif
#ifndef SLABFATAL
#define SLABFATAL(...)
#endif

static int __slabinc_initialized = 0;
static UINTN __slabinc_size = 0;
static UINT32 __slabinc_slabs = 0;
static LIST_HEAD(slabqueue, slab) __slabinc_slabq;

#ifndef SLABMAGIC
#define SLABMAGIC 0x12211221
#endif

#ifdef DECLARE_SPIN_LOCK
DECLARE_SPIN_LOCK (__slabinc_lock);
#endif /* DECLARE_SPIN_LOCK */

struct objhdr
{
  SLIST_ENTRY (objhdr) list_entry;
};

int SLABFUNC (grow) (struct slab * sc)
{
  INT32 i;
  struct objhdr *ptr;
  struct slabhdr *sh;
  CONST unsigned long objs = ___slabobjs (sc->objsize);

  sh = ___slaballoc (&ptr);
  if (sh == NULL)
    SLABFATAL ("OOM");

  sh->magic = SLABMAGIC;
  sh->cache = sc;
  sh->freecnt = objs;
  SLIST_INIT (&sh->freeq);

  for (i = 0; i < objs; i++)
    {
      SLIST_INSERT_HEAD (&sh->freeq, ptr, list_entry);
      ptr = (struct objhdr *) ((UINT8 *) ptr + sc->objsize);
    }

  SPIN_LOCK (sc->lock);
  LIST_INSERT_HEAD (&sc->emptyq, sh, list_entry);
  sc->emptycnt++;
  SPIN_UNLOCK (sc->lock);

  return 1;
}

int SLABFUNC (shrink) (struct slab * sc)
{
  int shrunk = 0;
  struct slabhdr *sh;

  SPIN_LOCK (sc->lock);
  while (!LIST_EMPTY (&sc->emptyq))
    {
      sh = LIST_FIRST (&sc->emptyq);
      LIST_REMOVE (sh, list_entry);
      sc->emptycnt--;

      SPIN_UNLOCK (sc->lock);
      ___slabfree ((VOID *) sh);
      shrunk++;
      SPIN_LOCK (sc->lock);
    }
  SPIN_UNLOCK (sc->lock);

  return shrunk;
}

VOID *SLABFUNC (alloc_opq) (struct slab * sc, VOID *opq)
{
  int tries = 0;
  VOID *addr = NULL;
  struct objhdr *oh;
  struct slabhdr *sh = NULL;

  SPIN_LOCK (sc->lock);

retry:
  if (!LIST_EMPTY (&sc->freeq))
    {
      sh = LIST_FIRST (&sc->freeq);
      if (sh->freecnt == 1)
	{
	  LIST_REMOVE (sh, list_entry);
	  LIST_INSERT_HEAD (&sc->fullq, sh, list_entry);
	  sc->freecnt--;
	  sc->fullcnt++;
	}
    }

  if (!sh && !LIST_EMPTY (&sc->emptyq))
    {
      sh = LIST_FIRST (&sc->emptyq);
      LIST_REMOVE (sh, list_entry);
      LIST_INSERT_HEAD (&sc->freeq, sh, list_entry);
      sc->emptycnt--;
      sc->freecnt++;
    }

  if (!sh)
    {
      SPIN_UNLOCK (sc->lock);

      if (tries++ > 3)
	goto out;

      SLABFUNC (grow) (sc);

      SPIN_LOCK (sc->lock);
      goto retry;
    }

  oh = SLIST_FIRST (&sh->freeq);
  SLIST_REMOVE_HEAD (&sh->freeq, list_entry);
  sh->freecnt--;

  SPIN_UNLOCK (sc->lock);

  addr = (VOID *) oh;
  memset (addr, 0, sizeof (*oh));

  if (sc->ctr)
    sc->ctr (addr, opq, 0);

out:
  return addr;
}

VOID SLABFUNC (free) (VOID *ptr)
{
  struct slab *sc;
  struct slabhdr *sh;
  unsigned MaxObjs;

  sh = ___slabgethdr (ptr);
  if (!sh)
    return;
  sc = sh->cache;
  MaxObjs = ___slabobjs (sc->objsize);

  if (sc->ctr)
    sc->ctr (ptr, NULL, 1);

  SPIN_LOCK (sc->lock);
  SLIST_INSERT_HEAD (&sh->freeq, (struct objhdr *) ptr, list_entry);
  sh->freecnt++;

  if (sh->freecnt == 1)
    {
      LIST_REMOVE (sh, list_entry);
      LIST_INSERT_HEAD (&sc->freeq, sh, list_entry);
      sc->fullcnt--;
      sc->freecnt++;
    }
  else if (sh->freecnt == MaxObjs)
    {
      LIST_REMOVE (sh, list_entry);
      LIST_INSERT_HEAD (&sc->emptyq, sh, list_entry);
      sc->freecnt--;
      sc->emptycnt++;
    }
  SPIN_UNLOCK (sc->lock);

  return;
}

VOID
SLABFUNC (register) (struct slab * sc, PCCHAR8name, UINTN objsize,
		     VOID (*ctr) (VOID *, VOID *, INT32), INT32 cachealign)
{
#define MAX(_a,_b) ((_a) >= (_b) ? (_a) :  (_b))

  if (__slabinc_initialized == 0)
    {
      __slabinc_size = ___slabsize ();
      LIST_INIT (&__slabinc_slabq);
      SPIN_LOCK_INIT (__slabinc_lock);
      __slabinc_initialized++;
    }

  sc->name = name;

  if (cachealign)
    {
      sc->objsize = ((objsize + 63) & ~63L);
    }
  else
    {
      sc->objsize = objsize;
    }
  sc->objsize = MAX (sc->objsize, sizeof(struct objhdr));

  sc->ctr = ctr;

  sc->emptycnt = 0;
  sc->freecnt = 0;
  sc->fullcnt = 0;

  SPIN_LOCK_INIT (sc->lock);
  LIST_INIT (&sc->emptyq);
  LIST_INIT (&sc->freeq);
  LIST_INIT (&sc->fullq);

  SPIN_LOCK (__slabinc_lock);
  LIST_INSERT_HEAD (&__slabinc_slabq, sc, list_entry);
  __slabinc_slabs++;
  SPIN_UNLOCK (__slabinc_lock);
}

VOID SLABFUNC (deregister) (struct slab * sc)
{
  struct slabhdr *sh;

  while (!LIST_EMPTY (&sc->emptyq))
    {
      sh = LIST_FIRST (&sc->emptyq);
      LIST_REMOVE (sh, list_entry);

      ___slabfree ((VOID *) sh);
    }

  while (!LIST_EMPTY (&sc->freeq))
    {
      sh = LIST_FIRST (&sc->freeq);
      LIST_REMOVE (sh, list_entry);
      ___slabfree ((VOID *) sh);
    }

  while (!LIST_EMPTY (&sc->fullq))
    {
      sh = LIST_FIRST (&sc->fullq);
      LIST_REMOVE (sh, list_entry);
      ___slabfree ((VOID *) sh);
    }

  SPIN_LOCK_FREE (sc->lock);

  SPIN_LOCK (__slabinc_lock);
  LIST_REMOVE (sc, list_entry);
  __slabinc_slabs--;
  SPIN_UNLOCK (__slabinc_lock);
}

VOID SLABFUNC (printstats) (VOID)
{
  struct slab *sc;

  SLABPRINT (SLABFUNC_NAME " usage statistics:");
  SLABPRINT ("%-16s %-8s %-8s %-8s", "Name", "Empty", "Partial", "Full");
  SPIN_LOCK (__slabinc_lock);
  LIST_FOREACH (sc, &__slabinc_slabq, list_entry)
  {
    SLABPRINT ("%-16s %-8d %-8d %-8d",
	       sc->name, sc->emptycnt, sc->freecnt, sc->fullcnt);
  }
  SPIN_UNLOCK (__slabinc_lock);
}
