/*
 * GroupBox Widget Implementation
 *
 * Container widget with optional border and title.
 * Uses new event dispatching architecture with ITuiWidget, ITuiDrawListener.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

#define MAX_CHILDREN 64

typedef struct {
    VOID *Widget;
    INT32 X;
    INT32 Y;
} ChildWidget;

typedef struct {
    ITuiWidget WidgetInterface;
    ITuiDrawListener DrawListener;

    WIDGET_STATE State;
    CHAR8 Title[256];
    TUI_BORDER_STYLE BorderStyle;
    ChildWidget Children[MAX_CHILDREN];
    UINT32 ChildCount;
    UINT32 PaddingTop;
    UINT32 PaddingRight;
    UINT32 PaddingBottom;
    UINT32 PaddingLeft;
    ITuiResponder *NextResponder;
    ITuiSurface *Surface;
} TuiGroupBoxImpl;

/* Helper macros for interface conversions */
#define GROUPBOX_FROM_WIDGET(w) ((TuiGroupBoxImpl*)((UINT8*)(w) - offsetof(TuiGroupBoxImpl, WidgetInterface)))
#define GROUPBOX_FROM_DRAW(d) ((TuiGroupBoxImpl*)((UINT8*)(d) - offsetof(TuiGroupBoxImpl, DrawListener)))

/*=============================================================================
 * ITuiWidget Implementation
 *===========================================================================*/

static HRESULT ANXAPI GroupBoxWidget_QueryInterface(ITuiWidget *This, REFIID riid, VOID **ppvObject)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
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

static UINTN ANXAPI GroupBoxWidget_AddRef(ITuiWidget *This)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI GroupBoxWidget_Release(ITuiWidget *This)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        if (impl->Surface) impl->Surface->Vtbl->Release(impl->Surface);
        if (impl->NextResponder) impl->NextResponder->Vtbl->Release(impl->NextResponder);
        free(impl);
    }
    return refCount;
}

static HRESULT ANXAPI GroupBoxWidget_SetBounds(ITuiWidget *This, CONST TUI_RECT *Bounds)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_GetBounds(ITuiWidget *This, TUI_RECT *Bounds)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_SetVisible(ITuiWidget *This, BOOLEAN Visible)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    impl->State.Visible = Visible;
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_GetVisible(ITuiWidget *This, BOOLEAN *Visible)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    if (!Visible) return E_INVALIDARG;
    *Visible = impl->State.Visible;
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_SetEnabled(ITuiWidget *This, BOOLEAN Enabled)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    impl->State.Enabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_GetEnabled(ITuiWidget *This, BOOLEAN *Enabled)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    if (!Enabled) return E_INVALIDARG;
    *Enabled = impl->State.Enabled;
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_SetParent(ITuiWidget *This, ITuiWidget *Parent)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
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

