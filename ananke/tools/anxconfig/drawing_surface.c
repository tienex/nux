/*
 * drawing_surface.c - Drawing Surface Abstraction
 *
 * Provides an intermediate drawing layer between widgets and screen with:
 * - Clipping support
 * - Fill and stroke operations
 * - Box drawing with various styles
 * - Line drawing
 * - Character and attribute manipulation
 * - Double-buffering capability
 * - Off-screen rendering
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Surface cell - character + attributes */
typedef struct {
    CHAR16 Character;
    TUI_COLOR Foreground;
    TUI_COLOR Background;
    UINT8 Attributes;
    BOOLEAN Dirty;  /* Cell modified since last flush */
} SurfaceCell;

typedef struct {
    ITuiSurface Interface;
    UINTN RefCount;

    /* Dimensions */
    UINT32 Width;
    UINT32 Height;

    /* Buffer */
    SurfaceCell **Cells;  /* [Height][Width] */

    /* Clipping region */
    TUI_RECT ClipRect;
    BOOLEAN ClippingEnabled;

    /* Target screen (for flushing) */
    ITuiScreen *Screen;

    /* Drawing state */
    TUI_COLOR DefaultForeground;
    TUI_COLOR DefaultBackground;

} TuiSurfaceImpl;

/* Helper: Check if point is clipped */
static inline BOOLEAN IsClipped(TuiSurfaceImpl *impl, INT32 X, INT32 Y)
{
    if (!impl->ClippingEnabled) {
        return (X < 0 || Y < 0 || X >= (INT32)impl->Width || Y >= (INT32)impl->Height);
    }

    return (X < impl->ClipRect.X ||
            Y < impl->ClipRect.Y ||
            X >= impl->ClipRect.X + impl->ClipRect.Width ||
            Y >= impl->ClipRect.Y + impl->ClipRect.Height ||
            X < 0 || Y < 0 ||
            X >= (INT32)impl->Width ||
            Y >= (INT32)impl->Height);
}

/* IUnknown methods */
static HRESULT ANXAPI Surface_QueryInterface(
    ITuiSurface *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiSurface)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI Surface_AddRef(ITuiSurface *This)
{
    TuiSurfaceImpl *impl = (TuiSurfaceImpl *)This;
    return ++impl->RefCount;
}

static UINTN ANXAPI Surface_Release(ITuiSurface *This)
{
    TuiSurfaceImpl *impl = (TuiSurfaceImpl *)This;
    UINTN count = --impl->RefCount;

    if (count == 0) {
        /* Free buffer */
        if (impl->Cells) {
            for (UINT32 i = 0; i < impl->Height; i++) {
                free(impl->Cells[i]);
            }
            free(impl->Cells);
        }

        /* Release screen */
        if (impl->Screen) {
            impl->Screen->Vtbl->Release(impl->Screen);
        }

        free(impl);
    }

    return count;
}

/* Set clipping rectangle */
static HRESULT ANXAPI Surface_SetClipRect(
    ITuiSurface *This,
    CONST TUI_RECT *Rect
)
{
    TuiSurfaceImpl *impl = (TuiSurfaceImpl *)This;

    if (!Rect) {
        impl->ClippingEnabled = FALSE;
        return S_OK;
    }

    impl->ClipRect = *Rect;
    impl->ClippingEnabled = TRUE;

    return S_OK;
}

/* Get clipping rectangle */
static HRESULT ANXAPI Surface_GetClipRect(
    ITuiSurface *This,
    TUI_RECT *Rect
)
{
    TuiSurfaceImpl *impl = (TuiSurfaceImpl *)This;

    if (!Rect) return E_INVALIDARG;

    if (impl->ClippingEnabled) {
        *Rect = impl->ClipRect;
    } else {
        Rect->X = 0;
        Rect->Y = 0;
        Rect->Width = impl->Width;
        Rect->Height = impl->Height;
    }

    return S_OK;
}

/* Set character at position */
static HRESULT ANXAPI Surface_SetChar(
    ITuiSurface *This,
    INT32 X,
    INT32 Y,
    CHAR16 Character,
    TUI_COLOR Foreground,
    TUI_COLOR Background
)
{
    TuiSurfaceImpl *impl = (TuiSurfaceImpl *)This;

    if (IsClipped(impl, X, Y)) return S_OK;

    SurfaceCell *cell = &impl->Cells[Y][X];
    cell->Character = Character;
    cell->Foreground = Foreground;
    cell->Background = Background;
    cell->Dirty = TRUE;

    return S_OK;
}

/* Get character at position */
static HRESULT ANXAPI Surface_GetChar(
    ITuiSurface *This,
    INT32 X,
    INT32 Y,
    CHAR16 *Character,
    TUI_COLOR *Foreground,
    TUI_COLOR *Background
)
{
    TuiSurfaceImpl *impl = (TuiSurfaceImpl *)This;

    if (X < 0 || Y < 0 || X >= (INT32)impl->Width || Y >= (INT32)impl->Height) {
        return E_INVALIDARG;
    }

    SurfaceCell *cell = &impl->Cells[Y][X];
    if (Character) *Character = cell->Character;
    if (Foreground) *Foreground = cell->Foreground;
    if (Background) *Background = cell->Background;

    return S_OK;
}

