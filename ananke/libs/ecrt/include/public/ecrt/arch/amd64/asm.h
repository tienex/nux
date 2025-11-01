/** @file
  eCRT - An embedded C runtime library

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

/*
  eCRT - An embedded non standard C library
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __ecrt_arch_amd64_asm_h__
#define __ecrt_arch_amd64_asm_h__

#define _ALIGN_TEXT     .align 16
#define _C_LABEL(x)	x
#define _C_LABEL_STRING(x) #x

//#ifdef _ASSEMBLER

#define _LABEL(x) .globl x; x:
#define _ENTRY(x)					\
    .text; _ALIGN_TEXT; .globl x; .type x,@function; x:

#define LABEL(y)        _LABEL(_C_LABEL(y))
#define ENTRY(y)        _ENTRY(_C_LABEL(y))
#define END(y)          .size y, . - y

#define WEAK_ALIAS(alias,sym)			\
  .weak alias;					\
  alias = sym

#define STRONG_ALIAS(alias,sym)			\
  .globl alias;					\
  alias = sym

//#endif /* _ASSEMBLER */

#endif /* AMD64_ASM_H */
