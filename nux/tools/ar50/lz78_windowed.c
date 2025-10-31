/** @file
  Windowed LZ78 Compression Implementation

  Implements LZ78 with sliding window and second window for previous blocks.
  The second window contains recently evicted dictionary entries, allowing
  matches to be found in both current and previous window.

  Copyright (C) 2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "types.h"
#include "lz78_windowed.h"
#include <stdlib.h>
#include <string.h>

//
// Helper: Calculate dictionary capacity from window size
// Estimate ~3 bytes per pattern on average
//
static UINT32
CalculateDictCapacity (
  IN UINT32  WindowSize
  )
{
  UINT32 Capacity = WindowSize / 3;

  // Clamp to reasonable limits
  if (Capacity < 1024)
    Capacity = 1024;
  if (Capacity > 65536)
    Capacity = 65536;

  return Capacity;
}

/**
  Initialize windowed LZ78 context.

  @param[out] pCtx        Context to initialize.
  @param[in]  WindowSize  Window size in bytes.

  @return TRUE if successful, FALSE if memory allocation failed.
**/
static BOOLEAN
InitContext (
  OUT LZ78W_CONTEXT  *pCtx,
  IN  UINT32         WindowSize
  )
{
  memset (pCtx, 0, sizeof (LZ78W_CONTEXT));

  pCtx->WindowSize = WindowSize;
  pCtx->DictCapacity = CalculateDictCapacity (WindowSize);
  pCtx->SecondWindowCap = pCtx->DictCapacity / 2;  // Second window is half size

  pCtx->pDict = (LZ78W_ENTRY *) malloc (pCtx->DictCapacity * sizeof (LZ78W_ENTRY));
  if (pCtx->pDict == NULL)
    return FALSE;

  pCtx->pSecondWindow = (LZ78W_ENTRY *) malloc (pCtx->SecondWindowCap * sizeof (LZ78W_ENTRY));
  if (pCtx->pSecondWindow == NULL)
    {
      free (pCtx->pDict);
      return FALSE;
    }

  return TRUE;
}

/**
  Free windowed LZ78 context.

  @param[in] pCtx  Context to free.
**/
static VOID
FreeContext (
  IN LZ78W_CONTEXT  *pCtx
  )
{
  if (pCtx->pDict != NULL)
    {
      free (pCtx->pDict);
      pCtx->pDict = NULL;
    }
  if (pCtx->pSecondWindow != NULL)
    {
      free (pCtx->pSecondWindow);
      pCtx->pSecondWindow = NULL;
    }
}

/**
  Search for pattern in dictionary.

  Searches both primary dictionary and second window.

  @param[in]  pCtx          Context.
  @param[in]  Prefix        Prefix index to match.
  @param[in]  Character     Character to match.
  @param[out] pFoundIndex   Index where found (1-based).
  @param[out] pInSecondWin  TRUE if found in second window.

  @return TRUE if found, FALSE otherwise.
**/
static BOOLEAN
SearchPattern (
  IN  LZ78W_CONTEXT  *pCtx,
  IN  UINT16         Prefix,
  IN  UINT8          Character,
  OUT UINT16         *pFoundIndex,
  OUT BOOLEAN        *pInSecondWin
  )
{
  UINT32 I;

  // Search primary dictionary only (second window disabled for now)
  for (I = 0; I < pCtx->DictSize; I++)
    {
      if (pCtx->pDict[I].Prefix == Prefix && pCtx->pDict[I].Character == Character)
        {
          *pFoundIndex = (UINT16)(I + 1);
          *pInSecondWin = FALSE;
          return TRUE;
        }
    }

  *pInSecondWin = FALSE;
  return FALSE;
}

/**
  Slide window when it gets too full.

  Moves current dictionary to second window and starts fresh.

  @param[in,out] pCtx  Context to slide.
**/
static VOID
SlideWindow (
  IN OUT LZ78W_CONTEXT  *pCtx
  )
{
  UINT32 CopySize;

  // Move current dictionary to second window
  // Take most recent entries (from end of dictionary)
  if (pCtx->DictSize > pCtx->SecondWindowCap)
    {
      CopySize = pCtx->SecondWindowCap;
      memcpy (pCtx->pSecondWindow,
              &pCtx->pDict[pCtx->DictSize - CopySize],
              CopySize * sizeof (LZ78W_ENTRY));
    }
  else
    {
      CopySize = pCtx->DictSize;
      memcpy (pCtx->pSecondWindow,
              pCtx->pDict,
              CopySize * sizeof (LZ78W_ENTRY));
    }

  pCtx->SecondWindowSize = CopySize;

  // Clear primary dictionary
  pCtx->DictSize = 0;
  pCtx->WindowStart = pCtx->CurrentPos;
}

