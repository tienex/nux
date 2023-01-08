/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  See COPYING file for the full license.

  SPDX-License-Identifier:	GPL2.0+
*/

#include <elfpayload.h>

extern int _end;

void *
payload_get (unsigned i, size_t *size)
{
  struct payload_hdr *ptr = (struct payload_hdr *) &_end;
  unsigned j;

  j = 0;
  while (ptr->magic == ELFPAYLOAD_MAGIC)
    {
      if (j != i)
	{
	  ptr = (struct payload_hdr *) ((void *) (ptr + 1) + ptr->size);
	  j++;
	  continue;
	}

      if (size)
	*size = ptr->size;
      return (void *) (ptr + 1);
    }

  if (size)
    *size = 0;
  return ptr;
}
