/** @file
  LZ78 Dictionary Compression Implementation

  Implements the LZ78 compression algorithm using a dictionary-based
  approach. The algorithm builds a dictionary of patterns during encoding
  and emits (index, character) pairs.

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "types.h"
#include <string.h>
#include <stdlib.h>
#include "lz78.h"

#define LZ78_MAX_DICT_SIZE 4096  // Maximum dictionary entries

/**
  Compress data using LZ78 algorithm.

  @param[in]  pInput      Input data buffer.
  @param[in]  InputSize   Size of input data.
  @param[out] pOutput     Output buffer for compressed data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pCompSize   Actual compressed size.

  @return TRUE if successful, FALSE if output buffer too small.
**/
BOOLEAN
LZ78Compress (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pCompSize
  )
{
  LZ78_ENTRY *pDict;
  UINT16 DictSize;
  size_t InPos, OutPos;
  UINT16 CurrentIndex;
  UINT8 CurrentChar;

  if (pInput == NULL || pOutput == NULL || pCompSize == NULL)
    return FALSE;

  if (InputSize == 0)
    {
      *pCompSize = 0;
      return TRUE;
    }

  // Allocate dictionary
  pDict = (LZ78_ENTRY *) malloc (LZ78_MAX_DICT_SIZE * sizeof (LZ78_ENTRY));
  if (pDict == NULL)
    return FALSE;

  DictSize = 0;
  InPos = 0;
  OutPos = 1;  // Reserve byte 0 for final match flag
  CurrentIndex = 0;
  UINT8 HasFinalMatch = 0;

  while (InPos < InputSize)
    {
      CurrentChar = pInput[InPos++];

      // Search for (CurrentIndex, CurrentChar) in dictionary
      UINT16 FoundIndex = 0;
      BOOLEAN Found = FALSE;

      for (UINT16 I = 0; I < DictSize; I++)
        {
          if (pDict[I].Prefix == CurrentIndex && pDict[I].Character == CurrentChar)
            {
              FoundIndex = I + 1;  // Dictionary indices are 1-based
              Found = TRUE;
              break;
            }
        }

      if (Found)
        {
          // Continue with this pattern
          CurrentIndex = FoundIndex;
        }
      else
        {
          // Emit (CurrentIndex, CurrentChar)
          if (OutPos + 3 > OutputSize)
            {
              free (pDict);
              return FALSE;  // Output buffer too small
            }

          pOutput[OutPos++] = (CurrentIndex >> 8) & 0xFF;
          pOutput[OutPos++] = CurrentIndex & 0xFF;
          pOutput[OutPos++] = CurrentChar;

          // Add to dictionary if space available
          if (DictSize < LZ78_MAX_DICT_SIZE)
            {
              pDict[DictSize].Prefix = CurrentIndex;
              pDict[DictSize].Character = CurrentChar;
              DictSize++;
            }

          CurrentIndex = 0;
        }
    }

  // Emit final partial match if needed
  if (CurrentIndex != 0)
    {
      if (OutPos + 2 > OutputSize)
        {
          free (pDict);
          return FALSE;
        }

      pOutput[OutPos++] = (CurrentIndex >> 8) & 0xFF;
      pOutput[OutPos++] = CurrentIndex & 0xFF;
      HasFinalMatch = 1;
    }

  // Write flag byte at position 0
  pOutput[0] = HasFinalMatch;

  free (pDict);
  *pCompSize = OutPos;
  return TRUE;
}

/**
  Decompress LZ78 compressed data.

  @param[in]  pInput      Compressed data buffer.
  @param[in]  InputSize   Size of compressed data.
  @param[out] pOutput     Output buffer for decompressed data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pDecompSize Actual decompressed size.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
LZ78Decompress (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pDecompSize
  )
{
  LZ78_ENTRY *pDict;
  UINT16 DictSize;
  size_t InPos, OutPos;

  if (pInput == NULL || pOutput == NULL || pDecompSize == NULL)
    return FALSE;

  if (InputSize < 1)
    {
      *pDecompSize = 0;
      return TRUE;
    }

  // Read flag byte
  UINT8 HasFinalMatch = pInput[0];

  // Allocate dictionary
  pDict = (LZ78_ENTRY *) malloc (LZ78_MAX_DICT_SIZE * sizeof (LZ78_ENTRY));
  if (pDict == NULL)
    return FALSE;

  DictSize = 0;
  InPos = 1;  // Skip flag byte
  OutPos = 0;

  // Process (index, character) pairs
  while (InPos + 3 <= InputSize)
    {
      UINT16 Index = ((UINT16)pInput[InPos] << 8) | pInput[InPos + 1];
      UINT8 Character = pInput[InPos + 2];
      InPos += 3;

      // Reconstruct string from dictionary
      UINT8 Buffer[256];
      size_t BufLen = 0;

      // Trace back through dictionary
      UINT16 Idx = Index;
      while (Idx > 0 && BufLen < sizeof(Buffer))
        {
          if (Idx - 1 >= DictSize)
            {
              free (pDict);
              return FALSE;  // Invalid dictionary reference
            }
          Buffer[BufLen++] = pDict[Idx - 1].Character;
          Idx = pDict[Idx - 1].Prefix;
        }

      // Output in reverse order
      for (size_t I = BufLen; I > 0; I--)
        {
          if (OutPos >= OutputSize)
            {
              free (pDict);
              return FALSE;
            }
          pOutput[OutPos++] = Buffer[I - 1];
        }

      // Output new character
      if (OutPos >= OutputSize)
        {
          free (pDict);
          return FALSE;
        }
      pOutput[OutPos++] = Character;

      // Add to dictionary
      if (DictSize < LZ78_MAX_DICT_SIZE)
        {
          pDict[DictSize].Prefix = Index;
          pDict[DictSize].Character = Character;
          DictSize++;
        }
    }

  // Handle final partial match if present
  if (HasFinalMatch && InPos + 2 <= InputSize)
    {
      UINT16 Index = ((UINT16)pInput[InPos] << 8) | pInput[InPos + 1];

      // Reconstruct string from dictionary
      UINT8 Buffer[256];
      size_t BufLen = 0;

      UINT16 Idx = Index;
      while (Idx > 0 && BufLen < sizeof(Buffer))
        {
          if (Idx - 1 >= DictSize)
            {
              free (pDict);
              return FALSE;
            }
          Buffer[BufLen++] = pDict[Idx - 1].Character;
          Idx = pDict[Idx - 1].Prefix;
        }

      // Output in reverse order
      for (size_t I = BufLen; I > 0; I--)
        {
          if (OutPos >= OutputSize)
            {
              free (pDict);
              return FALSE;
            }
          pOutput[OutPos++] = Buffer[I - 1];
        }
    }

  free (pDict);
  *pDecompSize = OutPos;
  return TRUE;
}
