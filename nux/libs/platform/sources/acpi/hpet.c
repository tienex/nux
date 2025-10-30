/** @file
  HPET (High Precision Event Timer) Support

  Provides HPET timer initialization and management for ACPI platforms.
  Implements platform timer interface using HPET hardware.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <string.h>

#include <platform/acpi/internal.h>
#include <nux/nux.h>

#define HPET_SIZE 0x1000

#define REG_GENCAP 0x00
#define LEG_RT_CAP (1L << 15)

#define REG_GENCFG 0x10
#define LEG_RT_CNF (1L << 1)
#define ENABLE_CNF (1L << 0)

#define REG_GENISR 0x20

#define REG_COUNTER 0xF0

#define REG_TMRCAP(_n) ((0x20 * (_n)) + 0x100)
#define INT_TYPE_CNF (1LL << 1)
#define INT_ENB_CNF (1LL << 2)
#define REG_TMRCMP(_n) ((0x20 * (_n)) + 0x108)

#define TMR 0

static volatile VOID *gHpetBase;
static UINT64 gPeriod = 0;
static UINT64 gGenCfg;
static UINT64 gTmrCfg;

static INT32 gIrqLevel = 0;
static INT32 gIrqNo;

/**
  Read HPET register.

  Performs atomic 64-bit read from HPET MMIO register.

  @param[in] Offset  Register offset.

  @return 64-bit register value.
**/
static UINT64
HpetRead (
  IN UINT16  Offset
  )
{
  UINT32 Hi1, Hi2, Lo;
  volatile UINT32 *Ptr = gHpetBase + Offset;

  do
    {
      Hi1 = *(Ptr + 1);
      Lo = *Ptr;
      Hi2 = *(Ptr + 1);
    }
  while (Hi1 != Hi2);

  return (UINT64) Hi1 << 32 | Lo;
}

/**
  Write HPET register.

  Performs 64-bit write to HPET MMIO register.

  @param[in] Offset  Register offset.
  @param[in] Value   Value to write.
**/
static VOID
HpetWrite (
  IN UINT16  Offset,
  IN UINT64  Value
  )
{
  volatile UINT32 *Ptr = gHpetBase + Offset;
  *(Ptr + 1) = Value >> 32;
  *Ptr = (UINT32) Value;
}

/**
  Pause HPET counter.

  Stops HPET main counter by clearing enable bit.
**/
static VOID
HpetPause (
  VOID
  )
{
  HpetWrite (REG_GENCFG, gGenCfg);
}

/**
  Resume HPET counter.

  Starts HPET main counter by setting enable bit.
**/
static VOID
HpetResume (
  VOID
  )
{
  HpetWrite (REG_GENCFG, ENABLE_CNF | gGenCfg);
}

/**
  Handle HPET IRQ.

  Acknowledges HPET interrupt for level-triggered mode.
**/
VOID
HpetDoIrq (
  VOID
  )
{
  if (gIrqLevel)
    HpetWrite (REG_GENISR, HpetRead (REG_GENISR) | 1);
}

/**
  Initialize HPET timer.

  Maps HPET registers, configures timer, and enables interrupts.

  @param[in] HpetPhysAddr  Physical address of HPET base registers.

  @retval TRUE   HPET initialized successfully.
  @retval FALSE  HPET initialization failed.
**/
BOOLEAN
HpetInitialize (
  IN paddr_t  HpetPhysAddr
  )
{
  UINT64 Freq;
  UINT32 PeriodFemto;
  UINT64 GenCap;
  INT32 Num;

  gHpetBase = KvaMapPhysical (HpetPhysAddr, HPET_SIZE, HAL_PTE_P | HAL_PTE_W);
  GenCap = HpetRead (REG_GENCAP);
  gGenCfg = 0;
  PeriodFemto = GenCap >> 32;
  Num = 1 + ((GenCap >> 8) & 0xf);

  info ("HPET Found at %" PRIx64 ", mapped at %p", HpetPhysAddr, gHpetBase);
  info ("HPET period: %" PRIx32 "x, counters: %d", PeriodFemto, Num);

  if (PeriodFemto == 0)
    {
      error ("HPET period invalid.");
      return FALSE;
    }
  Freq = (1000000000000000LL / PeriodFemto);
  info ("HPET counter frequency: %" PRId64 " Hz", Freq);
  gPeriod = PeriodFemto;

  HpetPause ();		/* Stop, in case it's running */

  /* Find IRQ of first counter. */
  if (GenCap & LEG_RT_CAP && (TMR < 2))
    {
      debug ("Using Legacy Routing.\n");
      gGenCfg |= LEG_RT_CNF;
      if (TMR == 0)
	{
	  gIrqNo = 2;
	}
      else
	{
	  gIrqNo = 8;
	}
    }
  else
    {
      UINT32 IrqCap = HpetRead (REG_TMRCAP (TMR)) >> 32;
      if (IrqCap == 0)
	{
	  error ("No IRQ available, can't use counter %d", TMR);
	  return FALSE;
	}
      gIrqNo = ffs (IrqCap) - 1;
      gTmrCfg |= (gIrqNo << 9);
      debug ("Using Interrupt Routing (%d - %x).\n", gIrqNo, IrqCap);
    }

  if (PlatformIrqIsLevel (gIrqNo))
    {
      debug ("Using Level Interrupt");
      /* Reset ISR just in case. */
      HpetWrite (REG_GENISR, HpetRead (REG_GENISR) | 1);
      gIrqLevel = 1;
      gTmrCfg |= INT_TYPE_CNF;
    }

  /* Register HPET irq no. */
  gPlatformAcpiHpetIrq = gIrqNo;

  /* Start Time of Boot. */
  HpetWrite (REG_COUNTER, 0);

  /* Setup Counter 0 */
  HpetWrite (REG_TMRCAP (TMR), gTmrCfg);

  /* Enable HPET */
  HpetResume ();

  PlatformIrqEnable (gIrqNo);
  return TRUE;
}