/* Set attributes at position */
static HRESULT ANXAPI Surface_SetAttributes(
    ITuiSurface *This,
    INT32 X,
    INT32 Y,
    UINT8 Attributes
)
{
    TuiSurfaceImpl *impl = (TuiSurfaceImpl *)This;

    if (IsClipped(impl, X, Y)) return S_OK;

    impl->Cells[Y][X].Attributes = Attributes;
    impl->Cells[Y][X].Dirty = TRUE;

    return S_OK;
}

/* Fill rectangle */
static HRESULT ANXAPI Surface_FillRect(
    ITuiSurface *This,
    CONST TUI_RECT *Rect,
    CHAR16 Character,
    TUI_COLOR Foreground,
    TUI_COLOR Background
)
{
    TuiSurfaceImpl *impl = (TuiSurfaceImpl *)This;

    if (!Rect) return E_INVALIDARG;

    for (INT32 y = Rect->Y; y < Rect->Y + Rect->Height; y++) {
        for (INT32 x = Rect->X; x < Rect->X + Rect->Width; x++) {
            Surface_SetChar(This, x, y, Character, Foreground, Background);
        }
    }

    return S_OK;
}

/* Stroke rectangle (outline only) */
static HRESULT ANXAPI Surface_StrokeRect(
    ITuiSurface *This,
    CONST TUI_RECT *Rect,
    TUI_BORDER_STYLE Style,
    TUI_COLOR Foreground,
    TUI_COLOR Background
)
{
    TuiSurfaceImpl *impl = (TuiSurfaceImpl *)This;

    if (!Rect) return E_INVALIDARG;
    if (Rect->Width < 2 || Rect->Height < 2) return E_INVALIDARG;

    /* Get appropriate box characters for style */
    CHAR16 horizontal, vertical, topLeft, topRight, bottomLeft, bottomRight;

    switch (Style) {
        case TuiBorderSingle:
            horizontal = gBoxChars.SingleHorizontal;
            vertical = gBoxChars.SingleVertical;
            topLeft = gBoxChars.SingleTopLeft;
            topRight = gBoxChars.SingleTopRight;
            bottomLeft = gBoxChars.SingleBottomLeft;
            bottomRight = gBoxChars.SingleBottomRight;
            break;

        case TuiBorderDouble:
            horizontal = gBoxChars.DoubleHorizontal;
            vertical = gBoxChars.DoubleVertical;
            topLeft = gBoxChars.DoubleTopLeft;
            topRight = gBoxChars.DoubleTopRight;
            bottomLeft = gBoxChars.DoubleBottomLeft;
            bottomRight = gBoxChars.DoubleBottomRight;
            break;

        case TuiBorderAscii:
            horizontal = '-';
            vertical = '|';
            topLeft = '+';
            topRight = '+';
            bottomLeft = '+';
            bottomRight = '+';
            break;

        default:
            horizontal = gBoxChars.SingleHorizontal;
            vertical = gBoxChars.SingleVertical;
            topLeft = gBoxChars.SingleTopLeft;
            topRight = gBoxChars.SingleTopRight;
            bottomLeft = gBoxChars.SingleBottomLeft;
            bottomRight = gBoxChars.SingleBottomRight;
            break;
    }

    /* Draw corners */
    Surface_SetChar(This, Rect->X, Rect->Y, topLeft, Foreground, Background);
    Surface_SetChar(This, Rect->X + Rect->Width - 1, Rect->Y, topRight, Foreground, Background);
    Surface_SetChar(This, Rect->X, Rect->Y + Rect->Height - 1, bottomLeft, Foreground, Background);
    Surface_SetChar(This, Rect->X + Rect->Width - 1, Rect->Y + Rect->Height - 1, bottomRight, Foreground, Background);

    /* Draw horizontal lines */
    for (INT32 x = Rect->X + 1; x < Rect->X + Rect->Width - 1; x++) {
        Surface_SetChar(This, x, Rect->Y, horizontal, Foreground, Background);
        Surface_SetChar(This, x, Rect->Y + Rect->Height - 1, horizontal, Foreground, Background);
    }

    /* Draw vertical lines */
    for (INT32 y = Rect->Y + 1; y < Rect->Y + Rect->Height - 1; y++) {
        Surface_SetChar(This, Rect->X, y, vertical, Foreground, Background);
        Surface_SetChar(This, Rect->X + Rect->Width - 1, y, vertical, Foreground, Background);
    }

    return S_OK;
}

/* Draw line */
static HRESULT ANXAPI Surface_DrawLine(
    ITuiSurface *This,
    INT32 X1,
    INT32 Y1,
    INT32 X2,
    INT32 Y2,
    CHAR16 Character,
    TUI_COLOR Foreground,
    TUI_COLOR Background
)
{
    /* Bresenham's line algorithm */
    INT32 dx = abs(X2 - X1);
    INT32 dy = abs(Y2 - Y1);
    INT32 sx = (X1 < X2) ? 1 : -1;
    INT32 sy = (Y1 < Y2) ? 1 : -1;
    INT32 err = dx - dy;

    INT32 x = X1;
    INT32 y = Y1;

    while (TRUE) {
        Surface_SetChar(This, x, y, Character, Foreground, Background);

        if (x == X2 && y == Y2) break;

        INT32 e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }

    return S_OK;
}

