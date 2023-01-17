/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef _HAL_I386_CONFIG
#define _HAL_I386_CONFIG

#include <cdefs.h>

/*
  HAL Configuration.
*/

#define HAL_PAGED		/* This HAL uses paging. */

#define HAL_PAGE_SHIFT 12
#define HAL_MAXCPUS 256

/* KVA is (1 << HAL_KVA_SHIFT) size. */
#define HAL_KVA_SHIFT 28	/* 256Mb */
#define HAL_KVA_SIZE (1 << HAL_KVA_SHIFT)

#ifndef _ASSEMBLER

#include <stdint.h>

/*
  HAL pmap Alignment.

  Since the top level PAE is quite small (4 64bit entries), it makes
  sense to include that in the hal_pmap structure, to avoid small
  allocations.

  The PDPTR in PAE needs to be aligned to PAE_PDPTR_ALIGN, and we
  can't control the alignment of the structure of libnux.

  Solve this by increasing the structure size allocated by libnux by
  PAE_PDPTR_ALIGN - 1, so that we can align it.

  The alignment of the structure is also saved in the (aligned)
  structure itself, so that hal_pmap_current() can return the
  unaligned struct back to libnux.
*/

#define PAE_PDPTR_ALIGN 32

typedef uint64_t hal_l1e_t;

struct align_pmap
{
#define NPDPTE 4
  uint64_t pdptr[NPDPTE];
#undef NPDPTE
  uint64_t *l1s;
  unsigned align;
};

struct hal_pmap
{
  uint8_t buf[sizeof (struct align_pmap) + PAE_PDPTR_ALIGN - 1];
};

/*
  HAL CPU definition.
*/

struct i386_tss
{
  uint16_t ptl, tmp0;
  uint32_t esp0;
  uint16_t ss0, tmp1;
  uint32_t esp1;
  uint16_t ss1, tmp2;
  uint32_t esp2;
  uint16_t ss2, tmp3;
  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;
  uint16_t es, tmp4;
  uint16_t cs, tmp5;
  uint16_t ss, tmp6;
  uint16_t ds, tmp7;
  uint16_t fs, tmp8;
  uint16_t gs, tmp9;
  uint16_t ldt, tmpA;
  uint16_t t_flag, iomap;
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
  uint16_t ds;
  uint16_t es;
  uint16_t fs;
  uint16_t gs;
  /* CRs    */
  uint32_t cr2;
  uint32_t cr3;
  /* pushal */
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t espx;
  uint32_t ebx;
#define HAL_REG_A1 edx
  uint32_t edx;
#define HAL_REG_A2 ecx
  uint32_t ecx;
#define HAL_REG_A0 eax
  uint32_t eax;
  /* exception stack */
  uint32_t err;
  uint32_t eip;
  uint32_t cs;
  uint32_t eflags;
  uint32_t esp;
  uint32_t ss;
} __packed;


#endif



#endif /* _HAL_X86_CONFIG */
