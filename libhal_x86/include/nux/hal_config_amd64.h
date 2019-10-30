/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef _HAL_AMD64_CONFIG
#define _HAL_AMD64_CONFIG

#include <cdefs.h>

/*
  HAL Configuration.
*/

#define HAL_PAGED  /* This HAL uses paging. */

#define HAL_PAGE_SHIFT 12
#define HAL_MAXCPUS 64

#define HAL_KVA_SHIFT 39 /* 512Gb */
#define HAL_KVA_SIZE (1LL << HAL_KVA_SHIFT)

#ifndef _ASSEMBLER

#include <stdint.h>

typedef uint64_t hal_l1e_t;

struct amd64_tss
{
  uint32_t res0;
  uint64_t rsp0;
  uint64_t rsp1;
  uint64_t rsp2;
  uint64_t res1;
  uint64_t ist[7];
  uint64_t res2;
  uint16_t res3;
  uint16_t iomap;
} __packed;

struct hal_cpu
{
  void *data;
  struct amd64_tss tss;
};

/*
  HAL Frame definition.
*/

struct hal_frame
{
  /* exception stack */
  uint64_t err;
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
};

#endif

#endif
