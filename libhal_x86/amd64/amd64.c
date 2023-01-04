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
#include <string.h>
#include <nux/hal.h>
#include <nux/plt.h>
#include "amd64.h"
#include "../internal.h"

extern uint64_t _gdt[];
extern int _physmap_start;
extern int _physmap_end;

paddr_t pcpu_pstart;
vaddr_t pcpu_haldata[MAXCPUS];

static unsigned bsp_pcpuid;

static vaddr_t smp_oldva;
static hal_l1e_t smp_oldl1e;

/*
  CPU kernel stack allocation:

   CPUs at boot allocate a stack by incrementing (with a spinlock)
   'kstackno' and using the page pointed at 'pcpu_kstack'.

   Importantly, pcpu_kstack[pcpu_id] doesn't mean is the stack of PCPU ID.
*/
uint64_t pcpu_kstackno = 0;
uint64_t pcpu_kstackcnt = 0;
uint64_t pcpu_kstack[MAXCPUS];

void
set_tss (unsigned pcpuid, struct amd64_tss *tss)
{
  uintptr_t ptr = (uintptr_t) tss;
  uint16_t lo16 = (uint16_t) ptr;
  uint8_t ml8 = (uint8_t) (ptr >> 16);
  uint8_t mh8 = (uint8_t) (ptr >> 24);
  uint32_t hi32 = (uint32_t) (ptr >> 32);
  uint16_t limit = sizeof (*tss);
  uint32_t *ptr32 = (uint32_t *) (_gdt + TSS_GDTIDX (pcpuid));

  ptr32[0] = limit | ((uint32_t) lo16 << 16);
  ptr32[1] = ml8 | (0x0089 << 8) | ((uint32_t) mh8 << 24);
  ptr32[2] = hi32;
  ptr32[3] = 0;
}

void
set_kernel_gsbase (unsigned long gsbase)
{
  wrmsr (MSR_IA32_KERNEL_GS_BASE, gsbase);
}

void
set_gsbase (unsigned long gsbase)
{
  wrmsr (MSR_IA32_GS_BASE, gsbase);
}

uint64_t
get_gsbase (void)
{
  return rdmsr (MSR_IA32_GS_BASE);
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
  return VECT_IRQ0;
}

unsigned
hal_vect_ipibase (void)
{
  return VECT_IPI0;
}

unsigned
hal_vect_max (void)
{
  return 255;
}

static void *
alloc_stackpage (void)
{
  pfn_t pfn;
  void *va;

  pfn = pfn_alloc (1);
  assert (pfn != PFN_INVALID);
  va = kva_map (1, pfn, 1, HAL_PTE_W | HAL_PTE_P);
  assert (va != NULL);
  return va;
}

void
hal_pcpu_add (unsigned pcpuid, struct hal_cpu *haldata)
{

  assert (pcpuid < MAXCPUS);

  if (pcpuid == bsp_pcpuid)
    {
      /* Adding the BSP PCPU: Initialize TSS */
      extern char _bsp_stacktop;
      haldata->kstack = (uintptr_t) & _bsp_stacktop;
      haldata->tss.ist[0] = alloc_stackpage () + PAGE_SIZE;
      haldata->tss.ist[1] = alloc_stackpage () + PAGE_SIZE;
      haldata->tss.ist[2] = alloc_stackpage () + PAGE_SIZE;
      haldata->tss.rsp0 = (uintptr_t) & _bsp_stacktop;
      haldata->tss.iomap = 108;	/* XXX: FIX with sizeof(tss) + 1 */
    }
  else
    {
      /* Adding secondary CPU: Allocate one PCPU kernel stack. */
      void *va = alloc_stackpage ();
      pcpu_kstack[pcpu_kstackno++] = (uint64_t) va + PAGE_SIZE;
    }
  set_tss (pcpuid, &haldata->tss);

  pcpu_haldata[pcpuid] = (vaddr_t) (uintptr_t) haldata;
}

