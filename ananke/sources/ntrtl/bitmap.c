/** @file
  NT RTL Bitmap Functions Implementation

  Bitmap manipulation functions following Windows NT RTL conventions.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>
#include <ananke/ntrtl.h>

/* ---------------------------------------------------------------
 *  Helper Functions
 * --------------------------------------------------------------- */

/**
  Get the number of bits per UINTN element.

  @return Number of bits in UINTN type
**/
static INLINE UINTN
BitsPerElement (
  VOID
  )
{
  return sizeof(UINTN) * 8;
}

/**
  Count the number of set bits in a UINTN value.

  @param[in] Value  Value to count bits in

  @return Number of set bits
**/
static INLINE UINTN
CountSetBits (
  IN UINTN  Value
  )
{
  UINTN Count = 0;

  while (Value != 0) {
    Count++;
    Value &= Value - 1;  // Clear the lowest set bit
  }

  return Count;
}

/**
  Find the position of the first set bit in a UINTN value.

  @param[in] Value  Value to search

  @return Bit position (0-based), or -1 if no bits are set
**/
static INLINE INTN
FindFirstSetBitInElement (
  IN UINTN  Value
  )
{
  if (Value == 0) {
    return -1;
  }

  // Use ANX_CTZN for portable count trailing zeros
  UINTN Position = (UINTN)ANX_CTZN(Value);

  return (INTN)Position;
}

/**
  Find the position of the first clear bit in a UINTN value.

  @param[in] Value  Value to search

  @return Bit position (0-based), or -1 if no bits are clear
**/
static INLINE INTN
FindFirstClearBitInElement (
  IN UINTN  Value
  )
{
  return FindFirstSetBitInElement(~Value);
}

/**
  Find the position of the last set bit in a UINTN value.

  @param[in] Value  Value to search

  @return Bit position (0-based), or -1 if no bits are set
**/
static INLINE INTN
FindLastSetBitInElement (
  IN UINTN  Value
  )
{
  if (Value == 0) {
    return -1;
  }

  // Use ANX_CLZN for portable count leading zeros
  UINTN Position = (sizeof(UINTN) * 8 - 1) - (UINTN)ANX_CLZN(Value);

  return (INTN)Position;
}

/* ---------------------------------------------------------------
 *  Bitmap Initialization
 * --------------------------------------------------------------- */

VOID
ANXAPI
RtlInitializeBitMap (
  OUT PRTL_BITMAP  BitMap,
  IN  UINTN        *Buffer,
  IN  UINTN        SizeOfBitMap
  )
{
  BitMap->SizeOfBitMap = SizeOfBitMap;
  BitMap->Buffer = Buffer;
}

/* ---------------------------------------------------------------
 *  Bit Manipulation Functions
 * --------------------------------------------------------------- */

VOID
ANXAPI
RtlSetBit (
  IN OUT PRTL_BITMAP  BitMap,
  IN     UINTN        BitNumber
  )
{
  UINTN ElementIndex = BitNumber / BitsPerElement();
  UINTN BitOffset = BitNumber % BitsPerElement();

  BitMap->Buffer[ElementIndex] |= ((UINTN)1 << BitOffset);
}

VOID
ANXAPI
RtlClearBit (
  IN OUT PRTL_BITMAP  BitMap,
  IN     UINTN        BitNumber
  )
{
  UINTN ElementIndex = BitNumber / BitsPerElement();
  UINTN BitOffset = BitNumber % BitsPerElement();

  BitMap->Buffer[ElementIndex] &= ~((UINTN)1 << BitOffset);
}

BOOLEAN
ANXAPI
RtlTestBit (
  IN PRTL_BITMAP  BitMap,
  IN UINTN        BitNumber
  )
{
  UINTN ElementIndex = BitNumber / BitsPerElement();
  UINTN BitOffset = BitNumber % BitsPerElement();

  return (BitMap->Buffer[ElementIndex] & ((UINTN)1 << BitOffset)) != 0;
}

VOID
ANXAPI
RtlSetBits (
  IN OUT PRTL_BITMAP  BitMap,
  IN     UINTN        StartingIndex,
  IN     UINTN        NumberToSet
  )
{
  UINTN Index;

  for (Index = 0; Index < NumberToSet; Index++) {
    RtlSetBit(BitMap, StartingIndex + Index);
  }
}

