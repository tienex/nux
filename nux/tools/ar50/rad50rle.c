/** @file
  RAD-50 with RLE and Bit Transposition Implementation

  Optimized encoding for RAD-50 compatible data using:
  - LEB128 variable-length encoding for RAD-50 values (0-39)
  - RLE for non-RAD-50 values
  - Bit transposition for better compression

  Copyright (C) 2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rad50rle.h"

#define RAD50_MAX 39

/**
  Bit stream writer for non-byte-aligned output.
**/
typedef struct {
  UINT8 *pOutput;
  size_t OutputSize;
  size_t BytePos;
  UINT8 BitPos;
  UINT8 CurrentByte;
} BIT_WRITER;

/**
  Bit stream reader for non-byte-aligned input.
**/
typedef struct {
  const UINT8 *pInput;
  size_t InputSize;
  size_t BytePos;
  UINT8 BitPos;
} BIT_READER;

/**
  Initialize bit writer.
**/
static VOID
BitWriterInit (
  BIT_WRITER  *pWriter,
  UINT8       *pOutput,
  size_t      OutputSize
  )
{
  pWriter->pOutput = pOutput;
  pWriter->OutputSize = OutputSize;
  pWriter->BytePos = 0;
  pWriter->BitPos = 0;
  pWriter->CurrentByte = 0;
}

/**
  Write one bit to output.
**/
static BOOLEAN
BitWriteBit (
  BIT_WRITER  *pWriter,
  UINT8       Bit
  )
{
  pWriter->CurrentByte |= ((Bit & 1) << pWriter->BitPos);
  pWriter->BitPos++;

  if (pWriter->BitPos == 8)
    {
      if (pWriter->BytePos >= pWriter->OutputSize)
        return FALSE;
      pWriter->pOutput[pWriter->BytePos++] = pWriter->CurrentByte;
      pWriter->CurrentByte = 0;
      pWriter->BitPos = 0;
    }

  return TRUE;
}

/**
  Flush remaining bits to output.
**/
static BOOLEAN
BitWriterFlush (
  BIT_WRITER  *pWriter
  )
{
  if (pWriter->BitPos > 0)
    {
      if (pWriter->BytePos >= pWriter->OutputSize)
        return FALSE;
      pWriter->pOutput[pWriter->BytePos++] = pWriter->CurrentByte;
    }
  return TRUE;
}

/**
  Write unsigned integer in LEB128 format (bit-level).
**/
static BOOLEAN
BitWriteLEB128 (
  BIT_WRITER  *pWriter,
  UINT32      Value
  )
{
  do
    {
      UINT8 Byte = Value & 0x7F;
      Value >>= 7;
      if (Value != 0)
        Byte |= 0x80;  // More bytes follow

      // Write 8 bits
      for (int I = 0; I < 8; I++)
        {
          if (!BitWriteBit (pWriter, (Byte >> I) & 1))
            return FALSE;
        }
    }
  while (Value != 0);

  return TRUE;
}

/**
  Initialize bit reader.
**/
static VOID
BitReaderInit (
  BIT_READER   *pReader,
  const UINT8  *pInput,
  size_t       InputSize
  )
{
  pReader->pInput = pInput;
  pReader->InputSize = InputSize;
  pReader->BytePos = 0;
  pReader->BitPos = 0;
}

/**
  Read one bit from input.
**/
static BOOLEAN
BitReadBit (
  BIT_READER  *pReader,
  UINT8       *pBit
  )
{
  if (pReader->BytePos >= pReader->InputSize)
    return FALSE;

  *pBit = (pReader->pInput[pReader->BytePos] >> pReader->BitPos) & 1;
  pReader->BitPos++;

  if (pReader->BitPos == 8)
    {
      pReader->BytePos++;
      pReader->BitPos = 0;
    }

  return TRUE;
}

