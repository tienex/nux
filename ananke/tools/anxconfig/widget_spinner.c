/*
 * Spinner Widget Implementation
 *
 * Numeric input with up/down buttons for incrementing/decrementing values.
 * Also known as spin box or up-down control.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "widgets_common.h"

typedef struct {
    ITuiSpinner Interface;
    WIDGET_STATE State;
    CHAR8 Label[128];
    INT32 MinValue;
    INT32 MaxValue;
    INT32 CurrentValue;
    INT32 Step;
    UINT32 Width;
    BOOLEAN Wrap;              /* Wrap around at min/max */
    BOOLEAN ButtonUpHovered;
    BOOLEAN ButtonDownHovered;
    HRESULT (*ChangeCallback)(VOID *UserData, INT32 NewValue);
    VOID *UserData;
} TuiSpinnerImpl;

/* IUnknown methods */
static HRESULT ANXAPI Spinner_QueryInterface(
    ITuiSpinner *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI Spinner_AddRef(ITuiSpinner *This)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Spinner_Release(ITuiSpinner *This)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiSpinner methods */
static HRESULT ANXAPI Spinner_SetLabel(
    ITuiSpinner *This,
    CONST CHAR8 *Label
)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;
    if (Label == NULL) return E_POINTER;

    strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
    impl->Label[sizeof(impl->Label) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Spinner_SetRange(
    ITuiSpinner *This,
    INT32 MinValue,
    INT32 MaxValue
)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;

    if (MinValue >= MaxValue) return E_INVALIDARG;

    impl->MinValue = MinValue;
    impl->MaxValue = MaxValue;

    /* Clamp current value */
    if (impl->CurrentValue < MinValue) impl->CurrentValue = MinValue;
    if (impl->CurrentValue > MaxValue) impl->CurrentValue = MaxValue;

    return S_OK;
}

static HRESULT ANXAPI Spinner_SetValue(
    ITuiSpinner *This,
    INT32 Value
)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;

    if (Value < impl->MinValue) {
        Value = impl->Wrap ? impl->MaxValue : impl->MinValue;
    }
    if (Value > impl->MaxValue) {
        Value = impl->Wrap ? impl->MinValue : impl->MaxValue;
    }

    impl->CurrentValue = Value;

    /* Call change callback */
    if (impl->ChangeCallback != NULL) {
        impl->ChangeCallback(impl->UserData, Value);
    }

    return S_OK;
}

static HRESULT ANXAPI Spinner_GetValue(
    ITuiSpinner *This,
    INT32 *Value
)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;
    if (Value == NULL) return E_POINTER;
    *Value = impl->CurrentValue;
    return S_OK;
}

static HRESULT ANXAPI Spinner_SetStep(
    ITuiSpinner *This,
    INT32 Step
)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;
    impl->Step = Step;
    return S_OK;
}

static HRESULT ANXAPI Spinner_SetWrap(
    ITuiSpinner *This,
    BOOLEAN Wrap
)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;
    impl->Wrap = Wrap;
    return S_OK;
}

static HRESULT ANXAPI Spinner_SetWidth(
    ITuiSpinner *This,
    UINT32 Width
)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;
    impl->Width = Width;
    return S_OK;
}

static HRESULT ANXAPI Spinner_SetChangeCallback(
    ITuiSpinner *This,
    HRESULT (*Callback)(VOID *UserData, INT32 NewValue),
    VOID *UserData
)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;
    impl->ChangeCallback = Callback;
    impl->UserData = UserData;
    return S_OK;
}

static HRESULT ANXAPI Spinner_Increment(ITuiSpinner *This)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;
    return Spinner_SetValue(This, impl->CurrentValue + impl->Step);
}

static HRESULT ANXAPI Spinner_Decrement(ITuiSpinner *This)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;
    return Spinner_SetValue(This, impl->CurrentValue - impl->Step);
}

static HRESULT ANXAPI Spinner_Render(
    ITuiSpinner *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;
    CHAR8 display[256];
    CHAR8 valueStr[32];
    TUI_COLOR fg, bg;
    TUI_COLOR btnFg, btnBg;

    if (!impl->State.Visible) return S_OK;

    /* Choose colors */
    if (!impl->State.Enabled) {
        fg = TuiColorBrightBlack;
        bg = TuiColorBlack;
        btnFg = TuiColorBrightBlack;
        btnBg = TuiColorBlack;
    } else if (impl->State.Focused) {
        fg = TuiColorBlack;
        bg = TuiColorWhite;
        btnFg = TuiColorBlack;
        btnBg = TuiColorCyan;
    } else {
        fg = TuiColorWhite;
        bg = TuiColorBlue;
        btnFg = TuiColorWhite;
        btnBg = TuiColorBlue;
    }

    /* Format: Label: [  Value  ▲▼] */
    snprintf(valueStr, sizeof(valueStr), "%d", impl->CurrentValue);

    if (strlen(impl->Label) > 0) {
        snprintf(display, sizeof(display), "%s: ", impl->Label);
        Screen->Vtbl->WriteText(Screen, X, Y, display, fg, bg);
        X += strlen(display);
    }

    /* Value display */
    snprintf(display, sizeof(display), "[%*s]",
             (int)impl->Width, valueStr);
    Screen->Vtbl->WriteText(Screen, X, Y, display, fg, bg);

    /* Up button */
    if (impl->ButtonUpHovered && impl->State.Enabled) {
        Screen->Vtbl->WriteText(Screen, X + impl->Width + 1, Y, "▲",
                                TuiColorBlack, TuiColorYellow);
    } else {
        Screen->Vtbl->WriteText(Screen, X + impl->Width + 1, Y, "▲",
                                btnFg, btnBg);
    }

    /* Down button */
    if (impl->ButtonDownHovered && impl->State.Enabled) {
        Screen->Vtbl->WriteText(Screen, X + impl->Width + 2, Y, "▼",
                                TuiColorBlack, TuiColorYellow);
    } else {
        Screen->Vtbl->WriteText(Screen, X + impl->Width + 2, Y, "▼",
                                btnFg, btnBg);
    }

    return S_OK;
}

