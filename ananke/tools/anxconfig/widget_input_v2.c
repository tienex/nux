/*
 * Input Widget V2 Implementation
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
#include <ctype.h>
#include "widgets_common.h"

/* Forward declaration */
typedef struct _TuiInputImplV2 TuiInputImplV2;

struct _TuiInputImplV2 {
    /* Main interface - ITuiWidget */
    ITuiWidget WidgetInterface;

    /* Backward compatibility - ITuiInput */
    ITuiInput InputInterface;

    /* Listener interfaces */
    ITuiKeyListener KeyListener;
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
};

/* Helper macros for getting impl from any interface */
#define INPUT_FROM_WIDGET(w) ((TuiInputImplV2*)((UINT8*)(w) - offsetof(TuiInputImplV2, WidgetInterface)))
#define INPUT_FROM_INPUT(i) ((TuiInputImplV2*)((UINT8*)(i) - offsetof(TuiInputImplV2, InputInterface)))
#define INPUT_FROM_KEY(k) ((TuiInputImplV2*)((UINT8*)(k) - offsetof(TuiInputImplV2, KeyListener)))
#define INPUT_FROM_DRAW(d) ((TuiInputImplV2*)((UINT8*)(d) - offsetof(TuiInputImplV2, DrawListener)))

/*
 * ITuiWidget Implementation
 */

static HRESULT ANXAPI InputWidget_QueryInterface(
    ITuiWidget *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);

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

    if (IsEqualGUID(riid, &IID_ITuiInput)) {
        *ppvObject = &impl->InputInterface;
        impl->State.RefCount++;
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI InputWidget_AddRef(ITuiWidget *This)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI InputWidget_Release(ITuiWidget *This)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
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

static HRESULT ANXAPI InputWidget_SetBounds(
    ITuiWidget *This,
    CONST TUI_RECT *Bounds
)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;

    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_GetBounds(
    ITuiWidget *This,
    TUI_RECT *Bounds
)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;

    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_SetVisible(
    ITuiWidget *This,
    BOOLEAN Visible
)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
    impl->State.Visible = Visible;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_GetVisible(
    ITuiWidget *This,
    BOOLEAN *Visible
)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
    if (!Visible) return E_INVALIDARG;

    *Visible = impl->State.Visible;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_SetEnabled(
    ITuiWidget *This,
    BOOLEAN Enabled
)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
    impl->State.Enabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_GetEnabled(
    ITuiWidget *This,
    BOOLEAN *Enabled
)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
    if (!Enabled) return E_INVALIDARG;

    *Enabled = impl->State.Enabled;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_SetParent(
    ITuiWidget *This,
    ITuiWidget *Parent
)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);

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

static HRESULT ANXAPI InputWidget_GetParent(
    ITuiWidget *This,
    ITuiWidget **Parent
)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
    if (!Parent) return E_INVALIDARG;

    if (impl->NextResponder) {
        return impl->NextResponder->Vtbl->QueryInterface(impl->NextResponder,
                                                         &IID_ITuiWidget,
                                                         (VOID **)Parent);
    }

    *Parent = NULL;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_Invalidate(
    ITuiWidget *This,
    CONST TUI_RECT *Rect
)
{
    /* Mark widget as needing redraw */
    /* In a full implementation, this would notify the compositor */
    return S_OK;
}

static HRESULT ANXAPI InputWidget_GetNextResponder(
    ITuiWidget *This,
    ITuiResponder **NextResponder
)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
    if (!NextResponder) return E_INVALIDARG;

    *NextResponder = impl->NextResponder;
    if (impl->NextResponder) {
        impl->NextResponder->Vtbl->AddRef(impl->NextResponder);
    }

    return S_OK;
}

static HRESULT ANXAPI InputWidget_BecomeFirstResponder(ITuiWidget *This)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
    impl->State.Focused = TRUE;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_ResignFirstResponder(ITuiWidget *This)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
    impl->State.Focused = FALSE;
    return S_OK;
}