VOID
ANXAPI
RtlClearBits (
  IN OUT PRTL_BITMAP  BitMap,
  IN     UINTN        StartingIndex,
  IN     UINTN        NumberToClear
  )
{
  UINTN Index;

  for (Index = 0; Index < NumberToClear; Index++) {
    RtlClearBit(BitMap, StartingIndex + Index);
  }
}

VOID
ANXAPI
RtlSetAllBits (
  IN OUT PRTL_BITMAP  BitMap
  )
{
  UINTN NumElements = (BitMap->SizeOfBitMap + BitsPerElement() - 1) / BitsPerElement();
  UINTN Index;

  for (Index = 0; Index < NumElements; Index++) {
    BitMap->Buffer[Index] = (UINTN)(-1);
  }
}

VOID
ANXAPI
RtlClearAllBits (
  IN OUT PRTL_BITMAP  BitMap
  )
{
  UINTN NumElements = (BitMap->SizeOfBitMap + BitsPerElement() - 1) / BitsPerElement();
  UINTN Index;

  for (Index = 0; Index < NumElements; Index++) {
    BitMap->Buffer[Index] = 0;
  }
}

/* ---------------------------------------------------------------
 *  Bitmap Search Functions
 * --------------------------------------------------------------- */

INTN
ANXAPI
RtlFindFirstSetBit (
  IN PRTL_BITMAP  BitMap
  )
{
  UINTN NumElements = (BitMap->SizeOfBitMap + BitsPerElement() - 1) / BitsPerElement();
  UINTN ElementIndex;

  for (ElementIndex = 0; ElementIndex < NumElements; ElementIndex++) {
    if (BitMap->Buffer[ElementIndex] != 0) {
      INTN BitPosition = FindFirstSetBitInElement(BitMap->Buffer[ElementIndex]);
      if (BitPosition >= 0) {
        INTN GlobalPosition = (INTN)(ElementIndex * BitsPerElement() + (UINTN)BitPosition);
        if ((UINTN)GlobalPosition < BitMap->SizeOfBitMap) {
          return GlobalPosition;
        }
      }
    }
  }

  return -1;
}

INTN
ANXAPI
RtlFindFirstClearBit (
  IN PRTL_BITMAP  BitMap
  )
{
  UINTN NumElements = (BitMap->SizeOfBitMap + BitsPerElement() - 1) / BitsPerElement();
  UINTN ElementIndex;

  for (ElementIndex = 0; ElementIndex < NumElements; ElementIndex++) {
    if (BitMap->Buffer[ElementIndex] != (UINTN)(-1)) {
      INTN BitPosition = FindFirstClearBitInElement(BitMap->Buffer[ElementIndex]);
      if (BitPosition >= 0) {
        INTN GlobalPosition = (INTN)(ElementIndex * BitsPerElement() + (UINTN)BitPosition);
        if ((UINTN)GlobalPosition < BitMap->SizeOfBitMap) {
          return GlobalPosition;
        }
      }
    }
  }

  return -1;
}

INTN
ANXAPI
RtlFindLastSetBit (
  IN PRTL_BITMAP  BitMap
  )
{
  UINTN NumElements = (BitMap->SizeOfBitMap + BitsPerElement() - 1) / BitsPerElement();
  INTN ElementIndex;

  for (ElementIndex = (INTN)NumElements - 1; ElementIndex >= 0; ElementIndex--) {
    if (BitMap->Buffer[ElementIndex] != 0) {
      INTN BitPosition = FindLastSetBitInElement(BitMap->Buffer[ElementIndex]);
      if (BitPosition >= 0) {
        INTN GlobalPosition = (INTN)(((UINTN)ElementIndex) * BitsPerElement() + (UINTN)BitPosition);
        if ((UINTN)GlobalPosition < BitMap->SizeOfBitMap) {
          return GlobalPosition;
        }
      }
    }
  }

  return -1;
}

