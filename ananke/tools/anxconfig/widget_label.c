/*
 * Label Widget Implementation
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

    WIDGET_STATE State;
    CHAR8 Text[512];
    CHAR8 Hotkey;
    TUI_TEXT_ALIGNMENT Alignment;
    BOOLEAN Wrap;
    ITuiWidget *LinkedWidget;
    ITuiResponder *NextResponder;
    ITuiSurface *Surface;
} TuiLabelImpl;

/* Helper macros for interface conversions */
#define LABEL_FROM_WIDGET(w) ((TuiLabelImpl*)((UINT8*)(w) - offsetof(TuiLabelImpl, WidgetInterface)))
#define LABEL_FROM_KEY(k) ((TuiLabelImpl*)((UINT8*)(k) - offsetof(TuiLabelImpl, KeyListener)))
#define LABEL_FROM_MOUSE(m) ((TuiLabelImpl*)((UINT8*)(m) - offsetof(TuiLabelImpl, MouseListener)))
#define LABEL_FROM_DRAW(d) ((TuiLabelImpl*)((UINT8*)(d) - offsetof(TuiLabelImpl, DrawListener)))

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

/*=============================================================================
 * ITuiWidget Implementation
 *===========================================================================*/

static HRESULT ANXAPI LabelWidget_QueryInterface(ITuiWidget *This, REFIID riid, VOID **ppvObject)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
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

static UINTN ANXAPI LabelWidget_AddRef(ITuiWidget *This)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI LabelWidget_Release(ITuiWidget *This)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        if (impl->Surface) impl->Surface->Vtbl->Release(impl->Surface);
        free(impl);
    }
    return refCount;
}

static HRESULT ANXAPI LabelWidget_GetBounds(ITuiWidget *This, TUI_RECT *Bounds)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    if (!Bounds) return E_POINTER;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_SetBounds(ITuiWidget *This, CONST TUI_RECT *Bounds)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    if (!Bounds) return E_POINTER;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_GetPreferredSize(ITuiWidget *This, UINT32 *Width, UINT32 *Height)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    if (!Width || !Height) return E_POINTER;
    *Width = strlen(impl->Text);
    *Height = 1;
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_SetVisible(ITuiWidget *This, BOOLEAN Visible)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    impl->State.Visible = Visible;
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_IsVisible(ITuiWidget *This, BOOLEAN *Visible)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    if (!Visible) return E_POINTER;
    *Visible = impl->State.Visible;
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_SetEnabled(ITuiWidget *This, BOOLEAN Enabled)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    impl->State.Enabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_IsEnabled(ITuiWidget *This, BOOLEAN *Enabled)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    if (!Enabled) return E_POINTER;
    *Enabled = impl->State.Enabled;
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_SetFocused(ITuiWidget *This, BOOLEAN Focused)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    impl->State.Focused = Focused;
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_IsFocused(ITuiWidget *This, BOOLEAN *Focused)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    if (!Focused) return E_POINTER;
    *Focused = impl->State.Focused;
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_SetColors(ITuiWidget *This, TUI_COLOR Foreground, TUI_COLOR Background)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    impl->State.ForegroundColor = Foreground;
    impl->State.BackgroundColor = Background;
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_GetColors(ITuiWidget *This, TUI_COLOR *Foreground, TUI_COLOR *Background)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    if (Foreground) *Foreground = impl->State.ForegroundColor;
    if (Background) *Background = impl->State.BackgroundColor;
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_Invalidate(ITuiWidget *This, CONST TUI_RECT *Region)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    if (impl->Surface) {
        return impl->Surface->Vtbl->Invalidate(impl->Surface, Region);
    }
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_SetSurface(ITuiWidget *This, ITuiSurface *Surface)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    if (impl->Surface) impl->Surface->Vtbl->Release(impl->Surface);
    impl->Surface = Surface;
    if (Surface) Surface->Vtbl->AddRef(Surface);
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_GetSurface(ITuiWidget *This, ITuiSurface **Surface)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    if (!Surface) return E_POINTER;
    *Surface = impl->Surface;
    if (impl->Surface) impl->Surface->Vtbl->AddRef(impl->Surface);
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_GetNextResponder(ITuiWidget *This, ITuiResponder **NextResponder)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    if (!NextResponder) return E_POINTER;
    *NextResponder = impl->NextResponder;
    return S_OK;
}

