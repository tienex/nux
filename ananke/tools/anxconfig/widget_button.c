/*
 * Button Widget Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

typedef struct {
    ITuiButton Interface;
    WIDGET_STATE State;
    CHAR8 Label[256];
    HRESULT (*Callback)(VOID *UserData);
    VOID *UserData;
    BOOLEAN Pressed;
} TuiButtonImpl;

/* IUnknown methods */
static HRESULT ANXAPI Button_QueryInterface(
    ITuiButton *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI Button_AddRef(ITuiButton *This)
{
    TuiButtonImpl *impl = (TuiButtonImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Button_Release(ITuiButton *This)
{
    TuiButtonImpl *impl = (TuiButtonImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiButton methods */
static HRESULT ANXAPI Button_SetLabel(
    ITuiButton *This,
    CONST CHAR8 *Label
)
{
    TuiButtonImpl *impl = (TuiButtonImpl *)This;
    if (Label == NULL) return E_POINTER;

    strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
    impl->Label[sizeof(impl->Label) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Button_SetCallback(
    ITuiButton *This,
    HRESULT (*Callback)(VOID *UserData),
    VOID *UserData
)
{
    TuiButtonImpl *impl = (TuiButtonImpl *)This;
    impl->Callback = Callback;
    impl->UserData = UserData;
    return S_OK;
}

static HRESULT ANXAPI Button_Render(
    ITuiButton *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    BOOLEAN Focused
)
{
    TuiButtonImpl *impl = (TuiButtonImpl *)This;
    CHAR8 display[300];
    TUI_COLOR fg, bg;
    UINT32 width;

    if (!impl->State.Visible) return S_OK;

    width = strlen(impl->Label) + 4;  /* [ Label ] */

    /* Choose colors */
    if (!impl->State.Enabled) {
        fg = TuiColorBrightBlack;
        bg = TuiColorBlack;
    } else if (impl->Pressed) {
        fg = TuiColorWhite;
        bg = TuiColorBlue;
    } else if (Focused) {
        fg = TuiColorBlack;
        bg = TuiColorYellow;
    } else {
        fg = TuiColorWhite;
        bg = TuiColorBlue;
    }

    /* Format: < Label > */
    snprintf(display, sizeof(display), "< %s >", impl->Label);

    Screen->Vtbl->WriteText(Screen, X, Y, display, fg, bg);

    return S_OK;
}

static HRESULT ANXAPI Button_HandleKey(
    ITuiButton *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiButtonImpl *impl = (TuiButtonImpl *)This;
    HRESULT hr;

    if (!impl->State.Enabled) {
        *Handled = FALSE;
        return S_OK;
    }

    if (Key == TuiKeyEnter || Key == ' ') {
        impl->Pressed = TRUE;

        /* Call callback if registered */
        if (impl->Callback != NULL) {
            hr = impl->Callback(impl->UserData);
            if (FAILED(hr)) {
                impl->Pressed = FALSE;
                return hr;
            }
        }

        impl->Pressed = FALSE;
        *Handled = TRUE;
        return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiButton_Vtbl ButtonVtbl = {
    Button_QueryInterface,
    Button_AddRef,
    Button_Release,
    Button_SetLabel,
    Button_SetCallback,
    Button_Render,
    Button_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateButton(
    IN  CONST CHAR8 *Label,
    OUT ITuiButton **Button
)
{
    TuiButtonImpl *impl;

    if (Button == NULL) return E_POINTER;

    impl = (TuiButtonImpl *)calloc(1, sizeof(TuiButtonImpl));
    if (impl == NULL) {
        *Button = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &ButtonVtbl;
    InitWidgetState(&impl->State);

    if (Label != NULL) {
        strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
        impl->Label[sizeof(impl->Label) - 1] = '\0';
    } else {
        impl->Label[0] = '\0';
    }

    impl->Callback = NULL;
    impl->UserData = NULL;
    impl->Pressed = FALSE;

    *Button = &impl->Interface;
    return S_OK;
}