/**
  Read unsigned integer in LEB128 format (bit-level).
**/
static BOOLEAN
BitReadLEB128 (
  BIT_READER  *pReader,
  UINT32      *pValue
  )
{
  UINT32 Result = 0;
  UINT32 Shift = 0;

  while (TRUE)
    {
      UINT8 Byte = 0;

      // Read 8 bits
      for (int I = 0; I < 8; I++)
        {
          UINT8 Bit;
          if (!BitReadBit (pReader, &Bit))
            return FALSE;
          Byte |= (Bit << I);
        }

      Result |= ((UINT32)(Byte & 0x7F) << Shift);
      if ((Byte & 0x80) == 0)
        break;

      Shift += 7;
      if (Shift >= 32)
        return FALSE;  // Overflow
    }

  *pValue = Result;
  return TRUE;
}

/**
  Transpose bits: output byte N contains bit N from 8 consecutive input bytes.
**/
static VOID
BitTranspose (
  const UINT8  *pInput,
  size_t       InputSize,
  UINT8        *pOutput
  )
{
  size_t I, J;
  size_t Blocks = (InputSize + 7) / 8;

  memset (pOutput, 0, InputSize);

  for (I = 0; I < Blocks; I++)
    {
      size_t Base = I * 8;
      size_t Count = (Base + 8 <= InputSize) ? 8 : (InputSize - Base);

      for (J = 0; J < Count; J++)
        {
          UINT8 InByte = pInput[Base + J];
          // Distribute bits of InByte across 8 output bytes
          for (int Bit = 0; Bit < 8; Bit++)
            {
              if (InByte & (1 << Bit))
                pOutput[Base + Bit] |= (1 << J);
            }
        }
    }
}

/**
  Inverse bit transpose.
**/
static VOID
BitTransposeInverse (
  const UINT8  *pInput,
  size_t       InputSize,
  UINT8        *pOutput
  )
{
  // Bit transpose is self-inverse for full blocks
  BitTranspose (pInput, InputSize, pOutput);
}

/**
  Encode data using RAD-50 + RLE + bit transposition.
**/
BOOLEAN
RAD50RLEEncode (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pEncSize
  )
{
  BIT_WRITER Writer;
  UINT8 *pBitStream;
  UINT8 *pTransposed;
  size_t I, BitStreamSize;

  if (pInput == NULL || pOutput == NULL || pEncSize == NULL)
    return FALSE;

  if (InputSize == 0)
    {
      *pEncSize = 0;
      return TRUE;
    }

  // Allocate temporary buffer for bit stream (generous sizing)
  pBitStream = (UINT8 *) malloc (InputSize * 3);
  if (pBitStream == NULL)
    return FALSE;

  BitWriterInit (&Writer, pBitStream, InputSize * 3);

  // Encode each byte
  I = 0;
  while (I < InputSize)
    {
      UINT8 Value = pInput[I];

      if (Value <= RAD50_MAX)
        {
          // RAD-50 compatible: emit bit 1, then value in LEB128
          if (!BitWriteBit (&Writer, 1))
            {
              free (pBitStream);
              return FALSE;
            }
          if (!BitWriteLEB128 (&Writer, Value))
            {
              free (pBitStream);
              return FALSE;
            }
          I++;
        }
      else
        {
          // Non-RAD-50: emit bit 0, then RLE encode sequence
          if (!BitWriteBit (&Writer, 0))
            {
              free (pBitStream);
              return FALSE;
            }

          // Find run length of non-RAD-50 values
          size_t RunStart = I;
          while (I < InputSize && pInput[I] > RAD50_MAX)
            I++;

          size_t RunLen = I - RunStart;

          // Emit run length in LEB128
          if (!BitWriteLEB128 (&Writer, RunLen))
            {
              free (pBitStream);
              return FALSE;
            }

          // Emit each byte value in LEB128
          for (size_t J = RunStart; J < I; J++)
            {
              if (!BitWriteLEB128 (&Writer, pInput[J]))
                {
                  free (pBitStream);
                  return FALSE;
                }
            }
        }
    }

  // Flush bit writer
  if (!BitWriterFlush (&Writer))
    {
      free (pBitStream);
      return FALSE;
    }

  BitStreamSize = Writer.BytePos;

  // Allocate buffer for transposed data
  pTransposed = (UINT8 *) malloc (BitStreamSize);
  if (pTransposed == NULL)
    {
      free (pBitStream);
      return FALSE;
    }

  // Apply bit transposition
  BitTranspose (pBitStream, BitStreamSize, pTransposed);

  // Copy to output
  if (BitStreamSize + 4 > OutputSize)
    {
      free (pBitStream);
      free (pTransposed);
      return FALSE;
    }

  // Write header: original input size (for decoder to know output size)
  pOutput[0] = (InputSize >> 24) & 0xFF;
  pOutput[1] = (InputSize >> 16) & 0xFF;
  pOutput[2] = (InputSize >> 8) & 0xFF;
  pOutput[3] = InputSize & 0xFF;

  memcpy (pOutput + 4, pTransposed, BitStreamSize);

  free (pBitStream);
  free (pTransposed);

  *pEncSize = 4 + BitStreamSize;
  return TRUE;
}

