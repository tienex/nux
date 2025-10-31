/** @file
  Zoo64 String Encoding Implementation

  Implements Zoo64 string compression using fixed-point range arithmetic
  adaptive encoding. This format uses a 64-bit integer to encode filenames
  with adaptive range coding and fixed-point arithmetic for precision.

  Range arithmetic encoding provides better compression by using adaptive
  probability ranges for each character. Fixed-point arithmetic avoids
  floating-point operations while maintaining precision.

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "types.h"
#include <string.h>
#include <stdlib.h>
#include "zoo64.h"

/**
  Character frequency table for adaptive encoding.

  Frequencies for all 96 printable ASCII characters (space through tilde).
  Values are normalized so the sum equals 2^16 = 65536 for fixed-point arithmetic.
**/
static const UINT16 gCharFreq[ZOO64_CHARSET_SIZE] = {
  // Space (ASCII 32)
  800,
  // Punctuation (ASCII 33-47): ! " # $ % & ' ( ) * + , - . /
  100, 50, 80, 150, 50, 50, 80, 150, 150, 150, 80, 80,
  1200, 1000, 1500,
  // Digits (ASCII 48-57): 0-9
  900, 900, 850, 850, 800, 800, 750, 750, 700, 700,
  // More punctuation (ASCII 58-64): : ; < = > ? @
  300, 50, 50, 80, 50, 80, 80,
  // Uppercase (ASCII 65-90): A-Z
  500, 400, 450, 400, 500, 350, 350, 400, 450, 300, 250, 350,
  400, 400, 450, 400, 250, 450, 450, 450, 350, 300, 300, 250,
  300, 250,
  // Brackets (ASCII 91-96): [ \ ] ^ _ `
  150, 80, 150, 80, 1200, 80,
  // Lowercase (ASCII 97-122): a-z
  1300, 750, 850, 750, 1500, 650, 650, 750, 1100, 400, 500, 850,
  750, 1000, 1100, 750, 300, 1000, 1000, 1100, 650, 500, 500, 400,
  500, 300,
  // Final punctuation (ASCII 123-127): { | } ~ DEL
  80, 80, 80, 50, 50
};

/**
  Build cumulative frequency table.

  Constructs cumulative frequency distribution for range encoding.

  @param[out] pCumFreq  Output cumulative frequency table (size 97).
  @param[out] pTotal    Total frequency sum.
**/
static VOID
BuildCumulativeFreq (
  OUT UINT32  *pCumFreq,
  OUT UINT32  *pTotal
  )
{
  UINT32 Cum;
  size_t I;

  Cum = 0;
  for (I = 0; I < ZOO64_CHARSET_SIZE; I++)
    {
      pCumFreq[I] = Cum;
      Cum += gCharFreq[I];
    }
  pCumFreq[ZOO64_CHARSET_SIZE] = Cum;
  *pTotal = Cum;
}

/**
  Map character to charset index.

  @param[in] C  ASCII character.
  @return Charset index (0-95), or 0 for invalid characters.
**/
static inline UINT32
CharToIndex (
  IN char  C
  )
{
  if (C < 32 || C > 127)
    return 0;
  return (UINT32)(C - 32);
}

/**
  Map charset index to character.

  @param[in] Idx  Charset index.
  @return ASCII character (32-127).
**/
static inline char
IndexToChar (
  IN UINT32  Idx
  )
{
  if (Idx >= ZOO64_CHARSET_SIZE)
    return ' ';
  return (char)(Idx + 32);
}

