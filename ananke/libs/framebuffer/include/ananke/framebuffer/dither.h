/*++
    Module Name:

        dither.h

    Abstract:

        Classic Macintosh dithering patterns.
        Based on the original 1-bit Macintosh screen dithering
        used in QuickDraw and early Mac OS.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/framebuffer.h>

/* --------------------------------------------------------------- */
/*  Classic Mac Dither Patterns                                     */
/* --------------------------------------------------------------- */

/*
 * The classic Macintosh used 8x8 pattern matrices for dithering
 * grayscale values on its 1-bit display. These patterns create
 * the illusion of different gray levels through spatial dithering.
 */

/* 8x8 dither pattern threshold matrix (Bayer-style) */
extern CONST UINT8 gMacDitherPattern[8][8];

/* Dither pattern for specific gray levels (0-16) */
extern CONST UINT64 gMacGrayPatterns[17];

/* --------------------------------------------------------------- */
/*  Dithering Functions                                             */
/* --------------------------------------------------------------- */

/*
 * Apply classic Mac dithering to determine if a pixel should be
 * set or cleared based on grayscale intensity and position.
 *
 * Parameters:
 *   X, Y      - Pixel coordinates
 *   Intensity - Grayscale intensity (0-255)
 *
 * Returns:
 *   TRUE if pixel should be set, FALSE otherwise
 */
BOOLEAN
FbDitherClassicMac(
    IN UINT32 X,
    IN UINT32 Y,
    IN UINT8 Intensity
    );

/*
 * Get the dither pattern for a specific gray level (0-16).
 * Each pattern is a 64-bit value representing an 8x8 bitmap.
 */
UINT64
FbGetMacGrayPattern(
    IN UINT8 GrayLevel  /* 0 (black) to 16 (white) */
    );

/*
 * Apply dithering when converting RGB color to lower bit depth.
 * Uses ordered dithering with the classic Mac pattern matrix.
 */
VOID
FbDitherRgb(
    IN UINT32 X,
    IN UINT32 Y,
    IN OUT FB_COLOR *Color,
    IN FB_PIXEL_FORMAT TargetFormat
    );

/* --------------------------------------------------------------- */
/*  Advanced Monochrome Color Matching                             */
/* --------------------------------------------------------------- */

/*
 * Error diffusion dithering algorithms for better quality.
 * These require a buffer to accumulate error for the next scanline.
 */

typedef enum _FB_DITHER_ALGORITHM {
    FbDitherOrderedBayer = 0,      /* Ordered dithering (Bayer matrix) */
    FbDitherFloydSteinberg = 1,    /* Floyd-Steinberg error diffusion */
    FbDitherAtkinson = 2,          /* Atkinson dithering (Mac aesthetic) */
    FbDitherSierra = 3,            /* Sierra error diffusion */
    FbDitherBurkes = 4,            /* Burkes error diffusion */
    FbDitherStucki = 5,            /* Stucki error diffusion */
} FB_DITHER_ALGORITHM;

/*
 * Error diffusion buffer for storing quantization errors.
 * Must be allocated and maintained across scanlines.
 */
typedef struct _FB_ERROR_BUFFER {
    INT16  *Errors;        /* Error buffer (3 channels × width) */
    UINT32 Width;          /* Buffer width in pixels */
    UINT32 CurrentLine;    /* Current scanline number */
} FB_ERROR_BUFFER;

/*
 * Create an error diffusion buffer for a given width.
 * Returns NULL on allocation failure.
 */
FB_ERROR_BUFFER *
FbCreateErrorBuffer(
    IN UINT32 Width
    );

/*
 * Free an error diffusion buffer.
 */
VOID
FbFreeErrorBuffer(
    IN FB_ERROR_BUFFER *Buffer
    );

/*
 * Reset error buffer (clear all accumulated errors).
 * Call this when starting a new frame.
 */
VOID
FbResetErrorBuffer(
    IN FB_ERROR_BUFFER *Buffer
    );

/*
 * Convert RGBA color to monochrome (0 or 255) using error diffusion.
 * This produces superior results compared to ordered dithering for photographic images.
 *
 * Parameters:
 *   X, Y         - Pixel coordinates
 *   Color        - Input RGBA color (will be modified to monochrome output)
 *   Algorithm    - Dithering algorithm to use
 *   ErrorBuffer  - Error diffusion buffer (must persist across calls)
 *
 * Returns:
 *   TRUE on success, FALSE if error buffer is invalid
 */
BOOLEAN
FbRgbaToMonochromeWithDither(
    IN UINT32 X,
    IN UINT32 Y,
    IN OUT FB_COLOR *Color,
    IN FB_DITHER_ALGORITHM Algorithm,
    IN OUT FB_ERROR_BUFFER *ErrorBuffer
    );

/*
 * Compute perceptual color distance between two RGB colors.
 * Uses a fast approximation of CIE76 delta-E.
 *
 * Returns:
 *   Distance value (lower = more similar colors)
 */
UINT32
FbColorDistance(
    IN CONST FB_COLOR *Color1,
    IN CONST FB_COLOR *Color2
    );

/*
 * Find the closest color in a palette using perceptual color distance.
 *
 * Parameters:
 *   Color       - Input color to match
 *   Palette     - Array of palette colors
 *   PaletteSize - Number of colors in palette
 *
 * Returns:
 *   Index of the closest matching color
 */
UINT32
FbFindClosestColor(
    IN CONST FB_COLOR *Color,
    IN CONST FB_COLOR *Palette,
    IN UINT32 PaletteSize
    );
