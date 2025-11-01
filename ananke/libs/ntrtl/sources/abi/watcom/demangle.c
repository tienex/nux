/** @file
  NTRTL - NT Runtime Library

  Watcom/Borland C++ Name Demangling

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

/*
 * Watcom/Borland C++ Name Demangler
 *
 * Watcom and Borland use similar mangling schemes.
 * Mangled names typically start with '@' followed by class name,
 * then '$' and member name, then parameter encoding.
 *
 * Format: @<class>$<member><qualifiers><params>
 */

#include <ananke/base.h>

#define MAX_OUTPUT 4096

typedef struct {
    const CHAR8 *Input;
    CHAR8 *Output;
    UINTN OutputSize;
    UINTN OutputPos;
    BOOLEAN Error;
} DEMANGLE_STATE;

/*
 * Append string to output
 */
STATIC
VOID
Append(
    DEMANGLE_STATE *State,
    const CHAR8 *Str
    )
{
    while (*Str && State->OutputPos < State->OutputSize - 1) {
        State->Output[State->OutputPos++] = *Str++;
    }
}

/*
 * Append character to output
 */
STATIC
VOID
AppendChar(
    DEMANGLE_STATE *State,
    CHAR8 Ch
    )
{
    if (State->OutputPos < State->OutputSize - 1) {
        State->Output[State->OutputPos++] = Ch;
    }
}

/*
 * Parse identifier (up to delimiter)
 */
STATIC
VOID
ParseIdentifier(
    DEMANGLE_STATE *State,
    CHAR8 Delimiter
    )
{
    while (*State->Input && *State->Input != Delimiter) {
        AppendChar(State, *State->Input++);
    }
}

/*
 * Parse basic type
 */
STATIC
VOID
ParseType(
    DEMANGLE_STATE *State
    )
{
    CHAR8 TypeChar;

    if (!*State->Input) {
        return;
    }

    TypeChar = *State->Input++;

    switch (TypeChar) {
        case 'v': Append(State, "void"); break;
        case 'c': Append(State, "char"); break;
        case 's': Append(State, "short"); break;
        case 'i': Append(State, "int"); break;
        case 'l': Append(State, "long"); break;
        case 'f': Append(State, "float"); break;
        case 'd': Append(State, "double"); break;
        case 'r': Append(State, "long double"); break;
        case 'u': /* Unsigned */
            Append(State, "unsigned ");
            ParseType(State);
            break;
        case 'p': /* Pointer */
            ParseType(State);
            AppendChar(State, '*');
            break;
        case 'r': /* Reference */
            ParseType(State);
            AppendChar(State, '&');
            break;
        case 'x': /* Const */
            Append(State, "const ");
            ParseType(State);
            break;
        case 'w': /* Volatile */
            Append(State, "volatile ");
            ParseType(State);
            break;
        default:
            if (TypeChar >= 'A' && TypeChar <= 'Z') {
                /* Class name */
                State->Input--;
                ParseIdentifier(State, '$');
            } else {
                Append(State, "<type>");
            }
            break;
    }
}

/*
 * Parse parameters
 */
STATIC
VOID
ParseParameters(
    DEMANGLE_STATE *State
    )
{
    BOOLEAN First = TRUE;

    AppendChar(State, '(');

    while (*State->Input && *State->Input != '$' && *State->Input != '\0') {
        if (!First) {
            Append(State, ", ");
        }
        First = FALSE;

        ParseType(State);
    }

    AppendChar(State, ')');
}

/**
 * RtlDemangleNameWatcom - Demangle Watcom/Borland mangled name
 *
 * @param MangledName   - Mangled name string
 * @param DemangledName - Output buffer
 * @param BufferSize    - Output buffer size
 *
 * @return Length of demangled name, or 0 on error
 */
UINTN
EFIAPI
RtlDemangleNameWatcom(
    const CHAR8 *MangledName,
    CHAR8 *DemangledName,
    UINTN BufferSize
    )
{
    DEMANGLE_STATE State;
    BOOLEAN IsMethod = FALSE;

    if (MangledName == NULL || DemangledName == NULL || BufferSize == 0) {
        return 0;
    }

    /* Check for Watcom/Borland mangling */
    if (MangledName[0] != '@' && MangledName[0] != '_') {
        /* Not mangled, copy as-is */
        UINTN i = 0;
        while (MangledName[i] && i < BufferSize - 1) {
            DemangledName[i] = MangledName[i];
            i++;
        }
        DemangledName[i] = '\0';
        return i;
    }

    /* Initialize state */
    State.Input = MangledName;
    State.Output = DemangledName;
    State.OutputSize = BufferSize;
    State.OutputPos = 0;
    State.Error = FALSE;

    /* Skip leading '@' or '_' */
    if (*State.Input == '@' || *State.Input == '_') {
        State.Input++;
        IsMethod = (*MangledName == '@');
    }

    /* Parse class name if method */
    if (IsMethod) {
        ParseIdentifier(&State, '$');
        if (*State.Input == '$') {
            State.Input++;
            Append(&State, "::");
        }
    }

    /* Parse member name */
    ParseIdentifier(&State, '$');

    /* Parse parameters if present */
    if (*State.Input == '$') {
        State.Input++;
        /* Skip qualifiers */
        while (*State.Input == 'x' || *State.Input == 'w') {
            State.Input++;
        }
        ParseParameters(&State);
    }

    /* Null-terminate */
    State.Output[State.OutputPos] = '\0';

    return State.Error ? 0 : State.OutputPos;
}
