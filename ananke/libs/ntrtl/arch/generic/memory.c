/** @file
  NTRTL - NT Runtime Library

  Generic (portable) memory operations

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>

/**
  Copy memory (non-overlapping buffers)
**/
VOID
ANXAPI
RtlCopyMemory (
    OUT VOID        *Destination,
    IN  CONST VOID  *Source,
    IN  UINTN       Length
    )
{
    UINT8 *Dest = (UINT8 *)Destination;
    CONST UINT8 *Src = (CONST UINT8 *)Source;

    while (Length--) {
        *Dest++ = *Src++;
    }
}

/**
  Move memory (buffers may overlap)
**/
VOID
ANXAPI
RtlMoveMemory (
    OUT VOID        *Destination,
    IN  CONST VOID  *Source,
    IN  UINTN       Length
    )
{
    UINT8 *Dest = (UINT8 *)Destination;
    CONST UINT8 *Src = (CONST UINT8 *)Source;

    if (Dest <= Src || Dest >= (Src + Length)) {
        /* Non-overlapping or safe to copy forward */
        while (Length--) {
            *Dest++ = *Src++;
        }
    } else {
        /* Overlapping, copy backward */
        Dest += Length;
        Src += Length;
        while (Length--) {
            *--Dest = *--Src;
        }
    }
}

/**
  Fill memory with byte value
**/
VOID
ANXAPI
RtlFillMemory (
    OUT VOID   *Destination,
    IN  UINTN  Length,
    IN  UINT8  Fill
    )
{
    UINT8 *Dest = (UINT8 *)Destination;

    while (Length--) {
        *Dest++ = Fill;
    }
}

/**
  Zero memory
**/
VOID
ANXAPI
RtlZeroMemory (
    OUT VOID   *Destination,
    IN  UINTN  Length
    )
{
    RtlFillMemory(Destination, Length, 0);
}

/**
  Compare memory buffers

  @return Number of matching bytes (equals Length if identical)
**/
UINTN
ANXAPI
RtlCompareMemory (
    IN CONST VOID  *Source1,
    IN CONST VOID  *Source2,
    IN UINTN       Length
    )
{
    CONST UINT8 *Src1 = (CONST UINT8 *)Source1;
    CONST UINT8 *Src2 = (CONST UINT8 *)Source2;
    UINTN Index;

    for (Index = 0; Index < Length; Index++) {
        if (Src1[Index] != Src2[Index]) {
            return Index;
        }
    }

    return Length;
}

/**
  Check if memory buffers are equal
**/
BOOLEAN
ANXAPI
RtlEqualMemory (
    IN CONST VOID  *Source1,
    IN CONST VOID  *Source2,
    IN UINTN       Length
    )
{
    return (RtlCompareMemory(Source1, Source2, Length) == Length);
}

/**
  Find byte in memory

  @return Pointer to first occurrence, or NULL if not found
**/
VOID *
ANXAPI
RtlFindByteInMemory (
    IN CONST VOID  *Buffer,
    IN UINTN       Length,
    IN UINT8       Value
    )
{
    CONST UINT8 *Buf = (CONST UINT8 *)Buffer;

    while (Length--) {
        if (*Buf == Value) {
            return (VOID *)Buf;
        }
        Buf++;
    }

    return NULL;
}
