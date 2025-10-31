/*
 * Button Widget Implementation (v2 - Refactored Architecture)
 *
 * Uses new event dispatching system with:
 * - ITuiWidget base interface
 * - ITuiKeyListener for keyboard events
 * - ITuiDrawListener for rendering
 * - QueryInterface-based polymorphism
 * - Responder chain support
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

typedef struct _TuiButtonImplV2 TuiButtonImplV2;

struct _TuiButtonImplV2 {
    /* Main interface - ITuiWidget */
    ITuiWidget WidgetInterface;

    /* Backward compatibility - ITuiButton */
    ITuiButton ButtonInterface;

    /* Listener interfaces */
    ITuiKeyListener KeyListener;
    ITuiDrawListener DrawListener;

    /* Implementation data */
    WIDGET_STATE State;
    CHAR8 Label[256];
    HRESULT (*Callback)(VOID *UserData);
    VOID *UserData;
    BOOLEAN Pressed;
    BOOLEAN Focused;

    /* Responder chain */
    ITuiResponder *NextResponder;

    /* Drawing surface */
    ITuiSurface *Surface;
};

/* Forward declarations for interface methods */
static HRESULT ANXAPI ButtonWidget_QueryInterface(ITuiWidget *This, REFIID riid, VOID **ppvObject);
static HRESULT ANXAPI ButtonKey_QueryInterface(ITuiKeyListener *This, REFIID riid, VOID **ppvObject);
static HRESULT ANXAPI ButtonDraw_QueryInterface(ITuiDrawListener *This, REFIID riid, VOID **ppvObject);
static HRESULT ANXAPI ButtonCompat_QueryInterface(ITuiButton *This, REFIID riid, VOID **ppvObject);

/* Helper: Get impl from any interface */
#define BUTTON_FROM_WIDGET(w) ((TuiButtonImplV2*)((UINT8*)(w) - offsetof(TuiButtonImplV2, WidgetInterface)))
#define BUTTON_FROM_BUTTON(b) ((TuiButtonImplV2*)((UINT8*)(b) - offsetof(TuiButtonImplV2, ButtonInterface)))
#define BUTTON_FROM_KEY(k) ((TuiButtonImplV2*)((UINT8*)(k) - offsetof(TuiButtonImplV2, KeyListener)))
#define BUTTON_FROM_DRAW(d) ((TuiButtonImplV2*)((UINT8*)(d) - offsetof(TuiButtonImplV2, DrawListener)))

/*
 * ITuiWidget Implementation
 */

static HRESULT ANXAPI ButtonWidget_QueryInterface(
    ITuiWidget *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);

    if (!ppvObject) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ITuiSerializable) ||
        IsEqualGUID(riid, &IID_ITuiResponder) ||
        IsEqualGUID(riid, &IID_ITuiWidget)) {
        *ppvObject = &impl->WidgetInterface;
        impl->WidgetInterface.Vtbl->AddRef(&impl->WidgetInterface);
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ITuiKeyListener)) {
        *ppvObject = &impl->KeyListener;
        impl->KeyListener.Vtbl->AddRef(&impl->KeyListener);
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ITuiDrawListener)) {
        *ppvObject = &impl->DrawListener;
        impl->DrawListener.Vtbl->AddRef(&impl->DrawListener);
        return S_OK;
    }

    /* Backward compatibility - ITuiButton */
    if (IsEqualGUID(riid, &IID_ITuiButton)) {
        *ppvObject = &impl->ButtonInterface;
        impl->ButtonInterface.Vtbl->AddRef(&impl->ButtonInterface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI ButtonWidget_AddRef(ITuiWidget *This)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ButtonWidget_Release(ITuiWidget *This)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        if (impl->Surface) {
            impl->Surface->Vtbl->Release(impl->Surface);
        }
        if (impl->NextResponder) {
            impl->NextResponder->Vtbl->Release(impl->NextResponder);
        }
        free(impl);
    }

    return count;
}

