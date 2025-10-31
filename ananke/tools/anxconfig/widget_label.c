/*
 * Label Widget Implementation
 *
 * Static text label with hotkey and linked widget support.
 * Follows the new architecture: ITuiSerializable > ITuiResponder > ITuiWidget > ITuiThemedWidget > ITuiThemedLabel > ITuiLabel
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

#define MAX_TEXT_LENGTH 512

typedef struct {
    ITuiLabel Interface;
    WIDGET_STATE State;

    /* Label data */
    CHAR8 Text[MAX_TEXT_LENGTH];
    CHAR8 Hotkey;
    TUI_TEXT_ALIGNMENT Alignment;
    TUI_TEXT_DIRECTION Direction;
    TUI_COLOR HotkeyColor;
    ITuiWidget *LinkedWidget;
} TuiLabelImpl;

/* Helper to get impl from interface */
#define LABEL_FROM_INTERFACE(iface) ((TuiLabelImpl*)((CHAR8*)(iface) - offsetof(TuiLabelImpl, Interface)))

/* Forward declarations */
static HRESULT ANXAPI Label_QueryInterface(ITuiLabel *This, REFIID riid, VOID **ppvObject);
static UINTN ANXAPI Label_AddRef(ITuiLabel *This);
static UINTN ANXAPI Label_Release(ITuiLabel *This);

/* ITuiSerializable methods */
static HRESULT ANXAPI Label_SerializeToYaml(ITuiLabel *This, CHAR8 **OutYaml, UINTN *OutLength)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

static HRESULT ANXAPI Label_DeserializeFromYaml(ITuiLabel *This, CONST CHAR8 *Yaml, UINTN Length)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

static HRESULT ANXAPI Label_GetTypeName(ITuiLabel *This, CONST CHAR8 **OutTypeName)
{
    if (OutTypeName == NULL) return E_POINTER;
    *OutTypeName = "Label";
    return S_OK;
}

static HRESULT ANXAPI Label_Clone(ITuiLabel *This, ITuiSerializable **OutClone)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

/* ITuiResponder methods */
static HRESULT ANXAPI Label_GetNextResponder(ITuiLabel *This, ITuiResponder **NextResponder)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

static HRESULT ANXAPI Label_SetNextResponder(ITuiLabel *This, ITuiResponder *NextResponder)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

static BOOLEAN ANXAPI Label_AcceptsFirstResponder(ITuiLabel *This)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);
    /* Labels accept first responder only if they have a linked widget or hotkey */
    return (impl->LinkedWidget != NULL || impl->Hotkey != '\0');
}

static HRESULT ANXAPI Label_BecomeFirstResponder(ITuiLabel *This)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

static HRESULT ANXAPI Label_ResignFirstResponder(ITuiLabel *This)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

/* ITuiWidget methods */
static HRESULT ANXAPI Label_SetBounds(ITuiLabel *This, CONST TUI_RECT *Bounds)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

static HRESULT ANXAPI Label_GetBounds(ITuiLabel *This, TUI_RECT *Bounds)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

static HRESULT ANXAPI Label_SetVisible(ITuiLabel *This, BOOLEAN Visible)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

static BOOLEAN ANXAPI Label_IsVisible(ITuiLabel *This)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);
    return impl->State.Visible;
}

static HRESULT ANXAPI Label_SetEnabled(ITuiLabel *This, BOOLEAN Enabled)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

static BOOLEAN ANXAPI Label_IsEnabled(ITuiLabel *This)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);
    return impl->State.Enabled;
}

static HRESULT ANXAPI Label_SetParent(ITuiLabel *This, ITuiWidget *Parent)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

static HRESULT ANXAPI Label_GetParent(ITuiLabel *This, ITuiWidget **Parent)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

static HRESULT ANXAPI Label_AddChild(ITuiLabel *This, ITuiWidget *Child)
{
    return E_NOTIMPL; /* Labels don't have children */
}

static HRESULT ANXAPI Label_RemoveChild(ITuiLabel *This, ITuiWidget *Child)
{
    return E_NOTIMPL; /* Labels don't have children */
}

static HRESULT ANXAPI Label_SetNeedsDisplay(ITuiLabel *This, BOOLEAN Needed)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

/* ITuiThemedWidget methods */
static HRESULT ANXAPI Label_ApplyTheme(ITuiLabel *This, ITuiTheme *Theme)
{
    return E_NOTIMPL; /* Framework wrapper handles this */
}

static HRESULT ANXAPI Label_GetColors(ITuiLabel *This, TUI_COLOR *Foreground, TUI_COLOR *Background)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);
    if (Foreground) *Foreground = impl->State.ForegroundColor;
    if (Background) *Background = impl->State.BackgroundColor;
    return S_OK;
}

static HRESULT ANXAPI Label_SetColors(ITuiLabel *This, TUI_COLOR Foreground, TUI_COLOR Background)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);
    impl->State.ForegroundColor = Foreground;
    impl->State.BackgroundColor = Background;
    return S_OK;
}

