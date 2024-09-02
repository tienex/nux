#ifndef _HAL_RISCV_CONFIG
#define _HAL_RISCV_CONFIG

#ifndef _ASSEMBLER

#include <stdint.h>

#define L1P_INVALID ((uintptr_t)0)

typedef uintptr_t hal_l1p_t;
typedef uint64_t hal_l1e_t;

static inline void
hal_debug (void)
{
  asm volatile ("ebreak\n");
}

#endif

#include <cdefs.h>

/*
  HAL Configuration.
*/

#define HAL_PAGED		/* This HAL uses paging. */

#define HAL_PAGE_SHIFT 12
#define HAL_MAXCPUS 64		/* Limited by CPUMAP implementation. */

#define HAL_KVA_SHIFT 39	/* 512Gb */
#define HAL_KVA_SIZE (1LL << HAL_KVA_SHIFT)

#define FRAMETYPE_INTR 0x0
#define FRAMETYPE_SYSC 0x1

#ifndef _ASSEMBLER

#include <stdint.h>

/*
  RISCV64 UMAP

  We save and restore only a small number of the 256 L4 PTEs that make
  the user mapping of a 4-level RISCV64 page table. This reduces the
  virtual address space of the user process.

  Change UMAP_L4PTES t o increase or reduce the virtual address
  space. The trade-off is between virtual address size and number of
  PTEs to be saved/restored at each UMAP switch.
*/
#define UMAP_LOG2_L4PTES 3	/* 39 + log2(8) = 42bit User VA. */
#define UMAP_L4PTES (1 << UMAP_LOG2_L4PTES)

struct hal_umap
{
  uint64_t l4[UMAP_L4PTES];
};

#include <stdio.h>
static inline void
hal_umap_debug (struct hal_umap *umap)
{
  printf ("hal_umap %p:", umap);
  for (int i = 0; i < UMAP_L4PTES; i++)
    printf ("  [%d] = %lx\n", i, umap->l4[i]);
}

#endif

struct hal_cpu
{
  void *data;
} __packed;

#endif
