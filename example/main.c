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

  uintptr_t x1, y1, x2, y2;

  x1 = kmem_alloc (0, 64);
  y1 = kmem_alloc (1, 5123);
  x2 = kmem_alloc (0, 64);
  y2 = kmem_alloc (1, 5123);

  kmem_free (1, y2, 5123);
  kmem_free (0, x2, 64);
  kmem_free (1, y1, 5123);
  kmem_free (0, x1, 64);

  kmem_trim ();
  printf("Done");
}