/**
  Get platform timer counter.

  Returns current HPET main counter value.

  @return Counter value.
**/
UINT64
PlatformTmrGetCounter (
  VOID
  )
{
  return HpetRead (0xf0);
}

/**
  Set platform timer counter.

  Sets HPET main counter to specified value.

  @param[in] Counter  Counter value to set.
**/
VOID
PlatformTmrSetCounter (
  IN UINT64  Counter
  )
{
  HpetPause ();
  HpetWrite (0xf0, Counter);
  HpetResume ();
}

/**
  Get platform timer period.

  Returns HPET timer period in femtoseconds.

  @return Timer period in femtoseconds.
**/
UINT64
PlatformTmrPeriod (
  VOID
  )
{
  return gPeriod;
}

/**
  Set platform timer alarm.

  Programs HPET comparator for timer interrupt after specified
  number of ticks.

  @param[in] Alarm  Number of ticks until alarm.
**/
VOID
PlatformTmrSetAlarm (
  IN UINT64  Alarm
  )
{
  if (Alarm == 0)
    Alarm = 1;
  HpetPause ();
  HpetWrite (REG_TMRCMP (TMR), PlatformTmrGetCounter () + Alarm);
  HpetWrite (REG_TMRCAP (TMR), gTmrCfg | INT_ENB_CNF);
  HpetResume ();
}

/**
  Clear platform timer alarm.

  Disables HPET timer interrupt.
**/
VOID
PlatformTmrClearAlarm (
  VOID
  )
{
  HpetWrite (REG_TMRCAP (TMR), gTmrCfg & ~INT_ENB_CNF);
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use HpetDoIrq instead **/
void hpet_doirq (void) {
  HpetDoIrq ();
}

/** @deprecated Use HpetInitialize instead **/
bool hpet_init (paddr_t hpetpa) {
  return HpetInitialize (hpetpa);
}

/** @deprecated Use PlatformTmrGetCounter instead **/
uint64_t plt_tmr_ctr (void) {
  return PlatformTmrGetCounter ();
}

/** @deprecated Use PlatformTmrSetCounter instead **/
void plt_tmr_setctr (uint64_t ctr) {
  PlatformTmrSetCounter (ctr);
}

/** @deprecated Use PlatformTmrPeriod instead **/
uint64_t plt_tmr_period (void) {
  return PlatformTmrPeriod ();
}

/** @deprecated Use PlatformTmrSetAlarm instead **/
void plt_tmr_setalm (uint64_t alm) {
  PlatformTmrSetAlarm (alm);
}

/** @deprecated Use PlatformTmrClearAlarm instead **/
void plt_tmr_clralm (void) {
  PlatformTmrClearAlarm ();
}

// Legacy global variable aliases
static volatile void *hpet_base __attribute__((alias("gHpetBase")));
static uint64_t period __attribute__((alias("gPeriod")));
static uint64_t gencfg __attribute__((alias("gGenCfg")));
static uint64_t tmrcfg __attribute__((alias("gTmrCfg")));
static int irqlvl __attribute__((alias("gIrqLevel")));
static int irqno __attribute__((alias("gIrqNo")));
