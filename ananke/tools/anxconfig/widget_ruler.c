/*
 * widget_ruler.c - Text Ruler Widget
 *
 * Horizontal/vertical ruler for text editors showing:
 * - Column/line numbers
 * - Tab stops
 * - Margins (left, right, indent)
 * - Current cursor position indicator
 * - Draggable margin markers
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TAB_STOPS 64
#define MAX_RULER_LENGTH 256

/* Ruler orientation */
typedef enum {
    RulerHorizontal,  /* Top ruler showing columns */
    RulerVertical     /* Left ruler showing lines */
} RulerOrientation;

/* Tab stop type */
typedef enum {
    TabLeft,
    TabCenter,
    TabRight,
    TabDecimal
} TabStopType;

/* Tab stop */
typedef struct {
    UINT32 Position;     /* Column position */
    TabStopType Type;
} TabStop;

typedef struct {
    ITuiRuler Interface;
    WIDGET_STATE State;

    /* Configuration */
    RulerOrientation Orientation;
    UINT32 Length;         /* Ruler length in units */
    UINT32 TickInterval;   /* Major tick every N units */

    /* Margins (for horizontal ruler) */
    UINT32 LeftMargin;
    UINT32 RightMargin;
    UINT32 FirstLineIndent;

    /* Tab stops */
    TabStop TabStops[MAX_TAB_STOPS];
    UINT32 TabStopCount;

    /* Current position indicator */
    UINT32 CurrentPosition;
    BOOLEAN ShowPosition;

    /* Colors */
    TUI_COLOR BackgroundColor;
    TUI_COLOR TextColor;
    TUI_COLOR MarkerColor;
    TUI_COLOR PositionColor;

    /* Callbacks */
    HRESULT (*OnMarginChanged)(VOID *UserData, UINT32 LeftMargin, UINT32 RightMargin, UINT32 Indent);
    HRESULT (*OnTabStopChanged)(VOID *UserData, CONST TabStop *Stops, UINT32 Count);
    VOID *UserData;

} TuiRulerImpl;

