/** @file
  RISC-V Hardware Abstraction Layer Implementation

  Core RISC-V HAL initialization, CPU operations, memory management,
  and hardware interface functions.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <cdefs.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <framebuffer.h>

#include <hal/hal.h>
#include <apxh/apxh.h>
#include <platform/platform.h>
#include <nux/nmiemul.h>
#include <hal/internal.h>
#include <nux/batree.h>

extern INT32 _info_start;

extern INT32 _stree_start[];
extern INT32 _stree_end[];

extern INT32 _fbuf_start;
extern INT32 _fbuf_end;

extern INT32 _memregs_start;
extern INT32 _memregs_end;

extern UINT64 _riscv64_physmap_start;
extern UINT64 _riscv64_physmap_end;

extern UINT64 _riscv64_pfncache_start;
extern UINT64 _riscv64_pfncache_end;

extern UINT64 _riscv64_kva_start;
extern UINT64 _riscv64_kva_end;

extern UINT64 _riscv64_kmem_start;
extern UINT64 _riscv64_kmem_end;

VOID set_stvec_final ();

CONST struct apxh_bootinfo *bootinfo = (struct apxh_bootinfo *) &_info_start;

FRAMEBUFFER_DESC fbdesc;
struct apxh_platformdesc pltdesc;

VOID *gHalStreePtr;
UINT32 gHalStreeOrder;

int gUseFb;
INT32 gNuxInitialized = 0;

struct hal_cpu *pcpu_haldata[HAL_MAXCPUS];

/**
  Print a string during early boot.

  @param[in] Str  String to print.
**/
static VOID
EarlyPrint (
  IN CONST CHAR8  *Str
  )
{
  CHAR8 *ptr = (CHAR8 *) Str;

  while (*ptr != '\0')
    hal_putchar (*ptr++);
}

//
// I/O ops aren't implemented in RISC-V.
//

UINTN
hal_cpu_in (
  IN UINT8   size,
  IN UINT32  port
  )
{
  return 0;
}

VOID
hal_cpu_out (
  IN UINT8   size,
  IN UINT32  port,
  IN UINTN   val
  )
{
}

VOID
hal_cpu_relax (
  VOID
  )
{
  ANX_CPU_NOP();
}

VOID
hal_cpu_trap (
  VOID
  )
{
  ANX_CPU_BREAKPOINT();
}

VOID
hal_cpu_idle (
  VOID
  )
{
  riscv_sie_user ();

  /* If IPI is pending, manually enable SI. */
  if (NmiEmulIpiPending ())
    ANX_CPU_CSR_SET_IMM(sip, SIP_SSIP);

  while (1)
    {
      ANX_CPU_RISCV_ENABLE_INTR_WFI();
    }
}

VOID
hal_cpu_halt (
  VOID
  )
{
  while (1)
    ANX_CPU_RISCV_DISABLE_INTR_LOOP();
}

UINT64
hal_cpu_cycles (
  VOID
  )
{
  return ANX_CPU_RDTSC();
}

VOID
hal_cpu_tlbop (
  IN hal_tlbop_t  tlbop
  )
{
  if (tlbop == HAL_TLBOP_NONE)
    return;

  ANX_CPU_FLUSHTLB();
}

VOID
hal_useraccess_start (
  VOID
  )
{
  ANX_CPU_CSR_SET(sstatus, SSTATUS_SUM);
}

VOID
hal_useraccess_end (
  VOID
  )
{
  ANX_CPU_CSR_CLEAR(sstatus, SSTATUS_SUM);
}

VIRTUAL_ADDRESS
hal_virtmem_dmapbase (
  VOID
  )
{
  return _riscv64_physmap_start;
}

CONST UINTN
hal_virtmem_dmapsize (
  VOID
  )
{
  return (UINTN) (_riscv64_physmap_end - _riscv64_physmap_start);
}

VIRTUAL_ADDRESS
hal_virtmem_pfn$base (
  VOID
  )
{
  return _riscv64_pfncache_start;
}

CONST UINTN
hal_virtmem_pfn$size (
  VOID
  )
{
  return (UINTN) (_riscv64_pfncache_end - _riscv64_pfncache_start);
}

CONST VIRTUAL_ADDRESS
hal_virtmem_userbase (
  VOID
  )
{
  return pt_umap_minaddr ();
}

CONST UINTN
hal_virtmem_usersize (
  VOID
  )
{
  return pt_umap_maxaddr ();
}

CONST VIRTUAL_ADDRESS
hal_virtmem_userentry (
  VOID
  )
{
  return (CONST VIRTUAL_ADDRESS) bootinfo->uentry;
}

UINTN
hal_physmem_maxpfn (
  VOID
  )
{
  return (UINTN) bootinfo->maxpfn;
}

