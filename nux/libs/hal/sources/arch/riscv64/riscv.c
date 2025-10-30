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
#include <nux/stree.h>

extern int _info_start;

extern int _stree_start[];
extern int _stree_end[];

extern int _fbuf_start;
extern int _fbuf_end;

extern int _memregs_start;
extern int _memregs_end;

extern uint64_t _riscv64_physmap_start;
extern uint64_t _riscv64_physmap_end;

extern uint64_t _riscv64_pfncache_start;
extern uint64_t _riscv64_pfncache_end;

extern uint64_t _riscv64_kva_start;
extern uint64_t _riscv64_kva_end;

extern uint64_t _riscv64_kmem_start;
extern uint64_t _riscv64_kmem_end;

void set_stvec_final ();

const struct apxh_bootinfo *bootinfo = (struct apxh_bootinfo *) &_info_start;

struct fbdesc fbdesc;
struct apxh_pltdesc pltdesc;

void *hal_stree_ptr;
unsigned hal_stree_order;

int use_fb;
INT32 gNuxInitialized = 0;

struct hal_cpu *pcpu_haldata[HAL_MAXCPUS];

/**
  Print a string during early boot.

  @param[in] pStr  String to print.
**/
static VOID
EarlyPrint (
  IN CONST CHAR8  *pStr
  )
{
  CHAR8 *ptr = (CHAR8 *) pStr;

  while (*ptr != '\0')
    hal_putchar (*ptr++);
}


/*
  I/O ops aren't implemented in RISC-V.
*/

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
  /* Should really use __builtin_riscv_pause() */
  asm volatile ("nop\n");
}

VOID
hal_cpu_trap (
  VOID
  )
{
  asm volatile ("ebreak;");
}

VOID
hal_cpu_idle (
  VOID
  )
{
  riscv_sie_user ();

  /* If IPI is pending, manually enable SI. */
  if (NmiEmulIpiPending ())
    asm volatile ("csrsi sip, %0"::"K" (SIP_SSIP));

  while (1)
    {
      asm volatile ("csrsi sstatus, 0x2; wfi;");
    }
}

VOID
hal_cpu_halt (
  VOID
  )
{
  while (1)
    asm volatile ("csrci sstatus, 0x2; 1: j 1b;");

}

UINT64
hal_cpu_cycles (
  VOID
  )
{
  UINT64 cycles;
  asm volatile ("rdcycle %0;" : "=r" (cycles));
  return cycles;
}

VOID
hal_cpu_tlbop (
  IN hal_tlbop_t  tlbop
  )
{
  if (tlbop == HAL_TLBOP_NONE)
    return;

  asm volatile ("sfence.vma x0, x0":::"memory");
}

VOID
hal_useraccess_start (
  VOID
  )
{
  asm volatile ("csrs sstatus, %0"::"r" (SSTATUS_SUM):"memory");
}

VOID
hal_useraccess_end (
  VOID
  )
{
  asm volatile ("csrc sstatus, %0"::"r" (SSTATUS_SUM):"memory");
}

vaddr_t
hal_virtmem_dmapbase (
  VOID
  )
{
  return _riscv64_physmap_start;
}

const size_t
hal_virtmem_dmapsize (
  VOID
  )
{
  return (size_t) (_riscv64_physmap_end - _riscv64_physmap_start);
}

vaddr_t
hal_virtmem_pfn$base (
  VOID
  )
{
  return _riscv64_pfncache_start;
}

const size_t
hal_virtmem_pfn$size (
  VOID
  )
{
  return (size_t) (_riscv64_pfncache_end - _riscv64_pfncache_start);
}

const vaddr_t
hal_virtmem_userbase (
  VOID
  )
{
  return pt_umap_minaddr ();
}

const size_t
hal_virtmem_usersize (
  VOID
  )
{
  return pt_umap_maxaddr ();
}

const vaddr_t
hal_virtmem_userentry (
  VOID
  )
{
  return (const vaddr_t) bootinfo->uentry;
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
  OUT UINT32  *pOrder OPTIONAL
  )
{
  if (pOrder)
    *pOrder = hal_stree_order;
  return hal_stree_ptr;
}

vaddr_t
hal_virtmem_kvabase (
  VOID
  )
{
  return (vaddr_t) _riscv64_kva_start;
}

const size_t
hal_virtmem_kvasize (
  VOID
  )
{
  return (size_t) (_riscv64_kva_end - _riscv64_kva_start);
}

vaddr_t
hal_virtmem_kmembase (
  VOID
  )
{
  return (vaddr_t) _riscv64_kmem_start;
}

const size_t
hal_virtmem_kmemsize (
  VOID
  )
{
  return (size_t) (_riscv64_kmem_end - _riscv64_kmem_start);
}

