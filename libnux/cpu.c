/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  See COPYING file for the full license.

  SPDX-License-Identifier:	GPL2.0+
*/


#include <assert.h>
#include <string.h>

#include <nux/types.h>
#include <nux/hal.h>
#include <nux/plt.h>
#include <nux/nux.h>
#include <nux/cpumask.h>

#include "internal.h"

static unsigned number_cpus = 0;
static unsigned cpu_phys_to_id[HAL_MAXCPUS] = { -1, };
static struct cpu_info *cpus[HAL_MAXCPUS] = { 0, };

static cpumask_t tlbmap = 0;
static cpumask_t cpus_active = 0;

/* We use this struct during bootstrap before the cpu infrastructure has been initialised. The CPU number is zero. */
struct cpu_info __boot_cpuinfo = { 0, };

static unsigned
cpu_idfromphys (unsigned physid)
{
  unsigned id;

  assert (physid < HAL_MAXCPUS);
  id = cpu_phys_to_id[physid];
  assert (id < HAL_MAXCPUS);
  return id;
}

static struct cpu_info *
cpu_getinfo (unsigned id)
{

  if (id >= HAL_MAXCPUS)
    {
      error ("CPU ID %d too big", id);
      return NULL;
    }
  else if (id >= number_cpus)
    {
      error ("Requested non-active cpu %d", id);
      return NULL;
    }
  return cpus[id];
}

static int
cpu_add (uint16_t physid)
{
  int id;
  struct cpu_info *cpuinfo;

  if (physid >= HAL_MAXCPUS)
    {
      warn ("CPU Phys ID %02x too big. Skipping.", physid);
      return -1;
    }

  if (number_cpus >= HAL_MAXCPUS)
    {
      warn ("Too many CPUs. Skipping.");
      return -1;
    }

  id = number_cpus++;
  info ("Found CPU %d (PHYS:%d)", id, physid);

  /* We are at init-time. We use LOW KMEM via BRK. */
  cpuinfo = (struct cpu_info *)kmem_brkgrow(1, sizeof (struct cpu_info));
  cpuinfo->cpu_id = id;
  cpuinfo->phys_id = physid;
  cpuinfo->self = cpuinfo;
  hal_pcpu_init (physid, &cpuinfo->hal_cpu);

  cpus[id] = cpuinfo;
  cpu_phys_to_id[physid] = id;
  return id;
}

static struct cpu_info *
cpu_curinfo (void)
{
  return (struct cpu_info *) hal_cpu_getdata ();
}

void
cpu_enter (void)
{
  struct cpu_info *cpu;
  unsigned pcpuid, cpuid;

  /* Setup Platform support for local CPU operations */
  plt_pcpu_enter ();

  pcpuid = plt_pcpu_id ();

  /* Setup local CPU HAL operations. */
  hal_pcpu_enter (pcpuid);

  /* Set per-cpu data. */
  cpuid = cpu_idfromphys (pcpuid);
  cpu = cpu_getinfo (cpuid);
  hal_cpu_setdata ((void *) cpu);

  /* Mark as active */
  atomic_cpumask_set (&cpus_active, cpuid);

  /* Setup CPU idle loop. */
  if (setjmp (cpu->idlejmp))
    {
      cpu_curinfo ()->idle = true;
      hal_cpu_idle ();
    }

  info ("CPU %d set as active", cpuid);
}

bool
cpu_wasidle (void)
{
  return cpu_curinfo ()->idle;
}

void
cpu_clridle (void)
{
  cpu_curinfo ()->idle = false;
}

void
cpu_init (void)
{
  unsigned pcpu;

  /* Add all CPUs found in the platform. */
  while ((pcpu = plt_pcpu_iterate ()) != PLT_PCPU_INVALID)
    cpu_add (pcpu);

  cpu_enter ();
}

void
cpu_tlbnmi (void)
{
  /* Called on NMI. No locks whatsoever. */
  hal_tlbop_t tlbop;
  struct cpu_info *ci = cpu_curinfo ();

  tlbop = __sync_fetch_and_and (&ci->tlbop, 0);
  if (tlbop != HAL_TLBOP_NONE)
    {
      hal_cpu_tlbop (tlbop);
    }
  atomic_cpumask_clear (&tlbmap, cpu_id ());
}

