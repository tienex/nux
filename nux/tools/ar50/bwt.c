/** @file
  Burrows-Wheeler Transform Implementation

  Implements the Burrows-Wheeler Transform using a sorting-based approach.
  The transform creates all rotations of the input, sorts them, and outputs
  the last column of the sorted matrix.

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "types.h"
#include <string.h>
#include <stdlib.h>
#include "bwt.h"

/**
  Comparison function for sorting rotations.
**/
typedef struct {
  const UINT8 *pData;
  size_t Size;
  UINT32 Index;
} BWT_ROTATION;

/**
  Compare two rotations for qsort.
**/
static int
CompareRotations (
  const void  *pA,
  const void  *pB
  )
{
  const BWT_ROTATION *pRotA = (const BWT_ROTATION *)pA;
  const BWT_ROTATION *pRotB = (const BWT_ROTATION *)pB;
  size_t I;

  for (I = 0; I < pRotA->Size; I++)
    {
      UINT8 CharA = pRotA->pData[(pRotA->Index + I) % pRotA->Size];
      UINT8 CharB = pRotB->pData[(pRotB->Index + I) % pRotB->Size];

      if (CharA < CharB)
        return -1;
      if (CharA > CharB)
        return 1;
    }

  return 0;
}

/**
  Apply Burrows-Wheeler Transform to data.

  @param[in]  pInput      Input data buffer.
  @param[in]  InputSize   Size of input data.
  @param[out] pOutput     Output buffer for transformed data.
  @param[out] pOrigIndex  Original rotation index.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
BWTTransform (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  OUT UINT32       *pOrigIndex
  )
{
  BWT_ROTATION *pRotations;
  size_t I;

  if (pInput == NULL || pOutput == NULL || pOrigIndex == NULL)
    return FALSE;

  if (InputSize == 0 || InputSize > 0x10000)  // Limit to 64KB for practicality
    return FALSE;

  // Create rotation table
  pRotations = (BWT_ROTATION *) malloc (InputSize * sizeof (BWT_ROTATION));
  if (pRotations == NULL)
    return FALSE;

  for (I = 0; I < InputSize; I++)
    {
      pRotations[I].pData = pInput;
      pRotations[I].Size = InputSize;
      pRotations[I].Index = I;
    }

  // Sort rotations
  qsort (pRotations, InputSize, sizeof (BWT_ROTATION), CompareRotations);

  // Find original index and output last column
  for (I = 0; I < InputSize; I++)
    {
      if (pRotations[I].Index == 0)
        *pOrigIndex = I;

      // Last character of each rotation
      pOutput[I] = pInput[(pRotations[I].Index + InputSize - 1) % InputSize];
    }

  free (pRotations);
  return TRUE;
}

/**
  Apply inverse Burrows-Wheeler Transform.

  @param[in]  pInput      BWT transformed data.
  @param[in]  InputSize   Size of transformed data.
  @param[in]  OrigIndex   Original rotation index.
  @param[out] pOutput     Output buffer for original data.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
BWTInverse (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  IN  UINT32       OrigIndex,
  OUT UINT8        *pOutput
  )
{
  UINT32 *pNext;
  UINT32 Count[256];
  UINT32 Sum[256];
  size_t I;
  UINT32 Idx;

  if (pInput == NULL || pOutput == NULL)
    return FALSE;

  if (InputSize == 0 || OrigIndex >= InputSize)
    return FALSE;

  // Allocate next array
  pNext = (UINT32 *) malloc (InputSize * sizeof (UINT32));
  if (pNext == NULL)
    return FALSE;

  // Count character frequencies
  memset (Count, 0, sizeof (Count));
  for (I = 0; I < InputSize; I++)
    Count[pInput[I]]++;

  // Calculate cumulative sums
  Sum[0] = 0;
  for (I = 1; I < 256; I++)
    Sum[I] = Sum[I - 1] + Count[I - 1];

  // Build next array
  for (I = 0; I < InputSize; I++)
    {
      UINT8 Ch = pInput[I];
      pNext[Sum[Ch]++] = I;
    }

  // Reconstruct original string
  Idx = OrigIndex;
  for (I = 0; I < InputSize; I++)
    {
      pOutput[I] = pInput[Idx];
      Idx = pNext[Idx];
    }

  free (pNext);
  return TRUE;
}
