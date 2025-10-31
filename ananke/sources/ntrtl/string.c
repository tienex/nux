/** @file
  NT RTL String Functions Implementation

  Copyright (C) 2025 ANANKE Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>
#include <ananke/ntrtl.h>
#include <ananke/intrinsics.h>

/* ---------------------------------------------------------------
 *  Helper Functions
 * --------------------------------------------------------------- */

static INLINE UINTN
AnsiStrLen (
    IN CONST CHAR8  *String
    )
{
    UINTN Length = 0;

    if (String == NULL) {
        return 0;
    }

    while (*String++) {
        Length++;
    }

    return Length;
}

static INLINE UINTN
UnicodeStrLen (
    IN CONST CHAR16  *String
    )
{
    UINTN Length = 0;

    if (String == NULL) {
        return 0;
    }

    while (*String++) {
        Length++;
    }

    return Length;
}

static INLINE CHAR8
AnsiToUpper (
    IN CHAR8  Ch
    )
{
    if (Ch >= 'a' && Ch <= 'z') {
        return Ch - ('a' - 'A');
    }
    return Ch;
}

static INLINE CHAR16
UnicodeToUpper (
    IN CHAR16  Ch
    )
{
    if (Ch >= L'a' && Ch <= L'z') {
        return Ch - (L'a' - L'A');
    }
    return Ch;
}

/* ---------------------------------------------------------------
 *  ANSI String Functions
 * --------------------------------------------------------------- */

VOID
EFIAPI
RtlInitString (
    OUT PSTRING      DestinationString,
    IN  CONST CHAR8  *SourceString OPTIONAL
    )
{
    DestinationString->Buffer = (CHAR8 *)SourceString;

    if (SourceString != NULL) {
        UINTN Length = AnsiStrLen(SourceString);
        DestinationString->Length = (UINT16)Length;
        DestinationString->MaximumLength = (UINT16)(Length + 1);
    } else {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }
}

VOID
EFIAPI
RtlInitStringEx (
    OUT PSTRING  DestinationString,
    IN  CHAR8    *SourceString,
    IN  UINT16   Length,
    IN  UINT16   MaximumLength
    )
{
    DestinationString->Buffer = SourceString;
    DestinationString->Length = Length;
    DestinationString->MaximumLength = MaximumLength;
}

