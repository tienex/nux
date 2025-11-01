/** @file
  NTRTL - NT Runtime Library

  Generic (portable) CHAR16/Unicode string operations

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>

/**
  Get length of CHAR16 string
**/
UINTN
ANXAPI
RtlStringLength16 (
    IN CONST CHAR16 *String
    )
{
    UINTN Length = 0;

    while (*String++) {
        Length++;
    }

    return Length;
}

/**
  Find character in CHAR16 string

  @return Pointer to first occurrence, or NULL if not found
**/
CHAR16 *
ANXAPI
RtlFindChar16 (
    IN CONST CHAR16 *String,
    IN CHAR16       Character
    )
{
    while (*String) {
        if (*String == Character) {
            return (CHAR16 *)String;
        }
        String++;
    }

    return (Character == 0) ? (CHAR16 *)String : NULL;
}

/**
  Find last occurrence of character in CHAR16 string

  @return Pointer to last occurrence, or NULL if not found
**/
CHAR16 *
ANXAPI
RtlFindLastChar16 (
    IN CONST CHAR16 *String,
    IN CHAR16       Character
    )
{
    CONST CHAR16 *Last = NULL;

    while (*String) {
        if (*String == Character) {
            Last = String;
        }
        String++;
    }

    if (Character == 0) {
        return (CHAR16 *)String;
    }

    return (CHAR16 *)Last;
}

/**
  Compare CHAR16 strings (up to N characters)

  @return <0 if String1 < String2, 0 if equal, >0 if String1 > String2
**/
INT32
ANXAPI
RtlCompareChars16 (
    IN CONST CHAR16 *String1,
    IN CONST CHAR16 *String2,
    IN UINTN        Length
    )
{
    while (Length--) {
        if (*String1 != *String2) {
            return *String1 - *String2;
        }
        if (*String1 == 0) {
            return 0;
        }
        String1++;
        String2++;
    }

    return 0;
}

/**
  Copy CHAR16 string

  @return Destination
**/
CHAR16 *
ANXAPI
RtlCopyString16 (
    OUT CHAR16       *Destination,
    IN  CONST CHAR16 *Source
    )
{
    CHAR16 *Dest = Destination;

    while ((*Dest++ = *Source++)) {
        ;
    }

    return Destination;
}

/**
  Copy CHAR16 string (up to N characters)

  @return Destination
**/
CHAR16 *
ANXAPI
RtlCopyChars16 (
    OUT CHAR16       *Destination,
    IN  CONST CHAR16 *Source,
    IN  UINTN        Length
    )
{
    CHAR16 *Dest = Destination;

    while (Length-- && (*Dest++ = *Source++)) {
        ;
    }

    /* Null-terminate if we didn't copy the null */
    if (Length == (UINTN)-1 && *(Dest - 1) != 0) {
        *Dest = 0;
    }

    return Destination;
}

/**
  Concatenate CHAR16 strings

  @return Destination
**/
CHAR16 *
ANXAPI
RtlConcatString16 (
    OUT CHAR16       *Destination,
    IN  CONST CHAR16 *Source
    )
{
    CHAR16 *Dest = Destination;

    /* Find end of destination */
    while (*Dest) {
        Dest++;
    }

    /* Copy source */
    while ((*Dest++ = *Source++)) {
        ;
    }

    return Destination;
}

/**
  Fill CHAR16 buffer with character

  @return Destination
**/
CHAR16 *
ANXAPI
RtlFillChars16 (
    OUT CHAR16 *Destination,
    IN  UINTN  Count,
    IN  CHAR16 Character
    )
{
    CHAR16 *Dest = Destination;

    while (Count--) {
        *Dest++ = Character;
    }

    return Destination;
}
