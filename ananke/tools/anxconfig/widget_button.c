/*
 * Button Widget Implementation
 *
 * Uses new event dispatching architecture with ITuiWidget, ITuiKeyListener, ITuiDrawListener
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

typedef struct {
    ITuiWidget WidgetInterface;
    ITuiKeyListener KeyListener;
    ITuiMouseListener MouseListener;
    ITuiDrawListener DrawListener;

    /* Implementation data */
    WIDGET_STATE State;
    CHAR8 Label[256];
    HRESULT (*Callback)(VOID *UserData);
    VOID *UserData;
    BOOLEAN Pressed;

    /* Responder chain */
    ITuiResponder *NextResponder;

    /* Drawing surface */
    ITuiSurface *Surface;
} TuiButtonImpl;

/* Helper macros for getting impl from any interface */
#define BUTTON_FROM_WIDGET(w) ((TuiButtonImpl*)((UINT8*)(w) - offsetof(TuiButtonImpl, WidgetInterface)))
#define BUTTON_FROM_KEY(k) ((TuiButtonImpl*)((UINT8*)(k) - offsetof(TuiButtonImpl, KeyListener)))
#define BUTTON_FROM_MOUSE(m) ((TuiButtonImpl*)((UINT8*)(m) - offsetof(TuiButtonImpl, MouseListener)))
#define BUTTON_FROM_DRAW(d) ((TuiButtonImpl*)((UINT8*)(d) - offsetof(TuiButtonImpl, DrawListener)))

/*
 * ITuiWidget Implementation
 */

static HRESULT ANXAPI ButtonWidget_QueryInterface(
    ITuiWidget *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);

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

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI ButtonWidget_AddRef(ITuiWidget *This)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ButtonWidget_Release(ITuiWidget *This)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
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

