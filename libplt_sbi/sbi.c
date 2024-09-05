#include <nux/plt.h>

void
plt_init (void)
{
}

void
plt_pcpu_enter (void)
{
  /* TODO */
}

int
plt_pcpu_iterate (void)
{
  static int next_pcpu = 0;

  /* TODO */

  if (next_pcpu++ == 0)
    return 0;
  else
    return PLT_PCPU_INVALID;
}

void
plt_pcpu_ipiall (unsigned vect)
{
  /* TODO */
}

void
plt_pcpu_ipi (int cpu, unsigned vect)
{
  /* TODO */
}

void
plt_pcpu_nmiall (void)
{
  /* TODO */
}

void
plt_pcpu_nmi (int cpu)
{
  /* TODO */
}

void
plt_pcpu_start (unsigned cpu, unsigned long startaddr)
{
  /* TODO */
}

unsigned
plt_pcpu_id (void)
{
  return 0;
}

bool
plt_vect_process (unsigned vect)
{
  /* TODO */
  return false;
}

enum plt_irq_type
plt_irq_type (unsigned irq)
{
  /* TODO */
  return PLT_IRQ_INVALID;
}

void
plt_irq_eoi (void)
{
  /* TODO */
}
