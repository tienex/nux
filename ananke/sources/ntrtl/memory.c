/** @file
  NT RTL Memory Functions Implementation

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>
#include <ananke/ntrtl.h>
#include <ananke/intrinsics.h>

/**
  Fill memory with zeros.
**/
VOID
ANXAPI
RtlZeroMemory (
    OUT VOID   *Destination,
    IN  UINTN  Length
    )
{
    ANX_MEMSET(Destination, 0, Length);
}

/**
  Fill memory with a specified byte value.
**/
VOID
ANXAPI
RtlFillMemory (
    OUT VOID   *Destination,
    IN  UINTN  Length,
    IN  UINT8  Fill
    )
{
    ANX_MEMSET(Destination, Fill, Length);
}

/**
  Securely zero memory (cannot be optimized away).
**/
VOID
ANXAPI
RtlSecureZeroMemory (
    OUT VOID   *Destination,
    IN  UINTN  Length
    )
{
    volatile UINT8 *Ptr = (volatile UINT8 *)Destination;

    while (Length--) {
        *Ptr++ = 0;
    }

    ANX_CPU_BARRIER();
}

/**
  Fill memory with a pattern (32-bit).
**/
VOID
ANXAPI
RtlFillMemoryUlong (
    OUT VOID    *Destination,
    IN  UINTN   Length,
    IN  UINT32  Pattern
    )
{
    UINT32 *Ptr = (UINT32 *)Destination;
    UINTN  Count = Length / sizeof(UINT32);

    while (Count--) {
        *Ptr++ = Pattern;
    }
}

/**
  Fill memory with a pattern (native word size).
**/
VOID
ANXAPI
RtlFillMemoryUintn (
    OUT VOID   *Destination,
    IN  UINTN  Length,
    IN  UINTN  Pattern
    )
{
    UINTN *Ptr = (UINTN *)Destination;
    UINTN Count = Length / sizeof(UINTN);

    while (Count--) {
        *Ptr++ = Pattern;
    }
}

/**
  Copy memory (buffers must not overlap).
**/
VOID
ANXAPI
RtlCopyMemory (
    OUT VOID        *Destination,
    IN  CONST VOID  *Source,
    IN  UINTN       Length
    )
{
    ANX_MEMCPY(Destination, Source, Length);
}

/**
  Move memory (buffers may overlap).
**/
VOID
ANXAPI
RtlMoveMemory (
    OUT VOID        *Destination,
    IN  CONST VOID  *Source,
    IN  UINTN       Length
    )
{
    ANX_MEMMOVE(Destination, Source, Length);
}

/**
  Copy memory in non-temporal way (bypass cache).
**/
VOID
ANXAPI
RtlCopyMemoryNonTemporal (
    OUT VOID        *Destination,
    IN  CONST VOID  *Source,
    IN  UINTN       Length
    )
{
    /* For now, just use regular copy.
       TODO: Implement non-temporal copies using SSE/AVX instructions */
    ANX_MEMCPY(Destination, Source, Length);
}

/**
  Compare two memory buffers.
**/
UINTN
ANXAPI
RtlCompareMemory (
    IN CONST VOID  *Source1,
    IN CONST VOID  *Source2,
    IN UINTN       Length
    )
{
    CONST UINT8 *Ptr1 = (CONST UINT8 *)Source1;
    CONST UINT8 *Ptr2 = (CONST UINT8 *)Source2;
    UINTN       Index;

    for (Index = 0; Index < Length; Index++) {
        if (Ptr1[Index] != Ptr2[Index]) {
            return Index;
        }
    }

    return Length;
}

/**
  Check if two memory buffers are equal.
**/
BOOLEAN
ANXAPI
RtlEqualMemory (
    IN CONST VOID  *Source1,
    IN CONST VOID  *Source2,
    IN UINTN       Length
    )
{
    return (ANX_MEMCMP(Source1, Source2, Length) == 0);
}

/**
  Compare two memory buffers (byte-by-byte).
**/
INT32
ANXAPI
RtlCompareMemoryUlong (
    IN CONST VOID  *Source1,
    IN CONST VOID  *Source2,
    IN  UINTN       Length
    )
{
    return ANX_MEMCMP(Source1, Source2, Length);
}

/**
  Prefetch memory for reading.
**/
VOID
ANXAPI
RtlPrefetchForRead (
    IN CONST VOID  *Address
    )
{
    ANX_CPU_PREFETCH_READ(Address);
}

/**
  Prefetch memory for writing.
**/
VOID
ANXAPI
RtlPrefetchForWrite (
    IN VOID  *Address
    )
{
    ANX_CPU_PREFETCH_WRITE(Address);
}