UINTN
hal_physmem_maxrampfn (
  VOID
  )
{
  return (UINTN) bootinfo->maxrampfn;
}

UINT32
hal_physmem_numregions (
  VOID
  )
{

  return (UINT32) bootinfo->numregions;
}

struct apxh_region *
hal_physmem_region (
  IN UINT32  i
  )
{
  struct apxh_region *ptr;

  if (i >= hal_physmem_numregions ())
    return NULL;

  ptr = (struct apxh_region *) &_memregs_start + i;
  assert (ptr < (struct apxh_region *) &_memregs_end);

  return ptr;
}

VOID *
hal_physmem_stree (
  OUT UINT32  *Order OPTIONAL
  )
{
  if (Order)
    *Order = gHalStreeOrder;
  return gHalStreePtr;
}

VIRTUAL_ADDRESS
hal_virtmem_kvabase (
  VOID
  )
{
  return (VIRTUAL_ADDRESS) _riscv64_kva_start;
}

CONST UINTN
hal_virtmem_kvasize (
  VOID
  )
{
  return (UINTN) (_riscv64_kva_end - _riscv64_kva_start);
}

VIRTUAL_ADDRESS
hal_virtmem_kmembase (
  VOID
  )
{
  return (VIRTUAL_ADDRESS) _riscv64_kmem_start;
}

CONST UINTN
hal_virtmem_kmemsize (
  VOID
  )
{
  return (UINTN) (_riscv64_kmem_end - _riscv64_kmem_start);
}

CONST struct apxh_platformdesc *
hal_pltinfo (
  VOID
  )
{
  return &pltdesc;
}

/**
  Initialize RISC-V hardware abstraction layer.

  Validates boot info, initializes framebuffer, physical memory tree,
  and platform descriptor.
**/
VOID
RiscvInitialize (
  VOID
  )
{
  UINTN StreeMemsize;
  struct apxh_stree *StreeHdr;

  if (bootinfo->magic != APXH_BOOTINFO_MAGIC)
    {
      /* Only way to let know that things are wrong. */
      hal_cpu_trap ();
    }

  fbdesc = bootinfo->fbdesc;
  fbdesc.addr = (UINT64) (UINTN) & _fbuf_start;
  gUseFb = framebuffer_init (&fbdesc);

  /* Check  APXH stree. */
  StreeHdr = (struct apxh_stree *) _stree_start;
  if (StreeHdr->magic != APXH_STREE_MAGIC)
    {
      EarlyPrint ("ERROR: Unrecognised stree magic!");
      hal_cpu_halt ();
    }
  if (StreeHdr->size != 8 * STREE_SIZE (StreeHdr->order))
    {
      EarlyPrint ("ERROR: stree size doesn't match!");
      hal_cpu_halt ();
    }
  StreeMemsize = (UINTN) ((VOID *) _stree_end - (VOID *) _stree_start);
  if (StreeHdr->size + StreeHdr->offset > StreeMemsize)
    {
      EarlyPrint ("ERROR: stree doesn't fit in allocated memory!");
      hal_cpu_halt ();
    }
  gHalStreeOrder = StreeHdr->order;
  gHalStreePtr = (UINT8 *) StreeHdr + StreeHdr->offset;

  pltdesc = bootinfo->pltdesc;

  EarlyPrint ("riscv64 HAL booting from APXH.\n");
}


INT32
hal_putchar (
  IN INT32  ch
  )
{
  ANX_CPU_SBI_PUTCHAR(ch);
  return ch;
}

VOID
hal_pcpu_init (
  VOID
  )
{
  /* TODO */
}

VOID
hal_pcpu_add (
  IN UINT32           pcpuid,
  IN struct hal_cpu   *HalData
  )
{
  extern INT32 _bsp_stacktop[];
  assert (pcpuid < HAL_MAXCPUS);

  HalData->kernsp = (UINTN) _bsp_stacktop;
  pcpu_haldata[pcpuid] = HalData;
}

VOID
hal_pcpu_enter (
  IN UINT32  pcpuid
  )
{
  assert (pcpuid < HAL_MAXCPUS);

  riscv_settp ((UINTN) pcpu_haldata[pcpuid]);

  /* CPU is up and running. Switch to full interrupt handler. */
  set_stvec_final ();

  /* Allow Software Interrupts to fire now. */
  riscv_sie_kernel ();
  riscv_sstatus_sti ();
}

PHYSICAL_ADDRESS
hal_pcpu_startaddr (
  IN UINT32  pcpuid
  )
{
  /* TODO */
  return PADDR_INVALID;
}

VOID
hal_cpu_setdata (
  IN VOID  *Data
  )
{
  ((struct hal_cpu *) __builtin_thread_pointer ())->data = Data;
}

