#define halerror prin
#include <stdint.h>
#include <nux/types.h>
#include <nux/hal.h>
#include <nux/nux.h>

#include "internal.h"

int
hw_cmos_write (uint8_t addr, uint8_t val)
{
  hal_cpu_out (1, 0x70, addr);
  hal_cpu_out (1, 0x71, val);
}

void
hw_delay (void)
{
  int i;
  for (i = 0; i < 1000; i++)
    hal_cpu_relax ();
}

void
hw_reset_vector (uint32_t start)
{
  uint16_t * base;

  base = (uint16_t *) kva_physmap (0, 0x467, 4, HAL_PTE_P|HAL_PTE_W);
  if (base == NULL)
    {
      error ("Could not map reset vector at addr 0x467");
      return;
    }

  *base++ = start & 0xf;
  *base   = start >> 4;

  kva_unmap (base);
}

