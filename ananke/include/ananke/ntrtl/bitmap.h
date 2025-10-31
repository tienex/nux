/** @file
  NT RTL Bitmap Functions

  Bitmap manipulation functions following Windows NT RTL conventions.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __ANANKE_NTRTL_BITMAP_H__
#define __ANANKE_NTRTL_BITMAP_H__

/* ---------------------------------------------------------------
 *  Bitmap Initialization
 * --------------------------------------------------------------- */

/**
  Initialize a bitmap descriptor.

  @param[out] BitMap       Bitmap descriptor to initialize
  @param[in]  Buffer       Buffer to use for bitmap storage
  @param[in]  SizeOfBitMap Number of bits in bitmap
**/
VOID
EFIAPI
RtlInitializeBitMap (
    OUT PRTL_BITMAP  BitMap,
    IN  UINTN        *Buffer,
    IN  UINTN        SizeOfBitMap
    );

/**
  Calculate the size needed for a bitmap buffer.

  @param[in] SizeOfBitMap  Number of bits in bitmap

  @return Size of buffer needed in bytes
**/
static INLINE UINTN
RtlBitmapBufferSize (
    IN UINTN  SizeOfBitMap
    )
{
    return ((SizeOfBitMap + (sizeof(UINTN) * 8) - 1) / (sizeof(UINTN) * 8)) * sizeof(UINTN);
}

/* ---------------------------------------------------------------
 *  Bit Manipulation Functions
 * --------------------------------------------------------------- */

/**
  Set a bit in a bitmap.

  @param[in,out] BitMap  Bitmap descriptor
  @param[in]     BitNumber Bit number to set (0-based)
**/
VOID
EFIAPI
RtlSetBit (
    IN OUT PRTL_BITMAP  BitMap,
    IN     UINTN        BitNumber
    );

/**
  Clear a bit in a bitmap.

  @param[in,out] BitMap     Bitmap descriptor
  @param[in]     BitNumber  Bit number to clear (0-based)
**/
VOID
EFIAPI
RtlClearBit (
    IN OUT PRTL_BITMAP  BitMap,
    IN     UINTN        BitNumber
    );

/**
  Test a bit in a bitmap.

  @param[in] BitMap     Bitmap descriptor
  @param[in] BitNumber  Bit number to test (0-based)

  @retval TRUE   Bit is set
  @retval FALSE  Bit is clear
**/
BOOLEAN
EFIAPI
RtlTestBit (
    IN PRTL_BITMAP  BitMap,
    IN UINTN        BitNumber
    );

/**
  Set a range of bits in a bitmap.

  @param[in,out] BitMap        Bitmap descriptor
  @param[in]     StartingIndex Starting bit number
  @param[in]     NumberToSet   Number of bits to set
**/
VOID
EFIAPI
RtlSetBits (
    IN OUT PRTL_BITMAP  BitMap,
    IN     UINTN        StartingIndex,
    IN     UINTN        NumberToSet
    );

/**
  Clear a range of bits in a bitmap.

  @param[in,out] BitMap        Bitmap descriptor
  @param[in]     StartingIndex Starting bit number
  @param[in]     NumberToClear Number of bits to clear
**/
VOID
EFIAPI
RtlClearBits (
    IN OUT PRTL_BITMAP  BitMap,
    IN     UINTN        StartingIndex,
    IN     UINTN        NumberToClear
    );

/**
  Set all bits in a bitmap.

  @param[in,out] BitMap  Bitmap descriptor
**/
VOID
EFIAPI
RtlSetAllBits (
    IN OUT PRTL_BITMAP  BitMap
    );

/**
  Clear all bits in a bitmap.

  @param[in,out] BitMap  Bitmap descriptor
**/
VOID
EFIAPI
RtlClearAllBits (
    IN OUT PRTL_BITMAP  BitMap
    );

/* ---------------------------------------------------------------
 *  Bitmap Search Functions
 * --------------------------------------------------------------- */

/**
  Find the first set bit in a bitmap.

  @param[in] BitMap  Bitmap descriptor

  @return Bit number of first set bit, or -1 if no bits are set
**/
INTN
EFIAPI
RtlFindFirstSetBit (
    IN PRTL_BITMAP  BitMap
    );

/**
  Find the first clear bit in a bitmap.

  @param[in] BitMap  Bitmap descriptor

  @return Bit number of first clear bit, or -1 if no bits are clear
**/
INTN
EFIAPI
RtlFindFirstClearBit (
    IN PRTL_BITMAP  BitMap
    );

/**
  Find the last set bit in a bitmap.

  @param[in] BitMap  Bitmap descriptor

  @return Bit number of last set bit, or -1 if no bits are set
**/
INTN
EFIAPI
RtlFindLastSetBit (
    IN PRTL_BITMAP  BitMap
    );

/**
  Find the last clear bit in a bitmap.

  @param[in] BitMap  Bitmap descriptor

  @return Bit number of last clear bit, or -1 if no bits are clear
**/
INTN
EFIAPI
RtlFindLastClearBit (
    IN PRTL_BITMAP  BitMap
    );

