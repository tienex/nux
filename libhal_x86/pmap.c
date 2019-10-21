#include <assert.h>
#include <stdbool.h>
#include <nux/hal.h>
#include "internal.h"

#define PTE_P       1
#define PTE_W       2
#define PTE_U       4
#define PTE_A       0x20
#define PTE_D       0x40
#define PTE_PS      0x80
#define PTE_G       0x100
#define PTE_AVAIL   0xe00
#define PTE_NX      0x8000000000000000LL

#define PTE_AVAIL0 (1 << 9)
#define PTE_AVAIL1 (2 << 9)
#define PTE_AVAIL2 (4 << 9)

#define L3_SHIFT (9 + 9 + PAGE_SHIFT)
#define L2_SHIFT (9 + PAGE_SHIFT)
#define L1_SHIFT PAGE_SHIFT

#define mkpte(_p, _f) ((uint64_t)((_p) << PAGE_SHIFT) | (_f))
#define pte_present(_pte) ((_pte) & PTE_P)

#define l2e_present(_l2e) pte_present((uint64_t)(_l2e))
#define l2e_leaf(_l2e) (((_l2e) & (PTE_PS|PTE_P)) == (PTE_PS|PTE_P))

extern int _linear_start;
extern int _linear_l2table;
const vaddr_t linaddr = (vaddr_t)&_linear_start;
const vaddr_t l2_linaddr = (vaddr_t)&_linear_l2table;
uint64_t pte_nx = 0;

typedef uint64_t l1e_t;
typedef uint64_t l2e_t;
typedef uint64_t l3e_t;

static l2e_t *
get_curl2p (vaddr_t va)
{
  l2e_t *ptr;

  va &= ~((1L << L2_SHIFT) - 1);
  printf("get_curl2p of %lx\n", va);
  ptr = (l3e_t *)(l2_linaddr + (va >> 18));
  printf("PTR = %lx\n", ptr);
  return ptr;
}

static l1e_t *
get_curl1p (vaddr_t va)
{
  l1e_t *ptr;

  va &= ~((1L << L1_SHIFT) - 1);

  ptr = (l1e_t *)(linaddr + (va >> 9));
  return ptr;
}

static void
set_pte (uint64_t *ptep, uint64_t pte)
{
 volatile uint32_t *ptr = (volatile uint32_t *) ptep;

  /* 32-bit specific.  This or atomic 64 bit write.  */
  *ptr = 0;
  *(ptr + 1) = pte >> 32;
  *ptr = pte & 0xffffffff;
}

static l1e_t *
get_l1p (void *pmap, unsigned long va)
{
  l2e_t *l2p, l2e;

  assert (pmap == NULL && "Cross-pmap not currently supported.");

  /* Assuming all L3 PTE are present. */

  l2p = get_curl2p (va);
  l2e = *l2p;
  if (!l2e_present (l2e))
    {
      /* Populate L1. */
      pfn_t pfn;
      uint64_t pte;

      pfn = hal_req_pfnalloc();
      if (pfn == PFN_INVALID)
	return NULL;

      /* Create an l2e with max permissions. L1 will filter. */
      pte = mkpte (pfn, PTE_U|PTE_P|PTE_W);
      printf("Setting pte %lx\n", pte);
      set_pte ((uint64_t *)l2p, pte);
      /* Not present, no TLB flush necessary. */

      l2e = (l2e_t)pte;
    }

  printf("l2e = %llx\n", l2e);

  assert (!l2e_leaf (l2e) && "Splintering big pages not supported.");

  return get_curl1p (va);
}

hal_l1e_t
hal_pmap_setl1e (struct hal_pmap *pmap, unsigned long va, hal_l1e_t l1e)
{
  l1e_t ol1e, *l1p;

  l1p = get_l1p (pmap, va);
  ol1e = *l1p;
  set_pte ((uint64_t *)l1p, (uint64_t)l1e);
  return ol1e;
}

hal_l1e_t
hal_pmap_boxl1e (unsigned long pfn, unsigned prot)
{
  hal_l1e_t l1e;

  l1e = (uint64_t) pfn << HAL_PAGE_SHIFT;

  if (prot & HAL_PFNPROT_PRESENT)
    l1e |= PTE_P;
  if (prot & HAL_PFNPROT_WRITE)
    l1e |= PTE_W;
  if (!(prot & HAL_PFNPROT_EXEC))
    l1e |= pte_nx;
  if (prot & HAL_PFNPROT_USER)
    l1e |= PTE_U;
  if (prot & HAL_PFNPROT_GLOBAL)
    l1e |= PTE_G;
  if (prot & HAL_PFNPROT_AVL0)
    l1e |= PTE_AVAIL0;
  if (prot & HAL_PFNPROT_AVL1)
    l1e |= PTE_AVAIL1;
  if (prot & HAL_PFNPROT_AVL2)
    l1e |= PTE_AVAIL2;

  return l1e;
}

void
hal_pmap_unboxl1e (hal_l1e_t l1e, unsigned long *pfnp, unsigned *protp)
{
  unsigned prot = 0;

  if (l1e & PTE_P)
    prot |= HAL_PFNPROT_PRESENT;
  if (l1e & PTE_W)
    prot |= HAL_PFNPROT_WRITE;
  if (!(l1e & PTE_NX))
    prot |= HAL_PFNPROT_EXEC;
  if (l1e & PTE_U)
    prot |= HAL_PFNPROT_USER;
  if (l1e & PTE_G)
    prot |= HAL_PFNPROT_GLOBAL;
  if (l1e & PTE_AVAIL0)
    prot |= HAL_PFNPROT_AVL0;
  if (l1e & PTE_AVAIL1)
    prot |= HAL_PFNPROT_AVL1;
  if (l1e & PTE_AVAIL2)
    prot |= HAL_PFNPROT_AVL2;

  *pfnp = l1e >> HAL_PAGE_SHIFT;
  *protp = prot;
}

static bool
cpu_supports_nx (void)
{
  uint64_t efer;

  efer = rdmsr (MSR_IA32_EFER);
  return !!(efer & _MSR_IA32_EFER_NXE);
}

void
pmap_init (void)
{
  if (cpu_supports_nx ())
      pte_nx = PTE_NX;
}
