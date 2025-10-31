/*
 * Hyperlink Widget Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

typedef struct {
    ITuiHyperlink Interface;
    WIDGET_STATE State;
    CHAR8 Text[256];
    CHAR8 Url[512];
    HRESULT (*Callback)(VOID *UserData, CONST CHAR8 *Url);
    VOID *UserData;
    BOOLEAN Visited;
    BOOLEAN Hovered;
} TuiHyperlinkImpl;

/* IUnknown methods */
static HRESULT ANXAPI Hyperlink_QueryInterface(
    ITuiHyperlink *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI Hyperlink_AddRef(ITuiHyperlink *This)
{
    TuiHyperlinkImpl *impl = (TuiHyperlinkImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Hyperlink_Release(ITuiHyperlink *This)
{
    TuiHyperlinkImpl *impl = (TuiHyperlinkImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiHyperlink methods */
static HRESULT ANXAPI Hyperlink_SetText(
    ITuiHyperlink *This,
    CONST CHAR8 *Text
)
{
    TuiHyperlinkImpl *impl = (TuiHyperlinkImpl *)This;
    if (Text == NULL) return E_POINTER;

    strncpy(impl->Text, Text, sizeof(impl->Text) - 1);
    impl->Text[sizeof(impl->Text) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Hyperlink_SetUrl(
    ITuiHyperlink *This,
    CONST CHAR8 *Url
)
{
    TuiHyperlinkImpl *impl = (TuiHyperlinkImpl *)This;
    if (Url == NULL) return E_POINTER;

    strncpy(impl->Url, Url, sizeof(impl->Url) - 1);
    impl->Url[sizeof(impl->Url) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Hyperlink_SetCallback(
    ITuiHyperlink *This,
    HRESULT (*Callback)(VOID *UserData, CONST CHAR8 *Url),
    VOID *UserData
)
{
    TuiHyperlinkImpl *impl = (TuiHyperlinkImpl *)This;
    impl->Callback = Callback;
    impl->UserData = UserData;
    return S_OK;
}

static HRESULT ANXAPI Hyperlink_Render(
    ITuiHyperlink *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    BOOLEAN Focused
)
{
    TuiHyperlinkImpl *impl = (TuiHyperlinkImpl *)This;
    TUI_COLOR fg, bg;

    if (!impl->State.Visible) return S_OK;

    /* Choose colors based on state */
    if (!impl->State.Enabled) {
        fg = TuiColorBrightBlack;
        bg = TuiColorBlack;
    } else if (Focused || impl->Hovered) {
        /* Focused/hovered: bright cyan with underline effect */
        fg = TuiColorCyan;
        bg = TuiColorBlack;
    } else if (impl->Visited) {
        /* Visited link: magenta */
        fg = TuiColorMagenta;
        bg = TuiColorBlack;
    } else {
        /* Unvisited link: blue */
        fg = TuiColorBlue;
        bg = TuiColorBlack;
    }

    /* Render link text */
    Screen->Vtbl->WriteText(Screen, X, Y, impl->Text, fg, bg);

    /* Add underline character for focused state */
    if (Focused && impl->State.Enabled) {
        UINT32 i, len = strlen(impl->Text);
        CHAR8 underline[256];
        for (i = 0; i < len && i < sizeof(underline) - 1; i++) {
            underline[i] = '_';
        }
        underline[i] = '\0';
        Screen->Vtbl->WriteText(Screen, X, Y + 1, underline, fg, bg);
    }

    return S_OK;
}

static HRESULT ANXAPI Hyperlink_HandleKey(
    ITuiHyperlink *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiHyperlinkImpl *impl = (TuiHyperlinkImpl *)This;
    HRESULT hr;

    if (!impl->State.Enabled) {
        *Handled = FALSE;
        return S_OK;
    }

    if (Key == TuiKeyEnter || Key == ' ') {
        /* Activate link */
        impl->Visited = TRUE;

        if (impl->Callback != NULL) {
            hr = impl->Callback(impl->UserData, impl->Url);
            if (FAILED(hr)) {
                return hr;
            }
        }

        *Handled = TRUE;
        return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

static HRESULT ANXAPI Hyperlink_HandleMouse(
    ITuiHyperlink *This,
    CONST TUI_MOUSE_EVENT *Event,
    BOOLEAN *Handled
)
{
    TuiHyperlinkImpl *impl = (TuiHyperlinkImpl *)This;
    HRESULT hr;

    if (!impl->State.Enabled) {
        *Handled = FALSE;
        return S_OK;
    }

    /* Check if mouse is over the link */
    BOOLEAN isOver = IsPointInWidget(&impl->State, Event->X, Event->Y);

    if (isOver) {
        if (Event->Type == TuiMouseMove) {
            impl->Hovered = TRUE;
            *Handled = TRUE;
            return S_OK;
        } else if (Event->Type == TuiMouseLeftDown) {
            /* Click to activate */
            impl->Visited = TRUE;

            if (impl->Callback != NULL) {
                hr = impl->Callback(impl->UserData, impl->Url);
                if (FAILED(hr)) {
                    return hr;
                }
            }

            *Handled = TRUE;
            return S_OK;
        }
    } else {
        impl->Hovered = FALSE;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiHyperlink_Vtbl HyperlinkVtbl = {
    Hyperlink_QueryInterface,
    Hyperlink_AddRef,
    Hyperlink_Release,
    Hyperlink_SetText,
    Hyperlink_SetUrl,
    Hyperlink_SetCallback,
    Hyperlink_Render,
    Hyperlink_HandleKey,
    Hyperlink_HandleMouse
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateHyperlink(
    IN  CONST CHAR8 *Text,
    IN  CONST CHAR8 *Url,
    OUT ITuiHyperlink **Hyperlink
)
{
    TuiHyperlinkImpl *impl;

    if (Hyperlink == NULL) return E_POINTER;

    impl = (TuiHyperlinkImpl *)calloc(1, sizeof(TuiHyperlinkImpl));
    if (impl == NULL) {
        *Hyperlink = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &HyperlinkVtbl;
    InitWidgetState(&impl->State);

    if (Text != NULL) {
        strncpy(impl->Text, Text, sizeof(impl->Text) - 1);
        impl->Text[sizeof(impl->Text) - 1] = '\0';
    } else {
        impl->Text[0] = '\0';
    }

    if (Url != NULL) {
        strncpy(impl->Url, Url, sizeof(impl->Url) - 1);
        impl->Url[sizeof(impl->Url) - 1] = '\0';
    } else {
        impl->Url[0] = '\0';
    }

    impl->Callback = NULL;
    impl->UserData = NULL;
    impl->Visited = FALSE;
    impl->Hovered = FALSE;

    *Hyperlink = &impl->Interface;
    return S_OK;
}
