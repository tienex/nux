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

  if (OutputSize < 12)  // Minimum: header (8) + BWT index (4)
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

  // Write header: BWT index
  pOutput[0] = (BWTIndex >> 24) & 0xFF;
  pOutput[1] = (BWTIndex >> 16) & 0xFF;
  pOutput[2] = (BWTIndex >> 8) & 0xFF;
  pOutput[3] = BWTIndex & 0xFF;

  // Stage 3: Range Encoding
  Result = RangeEncode (pMTFOutput, InputSize,
                        pOutput + 4, OutputSize - 4,
                        &RangeSize);

  free (pBWTOutput);
  free (pMTFOutput);

  if (!Result)
    return FALSE;

  *pCompSize = RangeSize + 4;
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

  if (InputSize < 4)
    return FALSE;

  // Read header: BWT index
  BWTIndex = ((UINT32)pInput[0] << 24) | ((UINT32)pInput[1] << 16) |
             ((UINT32)pInput[2] << 8) | pInput[3];

  // Allocate temporary buffers (use OutputSize as guide)
  pRangeOutput = (UINT8 *) malloc (OutputSize);
  pMTFOutput = (UINT8 *) malloc (OutputSize);

  if (pRangeOutput == NULL || pMTFOutput == NULL)
    {
      free (pRangeOutput);
      free (pMTFOutput);
      return FALSE;
    }

  // Stage 1: Range Decoding
  if (!RangeDecode (pInput + 4, InputSize - 4,
                    pRangeOutput, OutputSize,
                    &RangeSize))
    {
      free (pRangeOutput);
      free (pMTFOutput);
      return FALSE;
    }

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
