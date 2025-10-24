/** @file
  I/O APIC Support

  Provides I/O Advanced Programmable Interrupt Controller (I/O APIC)
  initialization, configuration, and Global System Interrupt (GSI) routing
  for ACPI platforms.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <inttypes.h>
#include <assert.h>
#include <nux/nux.h>
#include <nux/hal.h>
#include <nux/plt.h>
#include <nux/nux.h>

#include "internal.h"
#include "apic.h"

static UINT32 gIoapicsNo;

typedef struct ioapic_desc
{
  VOID *Base;
  UINT32 Irq;
  UINT32 Pins;
} IOAPIC_DESC;

static IOAPIC_DESC *gIoapics;
static UINT32 gGsisNo;

typedef struct gsi_desc
{
  UINT32 Irq;
  UINT32 Ioapic;
  UINT32 Pin;
  enum plt_irq_type Mode;
} GSI_DESC;

static GSI_DESC *gGsis;

#define IOAPIC_SIZE 0x20

/* Memory Registers */
#define IO_REGSEL 0x00
#define IO_WIN    0x10

/* I/O Registers */
#define IO_ID          0x00
#define IO_VER         0x01
#define IO_ARB         0x02
#define IO_RED_LO(x)   (0x10 + 2*(x))
#define IO_RED_HI(x)   (0x11 + 2*(x))

/**
  Initialize I/O APIC array.

  Allocates memory for I/O APIC descriptors.

  @param[in] No  Number of I/O APICs.
**/
VOID
IoapicInitialize (
  IN UINT32  No
  )
{
  gIoapics = (IOAPIC_DESC *) KmemBrkGrow (1, sizeof (IOAPIC_DESC) * No);
  gIoapicsNo = No;
}

/**
  Write I/O APIC register.

  Writes to an I/O register via indirect addressing.

  @param[in] Index  I/O APIC index.
  @param[in] Reg    Register number.
  @param[in] Val    Value to write.
**/
static VOID
IoapicWrite (
  IN UINT32  Index,
  IN UINT8   Reg,
  IN UINT32  Val
  )
{
  volatile UINT32 *pRegSel = (UINT32 *) (gIoapics[Index].Base + IO_REGSEL);
  volatile UINT32 *pWin = (UINT32 *) (gIoapics[Index].Base + IO_WIN);

  *pRegSel = Reg;
  *pWin = Val;
}

/**
  Read I/O APIC register.

  Reads from an I/O register via indirect addressing.

  @param[in] Index  I/O APIC index.
  @param[in] Reg    Register number.

  @return Register value.
**/
static UINT32
IoapicRead (
  IN UINT32  Index,
  IN UINT8   Reg
  )
{
  volatile UINT32 *pRegSel = (UINT32 *) (gIoapics[Index].Base + IO_REGSEL);
  volatile UINT32 *pWin = (UINT32 *) (gIoapics[Index].Base + IO_WIN);

  *pRegSel = Reg;
  return *pWin;
}

/**
  Add I/O APIC to array.

  Maps the I/O APIC registers, determines pin count, and masks
  all interrupts.

  @param[in] Num      I/O APIC index.
  @param[in] Base     Physical base address.
  @param[in] IrqBase  Base GSI number.
**/
VOID
IoapicAdd (
  IN UINT32  Num,
  IN UINT64  Base,
  IN UINT32  IrqBase
  )
{
  UINT32 i;

  gIoapics[Num].Base = KvaMapPhysical (Base, IOAPIC_SIZE, HAL_PTE_P | HAL_PTE_W);
  gIoapics[Num].Irq = IrqBase;
  gIoapics[Num].Pins = 1 + ((IoapicRead (Num, IO_VER) >> 16) & 0xff);

  /* Mask all interrupts */
  for (i = 0; i < gIoapics[Num].Pins; i++)
    {
      IoapicWrite (Num, IO_RED_LO (i), 0x00010000);
      IoapicWrite (Num, IO_RED_HI (i), 0x00000000);
    }
  info ("IOAPIC: %02d PA: %08" PRIx64 " VA: %p IRQ:%02d PINS: %02d",
	Num, Base, gIoapics[Num].Base, IrqBase, gIoapics[Num].Pins);
}

