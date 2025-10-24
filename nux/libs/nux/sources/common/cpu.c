/** @file
  NUX CPU Management

  Provides CPU initialization, per-CPU data management, inter-processor
  interrupts (IPI/NMI), TLB operations, user memory access, and user
  address space switching for multi-processor systems.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <assert.h>
#include <string.h>

#include <nux/types.h>
#include <nux/hal.h>
#include <nux/plt.h>
#include <nux/nux.h>
#include <nux/cpumask.h>

#include "internal.h"

static UINT32 gNumberCpus = 0;
static UINT32 gCpuPhysToId[HAL_MAXCPUS] = { -1, };
static struct cpu_info *gCpus[HAL_MAXCPUS] = { 0, };

static cpumask_t gTlbMap = 0;
static cpumask_t gCpusActive = 0;

/** We use this struct during bootstrap before the cpu infrastructure has been initialised. The CPU number is zero. **/
struct cpu_info __boot_cpuinfo = { 0, };

/**
  Add a CPU to the system.

  Requires NUXST_OKPLT status.

  @param[in] PhysId  Physical CPU ID.

  @return Logical CPU ID, or -1 on error.
**/
static INT32
CpuAdd (
  IN UINT16  PhysId
  )
{
  INT32 Id;
  struct cpu_info *pCpuInfo;

  assert (NuxStatus () & NUXST_OKPLT);
  if (PhysId >= HAL_MAXCPUS)
    {
      warn ("CPU Phys ID %02x too big. Skipping.", PhysId);
      return -1;
    }

  if (gNumberCpus >= HAL_MAXCPUS)
    {
      warn ("Too many CPUs. Skipping.");
      return -1;
    }

  Id = gNumberCpus++;
  printf ("%d[%d] ", Id, PhysId);

  /* We are at init-time. We use LOW KMEM via BRK. */
  pCpuInfo = (struct cpu_info *) kmem_brkgrow (1, sizeof (struct cpu_info));
  pCpuInfo->cpu_id = Id;
  pCpuInfo->phys_id = PhysId;
  pCpuInfo->self = pCpuInfo;
  hal_pcpu_add (PhysId, &pCpuInfo->hal_cpu);

  gCpus[Id] = pCpuInfo;
  gCpuPhysToId[PhysId] = Id;
  return Id;
}

/**
  Initialize CPU subsystem.

  Discovers all CPUs from platform, initializes HAL per-CPU support,
  and enters the first CPU. Requires NUXST_OKPLT status.
**/
VOID
CpuInitialize (
  VOID
  )
{
  UINT32 Pcpu;

  printf ("CPUs found: ");
  /* Add all CPUs found in the platform. */
  while ((Pcpu = plt_pcpu_iterate ()) != PLT_PCPU_INVALID)
    CpuAdd (Pcpu);
  printf ("\n");

  hal_pcpu_init ();

  CpuEnter ();
}

/**
  Get logical CPU ID from physical CPU ID.

  Requires NUXST_OKCPU status.

  @param[in] PhysId  Physical CPU ID.

  @return Logical CPU ID.
**/
static UINT32
CpuIdFromPhys (
  IN UINT32  PhysId
  )
{
  UINT32 Id;

  assert (PhysId < HAL_MAXCPUS);
  Id = gCpuPhysToId[PhysId];
  assert (Id < HAL_MAXCPUS);
  return Id;
}

/**
  Get CPU information structure by logical ID.

  Requires NUXST_OKPLT status.

  @param[in] Id  Logical CPU ID.

  @return Pointer to CPU info structure, or NULL on error.
**/
static struct cpu_info *
CpuGetInfo (
  IN UINT32  Id
  )
{
  if (Id >= HAL_MAXCPUS)
    {
      error ("CPU ID %d too big", Id);
      return NULL;
    }
  else if (Id >= gNumberCpus)
    {
      error ("Requested non-active cpu %d", Id);
      return NULL;
    }

  return gCpus[Id];
}

/**
  Get current CPU information structure.

  Requires NUXST_OKCPU status.

  @return Pointer to current CPU info structure.
**/
static struct cpu_info *
CpuGetCurrentInfo (
  VOID
  )
{
  return (struct cpu_info *) hal_cpu_getdata ();
}

