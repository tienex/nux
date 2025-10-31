/** @file
  RAD-50 with RLE and Bit Transposition Header

  Optimized encoding for RAD-50 compatible data using:
  - LEB128 variable-length encoding for RAD-50 values (0-39)
  - RLE for non-RAD-50 values
  - Bit transposition for better compression

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __rad50rle_h__
#define __rad50rle_h__

#include "types.h"
#include <stddef.h>

/**
  Encode data using RAD-50 + RLE + bit transposition.

  For each byte:
  - If value 0-39 (RAD-50 compatible): emit bit 1, then value in LEB128
  - If value 40-255: emit bit 0, then RLE encode sequence

  Then transpose bits: first byte = bit0 of 8 bytes, etc.

  @param[in]  pInput      Input data buffer.
  @param[in]  InputSize   Size of input data.
  @param[out] pOutput     Output buffer for encoded data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pEncSize    Actual encoded size.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
RAD50RLEEncode (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pEncSize
  );

/**
  Decode RAD-50 + RLE + bit transposition data.

  Reverses the encoding process.

  @param[in]  pInput      Encoded data buffer.
  @param[in]  InputSize   Size of encoded data.
  @param[out] pOutput     Output buffer for decoded data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pDecSize    Actual decoded size.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
RAD50RLEDecode (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pDecSize
  );

#endif /* __rad50rle_h__ */
