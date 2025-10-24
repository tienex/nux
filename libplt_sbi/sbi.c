/** @file
  RISC-V SBI Platform Support

  Provides Supervisor Binary Interface (SBI) platform initialization
  for RISC-V systems. Implements device tree parsing, PLIC discovery,
  timer management, and inter-processor interrupts.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <nux/plt.h>
#include <nux/nmiemul.h>
#include <nux/apxh.h>
#include <nux/hal.h>
#include <nux/nux.h>
#include <libfdt.h>
#include <string.h>

static UINT64 gTimebaseFrequency = 0;
static UINT64 gTmrOffset = 0;

#if 0
typedef struct plt_cpu
{
  UINT32 PlicCtx;  /* S-mode PLIC context. */
} PLT_CPU;

static PLT_CPU gPltCpus[HAL_MAXCPUS];
#endif

/**
  Get device tree cell sizes.

  Retrieves #address-cells and #size-cells properties from parent node.

  @param[in]  pFdt       Pointer to device tree.
  @param[in]  NodeOff    Node offset.
  @param[out] pAddrCells Number of address cells.
  @param[out] pSizeCells Number of size cells.
**/
static VOID
GetCells (
  IN CONST VOID  *pFdt,
  IN INT32       NodeOff,
  OUT UINT32     *pAddrCells,
  OUT UINT32     *pSizeCells
  )
{
  UINT32 AddrCells, SizeCells;
  INT32 Len;
  CONST VOID *pProp;

  /* Initialise to spec default. */
  AddrCells = 2;
  SizeCells = 1;

  NodeOff = fdt_parent_offset (pFdt, NodeOff);

  pProp = fdt_getprop (pFdt, NodeOff, "#address-cells", &Len);
  if (pProp && Len == sizeof (UINT32))
    {
      AddrCells = fdt32_to_cpu (*(UINT32 *) pProp);
    }
  else
    {
      warn ("DT: warning: using default #address-cells %d for node %s\n", AddrCells,
	    fdt_get_name (pFdt, NodeOff, NULL));
    }

  pProp = fdt_getprop (pFdt, NodeOff, "#size-cells", &Len);
  if (pProp && Len == sizeof (UINT32))
    {
      SizeCells = fdt32_to_cpu (*(UINT32 *) pProp);
    }
  else
    {
      warn ("DT: warning: using default #size-cells %d for node %s\n", SizeCells,
	    fdt_get_name (pFdt, NodeOff, NULL));
    }

  *pAddrCells = AddrCells;
  *pSizeCells = SizeCells;
}

/**
  Get device tree register property.

  Extracts base address and length from 'reg' property.

  @param[in]  pFdt    Pointer to device tree.
  @param[in]  NodeOff Node offset.
  @param[in]  Index   Register index.
  @param[out] pBase   Base address (optional).
  @param[out] pLength Length (optional).

  @retval TRUE   Register property found.
  @retval FALSE  Register property not found.
**/
static BOOLEAN
GetReg (
  IN CONST VOID     *pFdt,
  IN INT32          NodeOff,
  IN UINT32         Index,
  OUT OPTIONAL UINT64  *pBase,
  OUT OPTIONAL UINT64  *pLength
  )
{
  INT32 Len;
  CONST VOID *pProp;
  UINT32 AddrSz, SizeSz, RegSz;
  UINT64 Base, Length;

  GetCells (pFdt, NodeOff, &AddrSz, &SizeSz);
  RegSz = AddrSz + SizeSz;

  pProp = fdt_getprop (pFdt, NodeOff, "reg", &Len);
  if (!pProp)
    return FALSE;

  if (Len < (Index + 1) * RegSz * sizeof (UINT32))
    return FALSE;

  pProp += Index * RegSz * sizeof (UINT32);

  Base = 0;
  for (UINT32 i = 0; i < AddrSz; i++)
    {
      Base = (Base << 32) | fdt32_to_cpu (*(UINT32 *) pProp);
      pProp += sizeof (UINT32);
    }

  Length = 0;
  for (UINT32 i = 0; i < SizeSz; i++)
    {
      Length = (Length << 32) | fdt32_to_cpu (*(UINT32 *) pProp);
      pProp += sizeof (UINT32);
    }

  if (pBase)
    *pBase = Base;
  if (pLength)
    *pLength = Length;
  return TRUE;
}

