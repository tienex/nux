/** @file
  Zoo64 String Encoding Header

  Declares functions for Zoo64 string compression using fixed-point
  range arithmetic adaptive encoding. This format expands zoo format
  to 64-bit with adaptive fixed-point arithmetic for efficient
  filename encoding.

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __zoo64_h__
#define __zoo64_h__

#include "types.h"
#include <stddef.h>

// Fixed-point configuration for range arithmetic
#define ZOO64_FIXPOINT_BITS 32
#define ZOO64_FIXPOINT_ONE (1ULL << ZOO64_FIXPOINT_BITS)
#define ZOO64_FIXPOINT_MASK (ZOO64_FIXPOINT_ONE - 1)

// Maximum filename length supported
#define ZOO64_MAX_LENGTH 16

// Character set size for adaptive encoding
#define ZOO64_CHARSET_SIZE 96

/**
  Encode string to Zoo64 format.

  Uses fixed-point range arithmetic adaptive encoding to compress
  up to 16 ASCII characters into a 64-bit integer. Supports full
  printable ASCII character set (32-127).

  @param[in] pString  Null-terminated ASCII string (max 16 chars).

  @return 64-bit Zoo64 encoded value.
**/
UINT64 Zoo64Encode (const char *pString);

/**
  Decode Zoo64 to string with length limit.

  Decodes Zoo64 integer into ASCII string using fixed-point range
  arithmetic with adaptive decoding.

  @param[in]  Enc      Zoo64 encoded value.
  @param[in]  Len      Maximum length of output buffer.
  @param[out] pString  Output buffer for decoded string.

  @return Number of characters decoded.
**/
size_t Zoo64DecodeLen (UINT64 Enc, size_t Len, char *pString);

/**
  Decode Zoo64 to allocated string.

  Decodes Zoo64 integer into dynamically allocated null-terminated string.

  @param[in] Enc  Zoo64 encoded value.

  @return Pointer to allocated string (caller must free), or NULL on failure.
**/
char *Zoo64Decode (UINT64 Enc);

//
// Legacy function names (for backward compatibility)
//

/** @deprecated Use Zoo64Encode instead **/
UINT64 zoo64_encode (const char *string);

/** @deprecated Use Zoo64DecodeLen instead **/
size_t zoo64_decode_len (UINT64 enc, size_t len, char *string);

/** @deprecated Use Zoo64Decode instead **/
char *zoo64_decode (UINT64 enc);

#endif /* __zoo64_h__ */