/**
  Enter CPU context.

  Sets up platform and HAL support for the current CPU, configures
  per-CPU data and idle loop, and marks CPU as active. Can be called
  during initialization (NUXST_OKPLT) or by Application Processors.
**/
VOID
CpuEnter (
  VOID
  )
{
  struct cpu_info *pCpu;
  UINT32 PcpuId, CpuId;

  /* Setup Platform support for local CPU operations */
  plt_pcpu_enter ();

  PcpuId = plt_pcpu_id ();

  /* Setup local CPU HAL operations. */
  hal_pcpu_enter (PcpuId);

  /* Set per-cpu data. */
  CpuId = CpuIdFromPhys (PcpuId);
  pCpu = CpuGetInfo (CpuId);
  hal_cpu_setdata ((VOID *) pCpu);

  /* Setup CPU idle loop. */
  if (setjmp (pCpu->idlejmp))
    {
      /* From a longjmp, OKCPU post here. */
      CpuGetCurrentInfo ()->idle = TRUE;
      hal_cpu_idle ();
    }

  /* Mark as active */
  atomic_cpumask_set (&gCpusActive, CpuId);
  /* From now on we can receive NMIs. */

  /* Check if the system hit a panic before we could receive the NMI. */
  if (__predict_false (NuxStatus () & NUXST_PANIC))
    {
      hal_cpu_halt ();
      /* Unreachable */
    }
}

/**
  Check if current CPU was idle.

  Requires NUXST_OKCPU status.

  @retval TRUE   CPU was idle.
  @retval FALSE  CPU was not idle.
**/
BOOLEAN
CpuWasIdle (
  VOID
  )
{
  return CpuGetCurrentInfo ()->idle;
}

/**
  Clear current CPU idle state.

  Requires NUXST_OKCPU status.
**/
VOID
CpuClearIdle (
  VOID
  )
{
  CpuGetCurrentInfo ()->idle = FALSE;
}

/**
  Start all Application Processors.

  Iterates through all CPUs discovered by platform and initiates
  their boot sequence. Requires NUXST_OKCPU status.
**/
VOID
CpuStartAll (
  VOID
  )
{
  UINT32 Pcpu;

  while ((Pcpu = plt_pcpu_iterate ()) != PLT_PCPU_INVALID)
    {
      paddr_t Start;

      if (Pcpu == plt_pcpu_id ())
        continue;

      if (Pcpu >= HAL_MAXCPUS)
        continue;

      Start = hal_pcpu_startaddr (Pcpu);
      if (Start != PADDR_INVALID)
        {
          plt_pcpu_start (Pcpu, Start);
        }
      else
        {
          warn ("HAL can't prepare for boot CPU %d", CpuIdFromPhys (Pcpu));
        }
    }
}

/**
  Get active CPU mask.

  Requires NUXST_OKCPU status.

  @return Bitmask of active CPUs.
**/
cpumask_t
CpuGetActiveMask (
  VOID
  )
{
  cpumask_t Mask = atomic_cpumask (&gCpusActive);

  return Mask;
}

/**
  Get current CPU logical ID.

  Requires NUXST_OKCPU status.

  @return Current CPU logical ID.
**/
UINT32
CpuGetId (
  VOID
  )
{
  return CpuGetCurrentInfo ()->cpu_id;
}

/**
  Try to get current CPU logical ID.

  Can be called at any status level. Returns 0 if CPU subsystem
  is not yet initialized.

  @return Current CPU logical ID, or 0 if not initialized.
**/
UINT32
CpuTryGetId (
  VOID
  )
{
  if (!NuxStatusOkCpu ())
    {
      return 0;
    }
  else
    {
      struct cpu_info *pCi = CpuGetCurrentInfo ();
      return pCi->cpu_id;
    }
}

/**
  Set per-CPU private data pointer.

  Requires NUXST_OKPLT status.

  @param[in] pPtr  Pointer to per-CPU data.
**/
VOID
CpuSetData (
  IN VOID  *pPtr
  )
{
  CpuGetCurrentInfo ()->data = pPtr;
}

