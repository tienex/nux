/** @file
  Compression Pipeline Implementation

  Implements a complete compression pipeline combining BWT, MTF, RAD50RLE,
  LZ78, and Range encoding for maximum compression.

  Pipeline stages:
  1. BWT (Burrows-Wheeler Transform) - Groups similar characters
  2. MTF (Move-To-Front) - Converts to small values
  3. RAD50RLE - RAD-50 encoding with LEB128 + RLE + bit transposition
  4. LZ78 (Dictionary Compression) - Compresses repetitive patterns
  5. Range Encoding - Final entropy compression

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
#include "rad50rle.h"
#include "lz78.h"
#include "lz78_windowed.h"
#include "range.h"

/**
  Compress data using full pipeline (BWT -> MTF -> RAD50RLE -> Windowed LZ78 -> Range).

  @param[in]  pInput      Input data buffer.
  @param[in]  InputSize   Size of input data.
  @param[in]  WindowSize  Window size for LZ78 (4K-1M).
  @param[out] pOutput     Output buffer for compressed data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pCompSize   Actual compressed size.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
CompressFull (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  IN  UINT32       WindowSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pCompSize
  )
{
  UINT8 *pBWTOutput;
  UINT8 *pMTFOutput;
  UINT8 *pRAD50Output;
  UINT8 *pLZ78Output;
  size_t BWTSize, MTFSize, RAD50Size, LZ78Size, RangeSize;
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
  if (OutputSize < 20)
    return FALSE;

  // Allocate temporary buffers (generous sizing)
  pBWTOutput = (UINT8 *) malloc (InputSize);
  pMTFOutput = (UINT8 *) malloc (InputSize);
  pRAD50Output = (UINT8 *) malloc (InputSize * 3);  // RAD50RLE may expand
  pLZ78Output = (UINT8 *) malloc (InputSize * 3);

  if (pBWTOutput == NULL || pMTFOutput == NULL ||
      pRAD50Output == NULL || pLZ78Output == NULL)
    {
      free (pBWTOutput);
      free (pMTFOutput);
      free (pRAD50Output);
      free (pLZ78Output);
      return FALSE;
    }

  // Stage 1: BWT Transform
  if (!BWTTransform (pInput, InputSize, pBWTOutput, &BWTIndex))
    {
      free (pBWTOutput);
      free (pMTFOutput);
      free (pRAD50Output);
      free (pLZ78Output);
      return FALSE;
    }

  BWTSize = InputSize;

  // Stage 2: MTF Encoding
  if (!MTFEncode (pBWTOutput, BWTSize, pMTFOutput))
    {
      free (pBWTOutput);
      free (pMTFOutput);
      free (pRAD50Output);
      free (pLZ78Output);
      return FALSE;
    }

  MTFSize = BWTSize;

  // Stage 3: RAD50RLE Encoding (RAD-50 + LEB128 + RLE + bit transposition)
  if (!RAD50RLEEncode (pMTFOutput, MTFSize, pRAD50Output, InputSize * 3, &RAD50Size))
    {
      free (pBWTOutput);
      free (pMTFOutput);
      free (pRAD50Output);
      free (pLZ78Output);
      return FALSE;
    }

  // Stage 4: Windowed LZ78 Compression
  if (!LZ78WindowedCompress (pRAD50Output, RAD50Size, WindowSize,
                              pLZ78Output, InputSize * 3, &LZ78Size))
    {
      free (pBWTOutput);
      free (pMTFOutput);
      free (pRAD50Output);
      free (pLZ78Output);
      return FALSE;
    }

  // Write header: original size, BWT index, window size, RAD50 size, LZ78 size
  pOutput[0] = (InputSize >> 24) & 0xFF;
  pOutput[1] = (InputSize >> 16) & 0xFF;
  pOutput[2] = (InputSize >> 8) & 0xFF;
  pOutput[3] = InputSize & 0xFF;

  pOutput[4] = (BWTIndex >> 24) & 0xFF;
  pOutput[5] = (BWTIndex >> 16) & 0xFF;
  pOutput[6] = (BWTIndex >> 8) & 0xFF;
  pOutput[7] = BWTIndex & 0xFF;

  pOutput[8] = (WindowSize >> 24) & 0xFF;
  pOutput[9] = (WindowSize >> 16) & 0xFF;
  pOutput[10] = (WindowSize >> 8) & 0xFF;
  pOutput[11] = WindowSize & 0xFF;

  pOutput[12] = (RAD50Size >> 24) & 0xFF;
  pOutput[13] = (RAD50Size >> 16) & 0xFF;
  pOutput[14] = (RAD50Size >> 8) & 0xFF;
  pOutput[15] = RAD50Size & 0xFF;

  pOutput[16] = (LZ78Size >> 24) & 0xFF;
  pOutput[17] = (LZ78Size >> 16) & 0xFF;
  pOutput[18] = (LZ78Size >> 8) & 0xFF;
  pOutput[19] = LZ78Size & 0xFF;

  // Stage 5: Range Encoding
  Result = RangeEncode (pLZ78Output, LZ78Size,
                        pOutput + 20, OutputSize - 20,
                        &RangeSize);

  free (pBWTOutput);
  free (pMTFOutput);
  free (pRAD50Output);
  free (pLZ78Output);

  if (!Result)
    return FALSE;

  *pCompSize = 20 + RangeSize;
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
  UINT8 *pRAD50Output;
  UINT8 *pMTFOutput;
  UINT8 *pBWTOutput;
  UINT32 OrigSize, RAD50Size, LZ78Size, BWTIndex, WindowSize;
  size_t RangeSize, DecompLZ78Size, DecompRAD50Size, MTFSize, BWTSize;
  BOOLEAN Result;

  if (pInput == NULL || pOutput == NULL || pDecompSize == NULL)
    return FALSE;

  if (InputSize < 20)
    return FALSE;

  // Read header
  OrigSize = ((UINT32)pInput[0] << 24) | ((UINT32)pInput[1] << 16) |
             ((UINT32)pInput[2] << 8) | pInput[3];

  BWTIndex = ((UINT32)pInput[4] << 24) | ((UINT32)pInput[5] << 16) |
             ((UINT32)pInput[6] << 8) | pInput[7];

  WindowSize = ((UINT32)pInput[8] << 24) | ((UINT32)pInput[9] << 16) |
               ((UINT32)pInput[10] << 8) | pInput[11];

  RAD50Size = ((UINT32)pInput[12] << 24) | ((UINT32)pInput[13] << 16) |
              ((UINT32)pInput[14] << 8) | pInput[15];

  LZ78Size = ((UINT32)pInput[16] << 24) | ((UINT32)pInput[17] << 16) |
             ((UINT32)pInput[18] << 8) | pInput[19];

  if (OrigSize > OutputSize)
    return FALSE;

  // Allocate temporary buffers
  pRangeOutput = (UINT8 *) malloc (LZ78Size * 2);
  pLZ78Output = (UINT8 *) malloc (RAD50Size * 2);
  pRAD50Output = (UINT8 *) malloc (OrigSize * 2);
  pMTFOutput = (UINT8 *) malloc (OrigSize);
  pBWTOutput = (UINT8 *) malloc (OrigSize);

  if (pRangeOutput == NULL || pLZ78Output == NULL ||
      pRAD50Output == NULL || pMTFOutput == NULL || pBWTOutput == NULL)
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pRAD50Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  // Stage 1: Range Decoding
  if (!RangeDecode (pInput + 20, InputSize - 20,
                    pRangeOutput, LZ78Size * 2,
                    &RangeSize))
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pRAD50Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  // Verify LZ78 size matches
  if (RangeSize != LZ78Size)
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pRAD50Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  // Stage 2: Windowed LZ78 Decompression
  if (!LZ78WindowedDecompress (pRangeOutput, LZ78Size, WindowSize,
                                pLZ78Output, RAD50Size * 2,
                                &DecompLZ78Size))
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pRAD50Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  // Verify RAD50 size matches
  if (DecompLZ78Size != RAD50Size)
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pRAD50Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  // Stage 3: RAD50RLE Decoding
  if (!RAD50RLEDecode (pLZ78Output, RAD50Size,
                       pRAD50Output, OrigSize * 2,
                       &DecompRAD50Size))
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pRAD50Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  MTFSize = DecompRAD50Size;

  // Stage 4: MTF Decoding
  if (!MTFDecode (pRAD50Output, MTFSize, pMTFOutput))
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pRAD50Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  BWTSize = MTFSize;

  // Stage 5: Inverse BWT
  if (!BWTInverse (pMTFOutput, BWTSize, BWTIndex, pBWTOutput))
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pRAD50Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  // Verify size matches
  if (BWTSize != OrigSize)
    {
      free (pRangeOutput);
      free (pLZ78Output);
      free (pRAD50Output);
      free (pMTFOutput);
      free (pBWTOutput);
      return FALSE;
    }

  // Copy to output
  memcpy (pOutput, pBWTOutput, OrigSize);

  free (pRangeOutput);
  free (pLZ78Output);
  free (pRAD50Output);
  free (pMTFOutput);
  free (pBWTOutput);

  *pDecompSize = OrigSize;
  return TRUE;
}
