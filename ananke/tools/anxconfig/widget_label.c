/*
 * Label Widget Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

typedef struct {
    ITuiWidget WidgetInterface;
    ITuiLabel LabelInterface;
    ITuiDrawListener DrawListener;

    WIDGET_STATE State;
    CHAR8 Text[512];
    CHAR8 Hotkey;
    TUI_TEXT_ALIGNMENT Alignment;
    BOOLEAN Wrap;
    ITuiResponder *NextResponder;
    ITuiSurface *Surface;
} TuiLabelImpl;

/* Helper macros for interface conversions */
#define LABEL_FROM_WIDGET(w) ((TuiLabelImpl*)((UINT8*)(w) - offsetof(TuiLabelImpl, WidgetInterface)))
#define LABEL_FROM_LABEL(l) ((TuiLabelImpl*)((UINT8*)(l) - offsetof(TuiLabelImpl, LabelInterface)))
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
    if (IsEqualGUID(riid, &IID_ITuiDrawListener)) {
        *ppvObject = &impl->DrawListener;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiLabel)) {
        *ppvObject = &impl->LabelInterface;
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
 * ITuiLabel Implementation (Backward Compatibility)
 *===========================================================================*/

static HRESULT ANXAPI Label_QueryInterface(ITuiLabel *This, REFIID riid, VOID **ppvObject)
{
    TuiLabelImpl *impl = LABEL_FROM_LABEL(This);
    return LabelWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI Label_AddRef(ITuiLabel *This)
{
    TuiLabelImpl *impl = LABEL_FROM_LABEL(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Label_Release(ITuiLabel *This)
{
    TuiLabelImpl *impl = LABEL_FROM_LABEL(This);
    return LabelWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI Label_SetText(ITuiLabel *This, CONST CHAR8 *Text)
{
    TuiLabelImpl *impl = LABEL_FROM_LABEL(This);
    if (Text == NULL) return E_POINTER;

    strncpy(impl->Text, Text, sizeof(impl->Text) - 1);
    impl->Text[sizeof(impl->Text) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Label_GetText(ITuiLabel *This, CHAR8 *Buffer, UINTN BufferSize)
{
    TuiLabelImpl *impl = LABEL_FROM_LABEL(This);
    if (Buffer == NULL) return E_POINTER;

    strncpy(Buffer, impl->Text, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI Label_SetHotkey(ITuiLabel *This, CHAR8 Hotkey)
{
    TuiLabelImpl *impl = LABEL_FROM_LABEL(This);
    impl->Hotkey = Hotkey;
    return S_OK;
}

static HRESULT ANXAPI Label_SetAlignment(ITuiLabel *This, TUI_TEXT_ALIGNMENT Alignment)
{
    TuiLabelImpl *impl = LABEL_FROM_LABEL(This);
    impl->Alignment = Alignment;
    return S_OK;
}

static HRESULT ANXAPI Label_SetWordWrap(ITuiLabel *This, BOOLEAN Wrap)
{
    TuiLabelImpl *impl = LABEL_FROM_LABEL(This);
    impl->Wrap = Wrap;
    return S_OK;
}

static HRESULT ANXAPI Label_Render(ITuiLabel *This, ITuiScreen *Screen, INT32 X, INT32 Y, UINT32 Width)
{
    TuiLabelImpl *impl = LABEL_FROM_LABEL(This);

    /* Legacy render method - create temporary surface and delegate to OnDraw */
    if (impl->Surface) {
        TUI_RECT rect = { X, Y, X + Width, Y + 1 };
        return LabelDraw_OnDraw(&impl->DrawListener, impl->Surface, &rect);
    }

    return S_OK;
}

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

/*=============================================================================
 * Factory Function
 *===========================================================================*/

HRESULT ANXAPI AnxTuiCreateLabel(IN CONST CHAR8 *Text, OUT ITuiLabel **Label)
{
    TuiLabelImpl *impl;

    if (Label == NULL) return E_POINTER;

    impl = (TuiLabelImpl *)calloc(1, sizeof(TuiLabelImpl));
    if (impl == NULL) {
        *Label = NULL;
        return E_OUTOFMEMORY;
    }

    /* Initialize vtables */
    impl->WidgetInterface.Vtbl = &LabelWidgetVtbl;
    impl->LabelInterface.Vtbl = &LabelVtbl;
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
    impl->NextResponder = NULL;
    impl->Surface = NULL;

    *Label = &impl->LabelInterface;
    return S_OK;
}
