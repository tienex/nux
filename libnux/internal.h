/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef LIBNUX_INTERNAL_H
#define LIBNUX_INTERNAL_H

#include <setjmp.h>
#include <nux/types.h>
#include <nux/hal.h>

/* 
   CPU management
*/

#define MAX_CPUS 64
#define MAX_PHYSCPUS 64

struct cpu_info
{
  unsigned cpu_id;
  unsigned phys_id;
  struct cpu_info *self;

  /* Idle jmp_buf */
  jmp_buf idlejmp;
  bool idle;

  /* TLB shootdown */
  tlbop_t tlbop;

  /*
    User copy setjmp/longjmp for pagefaults.
  */
  jmp_buf usrpgfaultctx;
  unsigned usrpgfault;
  uaddr_t usrpgaddr;
  hal_pfinfo_t usrpginfo;

  /* 
     This pointer can be set by users of
     libnux to store their private data.
  */
  void *data;

  struct hal_cpu hal_cpu;
};

void _pfncache_bootstrap (void);
void pfninit (void);
void kvainit (void);
void kmeminit (void);
void pfncacheinit (void);

void cpu_init (void);
void cpu_enter (void);
__dead void cpu_idle (void);
bool cpu_wasidle (void);
void cpu_clridle (void);
void cpu_tlbnmi (void);
void cpu_useraccess_checkpf (uaddr_t addr, hal_pfinfo_t info);

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
uctxt_t *uctxt_get (struct hal_frame *f);

/* Get user context from a HAL frame. Expected to be a valid user context. */
uctxt_t *uctxt_getuser (struct hal_frame *f);

/* Transform a user context to a HAL frame. Or become idle. */
struct hal_frame *uctxt_frame (uctxt_t *uctxt);

/* Transform a user context to a HAL frame. Or return NULL. */
struct hal_frame *uctxt_frame_pointer (uctxt_t *uctxt);

#endif