void
cpu_startall (void)
{
  unsigned pcpu;

  while ((pcpu = plt_pcpu_iterate ()) != PLT_PCPU_INVALID)
    {
      paddr_t start;

      if (pcpu == plt_pcpu_id ())
	continue;

      if (pcpu >= HAL_MAXCPUS)
	continue;

      info ("Starting CPU %d", cpu_idfromphys (pcpu));

      start = hal_pcpu_prepare (pcpu);
      if (start != PADDR_INVALID)
	{
	  plt_pcpu_start (pcpu, start);
	}
      else
	{
	  warn ("HAL can't prepare for boot CPU %d",
		cpu_idfromphys (pcpu));
	}
    }
}

cpumask_t
cpu_activemask (void)
{
  cpumask_t mask = atomic_cpumask (&cpus_active);

  /* The following might happen when we call a cpu operation at init
     in PLT (e.g.) and we haven't set up CPUs yet. This should be
     fixed with init-time ad-hoc code. Use this assert to catch these
     issues.  */
  assert (mask != 0);
  return mask;
}

unsigned
cpu_id (void)
{
  return cpu_curinfo ()->cpu_id;
}

void
cpu_setdata (void *ptr)
{
  cpu_curinfo ()->data = ptr;
}

void *
cpu_getdata (void)
{
  return cpu_curinfo ()->data;
}

unsigned
cpu_num (void)
{
  return number_cpus;
}

void
cpu_nmi (int cpu)
{
  struct cpu_info *ci = cpu_getinfo (cpu);

  if (ci != NULL)
    plt_pcpu_nmi (ci->phys_id);
}

void
cpu_nmi_broadcast (void)
{
  plt_pcpu_nmiall ();
}

void
cpu_nmi_mask (cpumask_t map)
{
  foreach_cpumask (map, cpu_nmi (i));
}

unsigned
cpu_ipi_avail (void)
{
  unsigned vectmax = hal_vect_max ();
  unsigned ipibase = hal_vect_ipibase ();
  unsigned irqbase = hal_vect_irqbase ();
  unsigned ipilimit = ipibase < irqbase ? irqbase : vectmax;

  return ipilimit - ipibase;
}

void
cpu_ipi (int cpu, uint8_t vct)
{
  struct cpu_info *ci = cpu_getinfo (cpu);

  if (ci != NULL)
    plt_pcpu_ipi (ci->phys_id, vct);
}

void
cpu_ipi_broadcast (uint8_t vct)
{
  plt_pcpu_ipiall (vct);
}

void
cpu_ipi_mask (cpumask_t map, uint8_t vct)
{
  foreach_cpumask (map, cpu_ipi (i, vct));
}

void
cpu_idle (void)
{
  struct cpu_info *ci = cpu_curinfo ();

  longjmp (ci->idlejmp, 1);
}

void
cpu_tlbflush_sync (cpumask_t map)
{
  while (__sync_add_and_fetch (&tlbmap, 0) & map)
    {
      hal_cpu_relax ();
    }
}

void
cpu_tlbflush (int cpu, tlbop_t op, bool sync)
{
  struct cpu_info *ci = cpu_getinfo (cpu);

  if (ci != NULL)
    {
      __sync_fetch_and_or (&ci->tlbop, op);
      atomic_cpumask_set (&tlbmap, cpu);
      cpu_nmi (cpu);
    }


  if (sync)
    {
      cpumask_t cpumask = 0;

      cpumask_set (&cpumask, cpu_id ());
      cpu_tlbflush_sync (cpumask);
    }
}

void
cpu_tlbflush_mask (cpumask_t mask, tlbop_t op, bool sync)
{
  foreach_cpumask (mask, cpu_tlbflush (i, op, false));

  if (sync)
    cpu_tlbflush_sync (mask);
}

void
cpu_tlbflush_broadcast (tlbop_t op, bool sync)
{
  cpumask_t mask = atomic_cpumask (&cpus_active);

  if (mask == 0)
    {
      /* PLT code might call kva_map() to map pages at startup. When
         PLT starts the CPU subsystem hasn't started yet. Flush only
         the local TLBs.  */
      hal_cpu_tlbop (op);
    }
  else
    {
      cpu_tlbflush_mask (cpu_activemask (), op, sync);
    }
}

void
cpu_tlbflush_sync_broadcast (void)
{
  cpu_tlbflush_sync (cpu_activemask ());
}

static void
cpu_useraccess_reset (void)
{
  struct cpu_info *ci = cpu_curinfo ();

  ci->usrpgaddr = 0;
  ci->usrpginfo = 0;
  __insn_barrier ();
}

