#include <cdefs.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <nux/hal.h>

#include "i386.h"
#include "../internal.h"

void
hal_pcpu_init (unsigned pcpuid, struct hal_cpu *pcpu)
{
#if 0
  void _set_tss (unsigned, void *);
  void _set_fs (unsigned, void *);

  pcpu->tss.esp0 = KERNBASE + KERN_APBOOT(pcpuid) + 4096;
  pcpu->tss.ss0 = KDS;
  pcpu->tss.iomap = 108;
  pcpu->data = NULL;
  _set_tss (pcpuid, &pcpu->tss);
  _set_fs (pcpuid, &pcpu->data);
#endif
}

void
hal_pcpu_enter (unsigned pcpuid)
{
  uint16_t tss = (5 + 4 * pcpuid) << 3;
  uint16_t fs = (5 + 4 * pcpuid + 1) << 3;

  asm volatile ("ltr %%ax"::"a" (tss));
  asm volatile ("mov %%ax, %%fs"::"a" (fs));
}

uint64_t
hal_pcpu_prepare (unsigned pcpu)
{
#if 0
  extern char *_ap_start, *_ap_end;
  paddr_t pstart = KERN_APBOOT (pcpu);
  void *start = (void *) (KERNBASE + (uintptr_t) pstart);
  volatile uint16_t *reset = (volatile uint16_t *) (KERNBASE + 0x467);

  if (pcpu >= MAXCPUS)
    return PADDR_INVALID;

  *reset = pstart & 0xf;
  *(reset + 1) = pstart >> 4;

  /* Setup AP bootstrap page */
  memcpy (start, &_ap_start,
	  (size_t) ((void *) &_ap_end - (void *) &_ap_start));

  return pstart;
#endif
#warning Resolve the KERN_APBOOT issue.
  /* 
     We should keep one page reserved for AP boot that becomes stack, then
     we allocate a proper page for the AP stack.
     Then we can init another CPU.
  */
  return 0;
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
