/** @file
  NTRTL - NT Runtime Library

  MSVC C++ Name Demangling

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

/*
 * MSVC C++ Name Demangler
 *
 * MSVC uses a proprietary mangling scheme different from Itanium ABI.
 * Mangled names start with '?' followed by encoding.
 *
 * Format: ?<name>@<scope>@<qualifiers><type>
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
 * Parse identifier (up to '@')
 */
STATIC
VOID
ParseIdentifier(
    DEMANGLE_STATE *State
    )
{
    while (*State->Input && *State->Input != '@') {
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
        case 'X': Append(State, "void"); break;
        case 'D': Append(State, "char"); break;
        case 'E': Append(State, "unsigned char"); break;
        case 'F': Append(State, "short"); break;
        case 'G': Append(State, "unsigned short"); break;
        case 'H': Append(State, "int"); break;
        case 'I': Append(State, "unsigned int"); break;
        case 'J': Append(State, "long"); break;
        case 'K': Append(State, "unsigned long"); break;
        case '_': /* Extended types */
            if (*State->Input) {
                TypeChar = *State->Input++;
                switch (TypeChar) {
                    case 'J': Append(State, "long long"); break;
                    case 'K': Append(State, "unsigned long long"); break;
                    case 'N': Append(State, "bool"); break;
                    case 'W': Append(State, "wchar_t"); break;
                    default: Append(State, "<extended>"); break;
                }
            }
            break;
        case 'M': Append(State, "float"); break;
        case 'N': Append(State, "double"); break;
        case 'O': Append(State, "long double"); break;
        case 'P': /* Pointer */
            if (*State->Input == 'E') {
                State->Input++;
                Append(State, "const ");
            }
            ParseType(State);
            AppendChar(State, '*');
            break;
        case 'A': /* Reference */
            if (*State->Input == 'E') {
                State->Input++;
                Append(State, "const ");
            }
            ParseType(State);
            AppendChar(State, '&');
            break;
        case 'Q': /* const */
            Append(State, "const ");
            ParseType(State);
            break;
        case 'R': /* volatile */
            Append(State, "volatile ");
            ParseType(State);
            break;
        case 'V': /* Class/struct */
            ParseIdentifier(State);
            break;
        case 'U': /* Union */
            Append(State, "union ");
            ParseIdentifier(State);
            break;
        case 'T': /* Union */
            Append(State, "union ");
            ParseIdentifier(State);
            break;
        case 'Y': /* Array */
            Append(State, "<array>");
            /* Skip array encoding */
            while (*State->Input && *State->Input != '@') {
                State->Input++;
            }
            break;
        case '?': /* Template */
            Append(State, "<template>");
            break;
        default:
            Append(State, "<type>");
            break;
    }
}

/*
 * Parse scope (namespace/class qualifiers)
 */
STATIC
VOID
ParseScope(
    DEMANGLE_STATE *State
    )
{
    BOOLEAN First = TRUE;

    while (*State->Input && *State->Input != '@') {
        if (*State->Input == '@') {
            State->Input++;
            if (!First) {
                Append(State, "::");
            }
            First = FALSE;
        } else {
            ParseIdentifier(State);
            if (*State->Input == '@') {
                State->Input++;
                if (*State->Input && *State->Input != '@') {
                    Append(State, "::");
                }
            }
            break;
        }
    }

    /* Skip remaining '@' */
    while (*State->Input == '@') {
        State->Input++;
    }
}

/*
 * Parse function parameters
 */
STATIC
VOID
ParseParameters(
    DEMANGLE_STATE *State
    )
{
    BOOLEAN First = TRUE;

    AppendChar(State, '(');

    while (*State->Input && *State->Input != '@' && *State->Input != 'Z') {
        if (!First) {
            Append(State, ", ");
        }
        First = FALSE;

        if (*State->Input == 'X') {
            /* void - no parameters */
            State->Input++;
            break;
        }

        ParseType(State);
    }

    AppendChar(State, ')');

    /* Skip to end */
    while (*State->Input && *State->Input != 'Z') {
        State->Input++;
    }
}

/**
 * RtlDemangleNameMSVC - Demangle MSVC mangled name
 *
 * @param MangledName   - Mangled name string
 * @param DemangledName - Output buffer
 * @param BufferSize    - Output buffer size
 *
 * @return Length of demangled name, or 0 on error
 */
UINTN
EFIAPI
RtlDemangleNameMSVC(
    const CHAR8 *MangledName,
    CHAR8 *DemangledName,
    UINTN BufferSize
    )
{
    DEMANGLE_STATE State;

    if (MangledName == NULL || DemangledName == NULL || BufferSize == 0) {
        return 0;
    }

    /* Check for MSVC mangling */
    if (MangledName[0] != '?') {
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
    State.Input = MangledName + 1; /* Skip '?' */
    State.Output = DemangledName;
    State.OutputSize = BufferSize;
    State.OutputPos = 0;
    State.Error = FALSE;

    /* Parse name */
    ParseIdentifier(&State);

    /* Skip '@' and parse scope */
    if (*State.Input == '@') {
        State.Input++;
        ParseScope(&State);
    }

    /* Parse qualifiers and type */
    if (*State.Input) {
        /* Skip access specifiers (Y = public, Q = protected, etc.) */
        if (*State.Input == 'Y' || *State.Input == 'Q' ||
            *State.Input == 'I' || *State.Input == 'A' ||
            *State.Input == 'U' || *State.Input == 'V') {
            State.Input++;
            /* Skip calling convention */
            if (*State.Input == 'A' || *State.Input == 'E' ||
                *State.Input == 'I' || *State.Input == 'G') {
                State.Input++;
            }
        }

        /* Parse function parameters if this looks like a function */
        if (*State.Input && *State.Input != 'Z') {
            ParseParameters(&State);
        }
    }

    /* Null-terminate */
    State.Output[State.OutputPos] = '\0';

    return State.Error ? 0 : State.OutputPos;
}
