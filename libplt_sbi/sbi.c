#include <nux/plt.h>
#include <nux/nmiemul.h>
#include <nux/nux.h>

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
riscv_ipi (unsigned long mask)
{
  asm volatile ("mv a0, %0\n"
		"li a7, 4\n" "ecall\n"::"r" (&mask):"a0", "a1", "a7");
}

void
plt_pcpu_ipiall (void)
{
  nmiemul_ipi_setall ();
  asm volatile ("csrsi sip, 2\n");
  riscv_ipi (-1);
}

void
plt_pcpu_ipi (int cpu)
{
  nmiemul_ipi_set (cpu);
  if (cpu == cpu_id ())
    asm volatile ("csrsi sip, 2\n");
  else
  riscv_ipi (1L << cpu);
}

void
plt_pcpu_nmiall (void)
{
  nmiemul_nmi_setall ();
  asm volatile ("csrsi sip, 2\n");
  riscv_ipi (-1);
}

void
plt_pcpu_nmi (int cpu)
{
  nmiemul_nmi_set (cpu);
  if (cpu == cpu_id ())
    asm volatile ("csrsi sip, 2\n");
  else
  riscv_ipi (1L << cpu);
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

void
plt_vect_translate (unsigned vect, struct plt_vect_desc *desc)
{
  desc->type = PLT_VECT_IGN;
  desc->no = 0;
}