VOID *
hal_cpu_getdata (
  VOID
  )
{
  return ((struct hal_cpu *) __builtin_thread_pointer ())->data;
}

VOID
hal_init_done (
  VOID
  )
{
  /* TODO or nothing to do */
}

__dead VOID
hal_panic (
  IN UINT32            cpu,
  IN CONST CHAR8       *Error,
  IN struct hal_frame  *Frame
  )
{
  if (gUseFb)
    {
      /*
         Reset frame buffer. This will unlock in case any CPU was
         holding the spinlock.
       */
      framebuffer_reset ();
    }

  printf ("\n"
	  "----------------------------------------"
	  "---------------------------------------\n"
	  "Fatal error on CPU%d: %s\n", cpu, Error);
  if (Frame != NULL)
    {
      hal_frame_print (Frame);
    }
  printf ("----------------------------------------"
	  "---------------------------------------\n");
  hal_cpu_halt ();
}

UINT32
hal_vect_max (
  VOID
  )
{
  /* This is not a vector-based platform. We simply send a zero vector
     on external interrupt, and let the platform get the IRQ. */
  return 0;
}

/**
  Handle page fault exception.

  @param[in] Frame  Exception frame.

  @return Modified frame pointer.
**/
struct hal_frame *
DoPageFault (
  IN struct hal_frame  *Frame
  )
{
  hal_l1p_t l1p;
  hal_pfinfo_t pfinfo = 0;

  /* In RISCV, we have to manually create the reasons for page fault. */

  if (hal_frame_isuser (Frame))
    pfinfo |= HAL_PF_INFO_USER;

  if (Frame->scause == SCAUSE_SPF)
    pfinfo |= HAL_PF_INFO_WRITE;

  if (Frame->scause == SCAUSE_IPF)
    pfinfo |= HAL_PF_INFO_EXE;

  l1p = cpumap_get_l1p (Frame->stval, FALSE);
  if (l1p == L1P_INVALID)
    pfinfo |= HAL_PF_REASON_NOTP;
  else
    {
      hal_l1e_t l1e;

      l1e = get_pte (l1p);
      if ((l1e & (PTE_V | PTE_R)) != (PTE_V | PTE_R))
	pfinfo |= HAL_PF_REASON_NOTP;
    }

  return hal_entry_pf (Frame, Frame->stval, pfinfo);
}

/**
  Main HAL entry point for exceptions and interrupts.

  @param[in] Frame  Exception/interrupt frame.

  @return Modified frame pointer.
**/
struct hal_frame *
_hal_entry (
  IN struct hal_frame  *Frame
  )
{
  struct hal_frame *r;

  /*
     We are here with interrupts off.

     First of all, if we are entering from a software interrupt, clear
     the pending flag so it won't be fired again when we re-enable
     interrupts.
     Note: SI is the only interrupt enabled in kernel, for NMI emulation.
   */
  if (Frame->scause == SCAUSE_SSI)
    riscv_sip_siclear ();

  /*
     Now enable kernel interrupts.
   */
  riscv_sie_kernel ();
  riscv_sstatus_sti ();

  if (Frame->scause & SCAUSE_INTR)
    r = plt_interrupt (Frame->scause & ~SCAUSE_INTR, Frame);
  else
    {
      switch (Frame->scause)
	{
	case SCAUSE_SYSC:
	  Frame->pc += 4;
	  r =
	    hal_entry_syscall (Frame, Frame->a0, Frame->a1, Frame->a2, Frame->a3, Frame->a4, Frame->a5,
			       Frame->a6);
	  break;

	case SCAUSE_IPF:
	case SCAUSE_LPF:
	case SCAUSE_SPF:
	  r = DoPageFault (Frame);
	  break;

	default:
	  r = hal_entry_xcpt (Frame, Frame->scause);
	  break;
	}
    }

  /*
     If we are returning to an user or idle frame, check if IPI is
     pending. If so, re-enter.
   */
  while ((r->sie == SIE_USER) && NmiEmulIpiPending ())
    {
      NmiEmulIpiClear ();
      r = hal_entry_ipi (r);
    }

  /* Return with interrupts off. */
  riscv_sstatus_cli ();
  if (hal_frame_isuser (r))
    {
      ANX_CPU_WRITE_SSCRATCH_TP();
      //ANX_CPU_BREAKPOINT();
    }
  return r;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use RiscvInitialize instead **/
VOID riscv_init (VOID) {
  RiscvInitialize ();
}

/** @deprecated Use DoPageFault instead **/
struct hal_frame *do_pagefault (struct hal_frame *f) {
  return DoPageFault (f);
}

/** @deprecated Use EarlyPrint instead **/
static VOID early_print (PCCHAR8s) {
  EarlyPrint (s);
}

// Legacy global variable alias
int nux_initialized = 0;
