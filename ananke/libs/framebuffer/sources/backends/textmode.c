/*++
    Module Name:

        textmode.c

    Abstract:

        Text-mode graphics backend using libcaca-style techniques.
        Renders graphics using Unicode box drawing characters and ASCII art.

        Supports both monochrome (ASCII) and color (ANSI/VT100) text modes.
        This allows graphics rendering on serial consoles, SSH sessions,
        and other text-only environments.

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/backend_ext.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/framebuffer/dither.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Text Mode Definitions                                           */
/* --------------------------------------------------------------- */

#define TEXTMODE_MAX_WIDTH      160
#define TEXTMODE_MAX_HEIGHT     60

/* Character rendering modes */
typedef enum _TEXT_RENDER_MODE {
    TextRenderAscii         = 0,  /* ASCII art (7-bit) */
    TextRenderUnicode       = 1,  /* Unicode box drawing */
    TextRenderUnicodeBlocks = 2,  /* Unicode block characters */
} TEXT_RENDER_MODE;

/* Unicode block drawing characters */
#define UNICODE_UPPER_HALF      0x2580  /* ▀ */
#define UNICODE_LOWER_HALF      0x2584  /* ▄ */
#define UNICODE_LEFT_HALF       0x258C  /* ▌ */
#define UNICODE_RIGHT_HALF      0x2590  /* ▐ */
#define UNICODE_FULL_BLOCK      0x2588  /* █ */
#define UNICODE_LIGHT_SHADE     0x2591  /* ░ */
#define UNICODE_MEDIUM_SHADE    0x2592  /* ▒ */
#define UNICODE_DARK_SHADE      0x2593  /* ▓ */

/* ASCII approximations */
static CONST char gAsciiShades[] = " .'`^\",:;Il!i><~+_-?][}{1)(|/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";

/* --------------------------------------------------------------- */
/*  Text Cell Structure                                             */
/* --------------------------------------------------------------- */

typedef struct _TEXT_CELL {
    CHAR16  Character;
    UINT8   Foreground;     /* Color index 0-15 */
    UINT8   Background;     /* Color index 0-15 */
} TEXT_CELL;

/* --------------------------------------------------------------- */
/*  Text Mode Backend Structure                                     */
/* --------------------------------------------------------------- */

