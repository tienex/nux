/*++
    Module Name:

        font.h

    Abstract:

        Bitmap font support for framebuffer text rendering.
        Supports VGA fonts (8x8, 8x14, 8x16), Unicode fonts, and PSF format.
        Includes COM interface for hardware font loading (VGA text mode).

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>

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

/* --------------------------------------------------------------- */
/*  Hardware Font Loading (VGA Text Mode)                          */
/* --------------------------------------------------------------- */

typedef enum _FB_FONT_TYPE {
    FbFontBitmap        = 0,  /* Bitmap font (monochrome) */
    FbFontVgaText       = 1,  /* VGA text mode font (8x8, 8x14, 8x16, 9x16) */
    FbFontPsf1          = 2,  /* PC Screen Font v1 */
    FbFontPsf2          = 3,  /* PC Screen Font v2 (with Unicode table) */
} FB_FONT_TYPE;

typedef struct _FB_FONT_DESC {
    FB_FONT_TYPE    Type;
    UINT32          Width;          /* Character width in pixels */
    UINT32          Height;         /* Character height in pixels */
    UINT32          CharCount;      /* Number of characters (usually 256 or 512) */
    UINT32          BytesPerChar;   /* Bytes per character glyph */
    BOOLEAN         HasUnicode;     /* Has Unicode mapping table */
} FB_FONT_DESC;

/* VGA text mode font sizes */
#define FB_FONT_8x8     0   /* 8x8 font (CGA/EGA) */
#define FB_FONT_8x14    1   /* 8x14 font (EGA) */
#define FB_FONT_8x16    2   /* 8x16 font (VGA) */
#define FB_FONT_9x16    3   /* 9x16 font (VGA with 9th column) */

/* --------------------------------------------------------------- */
/*  IFramebufferFont - Font interface                              */
/* --------------------------------------------------------------- */

#define ANX_IID_IFramebufferFont "FB000016-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferFont,
    0xFB000016, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferFont, IUnknown,
    IID_IFramebufferFont, ANX_IID_IFramebufferFont)

    /* Get font descriptor */
    ANX_IFACE_METHOD(HRESULT, GetDescriptor, (
        OUT FB_FONT_DESC *Descriptor))

    /* Load font from memory */
    ANX_IFACE_METHOD(HRESULT, LoadFromMemory, (
        IN CONST VOID *FontData,
        IN UINT32 DataSize,
        IN FB_FONT_TYPE Type))

    /* Load standard VGA font */
    ANX_IFACE_METHOD(HRESULT, LoadVgaFont, (
        IN UINT32 FontSize))  /* FB_FONT_8x8, FB_FONT_8x14, etc. */

    /* Get glyph bitmap for a character */
    ANX_IFACE_METHOD(HRESULT, GetGlyph, (
        IN UINT32 Character,
        OUT CONST UINT8 **GlyphData,
        OUT UINT32 *Width,
        OUT UINT32 *Height))

    /* Get glyph for Unicode codepoint (if font has Unicode table) */
    ANX_IFACE_METHOD(HRESULT, GetGlyphUnicode, (
        IN UINT32 Codepoint,
        OUT CONST UINT8 **GlyphData,
        OUT UINT32 *Width,
        OUT UINT32 *Height))

    /* Set hardware font (for VGA text mode) */
    ANX_IFACE_METHOD(HRESULT, SetHardwareFont, (
        IN UINT32 FontBank,  /* 0 or 1 for VGA dual-font support */
        IN CONST VOID *FontData,
        IN UINT32 CharCount,
        IN UINT32 CharHeight))

    /* Get hardware font (from VGA text mode) */
    ANX_IFACE_METHOD(HRESULT, GetHardwareFont, (
        IN UINT32 FontBank,
        OUT VOID *FontData,
        IN UINT32 MaxChars,
        OUT UINT32 *CharHeight))

ANX_END_INTERFACE(IFramebufferFont)

/* --------------------------------------------------------------- */
/*  PSF (PC Screen Font) Format Support                            */
/* --------------------------------------------------------------- */

/* PSF1 header */
#define PSF1_MAGIC0     0x36
#define PSF1_MAGIC1     0x04

typedef struct _PSF1_HEADER {
    UINT8   Magic[2];       /* 0x36, 0x04 */
    UINT8   Mode;           /* Font mode */
    UINT8   CharSize;       /* Character size (bytes per glyph) */
} PSF1_HEADER;

/* PSF2 header */
#define PSF2_MAGIC0     0x72
#define PSF2_MAGIC1     0xb5
#define PSF2_MAGIC2     0x4a
#define PSF2_MAGIC3     0x86

typedef struct _PSF2_HEADER {
    UINT8   Magic[4];       /* 0x72, 0xb5, 0x4a, 0x86 */
    UINT32  Version;        /* PSF version */
    UINT32  HeaderSize;     /* Size of header in bytes */
    UINT32  Flags;          /* Font flags */
    UINT32  Length;         /* Number of glyphs */
    UINT32  CharSize;       /* Bytes per glyph */
    UINT32  Height;         /* Character height in pixels */
    UINT32  Width;          /* Character width in pixels */
} PSF2_HEADER;

/* PSF2 flags */
#define PSF2_HAS_UNICODE_TABLE  0x01

/* Load PSF font from memory */
HRESULT FbLoadPsfFont(
    IN CONST VOID *FontData,
    IN UINT32 DataSize,
    OUT FB_FONT_DESC *Descriptor,
    OUT CONST UINT8 **GlyphData
);
