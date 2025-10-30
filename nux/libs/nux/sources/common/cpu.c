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
#include <hal/hal.h>
#include <platform/platform.h>
#include <nux/nux.h>
#include <nux/cpumask.h>

#include <nux/internal.h>

static UINT32 gNumberCpus = 0;
static UINT32 gCpuPhysToId[HAL_MAXCPUS] = { -1, };
static CPU_INFO *gCpus[HAL_MAXCPUS] = { 0, };

static CPU_MASK gTlbMap = 0;
static CPU_MASK gCpusActive = 0;

/** We use this struct during bootstrap before the cpu infrastructure has been initialised. The CPU number is zero. **/
CPU_INFO __boot_cpuinfo = { 0, };

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
  CPU_INFO *CpuInfo;

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
  CpuInfo = (CPU_INFO *) KmemBrkGrow (1, sizeof (CPU_INFO));
  CpuInfo->CpuId = Id;
  CpuInfo->PhysId = PhysId;
  CpuInfo->self = CpuInfo;
  hal_pcpu_add (PhysId, &CpuInfo->hal_cpu);

  gCpus[Id] = CpuInfo;
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
static CPU_INFO *
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
static CPU_INFO *
CpuGetCurrentInfo (
  VOID
  )
{
  return (CPU_INFO *) hal_cpu_getdata ();
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
  CPU_INFO *Cpu;
  UINT32 PcpuId, CpuId;

  /* Setup Platform support for local CPU operations */
  plt_pcpu_enter ();

  PcpuId = plt_pcpu_id ();

  /* Setup local CPU HAL operations. */
  hal_pcpu_enter (PcpuId);

  /* Set per-cpu data. */
  CpuId = CpuIdFromPhys (PcpuId);
  Cpu = CpuGetInfo (CpuId);
  hal_cpu_setdata ((VOID *) Cpu);

  /* Setup CPU idle loop. */
  if (setjmp (Cpu->IdleJmp))
    {
      /* From a longjmp, OKCPU post here. */
      CpuGetCurrentInfo ()->Idle = TRUE;
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
  return CpuGetCurrentInfo ()->Idle;
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
  CpuGetCurrentInfo ()->Idle = FALSE;
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
      PHYSICAL_ADDRESS Start;

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
CPU_MASK
CpuGetActiveMask (
  VOID
  )
{
  CPU_MASK Mask = atomic_cpumask (&gCpusActive);

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
  return CpuGetCurrentInfo ()->CpuId;
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
      CPU_INFO *Ci = CpuGetCurrentInfo ();
      return Ci->CpuId;
    }
}

/**
  Set per-CPU private data pointer.

  Requires NUXST_OKPLT status.

  @param[in] Ptr  Pointer to per-CPU data.
**/
VOID
CpuSetData (
  IN VOID  *Ptr
  )
{
  CpuGetCurrentInfo ()->data = Ptr;
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
  CPU_INFO *Ci = CpuGetInfo (Cpu);

  if (Ci != NULL)
    plt_pcpu_nmi (Ci->PhysId);
}

/**
  Send NMI to CPUs in mask.

  Requires NUXST_OKCPU status.

  @param[in] Map  CPU mask.
**/
VOID
CpuSendNmiMask (
  IN CPU_MASK  Map
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
      CPU_MASK Mask = CpuGetActiveMask ();
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
  CPU_INFO *Ci = CpuGetInfo (Cpu);

  if (Ci != NULL)
    plt_pcpu_ipi (Ci->PhysId);
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
  IN CPU_MASK  Map
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
  CPU_INFO *Ci = CpuGetCurrentInfo ();

  longjmp (Ci->IdleJmp, 1);
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
  CPU_INFO *Ci = CpuGetCurrentInfo ();
  TLB_GENERATION CpuGlobal, CpuNormal;

  __atomic_load (&Ci->Ktlb.Global, &CpuGlobal, __ATOMIC_RELAXED);
  __atomic_load (&Ci->Ktlb.Normal, &CpuNormal, __ATOMIC_RELAXED);
  TLB_GENERATION KGlobal = KtlbGenGlobal ();
  TLB_GENERATION KNormal = KtlbGenNormal ();

  if (TlbGenCompare (KGlobal, CpuGlobal) > 0)
    {
      hal_cpu_tlbop (HAL_TLBOP_FLUSHALL);
      /*
         Ignore if CPU's tlbgens have been modified. This means an NMI
         has modified it in the meanwhile.

         Both failure and success case are relaxed because these
         variable are per cpu and accessed with relaxed order.
       */
      __atomic_compare_exchange (&Ci->Ktlb.Global, &CpuGlobal, &KGlobal,
                                 FALSE, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
      __atomic_compare_exchange (&Ci->Ktlb.Normal, &CpuNormal, &KNormal,
                                 FALSE, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
    }
  else if (TlbGenCompare (KNormal, CpuNormal) > 0)
    {
      hal_cpu_tlbop (HAL_TLBOP_FLUSH);
      __atomic_compare_exchange (&Ci->Ktlb.Normal, &CpuNormal, &KNormal,
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
  IN TLB_GENERATION  Target
  )
{
  if (!NuxStatusOkCpu ())
    {
      /* early boot: just flush the tlb. */
      hal_cpu_tlbop (HAL_TLBOP_FLUSH);
      return;
    }

  CPU_INFO *Ci = CpuGetCurrentInfo ();
  TLB_GENERATION CpuKtlb;

  __atomic_load (&Ci->Ktlb.Normal, &CpuKtlb, __ATOMIC_RELAXED);

  if (TlbGenCompare (Target, CpuKtlb) > 0)
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
  CPU_INFO *Ci = CpuGetCurrentInfo ();
  TLB_GENERATION KNormal = KtlbGenNormal ();
  TLB_GENERATION CpuNormal;

  __atomic_load (&Ci->Ktlb.Normal, &CpuNormal, __ATOMIC_RELAXED);
  hal_cpu_tlbop (HAL_TLBOP_FLUSH);
  __atomic_compare_exchange (&Ci->Ktlb.Normal, &CpuNormal, &KNormal,
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
  CPU_INFO *Ci = CpuGetCurrentInfo ();
  UINT32 NmiOp = Ci->NmiOp;

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
  CPU_INFO *Ci = CpuGetInfo (Cpu);

  if (Ci != NULL)
    return;

  __atomic_or_fetch (&Ci->NmiOp, NMIOP_KMAPUPDATE, __ATOMIC_RELAXED);
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
         PLT code might call KvaMap() to map pages at startup.
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
  CPU_INFO *Ci = CpuGetInfo (Cpu);

  if (Ci == NULL)
    return;

  __atomic_or_fetch (&Ci->NmiOp, NMIOP_TLBFLUSH, __ATOMIC_RELAXED);
  CpuSendNmi (Cpu);
}

/**
  Request TLB flush on CPUs in mask.

  Requires NUXST_OKPLT status.

  @param[in] Mask  CPU mask.
**/
VOID
CpuTlbFlushMask (
  IN CPU_MASK  Mask
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
         PLT code might call KvaMap() to map pages at startup.
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
  CPU_INFO *Ci = CpuGetCurrentInfo ();

  Ci->UsrPgAddr = 0;
  Ci->UsrPgInfo = 0;
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
  CPU_INFO *Ci = CpuGetCurrentInfo ();

  hal_useraccess_end ();
  Ci->UsrPgAddr = 0;
  Ci->UsrPgInfo = 0;
  Ci->UsrPgFault = 0;
  __insn_barrier ();
}

/**
  Copy data from user address space to kernel.

  Safely copies data from user space with optional page fault handling.
  If a page fault occurs, the provided handler is called.

  @param[out] Dst         Destination buffer in kernel space.
  @param[in]  Src          Source address in user space.
  @param[in]  Size         Number of bytes to copy.
  @param[in]  PfHandler   Optional page fault handler callback.

  @retval TRUE   Copy succeeded.
  @retval FALSE  Address invalid or unhandled page fault.
**/
BOOLEAN
CpuUserAccessCopyFrom (
  OUT VOID          *Dst,
  IN  USER_ADDRESS       Src,
  IN  UINTN        Size,
  IN  BOOLEAN       (*PfHandler)(USER_ADDRESS Va, hal_pfinfo_t Info)
  )
{
  CPU_INFO *Ci = CpuGetCurrentInfo ();

  if (!UaddrValidRange (Src, Size))
    return FALSE;

  CpuUserAccessStart ();
  Ci->UsrPgFault = 1;
  __insn_barrier ();
  if (setjmp (Ci->UsrPgFaultCtx) != 0)
    {
      USER_ADDRESS Uaddr = Ci->UsrPgAddr;
      hal_pfinfo_t PfInfo = Ci->UsrPgInfo;

      if (!PfHandler || !PfHandler (Uaddr, PfInfo))
        {
          CpuUserAccessEnd ();
          return FALSE;
        }

      CpuUserAccessReset ();
      // pass-through
    }

  memcpy (Dst, (VOID *) Src, Size);

  CpuUserAccessEnd ();
  return TRUE;
}

/**
  Copy data from kernel to user address space.

  Safely copies data to user space with optional page fault handling.
  If a page fault occurs, the provided handler is called.

  @param[in]  Dst          Destination address in user space.
  @param[in]  Src         Source buffer in kernel space.
  @param[in]  Size         Number of bytes to copy.
  @param[in]  PfHandler   Optional page fault handler callback.

  @retval TRUE   Copy succeeded.
  @retval FALSE  Address invalid or unhandled page fault.
**/
BOOLEAN
CpuUserAccessCopyTo (
  IN USER_ADDRESS  Dst,
  IN VOID     *Src,
  IN UINTN   Size,
  IN BOOLEAN  (*PfHandler)(USER_ADDRESS Va, hal_pfinfo_t Info)
  )
{
  CPU_INFO *Ci = CpuGetCurrentInfo ();

  if (!UaddrValidRange (Dst, Size))
    return FALSE;

  CpuUserAccessStart ();
  Ci->UsrPgFault = 1;
  __insn_barrier ();
  if (setjmp (Ci->UsrPgFaultCtx) != 0)
    {
      USER_ADDRESS Uaddr = Ci->UsrPgAddr;
      hal_pfinfo_t PfInfo = Ci->UsrPgInfo;

      if (!PfHandler || !PfHandler (Uaddr, PfInfo))
        {
          CpuUserAccessEnd ();
          return FALSE;
        }

      CpuUserAccessReset ();
      // pass-through
    }

  memcpy ((VOID *) Dst, Src, Size);

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
  @param[in]  PfHandler   Optional page fault handler callback.

  @retval TRUE   Operation succeeded.
  @retval FALSE  Address invalid or unhandled page fault.
**/
BOOLEAN
CpuUserAccessMemset (
  IN USER_ADDRESS  Dst,
  IN INT32    Ch,
  IN UINTN   Size,
  IN BOOLEAN  (*PfHandler)(USER_ADDRESS Va, hal_pfinfo_t Info)
  )
{
  CPU_INFO *Ci = CpuGetCurrentInfo ();

  if (!UaddrValidRange (Dst, Size))
    return FALSE;

  Ci->UsrPgFault = 1;
  __insn_barrier ();
  if (setjmp (Ci->UsrPgFaultCtx) != 0)
    {
      USER_ADDRESS Uaddr = Ci->UsrPgAddr;
      hal_pfinfo_t PfInfo = Ci->UsrPgInfo;

      if (!PfHandler || !PfHandler (Uaddr, PfInfo))
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
  IN USER_ADDRESS       Addr,
  IN hal_pfinfo_t  Info
  )
{
  CPU_INFO *Ci = CpuGetCurrentInfo ();

  if (Ci->UsrPgFault)
    {
      Ci->UsrPgAddr = Addr;
      Ci->UsrPgInfo = Info;
      __insn_barrier ();
      longjmp (Ci->UsrPgFaultCtx, 1);
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

  @param[in] Umap  User address space map to enter.
**/
VOID
CpuEnterUserMap (
  IN struct umap  *Umap
  )
{
  struct umap *CurUmap = CpuGetCurrentInfo ()->umap;

  if (Umap == CurUmap)
    return;

  if (CurUmap != NULL)
    atomic_cpumask_clear (&CurUmap->cpumask, CpuGetId ());

  __atomic_store (&CpuGetCurrentInfo ()->umap, &Umap, __ATOMIC_RELEASE);
  atomic_cpumask_set (&Umap->cpumask, CpuGetId ());
  hal_cpu_tlbop (hal_umap_load (&Umap->hal));
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
  struct umap *CurUmap;

  hal_cpu_tlbop (hal_umap_load (NULL));
  CurUmap = CpuGetCurrentInfo ()->umap;
  if (CurUmap == NULL)
    return NULL;

  __atomic_clear (&CpuGetCurrentInfo ()->umap, __ATOMIC_RELEASE);
  atomic_cpumask_clear (&CurUmap->cpumask, CpuGetId ());
  CpuGetCurrentInfo ()->umap = NULL;
  return CurUmap;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use CpuAdd instead **/
static int cpu_add (UINT16 physid) {
  return CpuAdd (physid);
}

/** @deprecated Use CpuInitialize instead **/
VOID cpu_init (VOID) {
  CpuInitialize ();
}

/** @deprecated Use CpuIdFromPhys instead **/
static UINT32 cpu_idfromphys (UINT32 physid) {
  return CpuIdFromPhys (physid);
}

/** @deprecated Use CpuGetInfo instead **/
static CPU_INFO *cpu_getinfo (UINT32 id) {
  return CpuGetInfo (id);
}

/** @deprecated Use CpuGetCurrentInfo instead **/
static CPU_INFO *cpu_curinfo (VOID) {
  return CpuGetCurrentInfo ();
}

/** @deprecated Use CpuEnter instead **/
VOID cpu_enter (VOID) {
  CpuEnter ();
}

/** @deprecated Use CpuWasIdle instead **/
BOOLEAN CpuWasIdle (VOID) {
  return CpuWasIdle ();
}

/** @deprecated Use CpuClearIdle instead **/
VOID CpuClearIdle (VOID) {
  CpuClearIdle ();
}

/** @deprecated Use CpuStartAll instead **/
VOID cpu_startall (VOID) {
  CpuStartAll ();
}

/** @deprecated Use CpuGetActiveMask instead **/
CPU_MASK CpuActiveMask (VOID) {
  return CpuGetActiveMask ();
}

/** @deprecated Use CpuGetId instead **/
unsigned CpuId (VOID) {
  return CpuGetId ();
}

/** @deprecated Use CpuTryGetId instead **/
unsigned CpuTryId (VOID) {
  return CpuTryGetId ();
}

/** @deprecated Use CpuSetData instead **/
VOID CpuSetData (VOID *ptr) {
  CpuSetData (ptr);
}

/** @deprecated Use CpuGetData instead **/
VOID *CpuGetData (VOID) {
  return CpuGetData ();
}

/** @deprecated Use CpuGetNumber instead **/
unsigned CpuNum (VOID) {
  return CpuGetNumber ();
}

/** @deprecated Use CpuSendNmi instead **/
VOID CpuNmi (INT32 cpu) {
  CpuSendNmi (cpu);
}

/** @deprecated Use CpuSendNmiMask instead **/
VOID cpu_nmi_mask (CPU_MASK map) {
  CpuSendNmiMask (map);
}

/** @deprecated Use CpuSendNmiAllButSelf instead **/
VOID cpu_nmi_allbutself (VOID) {
  CpuSendNmiAllButSelf ();
}

/** @deprecated Use CpuSendNmiBroadcast instead **/
VOID cpu_nmi_broadcast (VOID) {
  CpuSendNmiBroadcast ();
}

/** @deprecated Use CpuSendIpi instead **/
VOID CpuIpi (INT32 cpu) {
  CpuSendIpi (cpu);
}

/** @deprecated Use CpuSendIpiBroadcast instead **/
VOID cpu_ipi_broadcast (VOID) {
  CpuSendIpiBroadcast ();
}

/** @deprecated Use CpuSendIpiMask instead **/
VOID cpu_ipi_mask (CPU_MASK map) {
  CpuSendIpiMask (map);
}

/** @deprecated Use CpuIdle instead **/
VOID CpuIdle (VOID) {
  CpuIdle ();
}

/** @deprecated Use CpuKernelTlbUpdate instead **/
VOID cpu_ktlb_update (VOID) {
  CpuKernelTlbUpdate ();
}

/** @deprecated Use CpuKernelTlbReach instead **/
VOID cpu_ktlb_reach (TLB_GENERATION target) {
  CpuKernelTlbReach (target);
}

/** @deprecated Use CpuTlbFlushLocal instead **/
VOID cpu_tlbflush_local (VOID) {
  CpuTlbFlushLocal ();
}

/** @deprecated Use CpuNmiOperation instead **/
VOID CpuNmiOperation (VOID) {
  CpuNmiOperation ();
}

/** @deprecated Use CpuKernelMapUpdate instead **/
VOID cpu_kmapupdate (INT32 cpu) {
  CpuKernelMapUpdate (cpu);
}

/** @deprecated Use CpuKernelMapUpdateBroadcast instead **/
VOID CpuKmapUpdateBroadcast (VOID) {
  CpuKernelMapUpdateBroadcast ();
}

/** @deprecated Use CpuTlbFlush instead **/
VOID cpu_tlbflush (INT32 cpu) {
  CpuTlbFlush (cpu);
}

/** @deprecated Use CpuTlbFlushMask instead **/
VOID cpu_tlbflush_mask (CPU_MASK mask) {
  CpuTlbFlushMask (mask);
}

/** @deprecated Use CpuTlbFlushBroadcast instead **/
VOID cpu_tlbflush_broadcast (VOID) {
  CpuTlbFlushBroadcast ();
}

/** @deprecated Use CpuUserAccessStart instead **/
static VOID cpu_useraccess_start (VOID) {
  CpuUserAccessStart ();
}

/** @deprecated Use CpuUserAccessReset instead **/
static VOID cpu_useraccess_reset (VOID) {
  CpuUserAccessReset ();
}

/** @deprecated Use CpuUserAccessEnd instead **/
static VOID cpu_useraccess_end (VOID) {
  CpuUserAccessEnd ();
}

/** @deprecated Use CpuUserAccessCopyFrom instead **/
BOOLEAN cpu_useraccess_copyfrom (VOID *dst, USER_ADDRESS src, UINTN size,
                              BOOLEAN (*pf_handler) (USER_ADDRESS va, hal_pfinfo_t info)) {
  return CpuUserAccessCopyFrom (dst, src, size, pf_handler);
}

/** @deprecated Use CpuUserAccessCopyTo instead **/
BOOLEAN cpu_useraccess_copyto (USER_ADDRESS dst, VOID *src, UINTN size,
                            BOOLEAN (*pf_handler) (USER_ADDRESS va, hal_pfinfo_t info)) {
  return CpuUserAccessCopyTo (dst, src, size, pf_handler);
}

/** @deprecated Use CpuUserAccessMemset instead **/
BOOLEAN cpu_useraccess_memset (USER_ADDRESS dst, INT32 ch, UINTN size,
                            BOOLEAN (*pf_handler) (USER_ADDRESS va, hal_pfinfo_t info)) {
  return CpuUserAccessMemset (dst, ch, size, pf_handler);
}

/** @deprecated Use CpuUserAccessCheckPageFault instead **/
VOID CpuUserAccessCheckPageFault (USER_ADDRESS addr, hal_pfinfo_t info) {
  CpuUserAccessCheckPageFault (addr, info);
}

/** @deprecated Use CpuGetCurrentUserMap instead **/
struct umap *cpu_umap_current (VOID) {
  return CpuGetCurrentUserMap ();
}

/** @deprecated Use CpuEnterUserMap instead **/
VOID cpu_umap_enter (struct umap *umap) {
  CpuEnterUserMap (umap);
}

/** @deprecated Use CpuExitUserMap instead **/
struct umap *cpu_umap_exit (VOID) {
  return CpuExitUserMap ();
}

// Legacy global variable aliases
static UINT32 number_cpus __attribute__((alias("gNumberCpus")));
static UINT32 cpu_phys_to_id[HAL_MAXCPUS] __attribute__((alias("gCpuPhysToId")));
static CPU_INFO *cpus[HAL_MAXCPUS] __attribute__((alias("gCpus")));
static CPU_MASK tlbmap __attribute__((alias("gTlbMap")));
static CPU_MASK cpus_active __attribute__((alias("gCpusActive")));
