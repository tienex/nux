/*
  EC - An embedded non standard C library
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef EC_CDEFS_H
#define EC_CDEFS_H

#include <cdefs_elf.h>

#define __packed __attribute__((__packed__))
#define __aligned(x) __attribute__((__aligned__(x)))
#define __section(x) __attribute__((__section__(x)))

#define __dead __attribute__((__noreturn__))

#define __insn_barrier() __asm__ volatile("":::"memory")

#define __printflike(fmtarg, vararg)	\
  __attribute__((__format__(__printf__, fmtarg, vararg)))

#define __predict_false(_cond) __builtin_expect((_cond), 0)
#define __predict_true(_cond) __builtin_expect((_cond), 1)

#define NULL ((void *)0)

#define __UNCONST(c) ((void *)(unsigned long)(const void *)(c))

#define __BEGIN_DECLS
#define __END_DECLS

#define BUILD_ASSERT(_c) ((void)sizeof(char[1 - 2*!(_c)]))

#endif /* EC_CDEFS_H */
