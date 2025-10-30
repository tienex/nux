/** @file
  Local APIC Support

  Provides Local Advanced Programmable Interrupt Controller (LAPIC)
  initialization, configuration, and inter-processor interrupt operations
  for ACPI platforms.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <inttypes.h>
#include <string.h>
#include <assert.h>

#include <platform/acpi/internal.h>
#include <platform/acpi/apic.h>

#include <nux/nux.h>
#include <platform/platform.h>

#define MAXCPUS 256

static VOID *gLapicBase = NULL;
static UINT32 gLapicsNo;

typedef struct _LAPIC_DESC
{
  UINT32 PhysId;
  UINT32 PlatformId;
  UINT32 Lint[2];
} LAPIC_DESC, *PLAPIC_DESC, *PCLAPIC_DESC;

static LAPIC_DESC gLapics[MAXCPUS];

//
// Local APIC Delivery Modes
//

/**
  APIC Delivery Mode
**/
typedef enum _APIC_DELIVERY_MODE {
  ApicDeliveryFixed      = 0,  ///< Fixed delivery mode
  ApicDeliveryPriority   = 1,  ///< Lowest priority delivery
  ApicDeliverySmi        = 2,  ///< System Management Interrupt
  ApicDeliveryNmi        = 4,  ///< Non-Maskable Interrupt
  ApicDeliveryInit       = 5,  ///< INIT IPI
  ApicDeliveryStartup    = 6,  ///< Startup IPI
  ApicDeliveryExtInt     = 7   ///< External Interrupt
} APIC_DELIVERY_MODE;

/** Legacy compatibility **/
#define APIC_DLVR_FIX   ApicDeliveryFixed
#define APIC_DLVR_PRIO  ApicDeliveryPriority
#define APIC_DLVR_SMI   ApicDeliverySmi
#define APIC_DLVR_NMI   ApicDeliveryNmi
#define APIC_DLVR_INIT  ApicDeliveryInit
#define APIC_DLVR_START ApicDeliveryStartup
#define APIC_DLVR_EINT  ApicDeliveryExtInt

//
// LAPIC Registers
//
#define L_IDREG		0x20
#define L_VER		0x30
#define L_TSKPRIO	0x80
#define L_ARBPRIO	0x90
#define L_PROCPRIO	0xa0
#define L_EOI		0xb0
#define L_LOGDEST	0xd0
#define L_DESTFMT	0xe0
#define L_MISC		0xf0	/* Spurious vector */
#define L_ISR		0x100	/* 256 bit */
#define L_TMR		0x180	/* 256 bit */
#define L_IRR		0x200	/* 256 bit */
#define L_ERR		0x280
#define L_ICR_LO	0x300
#define L_ICR_HI	0x310
#define L_LVT_TIMER	0x320
#define L_LVT_THERM	0x330
#define L_LVT_PFMCNT	0x340
#define L_LVT_LINT(x)	(0x350 + (x * 0x10))
#define L_LVT_ERR	0x370
#define L_TMR_IC	0x380
#define L_TMR_CC	0x390
#define L_TMR_DIV	0x3e0

#define LAPIC_SIZE      (1UL << 12)

/**
  Read Local APIC register.

  Performs MMIO read from Local APIC register.

  @param[in] Reg  Register offset.

  @return 32-bit register value.
**/
static UINT32
LapicRead (
  IN UINT32  Reg
  )
{
  return *((volatile UINT32 *) (gLapicBase + Reg));
}

/**
  Write Local APIC register.

  Performs MMIO write to Local APIC register.

  @param[in] Reg   Register offset.
  @param[in] Data  Value to write.
**/
static VOID
LapicWrite (
  IN UINT32  Reg,
  IN UINT32  Data
  )
{
  *((volatile UINT32 *) (gLapicBase + Reg)) = Data;
}

