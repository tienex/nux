/** @file
  Range Arithmetic Encoder/Decoder Header

  Implements range arithmetic coding for data compression. Range coding
  provides near-optimal compression based on symbol frequencies.

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __range_h__
#define __range_h__

#include "types.h"
#include <stddef.h>

/**
  Compress data using range arithmetic encoding.

  Uses adaptive frequency-based encoding to compress data.

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
  );

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
  );

#endif /* __range_h__ */
