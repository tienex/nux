/** @file
  RAD-50 (Radix-50) String Encoding

  Implements DEC RAD-50 string compression for encoding short ASCII strings
  into compact 64-bit integers. RAD-50 uses base-40 encoding supporting
  uppercase letters, digits, and limited special characters.

  Copyright (C) 2015-2023 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/**
  Encode character to RAD-50.

  Converts ASCII character to RAD-50 encoding (0-39):
  - ' ' (space) -> 0
  - 'A'-'Z', 'a'-'z' -> 1-26
  - '$' -> 27 (033 octal)
  - '.' -> 28 (034 octal)
  - '0'-'9' -> 30-39
  - Other -> 29 (035 octal, '_')

  @param[in] C  ASCII character to encode.

  @return RAD-50 value (0-39).
**/
static int
ChEnc (
  IN char  C
  )
{
  if (C >= 'A' && C <= 'Z')
    return C - 'A' + 1;
  if (C >= 'a' && C <= 'z')
    return C - 'a' + 1;
  if (C >= '0' && C <= '9')
    return C - '0' + 036;

  return C == ' ' ? 0 : C == '$' ? 033 : C == '.' ? 034 : 035;
}

/**
  Decode RAD-50 to character.

  Converts RAD-50 value (0-39) back to ASCII character:
  - 0 -> ' ' (space)
  - 1-26 -> 'A'-'Z'
  - 27 -> '$'
  - 28 -> '.'
  - 29 -> '_'
  - 30-39 -> '0'-'9'

  @param[in] S40  RAD-50 value (0-39).

  @return ASCII character.
**/
static char
ChDec (
  IN int  S40
  )
{
  if (!S40)
    return ' ';
  if (S40 <= 032)
    return 'A' + S40 - 1;
  if (S40 >= 036)
    return '0' + S40 - 036;

  return S40 == 033 ? '$' : S40 == 034 ? '.' : '_';
}

/**
  Encode string to RAD-50.

  Encodes up to 12 ASCII characters into a 64-bit RAD-50 integer.
  String is encoded with first character at high bits, padded with
  zeros if less than 12 characters.

  @param[in] pString  Null-terminated ASCII string (max 12 chars).

  @return 64-bit RAD-50 encoded value.
**/
UINT64
Squoze (
  IN char  *pString
  )
{
  char *pPtr = pString, *pMax = pPtr + 12;
  UINT64 Sqz = 0;

  while ((*pPtr != '\0') && (pPtr < pMax))
    {
      Sqz *= 40;
      Sqz += ChEnc (*pPtr++);
    }

  /* Pad, to get first character at top bits */
  while (pPtr++ < pMax)
    Sqz *= 40;

  return Sqz;
}

/**
  Decode RAD-50 to string with length limit.

  Decodes RAD-50 integer into ASCII string with maximum length limit.
  String is zero-padded if decoded length is less than buffer size.

  @param[in]  Enc      RAD-50 encoded value.
  @param[in]  Len      Maximum length of output buffer.
  @param[out] pString  Output buffer for decoded string.

  @return Number of characters decoded.
**/
size_t
UnsquozeLen (
  IN UINT64  Enc,
  IN size_t  Len,
  OUT char   *pString
  )
{
  UINT64 Cut = 1LL * 40 * 40 * 40 * 40 * 40 * 40 * 40 * 40 * 40 * 40 * 40;
  char *pPtr = pString;

  while (Enc && Len-- > 0)
    {
      *pPtr++ = ChDec ((Enc / Cut) % 40);
      Enc = (Enc % Cut) * 40;
    }
  if (Len)
    memset (pPtr, 0, Len);
  return pPtr - pString;
}

/**
  Decode RAD-50 to allocated string.

  Decodes RAD-50 integer into dynamically allocated null-terminated
  string (12 characters + null terminator).

  @param[in] Enc  RAD-50 encoded value.

  @return Pointer to allocated string, or NULL on allocation failure.
          Caller must free() the returned string.
**/
char *
Unsquoze (
  IN UINT64  Enc
  )
{
  char *pStr = malloc (13);
  if (pStr == NULL)
    return NULL;

  UnsquozeLen (Enc, 13, pStr);
  pStr[12] = '\0';
  return pStr;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use ChEnc instead **/
static int chenc (char c) {
  return ChEnc (c);
}

/** @deprecated Use ChDec instead **/
static char chdec (int s40) {
  return ChDec (s40);
}

/** @deprecated Use Squoze instead **/
uint64_t squoze (char *string) {
  return Squoze (string);
}

/** @deprecated Use UnsquozeLen instead **/
size_t unsquozelen (uint64_t enc, size_t len, char *string) {
  return UnsquozeLen (enc, len, string);
}

/** @deprecated Use Unsquoze instead **/
char *unsquoze (uint64_t enc) {
  return Unsquoze (enc);
}
