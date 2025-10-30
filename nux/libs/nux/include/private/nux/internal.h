/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef __nux_internal_h__
#define __nux_internal_h__

#include <setjmp.h>
#include <nux/types.h>
#include <hal/hal.h>

/*
  NUX 'status' flags.
*/

#define NUXST_OKPLT   1		/* Platform initialized. */
#define NUXST_OKCPU   2		/* BSP is initialized and CPU operations are available. */
#define NUXST_RUNNING 4		/* NUX is fully initialized. */
#define NUXST_PANIC   128	/* NUX is in panic mode and shutting down. */
UINT8 nux_status (void);
UINT8 nux_status_setfl (UINT8 flags);
bool nux_status_okcpu (void);

/*
  Kernel TLB status.
*/
struct ktlb
{
  TLB_GENERATION global;		/* Global mappings. */
  TLB_GENERATION normal;		/* Non-global mappings. */
};

/*
  Return <0 if a < b. 0 if a == b, >0 if a > b or wrapcounts differ.
*/
static inline int
tlbgen_cmp (TLB_GENERATION a, TLB_GENERATION b)
{
  if (_TG_WRAP (a) == _TG_WRAP (b))
    {
      if (a < b)
	return -1;
      else if (a > b)
	return 1;
      else
	return 0;
    }
  else
    return 1;
}


/* 
   CPU management
*/

struct cpu_info
{
  unsigned cpu_id;
  unsigned phys_id;
  struct cpu_info *self;

  struct umap *umap;

  /* Idle jmp_buf */
  jmp_buf idlejmp;
  bool idle;


  /* NMI operations. */
#define NMIOP_KMAPUPDATE 1	/* Update kmap across all CPUs. */
#define NMIOP_TLBFLUSH 2	/* Flush TLBs. */
  unsigned nmiop;

  /* TLB status for current CPU. */
  volatile struct ktlb ktlb;

  /*
     User copy setjmp/longjmp for pagefaults.
   */
  jmp_buf usrpgfaultctx;
  unsigned usrpgfault;
  USER_ADDRESS usrpgaddr;
  hal_pfinfo_t usrpginfo;

  /* 
     This pointer can be set by users of
     libnux to store their private data.
   */
  void *data;

  struct hal_cpu hal_cpu;
};

void _pfncache_bootstrap (void);
void stree_pfninit (void);
void kvainit (void);
void kmeminit (void);
void pfncacheinit (void);

void cpu_init (void);
void cpu_enter (void);
__dead void cpu_idle (void);
bool cpu_wasidle (void);
void cpu_clridle (void);
void cpu_nmiop (void);
void cpu_useraccess_checkpf (USER_ADDRESS addr, hal_pfinfo_t info);
unsigned cpu_try_id (void);
void cpu_kmapupdate_broadcast (void);

void ktlbgen_markdirty (hal_tlbop_t op);
TLB_GENERATION ktlbgen_global (void);
TLB_GENERATION ktlbgen_normal (void);

/*
  User Context
*/

/* Invalid User Context.  Frame was kernel non-idle originated.  */
#define UCTXT_INVALID ((void *)-1)

/*
  Get User context from HAL frame. May return UCTXT_INVALID.

  This function it's used at hal entries, and must be called only once
  as it clears the cpu idle status!
 */
UCTXT *uctxt_get (struct hal_frame *f);

/* Get user context from a HAL frame. Expected to be a valid user context. */
UCTXT *uctxt_getuser (struct hal_frame *f);

/* Transform a user context to a HAL frame. Or become idle. */
struct hal_frame *uctxt_frame (UCTXT * uctxt);

/* Transform a user context to a HAL frame. Or return NULL. */
struct hal_frame *uctxt_frame_pointer (UCTXT * uctxt);

#include <nux/nuxperf.h>
#define NUXPERF_DECLARE
#include <nux/perf.h>

#endif
