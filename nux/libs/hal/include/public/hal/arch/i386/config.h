/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef __hal_arch_i386_config_h__
#define __hal_arch_i386_config_h__

#include <cdefs.h>

/*
  HAL Configuration.
*/

#define HAL_PAGED		/* This HAL uses paging. */

#define HAL_PAGE_SHIFT 12
#define HAL_MAXCPUS 64		/* Limited by CPUMAP implementation. */

/* KVA is (1 << HAL_KVA_SHIFT) size. */
#define HAL_KVA_SHIFT 28	/* 256Mb */
#define HAL_KVA_SIZE (1 << HAL_KVA_SHIFT)

#ifndef _ASSEMBLER

#include <stdint.h>

typedef UINT64 hal_l1e_t;


/*
  I386 UMAP.

  As we're using PAE, we save the lower L3s that make the 3Gb of user
  address space.
*/

#define UMAP_L3PTES 3

struct hal_umap
{
  UINT64 l3[UMAP_L3PTES];
};

#include <stdio.h>
static INLINE void
hal_umap_debug (struct hal_umap *umap)
{
  printf ("hal_umap: %p:", umap);
  for (int i = 0; i < UMAP_L3PTES; i++)
    printf ("  [%d] = %llx\n", i, umap->l3[i]);
}


/*
  HAL CPU definition.
*/

struct i386_tss
{
  UINT16 ptl, tmp0;
  UINT32 esp0;
  UINT16 ss0, tmp1;
  UINT32 esp1;
  UINT16 ss1, tmp2;
  UINT32 esp2;
  UINT16 ss2, tmp3;
  UINT32 cr3;
  UINT32 eip;
  UINT32 eflags;
  UINT32 eax;
  UINT32 ecx;
  UINT32 edx;
  UINT32 ebx;
  UINT32 esp;
  UINT32 ebp;
  UINT32 esi;
  UINT32 edi;
  UINT16 es, tmp4;
  UINT16 cs, tmp5;
  UINT16 ss, tmp6;
  UINT16 ds, tmp7;
  UINT16 fs, tmp8;
  UINT16 gs, tmp9;
  UINT16 ldt, tmpA;
  UINT16 t_flag, iomap;
} __packed;

struct hal_cpu
{
  void *data;
  struct i386_tss tss;
};


/*
  HAL Frame definition.
*/

struct hal_frame
{
  /* segments */
  UINT16 ds;
  UINT16 es;
  UINT16 fs;
  UINT16 gs;
  /* CRs    */
  UINT32 cr2;
  UINT32 cr3;
  /* pushal */
  UINT32 edi;
  UINT32 esi;
  UINT32 ebp;
  UINT32 espx;
  UINT32 ebx;
#define HAL_REG_A1 edx
  UINT32 edx;
#define HAL_REG_A2 ecx
  UINT32 ecx;
#define HAL_REG_A0 eax
  UINT32 eax;
  /* exception stack */
  UINT32 err;
  UINT32 eip;
  UINT32 cs;
  UINT32 eflags;
  UINT32 esp;
  UINT32 ss;
} __packed;


#endif



#endif /* _HAL_X86_CONFIG */