/**
  Decode RAD-50 + RLE + bit transposition data.
**/
BOOLEAN
RAD50RLEDecode (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pDecSize
  )
{
  BIT_READER Reader;
  UINT8 *pUntransposed;
  UINT32 OrigSize;
  size_t OutPos;
  size_t BitStreamSize;

  if (pInput == NULL || pOutput == NULL || pDecSize == NULL)
    return FALSE;

  if (InputSize < 4)
    return FALSE;

  // Read header: original size
  OrigSize = ((UINT32)pInput[0] << 24) | ((UINT32)pInput[1] << 16) |
             ((UINT32)pInput[2] << 8) | pInput[3];

  if (OrigSize > OutputSize)
    return FALSE;

  BitStreamSize = InputSize - 4;

  // Allocate buffer for untransposed data
  pUntransposed = (UINT8 *) malloc (BitStreamSize);
  if (pUntransposed == NULL)
    return FALSE;

  // Inverse bit transposition
  BitTransposeInverse (pInput + 4, BitStreamSize, pUntransposed);

  // Initialize bit reader
  BitReaderInit (&Reader, pUntransposed, BitStreamSize);

  OutPos = 0;
  while (OutPos < OrigSize)
    {
      UINT8 Flag;
      UINT32 Value;

      // Read flag bit
      if (!BitReadBit (&Reader, &Flag))
        {
          free (pUntransposed);
          return FALSE;
        }

      if (Flag == 1)
        {
          // RAD-50 value
          if (!BitReadLEB128 (&Reader, &Value))
            {
              free (pUntransposed);
              return FALSE;
            }

          if (Value > RAD50_MAX || OutPos >= OrigSize)
            {
              free (pUntransposed);
              return FALSE;
            }

          pOutput[OutPos++] = (UINT8)Value;
        }
      else
        {
          // RLE sequence
          UINT32 RunLen;

          if (!BitReadLEB128 (&Reader, &RunLen))
            {
              free (pUntransposed);
              return FALSE;
            }

          if (OutPos + RunLen > OrigSize)
            {
              free (pUntransposed);
              return FALSE;
            }

          // Read each byte value
          for (UINT32 I = 0; I < RunLen; I++)
            {
              if (!BitReadLEB128 (&Reader, &Value))
                {
                  free (pUntransposed);
                  return FALSE;
                }

              if (Value > 255)
                {
                  free (pUntransposed);
                  return FALSE;
                }

              pOutput[OutPos++] = (UINT8)Value;
            }
        }
    }

  free (pUntransposed);

  *pDecSize = OrigSize;
  return TRUE;
}
