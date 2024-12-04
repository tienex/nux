/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier:	BSD-2-Clause
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
