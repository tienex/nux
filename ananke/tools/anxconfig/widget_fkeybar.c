/*
 * Function Key Bar Widget Implementation
 *
 * Displays F1-F12 keys with their associated labels at the bottom
 * of the screen, like Norton Commander / Midnight Commander.
 * Uses new event dispatching architecture with ITuiWidget, ITuiKeyListener, ITuiDrawListener.
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
    ITuiWidget WidgetInterface;
    ITuiKeyListener KeyListener;
    ITuiDrawListener DrawListener;

    WIDGET_STATE State;
    FunctionKey Keys[MAX_FUNCTION_KEYS];
    TUI_COLOR KeyColor;
    TUI_COLOR LabelColor;
    ITuiResponder *NextResponder;
    ITuiSurface *Surface;
} TuiFKeyBarImpl;

/* Helper macros for interface conversions */
#define FKEYBAR_FROM_WIDGET(w) ((TuiFKeyBarImpl*)((UINT8*)(w) - offsetof(TuiFKeyBarImpl, WidgetInterface)))
#define FKEYBAR_FROM_KEY(k) ((TuiFKeyBarImpl*)((UINT8*)(k) - offsetof(TuiFKeyBarImpl, KeyListener)))
#define FKEYBAR_FROM_DRAW(d) ((TuiFKeyBarImpl*)((UINT8*)(d) - offsetof(TuiFKeyBarImpl, DrawListener)))

/*=============================================================================
 * ITuiWidget Implementation
 *===========================================================================*/

static HRESULT ANXAPI FKeyBarWidget_QueryInterface(ITuiWidget *This, REFIID riid, VOID **ppvObject)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
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

static UINTN ANXAPI FKeyBarWidget_AddRef(ITuiWidget *This)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI FKeyBarWidget_Release(ITuiWidget *This)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        if (impl->Surface) impl->Surface->Vtbl->Release(impl->Surface);
        if (impl->NextResponder) impl->NextResponder->Vtbl->Release(impl->NextResponder);
        free(impl);
    }
    return refCount;
}

