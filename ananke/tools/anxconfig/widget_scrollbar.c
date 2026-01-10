/*
 * ScrollBar Widget Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

typedef enum {
    ScrollbarVertical,
    ScrollbarHorizontal
} ScrollbarOrientation;

typedef struct {
    ITuiScrollBar Interface;
    WIDGET_STATE State;
    ScrollbarOrientation Orientation;
    INT32 MinValue;
    INT32 MaxValue;
    INT32 CurrentValue;
    INT32 PageSize;      /* Amount to scroll on Page Up/Down */
    INT32 ThumbSize;     /* Visual size of thumb (computed) */
    INT32 ThumbPos;      /* Visual position of thumb (computed) */
    BOOLEAN Dragging;
    INT32 DragStartPos;
    HRESULT (*ChangeCallback)(VOID *UserData, INT32 NewValue);
    VOID *UserData;
} TuiScrollBarImpl;

/* IUnknown methods */
static HRESULT ANXAPI ScrollBar_QueryInterface(
    ITuiScrollBar *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI ScrollBar_AddRef(ITuiScrollBar *This)
{
    TuiScrollBarImpl *impl = (TuiScrollBarImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ScrollBar_Release(ITuiScrollBar *This)
{
    TuiScrollBarImpl *impl = (TuiScrollBarImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiScrollBar methods */
static HRESULT ANXAPI ScrollBar_SetOrientation(
    ITuiScrollBar *This,
    BOOLEAN Vertical
)
{
    TuiScrollBarImpl *impl = (TuiScrollBarImpl *)This;
    impl->Orientation = Vertical ? ScrollbarVertical : ScrollbarHorizontal;
    return S_OK;
}

static HRESULT ANXAPI ScrollBar_SetRange(
    ITuiScrollBar *This,
    INT32 MinValue,
    INT32 MaxValue
)
{
    TuiScrollBarImpl *impl = (TuiScrollBarImpl *)This;

    if (MinValue >= MaxValue) return E_INVALIDARG;

    impl->MinValue = MinValue;
    impl->MaxValue = MaxValue;

    /* Clamp current value */
    if (impl->CurrentValue < MinValue) impl->CurrentValue = MinValue;
    if (impl->CurrentValue > MaxValue) impl->CurrentValue = MaxValue;

    return S_OK;
}

static HRESULT ANXAPI ScrollBar_SetValue(
    ITuiScrollBar *This,
    INT32 Value
)
{
    TuiScrollBarImpl *impl = (TuiScrollBarImpl *)This;

    if (Value < impl->MinValue) Value = impl->MinValue;
    if (Value > impl->MaxValue) Value = impl->MaxValue;

    impl->CurrentValue = Value;

    /* Call change callback */
    if (impl->ChangeCallback != NULL) {
        impl->ChangeCallback(impl->UserData, Value);
    }

    return S_OK;
}

static HRESULT ANXAPI ScrollBar_GetValue(
    ITuiScrollBar *This,
    INT32 *Value
)
{
    TuiScrollBarImpl *impl = (TuiScrollBarImpl *)This;
    if (Value == NULL) return E_POINTER;
    *Value = impl->CurrentValue;
    return S_OK;
}

static HRESULT ANXAPI ScrollBar_SetPageSize(
    ITuiScrollBar *This,
    INT32 PageSize
)
{
    TuiScrollBarImpl *impl = (TuiScrollBarImpl *)This;
    impl->PageSize = PageSize;
    return S_OK;
}

static HRESULT ANXAPI ScrollBar_SetChangeCallback(
    ITuiScrollBar *This,
    HRESULT (*Callback)(VOID *UserData, INT32 NewValue),
    VOID *UserData
)
{
    TuiScrollBarImpl *impl = (TuiScrollBarImpl *)This;
    impl->ChangeCallback = Callback;
    impl->UserData = UserData;
    return S_OK;
}

/* Helper: Compute thumb position and size */
static VOID ComputeThumb(
    TuiScrollBarImpl *impl,
    UINT32 BarLength
)
{
    INT32 range = impl->MaxValue - impl->MinValue;
    INT32 effectiveLength = BarLength - 2;  /* Subtract arrow buttons */

    if (range <= 0 || effectiveLength <= 0) {
        impl->ThumbSize = 1;
        impl->ThumbPos = 0;
        return;
    }

    /* Compute thumb size (proportional to page size vs range) */
    if (impl->PageSize > 0) {
        impl->ThumbSize = (effectiveLength * impl->PageSize) / (range + impl->PageSize);
        if (impl->ThumbSize < 1) impl->ThumbSize = 1;
    } else {
        impl->ThumbSize = effectiveLength / 10;
        if (impl->ThumbSize < 1) impl->ThumbSize = 1;
    }

    /* Compute thumb position */
    INT32 scrollableLength = effectiveLength - impl->ThumbSize;
    impl->ThumbPos = (scrollableLength * (impl->CurrentValue - impl->MinValue)) / range;
}

static HRESULT ANXAPI ScrollBar_Render(
    ITuiScrollBar *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height
)
{
    TuiScrollBarImpl *impl = (TuiScrollBarImpl *)This;
    TUI_COLOR fg, bg;
    UINT32 i, barLength;

    if (!impl->State.Visible) return S_OK;

    fg = impl->State.Enabled ? impl->State.ForegroundColor : TuiColorBrightBlack;
    bg = impl->State.BackgroundColor;

    if (impl->Orientation == ScrollbarVertical) {
        barLength = Height;

        /* Up arrow */
        Screen->Vtbl->WriteText(Screen, X, Y, "▲", fg, bg);

        /* Compute thumb */
        ComputeThumb(impl, barLength);

        /* Track and thumb */
        for (i = 1; i < barLength - 1; i++) {
            if (i >= (UINT32)(impl->ThumbPos + 1) &&
                i < (UINT32)(impl->ThumbPos + 1 + impl->ThumbSize)) {
                /* Thumb */
                Screen->Vtbl->WriteText(Screen, X, Y + i, "█", fg, bg);
            } else {
                /* Track */
                Screen->Vtbl->WriteText(Screen, X, Y + i, "│",
                                        TuiColorBrightBlack, bg);
            }
        }

        /* Down arrow */
        Screen->Vtbl->WriteText(Screen, X, Y + barLength - 1, "▼", fg, bg);

    } else {
        barLength = Width;

        /* Left arrow */
        Screen->Vtbl->WriteText(Screen, X, Y, "◄", fg, bg);

        /* Compute thumb */
        ComputeThumb(impl, barLength);

        /* Track and thumb */
        for (i = 1; i < barLength - 1; i++) {
            if (i >= (UINT32)(impl->ThumbPos + 1) &&
                i < (UINT32)(impl->ThumbPos + 1 + impl->ThumbSize)) {
                /* Thumb */
                Screen->Vtbl->WriteText(Screen, X + i, Y, "█", fg, bg);
            } else {
                /* Track */
                Screen->Vtbl->WriteText(Screen, X + i, Y, "─",
                                        TuiColorBrightBlack, bg);
            }
        }

        /* Right arrow */
        Screen->Vtbl->WriteText(Screen, X + barLength - 1, Y, "►", fg, bg);
    }

    return S_OK;
}

static HRESULT ANXAPI ScrollBar_HandleKey(
    ITuiScrollBar *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiScrollBarImpl *impl = (TuiScrollBarImpl *)This;
    INT32 newValue;

    if (!impl->State.Enabled) {
        *Handled = FALSE;
        return S_OK;
    }

    switch (Key) {
        case TuiKeyUp:
        case TuiKeyLeft:
            newValue = impl->CurrentValue - 1;
            ScrollBar_SetValue(This, newValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyDown:
        case TuiKeyRight:
            newValue = impl->CurrentValue + 1;
            ScrollBar_SetValue(This, newValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageUp:
            newValue = impl->CurrentValue - impl->PageSize;
            ScrollBar_SetValue(This, newValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageDown:
            newValue = impl->CurrentValue + impl->PageSize;
            ScrollBar_SetValue(This, newValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyHome:
            ScrollBar_SetValue(This, impl->MinValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEnd:
            ScrollBar_SetValue(This, impl->MaxValue);
            *Handled = TRUE;
            return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

static HRESULT ANXAPI ScrollBar_HandleMouse(
    ITuiScrollBar *This,
    CONST TUI_MOUSE_EVENT *Event,
    BOOLEAN *Handled
)
{
    TuiScrollBarImpl *impl = (TuiScrollBarImpl *)This;

    if (!impl->State.Enabled) {
        *Handled = FALSE;
        return S_OK;
    }

    /* Check if mouse is over the scrollbar */
    BOOLEAN isOver = IsPointInWidget(&impl->State, Event->X, Event->Y);

    if (!isOver) {
        impl->Dragging = FALSE;
        *Handled = FALSE;
        return S_OK;
    }

    if (Event->Type == TuiMouseLeftDown) {
        /* Start dragging or click on arrows/track */
        impl->Dragging = TRUE;
        impl->DragStartPos = (impl->Orientation == ScrollbarVertical) ? Event->Y : Event->X;
        *Handled = TRUE;
        return S_OK;

    } else if (Event->Type == TuiMouseLeftUp) {
        impl->Dragging = FALSE;
        *Handled = TRUE;
        return S_OK;

    } else if (Event->Type == TuiMouseMove && impl->Dragging) {
        /* Update value based on drag position */
        /* Simplified implementation */
        *Handled = TRUE;
        return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiScrollBar_Vtbl ScrollBarVtbl = {
    ScrollBar_QueryInterface,
    ScrollBar_AddRef,
    ScrollBar_Release,
    ScrollBar_SetOrientation,
    ScrollBar_SetRange,
    ScrollBar_SetValue,
    ScrollBar_GetValue,
    ScrollBar_SetPageSize,
    ScrollBar_SetChangeCallback,
    ScrollBar_Render,
    ScrollBar_HandleKey,
    ScrollBar_HandleMouse
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateScrollBar(
    IN  BOOLEAN Vertical,
    OUT ITuiScrollBar **ScrollBar
)
{
    TuiScrollBarImpl *impl;

    if (ScrollBar == NULL) return E_POINTER;

    impl = (TuiScrollBarImpl *)calloc(1, sizeof(TuiScrollBarImpl));
    if (impl == NULL) {
        *ScrollBar = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &ScrollBarVtbl;
    InitWidgetState(&impl->State);

    impl->Orientation = Vertical ? ScrollbarVertical : ScrollbarHorizontal;
    impl->MinValue = 0;
    impl->MaxValue = 100;
    impl->CurrentValue = 0;
    impl->PageSize = 10;
    impl->ThumbSize = 1;
    impl->ThumbPos = 0;
    impl->Dragging = FALSE;
    impl->ChangeCallback = NULL;
    impl->UserData = NULL;

    *ScrollBar = &impl->Interface;
    return S_OK;
}
