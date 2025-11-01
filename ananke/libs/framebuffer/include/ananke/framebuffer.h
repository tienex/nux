/*++
    Module Name:

        framebuffer.h

    Abstract:

        Framebuffer library with COM-based backend interfaces.
        Supports multiple pixel formats, palette modes, dithering,
        and Unicode text rendering with BIDI support.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>

/* --------------------------------------------------------------- */
/*  Pixel Format Definitions                                        */
/* --------------------------------------------------------------- */

typedef enum _FB_PIXEL_FORMAT {
    FbPixelFormatInvalid      = 0,

    /* Monochrome formats */
    FbPixelFormat1Bpp         = 1,   /* 1-bit monochrome (Hercules, Mac, Atari mono) */
    FbPixelFormat2Bpp         = 2,   /* 2-bit grayscale (NeXT) */

    /* CGA formats */
    FbPixelFormatCga4Indexed  = 10,  /* CGA 4-color indexed (320x200) */
    FbPixelFormatCga2Mono     = 11,  /* CGA 2-color monochrome (640x200) */

    /* EGA/VGA planar formats */
    FbPixelFormatEga16Planar  = 20,  /* EGA 16-color planar (640x350) */
    FbPixelFormatVga16Planar  = 21,  /* VGA 16-color planar (640x480) */

    /* VGA linear indexed formats */
    FbPixelFormatVga256       = 30,  /* VGA Mode 13h (320x200x256) */
    FbPixelFormatIndexed256   = 31,  /* 8-bit indexed color (general) */
    FbPixelFormatIndexed16    = 32,  /* 4-bit indexed color */
    FbPixelFormatIndexed4     = 33,  /* 2-bit indexed color */

    /* RGB formats */
    FbPixelFormatRgb332       = 40,  /* 8-bit RGB (3:3:2) */
    FbPixelFormatRgb555       = 41,  /* 15-bit RGB (5:5:5) */
    FbPixelFormatRgb565       = 42,  /* 16-bit RGB (5:6:5) */
    FbPixelFormatRgb888       = 43,  /* 24-bit RGB (8:8:8) */
    FbPixelFormatRgba8888     = 44,  /* 32-bit RGBA (8:8:8:8) */
    FbPixelFormatBgr888       = 45,  /* 24-bit BGR (8:8:8) - Apple quirk */
    FbPixelFormatBgra8888     = 46,  /* 32-bit BGRA (8:8:8:8) */

    /* Amiga formats */
    FbPixelFormatAmigaOcs     = 50,  /* Amiga OCS planar (up to 32 colors) */
    FbPixelFormatAmigaEcs     = 51,  /* Amiga ECS planar (up to 64 colors) */
    FbPixelFormatAmigaAga     = 52,  /* Amiga AGA chunky (8-bit) or HAM8 */

    /* Atari formats */
    FbPixelFormatAtariSt      = 60,  /* Atari ST 16-color planar */
    FbPixelFormatAtariTt      = 61,  /* Atari TT high color */
    FbPixelFormatAtariFalcon  = 62,  /* Atari Falcon true color */

    /* Sun SPARC formats */
    FbPixelFormatSunCgThree   = 70,  /* Sun cgthree 8-bit indexed */
    FbPixelFormatSunCgSix     = 71,  /* Sun cgsix 8-bit indexed */

    /* SGI formats */
    FbPixelFormatSgiRgb       = 80,  /* SGI RGB format */

    /* Acorn VIDC formats */
    FbPixelFormatVidc         = 90,  /* Acorn VIDC palette modes */
} FB_PIXEL_FORMAT;

/* --------------------------------------------------------------- */
/*  Framebuffer Descriptor                                          */
/* --------------------------------------------------------------- */

typedef struct _FRAMEBUFFER_DESC {
    FB_PIXEL_FORMAT PixelFormat;
    UINT32          Width;          /* Width in pixels */
    UINT32          Height;         /* Height in pixels */
    UINT32          Pitch;          /* Bytes per scanline */
    UINT64          PhysicalBase;   /* Physical address */
    UINT64          Size;           /* Total size in bytes */

    /* RGB bit masks (for RGB modes) */
    UINT32          RedMask;
    UINT32          GreenMask;
    UINT32          BlueMask;

    /* Planar mode info (for VGA16) */
    UINT32          PlaneStride;    /* Bytes between planes */
} FRAMEBUFFER_DESC;

/* --------------------------------------------------------------- */
/*  Color Definitions                                               */
/* --------------------------------------------------------------- */

typedef struct _FB_COLOR {
    UINT8 Red;
    UINT8 Green;
    UINT8 Blue;
    UINT8 Alpha;
} FB_COLOR;

