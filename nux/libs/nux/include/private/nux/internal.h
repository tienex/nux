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
UINT8 NuxStatus (VOID);
UINT8 NuxStatusSetFlags (UINT8 Flags);
BOOLEAN NuxStatusOkCpu (VOID);

/*
  Kernel TLB status.
*/
typedef struct _KTLB
{
  TLB_GENERATION Global;		/* Global mappings. */
  TLB_GENERATION Normal;		/* Non-global mappings. */
} KTLB, *PKTLB;

/*
  Return <0 if a < b. 0 if a == b, >0 if a > b or wrapcounts differ.
*/
static INLINE INT32
TlbGenCompare (TLB_GENERATION A, TLB_GENERATION B)
{
  if (_TG_WRAP (A) == _TG_WRAP (B))
    {
      if (A < B)
	return -1;
      else if (A > B)
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

typedef struct _CPU_INFO
{
  UINT32 CpuId;
  UINT32 PhysId;
  struct _CPU_INFO *Self;

  struct umap *Umap;

  /* Idle jmp_buf */
  jmp_buf IdleJmp;
  BOOLEAN Idle;


  /* NMI operations. */
#define NMIOP_KMAPUPDATE 1	/* Update kmap across all CPUs. */
#define NMIOP_TLBFLUSH 2	/* Flush TLBs. */
  UINT32 NmiOp;

  /* TLB status for current CPU. */
  VOLATILE KTLB Ktlb;

  /*
     User copy setjmp/longjmp for pagefaults.
   */
  jmp_buf UsrPgFaultCtx;
  UINT32 UsrPgFault;
  USER_ADDRESS UsrPgAddr;
  hal_pfinfo_t UsrPgInfo;

  /*
     This pointer can be set by users of
     libnux to store their private data.
   */
  VOID *Data;

  struct hal_cpu HalCpu;
} CPU_INFO, *PCPU_INFO;

VOID PfnCacheBootstrap (VOID);
VOID BatreePfnInitialize (VOID);
VOID KvaInitialize (VOID);
VOID KmemInitialize (VOID);
VOID PfnCacheInitialize (VOID);

VOID CpuInitialize (VOID);
VOID CpuEnter (VOID);
__dead VOID CpuIdle (VOID);
BOOLEAN CpuWasIdle (VOID);
VOID CpuClearIdle (VOID);
VOID CpuNmiOperation (VOID);
VOID CpuUserAccessCheckPageFault (USER_ADDRESS Addr, hal_pfinfo_t Info);
UINT32 CpuTryId (VOID);
VOID CpuKmapUpdateBroadcast (VOID);

VOID KtlbGenMarkDirty (hal_tlbop_t Op);
TLB_GENERATION KtlbGenGlobal (VOID);
TLB_GENERATION KtlbGenNormal (VOID);

/*
  User Context
*/

/* Invalid User Context.  Frame was kernel non-idle originated.  */
#define UCTXT_INVALID ((VOID *)-1)

/*
  Get User context from HAL frame. May return UCTXT_INVALID.

  This function it's used at hal entries, and must be called only once
  as it clears the cpu idle status!
 */
UCTXT *UctxtGet (struct hal_frame *Frame);

/* Get user context from a HAL frame. Expected to be a valid user context. */
UCTXT *UctxtGetUser (struct hal_frame *Frame);

/* Transform a user context to a HAL frame. Or become idle. */
struct hal_frame *UctxtFrame (UCTXT *Uctxt);

/* Transform a user context to a HAL frame. Or return NULL. */
struct hal_frame *UctxtFramePointer (UCTXT *Uctxt);

#include <nux/nuxperf.h>
#define NUXPERF_DECLARE
#include <nux/perf.h>

#endif
