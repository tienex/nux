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

   tlbop_t

   TLBs flushing operations
*/
enum tlbop
{
  TLBOP_LOCAL = (1 << 0),	/* Only flush non-global TLBs mappings. */
  TLBOP_GLOBAL = (1 << 1)	/* Flush all TLBs including globals. */
};
typedef enum tlbop tlbop_t;

/* 
   umap_t

   Opaque value for Virtual User mappings.
*/
typedef void umap_t;

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

#endif
