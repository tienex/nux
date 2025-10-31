#ifndef __hal_arch_riscv64_config_h__
#define __hal_arch_riscv64_config_h__

#ifndef _ASSEMBLER

#include <stdint.h>

#define L1P_INVALID ((uintptr_t)0)

typedef uintptr_t hal_l1p_t;
typedef UINT64 hal_l1e_t;

static INLINE VOID
hal_debug (VOID)
{
  asm volatile ("ebreak\n");
}

#endif

#include <cdefs.h>

/*
  HAL Configuration.
*/

#define HAL_PAGED		/* This HAL uses paging. */
#define HAL_NMIEMUL		/* This HAL requires NMI emulation. */

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
  UINT64 l4[UMAP_L4PTES];
};

#include <stdio.h>
static INLINE VOID
hal_umap_debug (struct hal_umap *umap)
{
  printf ("hal_umap %p:", umap);
  for (int i = 0; i < UMAP_L4PTES; i++)
    printf ("  [%d] = %lx\n", i, umap->l4[i]);
}

struct hal_cpu
{
  /* Entry handler. */
  unsigned INTN intrsp;
  unsigned INTN kernsp;
  /* NUX per-cpu data. */
  VOID *data;
};

struct hal_frame
{
  unsigned INTN sstatus;
  unsigned INTN sie;
  unsigned INTN stval;
  unsigned INTN scause;
  unsigned INTN pc;
  unsigned INTN t6;
  unsigned INTN t5;
  unsigned INTN t4;
  unsigned INTN t3;
  unsigned INTN s11;
  unsigned INTN s10;
  unsigned INTN s9;
  unsigned INTN s8;
  unsigned INTN s7;
  unsigned INTN s6;
  unsigned INTN s5;
  unsigned INTN s4;
  unsigned INTN s3;
  unsigned INTN s2;
  unsigned INTN a7;
  unsigned INTN a6;
  unsigned INTN a5;
  unsigned INTN a4;
  unsigned INTN a3;
  unsigned INTN a2;
  unsigned INTN a1;
  unsigned INTN a0;
  unsigned INTN s1;
  unsigned INTN fp;
  unsigned INTN t2;
  unsigned INTN t1;
  unsigned INTN t0;
  unsigned INTN tp;
  unsigned INTN gp;
  unsigned INTN sp;
  unsigned INTN ra;
} ANX_PACKED;


#endif
#endif