/**
  Get maximum IRQ number.

  Calculates the maximum GSI number based on all I/O APICs.

  @return Maximum IRQ number.
**/
UINT32
IoapicGetMaxIrq (
  VOID
  )
{
  UINT32 i;
  UINT32 LastIrq, MaxIrq = 0;

  for (i = 0; i < gIoapicsNo; i++)
    {
      LastIrq = gIoapics[i].Irq + gIoapics[i].Pins;
      if (LastIrq > MaxIrq)
	MaxIrq = LastIrq;
    }

  if (MaxIrq >= APIC_VECT_IRQMAX)
    {
      warn ("Maximum number of IRQs exceeded (%d >= %d). Some IRQs will not be available.",
	    MaxIrq, APIC_VECT_IRQMAX);
      MaxIrq = APIC_VECT_IRQMAX;
    }

  return MaxIrq;
}

/**
  Initialize GSI table.

  Allocates and initializes Global System Interrupt descriptors
  with default identity mapping and ISA edge-triggered modes.
**/
VOID
GsiInitialize (
  VOID
  )
{
  UINT32 i, Irqs = IoapicGetMaxIrq ();
  gGsisNo = Irqs;
  gGsis = (GSI_DESC *) KmemBrkGrow (1, sizeof (GSI_DESC) * Irqs);

  /* Setup identity map, edge triggered (this is ISA) */
  for (i = 0; i < 16; i++)
    {
      gGsis[i].Mode = PLT_IRQ_EDGE;
      gGsis[i].Irq = i;
    }

  for (; i < gGsisNo; i++)
    {
      gGsis[i].Mode = PLT_IRQ_LVLLO;
      gGsis[i].Irq = i;
    }
}

/**
  Set up GSI override.

  Overrides the default GSI to IRQ mapping and interrupt mode
  based on ACPI MADT interrupt source override entries.

  @param[in] Index  GSI number.
  @param[in] Irq    IRQ to map to.
  @param[in] Mode   Interrupt trigger mode.
**/
VOID
GsiSetup (
  IN UINT32              Index,
  IN UINT32              Irq,
  IN enum plt_irq_type   Mode
  )
{
  if (Index >= gGsisNo)
    {
      warn ("Warning: GSI %d bigger than existing I/O APIC GSIs", Index);
      return;
    }
  gGsis[Index].Irq = Irq;
  gGsis[Index].Mode = Mode;
}

/**
  Resolve GSI to I/O APIC and pin.

  Finds which I/O APIC and pin correspond to a GSI number.

  @param[in] Gsi  GSI number.

  @retval TRUE   GSI resolved successfully.
  @retval FALSE  GSI not found in any I/O APIC.
**/
static BOOLEAN
IrqResolve (
  IN UINT32  Gsi
  )
{
  UINT32 i, Start, End;

  for (i = 0; i < gIoapicsNo; i++)
    {
      Start = gIoapics[i].Irq;
      End = Start + gIoapics[i].Pins;

      if ((Gsi >= Start) && (Gsi < End))
	{
	  gGsis[Gsi].Ioapic = i;
	  gGsis[Gsi].Pin = Gsi - Start;
	  return TRUE;
	}
    }
  warn ("GSI not found in IOAPIC: %d", Gsi);
  return FALSE;
}

