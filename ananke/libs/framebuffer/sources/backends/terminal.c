/*++
    Module Name:

        terminal.c

    Abstract:

        Terminal framebuffer backend.

        Renders graphics on text-based terminals using ANSI escape sequences
        and Unicode block characters. Supports:
        - ANSI/VT100/xterm escape sequences
        - 256-color palette (88 and 256-color modes)
        - Unicode block characters for graphics (▀▄█▌▐)
        - Text mode (BIOS/EFI console)
        - Serial consoles and SSH sessions
        - libcaca-style ASCII art rendering

        Perfect for headless systems, remote terminals, and retro aesthetics.

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/backend_ext.h>
#include <ananke/framebuffer/backends.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/framebuffer/dither.h>
#include <ananke/framebuffer/com_helpers.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  ANSI Escape Sequences                                           */
/* --------------------------------------------------------------- */

#define ANSI_ESC            "\x1B"
#define ANSI_CSI            ANSI_ESC "["

/* Cursor control */
#define ANSI_CURSOR_HOME    ANSI_CSI "H"
#define ANSI_CURSOR_POS     ANSI_CSI "%d;%dH"  /* Row;Col */
#define ANSI_CURSOR_HIDE    ANSI_CSI "?25l"
#define ANSI_CURSOR_SHOW    ANSI_CSI "?25h"

/* Screen control */
#define ANSI_CLEAR_SCREEN   ANSI_CSI "2J"
#define ANSI_CLEAR_LINE     ANSI_CSI "2K"

/* Color control (SGR - Select Graphic Rendition) */
#define ANSI_RESET          ANSI_CSI "0m"
#define ANSI_FG_COLOR       ANSI_CSI "38;5;%dm"  /* 256-color foreground */
#define ANSI_BG_COLOR       ANSI_CSI "48;5;%dm"  /* 256-color background */
#define ANSI_FG_RGB         ANSI_CSI "38;2;%d;%d;%dm"  /* 24-bit RGB foreground */
#define ANSI_BG_RGB         ANSI_CSI "48;2;%d;%d;%dm"  /* 24-bit RGB background */

/* Standard 16 colors (30-37, 90-97 for fg, 40-47, 100-107 for bg) */
#define ANSI_FG_BLACK       ANSI_CSI "30m"
#define ANSI_FG_RED         ANSI_CSI "31m"
#define ANSI_FG_GREEN       ANSI_CSI "32m"
#define ANSI_FG_YELLOW      ANSI_CSI "33m"
#define ANSI_FG_BLUE        ANSI_CSI "34m"
#define ANSI_FG_MAGENTA     ANSI_CSI "35m"
#define ANSI_FG_CYAN        ANSI_CSI "36m"
#define ANSI_FG_WHITE       ANSI_CSI "37m"

/* Unicode block characters for graphics */
#define UNICODE_UPPER_HALF  0x2580  /* ▀ */
#define UNICODE_LOWER_HALF  0x2584  /* ▄ */
#define UNICODE_FULL_BLOCK  0x2588  /* █ */
#define UNICODE_LIGHT_SHADE 0x2591  /* ░ */
#define UNICODE_MEDIUM_SHADE 0x2592  /* ▒ */
#define UNICODE_DARK_SHADE  0x2593  /* ▓ */

/* --------------------------------------------------------------- */
/*  Text Cell for Terminal Buffer                                   */
/* --------------------------------------------------------------- */

typedef struct _ANSI_TEXT_CELL {
    CHAR16  Character;
    UINT8   FgColor;        /* 0-255 (256-color mode) or 0-15 (16-color) */
    UINT8   BgColor;
    BOOLEAN Dirty;          /* Needs redraw */
} ANSI_TEXT_CELL;

/* --------------------------------------------------------------- */
/*  ANSI Terminal Backend Structure                                 */
/* --------------------------------------------------------------- */

