/*++
    Module Name:

        bidi.c

    Abstract:

        Simplified BIDI implementation for common scripts.

--*/

#include <ananke/framebuffer/bidi.h>

/* --------------------------------------------------------------- */
/*  BIDI Character Classification                                   */
/* --------------------------------------------------------------- */

BIDI_CLASS
FbGetBidiClass(
    IN CHAR16 Character
    )
{
    /* ASCII Latin letters and most punctuation */
    if ((Character >= 0x0041 && Character <= 0x005A) ||  /* A-Z */
        (Character >= 0x0061 && Character <= 0x007A)) {  /* a-z */
        return BidiClassL;
    }

    /* Arabic (0x0600-0x06FF) */
    if (Character >= 0x0600 && Character <= 0x06FF) {
        return BidiClassAL;
    }

    /* Hebrew (0x0590-0x05FF) */
    if (Character >= 0x0590 && Character <= 0x05FF) {
        return BidiClassR;
    }

    /* ASCII digits */
    if (Character >= 0x0030 && Character <= 0x0039) {  /* 0-9 */
        return BidiClassEN;
    }

    /* Arabic-Indic digits (0x0660-0x0669) */
    if (Character >= 0x0660 && Character <= 0x0669) {
        return BidiClassAN;
    }

    /* Whitespace */
    if (Character == 0x0020 || Character == 0x0009 ||  /* space, tab */
        Character == 0x000A || Character == 0x000D) {  /* LF, CR */
        return BidiClassWS;
    }

    /* Common separators */
    if (Character == 0x002C || Character == 0x002E ||  /* comma, period */
        Character == 0x003A || Character == 0x003B) {  /* colon, semicolon */
        return BidiClassCS;
    }

    /* Cyrillic (LTR) */
    if (Character >= 0x0400 && Character <= 0x04FF) {
        return BidiClassL;
    }

    /* Default: neutral */
    return BidiClassON;
}

BIDI_DIRECTION
FbDetectTextDirection(
    IN CONST CHAR16 *String,
    IN UINTN Length
    )
{
    UINTN i;
    UINT32 LtrCount = 0;
    UINT32 RtlCount = 0;
    BIDI_CLASS Class;

    for (i = 0; i < Length; i++) {
        Class = FbGetBidiClass(String[i]);

        if (Class == BidiClassL) {
            LtrCount++;
        } else if (Class == BidiClassR || Class == BidiClassAL) {
            RtlCount++;
        }
    }

    /* Return dominant direction */
    return (RtlCount > LtrCount) ? BidiDirectionRTL : BidiDirectionLTR;
}

/* --------------------------------------------------------------- */
/*  Character Mirroring                                             */
/* --------------------------------------------------------------- */

typedef struct _MIRROR_PAIR {
    CHAR16 Original;
    CHAR16 Mirrored;
} MIRROR_PAIR;

static CONST MIRROR_PAIR gMirrorPairs[] = {
    { 0x0028, 0x0029 },  /* ( ) */
    { 0x0029, 0x0028 },  /* ) ( */
    { 0x003C, 0x003E },  /* < > */
    { 0x003E, 0x003C },  /* > < */
    { 0x005B, 0x005D },  /* [ ] */
    { 0x005D, 0x005B },  /* ] [ */
    { 0x007B, 0x007D },  /* { } */
    { 0x007D, 0x007B },  /* } { */
    { 0x00AB, 0x00BB },  /* « » */
    { 0x00BB, 0x00AB },  /* » « */
    { 0x2039, 0x203A },  /* ‹ › */
    { 0x203A, 0x2039 },  /* › ‹ */
};

BOOLEAN
FbNeedsMirroring(
    IN CHAR16 Character
    )
{
    UINTN i;

    for (i = 0; i < sizeof(gMirrorPairs) / sizeof(gMirrorPairs[0]); i++) {
        if (gMirrorPairs[i].Original == Character) {
            return TRUE;
        }
    }

    return FALSE;
}

CHAR16
FbGetMirroredChar(
    IN CHAR16 Character
    )
{
    UINTN i;

    for (i = 0; i < sizeof(gMirrorPairs) / sizeof(gMirrorPairs[0]); i++) {
        if (gMirrorPairs[i].Original == Character) {
            return gMirrorPairs[i].Mirrored;
        }
    }

    return Character;
}

/* --------------------------------------------------------------- */
/*  Simplified BIDI Reordering                                      */
/* --------------------------------------------------------------- */

UINTN
FbReorderBidiString(
    IN CONST CHAR16 *Input,
    IN UINTN Length,
    OUT CHAR16 *Output,
    IN BIDI_DIRECTION BaseDir
    )
{
    UINTN i, j;
    UINTN RunStart, RunEnd;
    BIDI_CLASS Class;

    if (Input == NULL || Output == NULL || Length == 0) {
        return 0;
    }

    /* For LTR base direction, simple copy with mirroring if needed */
    if (BaseDir == BidiDirectionLTR) {
        for (i = 0; i < Length; i++) {
            Output[i] = Input[i];
        }
        return Length;
    }

    /* For RTL base direction, reverse runs of RTL text */
    i = 0;
    j = 0;

    while (i < Length) {
        Class = FbGetBidiClass(Input[i]);

        /* Handle RTL runs */
        if (Class == BidiClassR || Class == BidiClassAL) {
            RunStart = i;

            /* Find end of RTL run */
            while (i < Length) {
                Class = FbGetBidiClass(Input[i]);
                if (Class != BidiClassR && Class != BidiClassAL &&
                    Class != BidiClassAN && Class != BidiClassEN &&
                    Class != BidiClassCS && Class != BidiClassON) {
                    break;
                }
                i++;
            }

            RunEnd = i;

            /* Reverse the run */
            for (; RunEnd > RunStart; RunEnd--) {
                CHAR16 Ch = Input[RunEnd - 1];
                if (FbNeedsMirroring(Ch)) {
                    Ch = FbGetMirroredChar(Ch);
                }
                Output[j++] = Ch;
            }
        }
        /* Handle LTR runs */
        else if (Class == BidiClassL) {
            RunStart = i;

            /* Find end of LTR run */
            while (i < Length && FbGetBidiClass(Input[i]) == BidiClassL) {
                i++;
            }

            /* Copy LTR run as-is */
            for (; RunStart < i; RunStart++) {
                Output[j++] = Input[RunStart];
            }
        }
        /* Handle neutral characters */
        else {
            Output[j++] = Input[i++];
        }
    }

    return j;
}