static HRESULT ANXAPI ButtonWidget_SetBounds(
    ITuiWidget *This,
    CONST TUI_RECT *Bounds
)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;

    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_GetBounds(
    ITuiWidget *This,
    TUI_RECT *Bounds
)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;

    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_SetVisible(
    ITuiWidget *This,
    BOOLEAN Visible
)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
    impl->State.Visible = Visible;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_GetVisible(
    ITuiWidget *This,
    BOOLEAN *Visible
)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
    if (!Visible) return E_INVALIDARG;

    *Visible = impl->State.Visible;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_SetEnabled(
    ITuiWidget *This,
    BOOLEAN Enabled
)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
    impl->State.Enabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_GetEnabled(
    ITuiWidget *This,
    BOOLEAN *Enabled
)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
    if (!Enabled) return E_INVALIDARG;

    *Enabled = impl->State.Enabled;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_SetParent(
    ITuiWidget *This,
    ITuiWidget *Parent
)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);

    /* Release old parent */
    if (impl->NextResponder) {
        impl->NextResponder->Vtbl->Release(impl->NextResponder);
        impl->NextResponder = NULL;
    }

    /* Set new parent as next responder */
    if (Parent) {
        ITuiResponder *parentResponder = NULL;
        HRESULT hr = Parent->Vtbl->QueryInterface((ITuiWidget *)Parent, &IID_ITuiResponder,
                                                   (VOID **)&parentResponder);
        if (SUCCEEDED(hr)) {
            impl->NextResponder = parentResponder;
        }
    }

    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_GetParent(
    ITuiWidget *This,
    ITuiWidget **Parent
)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
    if (!Parent) return E_INVALIDARG;

    if (impl->NextResponder) {
        return impl->NextResponder->Vtbl->QueryInterface(impl->NextResponder,
                                                         &IID_ITuiWidget,
                                                         (VOID **)Parent);
    }

    *Parent = NULL;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_Invalidate(
    ITuiWidget *This,
    CONST TUI_RECT *Rect
)
{
    /* Mark widget as needing redraw */
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_GetNextResponder(
    ITuiWidget *This,
    ITuiResponder **NextResponder
)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
    if (!NextResponder) return E_INVALIDARG;

    *NextResponder = impl->NextResponder;
    if (impl->NextResponder) {
        impl->NextResponder->Vtbl->AddRef(impl->NextResponder);
    }

    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_BecomeFirstResponder(ITuiWidget *This)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
    impl->State.Focused = TRUE;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_ResignFirstResponder(ITuiWidget *This)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
    impl->State.Focused = FALSE;
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_SerializeToYaml(
    ITuiWidget *This,
    CHAR8 **OutYaml,
    UINTN *OutLength
)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
    CHAR8 *yaml;

    yaml = (CHAR8 *)malloc(1024);
    if (!yaml) return E_OUTOFMEMORY;

    snprintf(yaml, 1024,
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
    *OutLength = strlen(yaml);
    return S_OK;
}

static HRESULT ANXAPI ButtonWidget_DeserializeFromYaml(
    ITuiWidget *This,
    CONST CHAR8 *Yaml,
    UINTN Length
)
{
    /* Simplified - would use proper YAML parser */
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
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(This);
    ITuiWidget *newButton = NULL;
    HRESULT hr;

    hr = AnxTuiCreateButton(impl->Label, &newButton);
    if (FAILED(hr)) return hr;

    *OutClone = (ITuiSerializable *)newButton;
    return S_OK;
}

/* ITuiWidget vtable */
static ITuiWidget_Vtbl ButtonWidgetVtbl = {
    ButtonWidget_QueryInterface,
    ButtonWidget_AddRef,
    ButtonWidget_Release,
    ButtonWidget_SetBounds,
    ButtonWidget_GetBounds,
    ButtonWidget_SetVisible,
    ButtonWidget_GetVisible,
    ButtonWidget_SetEnabled,
    ButtonWidget_GetEnabled,
    ButtonWidget_SetParent,
    ButtonWidget_GetParent,
    ButtonWidget_Invalidate,
    ButtonWidget_GetNextResponder,
    ButtonWidget_BecomeFirstResponder,
    ButtonWidget_ResignFirstResponder,
    ButtonWidget_SerializeToYaml,
    ButtonWidget_DeserializeFromYaml,
    ButtonWidget_GetTypeName,
    ButtonWidget_Clone
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
    TuiButtonImpl *impl = BUTTON_FROM_KEY(This);
    return ButtonWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI ButtonKey_AddRef(ITuiKeyListener *This)
{
    TuiButtonImpl *impl = BUTTON_FROM_KEY(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ButtonKey_Release(ITuiKeyListener *This)
{
    TuiButtonImpl *impl = BUTTON_FROM_KEY(This);
    return ButtonWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI ButtonKey_OnKeyDown(
    ITuiKeyListener *This,
    TUI_KEY Key,
    UINT32 Modifiers,
    BOOLEAN *Handled
)
{
    TuiButtonImpl *impl = BUTTON_FROM_KEY(This);
    HRESULT hr;

    *Handled = FALSE;

    if (!impl->State.Enabled) {
        return S_OK;
    }

    if (Key == TuiKeyEnter || Key == ' ') {
        impl->Pressed = TRUE;

        /* Call callback if registered */
        if (impl->Callback) {
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

/* ITuiKeyListener vtable */
static ITuiKeyListener_Vtbl ButtonKeyVtbl = {
    ButtonKey_QueryInterface,
    ButtonKey_AddRef,
    ButtonKey_Release,
    ButtonKey_OnKeyDown,
    ButtonKey_OnKeyUp,
    ButtonKey_OnChar
};

/*
 * ITuiMouseListener Implementation
 */

static HRESULT ANXAPI ButtonMouse_QueryInterface(
    ITuiMouseListener *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiButtonImpl *impl = BUTTON_FROM_MOUSE(This);
    return ButtonWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI ButtonMouse_AddRef(ITuiMouseListener *This)
{
    TuiButtonImpl *impl = BUTTON_FROM_MOUSE(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ButtonMouse_Release(ITuiMouseListener *This)
{
    TuiButtonImpl *impl = BUTTON_FROM_MOUSE(This);
    return ButtonWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI ButtonMouse_OnMouseEvent(
    ITuiMouseListener *This,
    CONST TUI_MOUSE_EVENT *Event,
    BOOLEAN *Handled
)
{
    TuiButtonImpl *impl = BUTTON_FROM_MOUSE(This);
    HRESULT hr;

    *Handled = FALSE;

    if (!impl->State.Enabled) return S_OK;

    /* Check if mouse is over button */
    BOOLEAN isOver = IsPointInWidget(&impl->State, Event->X, Event->Y);

    if (!isOver) {
        impl->Pressed = FALSE;
        return S_OK;
    }

    if (Event->Type == TuiMouseLeftDown) {
        impl->Pressed = TRUE;
        *Handled = TRUE;
        return S_OK;
    }

    if (Event->Type == TuiMouseLeftUp && impl->Pressed) {
        impl->Pressed = FALSE;

        /* Call callback if registered */
        if (impl->Callback) {
            hr = impl->Callback(impl->UserData);
            if (FAILED(hr)) {
                return hr;
            }
        }

        *Handled = TRUE;
        return S_OK;
    }

    return S_OK;
}

/* ITuiMouseListener vtable */
static ITuiMouseListener_Vtbl ButtonMouseVtbl = {
    ButtonMouse_QueryInterface,
    ButtonMouse_AddRef,
    ButtonMouse_Release,
    ButtonMouse_OnMouseEvent
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
    TuiButtonImpl *impl = BUTTON_FROM_DRAW(This);
    return ButtonWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI ButtonDraw_AddRef(ITuiDrawListener *This)
{
    TuiButtonImpl *impl = BUTTON_FROM_DRAW(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ButtonDraw_Release(ITuiDrawListener *This)
{
    TuiButtonImpl *impl = BUTTON_FROM_DRAW(This);
    return ButtonWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI ButtonDraw_OnDraw(
    ITuiDrawListener *This,
    ITuiSurface *Surface,
    CONST TUI_RECT *DirtyRect
)
{
    TuiButtonImpl *impl = BUTTON_FROM_DRAW(This);
    CHAR8 display[300];
    TUI_COLOR fg, bg;

    if (!impl->State.Visible) return S_OK;

    /* Choose colors based on state */
    if (!impl->State.Enabled) {
        fg = TuiColorBrightBlack;
        bg = TuiColorBlack;
    } else if (impl->Pressed) {
        fg = TuiColorWhite;
        bg = TuiColorBlue;
    } else if (impl->State.Focused) {
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
    TuiButtonImpl *impl = BUTTON_FROM_DRAW(This);

    if (Width) {
        *Width = strlen(impl->Label) + 4;  /* < Label > */
    }
    if (Height) {
        *Height = 1;
    }

    return S_OK;
}

/* ITuiDrawListener vtable */
static ITuiDrawListener_Vtbl ButtonDrawVtbl = {
    ButtonDraw_QueryInterface,
    ButtonDraw_AddRef,
    ButtonDraw_Release,
    ButtonDraw_OnDraw,
    ButtonDraw_OnGetPreferredSize
};

/*
 * Factory Function
 */

HRESULT ANXAPI AnxTuiCreateButton(
    IN  CONST CHAR8 *Label,
    OUT ITuiWidget **Widget
)
{
    TuiButtonImpl *impl;

    if (!Widget) return E_POINTER;

    impl = (TuiButtonImpl *)calloc(1, sizeof(TuiButtonImpl));
    if (!impl) {
        *Widget = NULL;
        return E_OUTOFMEMORY;
    }

    /* Initialize all vtables */
    impl->WidgetInterface.Vtbl = &ButtonWidgetVtbl;
    impl->KeyListener.Vtbl = &ButtonKeyVtbl;
    impl->MouseListener.Vtbl = &ButtonMouseVtbl;
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
    impl->NextResponder = NULL;
    impl->Surface = NULL;

    *Widget = &impl->WidgetInterface;
    return S_OK;
}

/* Convenience methods for button-specific functionality */
HRESULT ANXAPI AnxTuiButtonSetLabel(ITuiWidget *Widget, CONST CHAR8 *Label)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(Widget);
    if (!Label) return E_POINTER;

    strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
    impl->Label[sizeof(impl->Label) - 1] = '\0';
    return S_OK;
}

HRESULT ANXAPI AnxTuiButtonSetCallback(ITuiWidget *Widget, HRESULT (*Callback)(VOID *UserData), VOID *UserData)
{
    TuiButtonImpl *impl = BUTTON_FROM_WIDGET(Widget);
    impl->Callback = Callback;
    impl->UserData = UserData;
    return S_OK;
}
