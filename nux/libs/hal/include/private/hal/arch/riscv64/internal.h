#ifndef __hal_arch_riscv64_internal_h__
#define __hal_arch_riscv64_internal_h__

#ifndef _ASSEMBLER
#include <nux/nux.h>
#include <hal/config.h>
#endif


#include <hal/arch/riscv64/riscv.h>

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
static INLINE unsigned long
riscv_sstatus_cli (VOID)
{
  return ANX_CPU_CSR_RCI_SSTATUS(SSTATUS_SIE);
}

static INLINE unsigned long
riscv_sstatus_sti (VOID)
{
  return ANX_CPU_CSR_RSI_SSTATUS(SSTATUS_SIE);
}

static INLINE unsigned long
riscv_sie_kernel (VOID)
{
  unsigned INTN old;
  ANX_CPU_CSR_RW_SIE(SIE_KERNEL, old);
  return old;
}

static INLINE unsigned long
riscv_sie_user (VOID)
{
  unsigned INTN old;
  ANX_CPU_CSR_RW_SIE(SIE_USER, old);
  return old;
}

static INLINE VOID
riscv_sip_siclear (VOID)
{
  ANX_CPU_CSR_CLEAR_IMM(sip, SIP_SSIP);
}
#endif


/*
  RiscV Paging.
*/

#ifndef _ASSEMBLER
typedef UINT64 pte_t;
typedef UINTN ptep_t;
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

#define PTE_INVALID ((UINT64)0)

#ifndef _ASSEMBLER
static INLINE ptep_t
mkptep (PFN pfn, UINT32 offset)
{
  return (pfn << PAGE_SHIFT) | (offset << 3);
}

static INLINE pte_t
mkpte (PFN pfn, UINT32 flags)
{
  return (pte_t) ((pfn << PTE_PFN_SHIFT) | flags);
}

static INLINE PFN
pte_pfn (pte_t pte)
{
  return (PFN) (pte >> PTE_PFN_SHIFT);
}

static INLINE BOOLEAN
pte_valid_table (pte_t pte)
{
  return ((pte & PTE_FLAGS) == PTE_V);
}

static INLINE BOOLEAN
pte_valid_leaf (pte_t pte)
{
  return ((pte & (PTE_V | PTE_R)) == (PTE_V | PTE_R));
}

static INLINE BOOLEAN
pte_valid (pte_t pte)
{
  return pte & PTE_V;
}

static INLINE pte_t
get_pte (ptep_t ptep)
{
  pte_t *t, pte;
  PFN pfn;
  UINT32 offset;

  pfn = ptep >> PAGE_SHIFT;
  offset = (ptep >> 3) & 0x1ff;
  t = (pte_t *) pfn_get (pfn);
  pte = t[offset];
  pfn_put (pfn, t);

  return pte;
}

static INLINE pte_t
set_pte (ptep_t ptep, pte_t pte)
{
  pte_t *t, old;
  PFN pfn;
  UINT32 offset;

  pfn = ptep >> PAGE_SHIFT;
  offset = (ptep >> 3) & 0x1ff;

  t = (pte_t *) pfn_get (pfn);
  old = t[offset];
  t[offset] = pte;
  pfn_put (pfn, t);

  return old;
}

static INLINE pte_t
alloc_table (VOID)
{
  PFN pfn;

  pfn = pfn_alloc (0);
  if (pfn == PFN_INVALID)
    return PTE_INVALID;

  return mkpte (pfn, PTE_V);
}

static INLINE unsigned long
riscv_satp (VOID)
{
  unsigned INTN satp;
  ANX_CPU_CSR_READ(satp, satp);
  return satp;
}

static INLINE VOID
riscv_invlpg (unsigned INTN va, BOOLEAN no_svvptc_only)
{
  ANX_CPU_INVLPG(va);
}

static INLINE VOID
riscv_settp (unsigned INTN data)
{
  ANX_CPU_SET_TP(data);
}

static INLINE unsigned long
riscv_gettp (VOID)
{
  return ANX_CPU_GET_TP();
}

hal_l1p_t cpumap_get_l1p (unsigned INTN va, INT32 alloc);
hal_l1p_t umap_get_l1p (struct hal_umap *umap, unsigned INTN va, BOOLEAN alloc);
USER_ADDRESS pt_umap_next (struct hal_umap *umap, USER_ADDRESS uaddr, hal_l1p_t * l1p_out,
		   hal_l1e_t * l1e_out);
VOID pt_umap_free (struct hal_umap *umap);
unsigned long pt_umap_minaddr (VOID);
unsigned long pt_umap_maxaddr (VOID);
#endif /* _ASSEMBLER */

#endif /* _HAL_INTERNAL_H */
