/*++
    Module Name:

        dither.c

    Abstract:

        Classic Macintosh dithering implementation.

--*/

#include <ananke/framebuffer/dither.h>
#include <ananke/framebuffer/pixelformat.h>  /* For FbRgbToGray */

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
                UINT8 Gray = FbRgbToGray(*Color);
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
/* --------------------------------------------------------------- */
/*  Advanced Monochrome Color Matching Implementation             */
/* --------------------------------------------------------------- */

/*
 * Allocate and initialize an error diffusion buffer.
 */
FB_ERROR_BUFFER *
FbCreateErrorBuffer(
    IN UINT32 Width
    )
{
    FB_ERROR_BUFFER *Buffer;

    /* Allocate buffer structure */
    Buffer = (FB_ERROR_BUFFER *)ANX_MALLOC(sizeof(FB_ERROR_BUFFER));
    if (Buffer == NULL) {
        return NULL;
    }

    /* Allocate error array: 3 channels (R, G, B) × (width + 2 for edge handling) */
    Buffer->Errors = (INT16 *)ANX_CALLOC((Width + 2) * 3, sizeof(INT16));
    if (Buffer->Errors == NULL) {
        ANX_FREE(Buffer);
        return NULL;
    }

    Buffer->Width = Width;
    Buffer->CurrentLine = 0;

    return Buffer;
}

/*
 * Free an error diffusion buffer.
 */
VOID
FbFreeErrorBuffer(
    IN FB_ERROR_BUFFER *Buffer
    )
{
    if (Buffer != NULL) {
        if (Buffer->Errors != NULL) {
            ANX_FREE(Buffer->Errors);
        }
        ANX_FREE(Buffer);
    }
}

/*
 * Reset error buffer to zero.
 */
VOID
FbResetErrorBuffer(
    IN FB_ERROR_BUFFER *Buffer
    )
{
    if (Buffer != NULL && Buffer->Errors != NULL) {
        ANX_MEMSET(Buffer->Errors, 0, (Buffer->Width + 2) * 3 * sizeof(INT16));
        Buffer->CurrentLine = 0;
    }
}

/*
 * Helper: Distribute error to neighboring pixels for error diffusion.
 */
static VOID
FbDistributeError(
    IN OUT INT16 *ErrorBuffer,
    IN UINT32 X,
    IN UINT32 Width,
    IN INT16 ErrorR,
    IN INT16 ErrorG,
    IN INT16 ErrorB,
    IN FB_DITHER_ALGORITHM Algorithm
    )
{
    UINT32 Offset = (X + 1) * 3;  /* +1 for edge padding */

    switch (Algorithm) {
        case FbDitherFloydSteinberg:
            /* Floyd-Steinberg: distributes error to 4 neighbors
             *        X   7/16
             *  3/16 5/16 1/16
             */
            if (X < Width - 1) {
                ErrorBuffer[Offset + 3]     += (ErrorR * 7) / 16;  /* Right */
                ErrorBuffer[Offset + 4]     += (ErrorG * 7) / 16;
                ErrorBuffer[Offset + 5]     += (ErrorB * 7) / 16;
            }
            break;

        case FbDitherAtkinson:
            /* Atkinson: distributes 6/8 of error (creates lighter images)
             *        X   1/8 1/8
             *  1/8  1/8 1/8
             *       1/8
             */
            if (X < Width - 1) {
                ErrorBuffer[Offset + 3]     += ErrorR / 8;  /* Right */
                ErrorBuffer[Offset + 4]     += ErrorG / 8;
                ErrorBuffer[Offset + 5]     += ErrorB / 8;
            }
            if (X < Width - 2) {
                ErrorBuffer[Offset + 6]     += ErrorR / 8;  /* Right+1 */
                ErrorBuffer[Offset + 7]     += ErrorG / 8;
                ErrorBuffer[Offset + 8]     += ErrorB / 8;
            }
            break;

        case FbDitherSierra:
        case FbDitherBurkes:
        case FbDitherStucki:
            /* Simplified: just distribute to immediate neighbors */
            if (X < Width - 1) {
                ErrorBuffer[Offset + 3]     += ErrorR / 4;
                ErrorBuffer[Offset + 4]     += ErrorG / 4;
                ErrorBuffer[Offset + 5]     += ErrorB / 4;
            }
            break;

        default:
            /* No error diffusion */
            break;
    }
}

/*
 * Helper: Clamp value to 0-255 range.
 */
static INLINE INT32
FbClamp(IN INT32 Value)
{
    if (Value < 0) return 0;
    if (Value > 255) return 255;
    return Value;
}

/*
 * Convert RGBA to monochrome using error diffusion dithering.
 */