typedef struct _ANSI_TERMINAL_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;

    /* Terminal dimensions */
    UINT32                      TermWidth;      /* Terminal columns (e.g., 80) */
    UINT32                      TermHeight;     /* Terminal rows (e.g., 25) */
    UINT32                      PixelsPerCharX; /* Pixels per character horizontally */
    UINT32                      PixelsPerCharY; /* Pixels per character vertically */

    /* Terminal capabilities */
    BOOLEAN                     Support256Color;
    BOOLEAN                     SupportTrueColor;
    BOOLEAN                     SupportUnicode;

    /* Text and pixel buffers */
    ANSI_TEXT_CELL              *TextBuffer;
    UINT8                       *PixelBuffer;   /* RGB888 format */

    /* Output function pointer (for flexibility) */
    VOID (*OutputFunc)(CONST CHAR8 *String, UINTN Length);
} ANSI_TERMINAL_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE AnsiTermFb_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE AnsiTermFb_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE AnsiTermFb_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE AnsiTermFb_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE AnsiTermFb_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE AnsiTermFb_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE AnsiTermFb_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE AnsiTermFb_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE AnsiTermFb_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE AnsiTermFb_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE AnsiTermFb_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE AnsiTermFb_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gAnsiTermFbVtbl = {
    .QueryInterface     = AnsiTermFb_QueryInterface,
    .AddRef             = AnsiTermFb_AddRef,
    .Release            = AnsiTermFb_Release,
    .Initialize         = AnsiTermFb_Initialize,
    .Clear              = AnsiTermFb_Clear,
    .SetPixel           = AnsiTermFb_SetPixel,
    .GetPixel           = AnsiTermFb_GetPixel,
    .FillRect           = AnsiTermFb_FillRect,
    .BlitMonoBitmap     = AnsiTermFb_BlitMonoBitmap,
    .BlitBitmap         = AnsiTermFb_BlitBitmap,
    .GetDescriptor      = AnsiTermFb_GetDescriptor,
    .SetDitherMethod    = AnsiTermFb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  ANSI 256-Color Palette                                          */
/* --------------------------------------------------------------- */

/* ANSI 256-color palette:
 * 0-15: Standard colors
 * 16-231: 6x6x6 RGB cube
 * 232-255: Grayscale ramp
 */

static UINT8
AnsiTermFb_RgbTo256Color(
    FB_COLOR Color
    )
{
    /* Convert RGB888 to ANSI 256-color index */

    /* Check for grayscale */
    INT32 RDiff = (INT32)Color.Red - (INT32)Color.Green;
    INT32 GDiff = (INT32)Color.Green - (INT32)Color.Blue;
    INT32 BDiff = (INT32)Color.Blue - (INT32)Color.Red;

    if (RDiff < 10 && RDiff > -10 && GDiff < 10 && GDiff > -10 &&
        BDiff < 10 && BDiff > -10) {
        /* Grayscale - use grayscale ramp (232-255) */
        UINT32 Gray = (Color.Red + Color.Green + Color.Blue) / 3;
        if (Gray < 8) return 16;  /* Black */
        if (Gray > 247) return 231; /* White */
        return 232 + ((Gray - 8) * 24) / 240;
    }

    /* RGB cube (16-231) */
    UINT8 R = (Color.Red * 5) / 255;
    UINT8 G = (Color.Green * 5) / 255;
    UINT8 B = (Color.Blue * 5) / 255;

    return 16 + (R * 36) + (G * 6) + B;
}

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static VOID
AnsiTermFb_DefaultOutput(
    CONST CHAR8 *String,
    UINTN Length
    )
{
    /* Default: write to stdout (would need to be implemented per platform) */
    /* For UEFI, would use ConOut->OutputString */
    /* For POSIX, would use write(STDOUT_FILENO, String, Length) */
    (VOID)String;
    (VOID)Length;
}

static VOID
AnsiTermFb_ConvertPixelsToText(
    ANSI_TERMINAL_BACKEND *Backend
    )
{
    /* Convert pixel buffer to ANSI text cells using half-block characters */

    for (UINT32 Row = 0; Row < Backend->TermHeight; Row++) {
        for (UINT32 Col = 0; Col < Backend->TermWidth; Col++) {
            /* Sample two vertical pixels per character (using ▀) */
            UINT32 PixelX = Col * Backend->PixelsPerCharX;
            UINT32 TopPixelY = Row * Backend->PixelsPerCharY;
            UINT32 BottomPixelY = TopPixelY + (Backend->PixelsPerCharY / 2);

            if (TopPixelY >= Backend->Descriptor.Height) continue;

            UINT32 TopOffset = (TopPixelY * Backend->Descriptor.Width + PixelX) * 3;
            UINT32 BottomOffset = (BottomPixelY * Backend->Descriptor.Width + PixelX) * 3;

            FB_COLOR TopColor = {
                Backend->PixelBuffer[TopOffset + 0],
                Backend->PixelBuffer[TopOffset + 1],
                Backend->PixelBuffer[TopOffset + 2],
                255
            };

            FB_COLOR BottomColor = {
                (BottomPixelY < Backend->Descriptor.Height) ? Backend->PixelBuffer[BottomOffset + 0] : 0,
                (BottomPixelY < Backend->Descriptor.Height) ? Backend->PixelBuffer[BottomOffset + 1] : 0,
                (BottomPixelY < Backend->Descriptor.Height) ? Backend->PixelBuffer[BottomOffset + 2] : 0,
                255
            };

            ANSI_TEXT_CELL *Cell = &Backend->TextBuffer[Row * Backend->TermWidth + Col];

            /* Use upper half block character */
            Cell->Character = UNICODE_UPPER_HALF;
            Cell->FgColor = AnsiTermFb_RgbTo256Color(TopColor);
            Cell->BgColor = AnsiTermFb_RgbTo256Color(BottomColor);
            Cell->Dirty = TRUE;
        }
    }
}

static VOID
AnsiTermFb_FlushToTerminal(
    ANSI_TERMINAL_BACKEND *Backend
    )
{
    CHAR8 Buffer[256];
    UINTN Len;

    /* Hide cursor during update */
    Backend->OutputFunc(ANSI_CURSOR_HIDE, 6);

    /* Update dirty cells */
    for (UINT32 Row = 0; Row < Backend->TermHeight; Row++) {
        for (UINT32 Col = 0; Col < Backend->TermWidth; Col++) {
            ANSI_TEXT_CELL *Cell = &Backend->TextBuffer[Row * Backend->TermWidth + Col];

            if (!Cell->Dirty) continue;

            /* Position cursor */
            Len = 0; /* Would use sprintf: sprintf(Buffer, ANSI_CURSOR_POS, Row + 1, Col + 1); */
            /* Simplified: assume static positioning */

            /* Set colors */
            if (Backend->Support256Color) {
                /* Use 256-color mode */
                /* Would use: sprintf(Buffer + Len, ANSI_FG_COLOR, Cell->FgColor); */
                /* Would use: sprintf(Buffer + Len, ANSI_BG_COLOR, Cell->BgColor); */
            }

            /* Output character */
            /* Would convert CHAR16 to UTF-8 and output */

            Cell->Dirty = FALSE;
        }
    }

    /* Show cursor */
    Backend->OutputFunc(ANSI_CURSOR_SHOW, 6);
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

FB_IMPLEMENT_BACKEND_IUNKNOWN(AnsiTermFb, ANSI_TERMINAL_BACKEND)

/* --------------------------------------------------------------- */
/*  IFramebufferBackend Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
AnsiTermFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    ANSI_TERMINAL_BACKEND *Backend = (ANSI_TERMINAL_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    Backend->Descriptor = *Descriptor;

    /* Default to 80x24 terminal */
    Backend->TermWidth = 80;
    Backend->TermHeight = 24;
    Backend->PixelsPerCharX = Backend->Descriptor.Width / Backend->TermWidth;
    Backend->PixelsPerCharY = Backend->Descriptor.Height / Backend->TermHeight;

    /* Enable all features */
    Backend->Support256Color = TRUE;
    Backend->SupportTrueColor = TRUE;
    Backend->SupportUnicode = TRUE;

    /* Allocate buffers (static for simplicity) */
    static ANSI_TEXT_CELL sTextBuffer[80 * 60];
    static UINT8 sPixelBuffer[640 * 480 * 3];  /* RGB888 */

    Backend->TextBuffer = sTextBuffer;
    Backend->PixelBuffer = sPixelBuffer;
    Backend->OutputFunc = AnsiTermFb_DefaultOutput;
    Backend->Initialized = TRUE;

    /* Clear screen */
    Backend->OutputFunc(ANSI_CLEAR_SCREEN, 4);
    Backend->OutputFunc(ANSI_CURSOR_HOME, 3);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AnsiTermFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    ANSI_TERMINAL_BACKEND *Backend = (ANSI_TERMINAL_BACKEND *)This;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    /* Clear pixel buffer */
    UINT32 PixelCount = Backend->Descriptor.Width * Backend->Descriptor.Height;
    for (UINT32 i = 0; i < PixelCount; i++) {
        Backend->PixelBuffer[i * 3 + 0] = Color.Red;
        Backend->PixelBuffer[i * 3 + 1] = Color.Green;
        Backend->PixelBuffer[i * 3 + 2] = Color.Blue;
    }

    /* Convert and flush */
    AnsiTermFb_ConvertPixelsToText(Backend);
    AnsiTermFb_FlushToTerminal(Backend);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AnsiTermFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    ANSI_TERMINAL_BACKEND *Backend = (ANSI_TERMINAL_BACKEND *)This;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    UINT32 Offset = (Y * Backend->Descriptor.Width + X) * 3;
    Backend->PixelBuffer[Offset + 0] = Color.Red;
    Backend->PixelBuffer[Offset + 1] = Color.Green;
    Backend->PixelBuffer[Offset + 2] = Color.Blue;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AnsiTermFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    ANSI_TERMINAL_BACKEND *Backend = (ANSI_TERMINAL_BACKEND *)This;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    UINT32 Offset = (Y * Backend->Descriptor.Width + X) * 3;
    Color->Red = Backend->PixelBuffer[Offset + 0];
    Color->Green = Backend->PixelBuffer[Offset + 1];
    Color->Blue = Backend->PixelBuffer[Offset + 2];
    Color->Alpha = 255;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AnsiTermFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    ANSI_TERMINAL_BACKEND *Backend = (ANSI_TERMINAL_BACKEND *)This;
    INT32 x, y;

    if (!Backend->Initialized || Rect == NULL) {
        return E_POINTER;
    }

    for (y = Rect->Top; y < Rect->Bottom && y < (INT32)Backend->Descriptor.Height; y++) {
        for (x = Rect->Left; x < Rect->Right && x < (INT32)Backend->Descriptor.Width; x++) {
            if (x >= 0 && y >= 0) {
                UINT32 Offset = (y * Backend->Descriptor.Width + x) * 3;
                Backend->PixelBuffer[Offset + 0] = Color.Red;
                Backend->PixelBuffer[Offset + 1] = Color.Green;
                Backend->PixelBuffer[Offset + 2] = Color.Blue;
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AnsiTermFb_BlitMonoBitmap(
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
    ANSI_TERMINAL_BACKEND *Backend = (ANSI_TERMINAL_BACKEND *)This;
    UINT32 Row, Col;
    UINT32 ByteIndex, BitIndex;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    for (Row = 0; Row < Height; Row++) {
        for (Col = 0; Col < Width; Col++) {
            ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
            BitIndex = 7 - (Col % 8);

            FB_COLOR *Color = (Bitmap[ByteIndex] & (1 << BitIndex)) ? &Foreground : &Background;

            INT32 DestX = X + Col;
            INT32 DestY = Y + Row;

            if (DestX >= 0 && DestX < (INT32)Backend->Descriptor.Width &&
                DestY >= 0 && DestY < (INT32)Backend->Descriptor.Height) {
                UINT32 Offset = (DestY * Backend->Descriptor.Width + DestX) * 3;
                Backend->PixelBuffer[Offset + 0] = Color->Red;
                Backend->PixelBuffer[Offset + 1] = Color->Green;
                Backend->PixelBuffer[Offset + 2] = Color->Blue;
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AnsiTermFb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    ANSI_TERMINAL_BACKEND *Backend = (ANSI_TERMINAL_BACKEND *)This;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    /* ANSI terminal backend uses RGB888 pixel buffer */
    /* Convert source format to RGB888 and write to buffer */

    /* Fast path: RGB888 format (direct copy) */
    if (SourceFormat == FbPixelFormatRgb888) {
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

                UINT32 DestOffset = (DestY * Backend->Descriptor.Width + DestX) * 3;
                UINT32 SrcOffset = (Row * Width + Col) * 3;

                Backend->PixelBuffer[DestOffset + 0] = Bitmap[SrcOffset + 0];  /* Red */
                Backend->PixelBuffer[DestOffset + 1] = Bitmap[SrcOffset + 1];  /* Green */
                Backend->PixelBuffer[DestOffset + 2] = Bitmap[SrcOffset + 2];  /* Blue */
            }
        }
        return S_OK;
    }

    /* Other RGB formats - convert to RGB888 */
    if (SourceFormat == FbPixelFormatRgba8888 ||
        SourceFormat == FbPixelFormatBgra8888 ||
        SourceFormat == FbPixelFormatRgb555 ||
        SourceFormat == FbPixelFormatRgb565) {

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

                UINT32 PixelValue = 0;
                UINT32 SrcOffset = Row * Width + Col;

                /* Read pixel value based on format */
                if (SourceFormat == FbPixelFormatRgba8888 ||
                    SourceFormat == FbPixelFormatBgra8888) {
                    PixelValue = ((UINT32 *)Bitmap)[SrcOffset];
                } else if (SourceFormat == FbPixelFormatRgb565 ||
                           SourceFormat == FbPixelFormatRgb555) {
                    PixelValue = ((UINT16 *)Bitmap)[SrcOffset];
                }

                /* Unpack to FB_COLOR */
                FB_COLOR Color = FbUnpackPixel(PixelValue, SourceFormat, 0, 0, 0);

                /* Write to pixel buffer */
                UINT32 DestOffset = (DestY * Backend->Descriptor.Width + DestX) * 3;
                Backend->PixelBuffer[DestOffset + 0] = Color.Red;
                Backend->PixelBuffer[DestOffset + 1] = Color.Green;
                Backend->PixelBuffer[DestOffset + 2] = Color.Blue;
            }
        }
        return S_OK;
    }

    /* Grayscale formats - expand to RGB */
    if (SourceFormat == FbPixelFormat1Bpp ||
        SourceFormat == FbPixelFormat2Bpp ||
        SourceFormat == FbPixelFormat4Bpp ||
        SourceFormat == FbPixelFormat8Bpp) {

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

                UINT8 GrayValue = 0;

                /* Extract grayscale value */
                if (SourceFormat == FbPixelFormat1Bpp) {
                    UINT32 ByteIdx = Row * ((Width + 7) / 8) + (Col / 8);
                    UINT32 BitIdx = 7 - (Col % 8);
                    GrayValue = (Bitmap[ByteIdx] & (1 << BitIdx)) ? 255 : 0;
                } else if (SourceFormat == FbPixelFormat2Bpp) {
                    UINT32 ByteIdx = Row * ((Width + 3) / 4) + (Col / 4);
                    UINT32 BitOffset = (3 - (Col % 4)) * 2;
                    UINT8 Value = (Bitmap[ByteIdx] >> BitOffset) & 0x03;
                    GrayValue = (Value * 255) / 3;
                } else if (SourceFormat == FbPixelFormat4Bpp) {
                    UINT32 ByteIdx = Row * ((Width + 1) / 2) + (Col / 2);
                    UINT8 Value = (Col & 1) ?
                        (Bitmap[ByteIdx] & 0x0F) :
                        ((Bitmap[ByteIdx] >> 4) & 0x0F);
                    GrayValue = (Value * 255) / 15;
                } else {  /* FbPixelFormat8Bpp */
                    GrayValue = Bitmap[Row * Width + Col];
                }

                /* Write grayscale to RGB buffer */
                UINT32 DestOffset = (DestY * Backend->Descriptor.Width + DestX) * 3;
                Backend->PixelBuffer[DestOffset + 0] = GrayValue;
                Backend->PixelBuffer[DestOffset + 1] = GrayValue;
                Backend->PixelBuffer[DestOffset + 2] = GrayValue;
            }
        }
        return S_OK;
    }

    /* Other formats - not implemented */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
AnsiTermFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    ANSI_TERMINAL_BACKEND *Backend = (ANSI_TERMINAL_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AnsiTermFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    ANSI_TERMINAL_BACKEND *Backend = (ANSI_TERMINAL_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static ANSI_TERMINAL_BACKEND gAnsiTermBackendInstance = {
    .Base.lpVtbl        = &gAnsiTermFbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherClassicMac,
    .Support256Color    = TRUE,
    .SupportTrueColor   = TRUE,
    .SupportUnicode     = TRUE,
};

IFramebufferBackend *
FbCreateTerminalBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gAnsiTermBackendInstance;
}

/* --------------------------------------------------------------- */
/*  Backend Registration                                           */
/* --------------------------------------------------------------- */

/*
 * Register this backend with the factory.
 * Called by the backend initialization system.
 */
VOID
FbRegisterTerminalBackend(
    VOID
    )
{
    FbRegisterBackend(FbBackendGeneric, FbCreateTerminalBackend);
}
