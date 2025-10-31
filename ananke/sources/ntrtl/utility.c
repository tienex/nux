/** @file
  NT RTL Utility Functions Implementation

  Miscellaneous utility functions for debugging and system information.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>
#include <ananke/ntrtl.h>

/* ---------------------------------------------------------------
 *  Assertion and Debugging
 * --------------------------------------------------------------- */

/**
  Raise an assertion failure.

  Prints assertion information and halts execution.

  @param[in] Expression  Expression that failed
  @param[in] File        Source file name
  @param[in] Line        Line number
**/
VOID
EFIAPI
RtlAssert (
  IN CONST CHAR8  *Expression,
  IN CONST CHAR8  *File,
  IN UINT32       Line
  )
{
  // In a real implementation, this would:
  // 1. Print to console/serial/log
  // 2. Possibly trigger debugger breakpoint
  // 3. Halt or panic

  // For now, we'll use a simple infinite loop
  // Actual implementation would use platform-specific debugging

  // Attempt to trigger a breakpoint if available
  ANX_CPU_BREAKPOINT();

  // Halt
  while (1) {
    ANX_CPU_PAUSE();
  }
}

/**
  Capture the current stack backtrace.

  Walks the stack and captures return addresses. This is a simplified
  implementation that may not work on all platforms.

  @param[in]  FramesToSkip    Number of frames to skip
  @param[in]  FramesToCapture Number of frames to capture
  @param[out] BackTrace       Buffer to store backtrace
  @param[out] BackTraceHash   Optional hash of backtrace

  @return Number of frames captured
**/
UINTN
EFIAPI
RtlCaptureStackBackTrace (
  IN  UINT32  FramesToSkip,
  IN  UINT32  FramesToCapture,
  OUT VOID    **BackTrace,
  OUT UINT32  *BackTraceHash OPTIONAL
  )
{
  UINTN Count = 0;
  UINT32 Hash = 0;

#if defined(__GNUC__) || defined(__clang__)
  // Use GCC/Clang builtin for stack walking
  VOID *FramePointer = __builtin_frame_address(0);
  UINTN FrameIndex = 0;

  while (FramePointer != NULL && Count < FramesToCapture) {
    if (FrameIndex >= FramesToSkip) {
      // Get return address
      VOID **Frame = (VOID **)FramePointer;
      VOID *ReturnAddress = *(Frame + 1);

      BackTrace[Count] = ReturnAddress;

      // Update hash
      Hash = ((Hash << 5) + Hash) + (UINT32)(UINTN)ReturnAddress;

      Count++;
    }

    // Move to next frame
    VOID **Frame = (VOID **)FramePointer;
    FramePointer = *Frame;
    FrameIndex++;

    // Sanity check: prevent infinite loops
    if (FrameIndex > 100) {
      break;
    }
  }
#else
  // For other compilers, we can't easily walk the stack
  // Return empty backtrace
  (VOID)FramesToSkip;
  (VOID)FramesToCapture;
#endif

  if (BackTraceHash != NULL) {
    *BackTraceHash = Hash;
  }

  return Count;
}

/* ---------------------------------------------------------------
 *  CRC32 Checksum
 * --------------------------------------------------------------- */

/**
  CRC32 lookup table.
**/
static UINT32 gCrc32Table[256] = {0};
static BOOLEAN gCrc32TableInitialized = FALSE;

/**
  Initialize CRC32 lookup table.
**/
static VOID
InitializeCrc32Table (
  VOID
  )
{
  UINT32 Polynomial = 0xEDB88320;
  UINT32 Index, Bit;

  for (Index = 0; Index < 256; Index++) {
    UINT32 Crc = Index;

    for (Bit = 0; Bit < 8; Bit++) {
      if (Crc & 1) {
        Crc = (Crc >> 1) ^ Polynomial;
      } else {
        Crc >>= 1;
      }
    }

    gCrc32Table[Index] = Crc;
  }

  gCrc32TableInitialized = TRUE;
}

/**
  Compute CRC32 checksum.

  Uses the standard CRC32 algorithm with polynomial 0xEDB88320.

  @param[in] InitialCrc  Initial CRC value
  @param[in] Buffer      Buffer to checksum
  @param[in] Length      Length of buffer

  @return CRC32 checksum
**/
UINT32
EFIAPI
RtlComputeCrc32 (
  IN UINT32       InitialCrc,
  IN CONST VOID   *Buffer,
  IN UINTN        Length
  )
{
  CONST UINT8 *Ptr = (CONST UINT8 *)Buffer;
  UINT32 Crc = InitialCrc;
  UINTN Index;

  // Initialize table if needed
  if (!gCrc32TableInitialized) {
    InitializeCrc32Table();
  }

  // Compute CRC
  for (Index = 0; Index < Length; Index++) {
    UINT8 TableIndex = (UINT8)((Crc ^ Ptr[Index]) & 0xFF);
    Crc = (Crc >> 8) ^ gCrc32Table[TableIndex];
  }

  return Crc;
}

/* ---------------------------------------------------------------
 *  Version Information
 * --------------------------------------------------------------- */

/**
  Get version of NT RTL library.

  @return Version number (major << 16 | minor)
**/
UINT32
EFIAPI
RtlGetVersion (
  VOID
  )
{
  return (RTL_VERSION_MAJOR << 16) | RTL_VERSION_MINOR;
}