/**
  Encode string to Zoo64 format using range arithmetic.

  Uses adaptive range coding with fixed-point arithmetic. Each character
  narrows the encoding range based on its frequency. The algorithm encodes
  up to 8 characters to avoid overflow.

  @param[in] pString  Null-terminated ASCII string (max 8 chars).
  @return 64-bit Zoo64 encoded value.
**/
UINT64
Zoo64Encode (
  IN const char  *pString
  )
{
  UINT32 CumFreq[ZOO64_CHARSET_SIZE + 1];
  UINT32 Total;
  UINT64 Low, High, Range;
  const char *pPtr;
  UINT32 Idx;
  UINT32 CharLow, CharHigh;
  size_t Len;

  if (pString == NULL || *pString == '\0')
    return 0;

  BuildCumulativeFreq (CumFreq, &Total);

  // Initialize range [0, 2^56) to leave room for length encoding
  Low = 0;
  High = 0x00FFFFFFFFFFFFFFULL;  // 56 bits of range

  pPtr = pString;
  Len = 0;

  // Encode up to 8 characters
  while (*pPtr != '\0' && Len < 8)
    {
      Idx = CharToIndex (*pPtr);
      Range = High - Low + 1;

      CharLow = CumFreq[Idx];
      CharHigh = CumFreq[Idx + 1];

      // Update range using fixed-point arithmetic
      // New range = [Low + Range * CharLow / Total, Low + Range * CharHigh / Total)
      High = Low + ((Range * (UINT64)CharHigh) / Total) - 1;
      Low = Low + ((Range * (UINT64)CharLow) / Total);

      pPtr++;
      Len++;
    }

  // Encode length in upper 8 bits
  return (Low & 0x00FFFFFFFFFFFFFFULL) | ((UINT64)Len << 56);
}

/**
  Decode Zoo64 to string with length limit.

  Decodes using adaptive range arithmetic. Extracts length from upper bits
  and decodes each character by finding which range contains the value.

  @param[in]  Enc      Zoo64 encoded value.
  @param[in]  Len      Maximum output buffer length.
  @param[out] pString  Output buffer.
  @return Number of characters decoded.
**/
size_t
Zoo64DecodeLen (
  IN UINT64  Enc,
  IN size_t  Len,
  OUT char   *pString
  )
{
  UINT32 CumFreq[ZOO64_CHARSET_SIZE + 1];
  UINT32 Total;
  UINT64 Low, High, Range, Value;
  char *pPtr;
  UINT32 EncLen, I, Idx;
  UINT32 CharLow, CharHigh;
  UINT64 Scaled;

  if (pString == NULL || Len == 0)
    return 0;

  BuildCumulativeFreq (CumFreq, &Total);

  // Extract length from upper 8 bits
  EncLen = (UINT32)((Enc >> 56) & 0xFF);
  Value = Enc & 0x00FFFFFFFFFFFFFFULL;

  if (EncLen == 0 || EncLen > 8)
    {
      *pString = '\0';
      return 0;
    }

  // Initialize range
  Low = 0;
  High = 0x00FFFFFFFFFFFFFFULL;

  pPtr = pString;

  // Decode each character
  for (I = 0; I < EncLen && I < Len - 1; I++)
    {
      Range = High - Low + 1;

      // Find character whose range contains Value
      // Scaled = (Value - Low) * Total / Range
      Scaled = ((Value - Low) * Total) / Range;

      // Binary search for character
      for (Idx = 0; Idx < ZOO64_CHARSET_SIZE; Idx++)
        {
          if (CumFreq[Idx] <= Scaled && Scaled < CumFreq[Idx + 1])
            {
              *pPtr++ = IndexToChar (Idx);

              CharLow = CumFreq[Idx];
              CharHigh = CumFreq[Idx + 1];

              // Update range
              High = Low + ((Range * (UINT64)CharHigh) / Total) - 1;
              Low = Low + ((Range * (UINT64)CharLow) / Total);

              break;
            }
        }

      if (Idx >= ZOO64_CHARSET_SIZE)
        break;  // Decoding error
    }

  *pPtr = '\0';
  return pPtr - pString;
}

/**
  Decode Zoo64 to allocated string.

  @param[in] Enc  Zoo64 encoded value.
  @return Pointer to allocated string (caller must free), or NULL on failure.
**/
char *
Zoo64Decode (
  IN UINT64  Enc
  )
{
  char *pStr;

  pStr = malloc (9);  // Max 8 chars + null terminator
  if (pStr == NULL)
    return NULL;

  Zoo64DecodeLen (Enc, 9, pStr);
  return pStr;
}

//
// Legacy Function Wrappers
//

UINT64
zoo64_encode (
  const char  *string
  )
{
  return Zoo64Encode (string);
}

size_t
zoo64_decode_len (
  UINT64  enc,
  size_t  len,
  char    *string
  )
{
  return Zoo64DecodeLen (enc, len, string);
}

char *
zoo64_decode (
  UINT64  enc
  )
{
  return Zoo64Decode (enc);
}