/**
  Initialize PLIC from device tree.

  Discovers PLIC base address and context mappings from device tree.

  @param[in] pFdt    Pointer to device tree.
  @param[in] NodeOff PLIC node offset.
**/
static VOID
PlicInitialize (
  IN CONST VOID  *pFdt,
  IN INT32       NodeOff
  )
{
  INT32 Len;
  CONST VOID *pProp;
  UINT64 Base, Length;

  if (!GetReg (pFdt, NodeOff, 0, &Base, &Length))
    return;
  printf ("PLIC: %s [%016" PRIx64 ":%016" PRIx64 "]\n",
	  fdt_get_name (pFdt, NodeOff, NULL), Base, Base + Length);

  pProp = fdt_getprop (pFdt, NodeOff, "interrupts-extended", &Len);

  printf ("PLIC: External Interrupts Contexts: ");
  for (INT32 i = 0; i < Len; i += sizeof (UINT32) * 2)
    {
      UINT32 PHandle, Intr;
      PHandle = fdt32_to_cpu (*(UINT32 *) (pProp + i));
      Intr = fdt32_to_cpu (*((UINT32 *) (pProp + i) + 1));

      /*
       * External Interrupts for S-mode. The bit we're interested
       * about.
       */
      if (Intr == 9)
	{
	  UINT64 Cpu;
	  INT32 HartOff, ParentOff;

	  HartOff = fdt_node_offset_by_phandle (pFdt, PHandle);
	  if (HartOff < 0)
	    continue;
	  ParentOff = fdt_parent_offset (pFdt, HartOff);
	  if (ParentOff < 0)
	    continue;
	  if (!GetReg (pFdt, ParentOff, 0, &Cpu, NULL))
	    continue;

	  printf ("%" PRId64 "[%d] ", Cpu, i / (sizeof (UINT32) * 2));
	}
    }
  printf ("\n");
}

/**
  Initialize platform.

  Parses device tree to discover CPUs, PLIC, and timebase frequency.
**/
VOID
PltInitialize (
  VOID
  )
{
  CONST struct apxh_pltdesc *pDesc;
  struct fdt_header *pFdtHeader;
  CONST VOID *pFdt, *pProp;
  INT32 Len, CpusOff;
  UINT32 Size;

  pDesc = hal_pltinfo ();
  if (pDesc == NULL)
    fatal ("Invalid PLT Boot Table.");

  if (pDesc->type != PLT_DTB)
    fatal ("No Device Tree Found.");

  info ("DT: DTB at %016" PRIx64, pDesc->pltptr);

  pFdtHeader = (struct fdt_header *) KvaMapPhysical (pDesc->pltptr, sizeof (*pFdtHeader),
				       HAL_PTE_P);
  if (fdt_check_header (pFdtHeader) != 0)
    fatal ("Invalid DTB Header.");

  Size = fdt32_to_cpu (pFdtHeader->totalsize);

  KvaUnmap (pFdtHeader, sizeof (*pFdtHeader));

  pFdt = (CONST VOID *) KvaMapPhysical (pDesc->pltptr, Size, HAL_PTE_P);

  /*
   * Scan the /cpus node, gathering information about HARTs, timer and
   * interrupts.
   */
  CpusOff = fdt_path_offset (pFdt, "/cpus");
  if (CpusOff < 0)
    {
      fatal ("Device tree does not contain '/cpus' node.");
    }

  /*
   * Technically the Device Tree specification says that
   * 'timebase-frequency' should be a property of a single CPU node.
   * Practically in RV this is often found in /cpus.
   */
  pProp = fdt_getprop (pFdt, CpusOff, "timebase-frequency", &Len);
  if (pProp != NULL)
    {
      if (Len == sizeof (UINT32))
	{
	  gTimebaseFrequency = fdt32_to_cpu (*(UINT32 *) pProp);
	}
      else if (Len == sizeof (UINT64))
	{
	  gTimebaseFrequency = fdt32_to_cpu (*(UINT64 *) pProp);
	}
      else
	{
	  warn ("Unexpected length %d in %s/timebase-frequency\n", Len,
		fdt_get_name (pFdt, CpusOff, NULL));
	}
    }

  printf ("DT: ");

  for (INT32 CpuOff = fdt_first_subnode (pFdt, CpusOff);
       CpuOff >= 0; CpuOff = fdt_next_subnode (pFdt, CpuOff))
    {
      CONST CHAR8 *pName;
      pName = fdt_get_name (pFdt, CpuOff, NULL);
      if (pName == NULL)
	continue;

      if (strncmp (pName, "cpu@", 4) != 0)
	continue;

      printf ("%s ", pName);

      pProp = fdt_getprop (pFdt, CpusOff, "timebase-frequency", &Len);
      if (pProp != NULL)
	{
	  UINT64 Freq;
	  if (Len == sizeof (UINT32))
	    {
	      Freq = fdt32_to_cpu (*(UINT32 *) pProp);
	    }
	  else if (Len == sizeof (UINT64))
	    {
	      Freq = fdt32_to_cpu (*(UINT64 *) pProp);
	    }
	  else
	    continue;		/* Let's ignore an invalid value. Should be fatal? */

	  if (gTimebaseFrequency == 0)
	    gTimebaseFrequency = Freq;
	  else if (gTimebaseFrequency != Freq)
	    fatal ("Inconsistent timebase frequencies: previous %lx, found %lx\n",
	       gTimebaseFrequency, Freq);
	}
    }

  printf ("\n");

  printf ("DT: timebase-frequency: %ld\n", gTimebaseFrequency);


  /* Search for PLIC. */
  for (INT32 NodeOff = fdt_next_node (pFdt, -1, NULL);
       NodeOff >= 0; NodeOff = fdt_next_node (pFdt, NodeOff, NULL))
    {
      INT32 Pos = 0;
      pProp = fdt_getprop (pFdt, NodeOff, "compatible", &Len);
      if (pProp)
	{
	  while (Pos < Len)
	    {
	      if (!strncmp ((CHAR8 *) pProp + Pos, "sifive,plic-1.0.0", 17))
		{
		  PlicInitialize (pFdt, NodeOff);
		}
	      Pos += strlen ((CHAR8 *) pProp + Pos) + 1;
	    }
	}
    }

  KvaUnmap ((VOID *) pFdt, Size);
}

