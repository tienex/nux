/*
 * Function Key Bar Widget Implementation
 *
 * Displays F1-F12 keys with their associated labels at the bottom
 * of the screen, like Norton Commander / Midnight Commander.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

#define MAX_FUNCTION_KEYS 12

typedef struct {
    CHAR8 Label[32];
    HRESULT (*Callback)(VOID *UserData);
    VOID *UserData;
    BOOLEAN Enabled;
} FunctionKey;

typedef struct {
    ITuiFKeyBar Interface;
    WIDGET_STATE State;
    FunctionKey Keys[MAX_FUNCTION_KEYS];
    TUI_COLOR KeyColor;
    TUI_COLOR LabelColor;
    TUI_COLOR BackgroundColor;
} TuiFKeyBarImpl;

/* IUnknown methods */
static HRESULT ANXAPI FKeyBar_QueryInterface(
    ITuiFKeyBar *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI FKeyBar_AddRef(ITuiFKeyBar *This)
{
    TuiFKeyBarImpl *impl = (TuiFKeyBarImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI FKeyBar_Release(ITuiFKeyBar *This)
{
    TuiFKeyBarImpl *impl = (TuiFKeyBarImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiFKeyBar methods */
static HRESULT ANXAPI FKeyBar_SetKeyLabel(
    ITuiFKeyBar *This,
    UINT32 KeyIndex,
    CONST CHAR8 *Label
)
{
    TuiFKeyBarImpl *impl = (TuiFKeyBarImpl *)This;

    if (KeyIndex < 1 || KeyIndex > MAX_FUNCTION_KEYS) return E_INVALIDARG;
    if (Label == NULL) return E_POINTER;

    strncpy(impl->Keys[KeyIndex - 1].Label, Label,
            sizeof(impl->Keys[0].Label) - 1);
    impl->Keys[KeyIndex - 1].Label[sizeof(impl->Keys[0].Label) - 1] = '\0';

    return S_OK;
}

static HRESULT ANXAPI FKeyBar_GetKeyLabel(
    ITuiFKeyBar *This,
    UINT32 KeyIndex,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    TuiFKeyBarImpl *impl = (TuiFKeyBarImpl *)This;

    if (KeyIndex < 1 || KeyIndex > MAX_FUNCTION_KEYS) return E_INVALIDARG;
    if (Buffer == NULL) return E_POINTER;

    strncpy(Buffer, impl->Keys[KeyIndex - 1].Label, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';

    return S_OK;
}

static HRESULT ANXAPI FKeyBar_SetKeyCallback(
    ITuiFKeyBar *This,
    UINT32 KeyIndex,
    HRESULT (*Callback)(VOID *UserData),
    VOID *UserData
)
{
    TuiFKeyBarImpl *impl = (TuiFKeyBarImpl *)This;

    if (KeyIndex < 1 || KeyIndex > MAX_FUNCTION_KEYS) return E_INVALIDARG;

    impl->Keys[KeyIndex - 1].Callback = Callback;
    impl->Keys[KeyIndex - 1].UserData = UserData;

    return S_OK;
}

static HRESULT ANXAPI FKeyBar_SetKeyEnabled(
    ITuiFKeyBar *This,
    UINT32 KeyIndex,
    BOOLEAN Enabled
)
{
    TuiFKeyBarImpl *impl = (TuiFKeyBarImpl *)This;

    if (KeyIndex < 1 || KeyIndex > MAX_FUNCTION_KEYS) return E_INVALIDARG;

    impl->Keys[KeyIndex - 1].Enabled = Enabled;

    return S_OK;
}

static HRESULT ANXAPI FKeyBar_ClearAllKeys(ITuiFKeyBar *This)
{
    TuiFKeyBarImpl *impl = (TuiFKeyBarImpl *)This;
    UINT32 i;

    for (i = 0; i < MAX_FUNCTION_KEYS; i++) {
        impl->Keys[i].Label[0] = '\0';
        impl->Keys[i].Callback = NULL;
        impl->Keys[i].UserData = NULL;
        impl->Keys[i].Enabled = TRUE;
    }

    return S_OK;
}

static HRESULT ANXAPI FKeyBar_Render(
    ITuiFKeyBar *This,
    ITuiScreen *Screen,
    INT32 Y
)
{
    TuiFKeyBarImpl *impl = (TuiFKeyBarImpl *)This;
    UINT32 screenWidth, screenHeight;
    UINT32 i;
    INT32 x = 0;
    CHAR8 display[64];
    TUI_COLOR keyFg, labelFg, bg;

    if (!impl->State.Visible) return S_OK;

    Screen->Vtbl->GetDimensions(Screen, &screenWidth, &screenHeight);

    bg = impl->BackgroundColor;
    keyFg = impl->KeyColor;
    labelFg = impl->LabelColor;

    /* Clear the bar background */
    ClearRect(Screen, 0, Y, screenWidth, 1, bg);

    /* Calculate width per key */
    UINT32 keyWidth = screenWidth / MAX_FUNCTION_KEYS;

    /* Render each function key */
    for (i = 0; i < MAX_FUNCTION_KEYS; i++) {
        x = i * keyWidth;

        /* Format: "F1 Help" or just "F1" if no label */
        if (strlen(impl->Keys[i].Label) > 0) {
            snprintf(display, sizeof(display), " F%-2d%-*s",
                     i + 1,
                     (int)(keyWidth - 5),
                     impl->Keys[i].Label);
        } else {
            snprintf(display, sizeof(display), " F%-2d%*s",
                     i + 1,
                     (int)(keyWidth - 4), "");
        }

        /* Truncate if too long */
        if (strlen(display) > keyWidth) {
            display[keyWidth] = '\0';
        }

        /* Render key number */
        CHAR8 keyNum[8];
        snprintf(keyNum, sizeof(keyNum), " F%d ", i + 1);

        if (impl->Keys[i].Enabled) {
            Screen->Vtbl->WriteText(Screen, x, Y, keyNum, keyFg, bg);
        } else {
            Screen->Vtbl->WriteText(Screen, x, Y, keyNum,
                                    TuiColorBrightBlack, bg);
        }

        /* Render label */
        if (strlen(impl->Keys[i].Label) > 0) {
            INT32 labelX = x + strlen(keyNum);
            if (impl->Keys[i].Enabled) {
                Screen->Vtbl->WriteText(Screen, labelX, Y,
                                        impl->Keys[i].Label, labelFg, bg);
            } else {
                Screen->Vtbl->WriteText(Screen, labelX, Y,
                                        impl->Keys[i].Label,
                                        TuiColorBrightBlack, bg);
            }
        }
    }

    return S_OK;
}

static HRESULT ANXAPI FKeyBar_HandleKey(
    ITuiFKeyBar *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiFKeyBarImpl *impl = (TuiFKeyBarImpl *)This;
    UINT32 keyIndex = 0;
    HRESULT hr;

    /* Check if it's a function key */
    if (Key >= TuiKeyF1 && Key <= TuiKeyF12) {
        keyIndex = Key - TuiKeyF1;

        if (impl->Keys[keyIndex].Enabled &&
            impl->Keys[keyIndex].Callback != NULL) {
            hr = impl->Keys[keyIndex].Callback(impl->Keys[keyIndex].UserData);
            *Handled = TRUE;
            return hr;
        }

        *Handled = TRUE;
        return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiFKeyBar_Vtbl FKeyBarVtbl = {
    FKeyBar_QueryInterface,
    FKeyBar_AddRef,
    FKeyBar_Release,
    FKeyBar_SetKeyLabel,
    FKeyBar_GetKeyLabel,
    FKeyBar_SetKeyCallback,
    FKeyBar_SetKeyEnabled,
    FKeyBar_ClearAllKeys,
    FKeyBar_Render,
    FKeyBar_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateFKeyBar(OUT ITuiFKeyBar **FKeyBar)
{
    TuiFKeyBarImpl *impl;
    UINT32 i;

    if (FKeyBar == NULL) return E_POINTER;

    impl = (TuiFKeyBarImpl *)calloc(1, sizeof(TuiFKeyBarImpl));
    if (impl == NULL) {
        *FKeyBar = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &FKeyBarVtbl;
    InitWidgetState(&impl->State);

    /* Initialize all keys */
    for (i = 0; i < MAX_FUNCTION_KEYS; i++) {
        impl->Keys[i].Label[0] = '\0';
        impl->Keys[i].Callback = NULL;
        impl->Keys[i].UserData = NULL;
        impl->Keys[i].Enabled = TRUE;
    }

    /* Set default colors (Norton Commander style) */
    impl->KeyColor = TuiColorBlack;
    impl->LabelColor = TuiColorBlack;
    impl->BackgroundColor = TuiColorCyan;

    *FKeyBar = &impl->Interface;
    return S_OK;
}

/* Helper: Set default keybindings (common pattern) */
HRESULT ANXAPI AnxTuiSetDefaultFKeyBindings(ITuiFKeyBar *FKeyBar)
{
    /* Common defaults like Norton Commander */
    FKeyBar->Vtbl->SetKeyLabel(FKeyBar, 1, "Help");
    FKeyBar->Vtbl->SetKeyLabel(FKeyBar, 2, "Menu");
    FKeyBar->Vtbl->SetKeyLabel(FKeyBar, 3, "View");
    FKeyBar->Vtbl->SetKeyLabel(FKeyBar, 4, "Edit");
    FKeyBar->Vtbl->SetKeyLabel(FKeyBar, 5, "Copy");
    FKeyBar->Vtbl->SetKeyLabel(FKeyBar, 6, "Move");
    FKeyBar->Vtbl->SetKeyLabel(FKeyBar, 7, "NewFld");
    FKeyBar->Vtbl->SetKeyLabel(FKeyBar, 8, "Delete");
    FKeyBar->Vtbl->SetKeyLabel(FKeyBar, 9, "Config");
    FKeyBar->Vtbl->SetKeyLabel(FKeyBar, 10, "Quit");

    return S_OK;
}