static void
cpu_useraccess_end (void)
{
  struct cpu_info *ci = cpu_curinfo ();

  ci->usrpgaddr = 0;
  ci->usrpginfo = 0;
  ci->usrpgfault = 0;
  __insn_barrier ();
}

bool
cpu_useraccess_copyfrom (void *dst, uaddr_t src, size_t size,
		       bool (*pf_handler)(uaddr_t va, hal_pfinfo_t info))
{
  struct cpu_info *ci = cpu_curinfo ();

  if (! uaddr_validrange (src, size))
    return false;

  ci->usrpgfault = 1;
  __insn_barrier();
  if (setjmp (ci->usrpgfaultctx) != 0)
    {
      uaddr_t uaddr = ci->usrpgaddr;
      hal_pfinfo_t pfinfo = ci->usrpginfo;

      if (!pf_handler || ! pf_handler (uaddr, pfinfo))
	{
	  cpu_useraccess_end ();
	  return false;
	}
      cpu_useraccess_reset ();
      // pass-through
    }

  memcpy (dst, (void *)src, size);

  cpu_useraccess_end ();
  return true;
}

bool
cpu_useraccess_copyto (uaddr_t dst, void *src, size_t size,
		       bool (*pf_handler)(uaddr_t va, hal_pfinfo_t info))
{
  struct cpu_info *ci = cpu_curinfo ();

  if (! uaddr_validrange (dst, size))
    return false;

  ci->usrpgfault = 1;
  __insn_barrier();
  if (setjmp (ci->usrpgfaultctx) != 0)
    {
      uaddr_t uaddr = ci->usrpgaddr;
      hal_pfinfo_t pfinfo = ci->usrpginfo;

      if (!pf_handler || ! pf_handler (uaddr, pfinfo))
	{
	  cpu_useraccess_end ();
	  return false;
	}
      cpu_useraccess_reset ();
      // pass-through
    }

  memcpy ((void *)dst, src, size);

  cpu_useraccess_end ();
  return true;
}

bool
cpu_useraccess_memset (uaddr_t dst, int ch, size_t size,
		       bool (*pf_handler)(uaddr_t va, hal_pfinfo_t info))
{
  struct cpu_info *ci = cpu_curinfo ();

  if (! uaddr_validrange (dst, size))
    return false;

  ci->usrpgfault = 1;
  __insn_barrier();
  if (setjmp (ci->usrpgfaultctx) != 0)
    {
      uaddr_t uaddr = ci->usrpgaddr;
      hal_pfinfo_t pfinfo = ci->usrpginfo;

      if (!pf_handler || ! pf_handler (uaddr, pfinfo))
	{
	  cpu_useraccess_end ();
	  return false;
	}
      cpu_useraccess_reset ();
      // pass-through
    }

  memset ((void *)dst, ch, size);

  cpu_useraccess_end ();
  return true;
}

bool
cpu_useraccess_signal (uctxt_t *uctxt, unsigned long ip, unsigned long arg,
		       bool (*pf_handler)(uaddr_t va, hal_pfinfo_t info))
{
  struct cpu_info *ci = cpu_curinfo ();
  struct hal_frame *f;


  f = uctxt_frame_pointer (uctxt);
  if (f == NULL)
    return false;

  if (! hal_frame_isuser (f))
    return false;

  /*
    We can't be sure of what addresses the HAL will write, and the HAL
    is reponsible for checking that it's writing to userspace addresses.
  */

  ci->usrpgfault = 1;
  __insn_barrier();
  if (setjmp (ci->usrpgfaultctx) != 0)
    {
      uaddr_t uaddr = ci->usrpgaddr;
      hal_pfinfo_t pfinfo = ci->usrpginfo;

      if (!pf_handler || ! pf_handler (uaddr, pfinfo))
	{
	  cpu_useraccess_end ();
	  return false;
	}
      cpu_useraccess_reset ();
      // pass-through
    }

  hal_frame_signal (f, ip, arg);

  cpu_useraccess_end ();
  return true;

}

void
cpu_useraccess_checkpf (uaddr_t addr, hal_pfinfo_t info)
{
  struct cpu_info *ci = cpu_curinfo ();

  if (ci->usrpgfault)
    {
      ci->usrpgaddr = addr;
      ci->usrpginfo = info;
      __insn_barrier ();
      longjmp(ci->usrpgfaultctx, 1);
      /* Not reached */
    }
}