static HRESULT ANXAPI GroupBoxWidget_GetParent(ITuiWidget *This, ITuiWidget **Parent)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    if (!Parent) return E_INVALIDARG;
    if (impl->NextResponder) {
        return impl->NextResponder->Vtbl->QueryInterface(impl->NextResponder,
                                                         &IID_ITuiWidget,
                                                         (VOID **)Parent);
    }
    *Parent = NULL;
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_Invalidate(ITuiWidget *This, CONST TUI_RECT *Rect)
{
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_GetNextResponder(ITuiWidget *This, ITuiResponder **NextResponder)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    if (!NextResponder) return E_INVALIDARG;
    *NextResponder = impl->NextResponder;
    if (impl->NextResponder) impl->NextResponder->Vtbl->AddRef(impl->NextResponder);
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_BecomeFirstResponder(ITuiWidget *This)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    impl->State.Focused = TRUE;
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_ResignFirstResponder(ITuiWidget *This)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    impl->State.Focused = FALSE;
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_SerializeToYaml(ITuiWidget *This, CHAR8 **OutYaml, UINTN *OutLength)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    CHAR8 *yaml = (CHAR8 *)malloc(2048);
    if (!yaml) return E_OUTOFMEMORY;

    snprintf(yaml, 2048,
        "type: GroupBox\ntitle: \"%s\"\nbounds:\n  x: %d\n  y: %d\n  width: %d\n  height: %d\n"
        "visible: %s\nenabled: %s\nborder_style: %d\npadding: [%u, %u, %u, %u]\n",
        impl->Title,
        impl->State.Bounds.X, impl->State.Bounds.Y, impl->State.Bounds.Width, impl->State.Bounds.Height,
        impl->State.Visible ? "true" : "false", impl->State.Enabled ? "true" : "false",
        impl->BorderStyle,
        impl->PaddingTop, impl->PaddingRight, impl->PaddingBottom, impl->PaddingLeft);

    *OutYaml = yaml;
    *OutLength = strlen(yaml);
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_DeserializeFromYaml(ITuiWidget *This, CONST CHAR8 *Yaml, UINTN Length)
{
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_GetTypeName(ITuiWidget *This, CONST CHAR8 **OutTypeName)
{
    *OutTypeName = "GroupBox";
    return S_OK;
}

static HRESULT ANXAPI GroupBoxWidget_Clone(ITuiWidget *This, ITuiSerializable **OutClone)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(This);
    ITuiWidget *newGroupBox = NULL;
    HRESULT hr = AnxTuiCreateGroupBox(impl->Title, &newGroupBox);
    if (FAILED(hr)) return hr;

    AnxTuiGroupBoxSetBorderStyle(newGroupBox, impl->BorderStyle);
    AnxTuiGroupBoxSetPadding(newGroupBox, impl->PaddingTop, impl->PaddingRight,
                              impl->PaddingBottom, impl->PaddingLeft);

    *OutClone = (ITuiSerializable *)newGroupBox;
    return S_OK;
}

static ITuiWidget_Vtbl GroupBoxWidgetVtbl = {
    GroupBoxWidget_QueryInterface, GroupBoxWidget_AddRef, GroupBoxWidget_Release,
    GroupBoxWidget_SetBounds, GroupBoxWidget_GetBounds, GroupBoxWidget_SetVisible, GroupBoxWidget_GetVisible,
    GroupBoxWidget_SetEnabled, GroupBoxWidget_GetEnabled, GroupBoxWidget_SetParent, GroupBoxWidget_GetParent,
    GroupBoxWidget_Invalidate, GroupBoxWidget_GetNextResponder, GroupBoxWidget_BecomeFirstResponder,
    GroupBoxWidget_ResignFirstResponder, GroupBoxWidget_SerializeToYaml, GroupBoxWidget_DeserializeFromYaml,
    GroupBoxWidget_GetTypeName, GroupBoxWidget_Clone
};

/*=============================================================================
 * ITuiDrawListener Implementation
 *===========================================================================*/

static HRESULT ANXAPI GroupBoxDraw_QueryInterface(ITuiDrawListener *This, REFIID riid, VOID **ppvObject)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_DRAW(This);
    return GroupBoxWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI GroupBoxDraw_AddRef(ITuiDrawListener *This)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_DRAW(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI GroupBoxDraw_Release(ITuiDrawListener *This)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_DRAW(This);
    return GroupBoxWidget_Release(&impl->WidgetInterface);
}

/* Helper: Draw border with different styles */
static VOID DrawBorder(
    ITuiSurface *Surface,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    TUI_BORDER_STYLE Style,
    CONST CHAR8 *Title,
    TUI_COLOR Fg,
    TUI_COLOR Bg
)
{
    CHAR8 line[256];
    UINT32 i;

    /* Border characters based on style */
    CONST CHAR8 *topLeft, *topRight, *bottomLeft, *bottomRight;
    CONST CHAR8 *horizontal, *vertical;

    switch (Style) {
        case TuiBorderSingle:
            topLeft = "┌"; topRight = "┐";
            bottomLeft = "└"; bottomRight = "┘";
            horizontal = "─"; vertical = "│";
            break;
        case TuiBorderDouble:
            topLeft = "╔"; topRight = "╗";
            bottomLeft = "╚"; bottomRight = "╝";
            horizontal = "═"; vertical = "║";
            break;
        case TuiBorderRounded:
            topLeft = "╭"; topRight = "╮";
            bottomLeft = "╰"; bottomRight = "╯";
            horizontal = "─"; vertical = "│";
            break;
        case TuiBorderAscii:
            topLeft = "+"; topRight = "+";
            bottomLeft = "+"; bottomRight = "+";
            horizontal = "-"; vertical = "|";
            break;
        default:
            topLeft = "+"; topRight = "+";
            bottomLeft = "+"; bottomRight = "+";
            horizontal = "-"; vertical = "|";
            break;
    }

    /* Top border */
    Surface->Vtbl->WriteText(Surface, X, Y, topLeft, Fg, Bg);

    /* Title in top border */
    if (Title != NULL && Title[0] != '\0') {
        UINT32 titleLen = strlen(Title);
        UINT32 titleStart = 2;

        snprintf(line, sizeof(line), " %s ", Title);
        Surface->Vtbl->WriteText(Surface, X + titleStart, Y, line, Fg, Bg);

        /* Fill remaining top border */
        for (i = titleStart + titleLen + 2; i < Width - 1; i++) {
            Surface->Vtbl->WriteText(Surface, X + i, Y, horizontal, Fg, Bg);
        }
    } else {
        /* No title, just horizontal line */
        for (i = 1; i < Width - 1; i++) {
            Surface->Vtbl->WriteText(Surface, X + i, Y, horizontal, Fg, Bg);
        }
    }

    Surface->Vtbl->WriteText(Surface, X + Width - 1, Y, topRight, Fg, Bg);

    /* Side borders */
    for (i = 1; i < Height - 1; i++) {
        Surface->Vtbl->WriteText(Surface, X, Y + i, vertical, Fg, Bg);
        Surface->Vtbl->WriteText(Surface, X + Width - 1, Y + i, vertical, Fg, Bg);
    }

    /* Bottom border */
    Surface->Vtbl->WriteText(Surface, X, Y + Height - 1, bottomLeft, Fg, Bg);
    for (i = 1; i < Width - 1; i++) {
        Surface->Vtbl->WriteText(Surface, X + i, Y + Height - 1, horizontal, Fg, Bg);
    }
    Surface->Vtbl->WriteText(Surface, X + Width - 1, Y + Height - 1, bottomRight, Fg, Bg);
}

static HRESULT ANXAPI GroupBoxDraw_OnDraw(ITuiDrawListener *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_DRAW(This);
    TUI_COLOR fg, bg;
    UINT32 i, width, height;

    if (!impl->State.Visible) return S_OK;

    fg = impl->State.ForegroundColor;
    bg = impl->State.BackgroundColor;
    width = impl->State.Bounds.Width;
    height = impl->State.Bounds.Height;

    /* Draw border */
    if (impl->BorderStyle != TuiBorderNone) {
        DrawBorder(Surface, 0, 0, width, height, impl->BorderStyle, impl->Title, fg, bg);
    } else if (impl->Title[0] != '\0') {
        /* No border but show title */
        Surface->Vtbl->WriteText(Surface, 0, 0, impl->Title, fg, bg);
    }

    /* Children would be rendered by the container system */
    /* The groupbox provides the container structure, padding, and border */

    return S_OK;
}

static HRESULT ANXAPI GroupBoxDraw_OnGetPreferredSize(ITuiDrawListener *This, UINT32 *Width, UINT32 *Height)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_DRAW(This);

    /* Minimum size to show border and title */
    if (Width) {
        UINT32 minWidth = strlen(impl->Title) + 6;  /* Title + padding + border */
        *Width = minWidth < 10 ? 10 : minWidth;
    }
    if (Height) *Height = 5;  /* Minimum height for border */

    return S_OK;
}