/**
  Get per-CPU private data pointer.

  Requires NUXST_OKCPU status.

  @return Pointer to per-CPU data.
**/
VOID *
CpuGetData (
  VOID
  )
{
  return CpuGetCurrentInfo ()->data;
}

/**
  Get number of CPUs in system.

  Requires NUXST_OKCPU status.

  @return Number of CPUs.
**/
UINT32
CpuGetNumber (
  VOID
  )
{
  return gNumberCpus;
}

/**
  Send NMI to specific CPU.

  Requires NUXST_OKCPU status.

  @param[in] Cpu  Logical CPU ID.
**/
VOID
CpuSendNmi (
  IN INT32  Cpu
  )
{
  struct cpu_info *pCi = CpuGetInfo (Cpu);

  if (pCi != NULL)
    plt_pcpu_nmi (pCi->phys_id);
}

/**
  Send NMI to CPUs in mask.

  Requires NUXST_OKCPU status.

  @param[in] Map  CPU mask.
**/
VOID
CpuSendNmiMask (
  IN cpumask_t  Map
  )
{
  foreach_cpumask (Map, CpuSendNmi (i));
}

/**
  Send NMI to all CPUs except current.

  Can be called at any status level.
**/
VOID
CpuSendNmiAllButSelf (
  VOID
  )
{
  if (NuxStatusOkCpu ())
    {
      cpumask_t Mask = CpuGetActiveMask ();
      cpumask_clear (&Mask, CpuGetId ());
      CpuSendNmiMask (Mask);
    }
}

/**
  Broadcast NMI to all CPUs.

  Requires NUXST_OKCPU status.
**/
VOID
CpuSendNmiBroadcast (
  VOID
  )
{
  CpuTlbFlushMask (CpuGetActiveMask ());
}

/**
  Send IPI to specific CPU.

  Requires NUXST_OKCPU status.

  @param[in] Cpu  Logical CPU ID.
**/
VOID
CpuSendIpi (
  IN INT32  Cpu
  )
{
  struct cpu_info *pCi = CpuGetInfo (Cpu);

  if (pCi != NULL)
    plt_pcpu_ipi (pCi->phys_id);
}

/**
  Broadcast IPI to all CPUs.

  Requires NUXST_OKCPU status.
**/
VOID
CpuSendIpiBroadcast (
  VOID
  )
{
  plt_pcpu_ipiall ();
}

/**
  Send IPI to CPUs in mask.

  Requires NUXST_OKCPU status.

  @param[in] Map  CPU mask.
**/
VOID
CpuSendIpiMask (
  IN cpumask_t  Map
  )
{
  foreach_cpumask (Map, CpuSendIpi (i));
}

/**
  Enter CPU idle state.

  Longjmps to the idle loop set up by CpuEnter. Requires NUXST_OKCPU status.
**/
VOID
CpuIdle (
  VOID
  )
{
  struct cpu_info *pCi = CpuGetCurrentInfo ();

  longjmp (pCi->idlejmp, 1);
}