typedef struct _FB_PALETTE_ENTRY {
    UINT8 Red;
    UINT8 Green;
    UINT8 Blue;
    UINT8 Reserved;
} FB_PALETTE_ENTRY;

/* --------------------------------------------------------------- */
/*  Rectangle Definition                                            */
/* --------------------------------------------------------------- */

typedef struct _FB_RECT {
    INT32 Left;
    INT32 Top;
    INT32 Right;
    INT32 Bottom;
} FB_RECT;

/* --------------------------------------------------------------- */
/*  Text Direction (for BIDI support)                               */
/* --------------------------------------------------------------- */

typedef enum _FB_TEXT_DIRECTION {
    FbTextDirectionLTR = 0,  /* Left-to-right (Latin, Cyrillic, etc.) */
    FbTextDirectionRTL = 1,  /* Right-to-left (Arabic, Hebrew) */
} FB_TEXT_DIRECTION;

/* --------------------------------------------------------------- */
/*  Dithering Algorithm Selection                                   */
/* --------------------------------------------------------------- */

typedef enum _FB_DITHER_METHOD {
    FbDitherNone       = 0,  /* No dithering */
    FbDitherClassicMac = 1,  /* Classic Macintosh pattern dithering */
} FB_DITHER_METHOD;

/* --------------------------------------------------------------- */
/*  IFramebufferBackend - Main drawing interface                    */
/* --------------------------------------------------------------- */

#define ANX_IID_IFramebufferBackend "FB000001-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferBackend,
    0xFB000001, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferBackend, IUnknown,
    IID_IFramebufferBackend, ANX_IID_IFramebufferBackend)

    /* Initialize backend with framebuffer descriptor */
    ANX_IFACE_METHOD(HRESULT, Initialize, (
        IN CONST FRAMEBUFFER_DESC *Descriptor))

    /* Reset/clear the framebuffer */
    ANX_IFACE_METHOD(HRESULT, Clear, (
        IN FB_COLOR Color))

    /* Set a single pixel */
    ANX_IFACE_METHOD(HRESULT, SetPixel, (
        IN INT32 X,
        IN INT32 Y,
        IN FB_COLOR Color))

    /* Get a single pixel */
    ANX_IFACE_METHOD(HRESULT, GetPixel, (
        IN INT32 X,
        IN INT32 Y,
        OUT FB_COLOR *Color))

    /* Fill a rectangle */
    ANX_IFACE_METHOD(HRESULT, FillRect, (
        IN CONST FB_RECT *Rect,
        IN FB_COLOR Color))

    /* Blit a bitmap (1-bit per pixel) */
    ANX_IFACE_METHOD(HRESULT, BlitMonoBitmap, (
        IN INT32 X,
        IN INT32 Y,
        IN UINT32 Width,
        IN UINT32 Height,
        IN CONST UINT8 *Bitmap,
        IN FB_COLOR Foreground,
        IN FB_COLOR Background))

    /* Blit a full-color bitmap */
    ANX_IFACE_METHOD(HRESULT, BlitBitmap, (
        IN INT32 X,
        IN INT32 Y,
        IN UINT32 Width,
        IN UINT32 Height,
        IN CONST UINT8 *Bitmap,
        IN FB_PIXEL_FORMAT SourceFormat))

    /* Get framebuffer descriptor */
    ANX_IFACE_METHOD(HRESULT, GetDescriptor, (
        OUT FRAMEBUFFER_DESC *Descriptor))

    /* Set dithering method */
    ANX_IFACE_METHOD(HRESULT, SetDitherMethod, (
        IN FB_DITHER_METHOD Method))

ANX_END_INTERFACE(IFramebufferBackend)

/* --------------------------------------------------------------- */
/*  IFramebufferPalette - Palette management interface              */
/* --------------------------------------------------------------- */

#define ANX_IID_IFramebufferPalette "FB000002-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferPalette,
    0xFB000002, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferPalette, IUnknown,
    IID_IFramebufferPalette, ANX_IID_IFramebufferPalette)

    /* Set palette entry */
    ANX_IFACE_METHOD(HRESULT, SetPaletteEntry, (
        IN UINT8 Index,
        IN FB_PALETTE_ENTRY Entry))

    /* Get palette entry */
    ANX_IFACE_METHOD(HRESULT, GetPaletteEntry, (
        IN UINT8 Index,
        OUT FB_PALETTE_ENTRY *Entry))

    /* Set entire palette (256 entries) */
    ANX_IFACE_METHOD(HRESULT, SetPalette, (
        IN CONST FB_PALETTE_ENTRY *Palette,
        IN UINT32 Count))

    /* Get entire palette */
    ANX_IFACE_METHOD(HRESULT, GetPalette, (
        OUT FB_PALETTE_ENTRY *Palette,
        IN UINT32 Count))

    /* Load standard VGA palette */
    ANX_IFACE_METHOD(HRESULT, LoadStandardVgaPalette, (
        VOID))

