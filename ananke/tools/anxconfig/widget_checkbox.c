/*
 * Checkbox Widget Implementation
 *
 * Uses new event dispatching architecture with ITuiWidget, ITuiKeyListener, ITuiMouseListener, ITuiDrawListener
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
    BOOLEAN Checked;
    BOOLEAN Tristate;
    UINT8 TristateValue;  /* 0=N, 1=M, 2=Y */

    /* Responder chain */
    ITuiResponder *NextResponder;

    /* Drawing surface */
    ITuiSurface *Surface;
} TuiCheckboxImpl;

/* Helper macros for getting impl from any interface */
#define CHECKBOX_FROM_WIDGET(w) ((TuiCheckboxImpl*)((UINT8*)(w) - offsetof(TuiCheckboxImpl, WidgetInterface)))
#define CHECKBOX_FROM_KEY(k) ((TuiCheckboxImpl*)((UINT8*)(k) - offsetof(TuiCheckboxImpl, KeyListener)))
#define CHECKBOX_FROM_MOUSE(m) ((TuiCheckboxImpl*)((UINT8*)(m) - offsetof(TuiCheckboxImpl, MouseListener)))
#define CHECKBOX_FROM_DRAW(d) ((TuiCheckboxImpl*)((UINT8*)(d) - offsetof(TuiCheckboxImpl, DrawListener)))

/* Helper: Toggle checkbox state */
static VOID ToggleCheckbox(TuiCheckboxImpl *impl)
{
    if (impl->Tristate) {
        impl->TristateValue = (impl->TristateValue + 1) % 3;
        impl->Checked = (impl->TristateValue != 0);
    } else {
        impl->Checked = !impl->Checked;
        impl->TristateValue = impl->Checked ? 2 : 0;
    }
}

/*
 * ITuiWidget Implementation
 */

static HRESULT ANXAPI CheckboxWidget_QueryInterface(
    ITuiWidget *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);

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

static UINTN ANXAPI CheckboxWidget_AddRef(ITuiWidget *This)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI CheckboxWidget_Release(ITuiWidget *This)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
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

static HRESULT ANXAPI CheckboxWidget_SetBounds(ITuiWidget *This, CONST TUI_RECT *Bounds)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_GetBounds(ITuiWidget *This, TUI_RECT *Bounds)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_SetVisible(ITuiWidget *This, BOOLEAN Visible)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
    impl->State.Visible = Visible;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_GetVisible(ITuiWidget *This, BOOLEAN *Visible)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
    if (!Visible) return E_INVALIDARG;
    *Visible = impl->State.Visible;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_SetEnabled(ITuiWidget *This, BOOLEAN Enabled)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
    impl->State.Enabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_GetEnabled(ITuiWidget *This, BOOLEAN *Enabled)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
    if (!Enabled) return E_INVALIDARG;
    *Enabled = impl->State.Enabled;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_SetParent(ITuiWidget *This, ITuiWidget *Parent)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);

    if (impl->NextResponder) {
        impl->NextResponder->Vtbl->Release(impl->NextResponder);
        impl->NextResponder = NULL;
    }

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

