/** @file
  NT RTL String Functions

  String manipulation functions following Windows NT RTL conventions.

  Copyright (C) 2025 ANANKE Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __ANANKE_NTRTL_STRING_H__
#define __ANANKE_NTRTL_STRING_H__

/* ---------------------------------------------------------------
 *  ANSI String Functions
 * --------------------------------------------------------------- */

/**
  Initialize an ANSI string.

  @param[out] DestinationString  String to initialize
  @param[in]  SourceString       Source null-terminated string (optional)
**/
VOID
EFIAPI
RtlInitString (
    OUT PSTRING     DestinationString,
    IN  CONST CHAR8 *SourceString OPTIONAL
    );

/**
  Initialize an ANSI string with explicit length.

  @param[out] DestinationString  String to initialize
  @param[in]  SourceString       Source string buffer
  @param[in]  Length             Length of string in bytes
  @param[in]  MaximumLength      Maximum length of buffer
**/
VOID
EFIAPI
RtlInitStringEx (
    OUT PSTRING     DestinationString,
    IN  CHAR8       *SourceString,
    IN  UINT16      Length,
    IN  UINT16      MaximumLength
    );

/**
  Copy an ANSI string.

  @param[out] DestinationString  Destination string
  @param[in]  SourceString       Source string

  @retval STATUS_SUCCESS           Copy successful
  @retval STATUS_BUFFER_TOO_SMALL  Destination buffer too small
**/
STATUS
EFIAPI
RtlCopyString (
    OUT PSTRING  DestinationString,
    IN  PSTRING  SourceString OPTIONAL
    );

/**
  Compare two ANSI strings.

  @param[in] String1        First string
  @param[in] String2        Second string
  @param[in] CaseInSensitive TRUE for case-insensitive comparison

  @return <0 if String1 < String2, 0 if equal, >0 if String1 > String2
**/
INT32
EFIAPI
RtlCompareString (
    IN PSTRING  String1,
    IN PSTRING  String2,
    IN BOOLEAN  CaseInSensitive
    );

/**
  Check if two ANSI strings are equal.

  @param[in] String1        First string
  @param[in] String2        Second string
  @param[in] CaseInSensitive TRUE for case-insensitive comparison

  @retval TRUE   Strings are equal
  @retval FALSE  Strings are not equal
**/
BOOLEAN
EFIAPI
RtlEqualString (
    IN PSTRING  String1,
    IN PSTRING  String2,
    IN BOOLEAN  CaseInSensitive
    );

/**
  Convert ANSI string to uppercase.

  @param[in,out] DestinationString  Destination string (can be same as source)
  @param[in]     SourceString       Source string

  @retval STATUS_SUCCESS  Conversion successful
**/
STATUS
EFIAPI
RtlUpperString (
    IN OUT PSTRING  DestinationString,
    IN     PSTRING  SourceString
    );

/**
  Append one ANSI string to another.

  @param[in,out] Destination  Destination string
  @param[in]     Source       Source string to append

  @retval STATUS_SUCCESS           Append successful
  @retval STATUS_BUFFER_TOO_SMALL  Destination buffer too small
**/
STATUS
EFIAPI
RtlAppendStringToString (
    IN OUT PSTRING  Destination,
    IN     PSTRING  Source
    );

/* ---------------------------------------------------------------
 *  Unicode String Functions
 * --------------------------------------------------------------- */

/**
  Initialize a Unicode string.

  @param[out] DestinationString  String to initialize
  @param[in]  SourceString       Source null-terminated string (optional)
**/
VOID
EFIAPI
RtlInitUnicodeString (
    OUT PUNICODE_STRING  DestinationString,
    IN  CONST CHAR16     *SourceString OPTIONAL
    );

/**
  Initialize a Unicode string with explicit length.

  @param[out] DestinationString  String to initialize
  @param[in]  SourceString       Source string buffer
  @param[in]  Length             Length of string in bytes
  @param[in]  MaximumLength      Maximum length of buffer in bytes
**/
VOID
EFIAPI
RtlInitUnicodeStringEx (
    OUT PUNICODE_STRING  DestinationString,
    IN  CHAR16           *SourceString,
    IN  UINT16           Length,
    IN  UINT16           MaximumLength
    );

/**
  Copy a Unicode string.

  @param[out] DestinationString  Destination string
  @param[in]  SourceString       Source string

  @retval STATUS_SUCCESS           Copy successful
  @retval STATUS_BUFFER_TOO_SMALL  Destination buffer too small
**/
STATUS
EFIAPI
RtlCopyUnicodeString (
    OUT PUNICODE_STRING  DestinationString,
    IN  PUNICODE_STRING  SourceString OPTIONAL
    );

/**
  Compare two Unicode strings.

  @param[in] String1        First string
  @param[in] String2        Second string
  @param[in] CaseInSensitive TRUE for case-insensitive comparison

  @return <0 if String1 < String2, 0 if equal, >0 if String1 > String2
**/
INT32
EFIAPI
RtlCompareUnicodeString (
    IN PUNICODE_STRING  String1,
    IN PUNICODE_STRING  String2,
    IN BOOLEAN          CaseInSensitive
    );

/**
  Check if two Unicode strings are equal.

  @param[in] String1        First string
  @param[in] String2        Second string
  @param[in] CaseInSensitive TRUE for case-insensitive comparison

  @retval TRUE   Strings are equal
  @retval FALSE  Strings are not equal
**/
BOOLEAN
EFIAPI
RtlEqualUnicodeString (
    IN PUNICODE_STRING  String1,
    IN PUNICODE_STRING  String2,
    IN BOOLEAN          CaseInSensitive
    );

