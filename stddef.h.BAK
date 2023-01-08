/*
  EC - An embedded non standard C library

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef EC_STDDEF_H
#define EC_STDDEF_H

#define NULL ((void *)0)

#include <machine/ansi.h>

#define offsetof(type, member)	__builtin_offsetof(type, member)

#define container_of(ptr, type, member) ({			\
      const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
      (type *)( (char *)__mptr - offsetof(type,member));	\
    })

#endif /* EC_STDDEF_H */
