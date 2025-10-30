/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef __hal_arch_amd64_config_h__
#define __hal_arch_amd64_config_h__

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
  AMD64 UMAP.

  We save and restore only a small number of the 256 L4 PTEs that make
  the user mapping of a 4-level AMD64 page table. This reduces the
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


/*
  HAL CPU definition.
*/

struct amd64_tss
{
  UINT32 res0;
  UINT64 rsp0;
  UINT64 rsp1;
  UINT64 rsp2;
  UINT64 res1;
  UINT64 ist[7];
  UINT64 res2;
  UINT16 res3;
  UINT16 iomap;
} __packed;

struct hal_cpu
{
  VOID *data;			/* Must be at %gs:0 */
  UINT64 kstack;		/* syscall kstack. Must be at %gs:8 */
  UINT64 scratch;		/* syscall scratch. Must be at %gs:16 */
  struct amd64_tss tss;
} __packed;

/*
  HAL Frame definition.
*/

struct hal_frame
{
  UINT64 type;
  union
  {
    struct amd64_intr_frame
    {
      UINT64 fsbase;
      UINT64 gsbase;

      UINT64 cr2;

      UINT64 rax;
      UINT64 rbx;
      UINT64 rcx;
      UINT64 rdx;
      UINT64 rbp;
      UINT64 rsi;
      UINT64 rdi;
      UINT64 r8;
      UINT64 r9;
      UINT64 r10;
      UINT64 r11;
      UINT64 r12;
      UINT64 r13;
      UINT64 r14;
      UINT64 r15;

      UINT64 vect;

      /* exception stack */
      UINT64 err;
      UINT64 rip;
      UINT64 cs;
      UINT64 rflags;
      UINT64 rsp;
      UINT64 ss;
    } intr;
  };
};

#endif

#endif