/**
  Convert Unicode string to uppercase.

  @param[in,out] DestinationString  Destination string
  @param[in]     SourceString       Source string

  @retval STATUS_SUCCESS  Conversion successful
**/
STATUS
EFIAPI
RtlUpcaseUnicodeString (
    IN OUT PUNICODE_STRING  DestinationString,
    IN     PUNICODE_STRING  SourceString
    );

/**
  Append one Unicode string to another.

  @param[in,out] Destination  Destination string
  @param[in]     Source       Source string to append

  @retval STATUS_SUCCESS           Append successful
  @retval STATUS_BUFFER_TOO_SMALL  Destination buffer too small
**/
STATUS
EFIAPI
RtlAppendUnicodeStringToString (
    IN OUT PUNICODE_STRING  Destination,
    IN     PUNICODE_STRING  Source
    );

/**
  Append a Unicode string to another with buffer.

  @param[in,out] Destination  Destination string
  @param[in]     Source       Source null-terminated string

  @retval STATUS_SUCCESS           Append successful
  @retval STATUS_BUFFER_TOO_SMALL  Destination buffer too small
**/
STATUS
EFIAPI
RtlAppendUnicodeToString (
    IN OUT PUNICODE_STRING  Destination,
    IN     CONST CHAR16     *Source OPTIONAL
    );

/* ---------------------------------------------------------------
 *  String Conversion Functions
 * --------------------------------------------------------------- */

/**
  Convert Unicode string to ANSI string.

  @param[out] DestinationString  Destination ANSI string
  @param[in]  SourceString       Source Unicode string
  @param[in]  AllocateDestination TRUE to allocate destination buffer

  @retval STATUS_SUCCESS           Conversion successful
  @retval STATUS_BUFFER_TOO_SMALL  Destination buffer too small
**/
STATUS
EFIAPI
RtlUnicodeStringToAnsiString (
    OUT PANSI_STRING     DestinationString,
    IN  PUNICODE_STRING  SourceString,
    IN  BOOLEAN          AllocateDestinationString
    );

/**
  Convert ANSI string to Unicode string.

  @param[out] DestinationString      Destination Unicode string
  @param[in]  SourceString           Source ANSI string
  @param[in]  AllocateDestinationString TRUE to allocate destination buffer

  @retval STATUS_SUCCESS           Conversion successful
  @retval STATUS_BUFFER_TOO_SMALL  Destination buffer too small
**/
STATUS
EFIAPI
RtlAnsiStringToUnicodeString (
    OUT PUNICODE_STRING      DestinationString,
    IN  PANSI_STRING         SourceString,
    IN  BOOLEAN              AllocateDestinationString
    );

/**
  Convert integer to Unicode string.

  @param[in]     Value   Integer value to convert
  @param[in]     Base    Number base (2-36), 0 for default (10)
  @param[in,out] String  Unicode string to receive result

  @retval STATUS_SUCCESS           Conversion successful
  @retval STATUS_BUFFER_TOO_SMALL  String buffer too small
**/
STATUS
EFIAPI
RtlIntegerToUnicodeString (
    IN     UINTN             Value,
    IN     UINT32            Base OPTIONAL,
    IN OUT PUNICODE_STRING   String
    );

/**
  Convert integer to ANSI string.

  @param[in]     Value   Integer value to convert
  @param[in]     Base    Number base (2-36), 0 for default (10)
  @param[in,out] String  ANSI string to receive result

  @retval STATUS_SUCCESS           Conversion successful
  @retval STATUS_BUFFER_TOO_SMALL  String buffer too small
**/
STATUS
EFIAPI
RtlIntegerToString (
    IN     UINTN    Value,
    IN     UINT32   Base OPTIONAL,
    IN OUT PSTRING  String
    );

/**
  Convert Unicode string to integer.

  @param[in]  String  Unicode string to convert
  @param[in]  Base    Number base (2-36), 0 for auto-detect
  @param[out] Value   Converted integer value

  @retval STATUS_SUCCESS  Conversion successful
  @retval STATUS_INVALID  Invalid string format
**/
STATUS
EFIAPI
RtlUnicodeStringToInteger (
    IN  PUNICODE_STRING  String,
    IN  UINT32           Base OPTIONAL,
    OUT UINTN            *Value
    );

/**
  Convert ANSI string to integer.

  @param[in]  String  ANSI string to convert
  @param[in]  Base    Number base (2-36), 0 for auto-detect
  @param[out] Value   Converted integer value

  @retval STATUS_SUCCESS  Conversion successful
  @retval STATUS_INVALID  Invalid string format
**/
STATUS
EFIAPI
RtlStringToInteger (
    IN  PSTRING  String,
    IN  UINT32   Base OPTIONAL,
    OUT UINTN    *Value
    );

/**
  Compute hash of Unicode string.

  @param[in] String        String to hash
  @param[in] CaseInSensitive TRUE for case-insensitive hash

  @return Hash value
**/
UINT32
EFIAPI
RtlHashUnicodeString (
    IN PUNICODE_STRING  String,
    IN BOOLEAN          CaseInSensitive
    );

/**
  Compute hash of ANSI string.

  @param[in] String        String to hash
  @param[in] CaseInSensitive TRUE for case-insensitive hash

  @return Hash value
**/
UINT32
EFIAPI
RtlHashString (
    IN PSTRING  String,
    IN BOOLEAN  CaseInSensitive
    );

#endif /* __ANANKE_NTRTL_STRING_H__ */