static HRESULT ANXAPI Label_OnMouseEvent(ITuiLabel *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);

    if (!Handled) return E_POINTER;
    *Handled = FALSE;

    /* Handle mouse click to focus linked widget */
    if (Event->Type == TuiMouseButtonDown && Event->Button == TuiMouseButtonLeft) {
        if (impl->LinkedWidget != NULL) {
            ITuiResponder *responder = NULL;
            HRESULT hr = impl->LinkedWidget->Vtbl->QueryInterface(
                impl->LinkedWidget,
                &IID_ITuiResponder,
                (VOID**)&responder
            );

            if (SUCCEEDED(hr) && responder != NULL) {
                responder->Vtbl->BecomeFirstResponder(responder);
                responder->Vtbl->Release(responder);
                *Handled = TRUE;
                return S_OK;
            }
        }
    }

    return S_OK;
}

static HRESULT ANXAPI Label_OnKeyEvent(ITuiLabel *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);

    if (!Handled) return E_POINTER;
    *Handled = FALSE;

    /* Handle hotkey to focus linked widget */
    if (impl->Hotkey != '\0' && impl->LinkedWidget != NULL) {
        CHAR8 upperKey = (Key >= 'a' && Key <= 'z') ? (Key - 32) : Key;
        CHAR8 upperHotkey = (impl->Hotkey >= 'a' && impl->Hotkey <= 'z') ? (impl->Hotkey - 32) : impl->Hotkey;

        if (upperKey == upperHotkey) {
            ITuiResponder *responder = NULL;
            HRESULT hr = impl->LinkedWidget->Vtbl->QueryInterface(
                impl->LinkedWidget,
                &IID_ITuiResponder,
                (VOID**)&responder
            );

            if (SUCCEEDED(hr) && responder != NULL) {
                responder->Vtbl->BecomeFirstResponder(responder);
                responder->Vtbl->Release(responder);
                *Handled = TRUE;
                return S_OK;
            }
        }
    }

    return S_OK;
}

static HRESULT ANXAPI Label_Draw(ITuiLabel *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);

    if (Surface == NULL) return E_POINTER;
    if (!impl->State.Visible) return S_OK;

    /* Get bounds */
    TUI_RECT bounds = impl->State.Bounds;
    if (bounds.Width == 0) return S_OK;

    /* Draw label text */
    UINT32 textLen = (UINT32)strlen(impl->Text);
    INT32 x = bounds.X;
    INT32 y = bounds.Y;

    /* Calculate alignment offset */
    if (impl->Alignment == TuiTextAlignmentCenter && textLen < bounds.Width) {
        x += (bounds.Width - textLen) / 2;
    } else if (impl->Alignment == TuiTextAlignmentRight && textLen < bounds.Width) {
        x += bounds.Width - textLen;
    }

    /* Draw each character */
    for (UINT32 i = 0; i < textLen && i < bounds.Width; i++) {
        TUI_COLOR fg = impl->State.ForegroundColor;

        /* Highlight hotkey character */
        if (impl->Hotkey != '\0') {
            CHAR8 upperChar = (impl->Text[i] >= 'a' && impl->Text[i] <= 'z') ? (impl->Text[i] - 32) : impl->Text[i];
            CHAR8 upperHotkey = (impl->Hotkey >= 'a' && impl->Hotkey <= 'z') ? (impl->Hotkey - 32) : impl->Hotkey;

            if (upperChar == upperHotkey) {
                fg = impl->HotkeyColor;
            }
        }

        /* Use surface to draw character */
        Surface->Vtbl->WriteChar(Surface, x + i, y, impl->Text[i], fg, impl->State.BackgroundColor);
    }

    return S_OK;
}

/* ITuiThemedLabel methods */
static HRESULT ANXAPI Label_GetHotkeyColor(ITuiLabel *This, TUI_COLOR *HotkeyColor)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);
    if (HotkeyColor == NULL) return E_POINTER;
    *HotkeyColor = impl->HotkeyColor;
    return S_OK;
}

static HRESULT ANXAPI Label_SetHotkeyColor(ITuiLabel *This, TUI_COLOR HotkeyColor)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);
    impl->HotkeyColor = HotkeyColor;
    return S_OK;
}

/* ITuiLabel methods */
static HRESULT ANXAPI Label_SetText(ITuiLabel *This, CONST CHAR8 *Text)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);

    if (Text == NULL) return E_POINTER;

    strncpy(impl->Text, Text, MAX_TEXT_LENGTH - 1);
    impl->Text[MAX_TEXT_LENGTH - 1] = '\0';

    return S_OK;
}

static HRESULT ANXAPI Label_GetText(ITuiLabel *This, CHAR8 *Buffer, UINTN BufferSize)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);

    if (Buffer == NULL) return E_POINTER;
    if (BufferSize == 0) return E_INVALIDARG;

    strncpy(Buffer, impl->Text, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';

    return S_OK;
}