/**
  Enter processor.

  Per-CPU platform initialization.
**/
VOID
PltPcpuEnter (
  VOID
  )
{
  /* TODO */
}

/**
  Iterate through processors.

  Returns the next processor ID in sequence.

  @return Physical CPU ID, or PLT_PCPU_INVALID.
**/
INT32
PltPcpuIterate (
  VOID
  )
{
  static INT32 NextPcpu = 0;

  /* TODO */

  if (NextPcpu++ == 0)
    return 0;
  else
    return PLT_PCPU_INVALID;
}

/**
  Send RISC-V IPI.

  Issues SBI IPI ecall to send inter-processor interrupt.

  @param[in] Mask  CPU mask for IPI targets.
**/
VOID
RiscvIpi (
  IN UINT64  Mask
  )
{
  asm volatile ("mv a0, %0\n"
		"li a7, 4\n" "ecall\n"::"r" (&Mask):"a0", "a1", "a7");
}

/**
  Broadcast IPI to all processors.

  Sends inter-processor interrupt to all processors except self.
**/
VOID
PltPcpuIpiAll (
  VOID
  )
{
  NmiEmulIpiSetAll ();
  asm volatile ("csrsi sip, 2\n");
  RiscvIpi (-1);
}

/**
  Send IPI to processor.

  Sends inter-processor interrupt to specified processor.

  @param[in] Cpu  Target CPU ID.
**/
VOID
PltPcpuIpi (
  IN INT32  Cpu
  )
{
  NmiEmulIpiSet (Cpu);
  if (Cpu == CpuGetId ())
    asm volatile ("csrsi sip, 2\n");
  else
    RiscvIpi (1L << Cpu);
}

/**
  Broadcast NMI to all processors.

  Sends Non-Maskable Interrupt to all processors except self.
**/
VOID
PltPcpuNmiAll (
  VOID
  )
{
  NmiEmulNmiSetAll ();
  asm volatile ("csrsi sip, 2\n");
  RiscvIpi (-1);
}

/**
  Send NMI to processor.

  Sends Non-Maskable Interrupt to specified processor.

  @param[in] Cpu  Target CPU ID.
**/
VOID
PltPcpuNmi (
  IN INT32  Cpu
  )
{
  NmiEmulNmiSet (Cpu);
  if (Cpu == CpuGetId ())
    asm volatile ("csrsi sip, 2\n");
  else
    RiscvIpi (1L << Cpu);
}

/**
  Start processor.

  Starts the specified processor at the given address.

  @param[in] Cpu        Target CPU ID.
  @param[in] StartAddr  Start address.
**/
VOID
PltPcpuStart (
  IN UINT32  Cpu,
  IN UINT64  StartAddr
  )
{
  /* TODO */
}

