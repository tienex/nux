/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _HAL_X86_CONFIG
#define _HAL_X86_CONFIG

#ifndef _ASSEMBLER

#include <stdint.h>

#define L1P_INVALID ((uintptr_t)0)

typedef uintptr_t hal_l1p_t;
typedef uint64_t hal_l1e_t;

static inline void
hal_debug (void)
{
  asm volatile ("int3\n\t");
}

#endif

#ifdef __i386__
#include "arch/i386/config.h"
#endif

#ifdef __amd64__
#include "arch/amd64/config.h"
#endif

#ifdef __riscv
#include "arch/riscv64/config.h"
#endif

#endif
