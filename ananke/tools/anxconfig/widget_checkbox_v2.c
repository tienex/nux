/*
 * Checkbox Widget V2 Implementation
 *
 * Refactored to use the new event dispatching architecture:
 * - ITuiWidget base interface
 * - ITuiKeyListener for keyboard events
 * - ITuiDrawListener for rendering
 * - QueryInterface for polymorphism
 * - Responder chain support
 * - YAML serialization
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

/* Forward declaration */
typedef struct _TuiCheckboxImplV2 TuiCheckboxImplV2;

struct _TuiCheckboxImplV2 {
    /* Main interface - ITuiWidget */
    ITuiWidget WidgetInterface;

    /* Backward compatibility - ITuiCheckbox */
    ITuiCheckbox CheckboxInterface;

    /* Listener interfaces */
    ITuiKeyListener KeyListener;
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
};

/* Helper macros for getting impl from any interface */
#define CHECKBOX_FROM_WIDGET(w) ((TuiCheckboxImplV2*)((UINT8*)(w) - offsetof(TuiCheckboxImplV2, WidgetInterface)))
#define CHECKBOX_FROM_CHECKBOX(c) ((TuiCheckboxImplV2*)((UINT8*)(c) - offsetof(TuiCheckboxImplV2, CheckboxInterface)))
#define CHECKBOX_FROM_KEY(k) ((TuiCheckboxImplV2*)((UINT8*)(k) - offsetof(TuiCheckboxImplV2, KeyListener)))
#define CHECKBOX_FROM_DRAW(d) ((TuiCheckboxImplV2*)((UINT8*)(d) - offsetof(TuiCheckboxImplV2, DrawListener)))

/*
 * ITuiWidget Implementation
 */