static HRESULT ANXAPI Spinner_HandleKey(
    ITuiSpinner *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;

    if (!impl->State.Enabled) {
        *Handled = FALSE;
        return S_OK;
    }

    switch (Key) {
        case TuiKeyUp:
        case '+':
            Spinner_Increment(This);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyDown:
        case '-':
            Spinner_Decrement(This);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageUp:
            /* Large increment */
            Spinner_SetValue(This, impl->CurrentValue + impl->Step * 10);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageDown:
            /* Large decrement */
            Spinner_SetValue(This, impl->CurrentValue - impl->Step * 10);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyHome:
            Spinner_SetValue(This, impl->MinValue);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEnd:
            Spinner_SetValue(This, impl->MaxValue);
            *Handled = TRUE;
            return S_OK;

        default:
            /* Handle numeric input */
            if (Key >= '0' && Key <= '9') {
                /* Direct numeric input (simplified) */
                /* Would need a proper input buffer for multi-digit entry */
                *Handled = FALSE;
                return S_OK;
            }
            break;
    }

    *Handled = FALSE;
    return S_OK;
}

static HRESULT ANXAPI Spinner_HandleMouse(
    ITuiSpinner *This,
    CONST TUI_MOUSE_EVENT *Event,
    BOOLEAN *Handled
)
{
    TuiSpinnerImpl *impl = (TuiSpinnerImpl *)This;

    if (!impl->State.Enabled) {
        *Handled = FALSE;
        return S_OK;
    }

    /* Calculate button positions */
    INT32 upButtonX = impl->State.Bounds.X + impl->Width + 1;
    INT32 downButtonX = impl->State.Bounds.X + impl->Width + 2;
    INT32 buttonY = impl->State.Bounds.Y;

    if (Event->Type == TuiMouseMove) {
        /* Update hover states */
        impl->ButtonUpHovered = (Event->X == upButtonX && Event->Y == buttonY);
        impl->ButtonDownHovered = (Event->X == downButtonX && Event->Y == buttonY);
        *Handled = TRUE;
        return S_OK;

    } else if (Event->Type == TuiMouseLeftDown) {
        /* Check if clicked on up button */
        if (Event->X == upButtonX && Event->Y == buttonY) {
            Spinner_Increment(This);
            *Handled = TRUE;
            return S_OK;
        }

        /* Check if clicked on down button */
        if (Event->X == downButtonX && Event->Y == buttonY) {
            Spinner_Decrement(This);
            *Handled = TRUE;
            return S_OK;
        }
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiSpinner_Vtbl SpinnerVtbl = {
    Spinner_QueryInterface,
    Spinner_AddRef,
    Spinner_Release,
    Spinner_SetLabel,
    Spinner_SetRange,
    Spinner_SetValue,
    Spinner_GetValue,
    Spinner_SetStep,
    Spinner_SetWrap,
    Spinner_SetWidth,
    Spinner_SetChangeCallback,
    Spinner_Increment,
    Spinner_Decrement,
    Spinner_Render,
    Spinner_HandleKey,
    Spinner_HandleMouse
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateSpinner(
    IN  CONST CHAR8 *Label,
    IN  INT32 MinValue,
    IN  INT32 MaxValue,
    OUT ITuiSpinner **Spinner
)
{
    TuiSpinnerImpl *impl;

    if (Spinner == NULL) return E_POINTER;

    impl = (TuiSpinnerImpl *)calloc(1, sizeof(TuiSpinnerImpl));
    if (impl == NULL) {
        *Spinner = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &SpinnerVtbl;
    InitWidgetState(&impl->State);

    if (Label != NULL) {
        strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
        impl->Label[sizeof(impl->Label) - 1] = '\0';
    } else {
        impl->Label[0] = '\0';
    }

    impl->MinValue = MinValue;
    impl->MaxValue = MaxValue;
    impl->CurrentValue = MinValue;
    impl->Step = 1;
    impl->Width = 10;
    impl->Wrap = FALSE;
    impl->ButtonUpHovered = FALSE;
    impl->ButtonDownHovered = FALSE;
    impl->ChangeCallback = NULL;
    impl->UserData = NULL;

    *Spinner = &impl->Interface;
    return S_OK;
}