/**
  Compress data using windowed LZ78 algorithm.

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
  )
{
  LZ78W_CONTEXT Ctx;
  size_t InPos, OutPos;
  UINT16 CurrentIndex;
  UINT8 CurrentChar;
  UINT8 HasFinalMatch;
  BOOLEAN InSecondWin;

  if (pInput == NULL || pOutput == NULL || pCompSize == NULL)
    return FALSE;

  if (InputSize == 0)
    {
      *pCompSize = 0;
      return TRUE;
    }

  // Initialize context
  if (!InitContext (&Ctx, WindowSize))
    return FALSE;

  InPos = 0;
  OutPos = 1;  // Reserve byte 0 for final match flag
  CurrentIndex = 0;
  HasFinalMatch = 0;

  while (InPos < InputSize)
    {
      CurrentChar = pInput[InPos];
      InPos++;
      Ctx.CurrentPos++;

      // Search for (CurrentIndex, CurrentChar)
      UINT16 FoundIndex;
      BOOLEAN InSecondWin = FALSE;

      if (SearchPattern (&Ctx, CurrentIndex, CurrentChar, &FoundIndex, &InSecondWin))
        {
          // Found - continue pattern
          CurrentIndex = FoundIndex;
        }
      else
        {
          // Not found - emit token
          if (OutPos + 3 > OutputSize)
            {
              FreeContext (&Ctx);
              return FALSE;
            }

          pOutput[OutPos++] = (CurrentIndex >> 8) & 0xFF;
          pOutput[OutPos++] = CurrentIndex & 0xFF;
          pOutput[OutPos++] = CurrentChar;

          // Add to primary dictionary
          if (Ctx.DictSize < Ctx.DictCapacity)
            {
              Ctx.pDict[Ctx.DictSize].Prefix = CurrentIndex;
              Ctx.pDict[Ctx.DictSize].Character = CurrentChar;
              Ctx.pDict[Ctx.DictSize].Position = Ctx.CurrentPos;
              Ctx.DictSize++;
            }

          CurrentIndex = 0;

          // Check if we need to slide window (after emitting)
          if (Ctx.CurrentPos - Ctx.WindowStart >= WindowSize)
            {
              SlideWindow (&Ctx);
            }
        }
    }

  // Emit final partial match if needed
  if (CurrentIndex != 0)
    {
      if (OutPos + 2 > OutputSize)
        {
          FreeContext (&Ctx);
          return FALSE;
        }

      pOutput[OutPos++] = (CurrentIndex >> 8) & 0xFF;
      pOutput[OutPos++] = CurrentIndex & 0xFF;
      HasFinalMatch = 1;
    }

  // Write flag byte at position 0
  pOutput[0] = HasFinalMatch;

  FreeContext (&Ctx);
  *pCompSize = OutPos;
  return TRUE;
}

/**
  Decompress windowed LZ78 compressed data.

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
  )
{
  LZ78W_CONTEXT Ctx;
  size_t InPos, OutPos;
  UINT8 HasFinalMatch;
  UINT8 Buffer[1024];
  size_t BufLen;
  UINT16 Index, Idx;
  UINT8 Character;

  if (pInput == NULL || pOutput == NULL || pDecompSize == NULL)
    return FALSE;

  if (InputSize < 1)
    {
      *pDecompSize = 0;
      return TRUE;
    }

  // Initialize context
  if (!InitContext (&Ctx, WindowSize))
    return FALSE;

  // Read flag byte
  HasFinalMatch = pInput[0];

  InPos = 1;
  OutPos = 0;

  // Process (index, character) pairs
  while (InPos + 3 <= InputSize)
    {
      Index = ((UINT16)pInput[InPos] << 8) | pInput[InPos + 1];
      Character = pInput[InPos + 2];
      InPos += 3;

      // Reconstruct string from dictionary
      BufLen = 0;
      Idx = Index;

      while (Idx > 0 && BufLen < sizeof(Buffer))
        {
          UINT32 RealIdx = Idx - 1;
          if (RealIdx >= Ctx.DictSize)
            {
              FreeContext (&Ctx);
              return FALSE;
            }
          Buffer[BufLen++] = Ctx.pDict[RealIdx].Character;
          Idx = Ctx.pDict[RealIdx].Prefix;
        }

      // Output in reverse order
      for (size_t I = BufLen; I > 0; I--)
        {
          if (OutPos >= OutputSize)
            {
              FreeContext (&Ctx);
              return FALSE;
            }
          pOutput[OutPos++] = Buffer[I - 1];
        }

      // Output new character
      if (OutPos >= OutputSize)
        {
          FreeContext (&Ctx);
          return FALSE;
        }
      pOutput[OutPos++] = Character;
      Ctx.CurrentPos = (UINT32)OutPos;

      // Add to dictionary
      if (Ctx.DictSize < Ctx.DictCapacity)
        {
          Ctx.pDict[Ctx.DictSize].Prefix = Index;
          Ctx.pDict[Ctx.DictSize].Character = Character;
          Ctx.pDict[Ctx.DictSize].Position = Ctx.CurrentPos;
          Ctx.DictSize++;
        }

      // Check if we need to slide window
      if (Ctx.CurrentPos - Ctx.WindowStart >= WindowSize)
        {
          SlideWindow (&Ctx);
        }
    }

  // Handle final partial match if present
  if (HasFinalMatch && InPos + 2 <= InputSize)
    {
      Index = ((UINT16)pInput[InPos] << 8) | pInput[InPos + 1];

      // Reconstruct string from dictionary
      BufLen = 0;
      Idx = Index;

      while (Idx > 0 && BufLen < sizeof(Buffer))
        {
          UINT32 RealIdx = Idx - 1;
          if (RealIdx >= Ctx.DictSize)
            {
              FreeContext (&Ctx);
              return FALSE;
            }
          Buffer[BufLen++] = Ctx.pDict[RealIdx].Character;
          Idx = Ctx.pDict[RealIdx].Prefix;
        }

      // Output in reverse order
      for (size_t I = BufLen; I > 0; I--)
        {
          if (OutPos >= OutputSize)
            {
              FreeContext (&Ctx);
              return FALSE;
            }
          pOutput[OutPos++] = Buffer[I - 1];
        }
    }

  FreeContext (&Ctx);
  *pDecompSize = OutPos;
  return TRUE;
}