static HRESULT ANXAPI LabelWidget_SetNextResponder(ITuiWidget *This, ITuiResponder *NextResponder)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(This);
    impl->NextResponder = NextResponder;
    return S_OK;
}

static CONST ITuiWidget_Vtbl LabelWidgetVtbl = {
    LabelWidget_QueryInterface,
    LabelWidget_AddRef,
    LabelWidget_Release,
    LabelWidget_GetBounds,
    LabelWidget_SetBounds,
    LabelWidget_GetPreferredSize,
    LabelWidget_SetVisible,
    LabelWidget_IsVisible,
    LabelWidget_SetEnabled,
    LabelWidget_IsEnabled,
    LabelWidget_SetFocused,
    LabelWidget_IsFocused,
    LabelWidget_SetColors,
    LabelWidget_GetColors,
    LabelWidget_Invalidate,
    LabelWidget_SetSurface,
    LabelWidget_GetSurface,
    LabelWidget_GetNextResponder,
    LabelWidget_SetNextResponder
};

/*=============================================================================
 * ITuiKeyListener Implementation
 *===========================================================================*/

static HRESULT ANXAPI LabelKey_QueryInterface(ITuiKeyListener *This, REFIID riid, VOID **ppvObject)
{
    TuiLabelImpl *impl = LABEL_FROM_KEY(This);
    return LabelWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI LabelKey_AddRef(ITuiKeyListener *This)
{
    TuiLabelImpl *impl = LABEL_FROM_KEY(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI LabelKey_Release(ITuiKeyListener *This)
{
    TuiLabelImpl *impl = LABEL_FROM_KEY(This);
    return LabelWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI LabelKey_OnKeyDown(ITuiKeyListener *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled)
{
    TuiLabelImpl *impl = LABEL_FROM_KEY(This);
    *Handled = FALSE;

    /* Check if pressed key matches hotkey (case insensitive) */
    if (impl->Hotkey != '\0' && impl->LinkedWidget != NULL) {
        CHAR8 upperKey = (Key >= 'a' && Key <= 'z') ? (Key - 32) : Key;
        CHAR8 upperHotkey = (impl->Hotkey >= 'a' && impl->Hotkey <= 'z') ? (impl->Hotkey - 32) : impl->Hotkey;

        if (upperKey == upperHotkey) {
            /* Focus the linked widget */
            ITuiResponder *responder = NULL;
            HRESULT hr = impl->LinkedWidget->Vtbl->QueryInterface(impl->LinkedWidget, &IID_ITuiResponder, (VOID**)&responder);
            if (SUCCEEDED(hr) && responder != NULL) {
                responder->Vtbl->BecomeFirstResponder(responder);
                responder->Vtbl->Release(responder);
            }
            *Handled = TRUE;
            return S_OK;
        }
    }

    return S_OK;
}

static HRESULT ANXAPI LabelKey_OnKeyUp(ITuiKeyListener *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled)
{
    *Handled = FALSE;
    return S_OK;
}

static HRESULT ANXAPI LabelKey_OnChar(ITuiKeyListener *This, CHAR16 Character, BOOLEAN *Handled)
{
    *Handled = FALSE;
    return S_OK;
}

static CONST ITuiKeyListener_Vtbl LabelKeyVtbl = {
    LabelKey_QueryInterface,
    LabelKey_AddRef,
    LabelKey_Release,
    LabelKey_OnKeyDown,
    LabelKey_OnKeyUp,
    LabelKey_OnChar
};

/*=============================================================================
 * ITuiMouseListener Implementation
 *===========================================================================*/

static HRESULT ANXAPI LabelMouse_QueryInterface(ITuiMouseListener *This, REFIID riid, VOID **ppvObject)
{
    TuiLabelImpl *impl = LABEL_FROM_MOUSE(This);
    return LabelWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI LabelMouse_AddRef(ITuiMouseListener *This)
{
    TuiLabelImpl *impl = LABEL_FROM_MOUSE(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI LabelMouse_Release(ITuiMouseListener *This)
{
    TuiLabelImpl *impl = LABEL_FROM_MOUSE(This);
    return LabelWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI LabelMouse_OnMouseEvent(ITuiMouseListener *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled)
{
    TuiLabelImpl *impl = LABEL_FROM_MOUSE(This);
    BOOLEAN isOver;

    *Handled = FALSE;

    /* Only handle if we have a linked widget */
    if (impl->LinkedWidget == NULL) return S_OK;

    isOver = IsPointInWidget(&impl->State, Event->X, Event->Y);
    if (!isOver) return S_OK;

    /* Click to focus linked widget */
    if (Event->Type == TuiMouseLeftDown) {
        ITuiResponder *responder = NULL;
        HRESULT hr = impl->LinkedWidget->Vtbl->QueryInterface(impl->LinkedWidget, &IID_ITuiResponder, (VOID**)&responder);
        if (SUCCEEDED(hr) && responder != NULL) {
            responder->Vtbl->BecomeFirstResponder(responder);
            responder->Vtbl->Release(responder);
        }
        *Handled = TRUE;
    }

    return S_OK;
}

static CONST ITuiMouseListener_Vtbl LabelMouseVtbl = {
    LabelMouse_QueryInterface,
    LabelMouse_AddRef,
    LabelMouse_Release,
    LabelMouse_OnMouseEvent
};

/*=============================================================================
 * ITuiDrawListener Implementation
 *===========================================================================*/

static HRESULT ANXAPI LabelDraw_QueryInterface(ITuiDrawListener *This, REFIID riid, VOID **ppvObject)
{
    TuiLabelImpl *impl = LABEL_FROM_DRAW(This);
    return LabelWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI LabelDraw_AddRef(ITuiDrawListener *This)
{
    TuiLabelImpl *impl = LABEL_FROM_DRAW(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI LabelDraw_Release(ITuiDrawListener *This)
{
    TuiLabelImpl *impl = LABEL_FROM_DRAW(This);
    return LabelWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI LabelDraw_OnDraw(ITuiDrawListener *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect)
{
    TuiLabelImpl *impl = LABEL_FROM_DRAW(This);
    TUI_COLOR fg, bg;
    INT32 hotkeyPos;
    INT32 startX;
    UINT32 textLen;
    UINT32 width;

    if (!impl->State.Visible) return S_OK;

    fg = impl->State.Enabled ? impl->State.ForegroundColor : TuiColorBrightBlack;
    bg = impl->State.BackgroundColor;

    textLen = strlen(impl->Text);
    width = impl->State.Bounds.Right - impl->State.Bounds.Left;

    /* Calculate starting X position based on alignment */
    startX = 0;
    if (width > 0) {
        if (impl->Alignment == TuiAlignCenter) {
            startX = (width - textLen) / 2;
        } else if (impl->Alignment == TuiAlignRight) {
            startX = width - textLen;
        }
    }

    /* Render text */
    if (impl->Wrap && width > 0) {
        /* Word wrap implementation (simple version) */
        CHAR8 line[256];
        CONST CHAR8 *p = impl->Text;
        INT32 currentY = 0;
        UINT32 linePos = 0;

        while (*p != '\0') {
            if (*p == '\n' || linePos >= width - 1) {
                line[linePos] = '\0';
                Surface->Vtbl->WriteText(Surface, 0, currentY, line, fg, bg);
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
            Surface->Vtbl->WriteText(Surface, 0, currentY, line, fg, bg);
        }
    } else {
        /* Single line, no wrap */
        Surface->Vtbl->WriteText(Surface, startX, 0, impl->Text, fg, bg);

        /* Underline hotkey character if present */
        if (impl->State.Enabled) {
            hotkeyPos = FindHotkeyPos(impl->Text, impl->Hotkey);
            if (hotkeyPos >= 0) {
                CHAR8 hotChar[2] = { impl->Text[hotkeyPos], '\0' };
                Surface->Vtbl->WriteText(Surface, startX + hotkeyPos, 0, hotChar,
                                        TuiColorYellow, bg);
            }
        }
    }

    return S_OK;
}

static CONST ITuiDrawListener_Vtbl LabelDrawVtbl = {
    LabelDraw_QueryInterface,
    LabelDraw_AddRef,
    LabelDraw_Release,
    LabelDraw_OnDraw
};

/*=============================================================================
 * Factory Function
 *===========================================================================*/

HRESULT ANXAPI AnxTuiCreateLabel(IN CONST CHAR8 *Text, OUT ITuiWidget **Widget)
{
    TuiLabelImpl *impl;

    if (Widget == NULL) return E_POINTER;

    impl = (TuiLabelImpl *)calloc(1, sizeof(TuiLabelImpl));
    if (impl == NULL) {
        *Widget = NULL;
        return E_OUTOFMEMORY;
    }

    /* Initialize vtables */
    impl->WidgetInterface.Vtbl = &LabelWidgetVtbl;
    impl->KeyListener.Vtbl = &LabelKeyVtbl;
    impl->MouseListener.Vtbl = &LabelMouseVtbl;
    impl->DrawListener.Vtbl = &LabelDrawVtbl;

    /* Initialize widget state */
    InitWidgetState(&impl->State);

    /* Initialize label-specific state */
    if (Text != NULL) {
        strncpy(impl->Text, Text, sizeof(impl->Text) - 1);
        impl->Text[sizeof(impl->Text) - 1] = '\0';
    } else {
        impl->Text[0] = '\0';
    }

    impl->Hotkey = '\0';
    impl->Alignment = TuiAlignLeft;
    impl->Wrap = FALSE;
    impl->LinkedWidget = NULL;
    impl->NextResponder = NULL;
    impl->Surface = NULL;

    *Widget = &impl->WidgetInterface;
    return S_OK;
}

/* Convenience methods for label-specific functionality */
HRESULT ANXAPI AnxTuiLabelSetText(ITuiWidget *Widget, CONST CHAR8 *Text)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(Widget);
    if (Text == NULL) return E_POINTER;

    strncpy(impl->Text, Text, sizeof(impl->Text) - 1);
    impl->Text[sizeof(impl->Text) - 1] = '\0';
    return S_OK;
}

HRESULT ANXAPI AnxTuiLabelGetText(ITuiWidget *Widget, CHAR8 *Buffer, UINTN BufferSize)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(Widget);
    if (Buffer == NULL) return E_POINTER;

    strncpy(Buffer, impl->Text, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

HRESULT ANXAPI AnxTuiLabelSetHotkey(ITuiWidget *Widget, CHAR8 Hotkey)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(Widget);
    impl->Hotkey = Hotkey;
    return S_OK;
}

HRESULT ANXAPI AnxTuiLabelSetAlignment(ITuiWidget *Widget, TUI_TEXT_ALIGNMENT Alignment)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(Widget);
    impl->Alignment = Alignment;
    return S_OK;
}

HRESULT ANXAPI AnxTuiLabelSetWordWrap(ITuiWidget *Widget, BOOLEAN Wrap)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(Widget);
    impl->Wrap = Wrap;
    return S_OK;
}

HRESULT ANXAPI AnxTuiLabelSetLinkedWidget(ITuiWidget *Widget, ITuiWidget *LinkedWidget)
{
    TuiLabelImpl *impl = LABEL_FROM_WIDGET(Widget);
    impl->LinkedWidget = LinkedWidget;
    return S_OK;
}