typedef struct _TEXTMODE_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;

    /* Text mode specific state */
    TEXT_RENDER_MODE            RenderMode;
    UINT32                      TextWidth;
    UINT32                      TextHeight;
    UINT32                      PixelsPerCharX;  /* Horizontal resolution per char */
    UINT32                      PixelsPerCharY;  /* Vertical resolution per char */

    /* Text buffer */
    TEXT_CELL                   *TextBuffer;

    /* Internal pixel buffer for rendering */
    UINT8                       *PixelBuffer;    /* Indexed color buffer */
    FB_PALETTE_ENTRY            Palette[16];     /* 16-color palette */
} TEXTMODE_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE TextmodeFb_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE TextmodeFb_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE TextmodeFb_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE TextmodeFb_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE TextmodeFb_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE TextmodeFb_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE TextmodeFb_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE TextmodeFb_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE TextmodeFb_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE TextmodeFb_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE TextmodeFb_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE TextmodeFb_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gTextmodeFbVtbl = {
    .QueryInterface     = TextmodeFb_QueryInterface,
    .AddRef             = TextmodeFb_AddRef,
    .Release            = TextmodeFb_Release,
    .Initialize         = TextmodeFb_Initialize,
    .Clear              = TextmodeFb_Clear,
    .SetPixel           = TextmodeFb_SetPixel,
    .GetPixel           = TextmodeFb_GetPixel,
    .FillRect           = TextmodeFb_FillRect,
    .BlitMonoBitmap     = TextmodeFb_BlitMonoBitmap,
    .BlitBitmap         = TextmodeFb_BlitBitmap,
    .GetDescriptor      = TextmodeFb_GetDescriptor,
    .SetDitherMethod    = TextmodeFb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  16-Color Text Mode Palette (VGA colors)                         */
/* --------------------------------------------------------------- */

static CONST FB_PALETTE_ENTRY gTextModePalette[16] = {
    { 0x00, 0x00, 0x00, 0 },  /* 0: Black */
    { 0x00, 0x00, 0xAA, 0 },  /* 1: Blue */
    { 0x00, 0xAA, 0x00, 0 },  /* 2: Green */
    { 0x00, 0xAA, 0xAA, 0 },  /* 3: Cyan */
    { 0xAA, 0x00, 0x00, 0 },  /* 4: Red */
    { 0xAA, 0x00, 0xAA, 0 },  /* 5: Magenta */
    { 0xAA, 0x55, 0x00, 0 },  /* 6: Brown */
    { 0xAA, 0xAA, 0xAA, 0 },  /* 7: Light Gray */
    { 0x55, 0x55, 0x55, 0 },  /* 8: Dark Gray */
    { 0x55, 0x55, 0xFF, 0 },  /* 9: Light Blue */
    { 0x55, 0xFF, 0x55, 0 },  /* 10: Light Green */
    { 0x55, 0xFF, 0xFF, 0 },  /* 11: Light Cyan */
    { 0xFF, 0x55, 0x55, 0 },  /* 12: Light Red */
    { 0xFF, 0x55, 0xFF, 0 },  /* 13: Light Magenta */
    { 0xFF, 0xFF, 0x55, 0 },  /* 14: Yellow */
    { 0xFF, 0xFF, 0xFF, 0 },  /* 15: White */
};

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static UINT8
TextmodeFb_MapColorToIndex(
    TEXTMODE_BACKEND *Backend,
    FB_COLOR Color
    )
{
    return FbFindClosestPaletteEntry(Color, Backend->Palette, 16);
}

static VOID
TextmodeFb_ConvertPixelBufferToText(
    TEXTMODE_BACKEND *Backend
    )
{
    /* Convert pixel buffer to text cells using Unicode block characters */

    for (UINT32 CharY = 0; CharY < Backend->TextHeight; CharY++) {
        for (UINT32 CharX = 0; CharX < Backend->TextWidth; CharX++) {
            /* Sample pixels in this character cell */
            UINT32 PixelX = CharX * Backend->PixelsPerCharX;
            UINT32 PixelY = CharY * Backend->PixelsPerCharY;

            if (Backend->RenderMode == TextRenderUnicodeBlocks && Backend->PixelsPerCharY >= 2) {
                /* Use half-block characters for 2:1 vertical resolution */
                UINT32 TopPixel = PixelY * Backend->Descriptor.Width + PixelX;
                UINT32 BottomPixel = (PixelY + 1) * Backend->Descriptor.Width + PixelX;

                UINT8 TopColor = Backend->PixelBuffer[TopPixel];
                UINT8 BottomColor = Backend->PixelBuffer[BottomPixel];

                TEXT_CELL *Cell = &Backend->TextBuffer[CharY * Backend->TextWidth + CharX];

                if (TopColor == BottomColor) {
                    Cell->Character = ' ';
                    Cell->Foreground = TopColor;
                    Cell->Background = TopColor;
                } else {
                    /* Use upper half block */
                    Cell->Character = UNICODE_UPPER_HALF;
                    Cell->Foreground = TopColor;
                    Cell->Background = BottomColor;
                }
            } else {
                /* Sample brightness and map to shade character */
                UINT32 SampleCount = 0;
                UINT32 BrightnessSum = 0;
                UINT8 AvgColor = 0;

                for (UINT32 dy = 0; dy < Backend->PixelsPerCharY; dy++) {
                    for (UINT32 dx = 0; dx < Backend->PixelsPerCharX; dx++) {
                        UINT32 SamplePixelX = PixelX + dx;
                        UINT32 SamplePixelY = PixelY + dy;

                        if (SamplePixelX < Backend->Descriptor.Width &&
                            SamplePixelY < Backend->Descriptor.Height) {
                            UINT32 Offset = SamplePixelY * Backend->Descriptor.Width + SamplePixelX;
                            UINT8 ColorIndex = Backend->PixelBuffer[Offset];

                            FB_COLOR Color = {
                                Backend->Palette[ColorIndex].Red,
                                Backend->Palette[ColorIndex].Green,
                                Backend->Palette[ColorIndex].Blue,
                                255
                            };

                            UINT8 Gray = FbRgbToGray(Color);
                            BrightnessSum += Gray;
                            SampleCount++;

                            if (SampleCount == 1) {
                                AvgColor = ColorIndex;
                            }
                        }
                    }
                }

                UINT8 AvgBrightness = (SampleCount > 0) ? (BrightnessSum / SampleCount) : 0;

                TEXT_CELL *Cell = &Backend->TextBuffer[CharY * Backend->TextWidth + CharX];

                if (Backend->RenderMode == TextRenderAscii) {
                    /* Map brightness to ASCII shade character */
                    UINT32 ShadeIndex = (AvgBrightness * (sizeof(gAsciiShades) - 1)) / 255;
                    Cell->Character = (CHAR16)gAsciiShades[ShadeIndex];
                    Cell->Foreground = 7;  /* Light gray */
                    Cell->Background = 0;  /* Black */
                } else {
                    /* Use Unicode shade blocks */
                    CHAR16 ShadeChar;
                    if (AvgBrightness < 64) {
                        ShadeChar = ' ';
                    } else if (AvgBrightness < 128) {
                        ShadeChar = UNICODE_LIGHT_SHADE;
                    } else if (AvgBrightness < 192) {
                        ShadeChar = UNICODE_MEDIUM_SHADE;
                    } else {
                        ShadeChar = UNICODE_DARK_SHADE;
                    }

                    Cell->Character = ShadeChar;
                    Cell->Foreground = AvgColor;
                    Cell->Background = 0;
                }
            }
        }
    }
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
TextmodeFb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    TEXTMODE_BACKEND *Backend = (TEXTMODE_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        TextmodeFb_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
TextmodeFb_AddRef(
    IFramebufferBackend *This
    )
{
    TEXTMODE_BACKEND *Backend = (TEXTMODE_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
TextmodeFb_Release(
    IFramebufferBackend *This
    )
{
    TEXTMODE_BACKEND *Backend = (TEXTMODE_BACKEND *)This;
    UINT32 RefCount = ANX_REF_DEC(&Backend->RefCount);

    if (RefCount == 0) {
        /* Cleanup - would free TextBuffer and PixelBuffer */
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  IFramebufferBackend Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
TextmodeFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    TEXTMODE_BACKEND *Backend = (TEXTMODE_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    Backend->Descriptor = *Descriptor;

    /* Calculate text mode dimensions (assume 80x25 default) */
    Backend->TextWidth = 80;
    Backend->TextHeight = 25;
    Backend->PixelsPerCharX = Backend->Descriptor.Width / Backend->TextWidth;
    Backend->PixelsPerCharY = Backend->Descriptor.Height / Backend->TextHeight;

    /* Default to Unicode blocks rendering */
    Backend->RenderMode = TextRenderUnicodeBlocks;

    /* Initialize palette */
    for (UINT32 i = 0; i < 16; i++) {
        Backend->Palette[i] = gTextModePalette[i];
    }

    /* Allocate buffers (static for now) */
    static TEXT_CELL sTextBuffer[TEXTMODE_MAX_WIDTH * TEXTMODE_MAX_HEIGHT];
    static UINT8 sPixelBuffer[640 * 480];  /* Max resolution */

    Backend->TextBuffer = sTextBuffer;
    Backend->PixelBuffer = sPixelBuffer;
    Backend->Initialized = TRUE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
TextmodeFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    TEXTMODE_BACKEND *Backend = (TEXTMODE_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    ColorIndex = TextmodeFb_MapColorToIndex(Backend, Color);

    /* Clear pixel buffer */
    UINT32 PixelCount = Backend->Descriptor.Width * Backend->Descriptor.Height;
    for (UINT32 i = 0; i < PixelCount; i++) {
        Backend->PixelBuffer[i] = ColorIndex;
    }

    /* Convert to text */
    TextmodeFb_ConvertPixelBufferToText(Backend);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
TextmodeFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    TEXTMODE_BACKEND *Backend = (TEXTMODE_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    ColorIndex = TextmodeFb_MapColorToIndex(Backend, Color);

    UINT32 Offset = Y * Backend->Descriptor.Width + X;
    Backend->PixelBuffer[Offset] = ColorIndex;

    /* Update affected text cell */
    UINT32 CharX = X / Backend->PixelsPerCharX;
    UINT32 CharY = Y / Backend->PixelsPerCharY;

    /* For now, just mark as needing update - full conversion done on flush */

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
TextmodeFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    TEXTMODE_BACKEND *Backend = (TEXTMODE_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    UINT32 Offset = Y * Backend->Descriptor.Width + X;
    ColorIndex = Backend->PixelBuffer[Offset];

    Color->Red = Backend->Palette[ColorIndex].Red;
    Color->Green = Backend->Palette[ColorIndex].Green;
    Color->Blue = Backend->Palette[ColorIndex].Blue;
    Color->Alpha = 255;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
TextmodeFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    TEXTMODE_BACKEND *Backend = (TEXTMODE_BACKEND *)This;
    UINT8 ColorIndex;
    INT32 x, y;

    if (!Backend->Initialized || Rect == NULL) {
        return E_POINTER;
    }

    ColorIndex = TextmodeFb_MapColorToIndex(Backend, Color);

    for (y = Rect->Top; y < Rect->Bottom && y < (INT32)Backend->Descriptor.Height; y++) {
        for (x = Rect->Left; x < Rect->Right && x < (INT32)Backend->Descriptor.Width; x++) {
            if (x >= 0 && y >= 0) {
                UINT32 Offset = y * Backend->Descriptor.Width + x;
                Backend->PixelBuffer[Offset] = ColorIndex;
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
TextmodeFb_BlitMonoBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_COLOR Foreground,
    FB_COLOR Background
    )
{
    TEXTMODE_BACKEND *Backend = (TEXTMODE_BACKEND *)This;
    UINT8 FgIndex, BgIndex;
    UINT32 Row, Col;
    UINT32 ByteIndex, BitIndex;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    FgIndex = TextmodeFb_MapColorToIndex(Backend, Foreground);
    BgIndex = TextmodeFb_MapColorToIndex(Backend, Background);

    for (Row = 0; Row < Height; Row++) {
        for (Col = 0; Col < Width; Col++) {
            ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
            BitIndex = 7 - (Col % 8);

            UINT8 ColorIndex = (Bitmap[ByteIndex] & (1 << BitIndex)) ? FgIndex : BgIndex;

            INT32 DestX = X + Col;
            INT32 DestY = Y + Row;

            if (DestX >= 0 && DestX < (INT32)Backend->Descriptor.Width &&
                DestY >= 0 && DestY < (INT32)Backend->Descriptor.Height) {
                UINT32 Offset = DestY * Backend->Descriptor.Width + DestX;
                Backend->PixelBuffer[Offset] = ColorIndex;
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
TextmodeFb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    TEXTMODE_BACKEND *Backend = (TEXTMODE_BACKEND *)This;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    /* Textmode backend uses indexed color pixel buffer */
    /* Convert source format to indexed colors and write to buffer */

    /* Fast path: indexed formats */
    if (SourceFormat == FbPixelFormatIndexed4 ||
        SourceFormat == FbPixelFormatIndexed16 ||
        SourceFormat == FbPixelFormatIndexed256) {

        for (UINT32 Row = 0; Row < Height; Row++) {
            INT32 DestY = Y + Row;
            if (DestY < 0 || DestY >= (INT32)Backend->Descriptor.Height) {
                continue;
            }

            for (UINT32 Col = 0; Col < Width; Col++) {
                INT32 DestX = X + Col;
                if (DestX < 0 || DestX >= (INT32)Backend->Descriptor.Width) {
                    continue;
                }

                UINT8 ColorIndex = 0;

                /* Extract color index based on format */
                if (SourceFormat == FbPixelFormatIndexed4) {
                    UINT32 ByteIdx = Row * ((Width + 1) / 2) + (Col / 2);
                    ColorIndex = (Col & 1) ?
                        (Bitmap[ByteIdx] & 0x0F) :
                        ((Bitmap[ByteIdx] >> 4) & 0x0F);
                } else if (SourceFormat == FbPixelFormatIndexed16) {
                    UINT32 ByteIdx = Row * Width + Col;
                    ColorIndex = Bitmap[ByteIdx] & 0x0F;
                } else {  /* FbPixelFormatIndexed256 */
                    UINT32 ByteIdx = Row * Width + Col;
                    ColorIndex = Bitmap[ByteIdx] & 0x0F;  /* Map to 16 colors */
                }

                /* Write to pixel buffer */
                UINT32 Offset = DestY * Backend->Descriptor.Width + DestX;
                Backend->PixelBuffer[Offset] = ColorIndex;
            }
        }
        return S_OK;
    }

    /* Format conversion handled by engine */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
TextmodeFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    TEXTMODE_BACKEND *Backend = (TEXTMODE_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
TextmodeFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    TEXTMODE_BACKEND *Backend = (TEXTMODE_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static TEXTMODE_BACKEND gTextmodeBackendInstance = {
    .Base.lpVtbl        = &gTextmodeFbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherClassicMac,
    .RenderMode         = TextRenderUnicodeBlocks,
};

IFramebufferBackend *
FbCreateTextmodeBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gTextmodeBackendInstance;
}
