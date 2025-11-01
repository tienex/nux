/*++
    Module Name:

        dither.c

    Abstract:

        Classic Macintosh dithering implementation.

--*/

#include <ananke/framebuffer/dither.h>

/* --------------------------------------------------------------- */
/*  Classic Mac Dither Pattern Matrix                              */
/* --------------------------------------------------------------- */

/*
 * 8x8 Bayer-style threshold matrix used by classic Macintosh.
 * Values range from 0-64, representing threshold levels.
 */
CONST UINT8 gMacDitherPattern[8][8] = {
    {  0, 32,  8, 40,  2, 34, 10, 42 },
    { 48, 16, 56, 24, 50, 18, 58, 26 },
    { 12, 44,  4, 36, 14, 46,  6, 38 },
    { 60, 28, 52, 20, 62, 30, 54, 22 },
    {  3, 35, 11, 43,  1, 33,  9, 41 },
    { 51, 19, 59, 27, 49, 17, 57, 25 },
    { 15, 47,  7, 39, 13, 45,  5, 37 },
    { 63, 31, 55, 23, 61, 29, 53, 21 }
};

/* --------------------------------------------------------------- */
/*  Classic Mac Gray Patterns (0-16)                                */
/* --------------------------------------------------------------- */

/*
 * These are the actual dither patterns used by classic Mac OS
 * for representing 17 levels of gray (0=black, 16=white) on
 * 1-bit displays. Each 64-bit value represents an 8x8 pattern.
 */
CONST UINT64 gMacGrayPatterns[17] = {
    0x0000000000000000ULL,  /*  0/16 - solid black */
    0x8800220088002200ULL,  /*  1/16 */
    0xAA00AA00AA00AA00ULL,  /*  2/16 */
    0xAA44AA11AA44AA11ULL,  /*  3/16 */
    0xAA55AA55AA55AA55ULL,  /*  4/16 */
    0xAA55AA77AA55AA77ULL,  /*  5/16 */
    0xBB55EE55BB55EE55ULL,  /*  6/16 */
    0xBB77EE77BB77EE77ULL,  /*  7/16 */
    0xFF55FF55FF55FF55ULL,  /*  8/16 - 50% gray */
    0xFF77FF77FF77FF77ULL,  /*  9/16 */
    0xFF77FFDDFF77FFDDULL,  /* 10/16 */
    0xFFDDFF77FFDDFF77ULL,  /* 11/16 */
    0xFFDDFFDDFFDDFFDDULL,  /* 12/16 */
    0xFFDDFFFFFFDDFFFFULL,  /* 13/16 */
    0xFFFFFFDDFFFFFFDDULL,  /* 14/16 */
    0xFFFFFFFFFF77FF77ULL,  /* 15/16 */
    0xFFFFFFFFFFFFFFFFULL,  /* 16/16 - solid white */
};

/* --------------------------------------------------------------- */
/*  Implementation                                                  */
/* --------------------------------------------------------------- */

BOOLEAN
FbDitherClassicMac(
    IN UINT32 X,
    IN UINT32 Y,
    IN UINT8 Intensity
    )
{
    UINT32 PatternX;
    UINT32 PatternY;
    UINT8 Threshold;

    /* Get position within 8x8 pattern */
    PatternX = X & 7;
    PatternY = Y & 7;

    /* Get threshold from pattern matrix */
    Threshold = gMacDitherPattern[PatternY][PatternX];

    /* Scale 0-255 intensity to 0-64 range for comparison */
    return ((Intensity >> 2) > Threshold);
}

UINT64
FbGetMacGrayPattern(
    IN UINT8 GrayLevel
    )
{
    if (GrayLevel > 16) {
        GrayLevel = 16;
    }

    return gMacGrayPatterns[GrayLevel];
}

VOID
FbDitherRgb(
    IN UINT32 X,
    IN UINT32 Y,
    IN OUT FB_COLOR *Color,
    IN FB_PIXEL_FORMAT TargetFormat
    )
{
    UINT32 PatternX;
    UINT32 PatternY;
    UINT8 Threshold;
    INT32 DitherError;

    if (Color == NULL) {
        return;
    }

    /* Get position within 8x8 pattern */
    PatternX = X & 7;
    PatternY = Y & 7;
    Threshold = gMacDitherPattern[PatternY][PatternX];

    /* Apply dithering based on target format */
    switch (TargetFormat) {
        case FbPixelFormat1Bpp:
            /* Convert to grayscale and apply threshold */
            {
                UINT8 Gray = (UINT8)((Color->Red * 77 + Color->Green * 150 + Color->Blue * 29) >> 8);
                BOOLEAN Pixel = FbDitherClassicMac(X, Y, Gray);
                Color->Red = Color->Green = Color->Blue = Pixel ? 255 : 0;
            }
            break;

        case FbPixelFormatRgb555:
            /* Dither to 5 bits per channel */
            DitherError = (Threshold - 32) >> 1;
            Color->Red   = (UINT8)((((INT32)Color->Red   + DitherError) >> 3) << 3);
            Color->Green = (UINT8)((((INT32)Color->Green + DitherError) >> 3) << 3);
            Color->Blue  = (UINT8)((((INT32)Color->Blue  + DitherError) >> 3) << 3);
            break;

        case FbPixelFormatRgb565:
            /* Dither red and blue to 5 bits, green to 6 bits */
            DitherError = (Threshold - 32) >> 1;
            Color->Red   = (UINT8)((((INT32)Color->Red   + DitherError) >> 3) << 3);
            Color->Green = (UINT8)((((INT32)Color->Green + DitherError) >> 2) << 2);
            Color->Blue  = (UINT8)((((INT32)Color->Blue  + DitherError) >> 3) << 3);
            break;

        case FbPixelFormatVga16Planar:
        case FbPixelFormatIndexed256:
            /* For indexed formats, dithering is applied during palette matching */
            /* This is handled in the palette conversion code */
            break;

        default:
            /* No dithering for other formats */
            break;
    }
}