/**
  Get current processor ID.

  Returns the physical ID of the current processor.

  @return Physical CPU ID.
**/
UINT32
PltPcpuId (
  VOID
  )
{
  return 0;
}

/**
  Process interrupt vector.

  Platform-specific vector processing.

  @param[in] Vect  Interrupt vector.

  @retval TRUE   Vector processed.
  @retval FALSE  Vector not handled.
**/
BOOLEAN
PltVectProcess (
  IN UINT32  Vect
  )
{
  /* TODO */
  return FALSE;
}

/**
  Get IRQ type.

  Returns the interrupt trigger mode for an IRQ.

  @param[in] Irq  IRQ number.

  @return Interrupt type, or PLT_IRQ_INVALID.
**/
enum plt_irq_type
PltIrqGetType (
  IN UINT32  Irq
  )
{
  /* TODO */
  return PLT_IRQ_INVALID;
}

/**
  Enable IRQ.

  Unmasks the specified interrupt.

  @param[in] Irq  IRQ number.
**/
VOID
PltIrqEnable (
  IN UINT32  Irq
  )
{
  /* TODO */
}

/**
  Disable IRQ.

  Masks the specified interrupt.

  @param[in] Irq  IRQ number.
**/
VOID
PltIrqDisable (
  IN UINT32  Irq
  )
{
  /* TODO */
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
  /* TODO */
  return 0;
}

/**
  Send End of Interrupt for IPI.

  Acknowledges IPI interrupt completion.
**/
VOID
PltEoiIpi (
  VOID
  )
{
  /* Nothing. */
}

/**
  Send End of Interrupt for IRQ.

  Acknowledges IRQ interrupt completion.

  @param[in] Irq  IRQ number.
**/
VOID
PltEoiIrq (
  IN UINT32  Irq
  )
{
  /* TODO. */
}

/**
  Send End of Interrupt for timer.

  Acknowledges timer interrupt completion.
**/
VOID
PltEoiTimer (
  VOID
  )
{
  /* Nothing. */
}

/**
  Platform interrupt handler.

  Routes interrupts to appropriate handlers based on vector.

  @param[in] Vect  Interrupt vector.
  @param[in] pFrame HAL frame at entry.

  @return Frame to return to.
**/
struct hal_frame *
PltInterrupt (
  IN UINT32            Vect,
  IN struct hal_frame  *pFrame
  )
{
  struct hal_frame *pResult;

  switch (Vect)
    {
    case 1:			/* Supervisor Software Interrupt. */
      pResult = NmiEmulEntry (pFrame);
      break;

    case 5:			/* Supervisor Timer Interrupt. */
      PltTmrClearAlarm ();
      pResult = hal_entry_timer (pFrame);
      break;

    case 9:			/* Supervisor External Interrupt. */
      /* TODO: External interrupts. */
      pResult = pFrame;
      break;

    default:
      pResult = pFrame;
    }

  return pResult;
}

/**
  Get platform timer counter.

  Reads RISC-V time CSR and applies offset.

  @return Counter value.
**/
UINT64
PltTmrGetCounter (
  VOID
  )
{
  UINT64 Time;

  asm volatile ("rdtime %0\n":"=r" (Time));
  return Time + gTmrOffset;
}

/**
  Set platform timer counter.

  Adjusts timer offset to set virtual counter value.

  @param[in] Counter  Counter value to set.
**/
VOID
PltTmrSetCounter (
  IN UINT64  Counter
  )
{
  UINT64 Time;

  asm volatile ("rdtime %0\n":"=r" (Time));
  gTmrOffset = Counter - Time;
}

/**
  Set platform timer alarm.

  Programs timer comparator via SBI call.

  @param[in] Alarm  Number of ticks until alarm.
**/
VOID
PltTmrSetAlarm (
  IN UINT64  Alarm
  )
{
  Alarm += PltTmrGetCounter ();
  asm volatile ("mv a0, %0\n"
		"mv a6, x0\n"
		"mv a7, x0\n" "ecall\n"::"r" (Alarm):"a0", "a6", "a7");
}

/**
  Get platform timer period.

  Returns timer period in femtoseconds based on timebase frequency.

  @return Timer period in femtoseconds.
**/
UINT64
PltTmrPeriod (
  VOID
  )
{
  return 1000000000000000L / gTimebaseFrequency;
}