/**
  Get current CPU physical ID.

  Reads the Local APIC ID register to determine the current
  processor's physical ID.

  @return Physical CPU ID, or 0 if LAPIC not initialized.
**/
static UINT32
LapicGetCurrent (
  VOID
  )
{
  if (gLapicBase == NULL)
    return 0;

  return (UINT32) (LapicRead (L_IDREG) >> 24);
}

/**
  Configure Local APIC for current CPU.

  Sets up LINT pins for NMI delivery based on ACPI tables,
  and enables the Local APIC.
**/
static VOID
LapicConfigure (
  VOID
  )
{
  UINT32 i, PhysId = LapicGetCurrent ();
  LAPIC_DESC *Desc = NULL;

  for (i = 0; i < gLapicsNo; i++)
    {
      if (gLapics[i].PhysId == PhysId)
	Desc = gLapics + i;
    }
  if (Desc == NULL)
    {
      warn ("Current CPU not in Platform Tables!");
      /* Try to continue, ignore the NMI configuration */
    }
  else
    {
      LapicWrite (L_LVT_LINT (0), Desc->Lint[0]);
      LapicWrite (L_LVT_LINT (1), Desc->Lint[1]);
    }
  /* Enable LAPIC */
  LapicWrite (L_MISC, LapicRead (L_MISC) | 0x100);
}

/**
  Write Interrupt Command Register.

  Waits for ICR to be idle, then writes the high and low
  32-bit words of the ICR to send an IPI.

  @param[in] Lo  Low 32 bits of ICR.
  @param[in] Hi  High 32 bits of ICR.
**/
static VOID
LapicIcrWrite (
  IN UINT32  Lo,
  IN UINT32  Hi
  )
{
  while (LapicRead (L_ICR_LO) & (1 << 12))
    hal_cpu_relax ();

  LapicWrite (L_ICR_HI, Hi);
  LapicWrite (L_ICR_LO, Lo);
}

/**
  Send IPI to specific CPU.

  Sends an inter-processor interrupt to the specified physical
  CPU ID with the given delivery mode and vector.

  @param[in] PhysId  Physical CPU ID.
  @param[in] Dlvr    Delivery mode.
  @param[in] Vct     Interrupt vector.
**/
static VOID
LapicIpi (
  IN UINT32  PhysId,
  IN UINT8   Dlvr,
  IN UINT8   Vct
  )
{
  UINT32 Hi, Lo;

  Lo = 0x4000 | (Dlvr & 0x7) << 8 | Vct;
  Hi = (PhysId & 0xff) << 24;
  LapicIcrWrite (Lo, Hi);
}

/**
  Broadcast IPI to all CPUs except self.

  Sends an inter-processor interrupt to all processors
  except the current one.

  @param[in] Dlvr  Delivery mode.
  @param[in] Vct   Interrupt vector.
**/
static VOID
LapicIpiBroadcast (
  IN UINT8  Dlvr,
  IN UINT8  Vct
  )
{
  UINT32 Lo;

  Lo = (Dlvr & 0x7) << 8 | Vct | /*ALLBUTSELF*/ 0xc0000 | /*ASSERT*/ 0x4000;
  LapicIcrWrite (Lo, 0);
}

/**
  Send End of Interrupt.

  Signals end of interrupt processing to the Local APIC.
**/
VOID
LapicEoi (
  VOID
  )
{
  LapicWrite (L_EOI, 0);
}

/**
  Configure NMI for CPU.

  Sets up NMI delivery on specified LINT pin for a processor
  or all processors (if pid == 0xff).

  @param[in] Pid  Platform ID (0xff for all CPUs).
  @param[in] L    LINT pin number (0 or 1).
**/
VOID
LapicAddNmi (
  IN UINT8  Pid,
  IN INT32  L
  )
{
  INT32 i;

  if (Pid == 0xff)
    {
      for (i = 0; i < gLapicsNo; i++)
	gLapics[i].Lint[L] = (1L << 16) | (APIC_DLVR_NMI << 8);
      return;
    }
  for (i = 0; i < gLapicsNo; i++)
    {
      if (gLapics[i].PlatformId == Pid)
	{
	  if (L)
	    L = 1;
	  gLapics[i].Lint[L] = (APIC_DLVR_NMI << 8);
	  return;
	}
    }
  warn ("LAPIC NMI for non-existing platform ID %d", Pid);
}

