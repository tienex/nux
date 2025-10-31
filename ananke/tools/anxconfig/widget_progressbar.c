/*
 * ProgressBar Widget Implementation
 *
 * Visual indicator of progress for long-running operations.
 * Supports determinate (with known progress) and indeterminate
 * (activity indicator) modes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

typedef enum {
    ProgressStyleBlocks,       /* ████████░░░░ */
    ProgressStyleBar,          /* [========>  ] */
    ProgressStyleDots,         /* ●●●●●○○○○○○○ */
    ProgressStyleNumbers       /* 45% [======>] */
} ProgressStyle;

typedef struct {
    ITuiWidget WidgetInterface;
    ITuiProgressBar ProgressBarInterface;
    ITuiDrawListener DrawListener;

    WIDGET_STATE State;
    CHAR8 Label[128];
    INT32 MinValue;
    INT32 MaxValue;
    INT32 CurrentValue;
    UINT32 Width;
    ProgressStyle Style;
    BOOLEAN ShowPercentage;
    BOOLEAN Indeterminate;
    UINT32 IndeterminatePos;
    ITuiResponder *NextResponder;
    ITuiSurface *Surface;
} TuiProgressBarImpl;

/* Helper macros for interface conversions */
#define PROGRESSBAR_FROM_WIDGET(w) ((TuiProgressBarImpl*)((UINT8*)(w) - offsetof(TuiProgressBarImpl, WidgetInterface)))
#define PROGRESSBAR_FROM_PROGRESSBAR(p) ((TuiProgressBarImpl*)((UINT8*)(p) - offsetof(TuiProgressBarImpl, ProgressBarInterface)))
#define PROGRESSBAR_FROM_DRAW(d) ((TuiProgressBarImpl*)((UINT8*)(d) - offsetof(TuiProgressBarImpl, DrawListener)))

/*=============================================================================
 * ITuiWidget Implementation
 *===========================================================================*/

static HRESULT ANXAPI ProgressBarWidget_QueryInterface(ITuiWidget *This, REFIID riid, VOID **ppvObject)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    if (!ppvObject) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ITuiWidget)) {
        *ppvObject = &impl->WidgetInterface;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiDrawListener)) {
        *ppvObject = &impl->DrawListener;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiProgressBar)) {
        *ppvObject = &impl->ProgressBarInterface;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiSerializable)) {
        *ppvObject = &impl->WidgetInterface;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiResponder)) {
        *ppvObject = &impl->WidgetInterface;
        impl->State.RefCount++;
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI ProgressBarWidget_AddRef(ITuiWidget *This)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ProgressBarWidget_Release(ITuiWidget *This)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        if (impl->Surface) impl->Surface->Vtbl->Release(impl->Surface);
        free(impl);
    }
    return refCount;
}

static HRESULT ANXAPI ProgressBarWidget_GetBounds(ITuiWidget *This, TUI_RECT *Bounds)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    if (!Bounds) return E_POINTER;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_SetBounds(ITuiWidget *This, CONST TUI_RECT *Bounds)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    if (!Bounds) return E_POINTER;
    impl->State.Bounds = *Bounds;
    impl->Width = Bounds->Right - Bounds->Left;
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_GetPreferredSize(ITuiWidget *This, UINT32 *Width, UINT32 *Height)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    if (!Width || !Height) return E_POINTER;
    *Width = impl->Width + (impl->ShowPercentage ? 5 : 0);
    *Height = strlen(impl->Label) > 0 ? 2 : 1;
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_SetVisible(ITuiWidget *This, BOOLEAN Visible)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    impl->State.Visible = Visible;
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_IsVisible(ITuiWidget *This, BOOLEAN *Visible)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    if (!Visible) return E_POINTER;
    *Visible = impl->State.Visible;
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_SetEnabled(ITuiWidget *This, BOOLEAN Enabled)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    impl->State.Enabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_IsEnabled(ITuiWidget *This, BOOLEAN *Enabled)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    if (!Enabled) return E_POINTER;
    *Enabled = impl->State.Enabled;
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_SetFocused(ITuiWidget *This, BOOLEAN Focused)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    impl->State.Focused = Focused;
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_IsFocused(ITuiWidget *This, BOOLEAN *Focused)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    if (!Focused) return E_POINTER;
    *Focused = impl->State.Focused;
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_SetColors(ITuiWidget *This, TUI_COLOR Foreground, TUI_COLOR Background)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    impl->State.ForegroundColor = Foreground;
    impl->State.BackgroundColor = Background;
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_GetColors(ITuiWidget *This, TUI_COLOR *Foreground, TUI_COLOR *Background)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    if (Foreground) *Foreground = impl->State.ForegroundColor;
    if (Background) *Background = impl->State.BackgroundColor;
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_Invalidate(ITuiWidget *This, CONST TUI_RECT *Region)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    if (impl->Surface) {
        return impl->Surface->Vtbl->Invalidate(impl->Surface, Region);
    }
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_SetSurface(ITuiWidget *This, ITuiSurface *Surface)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    if (impl->Surface) impl->Surface->Vtbl->Release(impl->Surface);
    impl->Surface = Surface;
    if (Surface) Surface->Vtbl->AddRef(Surface);
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_GetSurface(ITuiWidget *This, ITuiSurface **Surface)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    if (!Surface) return E_POINTER;
    *Surface = impl->Surface;
    if (impl->Surface) impl->Surface->Vtbl->AddRef(impl->Surface);
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_GetNextResponder(ITuiWidget *This, ITuiResponder **NextResponder)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    if (!NextResponder) return E_POINTER;
    *NextResponder = impl->NextResponder;
    return S_OK;
}

