#ifndef _HAL_INTERNAL_H
#define _HAL_INTERNAL_H

#include <nux/nux.h>
#include <nux/hal_config.h>


#include "riscv.h"

typedef uint64_t pte_t;
typedef uintptr_t ptep_t;

#define PTE_V (1 << 0)
#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_FLAGS (PTE_V | PTE_R | PTE_W | PTE_X)
#define PTE_U (1 << 4)
#define PTE_GLOBAL (1 << 5)
#define PTE_AVL0 (1 << 8)
#define PTE_AVL1 (1 << 9)


#define PTE_PFN_SHIFT 10

#define PTE_INVALID ((uint64_t)0)

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

hal_l1p_t kmap_get_l1p (unsigned long va, int alloc);
hal_l1p_t umap_get_l1p (struct hal_umap *umap, unsigned long va, bool alloc);
uaddr_t umap_next (struct hal_umap *umap, uaddr_t uaddr, hal_l1p_t * l1p_out,
		   hal_l1e_t * l1e_out);
void umap_free (struct hal_umap *umap);
unsigned long umap_minaddr (void);
unsigned long umap_maxaddr (void);


#endif /* _HAL_INTERNAL_H */