const struct apxh_pltdesc *
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
  size_t stree_memsize;
  struct apxh_stree *stree_hdr;

  if (bootinfo->magic != APXH_BOOTINFO_MAGIC)
    {
      /* Only way to let know that things are wrong. */
      hal_cpu_trap ();
    }

  fbdesc = bootinfo->fbdesc;
  fbdesc.addr = (UINT64) (uintptr_t) & _fbuf_start;
  use_fb = framebuffer_init (&fbdesc);

  /* Check  APXH stree. */
  stree_hdr = (struct apxh_stree *) _stree_start;
  if (stree_hdr->magic != APXH_STREE_MAGIC)
    {
      EarlyPrint ("ERROR: Unrecognised stree magic!");
      hal_cpu_halt ();
    }
  if (stree_hdr->size != 8 * STREE_SIZE (stree_hdr->order))
    {
      EarlyPrint ("ERROR: stree size doesn't match!");
      hal_cpu_halt ();
    }
  stree_memsize = (size_t) ((void *) _stree_end - (void *) _stree_start);
  if (stree_hdr->size + stree_hdr->offset > stree_memsize)
    {
      EarlyPrint ("ERROR: stree doesn't fit in allocated memory!");
      hal_cpu_halt ();
    }
  hal_stree_order = stree_hdr->order;
  hal_stree_ptr = (UINT8 *) stree_hdr + stree_hdr->offset;

  pltdesc = bootinfo->pltdesc;

  EarlyPrint ("riscv64 HAL booting from APXH.\n");
}


INT32
hal_putchar (
  IN INT32  ch
  )
{
  asm volatile ("mv a0, %0\n" "li a7, 1\n" "ecall\n"::"r" (ch):"a0", "a7");
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
  IN struct hal_cpu   *pHalData
  )
{
  extern int _bsp_stacktop[];
  assert (pcpuid < HAL_MAXCPUS);

  pHalData->kernsp = (uintptr_t) _bsp_stacktop;
  pcpu_haldata[pcpuid] = pHalData;
}

VOID
hal_pcpu_enter (
  IN UINT32  pcpuid
  )
{
  assert (pcpuid < HAL_MAXCPUS);

  riscv_settp ((uintptr_t) pcpu_haldata[pcpuid]);

  /* CPU is up and running. Switch to full interrupt handler. */
  set_stvec_final ();

  /* Allow Software Interrupts to fire now. */
  riscv_sie_kernel ();
  riscv_sstatus_sti ();
}

paddr_t
hal_pcpu_startaddr (
  IN UINT32  pcpuid
  )
{
  /* TODO */
  return PADDR_INVALID;
}

VOID
hal_cpu_setdata (
  IN VOID  *pData
  )
{
  ((struct hal_cpu *) __builtin_thread_pointer ())->data = pData;
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
  IN CONST CHAR8       *pError,
  IN struct hal_frame  *pFrame
  )
{
  if (use_fb)
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
	  "Fatal error on CPU%d: %s\n", cpu, pError);
  if (pFrame != NULL)
    {
      hal_frame_print (pFrame);
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

  @param[in] pFrame  Exception frame.

  @return Modified frame pointer.
**/
struct hal_frame *
DoPageFault (
  IN struct hal_frame  *pFrame
  )
{
  hal_l1p_t l1p;
  hal_pfinfo_t pfinfo = 0;

  /* In RISCV, we have to manually create the reasons for page fault. */

  if (hal_frame_isuser (pFrame))
    pfinfo |= HAL_PF_INFO_USER;

  if (pFrame->scause == SCAUSE_SPF)
    pfinfo |= HAL_PF_INFO_WRITE;

  if (pFrame->scause == SCAUSE_IPF)
    pfinfo |= HAL_PF_INFO_EXE;

  l1p = cpumap_get_l1p (pFrame->stval, false);
  if (l1p == L1P_INVALID)
    pfinfo |= HAL_PF_REASON_NOTP;
  else
    {
      hal_l1e_t l1e;

      l1e = get_pte (l1p);
      if ((l1e & (PTE_V | PTE_R)) != (PTE_V | PTE_R))
	pfinfo |= HAL_PF_REASON_NOTP;
    }

  return hal_entry_pf (pFrame, pFrame->stval, pfinfo);
}

/**
  Main HAL entry point for exceptions and interrupts.

  @param[in] pFrame  Exception/interrupt frame.

  @return Modified frame pointer.
**/
struct hal_frame *
_hal_entry (
  IN struct hal_frame  *pFrame
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
  if (pFrame->scause == SCAUSE_SSI)
    riscv_sip_siclear ();

  /*
     Now enable kernel interrupts.
   */
  riscv_sie_kernel ();
  riscv_sstatus_sti ();

  if (pFrame->scause & SCAUSE_INTR)
    r = plt_interrupt (pFrame->scause & ~SCAUSE_INTR, pFrame);
  else
    {
      switch (pFrame->scause)
	{
	case SCAUSE_SYSC:
	  pFrame->pc += 4;
	  r =
	    hal_entry_syscall (pFrame, pFrame->a0, pFrame->a1, pFrame->a2, pFrame->a3, pFrame->a4, pFrame->a5,
			       pFrame->a6);
	  break;

	case SCAUSE_IPF:
	case SCAUSE_LPF:
	case SCAUSE_SPF:
	  r = DoPageFault (pFrame);
	  break;

	default:
	  r = hal_entry_xcpt (pFrame, pFrame->scause);
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
      asm volatile ("csrw sscratch, tp\n");
      //asm volatile  ("ebreak\n");
    }
  return r;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use RiscvInitialize instead **/
void riscv_init (void) {
  RiscvInitialize ();
}

/** @deprecated Use DoPageFault instead **/
struct hal_frame *do_pagefault (struct hal_frame *f) {
  return DoPageFault (f);
}

/** @deprecated Use EarlyPrint instead **/
static void early_print (const char *s) {
  EarlyPrint (s);
}

// Legacy global variable alias
int nux_initialized = 0;