void
hal_pcpu_init (void)
{
  void *va;
  pfn_t pfn;
  void *start, *ptr;
  hal_l1p_t l1p;
  paddr_t pstart;
  volatile uint16_t *reset;
  extern char *_ap_start, *_ap_end;

  /* Allocate PCPU bootstrap code page. */
  pfn = pfn_alloc (1);
  /* This is tricky. The hope is that is low enough to be addressed by
     16 bit. */
  assert (pfn < (1 << 8) && "Can't allocate Memory below 1MB!");

  /* Map and prepare the bootstrap code page. */
  va = kva_map (1, pfn, 1, HAL_PTE_W | HAL_PTE_P);
  assert (va != NULL);
  start = va;
  size_t apbootsz = (size_t) ((void *) &_ap_end - (void *) &_ap_start);
  assert (apbootsz <= PAGE_SIZE);
  memcpy (start, &_ap_start, apbootsz);

  pstart = (paddr_t) pfn << PAGE_SHIFT;

  /*
     The following is trampoline dependent code, and configures the
     trampoline to use the page just selected as bootstrap page.
   */
  extern char _ap_gdtreg, _ap_ljmp1, _ap_ljmp2, _ap_cr3;
  extern uint64_t _bsp_cr3;

  /* Copy BSP CR3 into AP */
  ptr = start + ((void *) &_ap_cr3 - (void *) &_ap_start);
  *(uint64_t *) ptr = _bsp_cr3;

  /* Setup temporary GDT register. */
  ptr = start + ((void *) &_ap_gdtreg - (void *) &_ap_start);
  *(uint32_t *) (ptr + 2) += (uint32_t) pstart;

  /* Setup trampoline 1 */
  ptr = start + ((void *) &_ap_ljmp1 - (void *) &_ap_start);
  *(uint32_t *) ptr += (uint32_t) pstart;
  printf ("Trampoline 1 (%p) = %lx\n", ptr, *(uint32_t *) ptr);

  /* Setup trampoline 2 */
  ptr = start + ((void *) &_ap_ljmp2 - (void *) &_ap_start);
  *(uint32_t *) ptr += (uint32_t) pstart;
  printf ("Trampoline 2 (%p) = %lx\n", ptr, *(uint32_t *) ptr);


  /* Set reset vector */
  reset = kva_physmap (0, 0x467, 2, HAL_PTE_P | HAL_PTE_W | HAL_PTE_X);
  *reset = pstart & 0xf;
  *(reset + 1) = pstart >> 4;
  kva_unmap ((void *) reset);

  assert (hal_pmap_getl1p (NULL, pstart, true, &l1p));
  /* Save the l1e we're abou to overwrite. We'll restore it after init is done. */
  smp_oldl1e = hal_pmap_getl1e (NULL, l1p);
  smp_oldva = pstart;
  hal_pmap_setl1e (NULL, l1p, (pstart & ~PAGE_MASK) | PTE_P | PTE_W);
  pcpu_pstart = pstart;

  bsp_pcpuid = plt_pcpu_id ();
}

uint64_t
hal_pcpu_prepare (unsigned pcpu)
{
  if (pcpu >= MAXCPUS)
    return PADDR_INVALID;

  return pcpu_pstart;
}

void
hal_pcpu_enter (unsigned pcpuid)
{
  assert (pcpuid < MAXCPUS);

  set_gsbase (pcpu_haldata[pcpuid]);
  set_kernel_gsbase (pcpu_haldata[pcpuid]);

  asm volatile ("ltr %%ax"::"a" (TSS_GDTIDX (pcpuid) << 3));
}

void
amd64_init (void)
{
  extern char _syscall_frame_entry;
  wrmsr (MSR_IA32_EFER, rdmsr (MSR_IA32_EFER) | _MSR_IA32_EFER_SCE);
  wrmsr (MSR_IA32_LSTAR, (uintptr_t) & _syscall_frame_entry);
  wrmsr (MSR_IA32_FMASK, 0xfffffffd);
  wrmsr (MSR_IA32_STAR, 0x18L << 48);
}

void
amd64_init_ap (uintptr_t esp)
{
  extern char _syscall_frame_entry;
  unsigned pcpu = plt_pcpu_id ();
  struct hal_cpu *haldata = (struct hal_cpu *) (uintptr_t) pcpu_haldata[pcpu];

  wrmsr (MSR_IA32_EFER, rdmsr (MSR_IA32_EFER) | _MSR_IA32_EFER_SCE);
  wrmsr (MSR_IA32_LSTAR, (uintptr_t) & _syscall_frame_entry);
  wrmsr (MSR_IA32_FMASK, 0xfffffffd);
  wrmsr (MSR_IA32_STAR, 0);

  haldata->kstack = esp;
  haldata->tss.ist[0] = alloc_stackpage () + PAGE_SIZE;
  haldata->tss.ist[1] = alloc_stackpage () + PAGE_SIZE;
  haldata->tss.ist[2] = alloc_stackpage () + PAGE_SIZE;
  haldata->tss.rsp0 = esp;
  haldata->tss.iomap = 108;	/* XXX: FIX with sizeof(tss) + 1 */

  hal_main_ap ();

}

void
amd64_init_done (void)
{
  hal_l1p_t l1p;

  /* Restore the mapping created for boostrapping secondary CPUS. */
  assert (hal_pmap_getl1p (NULL, smp_oldva, true, &l1p));
  hal_pmap_setl1e (NULL, l1p, smp_oldl1e);
}
