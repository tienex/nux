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
