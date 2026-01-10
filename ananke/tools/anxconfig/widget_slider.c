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
    ITuiWidget WidgetInterface;
    ITuiKeyListener KeyListener;
    ITuiMouseListener MouseListener;
    ITuiDrawListener DrawListener;

    WIDGET_STATE State;
    SliderOrientation Orientation;
    INT32 MinValue;
    INT32 MaxValue;
    INT32 CurrentValue;
    INT32 Step;
    UINT32 Length;
    BOOLEAN ShowValue;
    BOOLEAN Dragging;
    HRESULT (*ChangeCallback)(VOID *UserData, INT32 NewValue);
    VOID *UserData;
    ITuiResponder *NextResponder;
    ITuiSurface *Surface;
} TuiSliderImpl;

/* Helper macros for interface conversions */
#define SLIDER_FROM_WIDGET(w) ((TuiSliderImpl*)((UINT8*)(w) - offsetof(TuiSliderImpl, WidgetInterface)))
#define SLIDER_FROM_KEY(k) ((TuiSliderImpl*)((UINT8*)(k) - offsetof(TuiSliderImpl, KeyListener)))
#define SLIDER_FROM_MOUSE(m) ((TuiSliderImpl*)((UINT8*)(m) - offsetof(TuiSliderImpl, MouseListener)))
#define SLIDER_FROM_DRAW(d) ((TuiSliderImpl*)((UINT8*)(d) - offsetof(TuiSliderImpl, DrawListener)))

/* Helper: Compute thumb position */
static INT32 ComputeThumbPosition(TuiSliderImpl *impl)
{
    INT32 range = impl->MaxValue - impl->MinValue;
    if (range <= 0) return 0;

    return ((impl->CurrentValue - impl->MinValue) * (impl->Length - 1)) / range;
}

/*=============================================================================
 * ITuiWidget Implementation
 *===========================================================================*/

static HRESULT ANXAPI SliderWidget_QueryInterface(ITuiWidget *This, REFIID riid, VOID **ppvObject)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    if (!ppvObject) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ITuiWidget)) {
        *ppvObject = &impl->WidgetInterface;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiKeyListener)) {
        *ppvObject = &impl->KeyListener;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiMouseListener)) {
        *ppvObject = &impl->MouseListener;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiDrawListener)) {
        *ppvObject = &impl->DrawListener;
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

static UINTN ANXAPI SliderWidget_AddRef(ITuiWidget *This)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI SliderWidget_Release(ITuiWidget *This)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        if (impl->Surface) impl->Surface->Vtbl->Release(impl->Surface);
        free(impl);
    }
    return refCount;
}

static HRESULT ANXAPI SliderWidget_GetBounds(ITuiWidget *This, TUI_RECT *Bounds)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    if (!Bounds) return E_POINTER;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_SetBounds(ITuiWidget *This, CONST TUI_RECT *Bounds)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    if (!Bounds) return E_POINTER;
    impl->State.Bounds = *Bounds;
    if (impl->Orientation == SliderHorizontal) {
        impl->Length = Bounds->Right - Bounds->Left - 2;
    } else {
        impl->Length = Bounds->Bottom - Bounds->Top - 2;
    }
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_GetPreferredSize(ITuiWidget *This, UINT32 *Width, UINT32 *Height)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    if (!Width || !Height) return E_POINTER;
    if (impl->Orientation == SliderHorizontal) {
        *Width = impl->Length + 2 + (impl->ShowValue ? 10 : 0);
        *Height = 1;
    } else {
        *Width = impl->ShowValue ? 10 : 1;
        *Height = impl->Length + 2;
    }
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_SetVisible(ITuiWidget *This, BOOLEAN Visible)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    impl->State.Visible = Visible;
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_IsVisible(ITuiWidget *This, BOOLEAN *Visible)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    if (!Visible) return E_POINTER;
    *Visible = impl->State.Visible;
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_SetEnabled(ITuiWidget *This, BOOLEAN Enabled)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    impl->State.Enabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_IsEnabled(ITuiWidget *This, BOOLEAN *Enabled)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    if (!Enabled) return E_POINTER;
    *Enabled = impl->State.Enabled;
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_SetFocused(ITuiWidget *This, BOOLEAN Focused)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    impl->State.Focused = Focused;
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_IsFocused(ITuiWidget *This, BOOLEAN *Focused)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    if (!Focused) return E_POINTER;
    *Focused = impl->State.Focused;
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_SetColors(ITuiWidget *This, TUI_COLOR Foreground, TUI_COLOR Background)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    impl->State.ForegroundColor = Foreground;
    impl->State.BackgroundColor = Background;
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_GetColors(ITuiWidget *This, TUI_COLOR *Foreground, TUI_COLOR *Background)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    if (Foreground) *Foreground = impl->State.ForegroundColor;
    if (Background) *Background = impl->State.BackgroundColor;
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_Invalidate(ITuiWidget *This, CONST TUI_RECT *Region)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    if (impl->Surface) {
        return impl->Surface->Vtbl->Invalidate(impl->Surface, Region);
    }
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_SetSurface(ITuiWidget *This, ITuiSurface *Surface)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    if (impl->Surface) impl->Surface->Vtbl->Release(impl->Surface);
    impl->Surface = Surface;
    if (Surface) Surface->Vtbl->AddRef(Surface);
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_GetSurface(ITuiWidget *This, ITuiSurface **Surface)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    if (!Surface) return E_POINTER;
    *Surface = impl->Surface;
    if (impl->Surface) impl->Surface->Vtbl->AddRef(impl->Surface);
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_GetNextResponder(ITuiWidget *This, ITuiResponder **NextResponder)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    if (!NextResponder) return E_POINTER;
    *NextResponder = impl->NextResponder;
    return S_OK;
}

