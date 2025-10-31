/** @file
  Zoo64 String Encoding Implementation

  Implements Zoo64 string compression using base-96 positional encoding
  with fixed-point arithmetic. This format uses base-96 to represent all
  printable ASCII characters (32-127).

  With base-96, we can encode up to 8 characters in 56 bits, leaving
  8 bits for length encoding.

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "types.h"
#include <string.h>
#include <stdlib.h>
#include "zoo64.h"

/**
  Base for Zoo64 encoding (printable ASCII count).
**/
#define ZOO64_BASE 96ULL

/**
  Maximum characters that fit in 56 bits with base-96.

  log_96(2^56) ≈ 8.5, so we can safely encode 8 characters.
  Using 56 bits leaves 8 bits for length encoding.
**/
#define ZOO64_MAX_CHARS 8

/**
  Pre-computed powers of 96 for encoding/decoding.

  Powers[i] = 96^i for i = 0 to 7
**/
static const UINT64 gPowersOf96[8] = {
  1ULL,                          // 96^0
  96ULL,                         // 96^1
  9216ULL,                       // 96^2
  884736ULL,                     // 96^3
  84934656ULL,                   // 96^4
  8153726976ULL,                 // 96^5
  782757789696ULL,               // 96^6
  75144747810816ULL              // 96^7
};

/**
  Map character to charset index.

  @param[in] C  ASCII character.
  @return Charset index (0-95).
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
  Encode string to Zoo64 format using base-96 positional encoding.

  Encodes up to 8 ASCII characters using base-96 positional notation.
  The result uses 56 bits for data and 8 bits for length.

  @param[in] pString  Null-terminated ASCII string (max 8 chars).
  @return 64-bit Zoo64 encoded value.
**/
UINT64
Zoo64Encode (
  IN const char  *pString
  )
{
  UINT64 Result;
  UINT32 Len;
  UINT32 I;

  if (pString == NULL || *pString == '\0')
    return 0;

  // Calculate string length (max 8 characters)
  Len = 0;
  while (pString[Len] != '\0' && Len < ZOO64_MAX_CHARS)
    Len++;

  // Encode using base-96 positional notation
  // result = c[0]*96^(Len-1) + c[1]*96^(Len-2) + ... + c[Len-1]*96^0
  Result = 0;
  for (I = 0; I < Len; I++)
    {
      UINT32 Idx = CharToIndex (pString[I]);
      Result += Idx * gPowersOf96[Len - 1 - I];
    }

  // Store length in upper 8 bits
  return Result | ((UINT64)Len << 56);
}

/**
  Decode Zoo64 to string with length limit.

  Decodes using base-96 positional notation.

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
  UINT64 Value;
  UINT32 EncLen;
  UINT32 I;
  char *pPtr;

  if (pString == NULL || Len == 0)
    return 0;

  // Extract length from upper 8 bits
  EncLen = (UINT32)((Enc >> 56) & 0xFF);
  Value = Enc & 0x00FFFFFFFFFFFFFFULL;

  if (EncLen == 0 || EncLen > ZOO64_MAX_CHARS || EncLen >= Len)
    {
      *pString = '\0';
      return 0;
    }

  // Decode each character using positional notation
  pPtr = pString;
  for (I = 0; I < EncLen; I++)
    {
      UINT64 Divisor = gPowersOf96[EncLen - 1 - I];
      UINT32 Idx = (UINT32)((Value / Divisor) % ZOO64_BASE);
      *pPtr++ = IndexToChar (Idx);
    }

  *pPtr = '\0';
  return EncLen;
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

  pStr = malloc (ZOO64_MAX_CHARS + 1);
  if (pStr == NULL)
    return NULL;

  Zoo64DecodeLen (Enc, ZOO64_MAX_CHARS + 1, pStr);
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
