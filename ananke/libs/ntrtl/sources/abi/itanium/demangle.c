/** @file
  NTRTL - NT Runtime Library

  Itanium C++ ABI Name Demangling

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

/*
 * Itanium C++ ABI Demangler (GCC/Clang/ICC)
 *
 * Based on the Itanium C++ ABI specification:
 * https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling
 *
 * Mangled names start with "_Z" followed by encoding.
 * Uses recursive descent parser with substitution table.
 */

#include <ananke/base.h>

#define MAX_SUBS 128
#define MAX_OUTPUT 4096

typedef struct {
    const CHAR8 *Input;
    const CHAR8 *InputStart;
    CHAR8 *Output;
    UINTN OutputSize;
    UINTN OutputPos;
    const CHAR8 *Subs[MAX_SUBS];
    UINTN SubsCount;
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
 * Parse decimal number
 */
STATIC
UINTN
ParseNumber(
    DEMANGLE_STATE *State
    )
{
    UINTN Result = 0;

    if (*State->Input < '0' || *State->Input > '9') {
        return 0;
    }

    while (*State->Input >= '0' && *State->Input <= '9') {
        Result = Result * 10 + (*State->Input - '0');
        State->Input++;
    }

    return Result;
}

/*
 * Add to substitution table
 */
STATIC
VOID
AddSubstitution(
    DEMANGLE_STATE *State,
    const CHAR8 *Pos
    )
{
    if (State->SubsCount < MAX_SUBS) {
        State->Subs[State->SubsCount++] = Pos;
    }
}

/*
 * Parse source name (length-prefixed identifier)
 */
STATIC
VOID
ParseSourceName(
    DEMANGLE_STATE *State
    )
{
    UINTN Length;
    UINTN i;

    Length = ParseNumber(State);
    if (Length == 0) {
        State->Error = TRUE;
        return;
    }

    for (i = 0; i < Length && *State->Input; i++) {
        AppendChar(State, *State->Input++);
    }

    if (i != Length) {
        State->Error = TRUE;
    }
}

/*
 * Forward declarations
 */
STATIC VOID ParseType(DEMANGLE_STATE *State);
STATIC VOID ParseName(DEMANGLE_STATE *State);

/*
 * Parse builtin type
 */
STATIC
VOID
ParseBuiltinType(
    DEMANGLE_STATE *State
    )
{
    CHAR8 TypeChar = *State->Input++;

    switch (TypeChar) {
        case 'v': Append(State, "void"); break;
        case 'w': Append(State, "wchar_t"); break;
        case 'b': Append(State, "bool"); break;
        case 'c': Append(State, "char"); break;
        case 'a': Append(State, "signed char"); break;
        case 'h': Append(State, "unsigned char"); break;
        case 's': Append(State, "short"); break;
        case 't': Append(State, "unsigned short"); break;
        case 'i': Append(State, "int"); break;
        case 'j': Append(State, "unsigned int"); break;
        case 'l': Append(State, "long"); break;
        case 'm': Append(State, "unsigned long"); break;
        case 'x': Append(State, "long long"); break;
        case 'y': Append(State, "unsigned long long"); break;
        case 'n': Append(State, "__int128"); break;
        case 'o': Append(State, "unsigned __int128"); break;
        case 'f': Append(State, "float"); break;
        case 'd': Append(State, "double"); break;
        case 'e': Append(State, "long double"); break;
        case 'g': Append(State, "__float128"); break;
        case 'z': Append(State, "..."); break;
        case 'D':
            State->Input++;
            switch (State->Input[-1]) {
                case 'n': Append(State, "decltype(nullptr)"); break;
                case 'a': Append(State, "auto"); break;
                default: Append(State, "<unknown>"); break;
            }
            break;
        default:
            Append(State, "<builtin>");
            break;
    }
}

/*
 * Parse substitution
 */
STATIC
VOID
ParseSubstitution(
    DEMANGLE_STATE *State
    )
{
    UINTN Index;

    State->Input++; /* Skip 'S' */

    if (*State->Input == '_') {
        /* S_ = first substitution */
        Index = 0;
        State->Input++;
    } else if (*State->Input >= '0' && *State->Input <= '9') {
        /* S<seq-id>_ */
        Index = 0;
        while (*State->Input != '_') {
            if (*State->Input >= '0' && *State->Input <= '9') {
                Index = Index * 36 + (*State->Input - '0');
            } else if (*State->Input >= 'A' && *State->Input <= 'Z') {
                Index = Index * 36 + (*State->Input - 'A' + 10);
            } else {
                break;
            }
            State->Input++;
        }
        Index++; /* seq-id is offset by 1 */
        if (*State->Input == '_') {
            State->Input++;
        }
    } else {
        /* Standard substitutions */
        switch (*State->Input++) {
            case 't': Append(State, "std"); return;
            case 'a': Append(State, "std::allocator"); return;
            case 'b': Append(State, "std::basic_string"); return;
            case 's': Append(State, "std::string"); return;
            case 'i': Append(State, "std::istream"); return;
            case 'o': Append(State, "std::ostream"); return;
            case 'd': Append(State, "std::iostream"); return;
            default: Append(State, "<subst>"); return;
        }
    }

    /* Look up substitution */
    if (Index < State->SubsCount) {
        Append(State, "<sub>");
    } else {
        State->Error = TRUE;
    }
}

/*
 * Parse template args
 */
STATIC
VOID
ParseTemplateArgs(
    DEMANGLE_STATE *State
    )
{
    BOOLEAN First = TRUE;

    if (*State->Input != 'I') {
        return;
    }

    State->Input++; /* Skip 'I' */
    AppendChar(State, '<');

    while (*State->Input && *State->Input != 'E') {
        if (!First) {
            Append(State, ", ");
        }
        First = FALSE;

        if (*State->Input == 'L') {
            /* Literal */
            State->Input++;
            ParseType(State);
            /* Skip literal value */
            while (*State->Input && *State->Input != 'E') {
                State->Input++;
            }
        } else if (*State->Input == 'X') {
            /* Expression */
            State->Input++;
            Append(State, "<expr>");
            while (*State->Input && *State->Input != 'E') {
                State->Input++;
            }
        } else {
            ParseType(State);
        }
    }

    AppendChar(State, '>');

    if (*State->Input == 'E') {
        State->Input++;
    }
}

/*
 * Parse type
 */
STATIC
VOID
ParseType(
    DEMANGLE_STATE *State
    )
{
    CHAR8 TypeChar;

    if (!*State->Input) {
        State->Error = TRUE;
        return;
    }

    TypeChar = *State->Input;

    switch (TypeChar) {
        case 'P': /* Pointer */
            State->Input++;
            ParseType(State);
            AppendChar(State, '*');
            break;

        case 'R': /* L-value reference */
            State->Input++;
            ParseType(State);
            AppendChar(State, '&');
            break;

        case 'O': /* R-value reference */
            State->Input++;
            ParseType(State);
            Append(State, "&&");
            break;

        case 'K': /* Const */
            State->Input++;
            Append(State, "const ");
            ParseType(State);
            break;

        case 'V': /* Volatile */
            State->Input++;
            Append(State, "volatile ");
            ParseType(State);
            break;

        case 'r': /* Restrict */
            State->Input++;
            Append(State, "restrict ");
            ParseType(State);
            break;

        case 'S': /* Substitution */
            ParseSubstitution(State);
            break;

        case 'F': /* Function type */
            State->Input++;
            Append(State, "<function>");
            /* Skip to 'E' */
            while (*State->Input && *State->Input != 'E') {
                State->Input++;
            }
            if (*State->Input == 'E') {
                State->Input++;
            }
            break;

        case 'A': /* Array */
            State->Input++;
            Append(State, "<array>");
            /* Skip array bounds */
            while (*State->Input && *State->Input != '_') {
                State->Input++;
            }
            if (*State->Input == '_') {
                State->Input++;
            }
            ParseType(State);
            break;

        default:
            if (TypeChar >= '0' && TypeChar <= '9') {
                /* Class name */
                const CHAR8 *Start = State->Input;
                ParseSourceName(State);
                AddSubstitution(State, Start);
                ParseTemplateArgs(State);
            } else {
                ParseBuiltinType(State);
            }
            break;
    }
}

/*
 * Parse unqualified name
 */
STATIC
VOID
ParseUnqualifiedName(
    DEMANGLE_STATE *State
    )
{
    if (*State->Input >= '0' && *State->Input <= '9') {
        ParseSourceName(State);
    } else if (*State->Input == 'C' || *State->Input == 'D') {
        /* Constructor/Destructor */
        CHAR8 Op = *State->Input++;
        State->Input++; /* Skip variant */
        if (Op == 'D') {
            AppendChar(State, '~');
        }
        Append(State, "<ctor/dtor>");
    } else {
        Append(State, "<operator>");
        State->Input += 2;
    }
}

/*
 * Parse nested name
 */
STATIC
VOID
ParseNestedName(
    DEMANGLE_STATE *State
    )
{
    BOOLEAN First = TRUE;

    State->Input++; /* Skip 'N' */

    /* Skip CV-qualifiers */
    while (*State->Input == 'r' || *State->Input == 'V' || *State->Input == 'K') {
        State->Input++;
    }

    while (*State->Input && *State->Input != 'E') {
        if (!First) {
            Append(State, "::");
        }
        First = FALSE;

        if (*State->Input == 'S') {
            ParseSubstitution(State);
        } else if (*State->Input == 'I') {
            ParseTemplateArgs(State);
        } else {
            const CHAR8 *Start = State->Input;
            ParseUnqualifiedName(State);
            AddSubstitution(State, Start);
        }
    }

    if (*State->Input == 'E') {
        State->Input++;
    }
}

/*
 * Parse name (top-level)
 */
STATIC
VOID
ParseName(
    DEMANGLE_STATE *State
    )
{
    if (*State->Input == 'N') {
        ParseNestedName(State);
    } else if (*State->Input == 'S') {
        ParseSubstitution(State);
    } else if (*State->Input >= '0' && *State->Input <= '9') {
        ParseSourceName(State);
    } else {
        ParseUnqualifiedName(State);
    }
}

/*
 * Parse function encoding
 */
STATIC
VOID
ParseEncoding(
    DEMANGLE_STATE *State
    )
{
    ParseName(State);

    /* Parse function parameters if present */
    if (*State->Input && *State->Input != '\0') {
        BOOLEAN First = TRUE;
        AppendChar(State, '(');

        while (*State->Input && *State->Input != '\0') {
            if (!First) {
                Append(State, ", ");
            }
            First = FALSE;

            ParseType(State);
        }

        AppendChar(State, ')');
    }
}

/**
 * RtlDemangleNameItanium - Demangle Itanium C++ ABI mangled name
 *
 * @param MangledName   - Mangled name string
 * @param DemangledName - Output buffer
 * @param BufferSize    - Output buffer size
 *
 * @return Length of demangled name, or 0 on error
 */
UINTN
EFIAPI
RtlDemangleNameItanium(
    const CHAR8 *MangledName,
    CHAR8 *DemangledName,
    UINTN BufferSize
    )
{
    DEMANGLE_STATE State;

    if (MangledName == NULL || DemangledName == NULL || BufferSize == 0) {
        return 0;
    }

    /* Check for Itanium mangling */
    if (MangledName[0] != '_' || MangledName[1] != 'Z') {
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
    State.Input = MangledName + 2; /* Skip "_Z" */
    State.InputStart = MangledName;
    State.Output = DemangledName;
    State.OutputSize = BufferSize;
    State.OutputPos = 0;
    State.SubsCount = 0;
    State.Error = FALSE;

    /* Parse encoding */
    ParseEncoding(&State);

    /* Null-terminate */
    State.Output[State.OutputPos] = '\0';

    return State.Error ? 0 : State.OutputPos;
}
