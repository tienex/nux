/*++
    Module Name:

        bidi.h

    Abstract:

        Bidirectional text (BIDI) support for Unicode text rendering.
        Implements Unicode Bidirectional Algorithm (UAX #9) subset
        for proper rendering of RTL text (Arabic, Hebrew).

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>

/* --------------------------------------------------------------- */
/*  BIDI Character Classes                                          */
/* --------------------------------------------------------------- */

typedef enum _BIDI_CLASS {
    BidiClassL    = 0,  /* Left-to-Right */
    BidiClassR    = 1,  /* Right-to-Left */
    BidiClassAL   = 2,  /* Arabic Letter (strong RTL) */
    BidiClassEN   = 3,  /* European Number */
    BidiClassAN   = 4,  /* Arabic Number */
    BidiClassCS   = 5,  /* Common Separator */
    BidiClassWS   = 6,  /* White Space */
    BidiClassON   = 7,  /* Other Neutral */
} BIDI_CLASS;

/* --------------------------------------------------------------- */
/*  BIDI Text Direction                                             */
/* --------------------------------------------------------------- */

typedef enum _BIDI_DIRECTION {
    BidiDirectionLTR = 0,
    BidiDirectionRTL = 1,
} BIDI_DIRECTION;

/* --------------------------------------------------------------- */
/*  BIDI Functions                                                  */
/* --------------------------------------------------------------- */

/*
 * Get the BIDI character class for a Unicode codepoint.
 * This is a simplified version covering common ranges.
 */
BIDI_CLASS
FbGetBidiClass(
    IN CHAR16 Character
    );

/*
 * Determine the overall text direction for a string.
 * Returns the dominant direction based on strong directional characters.
 */
BIDI_DIRECTION
FbDetectTextDirection(
    IN CONST CHAR16 *String,
    IN UINTN Length
    );

/*
 * Reorder a string for visual display according to BIDI rules.
 * This implements a simplified BIDI algorithm suitable for single-line text.
 *
 * Parameters:
 *   Input       - Input logical string
 *   Length      - Length of input string
 *   Output      - Buffer for reordered visual string (must be >= Length)
 *   BaseDir     - Base paragraph direction
 *
 * Returns:
 *   Number of characters written to Output
 */
UINTN
FbReorderBidiString(
    IN CONST CHAR16 *Input,
    IN UINTN Length,
    OUT CHAR16 *Output,
    IN BIDI_DIRECTION BaseDir
    );

/*
 * Check if a character needs mirroring when displayed RTL.
 * For example, '(' becomes ')' in RTL context.
 */
BOOLEAN
FbNeedsMirroring(
    IN CHAR16 Character
    );

/*
 * Get the mirrored version of a character.
 */
CHAR16
FbGetMirroredChar(
    IN CHAR16 Character
    );