static ITuiDrawListener_Vtbl GroupBoxDrawVtbl = {
    GroupBoxDraw_QueryInterface, GroupBoxDraw_AddRef, GroupBoxDraw_Release,
    GroupBoxDraw_OnDraw, GroupBoxDraw_OnGetPreferredSize
};

/*=============================================================================
 * Factory Function
 *===========================================================================*/

HRESULT ANXAPI AnxTuiCreateGroupBox(IN CONST CHAR8 *Title, OUT ITuiWidget **Widget)
{
    TuiGroupBoxImpl *impl;

    if (Widget == NULL) return E_POINTER;

    impl = (TuiGroupBoxImpl *)calloc(1, sizeof(TuiGroupBoxImpl));
    if (impl == NULL) {
        *Widget = NULL;
        return E_OUTOFMEMORY;
    }

    /* Initialize vtables */
    impl->WidgetInterface.Vtbl = &GroupBoxWidgetVtbl;
    impl->DrawListener.Vtbl = &GroupBoxDrawVtbl;

    /* Initialize widget state */
    InitWidgetState(&impl->State);

    /* Initialize groupbox-specific state */
    if (Title != NULL) {
        strncpy(impl->Title, Title, sizeof(impl->Title) - 1);
        impl->Title[sizeof(impl->Title) - 1] = '\0';
    } else {
        impl->Title[0] = '\0';
    }

    impl->BorderStyle = TuiBorderSingle;
    impl->ChildCount = 0;
    impl->PaddingTop = 1;
    impl->PaddingRight = 1;
    impl->PaddingBottom = 1;
    impl->PaddingLeft = 1;
    impl->NextResponder = NULL;
    impl->Surface = NULL;

    *Widget = &impl->WidgetInterface;
    return S_OK;
}