INTN
ANXAPI
RtlFindLastClearBit (
  IN PRTL_BITMAP  BitMap
  )
{
  UINTN NumElements = (BitMap->SizeOfBitMap + BitsPerElement() - 1) / BitsPerElement();
  INTN ElementIndex;

  for (ElementIndex = (INTN)NumElements - 1; ElementIndex >= 0; ElementIndex--) {
    if (BitMap->Buffer[ElementIndex] != (UINTN)(-1)) {
      INTN BitPosition = FindLastSetBitInElement(~BitMap->Buffer[ElementIndex]);
      if (BitPosition >= 0) {
        INTN GlobalPosition = (INTN)(((UINTN)ElementIndex) * BitsPerElement() + (UINTN)BitPosition);
        if ((UINTN)GlobalPosition < BitMap->SizeOfBitMap) {
          return GlobalPosition;
        }
      }
    }
  }

  return -1;
}

INTN
ANXAPI
RtlFindSetBits (
  IN PRTL_BITMAP  BitMap,
  IN UINTN        NumberToFind,
  IN UINTN        HintIndex
  )
{
  UINTN Index = HintIndex;
  UINTN RunLength = 0;
  UINTN RunStart = 0;

  // Wrap-around search
  for (UINTN Pass = 0; Pass < 2; Pass++) {
    while (Index < BitMap->SizeOfBitMap) {
      if (RtlTestBit(BitMap, Index)) {
        if (RunLength == 0) {
          RunStart = Index;
        }
        RunLength++;

        if (RunLength == NumberToFind) {
          return (INTN)RunStart;
        }
      } else {
        RunLength = 0;
      }

      Index++;
    }

    // Wrap around to beginning
    if (Pass == 0 && HintIndex > 0) {
      Index = 0;
      RunLength = 0;
      if (Index >= HintIndex) {
        break;
      }
    }
  }

  return -1;
}

INTN
ANXAPI
RtlFindClearBits (
  IN PRTL_BITMAP  BitMap,
  IN UINTN        NumberToFind,
  IN UINTN        HintIndex
  )
{
  UINTN Index = HintIndex;
  UINTN RunLength = 0;
  UINTN RunStart = 0;

  // Wrap-around search
  for (UINTN Pass = 0; Pass < 2; Pass++) {
    while (Index < BitMap->SizeOfBitMap) {
      if (!RtlTestBit(BitMap, Index)) {
        if (RunLength == 0) {
          RunStart = Index;
        }
        RunLength++;

        if (RunLength == NumberToFind) {
          return (INTN)RunStart;
        }
      } else {
        RunLength = 0;
      }

      Index++;
    }

    // Wrap around to beginning
    if (Pass == 0 && HintIndex > 0) {
      Index = 0;
      RunLength = 0;
      if (Index >= HintIndex) {
        break;
      }
    }
  }

  return -1;
}

INTN
ANXAPI
RtlFindClearBitsAndSet (
  IN OUT PRTL_BITMAP  BitMap,
  IN     UINTN        NumberToFind,
  IN     UINTN        HintIndex
  )
{
  INTN StartingIndex = RtlFindClearBits(BitMap, NumberToFind, HintIndex);

  if (StartingIndex >= 0) {
    RtlSetBits(BitMap, (UINTN)StartingIndex, NumberToFind);
  }

  return StartingIndex;
}

INTN
ANXAPI
RtlFindNextSetBit (
  IN PRTL_BITMAP  BitMap,
  IN UINTN        StartingIndex
  )
{
  UINTN Index;

  for (Index = StartingIndex + 1; Index < BitMap->SizeOfBitMap; Index++) {
    if (RtlTestBit(BitMap, Index)) {
      return (INTN)Index;
    }
  }

  return -1;
}

INTN
ANXAPI
RtlFindNextClearBit (
  IN PRTL_BITMAP  BitMap,
  IN UINTN        StartingIndex
  )
{
  UINTN Index;

  for (Index = StartingIndex + 1; Index < BitMap->SizeOfBitMap; Index++) {
    if (!RtlTestBit(BitMap, Index)) {
      return (INTN)Index;
    }
  }

  return -1;
}

/* ---------------------------------------------------------------
 *  Bitmap Query Functions
 * --------------------------------------------------------------- */

