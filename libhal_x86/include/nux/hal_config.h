/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef _HAL_X86_CONFIG
#define _HAL_X86_CONFIG

#ifndef _ASSEMBLER

#include <stdint.h>

#define L1P_INVALID ((uintptr_t)NULL)

typedef uintptr_t hal_l1p_t;
typedef uint64_t hal_l1e_t;

#endif

#ifdef __i386__
#include "hal_config_i386.h"
#endif

#ifdef __amd64__
#include "hal_config_amd64.h"
#endif

#endif
