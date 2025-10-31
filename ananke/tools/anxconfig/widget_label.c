/*
 * Label Widget Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

typedef struct {
    ITuiLabel Interface;
    WIDGET_STATE State;
    CHAR8 Text[512];
    CHAR8 Hotkey;
    TUI_TEXT_ALIGNMENT Alignment;
    BOOLEAN Wrap;
} TuiLabelImpl;

/* IUnknown methods */
static HRESULT ANXAPI Label_QueryInterface(
    ITuiLabel *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI Label_AddRef(ITuiLabel *This)
{
    TuiLabelImpl *impl = (TuiLabelImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Label_Release(ITuiLabel *This)
{
    TuiLabelImpl *impl = (TuiLabelImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiLabel methods */
static HRESULT ANXAPI Label_SetText(
    ITuiLabel *This,
    CONST CHAR8 *Text
)
{
    TuiLabelImpl *impl = (TuiLabelImpl *)This;
    if (Text == NULL) return E_POINTER;

    strncpy(impl->Text, Text, sizeof(impl->Text) - 1);
    impl->Text[sizeof(impl->Text) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Label_GetText(
    ITuiLabel *This,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    TuiLabelImpl *impl = (TuiLabelImpl *)This;
    if (Buffer == NULL) return E_POINTER;

    strncpy(Buffer, impl->Text, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Label_SetHotkey(
    ITuiLabel *This,
    CHAR8 Hotkey
)
{
    TuiLabelImpl *impl = (TuiLabelImpl *)This;
    impl->Hotkey = Hotkey;
    return S_OK;
}

static HRESULT ANXAPI Label_SetAlignment(
    ITuiLabel *This,
    TUI_TEXT_ALIGNMENT Alignment
)
{
    TuiLabelImpl *impl = (TuiLabelImpl *)This;
    impl->Alignment = Alignment;
    return S_OK;
}

static HRESULT ANXAPI Label_SetWordWrap(
    ITuiLabel *This,
    BOOLEAN Wrap
)
{
    TuiLabelImpl *impl = (TuiLabelImpl *)This;
    impl->Wrap = Wrap;
    return S_OK;
}

/* Helper: Find hotkey position in text */
static INT32 FindHotkeyPos(CONST CHAR8 *Text, CHAR8 Hotkey)
{
    INT32 i;
    if (Hotkey == '\0') return -1;

    for (i = 0; Text[i] != '\0'; i++) {
        if (Text[i] == Hotkey || Text[i] == (Hotkey + 32) || Text[i] == (Hotkey - 32)) {
            return i;
        }
    }
    return -1;
}

static HRESULT ANXAPI Label_Render(
    ITuiLabel *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width
)
{
    TuiLabelImpl *impl = (TuiLabelImpl *)This;
    TUI_COLOR fg, bg;
    INT32 hotkeyPos;
    INT32 startX;
    UINT32 textLen;

    if (!impl->State.Visible) return S_OK;

    fg = impl->State.Enabled ? impl->State.ForegroundColor : TuiColorBrightBlack;
    bg = impl->State.BackgroundColor;

    textLen = strlen(impl->Text);

    /* Calculate starting X position based on alignment */
    startX = X;
    if (Width > 0) {
        if (impl->Alignment == TuiAlignCenter) {
            startX = X + (Width - textLen) / 2;
        } else if (impl->Alignment == TuiAlignRight) {
            startX = X + Width - textLen;
        }
    }

    /* Render text */
    if (impl->Wrap && Width > 0) {
        /* Word wrap implementation (simple version) */
        CHAR8 line[256];
        CONST CHAR8 *p = impl->Text;
        INT32 currentY = Y;
        UINT32 linePos = 0;

        while (*p != '\0') {
            if (*p == '\n' || linePos >= Width - 1) {
                line[linePos] = '\0';
                Screen->Vtbl->WriteText(Screen, X, currentY, line, fg, bg);
                currentY++;
                linePos = 0;
                if (*p == '\n') {
                    p++;
                    continue;
                }
            }
            line[linePos++] = *p++;
        }
        if (linePos > 0) {
            line[linePos] = '\0';
            Screen->Vtbl->WriteText(Screen, X, currentY, line, fg, bg);
        }
    } else {
        /* Single line, no wrap */
        Screen->Vtbl->WriteText(Screen, startX, Y, impl->Text, fg, bg);

        /* Underline hotkey character if present */
        if (impl->State.Enabled) {
            hotkeyPos = FindHotkeyPos(impl->Text, impl->Hotkey);
            if (hotkeyPos >= 0) {
                CHAR8 hotChar[2] = { impl->Text[hotkeyPos], '\0' };
                Screen->Vtbl->WriteText(Screen, startX + hotkeyPos, Y, hotChar,
                                        TuiColorYellow, bg);
            }
        }
    }

    return S_OK;
}

/* Vtable */
static CONST ITuiLabel_Vtbl LabelVtbl = {
    Label_QueryInterface,
    Label_AddRef,
    Label_Release,
    Label_SetText,
    Label_GetText,
    Label_SetHotkey,
    Label_SetAlignment,
    Label_SetWordWrap,
    Label_Render
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateLabel(
    IN  CONST CHAR8 *Text,
    OUT ITuiLabel **Label
)
{
    TuiLabelImpl *impl;

    if (Label == NULL) return E_POINTER;

    impl = (TuiLabelImpl *)calloc(1, sizeof(TuiLabelImpl));
    if (impl == NULL) {
        *Label = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &LabelVtbl;
    InitWidgetState(&impl->State);

    if (Text != NULL) {
        strncpy(impl->Text, Text, sizeof(impl->Text) - 1);
        impl->Text[sizeof(impl->Text) - 1] = '\0';
    } else {
        impl->Text[0] = '\0';
    }

    impl->Hotkey = '\0';
    impl->Alignment = TuiAlignLeft;
    impl->Wrap = FALSE;

    *Label = &impl->Interface;
    return S_OK;
}