/**
  Update kernel TLB generation.

  Can be called by NMI handler. Flushes TLB if kernel TLB generation
  has advanced. Requires NUXST_OKCPU status.
**/
VOID
CpuKernelTlbUpdate (
  VOID
  )
{
  struct cpu_info *pCi = CpuGetCurrentInfo ();
  tlbgen_t CpuGlobal, CpuNormal;

  __atomic_load (&pCi->ktlb.global, &CpuGlobal, __ATOMIC_RELAXED);
  __atomic_load (&pCi->ktlb.normal, &CpuNormal, __ATOMIC_RELAXED);
  tlbgen_t KGlobal = ktlbgen_global ();
  tlbgen_t KNormal = ktlbgen_normal ();

  if (tlbgen_cmp (KGlobal, CpuGlobal) > 0)
    {
      hal_cpu_tlbop (HAL_TLBOP_FLUSHALL);
      /*
         Ignore if CPU's tlbgens have been modified. This means an NMI
         has modified it in the meanwhile.

         Both failure and success case are relaxed because these
         variable are per cpu and accessed with relaxed order.
       */
      __atomic_compare_exchange (&pCi->ktlb.global, &CpuGlobal, &KGlobal,
                                 FALSE, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
      __atomic_compare_exchange (&pCi->ktlb.normal, &CpuNormal, &KNormal,
                                 FALSE, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
    }
  else if (tlbgen_cmp (KNormal, CpuNormal) > 0)
    {
      hal_cpu_tlbop (HAL_TLBOP_FLUSH);
      __atomic_compare_exchange (&pCi->ktlb.normal, &CpuNormal, &KNormal,
                                 FALSE, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
    }
}

/**
  Reach target kernel TLB generation.

  Updates CPU TLB if current generation is behind target. Can be called
  during early boot (NUXST_OKCPU not yet set) or after initialization.

  @param[in] Target  Target TLB generation to reach.
**/
VOID
CpuKernelTlbReach (
  IN tlbgen_t  Target
  )
{
  if (!NuxStatusOkCpu ())
    {
      /* early boot: just flush the tlb. */
      hal_cpu_tlbop (HAL_TLBOP_FLUSH);
      return;
    }

  struct cpu_info *pCi = CpuGetCurrentInfo ();
  tlbgen_t CpuKtlb;

  __atomic_load (&pCi->ktlb.normal, &CpuKtlb, __ATOMIC_RELAXED);

  if (tlbgen_cmp (Target, CpuKtlb) > 0)
    {
      CpuKernelTlbUpdate ();
    }
}

/**
  Flush local TLB.

  Can be called by NMI handler. Requires NUXST_OKCPU status.
**/
VOID
CpuTlbFlushLocal (
  VOID
  )
{
  /* We're flushing the cpu. Update the relevant kmap tlb generation. */
  struct cpu_info *pCi = CpuGetCurrentInfo ();
  tlbgen_t KNormal = ktlbgen_normal ();
  tlbgen_t CpuNormal;

  __atomic_load (&pCi->ktlb.normal, &CpuNormal, __ATOMIC_RELAXED);
  hal_cpu_tlbop (HAL_TLBOP_FLUSH);
  __atomic_compare_exchange (&pCi->ktlb.normal, &CpuNormal, &KNormal,
                             FALSE, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
}

/**
  Handle NMI operations.

  Called from NMI handler. Processes pending NMI operations including
  kernel map updates and TLB flushes. Requires NUXST_OKCPU status.
**/
VOID
CpuNmiOperation (
  VOID
  )
{
  struct cpu_info *pCi = CpuGetCurrentInfo ();
  UINT32 NmiOp = pCi->nmiop;

  if (NmiOp & NMIOP_KMAPUPDATE)
    {
      CpuKernelTlbUpdate ();
    }

  if (NmiOp & NMIOP_TLBFLUSH)
    {
      CpuTlbFlushLocal ();
    }

  atomic_cpumask_clear (&gTlbMap, CpuGetId ());
}

/**
  Request kernel map update on specific CPU.

  Requires NUXST_OKPLT status.

  @param[in] Cpu  Logical CPU ID.
**/
VOID
CpuKernelMapUpdate (
  IN INT32  Cpu
  )
{
  struct cpu_info *pCi = CpuGetInfo (Cpu);

  if (pCi != NULL)
    return;

  __atomic_or_fetch (&pCi->nmiop, NMIOP_KMAPUPDATE, __ATOMIC_RELAXED);
  CpuSendNmi (Cpu);
}

/**
  Broadcast kernel map update to all CPUs.

  Can be called at any status level. If CPU subsystem is not initialized,
  flushes only local TLBs globally.
**/
VOID
CpuKernelMapUpdateBroadcast (
  VOID
  )
{
  if (NuxStatusOkCpu ())
    {
      foreach_cpumask (CpuGetActiveMask (), CpuKernelMapUpdate (i));
    }
  else
    {
      /*
         PLT code might call kva_map() to map pages at startup.
         When PLT starts the CPU subsystem hasn't started yet.
         Flush only the local TLBs, but globally.
       */
      hal_cpu_tlbop (HAL_TLBOP_FLUSHALL);
    }
}

/**
  Request TLB flush on specific CPU.

  Requires NUXST_OKPLT status.

  @param[in] Cpu  Logical CPU ID.
**/
VOID
CpuTlbFlush (
  IN INT32  Cpu
  )
{
  struct cpu_info *pCi = CpuGetInfo (Cpu);

  if (pCi == NULL)
    return;

  __atomic_or_fetch (&pCi->nmiop, NMIOP_TLBFLUSH, __ATOMIC_RELAXED);
  CpuSendNmi (Cpu);
}

/**
  Request TLB flush on CPUs in mask.

  Requires NUXST_OKPLT status.

  @param[in] Mask  CPU mask.
**/
VOID
CpuTlbFlushMask (
  IN cpumask_t  Mask
  )
{
  foreach_cpumask (Mask, CpuTlbFlush (i));
}

/**
  Broadcast TLB flush to all CPUs.

  Can be called at any status level. If CPU subsystem is not initialized,
  flushes only local TLBs globally.
**/
VOID
CpuTlbFlushBroadcast (
  VOID
  )
{
  if (NuxStatusOkCpu ())
    {
      CpuTlbFlushMask (CpuGetActiveMask ());
    }
  else
    {
      /*
         PLT code might call kva_map() to map pages at startup.
         When PLT starts the CPU subsystem hasn't started yet.
         Flush only the local TLBs, but globally.
       */
      hal_cpu_tlbop (HAL_TLBOP_FLUSHALL);
    }
}

/**
  Start user memory access.

  Enables supervisor access to user pages (e.g., via SMAP/SMEP control).
**/
static VOID
CpuUserAccessStart (
  VOID
  )
{
  hal_useraccess_start ();
}

/**
  Reset user memory access page fault tracking.

  Clears page fault address and info after handling a recoverable fault.
**/
static VOID
CpuUserAccessReset (
  VOID
  )
{
  struct cpu_info *pCi = CpuGetCurrentInfo ();

  pCi->usrpgaddr = 0;
  pCi->usrpginfo = 0;
  __insn_barrier ();
}

/**
  End user memory access.

  Disables supervisor access to user pages and clears page fault tracking.
**/
static VOID
CpuUserAccessEnd (
  VOID
  )
{
  struct cpu_info *pCi = CpuGetCurrentInfo ();

  hal_useraccess_end ();
  pCi->usrpgaddr = 0;
  pCi->usrpginfo = 0;
  pCi->usrpgfault = 0;
  __insn_barrier ();
}

/**
  Copy data from user address space to kernel.

  Safely copies data from user space with optional page fault handling.
  If a page fault occurs, the provided handler is called.

  @param[out] pDst         Destination buffer in kernel space.
  @param[in]  Src          Source address in user space.
  @param[in]  Size         Number of bytes to copy.
  @param[in]  pPfHandler   Optional page fault handler callback.

  @retval TRUE   Copy succeeded.
  @retval FALSE  Address invalid or unhandled page fault.
**/
BOOLEAN
CpuUserAccessCopyFrom (
  OUT VOID          *pDst,
  IN  uaddr_t       Src,
  IN  size_t        Size,
  IN  BOOLEAN       (*pPfHandler)(uaddr_t Va, hal_pfinfo_t Info)
  )
{
  struct cpu_info *pCi = CpuGetCurrentInfo ();

  if (!uaddr_validrange (Src, Size))
    return FALSE;

  CpuUserAccessStart ();
  pCi->usrpgfault = 1;
  __insn_barrier ();
  if (setjmp (pCi->usrpgfaultctx) != 0)
    {
      uaddr_t Uaddr = pCi->usrpgaddr;
      hal_pfinfo_t PfInfo = pCi->usrpginfo;

      if (!pPfHandler || !pPfHandler (Uaddr, PfInfo))
        {
          CpuUserAccessEnd ();
          return FALSE;
        }

      CpuUserAccessReset ();
      // pass-through
    }

  memcpy (pDst, (VOID *) Src, Size);

  CpuUserAccessEnd ();
  return TRUE;
}

/**
  Copy data from kernel to user address space.

  Safely copies data to user space with optional page fault handling.
  If a page fault occurs, the provided handler is called.

  @param[in]  Dst          Destination address in user space.
  @param[in]  pSrc         Source buffer in kernel space.
  @param[in]  Size         Number of bytes to copy.
  @param[in]  pPfHandler   Optional page fault handler callback.

  @retval TRUE   Copy succeeded.
  @retval FALSE  Address invalid or unhandled page fault.
**/
BOOLEAN
CpuUserAccessCopyTo (
  IN uaddr_t  Dst,
  IN VOID     *pSrc,
  IN size_t   Size,
  IN BOOLEAN  (*pPfHandler)(uaddr_t Va, hal_pfinfo_t Info)
  )
{
  struct cpu_info *pCi = CpuGetCurrentInfo ();

  if (!uaddr_validrange (Dst, Size))
    return FALSE;

  CpuUserAccessStart ();
  pCi->usrpgfault = 1;
  __insn_barrier ();
  if (setjmp (pCi->usrpgfaultctx) != 0)
    {
      uaddr_t Uaddr = pCi->usrpgaddr;
      hal_pfinfo_t PfInfo = pCi->usrpginfo;

      if (!pPfHandler || !pPfHandler (Uaddr, PfInfo))
        {
          CpuUserAccessEnd ();
          return FALSE;
        }

      CpuUserAccessReset ();
      // pass-through
    }

  memcpy ((VOID *) Dst, pSrc, Size);

  CpuUserAccessEnd ();
  return TRUE;
}

/**
  Set memory in user address space.

  Safely sets memory in user space with optional page fault handling.
  If a page fault occurs, the provided handler is called.

  @param[in]  Dst          Destination address in user space.
  @param[in]  Ch           Byte value to set.
  @param[in]  Size         Number of bytes to set.
  @param[in]  pPfHandler   Optional page fault handler callback.

  @retval TRUE   Operation succeeded.
  @retval FALSE  Address invalid or unhandled page fault.
**/
BOOLEAN
CpuUserAccessMemset (
  IN uaddr_t  Dst,
  IN INT32    Ch,
  IN size_t   Size,
  IN BOOLEAN  (*pPfHandler)(uaddr_t Va, hal_pfinfo_t Info)
  )
{
  struct cpu_info *pCi = CpuGetCurrentInfo ();

  if (!uaddr_validrange (Dst, Size))
    return FALSE;

  pCi->usrpgfault = 1;
  __insn_barrier ();
  if (setjmp (pCi->usrpgfaultctx) != 0)
    {
      uaddr_t Uaddr = pCi->usrpgaddr;
      hal_pfinfo_t PfInfo = pCi->usrpginfo;

      if (!pPfHandler || !pPfHandler (Uaddr, PfInfo))
        {
          CpuUserAccessEnd ();
          return FALSE;
        }

      CpuUserAccessReset ();
      // pass-through
    }

  memset ((VOID *) Dst, Ch, Size);

  CpuUserAccessEnd ();
  return TRUE;
}

/**
  Check and handle user access page fault.

  Called from page fault handler when accessing user memory. If user
  access fault handling is enabled, longjmps to fault context.

  @param[in] Addr  Faulting address.
  @param[in] Info  Page fault information flags.
**/
VOID
CpuUserAccessCheckPageFault (
  IN uaddr_t       Addr,
  IN hal_pfinfo_t  Info
  )
{
  struct cpu_info *pCi = CpuGetCurrentInfo ();

  if (pCi->usrpgfault)
    {
      pCi->usrpgaddr = Addr;
      pCi->usrpginfo = Info;
      __insn_barrier ();
      longjmp (pCi->usrpgfaultctx, 1);
      /* Not reached */
    }
}

/**
  Get current user address space map.

  @return Pointer to current user map, or NULL if none.
**/
struct umap *
CpuGetCurrentUserMap (
  VOID
  )
{
  return CpuGetCurrentInfo ()->umap;
}

/**
  Enter user address space map.

  Switches to specified user address space, updating CPU mask and
  loading page tables.

  @param[in] pUmap  User address space map to enter.
**/
VOID
CpuEnterUserMap (
  IN struct umap  *pUmap
  )
{
  struct umap *pCurUmap = CpuGetCurrentInfo ()->umap;

  if (pUmap == pCurUmap)
    return;

  if (pCurUmap != NULL)
    atomic_cpumask_clear (&pCurUmap->cpumask, CpuGetId ());

  __atomic_store (&CpuGetCurrentInfo ()->umap, &pUmap, __ATOMIC_RELEASE);
  atomic_cpumask_set (&pUmap->cpumask, CpuGetId ());
  hal_cpu_tlbop (hal_umap_load (&pUmap->hal));
}

/**
  Exit current user address space map.

  Returns to kernel-only address space, clearing user map association.

  @return Pointer to previous user map, or NULL if none.
**/
struct umap *
CpuExitUserMap (
  VOID
  )
{
  struct umap *pCurUmap;

  hal_cpu_tlbop (hal_umap_load (NULL));
  pCurUmap = CpuGetCurrentInfo ()->umap;
  if (pCurUmap == NULL)
    return NULL;

  __atomic_clear (&CpuGetCurrentInfo ()->umap, __ATOMIC_RELEASE);
  atomic_cpumask_clear (&pCurUmap->cpumask, CpuGetId ());
  CpuGetCurrentInfo ()->umap = NULL;
  return pCurUmap;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use CpuAdd instead **/
static int cpu_add (uint16_t physid) {
  return CpuAdd (physid);
}

/** @deprecated Use CpuInitialize instead **/
void cpu_init (void) {
  CpuInitialize ();
}

/** @deprecated Use CpuIdFromPhys instead **/
static unsigned cpu_idfromphys (unsigned physid) {
  return CpuIdFromPhys (physid);
}

/** @deprecated Use CpuGetInfo instead **/
static struct cpu_info *cpu_getinfo (unsigned id) {
  return CpuGetInfo (id);
}

/** @deprecated Use CpuGetCurrentInfo instead **/
static struct cpu_info *cpu_curinfo (void) {
  return CpuGetCurrentInfo ();
}

/** @deprecated Use CpuEnter instead **/
void cpu_enter (void) {
  CpuEnter ();
}

/** @deprecated Use CpuWasIdle instead **/
bool cpu_wasidle (void) {
  return CpuWasIdle ();
}

/** @deprecated Use CpuClearIdle instead **/
void cpu_clridle (void) {
  CpuClearIdle ();
}

/** @deprecated Use CpuStartAll instead **/
void cpu_startall (void) {
  CpuStartAll ();
}

/** @deprecated Use CpuGetActiveMask instead **/
cpumask_t cpu_activemask (void) {
  return CpuGetActiveMask ();
}

/** @deprecated Use CpuGetId instead **/
unsigned cpu_id (void) {
  return CpuGetId ();
}

/** @deprecated Use CpuTryGetId instead **/
unsigned cpu_try_id (void) {
  return CpuTryGetId ();
}

/** @deprecated Use CpuSetData instead **/
void cpu_setdata (void *ptr) {
  CpuSetData (ptr);
}

/** @deprecated Use CpuGetData instead **/
void *cpu_getdata (void) {
  return CpuGetData ();
}

/** @deprecated Use CpuGetNumber instead **/
unsigned cpu_num (void) {
  return CpuGetNumber ();
}

/** @deprecated Use CpuSendNmi instead **/
void cpu_nmi (int cpu) {
  CpuSendNmi (cpu);
}

/** @deprecated Use CpuSendNmiMask instead **/
void cpu_nmi_mask (cpumask_t map) {
  CpuSendNmiMask (map);
}

/** @deprecated Use CpuSendNmiAllButSelf instead **/
void cpu_nmi_allbutself (void) {
  CpuSendNmiAllButSelf ();
}

/** @deprecated Use CpuSendNmiBroadcast instead **/
void cpu_nmi_broadcast (void) {
  CpuSendNmiBroadcast ();
}

/** @deprecated Use CpuSendIpi instead **/
void cpu_ipi (int cpu) {
  CpuSendIpi (cpu);
}

/** @deprecated Use CpuSendIpiBroadcast instead **/
void cpu_ipi_broadcast (void) {
  CpuSendIpiBroadcast ();
}

/** @deprecated Use CpuSendIpiMask instead **/
void cpu_ipi_mask (cpumask_t map) {
  CpuSendIpiMask (map);
}

/** @deprecated Use CpuIdle instead **/
void cpu_idle (void) {
  CpuIdle ();
}

/** @deprecated Use CpuKernelTlbUpdate instead **/
void cpu_ktlb_update (void) {
  CpuKernelTlbUpdate ();
}

/** @deprecated Use CpuKernelTlbReach instead **/
void cpu_ktlb_reach (tlbgen_t target) {
  CpuKernelTlbReach (target);
}

/** @deprecated Use CpuTlbFlushLocal instead **/
void cpu_tlbflush_local (void) {
  CpuTlbFlushLocal ();
}

/** @deprecated Use CpuNmiOperation instead **/
void cpu_nmiop (void) {
  CpuNmiOperation ();
}

/** @deprecated Use CpuKernelMapUpdate instead **/
void cpu_kmapupdate (int cpu) {
  CpuKernelMapUpdate (cpu);
}

/** @deprecated Use CpuKernelMapUpdateBroadcast instead **/
void cpu_kmapupdate_broadcast (void) {
  CpuKernelMapUpdateBroadcast ();
}

/** @deprecated Use CpuTlbFlush instead **/
void cpu_tlbflush (int cpu) {
  CpuTlbFlush (cpu);
}

/** @deprecated Use CpuTlbFlushMask instead **/
void cpu_tlbflush_mask (cpumask_t mask) {
  CpuTlbFlushMask (mask);
}

/** @deprecated Use CpuTlbFlushBroadcast instead **/
void cpu_tlbflush_broadcast (void) {
  CpuTlbFlushBroadcast ();
}

/** @deprecated Use CpuUserAccessStart instead **/
static void cpu_useraccess_start (void) {
  CpuUserAccessStart ();
}

/** @deprecated Use CpuUserAccessReset instead **/
static void cpu_useraccess_reset (void) {
  CpuUserAccessReset ();
}

/** @deprecated Use CpuUserAccessEnd instead **/
static void cpu_useraccess_end (void) {
  CpuUserAccessEnd ();
}

/** @deprecated Use CpuUserAccessCopyFrom instead **/
bool cpu_useraccess_copyfrom (void *dst, uaddr_t src, size_t size,
                              bool (*pf_handler) (uaddr_t va, hal_pfinfo_t info)) {
  return CpuUserAccessCopyFrom (dst, src, size, pf_handler);
}

/** @deprecated Use CpuUserAccessCopyTo instead **/
bool cpu_useraccess_copyto (uaddr_t dst, void *src, size_t size,
                            bool (*pf_handler) (uaddr_t va, hal_pfinfo_t info)) {
  return CpuUserAccessCopyTo (dst, src, size, pf_handler);
}

/** @deprecated Use CpuUserAccessMemset instead **/
bool cpu_useraccess_memset (uaddr_t dst, int ch, size_t size,
                            bool (*pf_handler) (uaddr_t va, hal_pfinfo_t info)) {
  return CpuUserAccessMemset (dst, ch, size, pf_handler);
}

/** @deprecated Use CpuUserAccessCheckPageFault instead **/
void cpu_useraccess_checkpf (uaddr_t addr, hal_pfinfo_t info) {
  CpuUserAccessCheckPageFault (addr, info);
}

/** @deprecated Use CpuGetCurrentUserMap instead **/
struct umap *cpu_umap_current (void) {
  return CpuGetCurrentUserMap ();
}

/** @deprecated Use CpuEnterUserMap instead **/
void cpu_umap_enter (struct umap *umap) {
  CpuEnterUserMap (umap);
}

/** @deprecated Use CpuExitUserMap instead **/
struct umap *cpu_umap_exit (void) {
  return CpuExitUserMap ();
}

// Legacy global variable aliases
static unsigned number_cpus __attribute__((alias("gNumberCpus")));
static unsigned cpu_phys_to_id[HAL_MAXCPUS] __attribute__((alias("gCpuPhysToId")));
static struct cpu_info *cpus[HAL_MAXCPUS] __attribute__((alias("gCpus")));
static cpumask_t tlbmap __attribute__((alias("gTlbMap")));
static cpumask_t cpus_active __attribute__((alias("gCpusActive")));