static HRESULT ANXAPI CheckboxWidget_GetParent(ITuiWidget *This, ITuiWidget **Parent)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
    if (!Parent) return E_INVALIDARG;

    if (impl->NextResponder) {
        return impl->NextResponder->Vtbl->QueryInterface(impl->NextResponder,
                                                         &IID_ITuiWidget,
                                                         (VOID **)Parent);
    }

    *Parent = NULL;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_Invalidate(ITuiWidget *This, CONST TUI_RECT *Rect)
{
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_GetNextResponder(ITuiWidget *This, ITuiResponder **NextResponder)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
    if (!NextResponder) return E_INVALIDARG;

    *NextResponder = impl->NextResponder;
    if (impl->NextResponder) {
        impl->NextResponder->Vtbl->AddRef(impl->NextResponder);
    }

    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_BecomeFirstResponder(ITuiWidget *This)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
    impl->State.Focused = TRUE;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_ResignFirstResponder(ITuiWidget *This)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
    impl->State.Focused = FALSE;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_SerializeToYaml(ITuiWidget *This, CHAR8 **OutYaml, UINTN *OutLength)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
    CHAR8 *yaml = (CHAR8 *)malloc(1024);
    if (!yaml) return E_OUTOFMEMORY;

    snprintf(yaml, 1024,
        "type: Checkbox\nlabel: \"%s\"\nchecked: %s\ntristate: %s\ntristate_value: %u\n"
        "bounds:\n  x: %d\n  y: %d\n  width: %d\n  height: %d\nvisible: %s\nenabled: %s\n",
        impl->Label, impl->Checked ? "true" : "false", impl->Tristate ? "true" : "false",
        impl->TristateValue, impl->State.Bounds.X, impl->State.Bounds.Y,
        impl->State.Bounds.Width, impl->State.Bounds.Height,
        impl->State.Visible ? "true" : "false", impl->State.Enabled ? "true" : "false");

    *OutYaml = yaml;
    *OutLength = strlen(yaml);
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_DeserializeFromYaml(ITuiWidget *This, CONST CHAR8 *Yaml, UINTN Length)
{
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_GetTypeName(ITuiWidget *This, CONST CHAR8 **OutTypeName)
{
    *OutTypeName = "Checkbox";
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_Clone(ITuiWidget *This, ITuiSerializable **OutClone)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(This);
    ITuiWidget *newCheckbox = NULL;
    HRESULT hr = AnxTuiCreateCheckbox(&newCheckbox);
    if (FAILED(hr)) return hr;

    AnxTuiCheckboxSetLabel(newCheckbox, impl->Label);
    AnxTuiCheckboxSetChecked(newCheckbox, impl->Checked);
    AnxTuiCheckboxSetTristate(newCheckbox, impl->Tristate);
    AnxTuiCheckboxSetTristateValue(newCheckbox, impl->TristateValue);

    *OutClone = (ITuiSerializable *)newCheckbox;
    return S_OK;
}

static ITuiWidget_Vtbl CheckboxWidgetVtbl = {
    CheckboxWidget_QueryInterface, CheckboxWidget_AddRef, CheckboxWidget_Release,
    CheckboxWidget_SetBounds, CheckboxWidget_GetBounds, CheckboxWidget_SetVisible, CheckboxWidget_GetVisible,
    CheckboxWidget_SetEnabled, CheckboxWidget_GetEnabled, CheckboxWidget_SetParent, CheckboxWidget_GetParent,
    CheckboxWidget_Invalidate, CheckboxWidget_GetNextResponder, CheckboxWidget_BecomeFirstResponder,
    CheckboxWidget_ResignFirstResponder, CheckboxWidget_SerializeToYaml, CheckboxWidget_DeserializeFromYaml,
    CheckboxWidget_GetTypeName, CheckboxWidget_Clone
};

/*
 * ITuiKeyListener Implementation
 */

static HRESULT ANXAPI CheckboxKey_QueryInterface(ITuiKeyListener *This, REFIID riid, VOID **ppvObject)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_KEY(This);
    return CheckboxWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI CheckboxKey_AddRef(ITuiKeyListener *This)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_KEY(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI CheckboxKey_Release(ITuiKeyListener *This)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_KEY(This);
    return CheckboxWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI CheckboxKey_OnKeyDown(ITuiKeyListener *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_KEY(This);
    *Handled = FALSE;

    if (!impl->State.Enabled) return S_OK;

    if (Key == TuiKeyEnter || Key == ' ') {
        ToggleCheckbox(impl);
        *Handled = TRUE;
        return S_OK;
    }

    return S_OK;
}

static HRESULT ANXAPI CheckboxKey_OnKeyUp(ITuiKeyListener *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled)
{
    *Handled = FALSE;
    return S_OK;
}

static HRESULT ANXAPI CheckboxKey_OnChar(ITuiKeyListener *This, CHAR16 Character, BOOLEAN *Handled)
{
    *Handled = FALSE;
    return S_OK;
}

static ITuiKeyListener_Vtbl CheckboxKeyVtbl = {
    CheckboxKey_QueryInterface, CheckboxKey_AddRef, CheckboxKey_Release,
    CheckboxKey_OnKeyDown, CheckboxKey_OnKeyUp, CheckboxKey_OnChar
};

/*
 * ITuiMouseListener Implementation
 */

static HRESULT ANXAPI CheckboxMouse_QueryInterface(ITuiMouseListener *This, REFIID riid, VOID **ppvObject)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_MOUSE(This);
    return CheckboxWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI CheckboxMouse_AddRef(ITuiMouseListener *This)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_MOUSE(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI CheckboxMouse_Release(ITuiMouseListener *This)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_MOUSE(This);
    return CheckboxWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI CheckboxMouse_OnMouseEvent(ITuiMouseListener *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_MOUSE(This);

    *Handled = FALSE;

    if (!impl->State.Enabled) return S_OK;

    /* Check if mouse is over checkbox */
    BOOLEAN isOver = IsPointInWidget(&impl->State, Event->X, Event->Y);

    if (!isOver) return S_OK;

    if (Event->Type == TuiMouseLeftDown) {
        ToggleCheckbox(impl);
        *Handled = TRUE;
        return S_OK;
    }

    return S_OK;
}

static ITuiMouseListener_Vtbl CheckboxMouseVtbl = {
    CheckboxMouse_QueryInterface, CheckboxMouse_AddRef, CheckboxMouse_Release,
    CheckboxMouse_OnMouseEvent
};

/*
 * ITuiDrawListener Implementation
 */

static HRESULT ANXAPI CheckboxDraw_QueryInterface(ITuiDrawListener *This, REFIID riid, VOID **ppvObject)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_DRAW(This);
    return CheckboxWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI CheckboxDraw_AddRef(ITuiDrawListener *This)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_DRAW(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI CheckboxDraw_Release(ITuiDrawListener *This)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_DRAW(This);
    return CheckboxWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI CheckboxDraw_OnDraw(ITuiDrawListener *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_DRAW(This);
    CHAR8 display[300], mark;
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

    /* Choose colors based on state */
    if (!impl->State.Enabled) {
        fg = TuiColorBrightBlack;
        bg = TuiColorBlack;
    } else if (impl->State.Focused) {
        fg = TuiColorBlack;
        bg = TuiColorCyan;
    } else {
        fg = impl->State.ForegroundColor;
        bg = impl->State.BackgroundColor;
    }

    snprintf(display, sizeof(display), "[%c] %s", mark, impl->Label);
    Surface->Vtbl->WriteText(Surface, 0, 0, display, fg, bg);

    return S_OK;
}

static HRESULT ANXAPI CheckboxDraw_OnGetPreferredSize(ITuiDrawListener *This, UINT32 *Width, UINT32 *Height)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_DRAW(This);

    if (Width) *Width = 4 + strlen(impl->Label);
    if (Height) *Height = 1;

    return S_OK;
}

static ITuiDrawListener_Vtbl CheckboxDrawVtbl = {
    CheckboxDraw_QueryInterface, CheckboxDraw_AddRef, CheckboxDraw_Release,
    CheckboxDraw_OnDraw, CheckboxDraw_OnGetPreferredSize
};

/*
 * Factory Function
 */

HRESULT ANXAPI AnxTuiCreateCheckbox(OUT ITuiWidget **Widget)
{
    TuiCheckboxImpl *impl;

    if (!Widget) return E_POINTER;

    impl = (TuiCheckboxImpl *)calloc(1, sizeof(TuiCheckboxImpl));
    if (!impl) {
        *Widget = NULL;
        return E_OUTOFMEMORY;
    }

    impl->WidgetInterface.Vtbl = &CheckboxWidgetVtbl;
    impl->KeyListener.Vtbl = &CheckboxKeyVtbl;
    impl->MouseListener.Vtbl = &CheckboxMouseVtbl;
    impl->DrawListener.Vtbl = &CheckboxDrawVtbl;

    InitWidgetState(&impl->State);
    impl->Label[0] = '\0';
    impl->Checked = FALSE;
    impl->Tristate = FALSE;
    impl->TristateValue = 0;
    impl->NextResponder = NULL;
    impl->Surface = NULL;

    *Widget = &impl->WidgetInterface;
    return S_OK;
}

/* Convenience methods for checkbox-specific functionality */
HRESULT ANXAPI AnxTuiCheckboxSetLabel(ITuiWidget *Widget, CONST CHAR8 *Label)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(Widget);
    if (!Label) return E_POINTER;

    strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
    impl->Label[sizeof(impl->Label) - 1] = '\0';
    return S_OK;
}

HRESULT ANXAPI AnxTuiCheckboxGetChecked(ITuiWidget *Widget, BOOLEAN *Checked)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(Widget);
    if (!Checked) return E_POINTER;

    *Checked = impl->Checked;
    return S_OK;
}

HRESULT ANXAPI AnxTuiCheckboxSetChecked(ITuiWidget *Widget, BOOLEAN Checked)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(Widget);
    impl->Checked = Checked;
    if (!impl->Tristate) {
        impl->TristateValue = Checked ? 2 : 0;
    }
    return S_OK;
}

HRESULT ANXAPI AnxTuiCheckboxSetTristate(ITuiWidget *Widget, BOOLEAN Tristate)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(Widget);
    impl->Tristate = Tristate;
    return S_OK;
}

HRESULT ANXAPI AnxTuiCheckboxGetTristateValue(ITuiWidget *Widget, UINT8 *Value)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(Widget);
    if (!Value) return E_POINTER;

    *Value = impl->TristateValue;
    return S_OK;
}

HRESULT ANXAPI AnxTuiCheckboxSetTristateValue(ITuiWidget *Widget, UINT8 Value)
{
    TuiCheckboxImpl *impl = CHECKBOX_FROM_WIDGET(Widget);
    if (Value > 2) return E_INVALIDARG;

    impl->TristateValue = Value;
    impl->Checked = (Value != 0);
    return S_OK;
}
