#include <cdefs.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <framebuffer.h>

#include <nux/hal.h>
#include <nux/apxh.h>
#include <nux/plt.h>
#include <nux/nmiemul.h>
#include "internal.h"
#include "stree.h"

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
int nux_initialized = 0;

struct hal_cpu *pcpu_haldata[HAL_MAXCPUS];

static void
early_print (const char *s)
{
  char *ptr = (char *) s;

  while (*ptr != '\0')
    hal_putchar (*ptr++);
}


/*
  I/O ops aren't implemented in RISC-V.
*/

unsigned long
hal_cpu_in (uint8_t size, uint32_t port)
{
  return 0;
}

void
hal_cpu_out (uint8_t size, uint32_t port, unsigned long val)
{
}

void
hal_cpu_relax (void)
{
  /* Should really use __builtin_riscv_pause() */
  asm volatile ("nop\n");
}

void
hal_cpu_trap (void)
{
  asm volatile ("ebreak;");
}

void
hal_cpu_idle (void)
{
  riscv_sie_user ();

  /* If IPI is pending, manually enable SI. */
  if (nmiemul_ipi_pending ())
    asm volatile ("csrsi sip, %0"::"K" (SIP_SSIP));

  while (1)
    {
      asm volatile ("csrsi sstatus, 0x2; wfi;");
    }
}

void
hal_cpu_halt (void)
{
  while (1)
    asm volatile ("csrci sstatus, 0x2; 1: j 1b;");

}

void
hal_cpu_tlbop (hal_tlbop_t tlbop)
{
  if (tlbop == HAL_TLBOP_NONE)
    return;

  asm volatile ("sfence.vma x0, x0":::"memory");
}

void
hal_useraccess_start (void)
{
  asm volatile ("csrs sstatus, %0"::"r" (SSTATUS_SUM):"memory");
}

void
hal_useraccess_end (void)
{
  asm volatile ("csrc sstatus, %0"::"r" (SSTATUS_SUM):"memory");
}

vaddr_t
hal_virtmem_dmapbase (void)
{
  return _riscv64_physmap_start;
}

const size_t
hal_virtmem_dmapsize (void)
{
  return (size_t) (_riscv64_physmap_end - _riscv64_physmap_start);
}

vaddr_t
hal_virtmem_pfn$base (void)
{
  return _riscv64_pfncache_start;
}

const size_t
hal_virtmem_pfn$size (void)
{
  return (size_t) (_riscv64_pfncache_end - _riscv64_pfncache_start);
}

const vaddr_t
hal_virtmem_userbase (void)
{
  return umap_minaddr ();
}

const size_t
hal_virtmem_usersize (void)
{
  return umap_maxaddr ();
}

const vaddr_t
hal_virtmem_userentry (void)
{
  return (const vaddr_t) bootinfo->uentry;
}

unsigned long
hal_physmem_maxpfn (void)
{
  return (unsigned long) bootinfo->maxpfn;
}

unsigned
hal_physmem_numregions (void)
{

  return (unsigned) bootinfo->numregions;
}

struct apxh_region *
hal_physmem_region (unsigned i)
{
  struct apxh_region *ptr;

  if (i >= hal_physmem_numregions ())
    return NULL;

  ptr = (struct apxh_region *) &_memregs_start + i;
  assert (ptr < (struct apxh_region *) &_memregs_end);

  return ptr;
}

void *
hal_physmem_stree (unsigned *order)
{
  if (order)
    *order = hal_stree_order;
  return hal_stree_ptr;
}

vaddr_t
hal_virtmem_kvabase (void)
{
  return (vaddr_t) _riscv64_kva_start;
}

const size_t
hal_virtmem_kvasize (void)
{
  return (size_t) (_riscv64_kva_end - _riscv64_kva_start);
}

vaddr_t
hal_virtmem_kmembase (void)
{
  return (vaddr_t) _riscv64_kmem_start;
}

const size_t
hal_virtmem_kmemsize (void)
{
  return (size_t) (_riscv64_kmem_end - _riscv64_kmem_start);
}

const struct apxh_pltdesc *
hal_pltinfo (void)
{
  return &pltdesc;
}

