/** @file
  eCRT - An embedded C runtime library

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

/*
  eCRT - An embedded non standard C library

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __ecrt_stddef_h__
#define __ecrt_stddef_h__

#define NULL ((void *)0)

#include <machine/ansi.h>

#define offsetof(type, member)	__builtin_offsetof(type, member)

#define container_of(ptr, type, member) ({			\
      const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
      (type *)( (char *)__mptr - offsetof(type,member));	\
    })

#include <stdint.h>

typedef uint16_t wchar_t;

#endif /* eCRT_STDDEF_H */