static HRESULT ANXAPI InputWidget_SerializeToYaml(
    ITuiWidget *This,
    CHAR8 **OutYaml,
    UINTN *OutLength
)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
    CHAR8 *yaml;

    yaml = (CHAR8 *)malloc(2048);
    if (!yaml) return E_OUTOFMEMORY;

    snprintf(yaml, 2048,
        "type: Input\n"
        "label: \"%s\"\n"
        "value: \"%s\"\n"
        "bounds:\n"
        "  x: %d\n"
        "  y: %d\n"
        "  width: %d\n"
        "  height: %d\n"
        "visible: %s\n"
        "enabled: %s\n"
        "width: %u\n"
        "password_mode: %s\n"
        "value_type: %d\n"
        "min_value: %lld\n"
        "max_value: %lld\n",
        impl->Label,
        impl->PasswordMode ? "******" : impl->Value,
        impl->State.Bounds.X, impl->State.Bounds.Y,
        impl->State.Bounds.Width, impl->State.Bounds.Height,
        impl->State.Visible ? "true" : "false",
        impl->State.Enabled ? "true" : "false",
        impl->Width,
        impl->PasswordMode ? "true" : "false",
        impl->Type,
        (long long)impl->MinValue,
        (long long)impl->MaxValue);

    *OutYaml = yaml;
    *OutLength = strlen(yaml);
    return S_OK;
}

static HRESULT ANXAPI InputWidget_DeserializeFromYaml(
    ITuiWidget *This,
    CONST CHAR8 *Yaml,
    UINTN Length
)
{
    /* Simplified YAML parsing - in real implementation would use proper parser */
    return S_OK;
}

static HRESULT ANXAPI InputWidget_GetTypeName(
    ITuiWidget *This,
    CONST CHAR8 **OutTypeName
)
{
    *OutTypeName = "Input";
    return S_OK;
}

static HRESULT ANXAPI InputWidget_Clone(
    ITuiWidget *This,
    ITuiSerializable **OutClone
)
{
    TuiInputImplV2 *impl = INPUT_FROM_WIDGET(This);
    ITuiInput *newInput = NULL;
    HRESULT hr;

    hr = AnxTuiCreateInputV2(impl->Label, &newInput);
    if (FAILED(hr)) return hr;

    /* Copy properties */
    newInput->Vtbl->SetValue(newInput, impl->Value);
    newInput->Vtbl->SetType(newInput, impl->Type);
    newInput->Vtbl->SetRange(newInput, impl->MinValue, impl->MaxValue);
    newInput->Vtbl->SetWidth(newInput, impl->Width);
    newInput->Vtbl->SetPasswordMode(newInput, impl->PasswordMode);

    *OutClone = (ITuiSerializable *)newInput;
    return S_OK;
}

/* ITuiWidget vtable */
static ITuiWidget_Vtbl InputWidgetVtbl = {
    InputWidget_QueryInterface,
    InputWidget_AddRef,
    InputWidget_Release,
    InputWidget_SetBounds,
    InputWidget_GetBounds,
    InputWidget_SetVisible,
    InputWidget_GetVisible,
    InputWidget_SetEnabled,
    InputWidget_GetEnabled,
    InputWidget_SetParent,
    InputWidget_GetParent,
    InputWidget_Invalidate,
    InputWidget_GetNextResponder,
    InputWidget_BecomeFirstResponder,
    InputWidget_ResignFirstResponder,
    InputWidget_SerializeToYaml,
    InputWidget_DeserializeFromYaml,
    InputWidget_GetTypeName,
    InputWidget_Clone
};

/*
 * ITuiKeyListener Implementation
 */

