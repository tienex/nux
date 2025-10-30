/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef __hal_config_h__
#define __hal_config_h__

#ifndef _ASSEMBLER

#include <stdint.h>

#define L1P_INVALID ((uintptr_t)0)

typedef uintptr_t hal_l1p_t;
typedef uint64_t hal_l1e_t;

static INLINE void
hal_debug (void)
{
  asm volatile ("int3\n\t");
}

#endif

#ifdef __i386__
#include <hal/arch/i386/config.h>
#endif

#ifdef __amd64__
#include <hal/arch/amd64/config.h>
#endif

#ifdef __riscv
#include <hal/arch/riscv64/config.h>
#endif

#endif
