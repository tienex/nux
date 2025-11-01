/*++
    Module Name:

        font.c

    Abstract:

        Font access implementation.

--*/

#include <ananke/framebuffer/font.h>

/* Font data from VGA ROM */
extern CONST UINT8 gFont8x8Data[];
extern CONST UINT8 gFont8x14Data[];
extern CONST UINT8 gFont8x16Data[];
extern CONST UINT8 gFontScrawlData[];

/* --------------------------------------------------------------- */
/*  Font Descriptors                                                */
/* --------------------------------------------------------------- */

CONST BITMAP_FONT gFont8x8 = {
    .Width      = 8,
    .Height     = 8,
    .FirstChar  = 0,
    .LastChar   = 255,
    .Glyphs     = gFont8x8Data,
};

CONST BITMAP_FONT gFont8x14 = {
    .Width      = 8,
    .Height     = 14,
    .FirstChar  = 0,
    .LastChar   = 255,
    .Glyphs     = gFont8x14Data,
};

CONST BITMAP_FONT gFont8x16 = {
    .Width      = 8,
    .Height     = 16,
    .FirstChar  = 0,
    .LastChar   = 255,
    .Glyphs     = gFont8x16Data,
};

CONST BITMAP_FONT gFontScrawl = {
    .Width      = 8,
    .Height     = 16,
    .FirstChar  = 0,
    .LastChar   = 255,
    .Glyphs     = gFontScrawlData,
};

/* --------------------------------------------------------------- */
/*  Implementation                                                  */
/* --------------------------------------------------------------- */

CONST UINT8 *
FbGetGlyph(
    IN CONST BITMAP_FONT *Font,
    IN CHAR16 Character
    )
{
    UINT32 Index;
    UINT32 GlyphSize;

    if (Font == NULL || Font->Glyphs == NULL) {
        return NULL;
    }

    /* Check if character is in range */
    if (Character < Font->FirstChar || Character > Font->LastChar) {
        /* Return glyph for '?' as fallback */
        Character = '?';
        if (Character < Font->FirstChar || Character > Font->LastChar) {
            return NULL;
        }
    }

    /* Calculate glyph offset */
    Index = Character - Font->FirstChar;
    GlyphSize = Font->Height * FbGetGlyphStride(Font);

    return &Font->Glyphs[Index * GlyphSize];
}

UINT32
FbGetGlyphStride(
    IN CONST BITMAP_FONT *Font
    )
{
    if (Font == NULL) {
        return 0;
    }

    /* For now, all fonts are 8 pixels wide = 1 byte per row */
    return (Font->Width + 7) / 8;
}