/**
  Set GSI interrupt type.

  Configures the redirection entry trigger mode for a GSI.

  @param[in] Irq   IRQ number.
  @param[in] Mode  Interrupt trigger mode.
**/
static VOID
GsiSetIrqType (
  IN UINT32              Irq,
  IN enum plt_irq_type   Mode
  )
{
  UINT32 Lo;

  Lo = IoapicRead (gGsis[Irq].Ioapic, IO_RED_LO (gGsis[Irq].Pin));
  Lo &= ~((1L << 13) | (1L << 15));

  gGsis[Irq].Mode = Mode;

  /* Setup Masked IOAPIC entry with no vector information */
  switch (gGsis[Irq].Mode)
    {
    default:
      warn ("Warning: GSI table corrupted. Setting GSI %d to EDGE", Irq);
    case PLT_IRQ_EDGE:
      break;
    case PLT_IRQ_LVLHI:
      Lo |= (1L << 15);
      break;
    case PLT_IRQ_LVLLO:
      Lo |= ((1L << 15) | (1L << 13));
      break;
    }

  IoapicWrite (gGsis[Irq].Ioapic, IO_RED_LO (gGsis[Irq].Pin), Lo);
}

/**
  Register vector for GSI.

  Programs the interrupt vector in the redirection entry.

  @param[in] Gsi   GSI number.
  @param[in] Vect  Interrupt vector.
**/
static VOID
GsiRegister (
  IN UINT32  Gsi,
  IN UINT32  Vect
  )
{
  UINT32 Lo;

  assert (Gsi < gGsisNo);
  assert (Vect < 256);

  Lo = IoapicRead (gGsis[Gsi].Ioapic, IO_RED_LO (gGsis[Gsi].Pin));
  IoapicWrite (gGsis[Gsi].Ioapic, IO_RED_LO (gGsis[Gsi].Pin), Lo | Vect);
}

/**
  Start GSI routing.

  Resolves all GSIs to I/O APIC pins and programs redirection
  entries with interrupt vectors.
**/
VOID
GsiStart (
  VOID
  )
{
  UINT32 i;
  for (i = 0; i < gGsisNo; i++)
    {
      /* Now that we have the proper GSI to IRQ mapping, resolve the
       * IOAPIC/PIN of the GSI. */
      if (IrqResolve (i))
	GsiSetIrqType (i, gGsis[i].Mode);
    }

  /* 1:1 map GSI <-> Kernel IRQ */
  for (i = 0; i < gGsisNo; i++)
    GsiRegister (i, APIC_VECT_IRQBASE + i);
}

/**
  Dump GSI table.

  Prints all GSI mappings for debugging.
**/
VOID
GsiDump (
  VOID
  )
{
  UINT32 i;

  for (i = 0; i < gGsisNo; i++)
    {
      info ("GSI: %02d IRQ: %02d MODE: %5s APIC: %02d PIN: %02d", i,
	    gGsis[i].Irq,
	    gGsis[i].Mode == PLT_IRQ_EDGE ? "EDGE" : gGsis[i].Mode ==
	    PLT_IRQ_LVLHI ? "LVLHI" : "LVLLO", gGsis[i].Ioapic, gGsis[i].Pin);
    }
}

/**
  Get number of IRQs.

  Returns the total number of Global System Interrupts.

  @return Number of GSIs.
**/
UINT32
PltIrqGetNo (
  VOID
  )
{
  return gGsisNo;
}

/**
  Set interrupt vector for GSI.

  Programs the interrupt vector in the I/O APIC redirection entry.

  @param[in] Gsi   GSI number.
  @param[in] Vect  Interrupt vector.
**/
VOID
PltIrqSetVector (
  IN UINT32  Gsi,
  IN UINT32  Vect
  )
{
  if (Gsi < gGsisNo)
    GsiRegister (Gsi, Vect);
  else
    warn ("gsi requested non existent: %d", Gsi);
}

/**
  Get IRQ type.

  Returns the interrupt trigger mode for a GSI.

  @param[in] Gsi  GSI number.

  @return Interrupt type, or PLT_IRQ_INVALID if GSI invalid.
**/
enum plt_irq_type
PltIrqGetType (
  IN UINT32  Gsi
  )
{
  if (Gsi < gGsisNo)
    return gGsis[Gsi].Mode;
  else
    return PLT_IRQ_INVALID;
}