STATUS
EFIAPI
RtlCopyString (
    OUT PSTRING  DestinationString,
    IN  PSTRING  SourceString OPTIONAL
    )
{
    if (SourceString == NULL) {
        DestinationString->Length = 0;
        return STATUS_SUCCESS;
    }

    if (SourceString->Length > DestinationString->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ANX_MEMCPY(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
    DestinationString->Length = SourceString->Length;

    return STATUS_SUCCESS;
}

INT32
EFIAPI
RtlCompareString (
    IN PSTRING  String1,
    IN PSTRING  String2,
    IN BOOLEAN  CaseInSensitive
    )
{
    UINTN MinLen = (String1->Length < String2->Length) ? String1->Length : String2->Length;
    UINTN Index;

    for (Index = 0; Index < MinLen; Index++) {
        CHAR8 Ch1 = String1->Buffer[Index];
        CHAR8 Ch2 = String2->Buffer[Index];

        if (CaseInSensitive) {
            Ch1 = AnsiToUpper(Ch1);
            Ch2 = AnsiToUpper(Ch2);
        }

        if (Ch1 != Ch2) {
            return (Ch1 < Ch2) ? -1 : 1;
        }
    }

    if (String1->Length == String2->Length) {
        return 0;
    }

    return (String1->Length < String2->Length) ? -1 : 1;
}

BOOLEAN
EFIAPI
RtlEqualString (
    IN PSTRING  String1,
    IN PSTRING  String2,
    IN BOOLEAN  CaseInSensitive
    )
{
    if (String1->Length != String2->Length) {
        return FALSE;
    }

    return (RtlCompareString(String1, String2, CaseInSensitive) == 0);
}

STATUS
EFIAPI
RtlUpperString (
    IN OUT PSTRING  DestinationString,
    IN     PSTRING  SourceString
    )
{
    UINTN Index;

    if (SourceString->Length > DestinationString->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    for (Index = 0; Index < SourceString->Length; Index++) {
        DestinationString->Buffer[Index] = AnsiToUpper(SourceString->Buffer[Index]);
    }

    DestinationString->Length = SourceString->Length;

    return STATUS_SUCCESS;
}

STATUS
EFIAPI
RtlAppendStringToString (
    IN OUT PSTRING  Destination,
    IN     PSTRING  Source
    )
{
    UINT16 NewLength = Destination->Length + Source->Length;

    if (NewLength > Destination->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ANX_MEMCPY(&Destination->Buffer[Destination->Length], Source->Buffer, Source->Length);
    Destination->Length = NewLength;

    return STATUS_SUCCESS;
}

/* ---------------------------------------------------------------
 *  Unicode String Functions
 * --------------------------------------------------------------- */

VOID
EFIAPI
RtlInitUnicodeString (
    OUT PUNICODE_STRING  DestinationString,
    IN  CONST CHAR16     *SourceString OPTIONAL
    )
{
    DestinationString->Buffer = (CHAR16 *)SourceString;

    if (SourceString != NULL) {
        UINTN Length = UnicodeStrLen(SourceString);
        DestinationString->Length = (UINT16)(Length * sizeof(CHAR16));
        DestinationString->MaximumLength = (UINT16)((Length + 1) * sizeof(CHAR16));
    } else {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }
}

VOID
EFIAPI
RtlInitUnicodeStringEx (
    OUT PUNICODE_STRING  DestinationString,
    IN  CHAR16           *SourceString,
    IN  UINT16           Length,
    IN  UINT16           MaximumLength
    )
{
    DestinationString->Buffer = SourceString;
    DestinationString->Length = Length;
    DestinationString->MaximumLength = MaximumLength;
}

STATUS
EFIAPI
RtlCopyUnicodeString (
    OUT PUNICODE_STRING  DestinationString,
    IN  PUNICODE_STRING  SourceString OPTIONAL
    )
{
    if (SourceString == NULL) {
        DestinationString->Length = 0;
        return STATUS_SUCCESS;
    }

    if (SourceString->Length > DestinationString->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ANX_MEMCPY(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
    DestinationString->Length = SourceString->Length;

    return STATUS_SUCCESS;
}

INT32
EFIAPI
RtlCompareUnicodeString (
    IN PUNICODE_STRING  String1,
    IN PUNICODE_STRING  String2,
    IN BOOLEAN          CaseInSensitive
    )
{
    UINTN MinLen = (String1->Length < String2->Length) ? String1->Length : String2->Length;
    UINTN NumChars = MinLen / sizeof(CHAR16);
    UINTN Index;

    for (Index = 0; Index < NumChars; Index++) {
        CHAR16 Ch1 = String1->Buffer[Index];
        CHAR16 Ch2 = String2->Buffer[Index];

        if (CaseInSensitive) {
            Ch1 = UnicodeToUpper(Ch1);
            Ch2 = UnicodeToUpper(Ch2);
        }

        if (Ch1 != Ch2) {
            return (Ch1 < Ch2) ? -1 : 1;
        }
    }

    if (String1->Length == String2->Length) {
        return 0;
    }

    return (String1->Length < String2->Length) ? -1 : 1;
}

BOOLEAN
EFIAPI
RtlEqualUnicodeString (
    IN PUNICODE_STRING  String1,
    IN PUNICODE_STRING  String2,
    IN BOOLEAN          CaseInSensitive
    )
{
    if (String1->Length != String2->Length) {
        return FALSE;
    }

    return (RtlCompareUnicodeString(String1, String2, CaseInSensitive) == 0);
}

STATUS
EFIAPI
RtlUpcaseUnicodeString (
    IN OUT PUNICODE_STRING  DestinationString,
    IN     PUNICODE_STRING  SourceString
    )
{
    UINTN NumChars = SourceString->Length / sizeof(CHAR16);
    UINTN Index;

    if (SourceString->Length > DestinationString->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    for (Index = 0; Index < NumChars; Index++) {
        DestinationString->Buffer[Index] = UnicodeToUpper(SourceString->Buffer[Index]);
    }

    DestinationString->Length = SourceString->Length;

    return STATUS_SUCCESS;
}

STATUS
EFIAPI
RtlAppendUnicodeStringToString (
    IN OUT PUNICODE_STRING  Destination,
    IN     PUNICODE_STRING  Source
    )
{
    UINT16 NewLength = Destination->Length + Source->Length;

    if (NewLength > Destination->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ANX_MEMCPY(
        (UINT8 *)Destination->Buffer + Destination->Length,
        Source->Buffer,
        Source->Length
    );
    Destination->Length = NewLength;

    return STATUS_SUCCESS;
}

STATUS
EFIAPI
RtlAppendUnicodeToString (
    IN OUT PUNICODE_STRING  Destination,
    IN     CONST CHAR16     *Source OPTIONAL
    )
{
    if (Source == NULL) {
        return STATUS_SUCCESS;
    }

    UINTN SourceLength = UnicodeStrLen(Source) * sizeof(CHAR16);
    UINT16 NewLength = Destination->Length + (UINT16)SourceLength;

    if (NewLength > Destination->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ANX_MEMCPY(
        (UINT8 *)Destination->Buffer + Destination->Length,
        Source,
        SourceLength
    );
    Destination->Length = NewLength;

    return STATUS_SUCCESS;
}

/* ---------------------------------------------------------------
 *  String Conversion Functions
 * --------------------------------------------------------------- */

STATUS
EFIAPI
RtlUnicodeStringToAnsiString (
    OUT PANSI_STRING     DestinationString,
    IN  PUNICODE_STRING  SourceString,
    IN  BOOLEAN          AllocateDestinationString
    )
{
    UINTN NumChars = SourceString->Length / sizeof(CHAR16);
    UINTN Index;

    if (AllocateDestinationString) {
        /* TODO: Implement allocation */
        return STATUS_NOT_IMPLEMENTED;
    }

    if (NumChars > DestinationString->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    for (Index = 0; Index < NumChars; Index++) {
        /* Simple conversion - just truncate to 8-bit */
        DestinationString->Buffer[Index] = (CHAR8)SourceString->Buffer[Index];
    }

    DestinationString->Length = (UINT16)NumChars;

    return STATUS_SUCCESS;
}

STATUS
EFIAPI
RtlAnsiStringToUnicodeString (
    OUT PUNICODE_STRING  DestinationString,
    IN  PANSI_STRING     SourceString,
    IN  BOOLEAN          AllocateDestinationString
    )
{
    UINTN NumChars = SourceString->Length;
    UINTN Index;

    if (AllocateDestinationString) {
        /* TODO: Implement allocation */
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((NumChars * sizeof(CHAR16)) > DestinationString->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    for (Index = 0; Index < NumChars; Index++) {
        /* Simple conversion - zero-extend to 16-bit */
        DestinationString->Buffer[Index] = (CHAR16)(UINT8)SourceString->Buffer[Index];
    }

    DestinationString->Length = (UINT16)(NumChars * sizeof(CHAR16));

    return STATUS_SUCCESS;
}

STATUS
EFIAPI
RtlIntegerToUnicodeString (
    IN     UINTN             Value,
    IN     UINT32            Base OPTIONAL,
    IN OUT PUNICODE_STRING   String
    )
{
    CHAR16 Buffer[66];  /* Enough for 64-bit binary + sign + null */
    UINTN  Index = 0;
    UINTN  Digit;
    BOOLEAN Negative = FALSE;

    if (Base == 0) {
        Base = 10;
    }

    if (Base < 2 || Base > 36) {
        return STATUS_INVALID;
    }

    /* Handle zero specially */
    if (Value == 0) {
        Buffer[Index++] = L'0';
    } else {
        /* Convert value to string (reversed) */
        while (Value != 0) {
            Digit = Value % Base;
            Buffer[Index++] = (CHAR16)(Digit < 10 ? L'0' + Digit : L'A' + Digit - 10);
            Value /= Base;
        }

        /* Reverse the string */
        for (UINTN i = 0; i < Index / 2; i++) {
            CHAR16 Temp = Buffer[i];
            Buffer[i] = Buffer[Index - 1 - i];
            Buffer[Index - 1 - i] = Temp;
        }
    }

    /* Check if buffer is large enough */
    if ((Index * sizeof(CHAR16)) > String->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Copy to destination */
    ANX_MEMCPY(String->Buffer, Buffer, Index * sizeof(CHAR16));
    String->Length = (UINT16)(Index * sizeof(CHAR16));

    return STATUS_SUCCESS;
}

STATUS
EFIAPI
RtlIntegerToString (
    IN     UINTN    Value,
    IN     UINT32   Base OPTIONAL,
    IN OUT PSTRING  String
    )
{
    CHAR8  Buffer[66];
    UINTN  Index = 0;
    UINTN  Digit;

    if (Base == 0) {
        Base = 10;
    }

    if (Base < 2 || Base > 36) {
        return STATUS_INVALID;
    }

    if (Value == 0) {
        Buffer[Index++] = '0';
    } else {
        while (Value != 0) {
            Digit = Value % Base;
            Buffer[Index++] = (CHAR8)(Digit < 10 ? '0' + Digit : 'A' + Digit - 10);
            Value /= Base;
        }

        /* Reverse */
        for (UINTN i = 0; i < Index / 2; i++) {
            CHAR8 Temp = Buffer[i];
            Buffer[i] = Buffer[Index - 1 - i];
            Buffer[Index - 1 - i] = Temp;
        }
    }

    if (Index > String->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ANX_MEMCPY(String->Buffer, Buffer, Index);
    String->Length = (UINT16)Index;

    return STATUS_SUCCESS;
}

STATUS
EFIAPI
RtlUnicodeStringToInteger (
    IN  PUNICODE_STRING  String,
    IN  UINT32           Base OPTIONAL,
    OUT UINTN            *Value
    )
{
    UINTN  Result = 0;
    UINTN  NumChars = String->Length / sizeof(CHAR16);
    UINTN  Index = 0;
    BOOLEAN Negative = FALSE;

    if (NumChars == 0) {
        return STATUS_INVALID;
    }

    if (Base == 0) {
        /* Auto-detect base */
        if (NumChars >= 2 && String->Buffer[0] == L'0' && String->Buffer[1] == L'x') {
            Base = 16;
            Index = 2;
        } else {
            Base = 10;
        }
    }

    if (Base < 2 || Base > 36) {
        return STATUS_INVALID;
    }

    /* Parse sign */
    if (String->Buffer[Index] == L'-') {
        Negative = TRUE;
        Index++;
    } else if (String->Buffer[Index] == L'+') {
        Index++;
    }

    /* Parse digits */
    for (; Index < NumChars; Index++) {
        CHAR16 Ch = String->Buffer[Index];
        UINTN Digit;

        if (Ch >= L'0' && Ch <= L'9') {
            Digit = Ch - L'0';
        } else if (Ch >= L'A' && Ch <= L'Z') {
            Digit = Ch - L'A' + 10;
        } else if (Ch >= L'a' && Ch <= L'z') {
            Digit = Ch - L'a' + 10;
        } else {
            return STATUS_INVALID;
        }

        if (Digit >= Base) {
            return STATUS_INVALID;
        }

        Result = Result * Base + Digit;
    }

    if (Negative) {
        Result = (UINTN)(-(INTN)Result);
    }

    *Value = Result;
    return STATUS_SUCCESS;
}

STATUS
EFIAPI
RtlStringToInteger (
    IN  PSTRING  String,
    IN  UINT32   Base OPTIONAL,
    OUT UINTN    *Value
    )
{
    UINTN  Result = 0;
    UINTN  Index = 0;
    BOOLEAN Negative = FALSE;

    if (String->Length == 0) {
        return STATUS_INVALID;
    }

    if (Base == 0) {
        if (String->Length >= 2 && String->Buffer[0] == '0' && String->Buffer[1] == 'x') {
            Base = 16;
            Index = 2;
        } else {
            Base = 10;
        }
    }

    if (Base < 2 || Base > 36) {
        return STATUS_INVALID;
    }

    if (String->Buffer[Index] == '-') {
        Negative = TRUE;
        Index++;
    } else if (String->Buffer[Index] == '+') {
        Index++;
    }

    for (; Index < String->Length; Index++) {
        CHAR8 Ch = String->Buffer[Index];
        UINTN Digit;

        if (Ch >= '0' && Ch <= '9') {
            Digit = Ch - '0';
        } else if (Ch >= 'A' && Ch <= 'Z') {
            Digit = Ch - 'A' + 10;
        } else if (Ch >= 'a' && Ch <= 'z') {
            Digit = Ch - 'a' + 10;
        } else {
            return STATUS_INVALID;
        }

        if (Digit >= Base) {
            return STATUS_INVALID;
        }

        Result = Result * Base + Digit;
    }

    if (Negative) {
        Result = (UINTN)(-(INTN)Result);
    }

    *Value = Result;
    return STATUS_SUCCESS;
}

/* ---------------------------------------------------------------
 *  Hash Functions
 * --------------------------------------------------------------- */

UINT32
EFIAPI
RtlHashUnicodeString (
    IN PUNICODE_STRING  String,
    IN BOOLEAN          CaseInSensitive
    )
{
    UINT32 Hash = 0;
    UINTN  NumChars = String->Length / sizeof(CHAR16);
    UINTN  Index;

    for (Index = 0; Index < NumChars; Index++) {
        CHAR16 Ch = String->Buffer[Index];

        if (CaseInSensitive) {
            Ch = UnicodeToUpper(Ch);
        }

        Hash = ((Hash << 5) + Hash) + Ch;
    }

    return Hash;
}

UINT32
EFIAPI
RtlHashString (
    IN PSTRING  String,
    IN BOOLEAN  CaseInSensitive
    )
{
    UINT32 Hash = 0;
    UINTN  Index;

    for (Index = 0; Index < String->Length; Index++) {
        CHAR8 Ch = String->Buffer[Index];

        if (CaseInSensitive) {
            Ch = AnsiToUpper(Ch);
        }

        Hash = ((Hash << 5) + Hash) + Ch;
    }

    return Hash;
}
