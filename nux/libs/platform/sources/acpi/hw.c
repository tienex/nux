/** @file
  ACPI Platform Hardware Support

  Provides low-level hardware access functions for CMOS and timing.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#define halerror printf
#include <stdint.h>
#include <nux/types.h>
#include <hal/hal.h>
#include <nux/nux.h>

#include "platform/acpi/internal.h"

/**
  Write to CMOS register.

  Writes a value to the specified CMOS register using I/O ports
  0x70 (address) and 0x71 (data).

  @param[in] Address  CMOS register address.
  @param[in] Value    Value to write.
**/
VOID
HwCmosWrite (
  IN UINT8  Address,
  IN UINT8  Value
  )
{
  hal_cpu_out (1, 0x70, Address);
  hal_cpu_out (1, 0x71, Value);
}

/**
  Brief hardware delay.

  Provides a short delay by relaxing the CPU for a fixed number
  of iterations.
**/
VOID
HwDelay (
  VOID
  )
{
  INT32 i;
  for (i = 0; i < 1000; i++)
    hal_cpu_relax ();
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use HwCmosWrite instead **/
void hw_cmos_write (uint8_t addr, uint8_t val) {
  HwCmosWrite (addr, val);
}

/** @deprecated Use HwDelay instead **/
void hw_delay (void) {
  HwDelay ();
}