static HRESULT ANXAPI SliderWidget_SetNextResponder(ITuiWidget *This, ITuiResponder *NextResponder)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(This);
    impl->NextResponder = NextResponder;
    return S_OK;
}

static CONST ITuiWidget_Vtbl SliderWidgetVtbl = {
    SliderWidget_QueryInterface, SliderWidget_AddRef, SliderWidget_Release,
    SliderWidget_GetBounds, SliderWidget_SetBounds, SliderWidget_GetPreferredSize,
    SliderWidget_SetVisible, SliderWidget_IsVisible, SliderWidget_SetEnabled,
    SliderWidget_IsEnabled, SliderWidget_SetFocused, SliderWidget_IsFocused,
    SliderWidget_SetColors, SliderWidget_GetColors, SliderWidget_Invalidate,
    SliderWidget_SetSurface, SliderWidget_GetSurface, SliderWidget_GetNextResponder,
    SliderWidget_SetNextResponder
};

/*=============================================================================
 * ITuiKeyListener Implementation
 *===========================================================================*/

static HRESULT ANXAPI SliderKey_QueryInterface(ITuiKeyListener *This, REFIID riid, VOID **ppvObject)
{
    TuiSliderImpl *impl = SLIDER_FROM_KEY(This);
    return SliderWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI SliderKey_AddRef(ITuiKeyListener *This)
{
    TuiSliderImpl *impl = SLIDER_FROM_KEY(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI SliderKey_Release(ITuiKeyListener *This)
{
    TuiSliderImpl *impl = SLIDER_FROM_KEY(This);
    return SliderWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI SliderKey_OnKeyDown(ITuiKeyListener *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled)
{
    TuiSliderImpl *impl = SLIDER_FROM_KEY(This);
    INT32 newValue;

    *Handled = FALSE;

    if (!impl->State.Enabled) return S_OK;

    switch (Key) {
        case TuiKeyLeft:
        case TuiKeyDown:
            newValue = impl->CurrentValue - impl->Step;
            if (newValue < impl->MinValue) newValue = impl->MinValue;
            if (newValue > impl->MaxValue) newValue = impl->MaxValue;
            impl->CurrentValue = newValue;
            if (impl->ChangeCallback) impl->ChangeCallback(impl->UserData, newValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyRight:
        case TuiKeyUp:
            newValue = impl->CurrentValue + impl->Step;
            if (newValue < impl->MinValue) newValue = impl->MinValue;
            if (newValue > impl->MaxValue) newValue = impl->MaxValue;
            impl->CurrentValue = newValue;
            if (impl->ChangeCallback) impl->ChangeCallback(impl->UserData, newValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyHome:
            impl->CurrentValue = impl->MinValue;
            if (impl->ChangeCallback) impl->ChangeCallback(impl->UserData, impl->MinValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEnd:
            impl->CurrentValue = impl->MaxValue;
            if (impl->ChangeCallback) impl->ChangeCallback(impl->UserData, impl->MaxValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageUp:
            newValue = impl->CurrentValue + impl->Step * 10;
            if (newValue < impl->MinValue) newValue = impl->MinValue;
            if (newValue > impl->MaxValue) newValue = impl->MaxValue;
            impl->CurrentValue = newValue;
            if (impl->ChangeCallback) impl->ChangeCallback(impl->UserData, newValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageDown:
            newValue = impl->CurrentValue - impl->Step * 10;
            if (newValue < impl->MinValue) newValue = impl->MinValue;
            if (newValue > impl->MaxValue) newValue = impl->MaxValue;
            impl->CurrentValue = newValue;
            if (impl->ChangeCallback) impl->ChangeCallback(impl->UserData, newValue);
            *Handled = TRUE;
            return S_OK;
    }

    return S_OK;
}

static HRESULT ANXAPI SliderKey_OnKeyUp(ITuiKeyListener *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled)
{
    *Handled = FALSE;
    return S_OK;
}

static CONST ITuiKeyListener_Vtbl SliderKeyVtbl = {
    SliderKey_QueryInterface, SliderKey_AddRef, SliderKey_Release,
    SliderKey_OnKeyDown, SliderKey_OnKeyUp
};

/*=============================================================================
 * ITuiMouseListener Implementation
 *===========================================================================*/

static HRESULT ANXAPI SliderMouse_QueryInterface(ITuiMouseListener *This, REFIID riid, VOID **ppvObject)
{
    TuiSliderImpl *impl = SLIDER_FROM_MOUSE(This);
    return SliderWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI SliderMouse_AddRef(ITuiMouseListener *This)
{
    TuiSliderImpl *impl = SLIDER_FROM_MOUSE(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI SliderMouse_Release(ITuiMouseListener *This)
{
    TuiSliderImpl *impl = SLIDER_FROM_MOUSE(This);
    return SliderWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI SliderMouse_OnMouseEvent(ITuiMouseListener *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled)
{
    TuiSliderImpl *impl = SLIDER_FROM_MOUSE(This);
    INT32 newValue;
    INT32 range;
    INT32 relX, relY;

    *Handled = FALSE;

    if (!impl->State.Enabled) return S_OK;

    BOOLEAN isOver = IsPointInWidget(&impl->State, Event->X, Event->Y);

    if (!isOver) {
        impl->Dragging = FALSE;
        return S_OK;
    }

    if (Event->Type == TuiMouseLeftDown) {
        impl->Dragging = TRUE;
        *Handled = TRUE;
        return S_OK;

    } else if (Event->Type == TuiMouseLeftUp) {
        impl->Dragging = FALSE;
        *Handled = TRUE;
        return S_OK;

    } else if (Event->Type == TuiMouseMove && impl->Dragging) {
        range = impl->MaxValue - impl->MinValue;

        if (impl->Orientation == SliderHorizontal) {
            relX = Event->X - impl->State.Bounds.Left - 1;
            if (relX < 0) relX = 0;
            if ((UINT32)relX >= impl->Length) relX = impl->Length - 1;

            newValue = impl->MinValue + (relX * range) / (impl->Length - 1);
        } else {
            relY = Event->Y - impl->State.Bounds.Top - 1;
            if (relY < 0) relY = 0;
            if ((UINT32)relY >= impl->Length) relY = impl->Length - 1;

            newValue = impl->MinValue + (relY * range) / (impl->Length - 1);
        }

        impl->CurrentValue = newValue;
        if (impl->ChangeCallback) impl->ChangeCallback(impl->UserData, newValue);
        *Handled = TRUE;
        return S_OK;
    }

    return S_OK;
}

static CONST ITuiMouseListener_Vtbl SliderMouseVtbl = {
    SliderMouse_QueryInterface, SliderMouse_AddRef, SliderMouse_Release,
    SliderMouse_OnMouseEvent
};

/*=============================================================================
 * ITuiDrawListener Implementation
 *===========================================================================*/

static HRESULT ANXAPI SliderDraw_QueryInterface(ITuiDrawListener *This, REFIID riid, VOID **ppvObject)
{
    TuiSliderImpl *impl = SLIDER_FROM_DRAW(This);
    return SliderWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI SliderDraw_AddRef(ITuiDrawListener *This)
{
    TuiSliderImpl *impl = SLIDER_FROM_DRAW(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI SliderDraw_Release(ITuiDrawListener *This)
{
    TuiSliderImpl *impl = SLIDER_FROM_DRAW(This);
    return SliderWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI SliderDraw_OnDraw(ITuiDrawListener *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect)
{
    TuiSliderImpl *impl = SLIDER_FROM_DRAW(This);
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
        Surface->Vtbl->WriteText(Surface, 0, 0, "[", fg, bg);
        for (i = 0; i < impl->Length; i++) {
            if ((INT32)i == thumbPos) {
                /* Thumb */
                Surface->Vtbl->WriteText(Surface, 1 + i, 0, "●", TuiColorCyan, bg);
            } else {
                /* Track */
                Surface->Vtbl->WriteText(Surface, 1 + i, 0, "─", TuiColorBrightBlack, bg);
            }
        }
        Surface->Vtbl->WriteText(Surface, 1 + impl->Length, 0, "]", fg, bg);

        /* Show value if requested */
        if (impl->ShowValue) {
            snprintf(valueStr, sizeof(valueStr), " %d", impl->CurrentValue);
            Surface->Vtbl->WriteText(Surface, 3 + impl->Length, 0, valueStr, fg, bg);
        }

    } else {
        /* Vertical slider */
        /* Top */
        Surface->Vtbl->WriteText(Surface, 0, 0, "▲", fg, bg);

        /* Track */
        for (i = 0; i < impl->Length; i++) {
            if ((INT32)i == thumbPos) {
                /* Thumb */
                Surface->Vtbl->WriteText(Surface, 0, 1 + i, "●", TuiColorCyan, bg);
            } else {
                /* Track */
                Surface->Vtbl->WriteText(Surface, 0, 1 + i, "│", TuiColorBrightBlack, bg);
            }
        }

        /* Bottom */
        Surface->Vtbl->WriteText(Surface, 0, 1 + impl->Length, "▼", fg, bg);

        /* Show value if requested */
        if (impl->ShowValue) {
            snprintf(valueStr, sizeof(valueStr), "%d", impl->CurrentValue);
            Surface->Vtbl->WriteText(Surface, 2, impl->Length / 2, valueStr, fg, bg);
        }
    }

    return S_OK;
}

static CONST ITuiDrawListener_Vtbl SliderDrawVtbl = {
    SliderDraw_QueryInterface, SliderDraw_AddRef, SliderDraw_Release,
    SliderDraw_OnDraw
};

/*=============================================================================
 * Factory Function
 *===========================================================================*/

HRESULT ANXAPI AnxTuiCreateSlider(IN BOOLEAN Horizontal, IN UINT32 Length, OUT ITuiWidget **Widget)
{
    TuiSliderImpl *impl;

    if (Widget == NULL) return E_POINTER;

    impl = (TuiSliderImpl *)calloc(1, sizeof(TuiSliderImpl));
    if (impl == NULL) {
        *Widget = NULL;
        return E_OUTOFMEMORY;
    }

    /* Initialize vtables */
    impl->WidgetInterface.Vtbl = &SliderWidgetVtbl;
    impl->KeyListener.Vtbl = &SliderKeyVtbl;
    impl->MouseListener.Vtbl = &SliderMouseVtbl;
    impl->DrawListener.Vtbl = &SliderDrawVtbl;

    /* Initialize widget state */
    InitWidgetState(&impl->State);

    /* Initialize slider-specific state */
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
    impl->NextResponder = NULL;
    impl->Surface = NULL;

    *Widget = &impl->WidgetInterface;
    return S_OK;
}

/* Convenience functions for slider-specific operations */
HRESULT ANXAPI AnxTuiSliderSetOrientation(ITuiWidget *Widget, BOOLEAN Horizontal)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(Widget);
    impl->Orientation = Horizontal ? SliderHorizontal : SliderVertical;
    return S_OK;
}

HRESULT ANXAPI AnxTuiSliderSetRange(ITuiWidget *Widget, INT32 MinValue, INT32 MaxValue)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(Widget);

    if (MinValue >= MaxValue) return E_INVALIDARG;

    impl->MinValue = MinValue;
    impl->MaxValue = MaxValue;

    /* Clamp current value */
    if (impl->CurrentValue < MinValue) impl->CurrentValue = MinValue;
    if (impl->CurrentValue > MaxValue) impl->CurrentValue = MaxValue;

    return S_OK;
}

HRESULT ANXAPI AnxTuiSliderSetValue(ITuiWidget *Widget, INT32 Value)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(Widget);

    if (Value < impl->MinValue) Value = impl->MinValue;
    if (Value > impl->MaxValue) Value = impl->MaxValue;

    impl->CurrentValue = Value;

    /* Call change callback */
    if (impl->ChangeCallback != NULL) {
        impl->ChangeCallback(impl->UserData, Value);
    }

    return S_OK;
}

HRESULT ANXAPI AnxTuiSliderGetValue(ITuiWidget *Widget, INT32 *Value)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(Widget);
    if (Value == NULL) return E_POINTER;
    *Value = impl->CurrentValue;
    return S_OK;
}

HRESULT ANXAPI AnxTuiSliderSetStep(ITuiWidget *Widget, INT32 Step)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(Widget);
    impl->Step = Step;
    return S_OK;
}

HRESULT ANXAPI AnxTuiSliderSetLength(ITuiWidget *Widget, UINT32 Length)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(Widget);
    impl->Length = Length;
    return S_OK;
}

HRESULT ANXAPI AnxTuiSliderSetShowValue(ITuiWidget *Widget, BOOLEAN ShowValue)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(Widget);
    impl->ShowValue = ShowValue;
    return S_OK;
}

HRESULT ANXAPI AnxTuiSliderSetChangeCallback(ITuiWidget *Widget, HRESULT (*Callback)(VOID *UserData, INT32 NewValue), VOID *UserData)
{
    TuiSliderImpl *impl = SLIDER_FROM_WIDGET(Widget);
    impl->ChangeCallback = Callback;
    impl->UserData = UserData;
    return S_OK;
}
