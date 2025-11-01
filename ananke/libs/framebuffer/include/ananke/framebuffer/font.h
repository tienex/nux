/*++
    Module Name:

        font.h

    Abstract:

        Bitmap font support for framebuffer text rendering.
        Supports VGA fonts (8x8, 8x14, 8x16) and Unicode fonts.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>

/* --------------------------------------------------------------- */
/*  Font Descriptor                                                 */
/* --------------------------------------------------------------- */

typedef struct _BITMAP_FONT {
    UINT8           Width;          /* Character width in pixels */
    UINT8           Height;         /* Character height in pixels */
    UINT16          FirstChar;      /* First character code */
    UINT16          LastChar;       /* Last character code */
    CONST UINT8     *Glyphs;        /* Glyph bitmap data */
} BITMAP_FONT;

/* --------------------------------------------------------------- */
/*  VGA Fonts (CP437 - Code Page 437)                               */
/* --------------------------------------------------------------- */

extern CONST BITMAP_FONT gFont8x8;
extern CONST BITMAP_FONT gFont8x14;
extern CONST BITMAP_FONT gFont8x16;

/* Scrawl font (alternate) */
extern CONST BITMAP_FONT gFontScrawl;

/* --------------------------------------------------------------- */
/*  Font Selection                                                  */
/* --------------------------------------------------------------- */

#ifndef FB_FONT_DEFAULT
#define FB_FONT_DEFAULT gFont8x16
#endif

/* --------------------------------------------------------------- */
/*  Font Access Functions                                           */
/* --------------------------------------------------------------- */

/*
 * Get glyph bitmap for a character.
 * Returns pointer to glyph data, or NULL if not available.
 */
CONST UINT8 *
FbGetGlyph(
    IN CONST BITMAP_FONT *Font,
    IN CHAR16 Character
    );

/*
 * Get glyph width in bytes.
 * For fonts where width <= 8, this is 1 byte per row.
 */
UINT32
FbGetGlyphStride(
    IN CONST BITMAP_FONT *Font
    );
