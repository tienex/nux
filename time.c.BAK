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

#include <nux/nux.h>
#include <nux/plt.h>
#include "internal.h"

void
timer_alarm (uint32_t time_ns)
{
  uint64_t period_fs = plt_tmr_period ();
  uint64_t time_fs = 1000000LL * time_ns;

  plt_tmr_setalm ((time_fs + period_fs - 1) / period_fs);
}

void
timer_clear (void)
{
  plt_tmr_clralm ();
}

uint64_t
timer_gettime (void)
{
  uint64_t period_fs = plt_tmr_period ();
  uint64_t ctr = plt_tmr_ctr ();

  return (period_fs * ctr) / 1000000;
}
