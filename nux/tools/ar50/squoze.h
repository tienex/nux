/** @file
  RAD-50 String Encoding Header

  Declares functions for DEC RAD-50 (Radix-50) string compression.

  Copyright (C) 2015-2023 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __squoze_h__
#define __squoze_h__

#include "types.h"

/**
  Encode string to RAD-50.

  Encodes up to 12 ASCII characters into a 64-bit RAD-50 integer.

  @param[in] pString  Null-terminated ASCII string (max 12 chars).

  @return 64-bit RAD-50 encoded value.
**/
UINT64 Squoze (char *pString);

/**
  Decode RAD-50 to string with length limit.

  Decodes RAD-50 integer into ASCII string with maximum length limit.

  @param[in]  Enc      RAD-50 encoded value.
  @param[in]  Len      Maximum length of output buffer.
  @param[out] pString  Output buffer for decoded string.

  @return Number of characters decoded.
**/
size_t UnsquozeLen (UINT64 Enc, size_t Len, char *pString);

/**
  Decode RAD-50 to allocated string.

  Decodes RAD-50 integer into dynamically allocated null-terminated string.

  @param[in] Enc  RAD-50 encoded value.

  @return Pointer to allocated string (caller must free), or NULL on failure.
**/
char *Unsquoze (UINT64 Enc);

//
// Legacy function names (for backward compatibility)
//

/** @deprecated Use Squoze instead **/
UINT64 squoze (char *string);

/** @deprecated Use UnsquozeLen instead **/
size_t unsquozelen (UINT64 enc, size_t len, char *string);

/** @deprecated Use Unsquoze instead **/
char *unsquoze (UINT64 enc);

#endif /* _SQUOZE_H_ */