static HRESULT ANXAPI ProgressBarWidget_SetNextResponder(ITuiWidget *This, ITuiResponder *NextResponder)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_WIDGET(This);
    impl->NextResponder = NextResponder;
    return S_OK;
}

static CONST ITuiWidget_Vtbl ProgressBarWidgetVtbl = {
    ProgressBarWidget_QueryInterface,
    ProgressBarWidget_AddRef,
    ProgressBarWidget_Release,
    ProgressBarWidget_GetBounds,
    ProgressBarWidget_SetBounds,
    ProgressBarWidget_GetPreferredSize,
    ProgressBarWidget_SetVisible,
    ProgressBarWidget_IsVisible,
    ProgressBarWidget_SetEnabled,
    ProgressBarWidget_IsEnabled,
    ProgressBarWidget_SetFocused,
    ProgressBarWidget_IsFocused,
    ProgressBarWidget_SetColors,
    ProgressBarWidget_GetColors,
    ProgressBarWidget_Invalidate,
    ProgressBarWidget_SetSurface,
    ProgressBarWidget_GetSurface,
    ProgressBarWidget_GetNextResponder,
    ProgressBarWidget_SetNextResponder
};

/*=============================================================================
 * ITuiDrawListener Implementation
 *===========================================================================*/

static HRESULT ANXAPI ProgressBarDraw_QueryInterface(ITuiDrawListener *This, REFIID riid, VOID **ppvObject)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_DRAW(This);
    return ProgressBarWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI ProgressBarDraw_AddRef(ITuiDrawListener *This)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_DRAW(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ProgressBarDraw_Release(ITuiDrawListener *This)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_DRAW(This);
    return ProgressBarWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI ProgressBarDraw_OnDraw(ITuiDrawListener *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_DRAW(This);
    CHAR8 display[256];
    UINT32 i;
    INT32 percentage;
    INT32 filledWidth;
    TUI_COLOR fg, bg;
    INT32 currentY = 0;

    if (!impl->State.Visible) return S_OK;

    fg = impl->State.ForegroundColor;
    bg = impl->State.BackgroundColor;

    /* Render label if present */
    if (strlen(impl->Label) > 0) {
        Surface->Vtbl->WriteText(Surface, 0, currentY, impl->Label, fg, bg);
        currentY++;
    }

    if (impl->Indeterminate) {
        /* Indeterminate mode - activity indicator */
        switch (impl->Style) {
            case ProgressStyleBlocks:
            case ProgressStyleBar:
                /* Animated moving block */
                display[0] = '[';
                for (i = 0; i < impl->Width; i++) {
                    if (i >= impl->IndeterminatePos && i < impl->IndeterminatePos + 5) {
                        display[i + 1] = '=';
                    } else {
                        display[i + 1] = ' ';
                    }
                }
                display[impl->Width + 1] = ']';
                display[impl->Width + 2] = '\0';
                Surface->Vtbl->WriteText(Surface, 0, currentY, display, fg, bg);

                /* Animate */
                impl->IndeterminatePos++;
                if (impl->IndeterminatePos >= impl->Width) {
                    impl->IndeterminatePos = 0;
                }
                break;

            case ProgressStyleDots:
                /* Spinning dots */
                {
                    CONST CHAR8 *spinner[] = { "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏" };
                    snprintf(display, sizeof(display), "%s Working...",
                             spinner[impl->IndeterminatePos % 10]);
                    Surface->Vtbl->WriteText(Surface, 0, currentY, display, fg, bg);
                    impl->IndeterminatePos++;
                }
                break;

            default:
                break;
        }

    } else {
        /* Determinate mode - show actual progress */
        INT32 range = impl->MaxValue - impl->MinValue;
        if (range <= 0) range = 1;

        percentage = ((impl->CurrentValue - impl->MinValue) * 100) / range;
        filledWidth = ((impl->CurrentValue - impl->MinValue) * impl->Width) / range;

        switch (impl->Style) {
            case ProgressStyleBlocks:
                /* ████████░░░░ 45% */
                for (i = 0; i < impl->Width; i++) {
                    if ((INT32)i < filledWidth) {
                        display[i] = '█';
                    } else {
                        display[i] = '░';
                    }
                }
                display[impl->Width] = '\0';
                Surface->Vtbl->WriteText(Surface, 0, currentY, display, TuiColorGreen, bg);

                if (impl->ShowPercentage) {
                    snprintf(display, sizeof(display), " %d%%", percentage);
                    Surface->Vtbl->WriteText(Surface, impl->Width + 1, currentY, display, fg, bg);
                }
                break;

            case ProgressStyleBar:
                /* [========>  ] 45% */
                display[0] = '[';
                for (i = 0; i < impl->Width; i++) {
                    if ((INT32)i < filledWidth - 1) {
                        display[i + 1] = '=';
                    } else if ((INT32)i == filledWidth - 1) {
                        display[i + 1] = '>';
                    } else {
                        display[i + 1] = ' ';
                    }
                }
                display[impl->Width + 1] = ']';
                display[impl->Width + 2] = '\0';
                Surface->Vtbl->WriteText(Surface, 0, currentY, display, fg, bg);

                if (impl->ShowPercentage) {
                    snprintf(display, sizeof(display), " %d%%", percentage);
                    Surface->Vtbl->WriteText(Surface, impl->Width + 3, currentY, display, fg, bg);
                }
                break;

            case ProgressStyleDots:
                /* ●●●●●○○○○○○○ 45% */
                for (i = 0; i < impl->Width; i++) {
                    if ((INT32)i < filledWidth) {
                        display[i] = '●';
                    } else {
                        display[i] = '○';
                    }
                }
                display[impl->Width] = '\0';
                Surface->Vtbl->WriteText(Surface, 0, currentY, display, TuiColorGreen, bg);

                if (impl->ShowPercentage) {
                    snprintf(display, sizeof(display), " %d%%", percentage);
                    Surface->Vtbl->WriteText(Surface, impl->Width + 1, currentY, display, fg, bg);
                }
                break;

            case ProgressStyleNumbers:
                /* 45% [======>] */
                snprintf(display, sizeof(display), "%3d%% ", percentage);
                Surface->Vtbl->WriteText(Surface, 0, currentY, display, fg, bg);

                display[0] = '[';
                for (i = 0; i < impl->Width; i++) {
                    if ((INT32)i < filledWidth - 1) {
                        display[i + 1] = '=';
                    } else if ((INT32)i == filledWidth - 1) {
                        display[i + 1] = '>';
                    } else {
                        display[i + 1] = ' ';
                    }
                }
                display[impl->Width + 1] = ']';
                display[impl->Width + 2] = '\0';
                Surface->Vtbl->WriteText(Surface, 5, currentY, display, fg, bg);
                break;
        }
    }

    return S_OK;
}

static CONST ITuiDrawListener_Vtbl ProgressBarDrawVtbl = {
    ProgressBarDraw_QueryInterface,
    ProgressBarDraw_AddRef,
    ProgressBarDraw_Release,
    ProgressBarDraw_OnDraw
};

/*=============================================================================
 * ITuiProgressBar Implementation (Backward Compatibility)
 *===========================================================================*/

static HRESULT ANXAPI ProgressBar_QueryInterface(ITuiProgressBar *This, REFIID riid, VOID **ppvObject)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_PROGRESSBAR(This);
    return ProgressBarWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI ProgressBar_AddRef(ITuiProgressBar *This)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_PROGRESSBAR(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ProgressBar_Release(ITuiProgressBar *This)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_PROGRESSBAR(This);
    return ProgressBarWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI ProgressBar_SetLabel(ITuiProgressBar *This, CONST CHAR8 *Label)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_PROGRESSBAR(This);
    if (Label == NULL) return E_POINTER;

    strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
    impl->Label[sizeof(impl->Label) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI ProgressBar_SetRange(ITuiProgressBar *This, INT32 MinValue, INT32 MaxValue)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_PROGRESSBAR(This);

    if (MinValue >= MaxValue) return E_INVALIDARG;

    impl->MinValue = MinValue;
    impl->MaxValue = MaxValue;

    /* Clamp current value */
    if (impl->CurrentValue < MinValue) impl->CurrentValue = MinValue;
    if (impl->CurrentValue > MaxValue) impl->CurrentValue = MaxValue;

    return S_OK;
}

static HRESULT ANXAPI ProgressBar_SetValue(ITuiProgressBar *This, INT32 Value)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_PROGRESSBAR(This);

    if (Value < impl->MinValue) Value = impl->MinValue;
    if (Value > impl->MaxValue) Value = impl->MaxValue;

    impl->CurrentValue = Value;

    return S_OK;
}

static HRESULT ANXAPI ProgressBar_GetValue(ITuiProgressBar *This, INT32 *Value)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_PROGRESSBAR(This);
    if (Value == NULL) return E_POINTER;
    *Value = impl->CurrentValue;
    return S_OK;
}

