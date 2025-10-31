/** @file
  Compression Pipeline Implementation

  Implements a complete compression pipeline combining BWT, MTF, LZ78,
  and Range encoding for maximum compression.

  Pipeline stages:
  1. BWT (Burrows-Wheeler Transform) - Groups similar characters
  2. MTF (Move-To-Front) - Converts to small values
  3. LZ78 (Dictionary Compression) - Compresses repetitive patterns
  4. Range Encoding - Final entropy compression

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "compress.h"
#include "bwt.h"
#include "mtf.h"
#include "lz78.h"
#include "range.h"

/**
  Compress data using full pipeline (BWT -> MTF -> LZ78 -> Range).

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
  UINT8 *pLZ78Output;
  size_t BWTSize, MTFSize, LZ78Size, RangeSize;
  UINT32 BWTIndex;
  BOOLEAN Result;

  if (pInput == NULL || pOutput == NULL || pCompSize == NULL)
    return FALSE;

  if (InputSize == 0)
    {
      *pCompSize = 0;
      return TRUE;
    }

  // Minimum output size needed for headers
  if (OutputSize < 16)
    return FALSE;

  // Allocate temporary buffers (generous sizing)
  pBWTOutput = (UINT8 *) malloc (InputSize);
  pMTFOutput = (UINT8 *) malloc (InputSize);
  pLZ78Output = (UINT8 *) malloc (InputSize * 2);

  if (pBWTOutput == NULL || pMTFOutput == NULL || pLZ78Output == NULL)
    {
      free (pBWTOutput);
      free (pMTFOutput);
      free (pLZ78Output);
      return FALSE;
    }

  // Stage 1: BWT Transform
  if (!BWTTransform (pInput, InputSize, pBWTOutput, &BWTIndex))
    {
      free (pBWTOutput);
      free (pMTFOutput);
      free (pLZ78Output);
      return FALSE;
    }

  BWTSize = InputSize;

  // Stage 2: MTF Encoding
  if (!MTFEncode (pBWTOutput, BWTSize, pMTFOutput))
    {
      free (pBWTOutput);
      free (pMTFOutput);
      free (pLZ78Output);
      return FALSE;
    }

  MTFSize = BWTSize;

  // Stage 3: LZ78 Compression
  if (!LZ78Compress (pMTFOutput, MTFSize, pLZ78Output, InputSize * 2, &LZ78Size))
    {
      free (pBWTOutput);
      free (pMTFOutput);
      free (pLZ78Output);
      return FALSE;
    }

  // Write header: original size, BWT index, LZ78 size
  pOutput[0] = (InputSize >> 24) & 0xFF;
  pOutput[1] = (InputSize >> 16) & 0xFF;
  pOutput[2] = (InputSize >> 8) & 0xFF;
  pOutput[3] = InputSize & 0xFF;

  pOutput[4] = (BWTIndex >> 24) & 0xFF;
  pOutput[5] = (BWTIndex >> 16) & 0xFF;
  pOutput[6] = (BWTIndex >> 8) & 0xFF;
  pOutput[7] = BWTIndex & 0xFF;

  pOutput[8] = (LZ78Size >> 24) & 0xFF;
  pOutput[9] = (LZ78Size >> 16) & 0xFF;
  pOutput[10] = (LZ78Size >> 8) & 0xFF;
  pOutput[11] = LZ78Size & 0xFF;

  // Stage 4: Range Encoding
  Result = RangeEncode (pLZ78Output, LZ78Size,
                        pOutput + 12, OutputSize - 12,
                        &RangeSize);

  free (pBWTOutput);
  free (pMTFOutput);
  free (pLZ78Output);

  if (!Result)
    return FALSE;

  *pCompSize = 12 + RangeSize;
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
  UINT8 *pLZ78Output;
  UINT8 *pMTFOutput;
  UINT8 *pBWTOutput;
  UINT32 OrigSize, LZ78Size, BWTIndex;
  size_t RangeSize, DecompLZ78Size, MTFSize, BWTSize;
  BOOLEAN Result;

  if (pInput == NULL || pOutput == NULL || pDecompSize == NULL)
    return FALSE;

  if (InputSize < 12)
    return FALSE;

  // Read header
  OrigSize = ((UINT32)pInput[0] << 24) | ((UINT32)pInput[1] << 16) |
             ((UINT32)pInput[2] << 8) | pInput[3];

  BWTIndex = ((UINT32)pInput[4] << 24) | ((UINT32)pInput[5] << 16) |
             ((UINT32)pInput[6] << 8) | pInput[7];

  LZ78Size = ((UINT32)pInput[8] << 24) | ((UINT32)pInput[9] << 16) |
             ((UINT32)pInput[10] << 8) | pInput[11];

  if (OrigSize > OutputSize)
    return FALSE;

  // Allocate temporary buffers
  pRangeOutput = (UINT8 *) malloc (LZ78Size * 2);
  pLZ78Output = (UINT8 *) malloc (OrigSize * 2);
  pMTFOutput = (UINT8 *) malloc (OrigSize);
  pBWTOutput = (UINT8 *) malloc (OrigSize);

  if (pRangeOutput == NULL || pLZ78Output == NULL ||
      pMTFOutput == NULL || pBWTOutput == NULL)
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  // Stage 1: Range Decoding
  if (!RangeDecode (pInput + 12, InputSize - 12,
                    pRangeOutput, LZ78Size * 2,
                    &RangeSize))
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  // Verify LZ78 size matches
  if (RangeSize != LZ78Size)
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  // Stage 2: LZ78 Decompression
  if (!LZ78Decompress (pRangeOutput, LZ78Size,
                       pLZ78Output, OrigSize * 2,
                       &DecompLZ78Size))
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  MTFSize = DecompLZ78Size;

  // Stage 3: MTF Decoding
  if (!MTFDecode (pLZ78Output, MTFSize, pMTFOutput))
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  BWTSize = MTFSize;

  // Stage 4: Inverse BWT
  if (!BWTInverse (pMTFOutput, BWTSize, BWTIndex, pBWTOutput))
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  // Verify size matches
  if (BWTSize != OrigSize)
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  // Copy to output
  memcpy (pOutput, pBWTOutput, OrigSize);

  free (pRangeOutput);
  free (pLZ78Output);
  free (pMTFOutput);
  free (pBWTOutput);

  *pDecompSize = OrigSize;
  return TRUE;
}
