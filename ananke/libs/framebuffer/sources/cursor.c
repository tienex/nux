/*++
    Module Name:

        cursor.c

    Abstract:

        IFramebufferCursor implementation.

        Provides software cursor rendering with save/restore mechanism.
        Supports monochrome (AND/XOR masks) and color (RGBA) cursors.
        Hardware cursor support can be added by backends in the future.

--*/

#include <ananke/framebuffer/cursor.h>
#include <ananke/framebuffer/screen.h>
#include <ananke/framebuffer/backends.h>
#include <ananke/framebuffer/com_helpers.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Cursor Implementation Structure                                */
/* --------------------------------------------------------------- */

typedef struct _FB_CURSOR_IMPL {
    IFramebufferCursor      Base;
    REFOBJ                  RefCount;
    IFramebufferBackend     *Backend;
    FB_CURSOR_DESC          Descriptor;
    BOOLEAN                 Visible;
    INT32                   X;
    INT32                   Y;

    /* Cursor bitmap data */
    UINT8                   AndMask[FB_CURSOR_MAX_WIDTH * FB_CURSOR_MAX_HEIGHT / 8];
    UINT8                   XorMask[FB_CURSOR_MAX_WIDTH * FB_CURSOR_MAX_HEIGHT / 8];
    UINT8                   ColorData[FB_CURSOR_MAX_WIDTH * FB_CURSOR_MAX_HEIGHT * 4];

    /* Background save/restore for software cursor */
    BOOLEAN                 BackgroundSaved;
    INT32                   SavedX;
    INT32                   SavedY;
    UINT32                  SavedWidth;
    UINT32                  SavedHeight;
    FB_COLOR                SavedBackground[FB_CURSOR_MAX_WIDTH * FB_CURSOR_MAX_HEIGHT];
} FB_CURSOR_IMPL;

