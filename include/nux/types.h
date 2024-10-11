/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef NUX_TYPES_H
#define NUX_TYPES_H

#include "defs.h"

#include <stdbool.h>
#include <stdint.h>
#include <queue.h>

/*
  NUX Basic Types.
*/

/* paddr_t: Physical Address.  */
#define PADDR_INVALID ((paddr_t)-1)
typedef uint64_t paddr_t;

/* vaddr_t: Virtual Address.  */
#define VADDR_INVALID ((vaddr_t)-1)
typedef unsigned long vaddr_t;

/* uaddr_t: User Virtual Address. */
#define UADDR_INVALID ((uaddr_t)-1)
typedef unsigned long uaddr_t;

/* pfn_t: Physical Frame Number.  */
#define PFN_INVALID ((pfn_t)-1)
typedef unsigned long pfn_t;

/* vfn_t: Virtual Frame Number.  */
#define VFN_INVALID ((pfn_t)-1)
typedef unsigned long vfn_t;

/*

   tlbgen_t

   Almost ordered TLB generation, wrap friendly.

   This type is unsigned long: must be atomically accessible.

   There are two parts in this type: the WRAP count and the GEN count.
   Increasing a generation count is the same as an addition (the
   higher bits are essentially a wrap cound. Equality is also the same
   as an integer.  What changes is the comparison:

       A < B iff wrap(A) == wrap(B) && gen(A) < gen(B)

   I.e., we can compare A and B only if their wrap count is the same,
   otherwhise the comparation fails, and we ought to flush.

   All of this is to avoid depending on a simple integer comparison
   and shooting ourself in the foot when the integer wraps. It costs a
   few rare spurious TLB flushes.

   The trade off is that generation part should be as big as possible
   to avoid spurious TLB flushes, wrap count must be as big as
   possible to avoid wrapping the wrap count, i.e. a TLB not being
   updated flushed while the wrapcount itself has wrapped would cause
   a missed TLB flush. The latter is a much more serious and deadly
   fault.
*/
typedef unsigned long tlbgen_t;
#define _TG_WSHIFT 6		/* Wrapcount of 64. */
#define _TG_WRAP(_t) ((_t) & ((1L << _TG_WSHIFT) - 1))
#define _TG_GCNT(_t) ((_t) >> _TG_WSHIFT)


/*
  cpumask_t

  Bit array of CPUs.
*/
typedef uint64_t cpumask_t;


/*
  uctxt: User Context

  This structure that contains the CPU state of an userspace
  program. It has enough information to restore the user mode context
  and continue its execution.
*/
typedef struct hal_frame uctxt_t;


/*
  When UCTXT_IDLE context is received on entry, it means that the CPU
  woke up from idle. When this context is returned from entry, we
  request the CPU to be put idle.
*/
#define UCTXT_IDLE NULL


/*
  TLB Operations.
*/
typedef enum
{
  HAL_TLBOP_NONE = 0,		/* No TLB operation.  */
  HAL_TLBOP_FLUSH = (1 << 0),	/* Normal TLB flush.  */
  HAL_TLBOP_FLUSHALL = (1 << 1)	/* Global TLB flush.  */
}
hal_tlbop_t;


/*
  umap: User Mappings

  This structure contains a set of user-space page tables.
*/
typedef struct umap {
  cpumask_t cpumask;
  hal_tlbop_t tlbop;
  struct hal_umap hal;
} umap_t;

#endif
