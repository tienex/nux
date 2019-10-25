#ifndef PLT_H
#define PLT_H

#include <stdbool.h>
#include <nux/types.h>

/*
  PLT Platform.
*/

/* Start platform device drivers. */
void plt_init (void);

/*
 PLT Standard Hardware.
*/

/* Console. */
void plt_hw_putc (int c);

/*
  PLT Platform IRQs.
*/


enum plt_irq_type
{
  PLT_IRQ_EDGE,
  PLT_IRQ_LVLLO,
  PLT_IRQ_LVLHI,
  PLT_IRQ_INVALID,
};

enum plt_irq_type plt_irq_type (unsigned irq);
void plt_irq_enable (unsigned irq);
void plt_irq_disable (unsigned irq);
void plt_irq_eoi (void);

static inline bool
plt_irq_islevel (unsigned irq)
{
  enum plt_irq_type t = plt_irq_type (irq);

  return t == PLT_IRQ_LVLHI || t == PLT_IRQ_LVLLO;
}

/*
  plt_vect_process: Process PLT interrupt vector (called from HAL).

  Platform might need to receive interrupts in case it implements
  device drivers.

  Returns 'true' if PLT timer alarm expired.

  Called from HAL.
*/
bool plt_vect_process (unsigned vect);


/*
  PLT Physical CPU Interface.
*/

#define PLT_PCPU_INVALID -1

/* Non-safe PCPU iterator. */
int plt_pcpu_iterate (void);

/*
  Load Platform specific status for the current CPU.

  This will be called, once for each CPU, at initialisation time. It
  will not be possible to use any pcpu interface (except for
  iterate()) on any CPU before this calls completes.
 */
void plt_pcpu_enter (void);

/* Issue an NMI. */
void plt_pcpu_nmi (int pcpuid);

/* Broadcast an NMI. */
void plt_pcpu_nmiall (void);

/* Issue an IPI. */
void plt_pcpu_ipi (int pcpuid, unsigned vct);

/* Broadcast an IPI. */
void plt_pcpu_ipiall (unsigned vct);

/* Get current pCPU ID. */
unsigned plt_pcpu_id (void);

/* Start-up remote CPU. */
void plt_pcpu_start (unsigned pcpuid, paddr_t paddr);


/*
  PLT Timer Support.
*/

/* Read timer counter. */
uint64_t plt_tmr_ctr (void);

/* Set timer counter. */
void plt_tmr_setctr (uint64_t ctr);

/* Read timer period (in femtoseconds) */
uint64_t plt_tmr_period (void);

/* Set timer alarm in ALM ticks in the future. */
void plt_tmr_setalm (uint64_t alm);

/* Disable timer alarm. */
void plt_tmr_clralm (void);


/*
  PLT Console Support.
*/

void plt_console_putc (int c);


/*
 PLT System Entries.
*/

/* System Entry for alarm timer. */
struct hal_frame *hal_entry_alarm (struct hal_frame *);


#endif
