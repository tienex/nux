/** @file
  NUX Timer Management

  Provides high-level timer alarm and time query functions built
  on platform timer services. Converts between nanosecond and
  femtosecond time representations.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <nux/nux.h>
#include <platform/platform.h>
#include <nux/internal.h>

/**
  Set timer alarm.

  Programs the platform timer to fire an alarm after the specified
  time period. Converts nanosecond period to platform timer ticks.

  @param[in] TimeNs  Alarm time in nanoseconds.
**/
VOID
TimerAlarm (
  IN UINT32  TimeNs
  )
{
  UINT64 PeriodFs = plt_tmr_period ();
  UINT64 TimeFs = 1000000LL * TimeNs;

  plt_tmr_setalm ((TimeFs + PeriodFs - 1) / PeriodFs);
}

/**
  Clear timer alarm.

  Cancels any pending timer alarm.
**/
VOID
TimerClear (
  VOID
  )
{
  plt_tmr_clralm ();
}

/**
  Get current time.

  Returns the current time in nanoseconds based on platform
  timer counter.

  @return Current time in nanoseconds.
**/
UINT64
TimerGetTime (
  VOID
  )
{
  UINT64 PeriodFs = plt_tmr_period ();
  UINT64 Ctr = plt_tmr_ctr ();

  return (PeriodFs * Ctr) / 1000000;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use TimerAlarm instead **/
void timer_alarm (UINT32 time_ns) {
  TimerAlarm (time_ns);
}

/** @deprecated Use TimerClear instead **/
void timer_clear (void) {
  TimerClear ();
}

/** @deprecated Use TimerGetTime instead **/
UINT64 timer_gettime (void) {
  return TimerGetTime ();
}
