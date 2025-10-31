/*
 * Slider Widget Implementation
 *
 * Horizontal or vertical slider control for selecting values
 * within a range. Supports keyboard and mouse input.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

typedef enum {
    SliderHorizontal,
    SliderVertical
} SliderOrientation;

typedef struct {
    ITuiSlider Interface;
    WIDGET_STATE State;
    SliderOrientation Orientation;
    INT32 MinValue;
    INT32 MaxValue;
    INT32 CurrentValue;
    INT32 Step;                /* Increment/decrement step */
    UINT32 Length;             /* Visual length of slider track */
    BOOLEAN ShowValue;         /* Show numeric value */
    BOOLEAN Dragging;
    HRESULT (*ChangeCallback)(VOID *UserData, INT32 NewValue);
    VOID *UserData;
} TuiSliderImpl;

/* IUnknown methods */
static HRESULT ANXAPI Slider_QueryInterface(
    ITuiSlider *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI Slider_AddRef(ITuiSlider *This)
{
    TuiSliderImpl *impl = (TuiSliderImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Slider_Release(ITuiSlider *This)
{
    TuiSliderImpl *impl = (TuiSliderImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiSlider methods */
static HRESULT ANXAPI Slider_SetOrientation(
    ITuiSlider *This,
    BOOLEAN Horizontal
)
{
    TuiSliderImpl *impl = (TuiSliderImpl *)This;
    impl->Orientation = Horizontal ? SliderHorizontal : SliderVertical;
    return S_OK;
}

static HRESULT ANXAPI Slider_SetRange(
    ITuiSlider *This,
    INT32 MinValue,
    INT32 MaxValue
)
{
    TuiSliderImpl *impl = (TuiSliderImpl *)This;

    if (MinValue >= MaxValue) return E_INVALIDARG;

    impl->MinValue = MinValue;
    impl->MaxValue = MaxValue;

    /* Clamp current value */
    if (impl->CurrentValue < MinValue) impl->CurrentValue = MinValue;
    if (impl->CurrentValue > MaxValue) impl->CurrentValue = MaxValue;

    return S_OK;
}

static HRESULT ANXAPI Slider_SetValue(
    ITuiSlider *This,
    INT32 Value
)
{
    TuiSliderImpl *impl = (TuiSliderImpl *)This;

    if (Value < impl->MinValue) Value = impl->MinValue;
    if (Value > impl->MaxValue) Value = impl->MaxValue;

    impl->CurrentValue = Value;

    /* Call change callback */
    if (impl->ChangeCallback != NULL) {
        impl->ChangeCallback(impl->UserData, Value);
    }

    return S_OK;
}

static HRESULT ANXAPI Slider_GetValue(
    ITuiSlider *This,
    INT32 *Value
)
{
    TuiSliderImpl *impl = (TuiSliderImpl *)This;
    if (Value == NULL) return E_POINTER;
    *Value = impl->CurrentValue;
    return S_OK;
}

static HRESULT ANXAPI Slider_SetStep(
    ITuiSlider *This,
    INT32 Step
)
{
    TuiSliderImpl *impl = (TuiSliderImpl *)This;
    impl->Step = Step;
    return S_OK;
}

static HRESULT ANXAPI Slider_SetLength(
    ITuiSlider *This,
    UINT32 Length
)
{
    TuiSliderImpl *impl = (TuiSliderImpl *)This;
    impl->Length = Length;
    return S_OK;
}

static HRESULT ANXAPI Slider_SetShowValue(
    ITuiSlider *This,
    BOOLEAN ShowValue
)
{
    TuiSliderImpl *impl = (TuiSliderImpl *)This;
    impl->ShowValue = ShowValue;
    return S_OK;
}

static HRESULT ANXAPI Slider_SetChangeCallback(
    ITuiSlider *This,
    HRESULT (*Callback)(VOID *UserData, INT32 NewValue),
    VOID *UserData
)
{
    TuiSliderImpl *impl = (TuiSliderImpl *)This;
    impl->ChangeCallback = Callback;
    impl->UserData = UserData;
    return S_OK;
}

/* Helper: Compute thumb position */
static INT32 ComputeThumbPosition(TuiSliderImpl *impl)
{
    INT32 range = impl->MaxValue - impl->MinValue;
    if (range <= 0) return 0;

    return ((impl->CurrentValue - impl->MinValue) * (impl->Length - 1)) / range;
}

static HRESULT ANXAPI Slider_Render(
    ITuiSlider *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiSliderImpl *impl = (TuiSliderImpl *)This;
    TUI_COLOR fg, bg;
    UINT32 i;
    INT32 thumbPos;
    CHAR8 valueStr[32];

    if (!impl->State.Visible) return S_OK;

    fg = impl->State.Enabled ? impl->State.ForegroundColor : TuiColorBrightBlack;
    bg = impl->State.BackgroundColor;

    if (impl->State.Focused) {
        fg = TuiColorYellow;
    }

    thumbPos = ComputeThumbPosition(impl);

    if (impl->Orientation == SliderHorizontal) {
        /* Horizontal slider: [----●----] */

        /* Draw track */
        Screen->Vtbl->WriteText(Screen, X, Y, "[", fg, bg);
        for (i = 0; i < impl->Length; i++) {
            if ((INT32)i == thumbPos) {
                /* Thumb */
                Screen->Vtbl->WriteText(Screen, X + 1 + i, Y, "●",
                                        TuiColorCyan, bg);
            } else {
                /* Track */
                Screen->Vtbl->WriteText(Screen, X + 1 + i, Y, "─",
                                        TuiColorBrightBlack, bg);
            }
        }
        Screen->Vtbl->WriteText(Screen, X + 1 + impl->Length, Y, "]", fg, bg);

        /* Show value if requested */
        if (impl->ShowValue) {
            snprintf(valueStr, sizeof(valueStr), " %d", impl->CurrentValue);
            Screen->Vtbl->WriteText(Screen, X + 3 + impl->Length, Y, valueStr,
                                    fg, bg);
        }

    } else {
        /* Vertical slider */
        /* Top */
        Screen->Vtbl->WriteText(Screen, X, Y, "▲", fg, bg);

        /* Track */
        for (i = 0; i < impl->Length; i++) {
            if ((INT32)i == thumbPos) {
                /* Thumb */
                Screen->Vtbl->WriteText(Screen, X, Y + 1 + i, "●",
                                        TuiColorCyan, bg);
            } else {
                /* Track */
                Screen->Vtbl->WriteText(Screen, X, Y + 1 + i, "│",
                                        TuiColorBrightBlack, bg);
            }
        }

        /* Bottom */
        Screen->Vtbl->WriteText(Screen, X, Y + 1 + impl->Length, "▼", fg, bg);

        /* Show value if requested */
        if (impl->ShowValue) {
            snprintf(valueStr, sizeof(valueStr), "%d", impl->CurrentValue);
            Screen->Vtbl->WriteText(Screen, X + 2, Y + impl->Length / 2,
                                    valueStr, fg, bg);
        }
    }

    return S_OK;
}

static HRESULT ANXAPI Slider_HandleKey(
    ITuiSlider *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiSliderImpl *impl = (TuiSliderImpl *)This;
    INT32 newValue;

    if (!impl->State.Enabled) {
        *Handled = FALSE;
        return S_OK;
    }

    switch (Key) {
        case TuiKeyLeft:
        case TuiKeyDown:
            /* Decrease value */
            newValue = impl->CurrentValue - impl->Step;
            Slider_SetValue(This, newValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyRight:
        case TuiKeyUp:
            /* Increase value */
            newValue = impl->CurrentValue + impl->Step;
            Slider_SetValue(This, newValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyHome:
            /* Minimum value */
            Slider_SetValue(This, impl->MinValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEnd:
            /* Maximum value */
            Slider_SetValue(This, impl->MaxValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageUp:
            /* Large step up */
            newValue = impl->CurrentValue + impl->Step * 10;
            Slider_SetValue(This, newValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageDown:
            /* Large step down */
            newValue = impl->CurrentValue - impl->Step * 10;
            Slider_SetValue(This, newValue);
            *Handled = TRUE;
            return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

static HRESULT ANXAPI Slider_HandleMouse(
    ITuiSlider *This,
    CONST TUI_MOUSE_EVENT *Event,
    BOOLEAN *Handled
)
{
    TuiSliderImpl *impl = (TuiSliderImpl *)This;

    if (!impl->State.Enabled) {
        *Handled = FALSE;
        return S_OK;
    }

    /* Check if mouse is over the slider */
    BOOLEAN isOver = IsPointInWidget(&impl->State, Event->X, Event->Y);

    if (!isOver) {
        impl->Dragging = FALSE;
        *Handled = FALSE;
        return S_OK;
    }

    if (Event->Type == TuiMouseLeftDown) {
        /* Start dragging */
        impl->Dragging = TRUE;
        *Handled = TRUE;
        return S_OK;

    } else if (Event->Type == TuiMouseLeftUp) {
        impl->Dragging = FALSE;
        *Handled = TRUE;
        return S_OK;

    } else if (Event->Type == TuiMouseMove && impl->Dragging) {
        /* Update value based on mouse position */
        INT32 newValue;
        INT32 range = impl->MaxValue - impl->MinValue;

        if (impl->Orientation == SliderHorizontal) {
            INT32 relX = Event->X - impl->State.Bounds.X - 1;
            if (relX < 0) relX = 0;
            if ((UINT32)relX >= impl->Length) relX = impl->Length - 1;

            newValue = impl->MinValue + (relX * range) / (impl->Length - 1);
        } else {
            INT32 relY = Event->Y - impl->State.Bounds.Y - 1;
            if (relY < 0) relY = 0;
            if ((UINT32)relY >= impl->Length) relY = impl->Length - 1;

            newValue = impl->MinValue + (relY * range) / (impl->Length - 1);
        }

        Slider_SetValue(This, newValue);
        *Handled = TRUE;
        return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiSlider_Vtbl SliderVtbl = {
    Slider_QueryInterface,
    Slider_AddRef,
    Slider_Release,
    Slider_SetOrientation,
    Slider_SetRange,
    Slider_SetValue,
    Slider_GetValue,
    Slider_SetStep,
    Slider_SetLength,
    Slider_SetShowValue,
    Slider_SetChangeCallback,
    Slider_Render,
    Slider_HandleKey,
    Slider_HandleMouse
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateSlider(
    IN  BOOLEAN Horizontal,
    IN  UINT32 Length,
    OUT ITuiSlider **Slider
)
{
    TuiSliderImpl *impl;

    if (Slider == NULL) return E_POINTER;

    impl = (TuiSliderImpl *)calloc(1, sizeof(TuiSliderImpl));
    if (impl == NULL) {
        *Slider = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &SliderVtbl;
    InitWidgetState(&impl->State);

    impl->Orientation = Horizontal ? SliderHorizontal : SliderVertical;
    impl->MinValue = 0;
    impl->MaxValue = 100;
    impl->CurrentValue = 0;
    impl->Step = 1;
    impl->Length = Length > 0 ? Length : 20;
    impl->ShowValue = TRUE;
    impl->Dragging = FALSE;
    impl->ChangeCallback = NULL;
    impl->UserData = NULL;

    *Slider = &impl->Interface;
    return S_OK;
}