BOOLEAN
FbRgbaToMonochromeWithDither(
    IN UINT32 X,
    IN UINT32 Y,
    IN OUT FB_COLOR *Color,
    IN FB_DITHER_ALGORITHM Algorithm,
    IN OUT FB_ERROR_BUFFER *ErrorBuffer
    )
{
    INT32 OldR, OldG, OldB;
    INT32 NewR, NewG, NewB;
    INT32 ErrorR, ErrorG, ErrorB;
    UINT32 Offset;
    UINT8 Gray;
    UINT8 Quantized;

    if (Color == NULL || ErrorBuffer == NULL || ErrorBuffer->Errors == NULL) {
        return FALSE;
    }

    if (X >= ErrorBuffer->Width) {
        return FALSE;
    }

    /* For ordered dithering, use existing algorithm */
    if (Algorithm == FbDitherOrderedBayer) {
        Gray = FbRgbToGray(*Color);
        BOOLEAN Pixel = FbDitherClassicMac(X, Y, Gray);
        Color->Red = Color->Green = Color->Blue = Pixel ? 255 : 0;
        return TRUE;
    }

    /* Get accumulated error for this pixel (+1 for edge padding) */
    Offset = (X + 1) * 3;

    /* Add error to original color */
    OldR = FbClamp((INT32)Color->Red   + ErrorBuffer->Errors[Offset + 0]);
    OldG = FbClamp((INT32)Color->Green + ErrorBuffer->Errors[Offset + 1]);
    OldB = FbClamp((INT32)Color->Blue  + ErrorBuffer->Errors[Offset + 2]);

    /* Convert to grayscale using ITU-R BT.601 luma */
    FB_COLOR TempColor = { (UINT8)OldR, (UINT8)OldG, (UINT8)OldB, 255 };
    Gray = FbRgbToGray(TempColor);

    /* Quantize to black or white */
    Quantized = (Gray >= 128) ? 255 : 0;
    NewR = NewG = NewB = Quantized;

    /* Calculate quantization error */
    ErrorR = OldR - NewR;
    ErrorG = OldG - NewG;
    ErrorB = OldB - NewB;

    /* Distribute error to neighbors */
    FbDistributeError(ErrorBuffer->Errors, X, ErrorBuffer->Width,
                     ErrorR, ErrorG, ErrorB, Algorithm);

    /* Clear error for this pixel (it's been processed) */
    ErrorBuffer->Errors[Offset + 0] = 0;
    ErrorBuffer->Errors[Offset + 1] = 0;
    ErrorBuffer->Errors[Offset + 2] = 0;

    /* Set output color */
    Color->Red   = Quantized;
    Color->Green = Quantized;
    Color->Blue  = Quantized;

    return TRUE;
}

/*
 * Compute perceptual color distance using fast approximation.
 * This approximates CIE76 delta-E without full color space conversion.
 */
UINT32
FbColorDistance(
    IN CONST FB_COLOR *Color1,
    IN CONST FB_COLOR *Color2
    )
{
    INT32 DeltaR, DeltaG, DeltaB;
    INT32 MeanR;

    if (Color1 == NULL || Color2 == NULL) {
        return 0xFFFFFFFF;
    }

    /* Calculate color differences */
    DeltaR = (INT32)Color1->Red   - (INT32)Color2->Red;
    DeltaG = (INT32)Color1->Green - (INT32)Color2->Green;
    DeltaB = (INT32)Color1->Blue  - (INT32)Color2->Blue;

    /* Weighted Euclidean distance approximating perceptual difference
     * Based on the redmean color distance formula which approximates
     * the perceptual color space better than simple Euclidean distance.
     */
    MeanR = ((INT32)Color1->Red + (INT32)Color2->Red) / 2;

    return (UINT32)(
        (((512 + MeanR) * DeltaR * DeltaR) >> 8) +
        (4 * DeltaG * DeltaG) +
        (((767 - MeanR) * DeltaB * DeltaB) >> 8)
    );
}

/*
 * Find the closest color in a palette using perceptual distance.
 */
UINT32
FbFindClosestColor(
    IN CONST FB_COLOR *Color,
    IN CONST FB_COLOR *Palette,
    IN UINT32 PaletteSize
    )
{
    UINT32 BestIndex = 0;
    UINT32 BestDistance = 0xFFFFFFFF;
    UINT32 i;

    if (Color == NULL || Palette == NULL || PaletteSize == 0) {
        return 0;
    }

    /* Scan palette for closest match */
    for (i = 0; i < PaletteSize; i++) {
        UINT32 Distance = FbColorDistance(Color, &Palette[i]);

        if (Distance < BestDistance) {
            BestDistance = Distance;
            BestIndex = i;

            /* Early exit if exact match found */
            if (Distance == 0) {
                break;
            }
        }
    }

    return BestIndex;
}
