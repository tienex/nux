
#include "internal.h"
#include <stdint.h>
#include <nux/hal.h>

extern uint32_t *_gdt;

void
set_tss (struct amd64_tss *tss)
{
  uintptr_t ptr = (uintptr_t)tss;
  uint16_t lo16 = (uint16_t)ptr;
  uint8_t ml8 = (uint8_t)(ptr >> 16);
  uint8_t mh8 = (uint8_t)(ptr >> 24);
  uint32_t hi32 = (uint32_t)(ptr >> 32);
  uint16_t limit = sizeof (*tss);

  _gdt[TSS_GDTIDX + 0] = limit | (lo16 << 16);
  _gdt[TSS_GDTIDX + 1] = ml8 | (0x0089 << 8) | (mh8 << 24);
  _gdt[TSS_GDTIDX + 2] = hi32;
  _gdt[TSS_GDTIDX + 3] = 0;
}

void
amd64_init (void)
{
  
}

int
hal_putchar (int c)
{
  return vga_putchar (c);
}
