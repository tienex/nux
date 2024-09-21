
/* Do not include this file directly. Must be always included indirectly. */
#ifndef SLABINC_H
#define SLABINC_H

#include <queue.h>
#include <stddef.h>

/* Simple and portable slab allocator.  */

/*
  Remember to define DECLARE_SPIN_LOCK if you want to use the lock
  extensions. E.g.:

  #define DECLARE_SPINLOCK(_x) spin_lock_type _x

  The other definitions to add are:

  #define SPIN_LOCK_INIT(_x) <initialise spinlock _x>
  #define SPIN_LOCK(_x)      <acquire spinlock _x>
  #define SPIN_UNLOCK(_x)    <release spinlock _x>
  #define SPIN_LOCK_FREE(_x) <destroy the spinlock _x>  
*/

struct slab;
struct objhdr;

struct slabhdr
{
  union
  {
    struct
    {
      unsigned long magic;
      struct slab *cache;
      unsigned freecnt;
        SLIST_HEAD (, objhdr) freeq;

        LIST_ENTRY (slabhdr) list_entry;
    };
    char cacheline[64];
  };
};

struct slab
{
#ifdef DECLARE_SPIN_LOCK
  DECLARE_SPIN_LOCK (lock);
#endif
  const char *name;
  size_t objsize;
  void (*ctr) (void *obj, void *opq, int dec);
  unsigned emptycnt;
  unsigned freecnt;
  unsigned fullcnt;
    LIST_HEAD (, slabhdr) emptyq;
    LIST_HEAD (, slabhdr) freeq;
    LIST_HEAD (, slabhdr) fullq;

    LIST_ENTRY (slab) list_entry;
};

#endif /* SLABINC_H */