static HRESULT ANXAPI FKeyBarWidget_SetBounds(ITuiWidget *This, CONST TUI_RECT *Bounds)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_GetBounds(ITuiWidget *This, TUI_RECT *Bounds)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_SetVisible(ITuiWidget *This, BOOLEAN Visible)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    impl->State.Visible = Visible;
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_GetVisible(ITuiWidget *This, BOOLEAN *Visible)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    if (!Visible) return E_INVALIDARG;
    *Visible = impl->State.Visible;
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_SetEnabled(ITuiWidget *This, BOOLEAN Enabled)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    impl->State.Enabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_GetEnabled(ITuiWidget *This, BOOLEAN *Enabled)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    if (!Enabled) return E_INVALIDARG;
    *Enabled = impl->State.Enabled;
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_SetParent(ITuiWidget *This, ITuiWidget *Parent)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    if (impl->NextResponder) {
        impl->NextResponder->Vtbl->Release(impl->NextResponder);
        impl->NextResponder = NULL;
    }
    if (Parent) {
        ITuiResponder *parentResponder = NULL;
        HRESULT hr = Parent->Vtbl->QueryInterface((ITuiWidget *)Parent, &IID_ITuiResponder,
                                                   (VOID **)&parentResponder);
        if (SUCCEEDED(hr)) impl->NextResponder = parentResponder;
    }
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_GetParent(ITuiWidget *This, ITuiWidget **Parent)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    if (!Parent) return E_INVALIDARG;
    if (impl->NextResponder) {
        return impl->NextResponder->Vtbl->QueryInterface(impl->NextResponder,
                                                         &IID_ITuiWidget,
                                                         (VOID **)Parent);
    }
    *Parent = NULL;
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_Invalidate(ITuiWidget *This, CONST TUI_RECT *Rect)
{
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_GetNextResponder(ITuiWidget *This, ITuiResponder **NextResponder)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    if (!NextResponder) return E_INVALIDARG;
    *NextResponder = impl->NextResponder;
    if (impl->NextResponder) impl->NextResponder->Vtbl->AddRef(impl->NextResponder);
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_BecomeFirstResponder(ITuiWidget *This)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    impl->State.Focused = TRUE;
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_ResignFirstResponder(ITuiWidget *This)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    impl->State.Focused = FALSE;
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_SerializeToYaml(ITuiWidget *This, CHAR8 **OutYaml, UINTN *OutLength)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(This);
    CHAR8 *yaml = (CHAR8 *)malloc(2048);
    if (!yaml) return E_OUTOFMEMORY;

    snprintf(yaml, 2048,
        "type: FKeyBar\nbounds:\n  x: %d\n  y: %d\n  width: %d\n  height: %d\n"
        "visible: %s\nenabled: %s\n",
        impl->State.Bounds.X, impl->State.Bounds.Y, impl->State.Bounds.Width, impl->State.Bounds.Height,
        impl->State.Visible ? "true" : "false", impl->State.Enabled ? "true" : "false");

    *OutYaml = yaml;
    *OutLength = strlen(yaml);
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_DeserializeFromYaml(ITuiWidget *This, CONST CHAR8 *Yaml, UINTN Length)
{
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_GetTypeName(ITuiWidget *This, CONST CHAR8 **OutTypeName)
{
    *OutTypeName = "FKeyBar";
    return S_OK;
}

static HRESULT ANXAPI FKeyBarWidget_Clone(ITuiWidget *This, ITuiSerializable **OutClone)
{
    ITuiWidget *newFKeyBar = NULL;
    HRESULT hr = AnxTuiCreateFKeyBar(&newFKeyBar);
    if (FAILED(hr)) return hr;

    *OutClone = (ITuiSerializable *)newFKeyBar;
    return S_OK;
}

static ITuiWidget_Vtbl FKeyBarWidgetVtbl = {
    FKeyBarWidget_QueryInterface, FKeyBarWidget_AddRef, FKeyBarWidget_Release,
    FKeyBarWidget_SetBounds, FKeyBarWidget_GetBounds, FKeyBarWidget_SetVisible, FKeyBarWidget_GetVisible,
    FKeyBarWidget_SetEnabled, FKeyBarWidget_GetEnabled, FKeyBarWidget_SetParent, FKeyBarWidget_GetParent,
    FKeyBarWidget_Invalidate, FKeyBarWidget_GetNextResponder, FKeyBarWidget_BecomeFirstResponder,
    FKeyBarWidget_ResignFirstResponder, FKeyBarWidget_SerializeToYaml, FKeyBarWidget_DeserializeFromYaml,
    FKeyBarWidget_GetTypeName, FKeyBarWidget_Clone
};

/*=============================================================================
 * ITuiKeyListener Implementation
 *===========================================================================*/

static HRESULT ANXAPI FKeyBarKey_QueryInterface(ITuiKeyListener *This, REFIID riid, VOID **ppvObject)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_KEY(This);
    return FKeyBarWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI FKeyBarKey_AddRef(ITuiKeyListener *This)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_KEY(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI FKeyBarKey_Release(ITuiKeyListener *This)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_KEY(This);
    return FKeyBarWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI FKeyBarKey_OnKeyDown(ITuiKeyListener *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_KEY(This);
    UINT32 keyIndex;
    HRESULT hr;

    *Handled = FALSE;

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

    return S_OK;
}

static HRESULT ANXAPI FKeyBarKey_OnKeyUp(ITuiKeyListener *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled)
{
    *Handled = FALSE;
    return S_OK;
}

static HRESULT ANXAPI FKeyBarKey_OnChar(ITuiKeyListener *This, CHAR16 Character, BOOLEAN *Handled)
{
    *Handled = FALSE;
    return S_OK;
}

static CONST ITuiKeyListener_Vtbl FKeyBarKeyVtbl = {
    FKeyBarKey_QueryInterface, FKeyBarKey_AddRef, FKeyBarKey_Release,
    FKeyBarKey_OnKeyDown, FKeyBarKey_OnKeyUp, FKeyBarKey_OnChar
};

/*=============================================================================
 * ITuiDrawListener Implementation
 *===========================================================================*/

static HRESULT ANXAPI FKeyBarDraw_QueryInterface(ITuiDrawListener *This, REFIID riid, VOID **ppvObject)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_DRAW(This);
    return FKeyBarWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI FKeyBarDraw_AddRef(ITuiDrawListener *This)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_DRAW(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI FKeyBarDraw_Release(ITuiDrawListener *This)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_DRAW(This);
    return FKeyBarWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI FKeyBarDraw_OnDraw(ITuiDrawListener *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_DRAW(This);
    UINT32 i;
    INT32 x;
    CHAR8 keyNum[8];
    TUI_COLOR keyFg, labelFg, bg;
    UINT32 width, keyWidth;

    if (!impl->State.Visible) return S_OK;

    width = impl->State.Bounds.Width;
    bg = impl->State.BackgroundColor;
    keyFg = impl->KeyColor;
    labelFg = impl->LabelColor;

    /* Calculate width per key */
    keyWidth = width / MAX_FUNCTION_KEYS;

    /* Render each function key */
    for (i = 0; i < MAX_FUNCTION_KEYS; i++) {
        x = i * keyWidth;

        /* Render key number */
        snprintf(keyNum, sizeof(keyNum), " F%d ", i + 1);

        if (impl->Keys[i].Enabled) {
            Surface->Vtbl->WriteText(Surface, x, 0, keyNum, keyFg, bg);
        } else {
            Surface->Vtbl->WriteText(Surface, x, 0, keyNum,
                                    TuiColorBrightBlack, bg);
        }

        /* Render label */
        if (strlen(impl->Keys[i].Label) > 0) {
            INT32 labelX = x + strlen(keyNum);
            if (impl->Keys[i].Enabled) {
                Surface->Vtbl->WriteText(Surface, labelX, 0,
                                        impl->Keys[i].Label, labelFg, bg);
            } else {
                Surface->Vtbl->WriteText(Surface, labelX, 0,
                                        impl->Keys[i].Label,
                                        TuiColorBrightBlack, bg);
            }
        }
    }

    return S_OK;
}

static HRESULT ANXAPI FKeyBarDraw_OnGetPreferredSize(ITuiDrawListener *This, UINT32 *Width, UINT32 *Height)
{
    if (Width) *Width = 80;  /* Default width */
    if (Height) *Height = 1;  /* FKey bar is always 1 line tall */
    return S_OK;
}

static ITuiDrawListener_Vtbl FKeyBarDrawVtbl = {
    FKeyBarDraw_QueryInterface, FKeyBarDraw_AddRef, FKeyBarDraw_Release,
    FKeyBarDraw_OnDraw, FKeyBarDraw_OnGetPreferredSize
};

/*=============================================================================
 * Factory Function
 *===========================================================================*/

HRESULT ANXAPI AnxTuiCreateFKeyBar(OUT ITuiWidget **Widget)
{
    TuiFKeyBarImpl *impl;
    UINT32 i;

    if (Widget == NULL) return E_POINTER;

    impl = (TuiFKeyBarImpl *)calloc(1, sizeof(TuiFKeyBarImpl));
    if (impl == NULL) {
        *Widget = NULL;
        return E_OUTOFMEMORY;
    }

    /* Initialize vtables */
    impl->WidgetInterface.Vtbl = &FKeyBarWidgetVtbl;
    impl->KeyListener.Vtbl = &FKeyBarKeyVtbl;
    impl->DrawListener.Vtbl = &FKeyBarDrawVtbl;

    /* Initialize widget state */
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
    impl->State.BackgroundColor = TuiColorCyan;

    impl->NextResponder = NULL;
    impl->Surface = NULL;

    *Widget = &impl->WidgetInterface;
    return S_OK;
}

/* Convenience functions for fkeybar-specific operations */
HRESULT ANXAPI AnxTuiFKeyBarSetKeyLabel(ITuiWidget *Widget, UINT32 KeyIndex, CONST CHAR8 *Label)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(Widget);

    if (KeyIndex < 1 || KeyIndex > MAX_FUNCTION_KEYS) return E_INVALIDARG;
    if (Label == NULL) return E_POINTER;

    strncpy(impl->Keys[KeyIndex - 1].Label, Label,
            sizeof(impl->Keys[0].Label) - 1);
    impl->Keys[KeyIndex - 1].Label[sizeof(impl->Keys[0].Label) - 1] = '\0';

    return S_OK;
}

HRESULT ANXAPI AnxTuiFKeyBarGetKeyLabel(ITuiWidget *Widget, UINT32 KeyIndex, CHAR8 *Buffer, UINTN BufferSize)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(Widget);

    if (KeyIndex < 1 || KeyIndex > MAX_FUNCTION_KEYS) return E_INVALIDARG;
    if (Buffer == NULL) return E_POINTER;

    strncpy(Buffer, impl->Keys[KeyIndex - 1].Label, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';

    return S_OK;
}

HRESULT ANXAPI AnxTuiFKeyBarSetKeyCallback(ITuiWidget *Widget, UINT32 KeyIndex,
                                            HRESULT (*Callback)(VOID *UserData), VOID *UserData)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(Widget);

    if (KeyIndex < 1 || KeyIndex > MAX_FUNCTION_KEYS) return E_INVALIDARG;

    impl->Keys[KeyIndex - 1].Callback = Callback;
    impl->Keys[KeyIndex - 1].UserData = UserData;

    return S_OK;
}

HRESULT ANXAPI AnxTuiFKeyBarSetKeyEnabled(ITuiWidget *Widget, UINT32 KeyIndex, BOOLEAN Enabled)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(Widget);

    if (KeyIndex < 1 || KeyIndex > MAX_FUNCTION_KEYS) return E_INVALIDARG;

    impl->Keys[KeyIndex - 1].Enabled = Enabled;

    return S_OK;
}

HRESULT ANXAPI AnxTuiFKeyBarClearAllKeys(ITuiWidget *Widget)
{
    TuiFKeyBarImpl *impl = FKEYBAR_FROM_WIDGET(Widget);
    UINT32 i;

    for (i = 0; i < MAX_FUNCTION_KEYS; i++) {
        impl->Keys[i].Label[0] = '\0';
        impl->Keys[i].Callback = NULL;
        impl->Keys[i].UserData = NULL;
        impl->Keys[i].Enabled = TRUE;
    }

    return S_OK;
}

/* Helper: Set default keybindings (common pattern) */
HRESULT ANXAPI AnxTuiSetDefaultFKeyBindings(ITuiWidget *Widget)
{
    /* Common defaults like Norton Commander */
    AnxTuiFKeyBarSetKeyLabel(Widget, 1, "Help");
    AnxTuiFKeyBarSetKeyLabel(Widget, 2, "Menu");
    AnxTuiFKeyBarSetKeyLabel(Widget, 3, "View");
    AnxTuiFKeyBarSetKeyLabel(Widget, 4, "Edit");
    AnxTuiFKeyBarSetKeyLabel(Widget, 5, "Copy");
    AnxTuiFKeyBarSetKeyLabel(Widget, 6, "Move");
    AnxTuiFKeyBarSetKeyLabel(Widget, 7, "NewFld");
    AnxTuiFKeyBarSetKeyLabel(Widget, 8, "Delete");
    AnxTuiFKeyBarSetKeyLabel(Widget, 9, "Config");
    AnxTuiFKeyBarSetKeyLabel(Widget, 10, "Quit");

    return S_OK;
}
