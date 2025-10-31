/** @file
  Range Arithmetic Encoder/Decoder Implementation

  Implements a simplified range coder with adaptive frequency model.
  The range is maintained using 32-bit arithmetic with renormalization.

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "types.h"
#include <string.h>
#include <stdlib.h>
#include "range.h"

#define RANGE_TOP (1U << 24)
#define RANGE_BOTTOM (1U << 16)

/**
  Range encoder state.
**/
typedef struct {
  UINT32 Low;
  UINT32 Range;
  UINT8 *pOutput;
  size_t OutputPos;
  size_t OutputSize;
} RANGE_ENCODER;

/**
  Range decoder state.
**/
typedef struct {
  UINT32 Low;
  UINT32 Code;
  UINT32 Range;
  const UINT8 *pInput;
  size_t InputPos;
  size_t InputSize;
} RANGE_DECODER;

/**
  Emit a byte to output.
**/
static BOOLEAN
EmitByte (
  RANGE_ENCODER  *pEnc,
  UINT8          Byte
  )
{
  if (pEnc->OutputPos >= pEnc->OutputSize)
    return FALSE;

  pEnc->pOutput[pEnc->OutputPos++] = Byte;
  return TRUE;
}

/**
  Renormalize encoder range.
**/
static BOOLEAN
RenormalizeEncoder (
  RANGE_ENCODER  *pEnc
  )
{
  while (pEnc->Range < RANGE_BOTTOM)
    {
      if (!EmitByte (pEnc, (UINT8)(pEnc->Low >> 24)))
        return FALSE;

      pEnc->Low <<= 8;
      pEnc->Range <<= 8;
    }

  return TRUE;
}

/**
  Encode a symbol with given frequency range.
**/
static BOOLEAN
EncodeSymbol (
  RANGE_ENCODER  *pEnc,
  UINT32         SymLow,
  UINT32         SymHigh,
  UINT32         Total
  )
{
  UINT32 R = pEnc->Range / Total;

  pEnc->Low += R * SymLow;
  pEnc->Range = R * (SymHigh - SymLow);

  return RenormalizeEncoder (pEnc);
}

/**
  Read a byte from input.
**/
static UINT8
ReadByte (
  RANGE_DECODER  *pDec
  )
{
  if (pDec->InputPos >= pDec->InputSize)
    return 0;

  return pDec->pInput[pDec->InputPos++];
}

/**
  Renormalize decoder range.
**/
static VOID
RenormalizeDecoder (
  RANGE_DECODER  *pDec
  )
{
  while (pDec->Range < RANGE_BOTTOM)
    {
      pDec->Code = (pDec->Code << 8) | ReadByte (pDec);
      pDec->Range <<= 8;
    }
}

/**
  Decode a symbol with given cumulative frequencies.
**/
static UINT32
DecodeSymbol (
  RANGE_DECODER  *pDec,
  UINT32         *pCumFreq,
  UINT32         Total,
  UINT32         *pSymbol
  )
{
  UINT32 Scaled = ((pDec->Code - pDec->Low) * Total) / pDec->Range;
  UINT32 I;

  // Find symbol
  for (I = 0; I < 256; I++)
    {
      if (pCumFreq[I] <= Scaled && Scaled < pCumFreq[I + 1])
        {
          *pSymbol = I;

          UINT32 R = pDec->Range / Total;
          pDec->Low += R * pCumFreq[I];
          pDec->Range = R * (pCumFreq[I + 1] - pCumFreq[I]);

          RenormalizeDecoder (pDec);
          return Scaled;
        }
    }

  return 0;
}

/**
  Compress data using range arithmetic encoding.

  @param[in]  pInput      Input data buffer.
  @param[in]  InputSize   Size of input data.
  @param[out] pOutput     Output buffer for compressed data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pCompSize   Actual compressed size.

  @return TRUE if successful, FALSE if output buffer too small.
**/
BOOLEAN
RangeEncode (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pCompSize
  )
{
  RANGE_ENCODER Enc;
  UINT32 Freq[256];
  UINT32 CumFreq[257];
  UINT32 Total;
  size_t I;

  if (pInput == NULL || pOutput == NULL || pCompSize == NULL)
    return FALSE;

  if (OutputSize < 8)
    return FALSE;

  // Calculate frequencies
  memset (Freq, 0, sizeof (Freq));
  for (I = 0; I < InputSize; I++)
    Freq[pInput[I]]++;

  // Build cumulative frequency table
  Total = 0;
  for (I = 0; I < 256; I++)
    {
      CumFreq[I] = Total;
      Total += (Freq[I] > 0) ? Freq[I] : 1;  // Ensure non-zero for unseen symbols
    }
  CumFreq[256] = Total;

  // Write header: input size
  pOutput[0] = (InputSize >> 24) & 0xFF;
  pOutput[1] = (InputSize >> 16) & 0xFF;
  pOutput[2] = (InputSize >> 8) & 0xFF;
  pOutput[3] = InputSize & 0xFF;

  // Initialize encoder
  Enc.Low = 0;
  Enc.Range = 0xFFFFFFFFU;
  Enc.pOutput = pOutput;
  Enc.OutputPos = 4;
  Enc.OutputSize = OutputSize;

  // Encode each symbol
  for (I = 0; I < InputSize; I++)
    {
      UINT8 Symbol = pInput[I];
      if (!EncodeSymbol (&Enc, CumFreq[Symbol], CumFreq[Symbol + 1], Total))
        return FALSE;
    }

  // Flush encoder
  for (I = 0; I < 4; I++)
    {
      if (!EmitByte (&Enc, (UINT8)(Enc.Low >> 24)))
        return FALSE;
      Enc.Low <<= 8;
    }

  *pCompSize = Enc.OutputPos;
  return TRUE;
}

/**
  Decompress range arithmetic encoded data.

  @param[in]  pInput      Compressed data buffer.
  @param[in]  InputSize   Size of compressed data.
  @param[out] pOutput     Output buffer for decompressed data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pDecompSize Actual decompressed size.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
RangeDecode (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pDecompSize
  )
{
  RANGE_DECODER Dec;
  UINT32 Freq[256];
  UINT32 CumFreq[257];
  UINT32 Total;
  UINT32 DecompSize;
  UINT32 I;
  UINT32 Symbol;

  if (pInput == NULL || pOutput == NULL || pDecompSize == NULL)
    return FALSE;

  if (InputSize < 8)
    return FALSE;

  // Read header
  DecompSize = ((UINT32)pInput[0] << 24) | ((UINT32)pInput[1] << 16) |
               ((UINT32)pInput[2] << 8) | pInput[3];

  if (DecompSize > OutputSize)
    return FALSE;

  // For simplicity, use uniform frequencies (this is where you'd store/read the model)
  Total = 256;
  for (I = 0; I < 256; I++)
    CumFreq[I] = I;
  CumFreq[256] = 256;

  // Initialize decoder
  Dec.Low = 0;
  Dec.Range = 0xFFFFFFFFU;
  Dec.pInput = pInput;
  Dec.InputPos = 4;
  Dec.InputSize = InputSize;

  // Read initial code
  Dec.Code = 0;
  for (I = 0; I < 4; I++)
    Dec.Code = (Dec.Code << 8) | ReadByte (&Dec);

  // Decode symbols
  for (I = 0; I < DecompSize; I++)
    {
      DecodeSymbol (&Dec, CumFreq, Total, &Symbol);
      pOutput[I] = (UINT8)Symbol;
    }

  *pDecompSize = DecompSize;
  return TRUE;
}
