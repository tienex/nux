/** @file
  Solid Compression Support for AR64

  Solid compression compresses multiple files together as a single stream,
  allowing the compression algorithm to find repetitions across file boundaries.
  Includes configurable windowing (4K-1M) and second window for previous blocks.

  Copyright (C) 2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef _SOLID_H_
#define _SOLID_H_

#include "types.h"

//
// Window size options (must be power of 2)
//
#define WINDOW_4K    (4 * 1024)
#define WINDOW_8K    (8 * 1024)
#define WINDOW_16K   (16 * 1024)
#define WINDOW_32K   (32 * 1024)
#define WINDOW_64K   (64 * 1024)
#define WINDOW_128K  (128 * 1024)
#define WINDOW_256K  (256 * 1024)
#define WINDOW_512K  (512 * 1024)
#define WINDOW_1M    (1024 * 1024)

#define DEFAULT_WINDOW_SIZE WINDOW_64K

//
// Solid archive structures
//

/**
  File entry in solid archive directory.
  Maps filename to position in uncompressed stream.
**/
typedef struct _SOLID_FILE_ENTRY
{
  UINT64 Filename;      // Zoo64 encoded filename
  UINT32 Offset;        // Offset in uncompressed stream
  UINT32 Size;          // Size in uncompressed stream
} __attribute__((packed)) SOLID_FILE_ENTRY;

/**
  Solid archive header.
  Contains metadata for entire solid block.
**/
typedef struct _SOLID_HEADER
{
  UINT64 Magic;              // AR64 solid magic
  UINT32 Version;            // Format version
  UINT32 FileCount;          // Number of files in archive
  UINT32 WindowSize;         // Compression window size
  UINT32 UncompressedSize;   // Total uncompressed size
  UINT32 CompressedSize;     // Total compressed size
  UINT32 DirectoryOffset;    // Offset to file directory
} __attribute__((packed)) SOLID_HEADER;

#define SOLID_MAGIC 0x534F4C4944415236ULL  // "SOLIDAR6"
#define SOLID_VERSION 1

/**
  Compress multiple files into a single solid block.

  Concatenates all input files and compresses them together as a single stream.
  Uses windowed compression for better memory efficiency and locality.

  @param[in]  ppFileData      Array of pointers to file data buffers
  @param[in]  pFileSizes      Array of file sizes
  @param[in]  pFilenames      Array of Zoo64-encoded filenames
  @param[in]  FileCount       Number of files
  @param[in]  WindowSize      Compression window size (4K-1M)
  @param[out] pOutput         Output buffer for compressed data
  @param[in]  OutputSize      Size of output buffer
  @param[out] pCompressedSize Size of compressed data

  @retval TRUE   Compression succeeded
  @retval FALSE  Compression failed
**/
BOOLEAN
SolidCompress (
  IN  const UINT8  **ppFileData,
  IN  const UINT32 *pFileSizes,
  IN  const UINT64 *pFilenames,
  IN  UINT32       FileCount,
  IN  UINT32       WindowSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pCompressedSize
  );

/**
  Decompress a solid block and extract all files.

  Decompresses the solid stream and splits it back into individual files
  using the directory information.

  @param[in]  pInput          Compressed solid block
  @param[in]  InputSize       Size of compressed data
  @param[out] ppFileData      Array of pointers to store extracted file data
  @param[out] pFileSizes      Array to store extracted file sizes
  @param[out] pFilenames      Array to store Zoo64-encoded filenames
  @param[out] pFileCount      Number of files extracted
  @param[in]  MaxFiles        Maximum number of files that can be stored

  @retval TRUE   Decompression succeeded
  @retval FALSE  Decompression failed
**/
BOOLEAN
SolidDecompress (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        **ppFileData,
  OUT UINT32       *pFileSizes,
  OUT UINT64       *pFilenames,
  OUT UINT32       *pFileCount,
  IN  UINT32       MaxFiles
  );

#endif // _SOLID_H_
