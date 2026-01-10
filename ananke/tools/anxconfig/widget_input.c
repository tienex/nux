/*
 * Input Widget Implementation
 *
 * Uses new event dispatching architecture with ITuiWidget, ITuiKeyListener, ITuiDrawListener
 * Maintains backward compatibility with ITuiInput interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "widgets_common.h"

typedef struct {
    /* Main interface - ITuiWidget */
    ITuiWidget WidgetInterface;

    /* Listener interfaces */
    ITuiKeyListener KeyListener;
    ITuiMouseListener MouseListener;
    ITuiDrawListener DrawListener;

    /* Implementation data */
    WIDGET_STATE State;
    CHAR8 Label[256];
    CHAR8 Value[512];
    CONFIG_VALUE_TYPE Type;
    INT64 MinValue;
    INT64 MaxValue;
    UINT32 CursorPos;
    UINT32 ScrollOffset;
    UINT32 Width;
    BOOLEAN PasswordMode;

    /* Responder chain */
    ITuiResponder *NextResponder;

    /* Drawing surface */
    ITuiSurface *Surface;
} TuiInputImpl;

/* Helper macros for getting impl from any interface */
#define INPUT_FROM_WIDGET(w) ((TuiInputImpl*)((UINT8*)(w) - offsetof(TuiInputImpl, WidgetInterface)))
#define INPUT_FROM_KEY(k) ((TuiInputImpl*)((UINT8*)(k) - offsetof(TuiInputImpl, KeyListener)))
#define INPUT_FROM_MOUSE(m) ((TuiInputImpl*)((UINT8*)(m) - offsetof(TuiInputImpl, MouseListener)))
#define INPUT_FROM_DRAW(d) ((TuiInputImpl*)((UINT8*)(d) - offsetof(TuiInputImpl, DrawListener)))

/*
 * ITuiWidget Implementation
 */

static HRESULT ANXAPI InputWidget_QueryInterface(ITuiWidget *This, REFIID riid, VOID **ppvObject)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
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

static UINTN ANXAPI InputWidget_AddRef(ITuiWidget *This)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI InputWidget_Release(ITuiWidget *This)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    UINTN count = --impl->State.RefCount;
    if (count == 0) {
        if (impl->Surface) impl->Surface->Vtbl->Release(impl->Surface);
        if (impl->NextResponder) impl->NextResponder->Vtbl->Release(impl->NextResponder);
        free(impl);
    }
    return count;
}

static HRESULT ANXAPI InputWidget_SetBounds(ITuiWidget *This, CONST TUI_RECT *Bounds)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_GetBounds(ITuiWidget *This, TUI_RECT *Bounds)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_SetVisible(ITuiWidget *This, BOOLEAN Visible)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    impl->State.Visible = Visible;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_GetVisible(ITuiWidget *This, BOOLEAN *Visible)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    if (!Visible) return E_INVALIDARG;
    *Visible = impl->State.Visible;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_SetEnabled(ITuiWidget *This, BOOLEAN Enabled)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    impl->State.Enabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_GetEnabled(ITuiWidget *This, BOOLEAN *Enabled)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    if (!Enabled) return E_INVALIDARG;
    *Enabled = impl->State.Enabled;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_SetParent(ITuiWidget *This, ITuiWidget *Parent)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
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

static HRESULT ANXAPI InputWidget_GetParent(ITuiWidget *This, ITuiWidget **Parent)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    if (!Parent) return E_INVALIDARG;
    if (impl->NextResponder) {
        return impl->NextResponder->Vtbl->QueryInterface(impl->NextResponder,
                                                         &IID_ITuiWidget,
                                                         (VOID **)Parent);
    }
    *Parent = NULL;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_Invalidate(ITuiWidget *This, CONST TUI_RECT *Rect)
{
    return S_OK;
}