static HRESULT ANXAPI Label_SetHotkey(ITuiLabel *This, CHAR8 Hotkey)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);
    impl->Hotkey = Hotkey;
    return S_OK;
}

static HRESULT ANXAPI Label_SetTextDirection(ITuiLabel *This, TUI_TEXT_DIRECTION Direction)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);
    impl->Direction = Direction;
    return S_OK;
}

static HRESULT ANXAPI Label_SetAlignment(ITuiLabel *This, INT32 Alignment)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);

    if (Alignment < 0 || Alignment > 2) return E_INVALIDARG;

    impl->Alignment = (TUI_TEXT_ALIGNMENT)Alignment;
    return S_OK;
}

static HRESULT ANXAPI Label_SetLinkedWidget(ITuiLabel *This, ITuiWidget *LinkedWidget)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);
    impl->LinkedWidget = LinkedWidget;
    return S_OK;
}

/* IUnknown methods */
static HRESULT ANXAPI Label_QueryInterface(ITuiLabel *This, REFIID riid, VOID **ppvObject)
{
    if (ppvObject == NULL) return E_POINTER;

    if (memcmp(riid, &IID_ITuiLabel, sizeof(GUID)) == 0) {
        *ppvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }

    if (memcmp(riid, &IID_ITuiThemedLabel, sizeof(GUID)) == 0) {
        *ppvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }

    if (memcmp(riid, &IID_ITuiThemedWidget, sizeof(GUID)) == 0) {
        *ppvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }

    if (memcmp(riid, &IID_ITuiWidget, sizeof(GUID)) == 0) {
        *ppvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }

    if (memcmp(riid, &IID_ITuiResponder, sizeof(GUID)) == 0) {
        *ppvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI Label_AddRef(ITuiLabel *This)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Label_Release(ITuiLabel *This)
{
    TuiLabelImpl *impl = LABEL_FROM_INTERFACE(This);
    UINTN refCount = --impl->State.RefCount;

    if (refCount == 0) {
        free(impl);
    }

    return refCount;
}

/* Vtable */
static CONST ITuiLabel_Vtbl LabelVtbl = {
    /* ITuiSerializable */
    Label_QueryInterface,
    Label_AddRef,
    Label_Release,
    Label_SerializeToYaml,
    Label_DeserializeFromYaml,
    Label_GetTypeName,
    Label_Clone,

    /* ITuiResponder */
    Label_GetNextResponder,
    Label_SetNextResponder,
    Label_AcceptsFirstResponder,
    Label_BecomeFirstResponder,
    Label_ResignFirstResponder,

    /* ITuiWidget */
    Label_SetBounds,
    Label_GetBounds,
    Label_SetVisible,
    Label_IsVisible,
    Label_SetEnabled,
    Label_IsEnabled,
    Label_SetParent,
    Label_GetParent,
    Label_AddChild,
    Label_RemoveChild,
    Label_SetNeedsDisplay,

    /* ITuiThemedWidget */
    Label_ApplyTheme,
    Label_GetColors,
    Label_SetColors,
    Label_OnMouseEvent,
    Label_OnKeyEvent,
    Label_Draw,

    /* ITuiThemedLabel */
    Label_GetHotkeyColor,
    Label_SetHotkeyColor,

    /* ITuiLabel */
    Label_SetText,
    Label_GetText,
    Label_SetHotkey,
    Label_SetTextDirection,
    Label_SetAlignment,
    Label_SetLinkedWidget
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateLabel(
    IN  ITuiWidget *Parent,
    IN  CONST CHAR8 *Text,
    OUT ITuiLabel **OutLabel
)
{
    TuiLabelImpl *impl;

    if (OutLabel == NULL) return E_POINTER;

    impl = (TuiLabelImpl *)calloc(1, sizeof(TuiLabelImpl));
    if (impl == NULL) {
        *OutLabel = NULL;
        return E_OUTOFMEMORY;
    }

    /* Initialize interface */
    impl->Interface.Vtbl = &LabelVtbl;

    /* Initialize state */
    InitWidgetState(&impl->State);

    /* Initialize label data */
    if (Text != NULL) {
        strncpy(impl->Text, Text, MAX_TEXT_LENGTH - 1);
        impl->Text[MAX_TEXT_LENGTH - 1] = '\0';
    } else {
        impl->Text[0] = '\0';
    }

    impl->Hotkey = '\0';
    impl->Alignment = TuiTextAlignmentLeft;
    impl->Direction = TuiTextDirectionLeftToRight;
    impl->HotkeyColor = TuiColorYellow; /* Default hotkey color */
    impl->LinkedWidget = NULL;

    /* Set parent if provided */
    if (Parent != NULL) {
        Parent->Vtbl->AddChild(Parent, (ITuiWidget*)&impl->Interface);
    }

    *OutLabel = &impl->Interface;
    return S_OK;
}
