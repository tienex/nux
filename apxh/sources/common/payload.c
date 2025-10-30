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
  @param[out] pSize  Pointer to receive payload size (optional).

  @return Pointer to payload data, or pointer to end marker if not found.
**/
VOID *
PayloadGet (
  IN UINT32          Index,
  OUT OPTIONAL size_t  *pSize
  )
{
  struct payload_hdr *pPtr = (struct payload_hdr *) &_end;
  UINT32 j;

  j = 0;
  while (pPtr->magic == ELFPAYLOAD_MAGIC)
    {
      if (j != Index)
	{
	  pPtr = (struct payload_hdr *) ((VOID *) (pPtr + 1) + pPtr->size);
	  j++;
	  continue;
	}

      if (pSize)
	*pSize = pPtr->size;
      return (VOID *) (pPtr + 1);
    }

  if (pSize)
    *pSize = 0;
  return pPtr;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use PayloadGet instead **/
void *payload_get (unsigned i, size_t *size) {
  return PayloadGet (i, size);
}
