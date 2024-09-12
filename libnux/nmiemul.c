#include "internal.h"
#include <nux/hal_config.h>	/* For HAL_NMIEMUL */
#include <nux/nmiemul.h>
#include <nux/nux.h>
#include <assert.h>

#ifdef HAL_NMIEMUL

#define PENDING_NMI 1
#define PENDING_IPI 2
static uint8_t pending[HAL_MAXCPUS];

static bool
nmiemul_nmi_pending (void)
{
  unsigned cpu = cpu_id ();
  uint8_t p;

  assert (cpu < HAL_MAXCPUS);

  __atomic_load (pending + cpu, &p, __ATOMIC_ACQUIRE);
  if (p & PENDING_NMI)
    return true;
  return false;
}

static void
nmiemul_nmi_clear (void)
{
  unsigned cpu = cpu_id ();

  assert (cpu < HAL_MAXCPUS);

  __atomic_fetch_and (pending + cpu, ~(PENDING_NMI), __ATOMIC_ACQUIRE);
}

bool
nmiemul_ipi_pending (void)
{
  unsigned cpu = cpu_id ();
  uint8_t p;

  assert (cpu < HAL_MAXCPUS);

  __atomic_load (pending + cpu, &p, __ATOMIC_ACQUIRE);
  if (p & PENDING_IPI)
    return true;
  return false;
}

void
nmiemul_ipi_clear (void)
{
  unsigned cpu = cpu_id ();

  assert (cpu < HAL_MAXCPUS);

  __atomic_fetch_and (pending + cpu, ~(PENDING_IPI), __ATOMIC_ACQUIRE);
}

void
nmiemul_nmi_set (unsigned cpu)
{
  assert (cpu < HAL_MAXCPUS);
  __atomic_fetch_or (pending + cpu, PENDING_NMI, __ATOMIC_RELEASE);

}

void
nmiemul_nmi_setall (void)
{
  for (unsigned i = 0; i < HAL_MAXCPUS; i++)
    __atomic_fetch_or (pending + i, PENDING_NMI, __ATOMIC_RELEASE);

}

void
nmiemul_ipi_set (unsigned cpu)
{
  assert (cpu < HAL_MAXCPUS);
  __atomic_fetch_or (pending + cpu, PENDING_IPI, __ATOMIC_RELEASE);
}

void
nmiemul_ipi_setall (void)
{
  for (unsigned i = 0; i < HAL_MAXCPUS; i++)
    __atomic_fetch_or (pending + i, PENDING_IPI, __ATOMIC_RELEASE);

}

struct hal_frame *
nmiemul_entry (struct hal_frame *f)
{
  if (nmiemul_nmi_pending ())
    {
      nmiemul_nmi_clear ();
      /* void */ hal_entry_nmi (f);
    }
  return f;
}

#endif /* HAVE_NMIEMUL */