ANX_END_INTERFACE(IFramebufferPalette)

/* --------------------------------------------------------------- */
/*  IFramebufferText - Unicode text rendering with BIDI             */
/* --------------------------------------------------------------- */

#define ANX_IID_IFramebufferText "FB000003-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferText,
    0xFB000003, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferText, IUnknown,
    IID_IFramebufferText, ANX_IID_IFramebufferText)

    /* Draw a single Unicode character */
    ANX_IFACE_METHOD(HRESULT, DrawChar, (
        IN INT32 X,
        IN INT32 Y,
        IN CHAR16 Character,
        IN FB_COLOR Foreground,
        IN FB_COLOR Background))

    /* Draw a Unicode string */
    ANX_IFACE_METHOD(HRESULT, DrawString, (
        IN INT32 X,
        IN INT32 Y,
        IN CONST CHAR16 *String,
        IN UINTN Length,
        IN FB_COLOR Foreground,
        IN FB_COLOR Background,
        IN FB_TEXT_DIRECTION Direction))

    /* Measure string width in pixels */
    ANX_IFACE_METHOD(HRESULT, MeasureString, (
        IN CONST CHAR16 *String,
        IN UINTN Length,
        OUT UINT32 *Width,
        OUT UINT32 *Height))

    /* Get font dimensions */
    ANX_IFACE_METHOD(HRESULT, GetFontMetrics, (
        OUT UINT32 *CharWidth,
        OUT UINT32 *CharHeight))

ANX_END_INTERFACE(IFramebufferText)

/* --------------------------------------------------------------- */
/*  Helper Macros for C (COBJMACROS style)                          */
/* --------------------------------------------------------------- */

#ifndef __cplusplus

#define IFramebufferBackend_Initialize(This, Desc) \
    ((This)->lpVtbl->Initialize(This, Desc))
#define IFramebufferBackend_Clear(This, Color) \
    ((This)->lpVtbl->Clear(This, Color))
#define IFramebufferBackend_SetPixel(This, X, Y, Color) \
    ((This)->lpVtbl->SetPixel(This, X, Y, Color))
#define IFramebufferBackend_GetPixel(This, X, Y, Color) \
    ((This)->lpVtbl->GetPixel(This, X, Y, Color))
#define IFramebufferBackend_FillRect(This, Rect, Color) \
    ((This)->lpVtbl->FillRect(This, Rect, Color))
#define IFramebufferBackend_BlitMonoBitmap(This, X, Y, W, H, Bmp, Fg, Bg) \
    ((This)->lpVtbl->BlitMonoBitmap(This, X, Y, W, H, Bmp, Fg, Bg))
#define IFramebufferBackend_BlitBitmap(This, X, Y, W, H, Bmp, Fmt) \
    ((This)->lpVtbl->BlitBitmap(This, X, Y, W, H, Bmp, Fmt))
#define IFramebufferBackend_GetDescriptor(This, Desc) \
    ((This)->lpVtbl->GetDescriptor(This, Desc))
#define IFramebufferBackend_SetDitherMethod(This, Method) \
    ((This)->lpVtbl->SetDitherMethod(This, Method))

#define IFramebufferPalette_SetPaletteEntry(This, Idx, Entry) \
    ((This)->lpVtbl->SetPaletteEntry(This, Idx, Entry))
#define IFramebufferPalette_GetPaletteEntry(This, Idx, Entry) \
    ((This)->lpVtbl->GetPaletteEntry(This, Idx, Entry))
#define IFramebufferPalette_SetPalette(This, Pal, Count) \
    ((This)->lpVtbl->SetPalette(This, Pal, Count))
#define IFramebufferPalette_GetPalette(This, Pal, Count) \
    ((This)->lpVtbl->GetPalette(This, Pal, Count))
#define IFramebufferPalette_LoadStandardVgaPalette(This) \
    ((This)->lpVtbl->LoadStandardVgaPalette(This))

#define IFramebufferText_DrawChar(This, X, Y, Ch, Fg, Bg) \
    ((This)->lpVtbl->DrawChar(This, X, Y, Ch, Fg, Bg))
#define IFramebufferText_DrawString(This, X, Y, Str, Len, Fg, Bg, Dir) \
    ((This)->lpVtbl->DrawString(This, X, Y, Str, Len, Fg, Bg, Dir))
#define IFramebufferText_MeasureString(This, Str, Len, W, H) \
    ((This)->lpVtbl->MeasureString(This, Str, Len, W, H))
#define IFramebufferText_GetFontMetrics(This, W, H) \
    ((This)->lpVtbl->GetFontMetrics(This, W, H))

#endif /* !__cplusplus */
