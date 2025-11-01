/*++
    Module Name:

        pixelformat.h

    Abstract:

        Pixel format conversion utilities.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/framebuffer.h>

/* --------------------------------------------------------------- */
/*  Color Conversion Functions                                      */
/* --------------------------------------------------------------- */

/*
 * Pack RGB888 color into specific pixel format
 */
UINT32
FbPackPixel(
    IN FB_COLOR Color,
    IN FB_PIXEL_FORMAT Format,
    IN UINT32 RedMask,
    IN UINT32 GreenMask,
    IN UINT32 BlueMask
    );

/*
 * Unpack pixel from specific format to RGB888
 */
FB_COLOR
FbUnpackPixel(
    IN UINT32 Pixel,
    IN FB_PIXEL_FORMAT Format,
    IN UINT32 RedMask,
    IN UINT32 GreenMask,
    IN UINT32 BlueMask
    );

/*
 * Convert RGB to grayscale
 */
UINT8
FbRgbToGray(
    IN FB_COLOR Color
    );

/*
 * Find closest palette entry for a given color
 */
UINT8
FbFindClosestPaletteEntry(
    IN FB_COLOR Color,
    IN CONST FB_PALETTE_ENTRY *Palette,
    IN UINT32 PaletteSize
    );

/*
 * Get standard VGA color for 16-color mode
 */
UINT8
FbGetVga16Color(
    IN FB_COLOR Color
    );

/* --------------------------------------------------------------- */
/*  Bit Manipulation Helpers                                        */
/* --------------------------------------------------------------- */

/*
 * Count leading zeros (used for mask processing)
 */
UINT32
FbCountLeadingZeros(
    IN UINT32 Value
    );

/*
 * Count trailing zeros (used for mask alignment)
 */
UINT32
FbCountTrailingZeros(
    IN UINT32 Value
    );

/*
 * Get bit field position and size from mask
 */
VOID
FbGetMaskInfo(
    IN UINT32 Mask,
    OUT UINT8 *Position,
    OUT UINT8 *Size
    );