/* --------------------------------------------------------------- */
/*  Forward Declarations                                            */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE FbCursor_QueryInterface(
    IFramebufferCursor *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE FbCursor_AddRef(IFramebufferCursor *This);
static UINT32 STDMETHODCALLTYPE FbCursor_Release(IFramebufferCursor *This);
static HRESULT STDMETHODCALLTYPE FbCursor_SetVisible(
    IFramebufferCursor *This, BOOLEAN Visible);
static HRESULT STDMETHODCALLTYPE FbCursor_IsVisible(
    IFramebufferCursor *This, BOOLEAN *Visible);
static HRESULT STDMETHODCALLTYPE FbCursor_SetPosition(
    IFramebufferCursor *This, INT32 X, INT32 Y);
static HRESULT STDMETHODCALLTYPE FbCursor_GetPosition(
    IFramebufferCursor *This, INT32 *X, INT32 *Y);
static HRESULT STDMETHODCALLTYPE FbCursor_SetMonoCursor(
    IFramebufferCursor *This, CONST UINT8 *AndMask, CONST UINT8 *XorMask,
    UINT32 Width, UINT32 Height, INT32 HotSpotX, INT32 HotSpotY);
static HRESULT STDMETHODCALLTYPE FbCursor_SetColorCursor(
    IFramebufferCursor *This, CONST UINT8 *Data,
    UINT32 Width, UINT32 Height, INT32 HotSpotX, INT32 HotSpotY);
static HRESULT STDMETHODCALLTYPE FbCursor_GetDescriptor(
    IFramebufferCursor *This, FB_CURSOR_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE FbCursor_IsHardwareCursor(
    IFramebufferCursor *This, BOOLEAN *IsHardware);

/* Internal helpers */
static VOID FbCursor_SaveBackground(FB_CURSOR_IMPL *Cursor);
static VOID FbCursor_RestoreBackground(FB_CURSOR_IMPL *Cursor);
static VOID FbCursor_DrawMono(FB_CURSOR_IMPL *Cursor);
static VOID FbCursor_DrawColor(FB_CURSOR_IMPL *Cursor);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferCursorVtbl gCursorVtbl = {
    .QueryInterface     = FbCursor_QueryInterface,
    .AddRef             = FbCursor_AddRef,
    .Release            = FbCursor_Release,
    .SetVisible         = FbCursor_SetVisible,
    .IsVisible          = FbCursor_IsVisible,
    .SetPosition        = FbCursor_SetPosition,
    .GetPosition        = FbCursor_GetPosition,
    .SetMonoCursor      = FbCursor_SetMonoCursor,
    .SetColorCursor     = FbCursor_SetColorCursor,
    .GetDescriptor      = FbCursor_GetDescriptor,
    .IsHardwareCursor   = FbCursor_IsHardwareCursor,
};

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

FB_IMPLEMENT_IUNKNOWN(FbCursor, FB_CURSOR_IMPL, IFramebufferCursor, IID_IFramebufferCursor)

/* --------------------------------------------------------------- */
/*  IFramebufferCursor Implementation                               */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbCursor_SetVisible(
    IFramebufferCursor *This,
    BOOLEAN Visible
    )
{
    FB_CURSOR_IMPL *Cursor = (FB_CURSOR_IMPL *)This;

    if (Visible == Cursor->Visible) {
        return S_OK;  /* No change */
    }

    if (Visible) {
        /* Show cursor */
        FbCursor_SaveBackground(Cursor);

        if (Cursor->Descriptor.Type == FbCursorMono) {
            FbCursor_DrawMono(Cursor);
        } else {
            FbCursor_DrawColor(Cursor);
        }
    } else {
        /* Hide cursor */
        FbCursor_RestoreBackground(Cursor);
    }

    Cursor->Visible = Visible;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbCursor_IsVisible(
    IFramebufferCursor *This,
    BOOLEAN *Visible
    )
{
    FB_CURSOR_IMPL *Cursor = (FB_CURSOR_IMPL *)This;

    if (Visible == NULL) {
        return E_POINTER;
    }

    *Visible = Cursor->Visible;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbCursor_SetPosition(
    IFramebufferCursor *This,
    INT32 X,
    INT32 Y
    )
{
    FB_CURSOR_IMPL *Cursor = (FB_CURSOR_IMPL *)This;

    /* If visible, hide at old position and show at new position */
    if (Cursor->Visible) {
        FbCursor_RestoreBackground(Cursor);
    }

    Cursor->X = X;
    Cursor->Y = Y;

    if (Cursor->Visible) {
        FbCursor_SaveBackground(Cursor);

        if (Cursor->Descriptor.Type == FbCursorMono) {
            FbCursor_DrawMono(Cursor);
        } else {
            FbCursor_DrawColor(Cursor);
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbCursor_GetPosition(
    IFramebufferCursor *This,
    INT32 *X,
    INT32 *Y
    )
{
    FB_CURSOR_IMPL *Cursor = (FB_CURSOR_IMPL *)This;

    if (X == NULL || Y == NULL) {
        return E_POINTER;
    }

    *X = Cursor->X;
    *Y = Cursor->Y;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbCursor_SetMonoCursor(
    IFramebufferCursor *This,
    CONST UINT8 *AndMask,
    CONST UINT8 *XorMask,
    UINT32 Width,
    UINT32 Height,
    INT32 HotSpotX,
    INT32 HotSpotY
    )
{
    FB_CURSOR_IMPL *Cursor = (FB_CURSOR_IMPL *)This;

    if (AndMask == NULL || XorMask == NULL) {
        return E_POINTER;
    }

    if (Width == 0 || Height == 0 || Width > FB_CURSOR_MAX_WIDTH || Height > FB_CURSOR_MAX_HEIGHT) {
        return E_INVALIDARG;
    }

    /* Hide cursor if visible */
    BOOLEAN WasVisible = Cursor->Visible;
    if (WasVisible) {
        FbCursor_SetVisible(This, FALSE);
    }

    /* Update descriptor */
    Cursor->Descriptor.Type = FbCursorMono;
    Cursor->Descriptor.Width = Width;
    Cursor->Descriptor.Height = Height;
    Cursor->Descriptor.HotSpotX = HotSpotX;
    Cursor->Descriptor.HotSpotY = HotSpotY;

    /* Copy masks */
    UINT32 MaskSize = (Width * Height + 7) / 8;
    ANX_MEMCPY(Cursor->AndMask, AndMask, MaskSize);
    ANX_MEMCPY(Cursor->XorMask, XorMask, MaskSize);

    /* Restore visibility */
    if (WasVisible) {
        FbCursor_SetVisible(This, TRUE);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbCursor_SetColorCursor(
    IFramebufferCursor *This,
    CONST UINT8 *Data,
    UINT32 Width,
    UINT32 Height,
    INT32 HotSpotX,
    INT32 HotSpotY
    )
{
    FB_CURSOR_IMPL *Cursor = (FB_CURSOR_IMPL *)This;

    if (Data == NULL) {
        return E_POINTER;
    }

    if (Width == 0 || Height == 0 || Width > FB_CURSOR_MAX_WIDTH || Height > FB_CURSOR_MAX_HEIGHT) {
        return E_INVALIDARG;
    }

    /* Hide cursor if visible */
    BOOLEAN WasVisible = Cursor->Visible;
    if (WasVisible) {
        FbCursor_SetVisible(This, FALSE);
    }

    /* Update descriptor */
    Cursor->Descriptor.Type = FbCursorColor;
    Cursor->Descriptor.Width = Width;
    Cursor->Descriptor.Height = Height;
    Cursor->Descriptor.HotSpotX = HotSpotX;
    Cursor->Descriptor.HotSpotY = HotSpotY;

    /* Copy color data (RGBA format) */
    ANX_MEMCPY(Cursor->ColorData, Data, Width * Height * 4);

    /* Restore visibility */
    if (WasVisible) {
        FbCursor_SetVisible(This, TRUE);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbCursor_GetDescriptor(
    IFramebufferCursor *This,
    FB_CURSOR_DESC *Descriptor
    )
{
    FB_CURSOR_IMPL *Cursor = (FB_CURSOR_IMPL *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    ANX_MEMCPY(Descriptor, &Cursor->Descriptor, sizeof(FB_CURSOR_DESC));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbCursor_IsHardwareCursor(
    IFramebufferCursor *This,
    BOOLEAN *IsHardware
    )
{
    if (IsHardware == NULL) {
        return E_POINTER;
    }

    /* Software cursor only for now */
    *IsHardware = FALSE;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Internal Helper Functions                                       */
/* --------------------------------------------------------------- */

static VOID
FbCursor_SaveBackground(
    FB_CURSOR_IMPL *Cursor
    )
{
    INT32 ScreenX = Cursor->X - Cursor->Descriptor.HotSpotX;
    INT32 ScreenY = Cursor->Y - Cursor->Descriptor.HotSpotY;
    UINT32 Width = Cursor->Descriptor.Width;
    UINT32 Height = Cursor->Descriptor.Height;

    /* Save background for restore */
    for (UINT32 Y = 0; Y < Height; Y++) {
        for (UINT32 X = 0; X < Width; X++) {
            FB_COLOR Color = {0, 0, 0, 0};
            IFramebufferBackend_GetPixel(Cursor->Backend, ScreenX + X, ScreenY + Y, &Color);
            Cursor->SavedBackground[Y * Width + X] = Color;
        }
    }

    Cursor->BackgroundSaved = TRUE;
    Cursor->SavedX = ScreenX;
    Cursor->SavedY = ScreenY;
    Cursor->SavedWidth = Width;
    Cursor->SavedHeight = Height;
}

static VOID
FbCursor_RestoreBackground(
    FB_CURSOR_IMPL *Cursor
    )
{
    if (!Cursor->BackgroundSaved) {
        return;
    }

    /* Restore saved background */
    for (UINT32 Y = 0; Y < Cursor->SavedHeight; Y++) {
        for (UINT32 X = 0; X < Cursor->SavedWidth; X++) {
            FB_COLOR Color = Cursor->SavedBackground[Y * Cursor->SavedWidth + X];
            IFramebufferBackend_SetPixel(Cursor->Backend, Cursor->SavedX + X, Cursor->SavedY + Y, Color);
        }
    }

    Cursor->BackgroundSaved = FALSE;
}

static VOID
FbCursor_DrawMono(
    FB_CURSOR_IMPL *Cursor
    )
{
    INT32 ScreenX = Cursor->X - Cursor->Descriptor.HotSpotX;
    INT32 ScreenY = Cursor->Y - Cursor->Descriptor.HotSpotY;
    UINT32 Width = Cursor->Descriptor.Width;
    UINT32 Height = Cursor->Descriptor.Height;

    /* Draw monochrome cursor with AND/XOR masks */
    for (UINT32 Y = 0; Y < Height; Y++) {
        for (UINT32 X = 0; X < Width; X++) {
            UINT32 BitIndex = Y * Width + X;
            UINT32 ByteIndex = BitIndex / 8;
            UINT32 BitOffset = 7 - (BitIndex % 8);

            BOOLEAN AndBit = (Cursor->AndMask[ByteIndex] & (1 << BitOffset)) != 0;
            BOOLEAN XorBit = (Cursor->XorMask[ByteIndex] & (1 << BitOffset)) != 0;

            if (!AndBit && !XorBit) {
                /* Transparent pixel - skip */
                continue;
            }

            FB_COLOR Color;
            IFramebufferBackend_GetPixel(Cursor->Backend, ScreenX + X, ScreenY + Y, &Color);

            if (AndBit && XorBit) {
                /* White pixel */
                Color.Red = Color.Green = Color.Blue = 255;
            } else if (AndBit && !XorBit) {
                /* Black pixel */
                Color.Red = Color.Green = Color.Blue = 0;
            } else if (!AndBit && XorBit) {
                /* Invert pixel */
                Color.Red = 255 - Color.Red;
                Color.Green = 255 - Color.Green;
                Color.Blue = 255 - Color.Blue;
            }

            IFramebufferBackend_SetPixel(Cursor->Backend, ScreenX + X, ScreenY + Y, Color);
        }
    }
}

static VOID
FbCursor_DrawColor(
    FB_CURSOR_IMPL *Cursor
    )
{
    INT32 ScreenX = Cursor->X - Cursor->Descriptor.HotSpotX;
    INT32 ScreenY = Cursor->Y - Cursor->Descriptor.HotSpotY;
    UINT32 Width = Cursor->Descriptor.Width;
    UINT32 Height = Cursor->Descriptor.Height;

    /* Draw color cursor with alpha blending */
    for (UINT32 Y = 0; Y < Height; Y++) {
        for (UINT32 X = 0; X < Width; X++) {
            UINT32 Index = (Y * Width + X) * 4;
            UINT8 R = Cursor->ColorData[Index + 0];
            UINT8 G = Cursor->ColorData[Index + 1];
            UINT8 B = Cursor->ColorData[Index + 2];
            UINT8 A = Cursor->ColorData[Index + 3];

            if (A == 0) {
                /* Fully transparent - skip */
                continue;
            }

            if (A == 255) {
                /* Fully opaque - direct write */
                FB_COLOR Color = {R, G, B, 255};
                IFramebufferBackend_SetPixel(Cursor->Backend, ScreenX + X, ScreenY + Y, Color);
            } else {
                /* Alpha blend */
                FB_COLOR BgColor;
                IFramebufferBackend_GetPixel(Cursor->Backend, ScreenX + X, ScreenY + Y, &BgColor);

                FB_COLOR BlendColor;
                BlendColor.Red   = (R * A + BgColor.Red * (255 - A)) / 255;
                BlendColor.Green = (G * A + BgColor.Green * (255 - A)) / 255;
                BlendColor.Blue  = (B * A + BgColor.Blue * (255 - A)) / 255;
                BlendColor.Alpha = 255;

                IFramebufferBackend_SetPixel(Cursor->Backend, ScreenX + X, ScreenY + Y, BlendColor);
            }
        }
    }
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

/*
 * Create a cursor for a backend.
 */
IFramebufferCursor *
FbCreateCursor(
    IN IFramebufferBackend *Backend
    )
{
    FB_CURSOR_IMPL *Cursor;

    if (Backend == NULL) {
        return NULL;
    }

    /* Allocate cursor object */
    Cursor = (FB_CURSOR_IMPL *)ANX_MALLOC(sizeof(FB_CURSOR_IMPL));
    if (Cursor == NULL) {
        return NULL;
    }

    /* Initialize */
    ANX_MEMSET(Cursor, 0, sizeof(FB_CURSOR_IMPL));
    Cursor->Base.lpVtbl = &gCursorVtbl;
    Cursor->RefCount.RefCount = 1;
    Cursor->Backend = Backend;
    Cursor->Visible = FALSE;
    Cursor->X = 0;
    Cursor->Y = 0;
    Cursor->BackgroundSaved = FALSE;

    /* Set default arrow cursor */
    Cursor->Descriptor.Type = FbCursorMono;
    Cursor->Descriptor.Width = 16;
    Cursor->Descriptor.Height = 16;
    Cursor->Descriptor.HotSpotX = 0;
    Cursor->Descriptor.HotSpotY = 0;

    /* Add reference to backend */
    IUnknown_AddRef((IUnknown *)Backend);

    return &Cursor->Base;
}

/* --------------------------------------------------------------- */
/*  Standard Cursor Bitmaps                                         */
/* --------------------------------------------------------------- */

/* Standard arrow cursor (16x16 monochrome) */
CONST UINT8 gStandardArrowCursorAnd[16 * 16 / 8] = {
    0x00, 0x00,  /* ................ */
    0x40, 0x00,  /* .#.............. */
    0x60, 0x00,  /* .##............. */
    0x70, 0x00,  /* .###............ */
    0x78, 0x00,  /* .####........... */
    0x7C, 0x00,  /* .#####.......... */
    0x7E, 0x00,  /* .######......... */
    0x7F, 0x00,  /* .#######........ */
    0x7F, 0x80,  /* .########....... */
    0x7C, 0x00,  /* .#####.......... */
    0x6C, 0x00,  /* .##.##.......... */
    0x46, 0x00,  /* .#...##......... */
    0x06, 0x00,  /* .....##......... */
    0x03, 0x00,  /* ......##........ */
    0x03, 0x00,  /* ......##........ */
    0x00, 0x00,  /* ................ */
};

CONST UINT8 gStandardArrowCursorXor[16 * 16 / 8] = {
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
};

/* Standard I-beam cursor (16x16 monochrome) */
CONST UINT8 gStandardIBeamCursorAnd[16 * 16 / 8] = {
    0x1F, 0x80,  /* ...######....... */
    0x0E, 0x00,  /* ....###......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x0E, 0x00,  /* ....###......... */
    0x1F, 0x80,  /* ...######....... */
    0x00, 0x00,  /* ................ */
};

CONST UINT8 gStandardIBeamCursorXor[16 * 16 / 8] = {0};

/* Standard wait/hourglass cursor (16x16 monochrome) */
CONST UINT8 gStandardWaitCursorAnd[16 * 16 / 8] = {
    0xFF, 0xFE,  /* ###############. */
    0x80, 0x02,  /* #.............#. */
    0x40, 0x04,  /* .#...........#.. */
    0x20, 0x08,  /* ..#.........#... */
    0x10, 0x10,  /* ...#.......#.... */
    0x08, 0x20,  /* ....#.....#..... */
    0x05, 0x40,  /* .....#.#.#...... */
    0x02, 0x80,  /* ......#.#....... */
    0x02, 0x80,  /* ......#.#....... */
    0x05, 0x40,  /* .....#.#.#...... */
    0x08, 0x20,  /* ....#.....#..... */
    0x10, 0x10,  /* ...#.......#.... */
    0x20, 0x08,  /* ..#.........#... */
    0x40, 0x04,  /* .#...........#.. */
    0x80, 0x02,  /* #.............#. */
    0xFF, 0xFE,  /* ###############. */
};

CONST UINT8 gStandardWaitCursorXor[16 * 16 / 8] = {0};

/* Standard crosshair cursor (16x16 monochrome) */
CONST UINT8 gStandardCrosshairCursorAnd[16 * 16 / 8] = {
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0xFF, 0xFE,  /* ###############. */
    0x04, 0x00,  /* .....#.......... */
    0xFF, 0xFE,  /* ###############. */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x04, 0x00,  /* .....#.......... */
    0x00, 0x00,  /* ................ */
};

CONST UINT8 gStandardCrosshairCursorXor[16 * 16 / 8] = {0};
