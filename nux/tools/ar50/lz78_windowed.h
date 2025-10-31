/** @file
  Windowed LZ78 Compression for Solid Archives

  Enhanced LZ78 implementation with sliding window support and second window
  for previous block searching. Supports window sizes from 4K to 1M.

  Copyright (C) 2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef _LZ78_WINDOWED_H_
#define _LZ78_WINDOWED_H_

#include "types.h"

/**
  LZ78 dictionary entry for windowed compression.
  Stores prefix index, character, and position for window management.
**/
typedef struct _LZ78W_ENTRY
{
  UINT16 Prefix;       // Index of prefix pattern (0 = no prefix)
  UINT8  Character;    // Character added to prefix
  UINT32 Position;     // Position in input stream (for window management)
} LZ78W_ENTRY;

/**
  Windowed LZ78 compression context.
  Maintains primary dictionary and second window for evicted entries.
**/
typedef struct _LZ78W_CONTEXT
{
  LZ78W_ENTRY *pDict;           // Primary dictionary (current window)
  UINT32      DictSize;         // Current dictionary size
  UINT32      DictCapacity;     // Maximum dictionary size

  LZ78W_ENTRY *pSecondWindow;   // Second window (evicted entries)
  UINT32      SecondWindowSize; // Size of second window
  UINT32      SecondWindowCap;  // Capacity of second window

  UINT32      WindowSize;       // Window size in bytes
  UINT32      CurrentPos;       // Current position in input
  UINT32      WindowStart;      // Start position of current window
} LZ78W_CONTEXT;

/**
  Compress data using windowed LZ78 algorithm.

  Uses a sliding window to limit memory usage and improve compression locality.
  Maintains a second window of recently evicted entries for better matches.

  @param[in]  pInput      Input data buffer.
  @param[in]  InputSize   Size of input data.
  @param[in]  WindowSize  Window size in bytes (4K-1M).
  @param[out] pOutput     Output buffer for compressed data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pCompSize   Actual compressed size.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
LZ78WindowedCompress (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  IN  UINT32       WindowSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pCompSize
  );

/**
  Decompress windowed LZ78 compressed data.

  Decompresses data that was compressed with windowed LZ78.
  Uses same windowing strategy as compression.

  @param[in]  pInput      Compressed data buffer.
  @param[in]  InputSize   Size of compressed data.
  @param[in]  WindowSize  Window size used during compression.
  @param[out] pOutput     Output buffer for decompressed data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pDecompSize Actual decompressed size.

  @return TRUE if successful, FALSE if error.
**/
BOOLEAN
LZ78WindowedDecompress (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  IN  UINT32       WindowSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pDecompSize
  );

#endif // _LZ78_WINDOWED_H_
