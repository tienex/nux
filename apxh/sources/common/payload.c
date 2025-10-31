/** @file
  APXH Payload Access

  Provides access to embedded ELF payloads appended to the bootloader binary.
  Payloads are stored after the _end symbol with headers containing magic
  numbers and size information.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <elfpayload.h>

extern INT32 _end;

/**
  Get embedded payload.

  Retrieves the i-th embedded payload from the bootloader binary.
  Payloads are stored sequentially after the _end symbol.

  @param[in]  Index  Payload index (0-based).
  @param[out] Size  Pointer to receive payload size (optional).

  @return Pointer to payload data, or pointer to end marker if not found.
**/
VOID *
PayloadGet (
  IN UINT32          Index,
  OUT OPTIONAL UINTN  *Size
  )
{
  PAYLOAD_HDR *Ptr = (PAYLOAD_HDR *) &_end;
  UINT32 j;

  j = 0;
  while (Ptr->Magic == ELFPAYLOAD_MAGIC)
    {
      if (j != Index)
	{
	  Ptr = (PAYLOAD_HDR *) ((VOID *) (Ptr + 1) + Ptr->Size);
	  j++;
	  continue;
	}

      if (Size)
	*Size = Ptr->Size;
      return (VOID *) (Ptr + 1);
    }

  if (Size)
    *Size = 0;
  return Ptr;
}