static HRESULT ANXAPI InputWidget_GetNextResponder(ITuiWidget *This, ITuiResponder **NextResponder)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    if (!NextResponder) return E_INVALIDARG;
    *NextResponder = impl->NextResponder;
    if (impl->NextResponder) impl->NextResponder->Vtbl->AddRef(impl->NextResponder);
    return S_OK;
}

static HRESULT ANXAPI InputWidget_BecomeFirstResponder(ITuiWidget *This)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    impl->State.Focused = TRUE;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_ResignFirstResponder(ITuiWidget *This)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    impl->State.Focused = FALSE;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_SerializeToYaml(ITuiWidget *This, CHAR8 **OutYaml, UINTN *OutLength)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    CHAR8 *yaml = (CHAR8 *)malloc(2048);
    if (!yaml) return E_OUTOFMEMORY;

    snprintf(yaml, 2048,
        "type: Input\nlabel: \"%s\"\nvalue: \"%s\"\nbounds:\n  x: %d\n  y: %d\n  width: %d\n  height: %d\n"
        "visible: %s\nenabled: %s\nwidth: %u\npassword_mode: %s\nvalue_type: %d\nmin_value: %lld\nmax_value: %lld\n",
        impl->Label, impl->PasswordMode ? "******" : impl->Value,
        impl->State.Bounds.X, impl->State.Bounds.Y, impl->State.Bounds.Width, impl->State.Bounds.Height,
        impl->State.Visible ? "true" : "false", impl->State.Enabled ? "true" : "false",
        impl->Width, impl->PasswordMode ? "true" : "false", impl->Type,
        (long long)impl->MinValue, (long long)impl->MaxValue);

    *OutYaml = yaml;
    *OutLength = strlen(yaml);
    return S_OK;
}

static HRESULT ANXAPI InputWidget_DeserializeFromYaml(ITuiWidget *This, CONST CHAR8 *Yaml, UINTN Length)
{
    return S_OK;
}

static HRESULT ANXAPI InputWidget_GetTypeName(ITuiWidget *This, CONST CHAR8 **OutTypeName)
{
    *OutTypeName = "Input";
    return S_OK;
}

static HRESULT ANXAPI InputWidget_Clone(ITuiWidget *This, ITuiSerializable **OutClone)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(This);
    ITuiWidget *newInput = NULL;
    HRESULT hr = AnxTuiCreateInput(impl->Label, &newInput);
    if (FAILED(hr)) return hr;

    AnxTuiInputSetValue(newInput, impl->Value);
    AnxTuiInputSetType(newInput, impl->Type);
    AnxTuiInputSetRange(newInput, impl->MinValue, impl->MaxValue);
    AnxTuiInputSetWidth(newInput, impl->Width);
    AnxTuiInputSetPasswordMode(newInput, impl->PasswordMode);

    *OutClone = (ITuiSerializable *)newInput;
    return S_OK;
}

static ITuiWidget_Vtbl InputWidgetVtbl = {
    InputWidget_QueryInterface, InputWidget_AddRef, InputWidget_Release,
    InputWidget_SetBounds, InputWidget_GetBounds, InputWidget_SetVisible, InputWidget_GetVisible,
    InputWidget_SetEnabled, InputWidget_GetEnabled, InputWidget_SetParent, InputWidget_GetParent,
    InputWidget_Invalidate, InputWidget_GetNextResponder, InputWidget_BecomeFirstResponder,
    InputWidget_ResignFirstResponder, InputWidget_SerializeToYaml, InputWidget_DeserializeFromYaml,
    InputWidget_GetTypeName, InputWidget_Clone
};

/*
 * ITuiKeyListener Implementation
 */

