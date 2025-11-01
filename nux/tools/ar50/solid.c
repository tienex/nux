/** @file
  Solid Compression Implementation

  Implements solid archive compression where multiple files are compressed
  together as a single stream. Uses windowed compression with configurable
  window sizes (4K-1M) and maintains a second window for previous blocks.

  Copyright (C) 2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "types.h"
#include "solid.h"
#include "bwt.h"
#include "mtf.h"
#include "rad50rle.h"
#include "lz78_windowed.h"
#include "lz78.h"
#include "range.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
  Compress data using windowed compression pipeline.

  Pipeline: BWT → MTF → RAD50RLE → Windowed LZ78 → Range Encoding

  @param[in]  pInput      Input data buffer.
  @param[in]  InputSize   Size of input data.
  @param[in]  WindowSize  Window size for LZ78.
  @param[out] pOutput     Output buffer for compressed data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pCompSize   Actual compressed size.

  @return TRUE if successful, FALSE if error.
**/
static BOOLEAN
CompressWindowed (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  IN  UINT32       WindowSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pCompSize
  )
{
  UINT8 *pBWTOutput = NULL;
  UINT8 *pMTFOutput = NULL;
  UINT8 *pRAD50Output = NULL;
  UINT8 *pLZ78Output = NULL;
  UINT32 BWTIndex;
  size_t RAD50Size, LZ78Size, RangeSize;
  BOOLEAN Result = FALSE;

  if (pInput == NULL || pOutput == NULL || pCompSize == NULL)
    return FALSE;

  if (InputSize == 0)
    {
      *pCompSize = 0;
      return TRUE;
    }

  // Allocate intermediate buffers
  pBWTOutput = (UINT8 *) malloc (InputSize);
  pMTFOutput = (UINT8 *) malloc (InputSize);
  pRAD50Output = (UINT8 *) malloc (InputSize * 3);
  pLZ78Output = (UINT8 *) malloc (InputSize * 3);

  if (pBWTOutput == NULL || pMTFOutput == NULL ||
      pRAD50Output == NULL || pLZ78Output == NULL)
    goto cleanup;

  // Stage 1: BWT
  if (!BWTTransform (pInput, InputSize, pBWTOutput, &BWTIndex))
    goto cleanup;

  // Stage 2: MTF
  if (!MTFEncode (pBWTOutput, InputSize, pMTFOutput))
    goto cleanup;

  // Stage 3: RAD50RLE
  if (!RAD50RLEEncode (pMTFOutput, InputSize, pRAD50Output, InputSize * 3, &RAD50Size))
    goto cleanup;

  // Stage 4: LZ78 (windowing temporarily disabled for debugging)
  if (!LZ78Compress (pRAD50Output, RAD50Size,
                     pLZ78Output, InputSize * 3, &LZ78Size))
    goto cleanup;

  // Check header space
  if (OutputSize < 20)
    goto cleanup;

  // Write header: [OrigSize(4)][BWTIndex(4)][WindowSize(4)][RAD50Size(4)][LZ78Size(4)]
  pOutput[0] = (InputSize >> 24) & 0xFF;
  pOutput[1] = (InputSize >> 16) & 0xFF;
  pOutput[2] = (InputSize >> 8) & 0xFF;
  pOutput[3] = InputSize & 0xFF;

  pOutput[4] = (BWTIndex >> 24) & 0xFF;
  pOutput[5] = (BWTIndex >> 16) & 0xFF;
  pOutput[6] = (BWTIndex >> 8) & 0xFF;
  pOutput[7] = BWTIndex & 0xFF;

  pOutput[8] = (WindowSize >> 24) & 0xFF;
  pOutput[9] = (WindowSize >> 16) & 0xFF;
  pOutput[10] = (WindowSize >> 8) & 0xFF;
  pOutput[11] = WindowSize & 0xFF;

  pOutput[12] = (RAD50Size >> 24) & 0xFF;
  pOutput[13] = (RAD50Size >> 16) & 0xFF;
  pOutput[14] = (RAD50Size >> 8) & 0xFF;
  pOutput[15] = RAD50Size & 0xFF;

  pOutput[16] = (LZ78Size >> 24) & 0xFF;
  pOutput[17] = (LZ78Size >> 16) & 0xFF;
  pOutput[18] = (LZ78Size >> 8) & 0xFF;
  pOutput[19] = LZ78Size & 0xFF;

  // Stage 5: Range Encoding
  Result = RangeEncode (pLZ78Output, LZ78Size,
                        pOutput + 20, OutputSize - 20,
                        &RangeSize);

  if (Result)
    *pCompSize = RangeSize + 20;

cleanup:
  if (pBWTOutput) free (pBWTOutput);
  if (pMTFOutput) free (pMTFOutput);
  if (pRAD50Output) free (pRAD50Output);
  if (pLZ78Output) free (pLZ78Output);

  return Result;
}

