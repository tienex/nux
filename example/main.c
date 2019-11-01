/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  See COPYING file for the full license.

  SPDX-License-Identifier:	GPL2.0+
*/

#include <stdio.h>
#include <nux/nux.h>

int main ()
{
  printf ("Hello");

  kmem_trim_setmode (TRIM_BRK);

  for (int i = 0; i < 2; i++) {
  uintptr_t x1, y1, x2, y2;

  x1 = kmem_alloc (0, 61234);
  y1 = kmem_alloc (1, 5123);
  x2 = kmem_alloc (0, 61234);
  y2 = kmem_alloc (1, 5123);

  kmem_free (1, y2, 5123);
  kmem_free (0, x2, 61234);
  kmem_free (1, y1, 5123);
  kmem_free (0, x1, 61234);

  kmem_trim_one (TRIM_BRK);
  }


  printf("Done");
}