/**
  Find a run of set bits in a bitmap.

  @param[in] BitMap          Bitmap descriptor
  @param[in] NumberToFind    Number of consecutive set bits to find
  @param[in] HintIndex       Hint for where to start searching

  @return Starting bit number of run, or -1 if not found
**/
INTN
EFIAPI
RtlFindSetBits (
    IN PRTL_BITMAP  BitMap,
    IN UINTN        NumberToFind,
    IN UINTN        HintIndex
    );

/**
  Find a run of clear bits in a bitmap.

  @param[in] BitMap          Bitmap descriptor
  @param[in] NumberToFind    Number of consecutive clear bits to find
  @param[in] HintIndex       Hint for where to start searching

  @return Starting bit number of run, or -1 if not found
**/
INTN
EFIAPI
RtlFindClearBits (
    IN PRTL_BITMAP  BitMap,
    IN UINTN        NumberToFind,
    IN UINTN        HintIndex
    );

/**
  Find a run of clear bits and set them.

  Atomically finds and sets a run of clear bits.

  @param[in,out] BitMap        Bitmap descriptor
  @param[in]     NumberToFind  Number of consecutive clear bits to find
  @param[in]     HintIndex     Hint for where to start searching

  @return Starting bit number of run, or -1 if not found
**/
INTN
EFIAPI
RtlFindClearBitsAndSet (
    IN OUT PRTL_BITMAP  BitMap,
    IN     UINTN        NumberToFind,
    IN     UINTN        HintIndex
    );

/**
  Find the next set bit after a given position.

  @param[in] BitMap        Bitmap descriptor
  @param[in] StartingIndex Starting bit number to search from

  @return Bit number of next set bit, or -1 if no more set bits
**/
INTN
EFIAPI
RtlFindNextSetBit (
    IN PRTL_BITMAP  BitMap,
    IN UINTN        StartingIndex
    );

/**
  Find the next clear bit after a given position.

  @param[in] BitMap        Bitmap descriptor
  @param[in] StartingIndex Starting bit number to search from

  @return Bit number of next clear bit, or -1 if no more clear bits
**/
INTN
EFIAPI
RtlFindNextClearBit (
    IN PRTL_BITMAP  BitMap,
    IN UINTN        StartingIndex
    );

/* ---------------------------------------------------------------
 *  Bitmap Query Functions
 * --------------------------------------------------------------- */

/**
  Count the number of set bits in a bitmap.

  @param[in] BitMap  Bitmap descriptor

  @return Number of set bits
**/
UINTN
EFIAPI
RtlNumberOfSetBits (
    IN PRTL_BITMAP  BitMap
    );

/**
  Count the number of clear bits in a bitmap.

  @param[in] BitMap  Bitmap descriptor

  @return Number of clear bits
**/
UINTN
EFIAPI
RtlNumberOfClearBits (
    IN PRTL_BITMAP  BitMap
    );

/**
  Count the number of set bits in a range.

  @param[in] BitMap        Bitmap descriptor
  @param[in] StartingIndex Starting bit number
  @param[in] Length        Number of bits to count

  @return Number of set bits in range
**/
UINTN
EFIAPI
RtlNumberOfSetBitsInRange (
    IN PRTL_BITMAP  BitMap,
    IN UINTN        StartingIndex,
    IN UINTN        Length
    );

/**
  Check if all bits in a bitmap are set.

  @param[in] BitMap  Bitmap descriptor

  @retval TRUE   All bits are set
  @retval FALSE  At least one bit is clear
**/
BOOLEAN
EFIAPI
RtlAreBitsSet (
    IN PRTL_BITMAP  BitMap
    );

/**
  Check if all bits in a bitmap are clear.

  @param[in] BitMap  Bitmap descriptor

  @retval TRUE   All bits are clear
  @retval FALSE  At least one bit is set
**/
BOOLEAN
EFIAPI
RtlAreBitsClear (
    IN PRTL_BITMAP  BitMap
    );

/**
  Check if a range of bits is set.

  @param[in] BitMap        Bitmap descriptor
  @param[in] StartingIndex Starting bit number
  @param[in] Length        Number of bits to check

  @retval TRUE   All bits in range are set
  @retval FALSE  At least one bit in range is clear
**/
BOOLEAN
EFIAPI
RtlAreRangeBitsSet (
    IN PRTL_BITMAP  BitMap,
    IN UINTN        StartingIndex,
    IN UINTN        Length
    );

/**
  Check if a range of bits is clear.

  @param[in] BitMap        Bitmap descriptor
  @param[in] StartingIndex Starting bit number
  @param[in] Length        Number of bits to check

  @retval TRUE   All bits in range are clear
  @retval FALSE  At least one bit in range is set
**/
BOOLEAN
EFIAPI
RtlAreRangeBitsClear (
    IN PRTL_BITMAP  BitMap,
    IN UINTN        StartingIndex,
    IN UINTN        Length
    );

/**
  Get the size of a bitmap.

  @param[in] BitMap  Bitmap descriptor

  @return Number of bits in bitmap
**/
static INLINE UINTN
RtlBitmapSize (
    IN PRTL_BITMAP  BitMap
    )
{
    return BitMap->SizeOfBitMap;
}

#endif /* __ANANKE_NTRTL_BITMAP_H__ */