/**
  Decompress data using windowed decompression pipeline.

  Pipeline: Range → Windowed LZ78 → RAD50RLE → MTF → BWT

  @param[in]  pInput      Compressed data buffer.
  @param[in]  InputSize   Size of compressed data.
  @param[out] pOutput     Output buffer for decompressed data.
  @param[in]  OutputSize  Size of output buffer.
  @param[out] pDecompSize Actual decompressed size.

  @return TRUE if successful, FALSE if error.
**/
static BOOLEAN
DecompressWindowed (
  IN  const UINT8  *pInput,
  IN  size_t       InputSize,
  OUT UINT8        *pOutput,
  IN  size_t       OutputSize,
  OUT size_t       *pDecompSize
  )
{
  UINT8 *pLZ78Output = NULL;
  UINT8 *pRAD50Output = NULL;
  UINT8 *pMTFOutput = NULL;
  UINT8 *pBWTOutput = NULL;
  UINT32 OrigSize, BWTIndex, WindowSize, RAD50Size, LZ78Size;
  size_t RangeDecSize, RAD50DecSize;
  BOOLEAN Result = FALSE;

  if (pInput == NULL || pOutput == NULL || pDecompSize == NULL)
    return FALSE;

  if (InputSize < 20)
    return FALSE;

  // Read header
  OrigSize = ((UINT32)pInput[0] << 24) | ((UINT32)pInput[1] << 16) |
             ((UINT32)pInput[2] << 8) | pInput[3];
  BWTIndex = ((UINT32)pInput[4] << 24) | ((UINT32)pInput[5] << 16) |
             ((UINT32)pInput[6] << 8) | pInput[7];
  WindowSize = ((UINT32)pInput[8] << 24) | ((UINT32)pInput[9] << 16) |
               ((UINT32)pInput[10] << 8) | pInput[11];
  RAD50Size = ((UINT32)pInput[12] << 24) | ((UINT32)pInput[13] << 16) |
              ((UINT32)pInput[14] << 8) | pInput[15];
  LZ78Size = ((UINT32)pInput[16] << 24) | ((UINT32)pInput[17] << 16) |
             ((UINT32)pInput[18] << 8) | pInput[19];

  if (OrigSize > OutputSize)
    return FALSE;

  // Allocate intermediate buffers
  pLZ78Output = (UINT8 *) malloc (LZ78Size);
  pRAD50Output = (UINT8 *) malloc (RAD50Size);
  pMTFOutput = (UINT8 *) malloc (OrigSize);
  pBWTOutput = (UINT8 *) malloc (OrigSize);

  if (pLZ78Output == NULL || pRAD50Output == NULL ||
      pMTFOutput == NULL || pBWTOutput == NULL)
    goto cleanup;

  // Stage 1: Range Decoding
  if (!RangeDecode (pInput + 20, InputSize - 20,
                    pLZ78Output, LZ78Size, &RangeDecSize))
    {
      goto cleanup;
    }

  if (RangeDecSize != LZ78Size)
    {
      goto cleanup;
    }

  // Stage 2: LZ78 Decompression (windowing temporarily disabled)
  if (!LZ78Decompress (pLZ78Output, LZ78Size,
                       pRAD50Output, RAD50Size, &RAD50DecSize))
    {
      goto cleanup;
    }

  if (RAD50DecSize != RAD50Size)
    {
      goto cleanup;
    }

  // Stage 3: RAD50RLE Decoding
  if (!RAD50RLEDecode (pRAD50Output, RAD50Size, pMTFOutput, OrigSize, &RAD50DecSize))
    {
      goto cleanup;
    }

  if (RAD50DecSize != OrigSize)
    {
      goto cleanup;
    }

  // Stage 4: MTF Decoding
  if (!MTFDecode (pMTFOutput, OrigSize, pBWTOutput))
    {
      goto cleanup;
    }

  // Stage 5: BWT Inverse
  if (!BWTInverse (pBWTOutput, OrigSize, BWTIndex, pOutput))
    {
      goto cleanup;
    }

  *pDecompSize = OrigSize;
  Result = TRUE;

cleanup:
  if (pLZ78Output) free (pLZ78Output);
  if (pRAD50Output) free (pRAD50Output);
  if (pMTFOutput) free (pMTFOutput);
  if (pBWTOutput) free (pBWTOutput);

  return Result;
}