static HRESULT ANXAPI ProgressBar_SetWidth(ITuiProgressBar *This, UINT32 Width)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_PROGRESSBAR(This);
    impl->Width = Width;
    return S_OK;
}

static HRESULT ANXAPI ProgressBar_SetStyle(ITuiProgressBar *This, UINT32 Style)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_PROGRESSBAR(This);
    impl->Style = (ProgressStyle)Style;
    return S_OK;
}

static HRESULT ANXAPI ProgressBar_SetShowPercentage(ITuiProgressBar *This, BOOLEAN ShowPercentage)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_PROGRESSBAR(This);
    impl->ShowPercentage = ShowPercentage;
    return S_OK;
}

static HRESULT ANXAPI ProgressBar_SetIndeterminate(ITuiProgressBar *This, BOOLEAN Indeterminate)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_PROGRESSBAR(This);
    impl->Indeterminate = Indeterminate;
    if (Indeterminate) {
        impl->IndeterminatePos = 0;
    }
    return S_OK;
}

static HRESULT ANXAPI ProgressBar_Increment(ITuiProgressBar *This, INT32 Delta)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_PROGRESSBAR(This);
    return ProgressBar_SetValue(This, impl->CurrentValue + Delta);
}

static HRESULT ANXAPI ProgressBar_Render(ITuiProgressBar *This, ITuiScreen *Screen, INT32 X, INT32 Y)
{
    TuiProgressBarImpl *impl = PROGRESSBAR_FROM_PROGRESSBAR(This);

    /* Legacy render method - delegate to OnDraw */
    if (impl->Surface) {
        TUI_RECT rect = { X, Y, X + impl->Width, Y + 2 };
        return ProgressBarDraw_OnDraw(&impl->DrawListener, impl->Surface, &rect);
    }

    return S_OK;
}

