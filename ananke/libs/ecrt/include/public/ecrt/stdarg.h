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

#ifndef __ecrt_stdarg_h__
#define __ecrt_stdarg_h__

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start((ap), (last))
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end

#endif /* eCRT_STDARG_H */