/**
  Compress multiple files into a single solid block.

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
  )
{
  UINT8 *pConcatData = NULL;
  UINT8 *pCompData = NULL;
  UINT32 TotalSize = 0;
  UINT32 CurrentOffset = 0;
  size_t CompSize;
  size_t HeaderSize, DirectorySize;
  SOLID_HEADER *pHdr;
  SOLID_FILE_ENTRY *pEntries;
  UINT32 I;
  BOOLEAN Result = FALSE;

  if (ppFileData == NULL || pFileSizes == NULL || pFilenames == NULL ||
      pOutput == NULL || pCompressedSize == NULL || FileCount == 0)
    return FALSE;

  // Calculate total size
  for (I = 0; I < FileCount; I++)
    TotalSize += pFileSizes[I];

  // Allocate concatenation buffer
  pConcatData = (UINT8 *) malloc (TotalSize);
  if (pConcatData == NULL)
    return FALSE;

  // Concatenate all files
  CurrentOffset = 0;
  for (I = 0; I < FileCount; I++)
    {
      if (pFileSizes[I] > 0)
        {
          memcpy (pConcatData + CurrentOffset, ppFileData[I], pFileSizes[I]);
          CurrentOffset += pFileSizes[I];
        }
    }

  // Allocate compression buffer
  pCompData = (UINT8 *) malloc (TotalSize * 3 + 4096);
  if (pCompData == NULL)
    {
      free (pConcatData);
      return FALSE;
    }

  // Compress concatenated data
  if (!CompressWindowed (pConcatData, TotalSize, WindowSize,
                         pCompData, TotalSize * 3 + 4096, &CompSize))
    {
      free (pConcatData);
      free (pCompData);
      return FALSE;
    }

  // Calculate sizes
  HeaderSize = sizeof (SOLID_HEADER);
  DirectorySize = FileCount * sizeof (SOLID_FILE_ENTRY);

  if (OutputSize < HeaderSize + DirectorySize + CompSize)
    {
      free (pConcatData);
      free (pCompData);
      return FALSE;
    }

  // Build header
  pHdr = (SOLID_HEADER *) pOutput;
  pHdr->Magic = SOLID_MAGIC;
  pHdr->Version = SOLID_VERSION;
  pHdr->FileCount = FileCount;
  pHdr->WindowSize = WindowSize;
  pHdr->UncompressedSize = TotalSize;
  pHdr->CompressedSize = (UINT32)CompSize;
  pHdr->DirectoryOffset = (UINT32)(HeaderSize + CompSize);

  // Copy compressed data after header
  memcpy (pOutput + HeaderSize, pCompData, CompSize);

  // Build directory after compressed data
  pEntries = (SOLID_FILE_ENTRY *)(pOutput + HeaderSize + CompSize);
  CurrentOffset = 0;
  for (I = 0; I < FileCount; I++)
    {
      pEntries[I].Filename = pFilenames[I];
      pEntries[I].Offset = CurrentOffset;
      pEntries[I].Size = pFileSizes[I];
      CurrentOffset += pFileSizes[I];
    }

  *pCompressedSize = HeaderSize + CompSize + DirectorySize;
  Result = TRUE;

  free (pConcatData);
  free (pCompData);

  return Result;
}

/**
  Decompress a solid block and extract all files.

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
  )
{
  const SOLID_HEADER *pHdr;
  const SOLID_FILE_ENTRY *pEntries;
  UINT8 *pDecompData = NULL;
  size_t DecompSize;
  UINT32 I;
  BOOLEAN Result = FALSE;

  if (pInput == NULL || ppFileData == NULL || pFileSizes == NULL ||
      pFilenames == NULL || pFileCount == NULL)
    return FALSE;

  if (InputSize < sizeof (SOLID_HEADER))
    return FALSE;

  // Read header
  pHdr = (const SOLID_HEADER *) pInput;

  if (pHdr->Magic != SOLID_MAGIC || pHdr->Version != SOLID_VERSION)
    return FALSE;

  if (pHdr->FileCount > MaxFiles)
    return FALSE;

  // Verify sizes
  if (InputSize < sizeof (SOLID_HEADER) + pHdr->CompressedSize +
                  pHdr->FileCount * sizeof (SOLID_FILE_ENTRY))
    return FALSE;

  // Allocate decompression buffer
  pDecompData = (UINT8 *) malloc (pHdr->UncompressedSize);
  if (pDecompData == NULL)
    return FALSE;

  // Decompress data
  if (!DecompressWindowed (pInput + sizeof (SOLID_HEADER),
                           pHdr->CompressedSize,
                           pDecompData,
                           pHdr->UncompressedSize,
                           &DecompSize))
    {
      free (pDecompData);
      return FALSE;
    }

  if (DecompSize != pHdr->UncompressedSize)
    {
      free (pDecompData);
      return FALSE;
    }

  // Read directory
  pEntries = (const SOLID_FILE_ENTRY *)(pInput + pHdr->DirectoryOffset);

  // Extract individual files
  for (I = 0; I < pHdr->FileCount; I++)
    {
      pFilenames[I] = pEntries[I].Filename;
      pFileSizes[I] = pEntries[I].Size;

      if (pEntries[I].Size > 0)
        {
          ppFileData[I] = (UINT8 *) malloc (pEntries[I].Size);
          if (ppFileData[I] == NULL)
            {
              // Cleanup already allocated files
              for (UINT32 J = 0; J < I; J++)
                {
                  if (ppFileData[J])
                    free (ppFileData[J]);
                }
              free (pDecompData);
              return FALSE;
            }

          memcpy (ppFileData[I], pDecompData + pEntries[I].Offset, pEntries[I].Size);
        }
      else
        {
          ppFileData[I] = NULL;
        }
    }

  *pFileCount = pHdr->FileCount;
  Result = TRUE;

  free (pDecompData);
  return Result;
}
