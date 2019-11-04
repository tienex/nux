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
#include <stdint.h>
#include <nux/hal.h>
#include "amd64.h"
#include "../internal.h"

extern uint64_t _gdt;
extern int _physmap_start;
extern int _physmap_end;

pfn_t pcpu_stackpfn[MAXCPUS];
vaddr_t pcpu_stackva[MAXCPUS];
vaddr_t pcpu_haldata[MAXCPUS];

void
set_tss (unsigned pcpuid, struct amd64_tss *tss)
{
  uintptr_t ptr = (uintptr_t)tss;
  uint16_t lo16 = (uint16_t)ptr;
  uint8_t ml8 = (uint8_t)(ptr >> 16);
  uint8_t mh8 = (uint8_t)(ptr >> 24);
  uint32_t hi32 = (uint32_t)(ptr >> 32);
  uint16_t limit = sizeof (*tss);
  uint32_t *ptr32 = (uint32_t *)(&_gdt + TSS_GDTIDX(pcpuid));

  ptr32[0] = limit | ((uint32_t)lo16 << 16);
  ptr32[1] = ml8 | (0x0089 << 8) | ((uint32_t)mh8 << 24);
  ptr32[2] = hi32;
  ptr32[3] = 0;
}

void
set_kernel_gsbase (unsigned long gsbase)
{
 wrmsr(MSR_IA32_KERNEL_GS_BASE, gsbase);
}

void
set_gsbase (unsigned long gsbase)
{
  wrmsr(MSR_IA32_GS_BASE, gsbase);
}

uint64_t
get_gsbase (void)
{
  return rdmsr(MSR_IA32_GS_BASE);
}

void
hal_cpu_setdata (void *data)
{
  asm volatile ("movq %0, %%gs:0\n"::"r" (data));
}

void *
hal_cpu_getdata (void)
{
  void *data;

  asm volatile ("movq %%gs:0, %0\n":"=r" (data));
  return data;
}

unsigned
hal_vect_irqbase (void)
{
  return 33;
}

unsigned
hal_vect_ipibase (void)
{
  return 34;
}

unsigned
hal_vect_max (void)
{
  return 255;
}

void
hal_pcpu_init (unsigned pcpuid, struct hal_cpu *haldata)
{
  vaddr_t va;
  pfn_t pfn;

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

  haldata->tss.rsp0 = (unsigned long)pcpu_stackva[pcpuid] + PAGE_SIZE;
  haldata->tss.iomap = 108; /* XXX: FIX with sizeof(tss) + 1 */
  haldata->data = NULL;

  set_tss (pcpuid, &haldata->tss);

  pcpu_haldata[pcpuid] = (vaddr_t)(uintptr_t)haldata;
}

void
hal_pcpu_enter (unsigned pcpuid)
{
  assert (pcpuid < MAXCPUS);

  set_gsbase (pcpu_haldata[pcpuid]);
  set_kernel_gsbase (pcpu_haldata[pcpuid]);
  asm volatile ("ltr %%ax" :: "a" (TSS_GDTIDX(pcpuid) << 3));
}

uint64_t
hal_pcpu_prepare (unsigned pcpu)
{
}

void
amd64_init (void)
{

}