/**
  Add Local APIC to table.

  Registers a Local APIC with its physical and platform IDs.

  @param[in] PhysId  Physical CPU ID.
  @param[in] PlId    Platform CPU ID.
**/
VOID
LapicAdd (
  IN UINT16  PhysId,
  IN UINT16  PlId
  )
{
  static UINT32 i = 0;

  if (i < MAXCPUS)
    {
      gLapics[i].PhysId = PhysId;
      gLapics[i].PlatformId = PlId;
      gLapics[i].Lint[0] = 0x10000;
      gLapics[i].Lint[1] = 0x10000;
      i++;
    }
  else
    {
      warn ("PCPU%d: MAXCPUS reached. Skipping", PhysId);
    }
}

/**
  Initialize Local APIC.

  Maps Local APIC registers and sets the number of processors.

  @param[in] Base  Physical base address of Local APIC.
  @param[in] No    Number of processors.
**/
VOID
LapicInitialize (
  IN UINT64  Base,
  IN UINT32  No
  )
{
  gLapicBase = KvaMapPhysical (Base, LAPIC_SIZE, HAL_PTE_P | HAL_PTE_W);
  gLapicsNo = No < MAXCPUS ? No : MAXCPUS;
  debug ("LAPIC PA: %08" PRIx64 " VA: %p", Base, gLapicBase);
}

//
// PCPU Module: Abstracted CPU Operations
//

/**
  Iterate through processors.

  Returns the physical ID of the next processor in sequence.
  Returns PLATFORM_PCPU_INVALID when iteration is complete.

  @return Physical CPU ID, or PLATFORM_PCPU_INVALID.
**/
INT32
PlatformPcpuIterate (
  VOID
  )
{
  static INT32 t = 0;

  if (t < gLapicsNo)
    return gLapics[t++].PhysId;
  else
    {
      t = 0;
      return PLATFORM_PCPU_INVALID;
    }
}

/**
  Enter processor.

  Configures the Local APIC for the current processor.
**/
VOID
PlatformPcpuEnter (
  VOID
  )
{
  LapicConfigure ();
}

/**
  Send NMI to processor.

  Sends a Non-Maskable Interrupt to the specified processor.

  @param[in] PcpuId  Physical CPU ID.
**/
VOID
PlatformPcpuNmi (
  IN INT32  PcpuId
  )
{
  LapicIpi (PcpuId, APIC_DLVR_NMI, 0);
}

/**
  Broadcast NMI to all processors.

  Sends a Non-Maskable Interrupt to all processors except self.
**/
VOID
PlatformPcpuNmiAll (
  VOID
  )
{
  LapicIpiBroadcast (APIC_DLVR_NMI, 0);
}

/**
  Send IPI to processor.

  Sends an inter-processor interrupt to the specified processor.

  @param[in] PcpuId  Physical CPU ID.
**/
VOID
PlatformPcpuIpi (
  IN INT32  PcpuId
  )
{
  LapicIpi (PcpuId, APIC_DLVR_FIX, APIC_VECT_IPIBASE);
}

/**
  Broadcast IPI to all processors.

  Sends an inter-processor interrupt to all processors except self.
**/
VOID
PlatformPcpuIpiAll (
  VOID
  )
{
  LapicIpiBroadcast (APIC_DLVR_FIX, APIC_VECT_IPIBASE);
}

/**
  Get current processor ID.

  Returns the physical ID of the current processor.

  @return Physical CPU ID.
**/
UINT32
PlatformPcpuId (
  VOID
  )
{
  return LapicGetCurrent ();
}

