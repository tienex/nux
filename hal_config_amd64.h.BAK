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

#define HAL_PAGED		/* This HAL uses paging. */

#define HAL_PAGE_SHIFT 12
#define HAL_MAXCPUS 256

#define HAL_KVA_SHIFT 39	/* 512Gb */
#define HAL_KVA_SIZE (1LL << HAL_KVA_SHIFT)

#define FRAMETYPE_INTR 0x0
#define FRAMETYPE_SYSC 0x1

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
  void *data;			/* Must be at %gs:0 */
  uint64_t kstack;		/* syscall kstack. Must be at %gs:8 */
  uint64_t scratch;		/* syscall scratch. Must be at %gs:16 */
  struct amd64_tss tss;
} __packed;

/*
  HAL Frame definition.
*/

struct hal_frame
{
  uint64_t type;
  union
  {
    struct amd64_intr_frame
    {
      uint64_t gsbase;

      uint64_t cr2;

      uint64_t rax;
      uint64_t rbx;
      uint64_t rcx;
      uint64_t rdx;
      uint64_t rbp;
      uint64_t rsi;
      uint64_t rdi;
      uint64_t r8;
      uint64_t r9;
      uint64_t r10;
      uint64_t r11;
      uint64_t r12;
      uint64_t r13;
      uint64_t r14;
      uint64_t r15;

      uint64_t vect;

      /* exception stack */
      uint64_t err;
      uint64_t rip;
      uint64_t cs;
      uint64_t rflags;
      uint64_t rsp;
      uint64_t ss;
    } intr;
  };
};

#endif

#endif