static HRESULT ANXAPI CheckboxWidget_QueryInterface(
    ITuiWidget *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);

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

    if (IsEqualGUID(riid, &IID_ITuiCheckbox)) {
        *ppvObject = &impl->CheckboxInterface;
        impl->State.RefCount++;
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI CheckboxWidget_AddRef(ITuiWidget *This)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI CheckboxWidget_Release(ITuiWidget *This)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
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

static HRESULT ANXAPI CheckboxWidget_SetBounds(
    ITuiWidget *This,
    CONST TUI_RECT *Bounds
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;

    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_GetBounds(
    ITuiWidget *This,
    TUI_RECT *Bounds
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;

    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_SetVisible(
    ITuiWidget *This,
    BOOLEAN Visible
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
    impl->State.Visible = Visible;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_GetVisible(
    ITuiWidget *This,
    BOOLEAN *Visible
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
    if (!Visible) return E_INVALIDARG;

    *Visible = impl->State.Visible;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_SetEnabled(
    ITuiWidget *This,
    BOOLEAN Enabled
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
    impl->State.Enabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_GetEnabled(
    ITuiWidget *This,
    BOOLEAN *Enabled
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
    if (!Enabled) return E_INVALIDARG;

    *Enabled = impl->State.Enabled;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_SetParent(
    ITuiWidget *This,
    ITuiWidget *Parent
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);

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

static HRESULT ANXAPI CheckboxWidget_GetParent(
    ITuiWidget *This,
    ITuiWidget **Parent
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
    if (!Parent) return E_INVALIDARG;

    if (impl->NextResponder) {
        return impl->NextResponder->Vtbl->QueryInterface(impl->NextResponder,
                                                         &IID_ITuiWidget,
                                                         (VOID **)Parent);
    }

    *Parent = NULL;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_Invalidate(
    ITuiWidget *This,
    CONST TUI_RECT *Rect
)
{
    /* Mark widget as needing redraw */
    /* In a full implementation, this would notify the compositor */
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_GetNextResponder(
    ITuiWidget *This,
    ITuiResponder **NextResponder
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
    if (!NextResponder) return E_INVALIDARG;

    *NextResponder = impl->NextResponder;
    if (impl->NextResponder) {
        impl->NextResponder->Vtbl->AddRef(impl->NextResponder);
    }

    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_BecomeFirstResponder(ITuiWidget *This)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
    impl->State.Focused = TRUE;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_ResignFirstResponder(ITuiWidget *This)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
    impl->State.Focused = FALSE;
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_SerializeToYaml(
    ITuiWidget *This,
    CHAR8 **OutYaml,
    UINTN *OutLength
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
    CHAR8 *yaml;

    yaml = (CHAR8 *)malloc(1024);
    if (!yaml) return E_OUTOFMEMORY;

    snprintf(yaml, 1024,
        "type: Checkbox\n"
        "label: \"%s\"\n"
        "checked: %s\n"
        "tristate: %s\n"
        "tristate_value: %u\n"
        "bounds:\n"
        "  x: %d\n"
        "  y: %d\n"
        "  width: %d\n"
        "  height: %d\n"
        "visible: %s\n"
        "enabled: %s\n",
        impl->Label,
        impl->Checked ? "true" : "false",
        impl->Tristate ? "true" : "false",
        impl->TristateValue,
        impl->State.Bounds.X, impl->State.Bounds.Y,
        impl->State.Bounds.Width, impl->State.Bounds.Height,
        impl->State.Visible ? "true" : "false",
        impl->State.Enabled ? "true" : "false");

    *OutYaml = yaml;
    *OutLength = strlen(yaml);
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_DeserializeFromYaml(
    ITuiWidget *This,
    CONST CHAR8 *Yaml,
    UINTN Length
)
{
    /* Simplified YAML parsing - in real implementation would use proper parser */
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_GetTypeName(
    ITuiWidget *This,
    CONST CHAR8 **OutTypeName
)
{
    *OutTypeName = "Checkbox";
    return S_OK;
}

static HRESULT ANXAPI CheckboxWidget_Clone(
    ITuiWidget *This,
    ITuiSerializable **OutClone
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_WIDGET(This);
    ITuiCheckbox *newCheckbox = NULL;
    HRESULT hr;

    hr = AnxTuiCreateCheckboxV2(&newCheckbox);
    if (FAILED(hr)) return hr;

    /* Copy properties */
    newCheckbox->Vtbl->SetLabel(newCheckbox, impl->Label);
    newCheckbox->Vtbl->SetChecked(newCheckbox, impl->Checked);
    newCheckbox->Vtbl->SetTristate(newCheckbox, impl->Tristate);
    newCheckbox->Vtbl->SetTristateValue(newCheckbox, impl->TristateValue);

    *OutClone = (ITuiSerializable *)newCheckbox;
    return S_OK;
}

/* ITuiWidget vtable */
static ITuiWidget_Vtbl CheckboxWidgetVtbl = {
    CheckboxWidget_QueryInterface,
    CheckboxWidget_AddRef,
    CheckboxWidget_Release,
    CheckboxWidget_SetBounds,
    CheckboxWidget_GetBounds,
    CheckboxWidget_SetVisible,
    CheckboxWidget_GetVisible,
    CheckboxWidget_SetEnabled,
    CheckboxWidget_GetEnabled,
    CheckboxWidget_SetParent,
    CheckboxWidget_GetParent,
    CheckboxWidget_Invalidate,
    CheckboxWidget_GetNextResponder,
    CheckboxWidget_BecomeFirstResponder,
    CheckboxWidget_ResignFirstResponder,
    CheckboxWidget_SerializeToYaml,
    CheckboxWidget_DeserializeFromYaml,
    CheckboxWidget_GetTypeName,
    CheckboxWidget_Clone
};

/*
 * ITuiKeyListener Implementation
 */

static HRESULT ANXAPI CheckboxKey_QueryInterface(
    ITuiKeyListener *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_KEY(This);
    return CheckboxWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI CheckboxKey_AddRef(ITuiKeyListener *This)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_KEY(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI CheckboxKey_Release(ITuiKeyListener *This)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_KEY(This);
    return CheckboxWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI CheckboxKey_OnKeyDown(
    ITuiKeyListener *This,
    TUI_KEY Key,
    UINT32 Modifiers,
    BOOLEAN *Handled
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_KEY(This);

    *Handled = FALSE;

    if (!impl->State.Enabled) {
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

    return S_OK;
}

static HRESULT ANXAPI CheckboxKey_OnKeyUp(
    ITuiKeyListener *This,
    TUI_KEY Key,
    UINT32 Modifiers,
    BOOLEAN *Handled
)
{
    *Handled = FALSE;
    return S_OK;
}

static HRESULT ANXAPI CheckboxKey_OnChar(
    ITuiKeyListener *This,
    CHAR16 Character,
    BOOLEAN *Handled
)
{
    *Handled = FALSE;
    return S_OK;
}

/* ITuiKeyListener vtable */
static ITuiKeyListener_Vtbl CheckboxKeyVtbl = {
    CheckboxKey_QueryInterface,
    CheckboxKey_AddRef,
    CheckboxKey_Release,
    CheckboxKey_OnKeyDown,
    CheckboxKey_OnKeyUp,
    CheckboxKey_OnChar
};

/*
 * ITuiDrawListener Implementation
 */

static HRESULT ANXAPI CheckboxDraw_QueryInterface(
    ITuiDrawListener *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_DRAW(This);
    return CheckboxWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI CheckboxDraw_AddRef(ITuiDrawListener *This)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_DRAW(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI CheckboxDraw_Release(ITuiDrawListener *This)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_DRAW(This);
    return CheckboxWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI CheckboxDraw_OnDraw(
    ITuiDrawListener *This,
    ITuiSurface *Surface,
    CONST TUI_RECT *DirtyRect
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_DRAW(This);
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

    /* Format: [X] Label */
    snprintf(display, sizeof(display), "[%c] %s", mark, impl->Label);

    /* Draw to surface */
    Surface->Vtbl->WriteText(Surface, 0, 0, display, fg, bg);

    return S_OK;
}

static HRESULT ANXAPI CheckboxDraw_OnGetPreferredSize(
    ITuiDrawListener *This,
    UINT32 *Width,
    UINT32 *Height
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_DRAW(This);

    if (Width) {
        *Width = 4 + strlen(impl->Label);  /* "[X] " + label */
    }
    if (Height) {
        *Height = 1;
    }

    return S_OK;
}

/* ITuiDrawListener vtable */
static ITuiDrawListener_Vtbl CheckboxDrawVtbl = {
    CheckboxDraw_QueryInterface,
    CheckboxDraw_AddRef,
    CheckboxDraw_Release,
    CheckboxDraw_OnDraw,
    CheckboxDraw_OnGetPreferredSize
};

/*
 * ITuiCheckbox Implementation (backward compatibility)
 */

static HRESULT ANXAPI Checkbox_QueryInterface(
    ITuiCheckbox *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_CHECKBOX(This);
    return CheckboxWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI Checkbox_AddRef(ITuiCheckbox *This)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_CHECKBOX(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Checkbox_Release(ITuiCheckbox *This)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_CHECKBOX(This);
    return CheckboxWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI Checkbox_SetLabel(
    ITuiCheckbox *This,
    CONST CHAR8 *Label
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_CHECKBOX(This);
    if (!Label) return E_POINTER;

    strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
    impl->Label[sizeof(impl->Label) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Checkbox_GetChecked(
    ITuiCheckbox *This,
    BOOLEAN *Checked
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_CHECKBOX(This);
    if (!Checked) return E_POINTER;

    *Checked = impl->Checked;
    return S_OK;
}

static HRESULT ANXAPI Checkbox_SetChecked(
    ITuiCheckbox *This,
    BOOLEAN Checked
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_CHECKBOX(This);
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
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_CHECKBOX(This);
    impl->Tristate = Tristate;
    return S_OK;
}

static HRESULT ANXAPI Checkbox_GetTristateValue(
    ITuiCheckbox *This,
    UINT8 *Value
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_CHECKBOX(This);
    if (!Value) return E_POINTER;

    *Value = impl->TristateValue;
    return S_OK;
}

static HRESULT ANXAPI Checkbox_SetTristateValue(
    ITuiCheckbox *This,
    UINT8 Value
)
{
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_CHECKBOX(This);
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
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_CHECKBOX(This);
    CHAR8 display[300];
    CHAR8 mark;
    TUI_COLOR fg, bg;

    if (!impl->State.Visible) return S_OK;

    impl->State.Focused = Focused;

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
    if (!impl->State.Enabled) {
        fg = TuiColorBrightBlack;
        bg = TuiColorBlack;
    } else if (Focused) {
        fg = TuiColorBlack;
        bg = TuiColorCyan;
    } else {
        fg = impl->State.ForegroundColor;
        bg = impl->State.BackgroundColor;
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
    TuiCheckboxImplV2 *impl = CHECKBOX_FROM_CHECKBOX(This);

    /* Delegate to key listener */
    ITuiKeyListener *keyListener = &impl->KeyListener;
    return keyListener->Vtbl->OnKeyDown(keyListener, Key, 0, Handled);
}

/* ITuiCheckbox vtable (backward compatibility) */
static ITuiCheckbox_Vtbl CheckboxVtbl = {
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

/*
 * Factory Function
 */

HRESULT ANXAPI AnxTuiCreateCheckboxV2(OUT ITuiCheckbox **Checkbox)
{
    TuiCheckboxImplV2 *impl;

    if (!Checkbox) return E_POINTER;

    impl = (TuiCheckboxImplV2 *)calloc(1, sizeof(TuiCheckboxImplV2));
    if (!impl) {
        *Checkbox = NULL;
        return E_OUTOFMEMORY;
    }

    /* Initialize all vtables */
    impl->WidgetInterface.Vtbl = &CheckboxWidgetVtbl;
    impl->CheckboxInterface.Vtbl = &CheckboxVtbl;
    impl->KeyListener.Vtbl = &CheckboxKeyVtbl;
    impl->DrawListener.Vtbl = &CheckboxDrawVtbl;

    /* Initialize state */
    InitWidgetState(&impl->State);
    impl->Label[0] = '\0';
    impl->Checked = FALSE;
    impl->Tristate = FALSE;
    impl->TristateValue = 0;
    impl->NextResponder = NULL;
    impl->Surface = NULL;

    *Checkbox = &impl->CheckboxInterface;
    return S_OK;
}