/**
  Start processor.

  Starts the specified processor using the INIT-SIPI-SIPI
  sequence with the given startup address.

  @param[in] PcpuId  Physical CPU ID.
  @param[in] Start   Physical startup address.
**/
VOID
PlatformPcpuStart (
  IN UINT32   PcpuId,
  IN PHYSICAL_ADDRESS  Start
  )
{
  if (PcpuId == LapicGetCurrent ())
    return;

  /* INIT-SIPI-SIPI sequence. */
  LapicIpi (PcpuId, APIC_DLVR_INIT, 0);
  HwDelay ();
  LapicIpi (PcpuId, APIC_DLVR_START, Start >> 12);
  HwDelay ();
  LapicIpi (PcpuId, APIC_DLVR_START, Start >> 12);
  HwDelay ();
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use LapicRead instead **/
static uint32_t lapic_read (unsigned reg) {
  return LapicRead (reg);
}

/** @deprecated Use LapicWrite instead **/
static void lapic_write (unsigned reg, uint32_t data) {
  LapicWrite (reg, data);
}

/** @deprecated Use LapicGetCurrent instead **/
static unsigned lapic_getcurrent (void) {
  return LapicGetCurrent ();
}

/** @deprecated Use LapicConfigure instead **/
static void lapic_configure (void) {
  LapicConfigure ();
}

/** @deprecated Use LapicIcrWrite instead **/
static void lapic_icr_write (uint32_t lo, uint32_t hi) {
  LapicIcrWrite (lo, hi);
}

/** @deprecated Use LapicIpi instead **/
static void lapic_ipi (unsigned physid, uint8_t dlvr, uint8_t vct) {
  LapicIpi (physid, dlvr, vct);
}

/** @deprecated Use LapicIpiBroadcast instead **/
static void lapic_ipi_broadcast (uint8_t dlvr, uint8_t vct) {
  LapicIpiBroadcast (dlvr, vct);
}

/** @deprecated Use LapicEoi instead **/
void lapic_eoi (void) {
  LapicEoi ();
}

/** @deprecated Use LapicAddNmi instead **/
void lapic_add_nmi (uint8_t pid, int l) {
  LapicAddNmi (pid, l);
}

/** @deprecated Use LapicAdd instead **/
void lapic_add (uint16_t physid, uint16_t plid) {
  LapicAdd (physid, plid);
}

/** @deprecated Use LapicInitialize instead **/
void lapic_init (uint64_t base, unsigned no) {
  LapicInitialize (base, no);
}

/** @deprecated Use PlatformPcpuIterate instead **/
int plt_pcpu_iterate (void) {
  return PlatformPcpuIterate ();
}

/** @deprecated Use PlatformPcpuEnter instead **/
void plt_pcpu_enter (void) {
  PlatformPcpuEnter ();
}

/** @deprecated Use PlatformPcpuNmi instead **/
void plt_pcpu_nmi (int pcpuid) {
  PlatformPcpuNmi (pcpuid);
}

/** @deprecated Use PlatformPcpuNmiAll instead **/
void plt_pcpu_nmiall (void) {
  PlatformPcpuNmiAll ();
}

/** @deprecated Use PlatformPcpuIpi instead **/
void plt_pcpu_ipi (int pcpuid) {
  PlatformPcpuIpi (pcpuid);
}

/** @deprecated Use PlatformPcpuIpiAll instead **/
void plt_pcpu_ipiall (void) {
  PlatformPcpuIpiAll ();
}

/** @deprecated Use PlatformPcpuId instead **/
unsigned plt_pcpu_id (void) {
  return PlatformPcpuId ();
}

/** @deprecated Use PlatformPcpuStart instead **/
void plt_pcpu_start (unsigned pcpuid, PHYSICAL_ADDRESS start) {
  PlatformPcpuStart (pcpuid, start);
}

// Legacy global variable aliases
static void *lapic_base __attribute__((alias("gLapicBase")));
static unsigned lapics_no __attribute__((alias("gLapicsNo")));
