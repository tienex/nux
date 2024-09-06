
#include <stdio.h>
#include <assert.h>
#include <stdio.h>
#include <nux/nux.h>

#include <nux/hal.h>


int
main (int argc, char *argv[])
{
  printf ("Hello from kernel!\n");

  printf ("Still here?\n");
  cpu_ipi (cpu_id ());
  cpu_nmi (cpu_id ());
  asm volatile ("":::"memory");
  printf ("EXITING!\n");
  return EXIT_IDLE;
}

int
main_ap ()
{
  return 0;
}

uctxt_t *
entry_sysc (uctxt_t * u,
	    unsigned long a1, unsigned long a2, unsigned long a3,
	    unsigned long a4, unsigned long a5, unsigned long a6)
{
  return UCTXT_IDLE;
}

uctxt_t *
entry_ipi (uctxt_t * uctxt)
{
  printf ("IPI\n");
  uctxt_print (uctxt);
  return UCTXT_IDLE;
}

uctxt_t *
entry_alarm (uctxt_t * uctxt)
{
  printf ("ALARM!\n");
  return UCTXT_IDLE;
}

uctxt_t *
entry_ex (uctxt_t * uctxt, unsigned ex)
{
  info ("Exception %d", ex);
  uctxt_print (uctxt);
  return UCTXT_IDLE;
}

uctxt_t *
entry_pf (uctxt_t * uctxt, vaddr_t va, hal_pfinfo_t pfi)
{
  info ("CPU #%d Pagefault at %08lx (%d)", cpu_id (), va, pfi);
  uctxt_print (uctxt);
  return UCTXT_IDLE;
}

uctxt_t *
entry_irq (uctxt_t * uctxt, unsigned irq, bool lvl)
{
  info ("IRQ %d", irq);
  return uctxt;

}