/* ITuiSerializable methods */
static HRESULT ANXAPI ButtonWidget_SerializeToYaml(
    ITuiWidget *This,
    CHAR8 **OutYaml,
    UINTN *OutLength
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    CHAR8 *yaml = (CHAR8*)malloc(1024);

    if (!yaml) return E_OUTOFMEMORY;

    UINTN len = snprintf(yaml, 1024,
        "type: Button\n"
        "label: \"%s\"\n"
        "bounds:\n"
        "  x: %d\n"
        "  y: %d\n"
        "  width: %d\n"
        "  height: %d\n"
        "visible: %s\n"
        "enabled: %s\n",
        impl->Label,
        impl->State.Bounds.X, impl->State.Bounds.Y,
        impl->State.Bounds.Width, impl->State.Bounds.Height,
        impl->State.Visible ? "true" : "false",
        impl->State.Enabled ? "true" : "false");

    *OutYaml = yaml;
    *OutLength = len;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_DeserializeFromYaml(
    ITuiWidget *This,
    CONST CHAR8 *Yaml,
    UINTN Length
)
{
    /* Simplified YAML parsing - would use proper parser in production */
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_GetTypeName(
    ITuiWidget *This,
    CONST CHAR8 **OutTypeName
)
{
    *OutTypeName = "Button";
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_Clone(
    ITuiWidget *This,
    ITuiSerializable **OutClone
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    ITuiButton *newButton = NULL;

    HRESULT hr = AnxTuiCreateButtonV2(impl->Label, &newButton);
    if (SUCCEEDED(hr)) {
        *OutClone = (ITuiSerializable*)newButton;
    }

    return hr;
}

/* ITuiResponder methods */
static HRESULT ANXAPI ButtonWidget_GetNextResponder(
    ITuiWidget *This,
    ITuiResponder **NextResponder
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    *NextResponder = impl->NextResponder;
    if (impl->NextResponder) {
        impl->NextResponder->Vtbl->AddRef(impl->NextResponder);
    }
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_SetNextResponder(
    ITuiWidget *This,
    ITuiResponder *NextResponder
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);

    if (impl->NextResponder) {
        impl->NextResponder->Vtbl->Release(impl->NextResponder);
    }

    impl->NextResponder = NextResponder;

    if (NextResponder) {
        NextResponder->Vtbl->AddRef(NextResponder);
    }

    return S_OK;
}

static BOOLEAN ANXAPI ButtonWidget_AcceptsFirstResponder(ITuiWidget *This)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    return impl->State.Enabled;
}

static HRESULT ANXAPI ButtonWidget_BecomeFirstResponder(ITuiWidget *This)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    impl->Focused = TRUE;
    impl->State.Focused = TRUE;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_ResignFirstResponder(ITuiWidget *This)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    impl->Focused = FALSE;
    impl->State.Focused = FALSE;
    return S_OK;
}

/* ITuiWidget methods */
static HRESULT ANXAPI ButtonWidget_SetBounds(
    ITuiWidget *This,
    CONST TUI_RECT *Bounds
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_GetBounds(
    ITuiWidget *This,
    TUI_RECT *Bounds
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_SetVisible(ITuiWidget *This, BOOLEAN Visible)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI ButtonWidget_IsVisible(ITuiWidget *This)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    return impl->State.Visible;
}

static HRESULT ANXAPI ButtonWidget_SetEnabled(ITuiWidget *This, BOOLEAN Enabled)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI ButtonWidget_IsEnabled(ITuiWidget *This)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_WIDGET(This);
    return impl->State.Enabled;
}

static HRESULT ANXAPI ButtonWidget_SetParent(ITuiWidget *This, ITuiWidget *Parent)
{
    /* Would implement parent-child relationship */
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_GetParent(ITuiWidget *This, ITuiWidget **Parent)
{
    *Parent = NULL;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_AddChild(ITuiWidget *This, ITuiWidget *Child)
{
    return E_NOTIMPL;  /* Buttons don't have children */
}

static HRESULT ANXAPI ButtonWidget_RemoveChild(ITuiWidget *This, ITuiWidget *Child)
{
    return E_NOTIMPL;
}

static HRESULT ANXAPI ButtonWidget_SetNeedsDisplay(ITuiWidget *This, BOOLEAN Needed)
{
    /* Mark for redraw */
    return S_OK;
}

/* ITuiWidget VTable */
static ITuiWidget_Vtbl ButtonWidgetVtbl = {
    /* ITuiSerializable */
    ButtonWidget_QueryInterface,
    ButtonWidget_AddRef,
    ButtonWidget_Release,
    ButtonWidget_SerializeToYaml,
    ButtonWidget_DeserializeFromYaml,
    ButtonWidget_GetTypeName,
    ButtonWidget_Clone,
    /* ITuiResponder */
    ButtonWidget_GetNextResponder,
    ButtonWidget_SetNextResponder,
    ButtonWidget_AcceptsFirstResponder,
    ButtonWidget_BecomeFirstResponder,
    ButtonWidget_ResignFirstResponder,
    /* ITuiWidget */
    ButtonWidget_SetBounds,
    ButtonWidget_GetBounds,
    ButtonWidget_SetVisible,
    ButtonWidget_IsVisible,
    ButtonWidget_SetEnabled,
    ButtonWidget_IsEnabled,
    ButtonWidget_SetParent,
    ButtonWidget_GetParent,
    ButtonWidget_AddChild,
    ButtonWidget_RemoveChild,
    ButtonWidget_SetNeedsDisplay
};

/*
 * ITuiKeyListener Implementation
 */

static HRESULT ANXAPI ButtonKey_QueryInterface(
    ITuiKeyListener *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_KEY(This);
    return ButtonWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI ButtonKey_AddRef(ITuiKeyListener *This)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_KEY(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ButtonKey_Release(ITuiKeyListener *This)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_KEY(This);
    return ButtonWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI ButtonKey_OnKeyDown(
    ITuiKeyListener *This,
    TUI_KEY Key,
    UINT32 Modifiers,
    BOOLEAN *Handled
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_KEY(This);

    *Handled = FALSE;

    if (!impl->State.Enabled) {
        return S_OK;
    }

    if (Key == TuiKeyEnter || Key == ' ') {
        impl->Pressed = TRUE;

        /* Call callback if registered */
        if (impl->Callback) {
            HRESULT hr = impl->Callback(impl->UserData);
            if (FAILED(hr)) {
                impl->Pressed = FALSE;
                return hr;
            }
        }

        impl->Pressed = FALSE;
        *Handled = TRUE;
        return S_OK;
    }

    return S_OK;
}

static HRESULT ANXAPI ButtonKey_OnKeyUp(
    ITuiKeyListener *This,
    TUI_KEY Key,
    UINT32 Modifiers,
    BOOLEAN *Handled
)
{
    *Handled = FALSE;
    return S_OK;
}

static HRESULT ANXAPI ButtonKey_OnChar(
    ITuiKeyListener *This,
    CHAR16 Character,
    BOOLEAN *Handled
)
{
    *Handled = FALSE;
    return S_OK;
}

/* ITuiKeyListener VTable */
static ITuiKeyListener_Vtbl ButtonKeyVtbl = {
    ButtonKey_QueryInterface,
    ButtonKey_AddRef,
    ButtonKey_Release,
    ButtonKey_OnKeyDown,
    ButtonKey_OnKeyUp,
    ButtonKey_OnChar
};

/*
 * ITuiDrawListener Implementation
 */

static HRESULT ANXAPI ButtonDraw_QueryInterface(
    ITuiDrawListener *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_DRAW(This);
    return ButtonWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI ButtonDraw_AddRef(ITuiDrawListener *This)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_DRAW(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ButtonDraw_Release(ITuiDrawListener *This)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_DRAW(This);
    return ButtonWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI ButtonDraw_OnDraw(
    ITuiDrawListener *This,
    ITuiSurface *Surface,
    CONST TUI_RECT *DirtyRect
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_DRAW(This);
    CHAR8 display[300];
    TUI_COLOR fg, bg;

    if (!impl->State.Visible) return S_OK;

    /* Choose colors */
    if (!impl->State.Enabled) {
        fg = TuiColorBrightBlack;
        bg = TuiColorBlack;
    } else if (impl->Pressed) {
        fg = TuiColorWhite;
        bg = TuiColorBlue;
    } else if (impl->Focused) {
        fg = TuiColorBlack;
        bg = TuiColorYellow;
    } else {
        fg = TuiColorWhite;
        bg = TuiColorBlue;
    }

    /* Format: < Label > */
    snprintf(display, sizeof(display), "< %s >", impl->Label);

    /* Draw to surface */
    Surface->Vtbl->WriteText(Surface, 0, 0, display, fg, bg);

    return S_OK;
}

static HRESULT ANXAPI ButtonDraw_OnGetPreferredSize(
    ITuiDrawListener *This,
    UINT32 *Width,
    UINT32 *Height
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_DRAW(This);

    *Width = strlen(impl->Label) + 4;  /* < Label > */
    *Height = 1;

    return S_OK;
}

/* ITuiDrawListener VTable */
static ITuiDrawListener_Vtbl ButtonDrawVtbl = {
    ButtonDraw_QueryInterface,
    ButtonDraw_AddRef,
    ButtonDraw_Release,
    ButtonDraw_OnDraw,
    ButtonDraw_OnGetPreferredSize
};

/*
 * ITuiButton Implementation (Backward Compatibility)
 */

static HRESULT ANXAPI ButtonCompat_QueryInterface(
    ITuiButton *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_BUTTON(This);
    return ButtonWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI ButtonCompat_AddRef(ITuiButton *This)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_BUTTON(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ButtonCompat_Release(ITuiButton *This)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_BUTTON(This);
    return ButtonWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI ButtonCompat_SetLabel(
    ITuiButton *This,
    CONST CHAR8 *Label
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_BUTTON(This);

    if (!Label) return E_POINTER;

    strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
    impl->Label[sizeof(impl->Label) - 1] = '\0';

    return S_OK;
}

static HRESULT ANXAPI ButtonCompat_SetCallback(
    ITuiButton *This,
    HRESULT (*Callback)(VOID *UserData),
    VOID *UserData
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_BUTTON(This);
    impl->Callback = Callback;
    impl->UserData = UserData;
    return S_OK;
}

static HRESULT ANXAPI ButtonCompat_Render(
    ITuiButton *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    BOOLEAN Focused
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_BUTTON(This);
    CHAR8 display[300];
    TUI_COLOR fg, bg;

    if (!impl->State.Visible) return S_OK;

    impl->Focused = Focused;

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

static HRESULT ANXAPI ButtonCompat_HandleKey(
    ITuiButton *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiButtonImplV2 *impl = BUTTON_FROM_BUTTON(This);
    UINT32 modifiers = 0;
    return ButtonKey_OnKeyDown(&impl->KeyListener, Key, modifiers, Handled);
}

/* ITuiButton VTable (Backward Compatibility) */
static ITuiButton_Vtbl ButtonCompatVtbl = {
    ButtonCompat_QueryInterface,
    ButtonCompat_AddRef,
    ButtonCompat_Release,
    ButtonCompat_SetLabel,
    ButtonCompat_SetCallback,
    ButtonCompat_Render,
    ButtonCompat_HandleKey
};

/*
 * Factory Function
 */

HRESULT ANXAPI AnxTuiCreateButtonV2(
    IN  CONST CHAR8 *Label,
    OUT ITuiButton **Button
)
{
    TuiButtonImplV2 *impl;

    if (!Button) return E_POINTER;

    impl = (TuiButtonImplV2*)calloc(1, sizeof(TuiButtonImplV2));
    if (!impl) {
        *Button = NULL;
        return E_OUTOFMEMORY;
    }

    /* Initialize interfaces */
    impl->WidgetInterface.Vtbl = &ButtonWidgetVtbl;
    impl->ButtonInterface.Vtbl = &ButtonCompatVtbl;
    impl->KeyListener.Vtbl = &ButtonKeyVtbl;
    impl->DrawListener.Vtbl = &ButtonDrawVtbl;

    /* Initialize state */
    InitWidgetState(&impl->State);

    if (Label) {
        strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
        impl->Label[sizeof(impl->Label) - 1] = '\0';
    } else {
        impl->Label[0] = '\0';
    }

    impl->Callback = NULL;
    impl->UserData = NULL;
    impl->Pressed = FALSE;
    impl->Focused = FALSE;
    impl->NextResponder = NULL;
    impl->Surface = NULL;

    /* Return ITuiButton for backward compatibility */
    *Button = &impl->ButtonInterface;

    return S_OK;
}