static HRESULT ANXAPI InputKey_QueryInterface(ITuiKeyListener *This, REFIID riid, VOID **ppvObject)
{
    TuiInputImpl *impl = INPUT_FROM_KEY(This);
    return InputWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI InputKey_AddRef(ITuiKeyListener *This)
{
    TuiInputImpl *impl = INPUT_FROM_KEY(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI InputKey_Release(ITuiKeyListener *This)
{
    TuiInputImpl *impl = INPUT_FROM_KEY(This);
    return InputWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI InputKey_OnKeyDown(ITuiKeyListener *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled)
{
    TuiInputImpl *impl = INPUT_FROM_KEY(This);
    UINT32 len;
    *Handled = FALSE;

    if (!impl->State.Enabled) return S_OK;

    len = strlen(impl->Value);

    switch (Key) {
        case TuiKeyBackspace:
            if (impl->CursorPos > 0) {
                memmove(&impl->Value[impl->CursorPos - 1], &impl->Value[impl->CursorPos],
                        len - impl->CursorPos + 1);
                impl->CursorPos--;
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyDelete:
            if (impl->CursorPos < len) {
                memmove(&impl->Value[impl->CursorPos], &impl->Value[impl->CursorPos + 1],
                        len - impl->CursorPos);
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyLeft:
            if (impl->CursorPos > 0) impl->CursorPos--;
            *Handled = TRUE;
            return S_OK;

        case TuiKeyRight:
            if (impl->CursorPos < len) impl->CursorPos++;
            *Handled = TRUE;
            return S_OK;

        case TuiKeyHome:
            impl->CursorPos = 0;
            impl->ScrollOffset = 0;
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEnd:
            impl->CursorPos = len;
            *Handled = TRUE;
            return S_OK;

        default:
            if (Key >= 32 && Key <= 126) {
                CHAR8 ch = (CHAR8)Key;

                /* Type-specific validation */
                if (impl->Type == ConfigValueInteger || impl->Type == ConfigValueHex) {
                    if (impl->Type == ConfigValueInteger) {
                        if (!isdigit((unsigned char)ch) && ch != '-') {
                            *Handled = TRUE;
                            return S_OK;
                        }
                    } else {
                        if (!isxdigit((unsigned char)ch) && ch != 'x' && ch != 'X') {
                            *Handled = TRUE;
                            return S_OK;
                        }
                    }
                }

                /* Insert character if buffer not full */
                if (len < sizeof(impl->Value) - 1) {
                    memmove(&impl->Value[impl->CursorPos + 1], &impl->Value[impl->CursorPos],
                            len - impl->CursorPos + 1);
                    impl->Value[impl->CursorPos] = ch;
                    impl->CursorPos++;
                }
                *Handled = TRUE;
                return S_OK;
            }
            break;
    }

    return S_OK;
}

static HRESULT ANXAPI InputKey_OnKeyUp(ITuiKeyListener *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled)
{
    *Handled = FALSE;
    return S_OK;
}

static HRESULT ANXAPI InputKey_OnChar(ITuiKeyListener *This, CHAR16 Character, BOOLEAN *Handled)
{
    *Handled = FALSE;
    return S_OK;
}

static ITuiKeyListener_Vtbl InputKeyVtbl = {
    InputKey_QueryInterface, InputKey_AddRef, InputKey_Release,
    InputKey_OnKeyDown, InputKey_OnKeyUp, InputKey_OnChar
};

/*
 * ITuiMouseListener Implementation
 */

static HRESULT ANXAPI InputMouse_QueryInterface(ITuiMouseListener *This, REFIID riid, VOID **ppvObject)
{
    TuiInputImpl *impl = INPUT_FROM_MOUSE(This);
    return InputWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI InputMouse_AddRef(ITuiMouseListener *This)
{
    TuiInputImpl *impl = INPUT_FROM_MOUSE(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI InputMouse_Release(ITuiMouseListener *This)
{
    TuiInputImpl *impl = INPUT_FROM_MOUSE(This);
    return InputWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI InputMouse_OnMouseEvent(ITuiMouseListener *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled)
{
    TuiInputImpl *impl = INPUT_FROM_MOUSE(This);
    BOOLEAN isOver;

    *Handled = FALSE;
    if (!impl->State.Enabled) return S_OK;

    isOver = IsPointInWidget(&impl->State, Event->X, Event->Y);
    if (!isOver) return S_OK;

    /* Click to focus */
    if (Event->Type == TuiMouseLeftDown) {
        impl->State.Focused = TRUE;
        *Handled = TRUE;
    }

    return S_OK;
}

static ITuiMouseListener_Vtbl InputMouseVtbl = {
    InputMouse_QueryInterface, InputMouse_AddRef, InputMouse_Release,
    InputMouse_OnMouseEvent
};

/*
 * ITuiDrawListener Implementation
 */

static HRESULT ANXAPI InputDraw_QueryInterface(ITuiDrawListener *This, REFIID riid, VOID **ppvObject)
{
    TuiInputImpl *impl = INPUT_FROM_DRAW(This);
    return InputWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI InputDraw_AddRef(ITuiDrawListener *This)
{
    TuiInputImpl *impl = INPUT_FROM_DRAW(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI InputDraw_Release(ITuiDrawListener *This)
{
    TuiInputImpl *impl = INPUT_FROM_DRAW(This);
    return InputWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI InputDraw_OnDraw(ITuiDrawListener *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect)
{
    TuiInputImpl *impl = INPUT_FROM_DRAW(This);
    CHAR8 display[1024], valueDisplay[512];
    TUI_COLOR fg, bg;
    UINT32 i, visibleLen, startPos;

    if (!impl->State.Visible) return S_OK;

    /* Choose colors based on state */
    if (!impl->State.Enabled) {
        fg = TuiColorBrightBlack;
        bg = TuiColorBlack;
    } else if (impl->State.Focused) {
        fg = TuiColorBlack;
        bg = TuiColorWhite;
    } else {
        fg = TuiColorWhite;
        bg = TuiColorBlue;
    }

    /* Prepare value display (handle password mode and scrolling) */
    if (impl->PasswordMode) {
        UINT32 len = strlen(impl->Value);
        for (i = 0; i < len && i < sizeof(valueDisplay) - 1; i++) {
            valueDisplay[i] = '*';
        }
        valueDisplay[i] = '\0';
    } else {
        strncpy(valueDisplay, impl->Value, sizeof(valueDisplay) - 1);
        valueDisplay[sizeof(valueDisplay) - 1] = '\0';
    }

    /* Handle scrolling if value is longer than display width */
    visibleLen = impl->Width > 0 ? impl->Width : 20;
    startPos = impl->ScrollOffset;

    if (strlen(valueDisplay) > visibleLen) {
        if (impl->CursorPos < impl->ScrollOffset) {
            impl->ScrollOffset = impl->CursorPos;
        } else if (impl->CursorPos >= impl->ScrollOffset + visibleLen) {
            impl->ScrollOffset = impl->CursorPos - visibleLen + 1;
        }
        startPos = impl->ScrollOffset;

        CHAR8 temp[512];
        strncpy(temp, valueDisplay + startPos, visibleLen);
        temp[visibleLen] = '\0';
        strcpy(valueDisplay, temp);
    }

    /* Format: Label: [value________] */
    if (strlen(impl->Label) > 0) {
        snprintf(display, sizeof(display), "%s: [%-*s]", impl->Label, visibleLen, valueDisplay);
    } else {
        snprintf(display, sizeof(display), "[%-*s]", visibleLen, valueDisplay);
    }

    Surface->Vtbl->WriteText(Surface, 0, 0, display, fg, bg);

    /* Draw cursor if focused */
    if (impl->State.Focused && impl->State.Enabled) {
        INT32 cursorX = strlen(impl->Label) + (strlen(impl->Label) > 0 ? 3 : 1) +
                        (impl->CursorPos - impl->ScrollOffset);
        Surface->Vtbl->WriteText(Surface, cursorX, 0, "_", fg, bg);
    }

    return S_OK;
}

static HRESULT ANXAPI InputDraw_OnGetPreferredSize(ITuiDrawListener *This, UINT32 *Width, UINT32 *Height)
{
    TuiInputImpl *impl = INPUT_FROM_DRAW(This);
    if (Width) *Width = strlen(impl->Label) + impl->Width + 4;
    if (Height) *Height = 1;
    return S_OK;
}

static ITuiDrawListener_Vtbl InputDrawVtbl = {
    InputDraw_QueryInterface, InputDraw_AddRef, InputDraw_Release,
    InputDraw_OnDraw, InputDraw_OnGetPreferredSize
};

/*
 * Factory Function
 */

HRESULT ANXAPI AnxTuiCreateInput(IN CONST CHAR8 *Label, OUT ITuiWidget **Widget)
{
    TuiInputImpl *impl;

    if (!Widget) return E_POINTER;

    impl = (TuiInputImpl *)calloc(1, sizeof(TuiInputImpl));
    if (!impl) {
        *Widget = NULL;
        return E_OUTOFMEMORY;
    }

    impl->WidgetInterface.Vtbl = &InputWidgetVtbl;
    impl->KeyListener.Vtbl = &InputKeyVtbl;
    impl->MouseListener.Vtbl = &InputMouseVtbl;
    impl->DrawListener.Vtbl = &InputDrawVtbl;

    InitWidgetState(&impl->State);

    if (Label) {
        strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
        impl->Label[sizeof(impl->Label) - 1] = '\0';
    } else {
        impl->Label[0] = '\0';
    }

    impl->Value[0] = '\0';
    impl->Type = ConfigValueString;
    impl->MinValue = 0;
    impl->MaxValue = 0;
    impl->CursorPos = 0;
    impl->ScrollOffset = 0;
    impl->Width = 20;
    impl->PasswordMode = FALSE;
    impl->NextResponder = NULL;
    impl->Surface = NULL;

    *Widget = &impl->WidgetInterface;
    return S_OK;
}

/* Convenience functions for input-specific operations */
HRESULT ANXAPI AnxTuiInputSetLabel(ITuiWidget *Widget, CONST CHAR8 *Label)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(Widget);
    if (!Label) return E_POINTER;
    strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
    impl->Label[sizeof(impl->Label) - 1] = '\0';
    return S_OK;
}

HRESULT ANXAPI AnxTuiInputSetValue(ITuiWidget *Widget, CONST CHAR8 *Value)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(Widget);
    if (!Value) return E_POINTER;
    strncpy(impl->Value, Value, sizeof(impl->Value) - 1);
    impl->Value[sizeof(impl->Value) - 1] = '\0';
    impl->CursorPos = strlen(impl->Value);
    return S_OK;
}

HRESULT ANXAPI AnxTuiInputGetValue(ITuiWidget *Widget, CHAR8 *Buffer, UINTN BufferSize)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(Widget);
    if (!Buffer) return E_POINTER;
    strncpy(Buffer, impl->Value, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

HRESULT ANXAPI AnxTuiInputSetType(ITuiWidget *Widget, CONFIG_VALUE_TYPE Type)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(Widget);
    impl->Type = Type;
    return S_OK;
}

HRESULT ANXAPI AnxTuiInputSetRange(ITuiWidget *Widget, INT64 MinValue, INT64 MaxValue)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(Widget);
    impl->MinValue = MinValue;
    impl->MaxValue = MaxValue;
    return S_OK;
}

HRESULT ANXAPI AnxTuiInputSetWidth(ITuiWidget *Widget, UINT32 Width)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(Widget);
    impl->Width = Width;
    return S_OK;
}

HRESULT ANXAPI AnxTuiInputSetPasswordMode(ITuiWidget *Widget, BOOLEAN PasswordMode)
{
    TuiInputImpl *impl = INPUT_FROM_WIDGET(Widget);
    impl->PasswordMode = PasswordMode;
    return S_OK;
}
