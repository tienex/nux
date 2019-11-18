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

#include <cdefs.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <nux/hal.h>
#include <nux/plt.h>

#include "i386.h"
#include "../internal.h"

static pfn_t pcpu_stackpfn[MAXCPUS];
static void *pcpu_stackva[MAXCPUS];
static int _enter_called = 0;

static vaddr_t smp_oldva[MAXCPUS];
static hal_l1e_t smp_oldl1e[MAXCPUS];

uint16_t
_i386_fs (void)
{
  if (_enter_called)
    return (5 + 4 * plt_pcpu_id() + 1) << 3;
  else
    return 0;
}

void
hal_pcpu_init (unsigned pcpuid, struct hal_cpu *pcpu)
{
  pfn_t pfn;
  void *va;
  void _set_tss (unsigned, void *);
  void _set_fs (unsigned, void *);

  assert (pcpuid < MAXCPUS);

  /* Allocate PCPU stack. Use KVA. */
  pfn = pfn_alloc (1);
  assert (pfn != PFN_INVALID);
  /* This is tricky. The hope is that is low enough to be addressed by 16 bit. */
  assert (pfn < (1 << 8) && "Can't allocate Memory below 1MB!");

  va = kva_map (1, pfn, 1, HAL_PTE_W|HAL_PTE_P);
  assert (va != NULL);

  pcpu_stackpfn[pcpuid] = pfn;
  pcpu_stackva[pcpuid] = va;

  pcpu->tss.esp0 = (unsigned long)pcpu_stackva[pcpuid] + 4096;
  pcpu->tss.ss0 = KDS;
  pcpu->tss.iomap = 108;
  pcpu->data = NULL;
  _set_tss (pcpuid, &pcpu->tss);
  _set_fs (pcpuid, &pcpu->data);
}

void
hal_pcpu_enter (unsigned pcpuid)
{
  uint16_t tss = (5 + 4 * pcpuid) << 3;
  uint16_t fs = (5 + 4 * pcpuid + 1) << 3;

  assert (pcpuid < MAXCPUS);

  asm volatile ("ltr %%ax"::"a" (tss));
  asm volatile ("mov %%ax, %%fs"::"a" (fs));

  _enter_called = 1;
}

uint64_t
hal_pcpu_prepare (unsigned pcpu)
{
  extern char *_ap_start, *_ap_end;
  hal_l1p_t l1p;
  paddr_t pstart;
  void *start, *ptr;
  volatile uint16_t *reset;

  if (pcpu >= MAXCPUS)
    return PADDR_INVALID;

  if (pcpu_stackva[pcpu] == NULL)
    return PADDR_INVALID;

  pstart = pcpu_stackpfn[pcpu] << PAGE_SHIFT;
  start = pcpu_stackva[pcpu];
  printf ("preparing %p (%lx)\n", pstart, start);

  reset = kva_physmap (0, 0x467, 2, HAL_PTE_P|HAL_PTE_W|HAL_PTE_X);
  *reset = pstart & 0xf;
  *(reset + 1) = pstart >> 4;
  kva_unmap ((void *)reset);

  /*
    Setup AP bootstrap page
  */
  memcpy (start, &_ap_start,
	  (size_t) ((void *) &_ap_end - (void *) &_ap_start));

  asm volatile ("":::"memory");

  /* The following assumes that CPUs are woke up one at the time. */
  assert (hal_pmap_getl1p (NULL, pstart, true, &l1p));
  smp_oldva[pcpu] = pstart;
  smp_oldl1e[pcpu] = hal_pmap_getl1e (NULL, l1p);
  hal_pmap_setl1e (NULL, l1p, (pstart & ~PAGE_MASK) | PTE_P | PTE_W);

  /*
    The following is trampoline dependent code, and configures the
    trampoline to use the page just selected as bootstrap page.
  */
  extern char _ap_gdtreg, _ap_ljmp, _ap_stackpage;

  /* Setup AP Stack. */
  ptr = start + ((void *)&_ap_stackpage - (void *)&_ap_start);
  *(void **)ptr = start + PAGE_SIZE;

  /* Setup temporary GDT register. */
  ptr = start + ((void *)&_ap_gdtreg - (void *)&_ap_start);
  *(uint32_t *)(ptr + 2) += (uint32_t)pstart;

  /* Setup trampoline 1 */
  ptr = start + ((void *)&_ap_ljmp - (void *)&_ap_start);
  *(uint32_t *)ptr += (uint32_t)pstart;

  return pstart;
}

void
hal_cpu_setdata (void *data)
{
  asm volatile ("movl %0, %%fs:0\n"::"r" (data));
}

void *
hal_cpu_getdata (void)
{
  void *data;

  asm volatile ("movl %%fs:0, %0\n":"=r" (data));
  return data;
}

unsigned
hal_vect_ipibase (void)
{
  return VECT_IPI0;
}

unsigned
hal_vect_irqbase (void)
{
  return VECT_IRQ0;
}

unsigned
hal_vect_max (void)
{
  return 255;
}

void
i386_init_ap (void)
{
  hal_l1p_t l1p;
  unsigned pcpu = plt_pcpu_id ();

  /* Restore old mapping. Assumes CPUs are brought up one at the time. */
  assert (hal_pmap_getl1p (NULL, smp_oldva[pcpu], false, &l1p));
  hal_pmap_setl1e (NULL, l1p, smp_oldl1e[pcpu]);
  printf ("Restored %lx at %lx\n", smp_oldl1e[pcpu], smp_oldva[pcpu]);
  smp_oldl1e[pcpu] = 0;
  smp_oldva[pcpu] = 0;

  hal_main_ap ();
}

void
i386_init (void)
{
}
