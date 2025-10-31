/*
 * Checkbox Widget Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

typedef struct {
    ITuiCheckbox Interface;
    WIDGET_STATE State;
    CHAR8 Label[256];
    BOOLEAN Checked;
    BOOLEAN Tristate;
    UINT8 TristateValue;  /* 0=N, 1=M, 2=Y */
} TuiCheckboxImpl;

/* IUnknown methods */
static HRESULT ANXAPI Checkbox_QueryInterface(
    ITuiCheckbox *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;

    /* For now, just return the same interface */
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI Checkbox_AddRef(ITuiCheckbox *This)
{
    TuiCheckboxImpl *impl = (TuiCheckboxImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Checkbox_Release(ITuiCheckbox *This)
{
    TuiCheckboxImpl *impl = (TuiCheckboxImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiCheckbox methods */
static HRESULT ANXAPI Checkbox_SetLabel(
    ITuiCheckbox *This,
    CONST CHAR8 *Label
)
{
    TuiCheckboxImpl *impl = (TuiCheckboxImpl *)This;
    if (Label == NULL) return E_POINTER;

    strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
    impl->Label[sizeof(impl->Label) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Checkbox_GetChecked(
    ITuiCheckbox *This,
    BOOLEAN *Checked
)
{
    TuiCheckboxImpl *impl = (TuiCheckboxImpl *)This;
    if (Checked == NULL) return E_POINTER;

    *Checked = impl->Checked;
    return S_OK;
}

static HRESULT ANXAPI Checkbox_SetChecked(
    ITuiCheckbox *This,
    BOOLEAN Checked
)
{
    TuiCheckboxImpl *impl = (TuiCheckboxImpl *)This;
    impl->Checked = Checked;
    if (!impl->Tristate) {
        impl->TristateValue = Checked ? 2 : 0;
    }
    return S_OK;
}

static HRESULT ANXAPI Checkbox_SetTristate(
    ITuiCheckbox *This,
    BOOLEAN Tristate
)
{
    TuiCheckboxImpl *impl = (TuiCheckboxImpl *)This;
    impl->Tristate = Tristate;
    return S_OK;
}

static HRESULT ANXAPI Checkbox_GetTristateValue(
    ITuiCheckbox *This,
    UINT8 *Value
)
{
    TuiCheckboxImpl *impl = (TuiCheckboxImpl *)This;
    if (Value == NULL) return E_POINTER;

    *Value = impl->TristateValue;
    return S_OK;
}

static HRESULT ANXAPI Checkbox_SetTristateValue(
    ITuiCheckbox *This,
    UINT8 Value
)
{
    TuiCheckboxImpl *impl = (TuiCheckboxImpl *)This;
    if (Value > 2) return E_INVALIDARG;

    impl->TristateValue = Value;
    impl->Checked = (Value != 0);
    return S_OK;
}

static HRESULT ANXAPI Checkbox_Render(
    ITuiCheckbox *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    BOOLEAN Focused
)
{
    TuiCheckboxImpl *impl = (TuiCheckboxImpl *)This;
    CHAR8 display[300];
    CHAR8 mark;
    TUI_COLOR fg, bg;

    if (!impl->State.Visible) return S_OK;

    /* Determine checkbox mark */
    if (impl->Tristate) {
        switch (impl->TristateValue) {
            case 0: mark = ' '; break;  /* N */
            case 1: mark = 'M'; break;  /* M (module) */
            case 2: mark = 'X'; break;  /* Y */
            default: mark = ' '; break;
        }
    } else {
        mark = impl->Checked ? 'X' : ' ';
    }

    /* Choose colors */
    fg = impl->State.Enabled ? impl->State.ForegroundColor : TuiColorBrightBlack;
    bg = impl->State.BackgroundColor;

    if (Focused) {
        fg = TuiColorBlack;
        bg = TuiColorCyan;
    }

    /* Format: [X] Label */
    snprintf(display, sizeof(display), "[%c] %s", mark, impl->Label);

    Screen->Vtbl->WriteText(Screen, X, Y, display, fg, bg);

    return S_OK;
}

static HRESULT ANXAPI Checkbox_HandleKey(
    ITuiCheckbox *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiCheckboxImpl *impl = (TuiCheckboxImpl *)This;

    if (!impl->State.Enabled) {
        *Handled = FALSE;
        return S_OK;
    }

    if (Key == TuiKeyEnter || Key == ' ') {
        if (impl->Tristate) {
            /* Cycle through N -> M -> Y -> N */
            impl->TristateValue = (impl->TristateValue + 1) % 3;
            impl->Checked = (impl->TristateValue != 0);
        } else {
            impl->Checked = !impl->Checked;
            impl->TristateValue = impl->Checked ? 2 : 0;
        }
        *Handled = TRUE;
        return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiCheckbox_Vtbl CheckboxVtbl = {
    Checkbox_QueryInterface,
    Checkbox_AddRef,
    Checkbox_Release,
    Checkbox_SetLabel,
    Checkbox_GetChecked,
    Checkbox_SetChecked,
    Checkbox_SetTristate,
    Checkbox_GetTristateValue,
    Checkbox_SetTristateValue,
    Checkbox_Render,
    Checkbox_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateCheckbox(OUT ITuiCheckbox **Checkbox)
{
    TuiCheckboxImpl *impl;

    if (Checkbox == NULL) return E_POINTER;

    impl = (TuiCheckboxImpl *)calloc(1, sizeof(TuiCheckboxImpl));
    if (impl == NULL) {
        *Checkbox = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &CheckboxVtbl;
    InitWidgetState(&impl->State);
    impl->Label[0] = '\0';
    impl->Checked = FALSE;
    impl->Tristate = FALSE;
    impl->TristateValue = 0;

    *Checkbox = &impl->Interface;
    return S_OK;
}