static HRESULT ANXAPI InputKey_QueryInterface(
    ITuiKeyListener *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiInputImplV2 *impl = INPUT_FROM_KEY(This);
    return InputWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI InputKey_AddRef(ITuiKeyListener *This)
{
    TuiInputImplV2 *impl = INPUT_FROM_KEY(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI InputKey_Release(ITuiKeyListener *This)
{
    TuiInputImplV2 *impl = INPUT_FROM_KEY(This);
    return InputWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI InputKey_OnKeyDown(
    ITuiKeyListener *This,
    TUI_KEY Key,
    UINT32 Modifiers,
    BOOLEAN *Handled
)
{
    TuiInputImplV2 *impl = INPUT_FROM_KEY(This);
    UINT32 len;

    *Handled = FALSE;

    if (!impl->State.Enabled) {
        return S_OK;
    }

    len = strlen(impl->Value);

    switch (Key) {
        case TuiKeyBackspace:
            if (impl->CursorPos > 0) {
                /* Remove character before cursor */
                memmove(&impl->Value[impl->CursorPos - 1],
                        &impl->Value[impl->CursorPos],
                        len - impl->CursorPos + 1);
                impl->CursorPos--;
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyDelete:
            if (impl->CursorPos < len) {
                /* Remove character at cursor */
                memmove(&impl->Value[impl->CursorPos],
                        &impl->Value[impl->CursorPos + 1],
                        len - impl->CursorPos);
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyLeft:
            if (impl->CursorPos > 0) {
                impl->CursorPos--;
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyRight:
            if (impl->CursorPos < len) {
                impl->CursorPos++;
            }
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
            /* Handle printable characters */
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
                        /* Hex: 0-9, a-f, A-F, x for 0x prefix */
                        if (!isxdigit((unsigned char)ch) && ch != 'x' && ch != 'X') {
                            *Handled = TRUE;
                            return S_OK;
                        }
                    }
                }

                /* Insert character if buffer not full */
                if (len < sizeof(impl->Value) - 1) {
                    memmove(&impl->Value[impl->CursorPos + 1],
                            &impl->Value[impl->CursorPos],
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

static HRESULT ANXAPI InputKey_OnKeyUp(
    ITuiKeyListener *This,
    TUI_KEY Key,
    UINT32 Modifiers,
    BOOLEAN *Handled
)
{
    *Handled = FALSE;
    return S_OK;
}

static HRESULT ANXAPI InputKey_OnChar(
    ITuiKeyListener *This,
    CHAR16 Character,
    BOOLEAN *Handled
)
{
    *Handled = FALSE;
    return S_OK;
}

/* ITuiKeyListener vtable */
static ITuiKeyListener_Vtbl InputKeyVtbl = {
    InputKey_QueryInterface,
    InputKey_AddRef,
    InputKey_Release,
    InputKey_OnKeyDown,
    InputKey_OnKeyUp,
    InputKey_OnChar
};

/*
 * ITuiDrawListener Implementation
 */

static HRESULT ANXAPI InputDraw_QueryInterface(
    ITuiDrawListener *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiInputImplV2 *impl = INPUT_FROM_DRAW(This);
    return InputWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI InputDraw_AddRef(ITuiDrawListener *This)
{
    TuiInputImplV2 *impl = INPUT_FROM_DRAW(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI InputDraw_Release(ITuiDrawListener *This)
{
    TuiInputImplV2 *impl = INPUT_FROM_DRAW(This);
    return InputWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI InputDraw_OnDraw(
    ITuiDrawListener *This,
    ITuiSurface *Surface,
    CONST TUI_RECT *DirtyRect
)
{
    TuiInputImplV2 *impl = INPUT_FROM_DRAW(This);
    CHAR8 display[1024];
    CHAR8 valueDisplay[512];
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
        /* Adjust scroll to keep cursor visible */
        if (impl->CursorPos < impl->ScrollOffset) {
            impl->ScrollOffset = impl->CursorPos;
        } else if (impl->CursorPos >= impl->ScrollOffset + visibleLen) {
            impl->ScrollOffset = impl->CursorPos - visibleLen + 1;
        }
        startPos = impl->ScrollOffset;

        /* Truncate visible portion */
        CHAR8 temp[512];
        strncpy(temp, valueDisplay + startPos, visibleLen);
        temp[visibleLen] = '\0';
        strcpy(valueDisplay, temp);
    }

    /* Format: Label: [value________] */
    if (strlen(impl->Label) > 0) {
        snprintf(display, sizeof(display), "%s: [%-*s]",
                 impl->Label, visibleLen, valueDisplay);
    } else {
        snprintf(display, sizeof(display), "[%-*s]",
                 visibleLen, valueDisplay);
    }

    /* Draw to surface */
    Surface->Vtbl->WriteText(Surface, 0, 0, display, fg, bg);

    /* Draw cursor if focused */
    if (impl->State.Focused && impl->State.Enabled) {
        INT32 cursorX = strlen(impl->Label) + (strlen(impl->Label) > 0 ? 3 : 1) +
                        (impl->CursorPos - impl->ScrollOffset);
        Surface->Vtbl->WriteText(Surface, cursorX, 0, "_", fg, bg);
    }

    return S_OK;
}

static HRESULT ANXAPI InputDraw_OnGetPreferredSize(
    ITuiDrawListener *This,
    UINT32 *Width,
    UINT32 *Height
)
{
    TuiInputImplV2 *impl = INPUT_FROM_DRAW(This);

    if (Width) {
        *Width = strlen(impl->Label) + impl->Width + 4;
    }
    if (Height) {
        *Height = 1;
    }

    return S_OK;
}

/* ITuiDrawListener vtable */
static ITuiDrawListener_Vtbl InputDrawVtbl = {
    InputDraw_QueryInterface,
    InputDraw_AddRef,
    InputDraw_Release,
    InputDraw_OnDraw,
    InputDraw_OnGetPreferredSize
};

/*
 * ITuiInput Implementation (backward compatibility)
 */

static HRESULT ANXAPI Input_QueryInterface(
    ITuiInput *This,
    REFIID riid,
    VOID **ppvObject
)
{
    TuiInputImplV2 *impl = INPUT_FROM_INPUT(This);
    return InputWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI Input_AddRef(ITuiInput *This)
{
    TuiInputImplV2 *impl = INPUT_FROM_INPUT(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Input_Release(ITuiInput *This)
{
    TuiInputImplV2 *impl = INPUT_FROM_INPUT(This);
    return InputWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI Input_SetLabel(
    ITuiInput *This,
    CONST CHAR8 *Label
)
{
    TuiInputImplV2 *impl = INPUT_FROM_INPUT(This);
    if (!Label) return E_POINTER;

    strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
    impl->Label[sizeof(impl->Label) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Input_SetValue(
    ITuiInput *This,
    CONST CHAR8 *Value
)
{
    TuiInputImplV2 *impl = INPUT_FROM_INPUT(This);
    if (!Value) return E_POINTER;

    strncpy(impl->Value, Value, sizeof(impl->Value) - 1);
    impl->Value[sizeof(impl->Value) - 1] = '\0';
    impl->CursorPos = strlen(impl->Value);
    return S_OK;
}

static HRESULT ANXAPI Input_GetValue(
    ITuiInput *This,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    TuiInputImplV2 *impl = INPUT_FROM_INPUT(This);
    if (!Buffer) return E_POINTER;

    strncpy(Buffer, impl->Value, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Input_SetType(
    ITuiInput *This,
    CONFIG_VALUE_TYPE Type
)
{
    TuiInputImplV2 *impl = INPUT_FROM_INPUT(This);
    impl->Type = Type;
    return S_OK;
}

static HRESULT ANXAPI Input_SetRange(
    ITuiInput *This,
    INT64 MinValue,
    INT64 MaxValue
)
{
    TuiInputImplV2 *impl = INPUT_FROM_INPUT(This);
    impl->MinValue = MinValue;
    impl->MaxValue = MaxValue;
    return S_OK;
}

static HRESULT ANXAPI Input_SetWidth(
    ITuiInput *This,
    UINT32 Width
)
{
    TuiInputImplV2 *impl = INPUT_FROM_INPUT(This);
    impl->Width = Width;
    return S_OK;
}

static HRESULT ANXAPI Input_SetPasswordMode(
    ITuiInput *This,
    BOOLEAN PasswordMode
)
{
    TuiInputImplV2 *impl = INPUT_FROM_INPUT(This);
    impl->PasswordMode = PasswordMode;
    return S_OK;
}

static HRESULT ANXAPI Input_Render(
    ITuiInput *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    BOOLEAN Focused
)
{
    TuiInputImplV2 *impl = INPUT_FROM_INPUT(This);

    /* Use new draw listener approach */
    ITuiDrawListener *drawListener = &impl->DrawListener;

    /* Create a temporary surface wrapper around the screen */
    /* In a full implementation, this would be a proper surface */
    /* For now, we'll just call the old rendering directly on the screen */

    CHAR8 display[1024];
    CHAR8 valueDisplay[512];
    TUI_COLOR fg, bg;
    UINT32 i, visibleLen, startPos;

    if (!impl->State.Visible) return S_OK;

    impl->State.Focused = Focused;

    /* Choose colors */
    if (!impl->State.Enabled) {
        fg = TuiColorBrightBlack;
        bg = TuiColorBlack;
    } else if (Focused) {
        fg = TuiColorBlack;
        bg = TuiColorWhite;
    } else {
        fg = TuiColorWhite;
        bg = TuiColorBlue;
    }

    /* Prepare value display */
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

    /* Handle scrolling */
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

    /* Format display */
    if (strlen(impl->Label) > 0) {
        snprintf(display, sizeof(display), "%s: [%-*s]",
                 impl->Label, visibleLen, valueDisplay);
    } else {
        snprintf(display, sizeof(display), "[%-*s]",
                 visibleLen, valueDisplay);
    }

    Screen->Vtbl->WriteText(Screen, X, Y, display, fg, bg);

    /* Draw cursor */
    if (Focused && impl->State.Enabled) {
        INT32 cursorX = X + strlen(impl->Label) + (strlen(impl->Label) > 0 ? 3 : 1) +
                        (impl->CursorPos - impl->ScrollOffset);
        Screen->Vtbl->WriteText(Screen, cursorX, Y, "_", fg, bg);
    }

    return S_OK;
}

static HRESULT ANXAPI Input_HandleKey(
    ITuiInput *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiInputImplV2 *impl = INPUT_FROM_INPUT(This);

    /* Delegate to key listener */
    ITuiKeyListener *keyListener = &impl->KeyListener;
    return keyListener->Vtbl->OnKeyDown(keyListener, Key, 0, Handled);
}

/* ITuiInput vtable (backward compatibility) */
static ITuiInput_Vtbl InputVtbl = {
    Input_QueryInterface,
    Input_AddRef,
    Input_Release,
    Input_SetLabel,
    Input_SetValue,
    Input_GetValue,
    Input_SetType,
    Input_SetRange,
    Input_SetWidth,
    Input_SetPasswordMode,
    Input_Render,
    Input_HandleKey
};

/*
 * Factory Function
 */

HRESULT ANXAPI AnxTuiCreateInputV2(
    IN  CONST CHAR8 *Label,
    OUT ITuiInput **Input
)
{
    TuiInputImplV2 *impl;

    if (!Input) return E_POINTER;

    impl = (TuiInputImplV2 *)calloc(1, sizeof(TuiInputImplV2));
    if (!impl) {
        *Input = NULL;
        return E_OUTOFMEMORY;
    }

    /* Initialize all vtables */
    impl->WidgetInterface.Vtbl = &InputWidgetVtbl;
    impl->InputInterface.Vtbl = &InputVtbl;
    impl->KeyListener.Vtbl = &InputKeyVtbl;
    impl->DrawListener.Vtbl = &InputDrawVtbl;

    /* Initialize state */
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

    *Input = &impl->InputInterface;
    return S_OK;
}