/* IUnknown methods */
static HRESULT ANXAPI Ruler_QueryInterface(
    ITuiRuler *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiRuler)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI Ruler_AddRef(ITuiRuler *This)
{
    TuiRulerImpl *impl = (TuiRulerImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Ruler_Release(ITuiRuler *This)
{
    TuiRulerImpl *impl = (TuiRulerImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        free(impl);
    }

    return count;
}

/* Render horizontal ruler */
static VOID RenderHorizontalRuler(
    TuiRulerImpl *impl,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    UINT32 width = impl->State.Bounds.Width;

    /* Draw background */
    for (UINT32 i = 0; i < width; i++) {
        Screen->Vtbl->WriteChar(Screen, X + i, Y, ' ', impl->TextColor, impl->BackgroundColor);
    }

    /* Draw ruler markings */
    for (UINT32 col = 0; col < impl->Length && col < width; col++) {
        CHAR8 marker = ' ';

        if (col % 10 == 0) {
            /* Major tick - show number */
            CHAR8 num[8];
            snprintf(num, sizeof(num), "%u", col);
            Screen->Vtbl->WriteText(Screen, X + col, Y, num, impl->TextColor, impl->BackgroundColor);
        } else if (col % impl->TickInterval == 0) {
            /* Major tick */
            marker = '|';
            Screen->Vtbl->WriteChar(Screen, X + col, Y, marker, impl->TextColor, impl->BackgroundColor);
        } else if (col % 5 == 0) {
            /* Minor tick */
            marker = ':';
            Screen->Vtbl->WriteChar(Screen, X + col, Y, marker, impl->TextColor, impl->BackgroundColor);
        } else {
            /* Normal tick */
            marker = '.';
            Screen->Vtbl->WriteChar(Screen, X + col, Y, marker, TuiColorBrightBlack, impl->BackgroundColor);
        }
    }

    /* Draw left margin marker */
    if (impl->LeftMargin < width) {
        Screen->Vtbl->WriteChar(Screen, X + impl->LeftMargin, Y, '[', impl->MarkerColor, impl->BackgroundColor);
    }

    /* Draw right margin marker */
    if (impl->RightMargin < width) {
        Screen->Vtbl->WriteChar(Screen, X + impl->RightMargin, Y, ']', impl->MarkerColor, impl->BackgroundColor);
    }

    /* Draw first-line indent marker */
    if (impl->FirstLineIndent < width) {
        Screen->Vtbl->WriteChar(Screen, X + impl->FirstLineIndent, Y, gBoxChars.ArrowDown, impl->MarkerColor, impl->BackgroundColor);
    }

    /* Draw tab stops */
    for (UINT32 i = 0; i < impl->TabStopCount; i++) {
        TabStop *tab = &impl->TabStops[i];
        if (tab->Position < width) {
            CHAR8 tabChar;
            switch (tab->Type) {
                case TabLeft:    tabChar = 'L'; break;
                case TabCenter:  tabChar = 'C'; break;
                case TabRight:   tabChar = 'R'; break;
                case TabDecimal: tabChar = 'D'; break;
                default:         tabChar = 'T'; break;
            }
            Screen->Vtbl->WriteChar(Screen, X + tab->Position, Y, tabChar, TuiColorYellow, impl->BackgroundColor);
        }
    }

    /* Draw current position indicator */
    if (impl->ShowPosition && impl->CurrentPosition < width) {
        Screen->Vtbl->WriteChar(Screen, X + impl->CurrentPosition, Y, gBoxChars.ArrowUp, impl->PositionColor, impl->BackgroundColor);
    }
}

/* Render vertical ruler */
static VOID RenderVerticalRuler(
    TuiRulerImpl *impl,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    UINT32 height = impl->State.Bounds.Height;

    /* Draw ruler markings */
    for (UINT32 line = 0; line < impl->Length && line < height; line++) {
        CHAR8 marker[8];

        if (line % 10 == 0) {
            /* Major tick - show number */
            snprintf(marker, sizeof(marker), "%4u", line);
            Screen->Vtbl->WriteText(Screen, X, Y + line, marker, impl->TextColor, impl->BackgroundColor);
        } else if (line % impl->TickInterval == 0) {
            /* Major tick */
            Screen->Vtbl->WriteChar(Screen, X + 3, Y + line, '-', impl->TextColor, impl->BackgroundColor);
        } else {
            /* Minor tick */
            Screen->Vtbl->WriteChar(Screen, X + 3, Y + line, '.', TuiColorBrightBlack, impl->BackgroundColor);
        }

        /* Draw current position indicator */
        if (impl->ShowPosition && line == impl->CurrentPosition) {
            Screen->Vtbl->WriteChar(Screen, X + 2, Y + line, gBoxChars.ArrowRight, impl->PositionColor, impl->BackgroundColor);
        }
    }
}

/* Render the ruler */
static HRESULT ANXAPI Ruler_Render(
    ITuiRuler *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiRulerImpl *impl = (TuiRulerImpl *)This;

    if (!impl->State.Visible) return S_OK;

    if (impl->Orientation == RulerHorizontal) {
        RenderHorizontalRuler(impl, Screen, X, Y);
    } else {
        RenderVerticalRuler(impl, Screen, X, Y);
    }

    return S_OK;
}

/* Set margins */
static HRESULT ANXAPI Ruler_SetMargins(
    ITuiRuler *This,
    UINT32 LeftMargin,
    UINT32 RightMargin,
    UINT32 FirstLineIndent
)
{
    TuiRulerImpl *impl = (TuiRulerImpl *)This;

    impl->LeftMargin = LeftMargin;
    impl->RightMargin = RightMargin;
    impl->FirstLineIndent = FirstLineIndent;

    if (impl->OnMarginChanged) {
        impl->OnMarginChanged(impl->UserData, LeftMargin, RightMargin, FirstLineIndent);
    }

    return S_OK;
}

/* Add tab stop */
static HRESULT ANXAPI Ruler_AddTabStop(
    ITuiRuler *This,
    UINT32 Position,
    UINT32 Type
)
{
    TuiRulerImpl *impl = (TuiRulerImpl *)This;

    if (impl->TabStopCount >= MAX_TAB_STOPS) {
        return E_OUTOFMEMORY;
    }

    TabStop *tab = &impl->TabStops[impl->TabStopCount++];
    tab->Position = Position;
    tab->Type = (TabStopType)Type;

    if (impl->OnTabStopChanged) {
        impl->OnTabStopChanged(impl->UserData, impl->TabStops, impl->TabStopCount);
    }

    return S_OK;
}

/* Clear tab stops */
static HRESULT ANXAPI Ruler_ClearTabStops(ITuiRuler *This)
{
    TuiRulerImpl *impl = (TuiRulerImpl *)This;

    impl->TabStopCount = 0;

    if (impl->OnTabStopChanged) {
        impl->OnTabStopChanged(impl->UserData, impl->TabStops, 0);
    }

    return S_OK;
}

/* Set current position */
static HRESULT ANXAPI Ruler_SetCurrentPosition(
    ITuiRuler *This,
    UINT32 Position
)
{
    TuiRulerImpl *impl = (TuiRulerImpl *)This;

    impl->CurrentPosition = Position;
    impl->ShowPosition = TRUE;

    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI Ruler_SetBounds(ITuiRuler *This, CONST TUI_RECT *Bounds)
{
    TuiRulerImpl *impl = (TuiRulerImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI Ruler_GetBounds(ITuiRuler *This, TUI_RECT *Bounds)
{
    TuiRulerImpl *impl = (TuiRulerImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI Ruler_SetVisible(ITuiRuler *This, BOOLEAN Visible)
{
    TuiRulerImpl *impl = (TuiRulerImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI Ruler_IsVisible(ITuiRuler *This)
{
    TuiRulerImpl *impl = (TuiRulerImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI Ruler_SetEnabled(ITuiRuler *This, BOOLEAN Enabled)
{
    TuiRulerImpl *impl = (TuiRulerImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI Ruler_IsEnabled(ITuiRuler *This)
{
    TuiRulerImpl *impl = (TuiRulerImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiRulerVtbl RulerVtbl = {
    Ruler_QueryInterface,
    Ruler_AddRef,
    Ruler_Release,
    Ruler_Render,
    Ruler_SetBounds,
    Ruler_GetBounds,
    Ruler_SetVisible,
    Ruler_IsVisible,
    Ruler_SetEnabled,
    Ruler_IsEnabled,
    Ruler_SetMargins,
    Ruler_AddTabStop,
    Ruler_ClearTabStops,
    Ruler_SetCurrentPosition
};

/* Factory function */
HRESULT AnxTuiCreateRuler(
    UINT32 Orientation,  /* 0=Horizontal, 1=Vertical */
    UINT32 Length,
    ITuiRuler **OutRuler
)
{
    TuiRulerImpl *impl;

    if (!OutRuler) return E_INVALIDARG;
    if (Length == 0 || Length > MAX_RULER_LENGTH) return E_INVALIDARG;

    impl = (TuiRulerImpl *)malloc(sizeof(TuiRulerImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiRulerImpl));
    impl->Interface.Vtbl = &RulerVtbl;
    InitWidgetState(&impl->State);

    impl->Orientation = (RulerOrientation)Orientation;
    impl->Length = Length;
    impl->TickInterval = 5;

    /* Default margins */
    impl->LeftMargin = 0;
    impl->RightMargin = 80;
    impl->FirstLineIndent = 0;

    impl->TabStopCount = 0;
    impl->CurrentPosition = 0;
    impl->ShowPosition = FALSE;

    /* Colors */
    impl->BackgroundColor = TuiColorBrightBlack;
    impl->TextColor = TuiColorWhite;
    impl->MarkerColor = TuiColorYellow;
    impl->PositionColor = TuiColorCyan;

    if (Orientation == 0) {
        /* Horizontal ruler */
        impl->State.Bounds.Width = Length;
        impl->State.Bounds.Height = 1;
    } else {
        /* Vertical ruler */
        impl->State.Bounds.Width = 5;
        impl->State.Bounds.Height = Length;
    }

    *OutRuler = &impl->Interface;
    return S_OK;
}