/* Write text */
static HRESULT ANXAPI Surface_WriteText(
    ITuiSurface *This,
    INT32 X,
    INT32 Y,
    CONST CHAR8 *Text,
    TUI_COLOR Foreground,
    TUI_COLOR Background
)
{
    if (!Text) return E_INVALIDARG;

    INT32 currentX = X;
    while (*Text) {
        Surface_SetChar(This, currentX++, Y, *Text++, Foreground, Background);
    }

    return S_OK;
}

/* Clear surface */
static HRESULT ANXAPI Surface_Clear(
    ITuiSurface *This,
    TUI_COLOR Background
)
{
    TuiSurfaceImpl *impl = (TuiSurfaceImpl *)This;

    for (UINT32 y = 0; y < impl->Height; y++) {
        for (UINT32 x = 0; x < impl->Width; x++) {
            impl->Cells[y][x].Character = ' ';
            impl->Cells[y][x].Foreground = impl->DefaultForeground;
            impl->Cells[y][x].Background = Background;
            impl->Cells[y][x].Attributes = 0;
            impl->Cells[y][x].Dirty = TRUE;
        }
    }

    return S_OK;
}

/* Flush surface to screen */
static HRESULT ANXAPI Surface_Flush(
    ITuiSurface *This,
    INT32 OffsetX,
    INT32 OffsetY
)
{
    TuiSurfaceImpl *impl = (TuiSurfaceImpl *)This;

    if (!impl->Screen) return E_FAIL;

    /* Copy dirty cells to screen */
    for (UINT32 y = 0; y < impl->Height; y++) {
        for (UINT32 x = 0; x < impl->Width; x++) {
            SurfaceCell *cell = &impl->Cells[y][x];
            if (cell->Dirty) {
                /* Convert CHAR16 to CHAR8 for now */
                CHAR8 ch = (CHAR8)(cell->Character & 0xFF);
                impl->Screen->Vtbl->WriteChar(
                    impl->Screen,
                    OffsetX + x,
                    OffsetY + y,
                    ch,
                    cell->Foreground,
                    cell->Background
                );
                cell->Dirty = FALSE;
            }
        }
    }

    return S_OK;
}

/* VTable */
static ITuiSurfaceVtbl SurfaceVtbl = {
    Surface_QueryInterface,
    Surface_AddRef,
    Surface_Release,
    Surface_SetClipRect,
    Surface_GetClipRect,
    Surface_SetChar,
    Surface_GetChar,
    Surface_SetAttributes,
    Surface_FillRect,
    Surface_StrokeRect,
    Surface_DrawLine,
    Surface_WriteText,
    Surface_Clear,
    Surface_Flush
};

/* Factory function */
HRESULT AnxTuiCreateSurface(
    UINT32 Width,
    UINT32 Height,
    ITuiScreen *Screen,
    ITuiSurface **OutSurface
)
{
    TuiSurfaceImpl *impl;
    HRESULT hr;

    if (!OutSurface) return E_INVALIDARG;
    if (Width == 0 || Height == 0) return E_INVALIDARG;

    impl = (TuiSurfaceImpl *)malloc(sizeof(TuiSurfaceImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiSurfaceImpl));
    impl->Interface.Vtbl = &SurfaceVtbl;
    impl->RefCount = 1;

    impl->Width = Width;
    impl->Height = Height;
    impl->DefaultForeground = TuiColorWhite;
    impl->DefaultBackground = TuiColorBlack;
    impl->ClippingEnabled = FALSE;

    /* Allocate buffer */
    impl->Cells = (SurfaceCell **)malloc(Height * sizeof(SurfaceCell *));
    if (!impl->Cells) {
        free(impl);
        return E_OUTOFMEMORY;
    }

    for (UINT32 i = 0; i < Height; i++) {
        impl->Cells[i] = (SurfaceCell *)malloc(Width * sizeof(SurfaceCell));
        if (!impl->Cells[i]) {
            /* Cleanup */
            for (UINT32 j = 0; j < i; j++) {
                free(impl->Cells[j]);
            }
            free(impl->Cells);
            free(impl);
            return E_OUTOFMEMORY;
        }

        /* Initialize cells */
        for (UINT32 x = 0; x < Width; x++) {
            impl->Cells[i][x].Character = ' ';
            impl->Cells[i][x].Foreground = impl->DefaultForeground;
            impl->Cells[i][x].Background = impl->DefaultBackground;
            impl->Cells[i][x].Attributes = 0;
            impl->Cells[i][x].Dirty = TRUE;
        }
    }

    /* Reference screen if provided */
    if (Screen) {
        impl->Screen = Screen;
        Screen->Vtbl->AddRef(Screen);
    }

    *OutSurface = &impl->Interface;
    return S_OK;
}
