/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#include "internal.h"

struct ksym {
  unsigned long addr;
  const char *name;
};

extern struct ksym _ksym_start[];
extern struct ksym _ksym_end[];

const char *
nux_symresolve (unsigned long addr)
{
  const struct ksym *sym = _ksym_start;
  const struct ksym *last = NULL;

  while (sym->addr != 0)
    {
      if (sym->addr > addr)
	break;
      last = sym++;
    }

  if (sym->addr == 0)
    last = NULL;

  if (last == NULL)
    return "unknown";
  else
    return last->name;
}