/**
  Enable IRQ.

  Unmasks the interrupt in the I/O APIC redirection entry.

  @param[in] Gsi  GSI number.
**/
VOID
PltIrqEnable (
  IN UINT32  Gsi
  )
{
  UINT32 Lo;

  Lo = IoapicRead (gGsis[Gsi].Ioapic, IO_RED_LO (gGsis[Gsi].Pin));
  Lo &= ~0x10000L;		/* UNMASK */
  IoapicWrite (gGsis[Gsi].Ioapic, IO_RED_LO (gGsis[Gsi].Pin), Lo);
}

/**
  Disable IRQ.

  Masks the interrupt in the I/O APIC redirection entry.

  @param[in] Gsi  GSI number.
**/
VOID
PltIrqDisable (
  IN UINT32  Gsi
  )
{
  UINT32 Lo;

  Lo = IoapicRead (gGsis[Gsi].Ioapic, IO_RED_LO (gGsis[Gsi].Pin));
  Lo |= 0x10000L;		/* MASK */
  IoapicWrite (gGsis[Gsi].Ioapic, IO_RED_LO (gGsis[Gsi].Pin), Lo);
}

/**
  Get maximum IRQ number.

  Returns the maximum IRQ number supported by the platform.

  @return Maximum IRQ number.
**/
UINT32
PltIrqGetMax (
  VOID
  )
{
  return APIC_VECT_IRQMAX;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use IoapicInitialize instead **/
void ioapic_init (unsigned no) {
  IoapicInitialize (no);
}

/** @deprecated Use IoapicWrite instead **/
static void ioapic_write (unsigned i, uint8_t reg, uint32_t val) {
  IoapicWrite (i, reg, val);
}

/** @deprecated Use IoapicRead instead **/
static uint32_t ioapic_read (unsigned i, uint8_t reg) {
  return IoapicRead (i, reg);
}

/** @deprecated Use IoapicAdd instead **/
void ioapic_add (unsigned num, uint64_t base, unsigned irqbase) {
  IoapicAdd (num, base, irqbase);
}

/** @deprecated Use IoapicGetMaxIrq instead **/
unsigned ioapic_irqs (void) {
  return IoapicGetMaxIrq ();
}

/** @deprecated Use GsiInitialize instead **/
void gsi_init (void) {
  GsiInitialize ();
}

/** @deprecated Use GsiSetup instead **/
void gsi_setup (unsigned i, unsigned irq, enum plt_irq_type mode) {
  GsiSetup (i, irq, mode);
}

/** @deprecated Use IrqResolve instead **/
static bool irqresolve (unsigned gsi) {
  return IrqResolve (gsi);
}

/** @deprecated Use GsiSetIrqType instead **/
static void gsi_set_irqtype (unsigned irq, enum plt_irq_type mode) {
  GsiSetIrqType (irq, mode);
}

/** @deprecated Use GsiRegister instead **/
static void gsi_register (unsigned gsi, unsigned vect) {
  GsiRegister (gsi, vect);
}

/** @deprecated Use GsiStart instead **/
void gsi_start (void) {
  GsiStart ();
}

/** @deprecated Use GsiDump instead **/
void gsi_dump (void) {
  GsiDump ();
}

/** @deprecated Use PltIrqGetNo instead **/
unsigned plt_irq_no (void) {
  return PltIrqGetNo ();
}

/** @deprecated Use PltIrqSetVector instead **/
void plt_irq_setvector (unsigned gsi, unsigned vect) {
  PltIrqSetVector (gsi, vect);
}

/** @deprecated Use PltIrqGetType instead **/
enum plt_irq_type plt_irq_type (unsigned gsi) {
  return PltIrqGetType (gsi);
}

/** @deprecated Use PltIrqEnable instead **/
void plt_irq_enable (unsigned gsi) {
  PltIrqEnable (gsi);
}

/** @deprecated Use PltIrqDisable instead **/
void plt_irq_disable (unsigned gsi) {
  PltIrqDisable (gsi);
}

/** @deprecated Use PltIrqGetMax instead **/
unsigned plt_irq_max (void) {
  return PltIrqGetMax ();
}

// Legacy global variable aliases
static unsigned ioapics_no __attribute__((alias("gIoapicsNo")));
static unsigned gsis_no __attribute__((alias("gGsisNo")));
