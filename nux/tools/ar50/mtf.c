/** @file
  Move-To-Front (MTF) Encoding Implementation

  Implements the Move-To-Front transform which maintains a list of symbols
  and encodes each symbol as its position in the list, then moves it to
  the front. This exploits locality of reference.

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "types.h"
#include <string.h>
#include "mtf.h"

/**
  Apply Move-To-Front encoding.

  @param[in]  pInput      Input data buffer.
  @param[in]  InputSize   Size of input data.
  @param[out] pOutput     Output buffer for MTF encoded data.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
MTFEncode (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput
  )
{
  UINT8 List[256];
  size_t I;
  UINT32 J;

  if (pInput == NULL || pOutput == NULL)
    return FALSE;

  // Initialize symbol list (0-255)
  for (I = 0; I < 256; I++)
    List[I] = (UINT8)I;

  // Encode each byte
  for (I = 0; I < InputSize; I++)
    {
      UINT8 Symbol = pInput[I];

      // Find symbol in list
      for (J = 0; J < 256; J++)
        {
          if (List[J] == Symbol)
            {
              pOutput[I] = (UINT8)J;

              // Move symbol to front
              if (J > 0)
                {
                  UINT8 Temp = List[J];
                  memmove (&List[1], &List[0], J);
                  List[0] = Temp;
                }

              break;
            }
        }
    }

  return TRUE;
}

/**
  Apply inverse Move-To-Front decoding.

  @param[in]  pInput      MTF encoded data.
  @param[in]  InputSize   Size of encoded data.
  @param[out] pOutput     Output buffer for decoded data.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
MTFDecode (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput
  )
{
  UINT8 List[256];
  size_t I;
  UINT8 Index;

  if (pInput == NULL || pOutput == NULL)
    return FALSE;

  // Initialize symbol list (0-255)
  for (I = 0; I < 256; I++)
    List[I] = (UINT8)I;

  // Decode each byte
  for (I = 0; I < InputSize; I++)
    {
      Index = pInput[I];

      if (Index >= 256)
        return FALSE;

      // Output symbol at position Index
      pOutput[I] = List[Index];

      // Move symbol to front
      if (Index > 0)
        {
          UINT8 Temp = List[Index];
          memmove (&List[1], &List[0], Index);
          List[0] = Temp;
        }
    }

  return TRUE;
}
