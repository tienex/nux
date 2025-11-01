/** @file
  LZ78 Dictionary Compression Header

  LZ78 is a dictionary-based compression algorithm that builds a dictionary
  of repeated patterns during encoding. It provides good compression for
  data with repeated sequences.

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __lz78_h__
#define __lz78_h__

#include "types.h"
#include <stddef.h>

/**
  LZ78 dictionary entry.
**/
typedef struct _LZ78_ENTRY {
  UINT16 Prefix;     // Previous dictionary entry index (0 = none)
  UINT8  Character;  // Character added to prefix
} LZ78_ENTRY;

/**
  Compress data using LZ78 algorithm.

  @param[in]  pInput      Input data buffer.
  @param[in]  InputSize   Size of input data.
  @param[out] pOutput     Output buffer for compressed data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pCompSize   Actual compressed size.

  @return TRUE if successful, FALSE if output buffer too small.
**/
BOOLEAN
LZ78Compress (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pCompSize
  );

/**
  Decompress LZ78 compressed data.

  @param[in]  pInput      Compressed data buffer.
  @param[in]  InputSize   Size of compressed data.
  @param[out] pOutput     Output buffer for decompressed data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pDecompSize Actual decompressed size.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
LZ78Decompress (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pDecompSize
  );

#endif /* __lz78_h__ */