static CONST ITuiProgressBar_Vtbl ProgressBarVtbl = {
    ProgressBar_QueryInterface,
    ProgressBar_AddRef,
    ProgressBar_Release,
    ProgressBar_SetLabel,
    ProgressBar_SetRange,
    ProgressBar_SetValue,
    ProgressBar_GetValue,
    ProgressBar_SetWidth,
    ProgressBar_SetStyle,
    ProgressBar_SetShowPercentage,
    ProgressBar_SetIndeterminate,
    ProgressBar_Increment,
    ProgressBar_Render
};

/*=============================================================================
 * Factory Function
 *===========================================================================*/

HRESULT ANXAPI AnxTuiCreateProgressBar(IN UINT32 Width, OUT ITuiProgressBar **ProgressBar)
{
    TuiProgressBarImpl *impl;

    if (ProgressBar == NULL) return E_POINTER;

    impl = (TuiProgressBarImpl *)calloc(1, sizeof(TuiProgressBarImpl));
    if (impl == NULL) {
        *ProgressBar = NULL;
        return E_OUTOFMEMORY;
    }

    /* Initialize vtables */
    impl->WidgetInterface.Vtbl = &ProgressBarWidgetVtbl;
    impl->ProgressBarInterface.Vtbl = &ProgressBarVtbl;
    impl->DrawListener.Vtbl = &ProgressBarDrawVtbl;

    /* Initialize widget state */
    InitWidgetState(&impl->State);

    /* Initialize progressbar-specific state */
    impl->Label[0] = '\0';
    impl->MinValue = 0;
    impl->MaxValue = 100;
    impl->CurrentValue = 0;
    impl->Width = Width > 0 ? Width : 40;
    impl->Style = ProgressStyleBar;
    impl->ShowPercentage = TRUE;
    impl->Indeterminate = FALSE;
    impl->IndeterminatePos = 0;
    impl->NextResponder = NULL;
    impl->Surface = NULL;

    *ProgressBar = &impl->ProgressBarInterface;
    return S_OK;
}
