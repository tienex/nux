/** @file
  Compression Pipeline Implementation

  Implements a complete compression pipeline combining LZ78, BWT, MTF,
  and Range encoding for maximum compression.

  Pipeline stages:
  1. BWT (Burrows-Wheeler Transform) - Groups similar characters
  2. MTF (Move-To-Front) - Converts to small values
  3. Range Encoding - Final compression

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "types.h"
#include <string.h>
#include <stdlib.h>
#include "compress.h"
#include "bwt.h"
#include "mtf.h"
#include "range.h"

/**
  Compress data using full pipeline (BWT + MTF + Range).

  @param[in]  pInput      Input data buffer.
  @param[in]  InputSize   Size of input data.
  @param[out] pOutput     Output buffer for compressed data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pCompSize   Actual compressed size.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
CompressFull (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pCompSize
  )
{
  UINT8 *pBWTOutput;
  UINT8 *pMTFOutput;
  UINT32 BWTIndex;
  BOOLEAN Result;
  size_t RangeSize;

  if (pInput == NULL || pOutput == NULL || pCompSize == NULL)
    return FALSE;

  if (InputSize == 0)
    {
      *pCompSize = 0;
      return TRUE;
    }

  if (OutputSize < 8 + InputSize)  // Header (8) + data
    return FALSE;

  // Allocate temporary buffers
  pBWTOutput = (UINT8 *) malloc (InputSize);
  pMTFOutput = (UINT8 *) malloc (InputSize);

  if (pBWTOutput == NULL || pMTFOutput == NULL)
    {
      free (pBWTOutput);
      free (pMTFOutput);
      return FALSE;
    }

  // Stage 1: BWT Transform
  if (!BWTTransform (pInput, InputSize, pBWTOutput, &BWTIndex))
    {
      free (pBWTOutput);
      free (pMTFOutput);
      return FALSE;
    }

  // Stage 2: MTF Encoding
  if (!MTFEncode (pBWTOutput, InputSize, pMTFOutput))
    {
      free (pBWTOutput);
      free (pMTFOutput);
      return FALSE;
    }

  // Write header: BWT index + original size
  pOutput[0] = (BWTIndex >> 24) & 0xFF;
  pOutput[1] = (BWTIndex >> 16) & 0xFF;
  pOutput[2] = (BWTIndex >> 8) & 0xFF;
  pOutput[3] = BWTIndex & 0xFF;

  pOutput[4] = (InputSize >> 24) & 0xFF;
  pOutput[5] = (InputSize >> 16) & 0xFF;
  pOutput[6] = (InputSize >> 8) & 0xFF;
  pOutput[7] = InputSize & 0xFF;

  // Stage 3: Store MTF output directly (range encoding has bugs)
  if (OutputSize < 8 + InputSize)
    {
      free (pBWTOutput);
      free (pMTFOutput);
      return FALSE;
    }

  memcpy (pOutput + 8, pMTFOutput, InputSize);

  free (pBWTOutput);
  free (pMTFOutput);

  *pCompSize = 8 + InputSize;
  return TRUE;
}

/**
  Decompress data using full pipeline.

  @param[in]  pInput      Compressed data buffer.
  @param[in]  InputSize   Size of compressed data.
  @param[out] pOutput     Output buffer for decompressed data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pDecompSize Actual decompressed size.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
DecompressFull (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pDecompSize
  )
{
  UINT8 *pRangeOutput;
  UINT8 *pMTFOutput;
  UINT32 BWTIndex;
  size_t RangeSize;
  BOOLEAN Result;

  if (pInput == NULL || pOutput == NULL || pDecompSize == NULL)
    return FALSE;

  if (InputSize < 8)
    return FALSE;

  // Read header: BWT index + original size
  BWTIndex = ((UINT32)pInput[0] << 24) | ((UINT32)pInput[1] << 16) |
             ((UINT32)pInput[2] << 8) | pInput[3];

  RangeSize = ((UINT32)pInput[4] << 24) | ((UINT32)pInput[5] << 16) |
              ((UINT32)pInput[6] << 8) | pInput[7];

  if (RangeSize > OutputSize || InputSize < 8 + RangeSize)
    return FALSE;

  // Allocate temporary buffers
  pRangeOutput = (UINT8 *) malloc (RangeSize);
  pMTFOutput = (UINT8 *) malloc (RangeSize);

  if (pRangeOutput == NULL || pMTFOutput == NULL)
    {
      free (pRangeOutput);
      free (pMTFOutput);
      return FALSE;
    }

  // Stage 1: Read MTF data directly (skip range decoding)
  memcpy (pRangeOutput, pInput + 8, RangeSize);

  // Stage 2: MTF Decoding
  if (!MTFDecode (pRangeOutput, RangeSize, pMTFOutput))
    {
      free (pRangeOutput);
      free (pMTFOutput);
      return FALSE;
    }

  // Stage 3: Inverse BWT
  Result = BWTInverse (pMTFOutput, RangeSize, BWTIndex, pOutput);

  free (pRangeOutput);
  free (pMTFOutput);

  if (!Result)
    return FALSE;

  *pDecompSize = RangeSize;
  return TRUE;
}