/**
  Clear platform timer alarm.

  Disables timer interrupt via SBI call.
**/
VOID
PltTmrClearAlarm (
  VOID
  )
{
  asm volatile ("li a0, -1\n"
		"mv a6, x0\n" "mv a7, x0\n" "ecall\n":::"a0", "a6", "a7");
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use GetCells instead **/
static void _get_cells (const void *fdt, int noff, unsigned *addr, unsigned *size) {
  GetCells (fdt, noff, addr, size);
}

/** @deprecated Use GetReg instead **/
static bool _get_reg (const void *fdt, int noff, unsigned idx, uint64_t *base, uint64_t *length) {
  return GetReg (fdt, noff, idx, base, length);
}

/** @deprecated Use PlicInitialize instead **/
static void plic_init (const void *fdt, int noff) {
  PlicInitialize (fdt, noff);
}

/** @deprecated Use PltInitialize instead **/
void plt_init (void) {
  PltInitialize ();
}

/** @deprecated Use PltPcpuEnter instead **/
void plt_pcpu_enter (void) {
  PltPcpuEnter ();
}

/** @deprecated Use PltPcpuIterate instead **/
int plt_pcpu_iterate (void) {
  return PltPcpuIterate ();
}

/** @deprecated Use RiscvIpi instead **/
void riscv_ipi (unsigned long mask) {
  RiscvIpi (mask);
}

/** @deprecated Use PltPcpuIpiAll instead **/
void plt_pcpu_ipiall (void) {
  PltPcpuIpiAll ();
}

/** @deprecated Use PltPcpuIpi instead **/
void plt_pcpu_ipi (int cpu) {
  PltPcpuIpi (cpu);
}

/** @deprecated Use PltPcpuNmiAll instead **/
void plt_pcpu_nmiall (void) {
  PltPcpuNmiAll ();
}

/** @deprecated Use PltPcpuNmi instead **/
void plt_pcpu_nmi (int cpu) {
  PltPcpuNmi (cpu);
}

/** @deprecated Use PltPcpuStart instead **/
void plt_pcpu_start (unsigned cpu, unsigned long startaddr) {
  PltPcpuStart (cpu, startaddr);
}

/** @deprecated Use PltPcpuId instead **/
unsigned plt_pcpu_id (void) {
  return PltPcpuId ();
}

/** @deprecated Use PltVectProcess instead **/
bool plt_vect_process (unsigned vect) {
  return PltVectProcess (vect);
}

/** @deprecated Use PltIrqGetType instead **/
enum plt_irq_type plt_irq_type (unsigned irq) {
  return PltIrqGetType (irq);
}

/** @deprecated Use PltIrqEnable instead **/
void plt_irq_enable (unsigned irq) {
  PltIrqEnable (irq);
}

/** @deprecated Use PltIrqDisable instead **/
void plt_irq_disable (unsigned irq) {
  PltIrqDisable (irq);
}

/** @deprecated Use PltIrqGetMax instead **/
unsigned plt_irq_max (void) {
  return PltIrqGetMax ();
}

/** @deprecated Use PltEoiIpi instead **/
void plt_eoi_ipi (void) {
  PltEoiIpi ();
}

/** @deprecated Use PltEoiIrq instead **/
void plt_eoi_irq (unsigned irq) {
  PltEoiIrq (irq);
}

/** @deprecated Use PltEoiTimer instead **/
void plt_eoi_timer (void) {
  PltEoiTimer ();
}

/** @deprecated Use PltInterrupt instead **/
struct hal_frame *plt_interrupt (unsigned vect, struct hal_frame *f) {
  return PltInterrupt (vect, f);
}

/** @deprecated Use PltTmrGetCounter instead **/
uint64_t plt_tmr_ctr (void) {
  return PltTmrGetCounter ();
}

/** @deprecated Use PltTmrSetCounter instead **/
void plt_tmr_setctr (uint64_t ctr) {
  PltTmrSetCounter (ctr);
}

/** @deprecated Use PltTmrSetAlarm instead **/
void plt_tmr_setalm (uint64_t alm) {
  PltTmrSetAlarm (alm);
}

/** @deprecated Use PltTmrPeriod instead **/
uint64_t plt_tmr_period (void) {
  return PltTmrPeriod ();
}

/** @deprecated Use PltTmrClearAlarm instead **/
void plt_tmr_clralm (void) {
  PltTmrClearAlarm ();
}

// Legacy global variable aliases
static uint64_t timebase_frequency __attribute__((alias("gTimebaseFrequency")));
static uint64_t tmr_offset __attribute__((alias("gTmrOffset")));