/* Convenience functions for groupbox-specific operations */
HRESULT ANXAPI AnxTuiGroupBoxSetTitle(ITuiWidget *Widget, CONST CHAR8 *Title)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(Widget);
    if (Title == NULL) return E_POINTER;

    strncpy(impl->Title, Title, sizeof(impl->Title) - 1);
    impl->Title[sizeof(impl->Title) - 1] = '\0';
    return S_OK;
}

HRESULT ANXAPI AnxTuiGroupBoxGetTitle(ITuiWidget *Widget, CHAR8 *Buffer, UINTN BufferSize)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(Widget);
    if (Buffer == NULL) return E_POINTER;

    strncpy(Buffer, impl->Title, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

HRESULT ANXAPI AnxTuiGroupBoxSetBorderStyle(ITuiWidget *Widget, TUI_BORDER_STYLE Style)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(Widget);
    impl->BorderStyle = Style;
    return S_OK;
}

HRESULT ANXAPI AnxTuiGroupBoxAddChild(ITuiWidget *Widget, VOID *ChildWidget, INT32 X, INT32 Y)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(Widget);

    if (ChildWidget == NULL) return E_POINTER;
    if (impl->ChildCount >= MAX_CHILDREN) return E_OUTOFMEMORY;

    impl->Children[impl->ChildCount].Widget = ChildWidget;
    impl->Children[impl->ChildCount].X = X;
    impl->Children[impl->ChildCount].Y = Y;
    impl->ChildCount++;

    return S_OK;
}

HRESULT ANXAPI AnxTuiGroupBoxRemoveChild(ITuiWidget *Widget, VOID *ChildWidget)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(Widget);
    UINT32 i, j;

    if (ChildWidget == NULL) return E_POINTER;

    for (i = 0; i < impl->ChildCount; i++) {
        if (impl->Children[i].Widget == ChildWidget) {
            /* Shift remaining children */
            for (j = i; j < impl->ChildCount - 1; j++) {
                impl->Children[j] = impl->Children[j + 1];
            }
            impl->ChildCount--;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

HRESULT ANXAPI AnxTuiGroupBoxClearChildren(ITuiWidget *Widget)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(Widget);
    impl->ChildCount = 0;
    return S_OK;
}

HRESULT ANXAPI AnxTuiGroupBoxSetPadding(ITuiWidget *Widget, UINT32 Top, UINT32 Right, UINT32 Bottom, UINT32 Left)
{
    TuiGroupBoxImpl *impl = GROUPBOX_FROM_WIDGET(Widget);
    impl->PaddingTop = Top;
    impl->PaddingRight = Right;
    impl->PaddingBottom = Bottom;
    impl->PaddingLeft = Left;
    return S_OK;
}
