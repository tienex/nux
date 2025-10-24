#ifndef _HAL_INTERNAL_H
#define _HAL_INTERNAL_H

#ifndef _ASSEMBLER
#include <nux/nux.h>
#include <nux/hal_config.h>
#endif


#include "riscv.h"

/*
  HAL interrupt levels.

  There are two kind of interrupts in a NUX hal: a NMI (that
  interrupts while in kernel mode) and normal interrupts (including
  IPIs), that are received when the CPU is idle or in user space.

  NMIs in Risc-V are essentially messengers of death, so can't be used
  as wanted in NUX. We have a single Software Interrupt, and we use
  NUX's nmiemul to emulate both NUX NMI and NUX IPIs with it.

  This requires though to enable interrupt in kernel, to receive
  software interrupts that might or might not be NMIs.

  We define three software interrupts level:

  1. CRITICAL: SSTATUS SIE is off and not even NMIs can be
     recevied. This is how we start and can be used in parts of the
     interrupt handlers that are critical.

  2. KENEL: SSTATUS SIE is ON and SIP SSIE is on, so only
     Software Interrupt can be received.

  3. USER: SSTATUS SIE is ON and SIP is fully on. This is the
     status of interrupts in idle or in user space.
*/

#define SIE_KERNEL (SIE_SSIE)
#define SIE_USER   (SIE_SSIE|SIE_STIE|SIE_SEIE)

#ifndef _ASSEMBLER
static inline unsigned long
riscv_sstatus_cli (void)
{
  unsigned long old;
  asm volatile ("csrrci %0, sstatus, %1\n":"=r" (old):"K" (SSTATUS_SIE));
  return old;
}

static inline unsigned long
riscv_sstatus_sti (void)
{
  unsigned long old;
  asm volatile ("csrrsi %0, sstatus, %1\n":"=r" (old):"K" (SSTATUS_SIE));
  return old;
}

static inline unsigned long
riscv_sie_kernel (void)
{
  unsigned long old;
  asm volatile ("csrrw %0, sie, %1\n":"=r" (old):"r" (SIE_KERNEL));
  return old;
}

static inline unsigned long
riscv_sie_user (void)
{
  unsigned long old;
  asm volatile ("csrrw %0, sie, %1\n":"=r" (old):"r" (SIE_USER));
  return old;
}

static inline void
riscv_sip_siclear (void)
{
  asm volatile ("csrci sip, %0\n"::"K" (SIP_SSIP));
}
#endif


/*
  RiscV Paging.
*/

#ifndef _ASSEMBLER
typedef uint64_t pte_t;
typedef uintptr_t ptep_t;
#endif

#define PTE_V (1 << 0)
#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_FLAGS (PTE_V | PTE_R | PTE_W | PTE_X)
#define PTE_U (1 << 4)
#define PTE_GLOBAL (1 << 5)
#define PTE_A (1 << 6)
#define PTE_D (1 << 7)
#define PTE_AVL0 (1 << 8)
#define PTE_AVL1 (1 << 9)


#define PTE_PFN_SHIFT 10

#define PTE_INVALID ((uint64_t)0)

#ifndef _ASSEMBLER
static inline ptep_t
mkptep (pfn_t pfn, unsigned offset)
{
  return (pfn << PAGE_SHIFT) | (offset << 3);
}

static inline pte_t
mkpte (pfn_t pfn, unsigned flags)
{
  return (pte_t) ((pfn << PTE_PFN_SHIFT) | flags);
}

static inline pfn_t
pte_pfn (pte_t pte)
{
  return (pfn_t) (pte >> PTE_PFN_SHIFT);
}

static inline bool
pte_valid_table (pte_t pte)
{
  return ((pte & PTE_FLAGS) == PTE_V);
}

static inline bool
pte_valid_leaf (pte_t pte)
{
  return ((pte & (PTE_V | PTE_R)) == (PTE_V | PTE_R));
}

static inline bool
pte_valid (pte_t pte)
{
  return pte & PTE_V;
}

static inline pte_t
get_pte (ptep_t ptep)
{
  pte_t *t, pte;
  pfn_t pfn;
  unsigned offset;

  pfn = ptep >> PAGE_SHIFT;
  offset = (ptep >> 3) & 0x1ff;
  t = (pte_t *) pfn_get (pfn);
  pte = t[offset];
  pfn_put (pfn, t);

  return pte;
}

static inline pte_t
set_pte (ptep_t ptep, pte_t pte)
{
  pte_t *t, old;
  pfn_t pfn;
  unsigned offset;

  pfn = ptep >> PAGE_SHIFT;
  offset = (ptep >> 3) & 0x1ff;

  t = (pte_t *) pfn_get (pfn);
  old = t[offset];
  t[offset] = pte;
  pfn_put (pfn, t);

  return old;
}

static inline pte_t
alloc_table (void)
{
  pfn_t pfn;

  pfn = pfn_alloc (0);
  if (pfn == PFN_INVALID)
    return PTE_INVALID;

  return mkpte (pfn, PTE_V);
}

static inline unsigned long
riscv_satp (void)
{
  unsigned long satp;

  asm volatile ("csrr %0, satp\n":"=r" (satp));
  return satp;
}

static inline void
riscv_invlpg (unsigned long va, bool no_svvptc_only)
{
  asm volatile ("sfence.vma x0, %0\n"::"r" (va));
}

static inline void
riscv_settp (unsigned long data)
{
  asm volatile ("mv tp, %0\n"::"r" (data));
}

static inline unsigned long
riscv_gettp (void)
{
  unsigned long data;
  asm volatile ("mv %0, tp\n":"=r" (data));
  return data;
}

hal_l1p_t cpumap_get_l1p (unsigned long va, int alloc);
hal_l1p_t umap_get_l1p (struct hal_umap *umap, unsigned long va, bool alloc);
uaddr_t pt_umap_next (struct hal_umap *umap, uaddr_t uaddr, hal_l1p_t * l1p_out,
		   hal_l1e_t * l1e_out);
void pt_umap_free (struct hal_umap *umap);
unsigned long pt_umap_minaddr (void);
unsigned long pt_umap_maxaddr (void);
#endif /* _ASSEMBLER */

#endif /* _HAL_INTERNAL_H */
