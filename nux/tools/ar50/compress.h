/** @file
  Compression Pipeline Header

  Provides a complete compression pipeline using:
  1. LZ78 - Dictionary compression (optional preprocessing)
  2. BWT - Burrows-Wheeler Transform
  3. MTF - Move-To-Front encoding
  4. Range - Range arithmetic encoding

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __compress_h__
#define __compress_h__

#include "types.h"
#include <stddef.h>

/**
  Compress data using full pipeline (BWT + MTF + RAD50RLE + Windowed LZ78 + Range).

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
  );

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
  );

#endif /* __compress_h__ */