void
riscv_init (void)
{
  size_t stree_memsize;
  struct apxh_stree *stree_hdr;

  if (bootinfo->magic != APXH_BOOTINFO_MAGIC)
    {
      /* Only way to let know that things are wrong. */
      hal_cpu_trap ();
    }

  fbdesc = bootinfo->fbdesc;
  fbdesc.addr = (uint64_t) (uintptr_t) & _fbuf_start;
  use_fb = framebuffer_init (&fbdesc);

  /* Check  APXH stree. */
  stree_hdr = (struct apxh_stree *) _stree_start;
  if (stree_hdr->magic != APXH_STREE_MAGIC)
    {
      early_print ("ERROR: Unrecognised stree magic!");
      hal_cpu_halt ();
    }
  if (stree_hdr->size != 8 * STREE_SIZE (stree_hdr->order))
    {
      early_print ("ERROR: stree size doesn't match!");
      hal_cpu_halt ();
    }
  stree_memsize = (size_t) ((void *) _stree_end - (void *) _stree_start);
  if (stree_hdr->size + stree_hdr->offset > stree_memsize)
    {
      early_print ("ERROR: stree doesn't fit in allocated memory!");
      hal_cpu_halt ();
    }
  hal_stree_order = stree_hdr->order;
  hal_stree_ptr = (uint8_t *) stree_hdr + stree_hdr->offset;

  pltdesc = bootinfo->pltdesc;

  early_print ("riscv64 HAL booting from APXH.\n");
}


int
hal_putchar (int ch)
{
  asm volatile ("mv a0, %0\n" "li a7, 1\n" "ecall\n"::"r" (ch):"a0", "a7");
  return ch;
}

void
hal_pcpu_init (void)
{
  /* TODO */
}

void
hal_pcpu_add (unsigned pcpuid, struct hal_cpu *haldata)
{
  extern int stacktop[];
  assert (pcpuid < HAL_MAXCPUS);

  haldata->kernsp = (uintptr_t) stacktop;
  pcpu_haldata[pcpuid] = haldata;
}

void
hal_pcpu_enter (unsigned pcpuid)
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
hal_pcpu_startaddr (unsigned pcpuid)
{
  /* TODO */
  return PADDR_INVALID;
}

void
hal_cpu_setdata (void *data)
{
  ((struct hal_cpu *) __builtin_thread_pointer ())->data = data;
}

void *
hal_cpu_getdata (void)
{
  return ((struct hal_cpu *) __builtin_thread_pointer ())->data;
}

void
hal_init_done (void)
{
  /* TODO or nothing to do */
}

__dead void
hal_panic (unsigned cpu, const char *error, struct hal_frame *f)
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
	  "Fatal error on CPU%d: %s\n", cpu, error);
  if (f != NULL)
    {
      hal_frame_print (f);
    }
  printf ("----------------------------------------"
	  "---------------------------------------\n");
  hal_cpu_halt ();
}

unsigned
hal_vect_max (void)
{
  /* This is not a vector-based platform. We simply send a zero vector
     on external interrupt, and let the platform get the IRQ. */
  return 0;
}

struct hal_frame *
do_pagefault (struct hal_frame *f)
{
  hal_l1p_t l1p;
  hal_pfinfo_t pfinfo = 0;

  /* In RISCV, we have to manually create the reasons for page fault. */

  if (hal_frame_isuser (f))
    pfinfo |= HAL_PF_INFO_USER;

  if (f->scause == SCAUSE_SPF)
    pfinfo |= HAL_PF_INFO_WRITE;

  l1p = cpumap_get_l1p (f->stval, false);
  if (l1p == L1P_INVALID)
    pfinfo |= HAL_PF_REASON_NOTP;
  else
    {
      hal_l1e_t l1e;

      l1e = get_pte (l1p);
      if ((l1e & (PTE_V | PTE_R)) != (PTE_V | PTE_R))
	pfinfo |= HAL_PF_REASON_NOTP;
    }

  return hal_entry_pf (f, f->stval, pfinfo);
}

struct hal_frame *
_hal_entry (struct hal_frame *f)
{
  struct hal_frame *r;

  /*
     We are here with interrupts off.

     First of all, if we are entering from a software interrupt, clear
     the pending flag so it won't be fired again when we re-enable
     interrupts.
     Note: SI is the only interrupt enabled in kernel, for NMI emulation.
   */
  if (f->scause == SCAUSE_SSI)
    riscv_sip_siclear ();

  /*
     Now enable kernel interrupts.
   */
  riscv_sie_kernel ();
  riscv_sstatus_sti ();

  if (f->scause & SCAUSE_INTR)
    r = plt_interrupt (f->scause & ~SCAUSE_INTR, f);
  else
    {
      switch (f->scause)
	{
	case SCAUSE_SYSC:
	  f->pc += 4;
	  r = hal_entry_syscall (f, f->a0, f->a1, f->a2, f->a3, f->a4, f->a5);
	  break;

	case SCAUSE_IPF:
	case SCAUSE_LPF:
	case SCAUSE_SPF:
	  r = do_pagefault (f);
	  break;

	default:
	  r = hal_entry_xcpt (f, f->scause);
	  break;
	}
    }

  /*
     If we are returning to an user or idle frame, check if IPI is
     pending. If so, re-enter.
   */
  while ((r->sie == SIE_USER) && nmiemul_ipi_pending ())
    {
      nmiemul_ipi_clear ();
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