UINTN
ANXAPI
RtlNumberOfSetBits (
  IN PRTL_BITMAP  BitMap
  )
{
  UINTN NumElements = (BitMap->SizeOfBitMap + BitsPerElement() - 1) / BitsPerElement();
  UINTN Count = 0;
  UINTN ElementIndex;

  for (ElementIndex = 0; ElementIndex < NumElements; ElementIndex++) {
    Count += CountSetBits(BitMap->Buffer[ElementIndex]);
  }

  // Adjust for partial last element
  UINTN ExtraBits = (NumElements * BitsPerElement()) - BitMap->SizeOfBitMap;
  if (ExtraBits > 0 && NumElements > 0) {
    UINTN LastElement = BitMap->Buffer[NumElements - 1];
    UINTN Mask = ((UINTN)1 << (BitsPerElement() - ExtraBits)) - 1;
    Count -= CountSetBits(LastElement & ~Mask);
  }

  return Count;
}

UINTN
ANXAPI
RtlNumberOfClearBits (
  IN PRTL_BITMAP  BitMap
  )
{
  return BitMap->SizeOfBitMap - RtlNumberOfSetBits(BitMap);
}

UINTN
ANXAPI
RtlNumberOfSetBitsInRange (
  IN PRTL_BITMAP  BitMap,
  IN UINTN        StartingIndex,
  IN UINTN        Length
  )
{
  UINTN Count = 0;
  UINTN Index;

  for (Index = 0; Index < Length; Index++) {
    if (RtlTestBit(BitMap, StartingIndex + Index)) {
      Count++;
    }
  }

  return Count;
}

BOOLEAN
ANXAPI
RtlAreBitsSet (
  IN PRTL_BITMAP  BitMap
  )
{
  UINTN NumElements = (BitMap->SizeOfBitMap + BitsPerElement() - 1) / BitsPerElement();
  UINTN ElementIndex;

  for (ElementIndex = 0; ElementIndex < NumElements - 1; ElementIndex++) {
    if (BitMap->Buffer[ElementIndex] != (UINTN)(-1)) {
      return FALSE;
    }
  }

  // Check last element with mask
  if (NumElements > 0) {
    UINTN LastElementIndex = NumElements - 1;
    UINTN BitsInLastElement = BitMap->SizeOfBitMap - (LastElementIndex * BitsPerElement());
    UINTN Mask = ((UINTN)1 << BitsInLastElement) - 1;

    if ((BitMap->Buffer[LastElementIndex] & Mask) != Mask) {
      return FALSE;
    }
  }

  return TRUE;
}

BOOLEAN
ANXAPI
RtlAreBitsClear (
  IN PRTL_BITMAP  BitMap
  )
{
  UINTN NumElements = (BitMap->SizeOfBitMap + BitsPerElement() - 1) / BitsPerElement();
  UINTN ElementIndex;

  for (ElementIndex = 0; ElementIndex < NumElements - 1; ElementIndex++) {
    if (BitMap->Buffer[ElementIndex] != 0) {
      return FALSE;
    }
  }

  // Check last element with mask
  if (NumElements > 0) {
    UINTN LastElementIndex = NumElements - 1;
    UINTN BitsInLastElement = BitMap->SizeOfBitMap - (LastElementIndex * BitsPerElement());
    UINTN Mask = ((UINTN)1 << BitsInLastElement) - 1;

    if ((BitMap->Buffer[LastElementIndex] & Mask) != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

BOOLEAN
ANXAPI
RtlAreRangeBitsSet (
  IN PRTL_BITMAP  BitMap,
  IN UINTN        StartingIndex,
  IN UINTN        Length
  )
{
  UINTN Index;

  for (Index = 0; Index < Length; Index++) {
    if (!RtlTestBit(BitMap, StartingIndex + Index)) {
      return FALSE;
    }
  }

  return TRUE;
}

BOOLEAN
ANXAPI
RtlAreRangeBitsClear (
  IN PRTL_BITMAP  BitMap,
  IN UINTN        StartingIndex,
  IN UINTN        Length
  )
{
  UINTN Index;

  for (Index = 0; Index < Length; Index++) {
    if (RtlTestBit(BitMap, StartingIndex + Index)) {
      return FALSE;
    }
  }

  return TRUE;
}
