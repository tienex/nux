/*
 * Input Widget Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "widgets_common.h"

typedef struct {
    ITuiInput Interface;
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
} TuiInputImpl;

/* IUnknown methods */
static HRESULT ANXAPI Input_QueryInterface(
    ITuiInput *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI Input_AddRef(ITuiInput *This)
{
    TuiInputImpl *impl = (TuiInputImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Input_Release(ITuiInput *This)
{
    TuiInputImpl *impl = (TuiInputImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiInput methods */
static HRESULT ANXAPI Input_SetLabel(
    ITuiInput *This,
    CONST CHAR8 *Label
)
{
    TuiInputImpl *impl = (TuiInputImpl *)This;
    if (Label == NULL) return E_POINTER;

    strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
    impl->Label[sizeof(impl->Label) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Input_SetValue(
    ITuiInput *This,
    CONST CHAR8 *Value
)
{
    TuiInputImpl *impl = (TuiInputImpl *)This;
    if (Value == NULL) return E_POINTER;

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
    TuiInputImpl *impl = (TuiInputImpl *)This;
    if (Buffer == NULL) return E_POINTER;

    strncpy(Buffer, impl->Value, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Input_SetType(
    ITuiInput *This,
    CONFIG_VALUE_TYPE Type
)
{
    TuiInputImpl *impl = (TuiInputImpl *)This;
    impl->Type = Type;
    return S_OK;
}

static HRESULT ANXAPI Input_SetRange(
    ITuiInput *This,
    INT64 MinValue,
    INT64 MaxValue
)
{
    TuiInputImpl *impl = (TuiInputImpl *)This;
    impl->MinValue = MinValue;
    impl->MaxValue = MaxValue;
    return S_OK;
}

static HRESULT ANXAPI Input_SetWidth(
    ITuiInput *This,
    UINT32 Width
)
{
    TuiInputImpl *impl = (TuiInputImpl *)This;
    impl->Width = Width;
    return S_OK;
}

static HRESULT ANXAPI Input_SetPasswordMode(
    ITuiInput *This,
    BOOLEAN PasswordMode
)
{
    TuiInputImpl *impl = (TuiInputImpl *)This;
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
    TuiInputImpl *impl = (TuiInputImpl *)This;
    CHAR8 display[1024];
    CHAR8 valueDisplay[512];
    TUI_COLOR fg, bg;
    UINT32 i, visibleLen, startPos;

    if (!impl->State.Visible) return S_OK;

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

    /* Prepare value display (handle password mode and scrolling) */
    if (impl->PasswordMode) {
        /* Show asterisks */
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

    Screen->Vtbl->WriteText(Screen, X, Y, display, fg, bg);

    /* Draw cursor if focused */
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
    TuiInputImpl *impl = (TuiInputImpl *)This;
    UINT32 len;

    if (!impl->State.Enabled) {
        *Handled = FALSE;
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
                    /* Only allow digits, minus for integers */
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

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiInput_Vtbl InputVtbl = {
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

/* Factory function */
HRESULT ANXAPI AnxTuiCreateInput(
    IN  CONST CHAR8 *Label,
    OUT ITuiInput **Input
)
{
    TuiInputImpl *impl;

    if (Input == NULL) return E_POINTER;

    impl = (TuiInputImpl *)calloc(1, sizeof(TuiInputImpl));
    if (impl == NULL) {
        *Input = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &InputVtbl;
    InitWidgetState(&impl->State);

    if (Label != NULL) {
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

    *Input = &impl->Interface;
    return S_OK;
}
