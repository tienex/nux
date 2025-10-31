/*
 * GroupBox Widget Implementation
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
    ITuiGroupBox Interface;
    WIDGET_STATE State;
    CHAR8 Title[256];
    TUI_BORDER_STYLE BorderStyle;
    ChildWidget Children[MAX_CHILDREN];
    UINT32 ChildCount;
    UINT32 PaddingTop;
    UINT32 PaddingRight;
    UINT32 PaddingBottom;
    UINT32 PaddingLeft;
} TuiGroupBoxImpl;

/* IUnknown methods */
static HRESULT ANXAPI GroupBox_QueryInterface(
    ITuiGroupBox *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI GroupBox_AddRef(ITuiGroupBox *This)
{
    TuiGroupBoxImpl *impl = (TuiGroupBoxImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI GroupBox_Release(ITuiGroupBox *This)
{
    TuiGroupBoxImpl *impl = (TuiGroupBoxImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiGroupBox methods */
static HRESULT ANXAPI GroupBox_SetTitle(
    ITuiGroupBox *This,
    CONST CHAR8 *Title
)
{
    TuiGroupBoxImpl *impl = (TuiGroupBoxImpl *)This;
    if (Title == NULL) return E_POINTER;

    strncpy(impl->Title, Title, sizeof(impl->Title) - 1);
    impl->Title[sizeof(impl->Title) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI GroupBox_GetTitle(
    ITuiGroupBox *This,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    TuiGroupBoxImpl *impl = (TuiGroupBoxImpl *)This;
    if (Buffer == NULL) return E_POINTER;

    strncpy(Buffer, impl->Title, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI GroupBox_SetBorderStyle(
    ITuiGroupBox *This,
    TUI_BORDER_STYLE Style
)
{
    TuiGroupBoxImpl *impl = (TuiGroupBoxImpl *)This;
    impl->BorderStyle = Style;
    return S_OK;
}

static HRESULT ANXAPI GroupBox_AddChild(
    ITuiGroupBox *This,
    VOID *Widget,
    INT32 X,
    INT32 Y
)
{
    TuiGroupBoxImpl *impl = (TuiGroupBoxImpl *)This;

    if (Widget == NULL) return E_POINTER;
    if (impl->ChildCount >= MAX_CHILDREN) return E_OUTOFMEMORY;

    impl->Children[impl->ChildCount].Widget = Widget;
    impl->Children[impl->ChildCount].X = X;
    impl->Children[impl->ChildCount].Y = Y;
    impl->ChildCount++;

    return S_OK;
}

static HRESULT ANXAPI GroupBox_RemoveChild(
    ITuiGroupBox *This,
    VOID *Widget
)
{
    TuiGroupBoxImpl *impl = (TuiGroupBoxImpl *)This;
    UINT32 i, j;

    if (Widget == NULL) return E_POINTER;

    for (i = 0; i < impl->ChildCount; i++) {
        if (impl->Children[i].Widget == Widget) {
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

static HRESULT ANXAPI GroupBox_ClearChildren(ITuiGroupBox *This)
{
    TuiGroupBoxImpl *impl = (TuiGroupBoxImpl *)This;
    impl->ChildCount = 0;
    return S_OK;
}

static HRESULT ANXAPI GroupBox_SetPadding(
    ITuiGroupBox *This,
    UINT32 Top,
    UINT32 Right,
    UINT32 Bottom,
    UINT32 Left
)
{
    TuiGroupBoxImpl *impl = (TuiGroupBoxImpl *)This;
    impl->PaddingTop = Top;
    impl->PaddingRight = Right;
    impl->PaddingBottom = Bottom;
    impl->PaddingLeft = Left;
    return S_OK;
}

/* Helper: Draw border with different styles */
static VOID DrawBorder(
    ITuiScreen *Screen,
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
            /* Simple single border */
            topLeft = "+"; topRight = "+";
            bottomLeft = "+"; bottomRight = "+";
            horizontal = "-"; vertical = "|";
            break;
    }

    /* Top border */
    Screen->Vtbl->WriteText(Screen, X, Y, topLeft, Fg, Bg);

    /* Title in top border */
    if (Title != NULL && Title[0] != '\0') {
        UINT32 titleLen = strlen(Title);
        UINT32 titleStart = 2;  /* Space after corner */

        snprintf(line, sizeof(line), " %s ", Title);
        Screen->Vtbl->WriteText(Screen, X + titleStart, Y, line, Fg, Bg);

        /* Fill remaining top border */
        for (i = titleStart + titleLen + 2; i < Width - 1; i++) {
            Screen->Vtbl->WriteText(Screen, X + i, Y, horizontal, Fg, Bg);
        }
    } else {
        /* No title, just horizontal line */
        for (i = 1; i < Width - 1; i++) {
            Screen->Vtbl->WriteText(Screen, X + i, Y, horizontal, Fg, Bg);
        }
    }

    Screen->Vtbl->WriteText(Screen, X + Width - 1, Y, topRight, Fg, Bg);

    /* Side borders */
    for (i = 1; i < Height - 1; i++) {
        Screen->Vtbl->WriteText(Screen, X, Y + i, vertical, Fg, Bg);
        Screen->Vtbl->WriteText(Screen, X + Width - 1, Y + i, vertical, Fg, Bg);
    }

    /* Bottom border */
    Screen->Vtbl->WriteText(Screen, X, Y + Height - 1, bottomLeft, Fg, Bg);
    for (i = 1; i < Width - 1; i++) {
        Screen->Vtbl->WriteText(Screen, X + i, Y + Height - 1, horizontal, Fg, Bg);
    }
    Screen->Vtbl->WriteText(Screen, X + Width - 1, Y + Height - 1, bottomRight, Fg, Bg);
}

static HRESULT ANXAPI GroupBox_Render(
    ITuiGroupBox *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height
)
{
    TuiGroupBoxImpl *impl = (TuiGroupBoxImpl *)This;
    TUI_COLOR fg, bg;
    UINT32 i;

    if (!impl->State.Visible) return S_OK;

    fg = impl->State.ForegroundColor;
    bg = impl->State.BackgroundColor;

    /* Draw border */
    if (impl->BorderStyle != TuiBorderNone) {
        DrawBorder(Screen, X, Y, Width, Height, impl->BorderStyle, impl->Title, fg, bg);
    } else if (impl->Title[0] != '\0') {
        /* No border but show title */
        Screen->Vtbl->WriteText(Screen, X, Y, impl->Title, fg, bg);
    }

    /* Render children (with padding offset) */
    INT32 contentX = X + impl->PaddingLeft + (impl->BorderStyle != TuiBorderNone ? 1 : 0);
    INT32 contentY = Y + impl->PaddingTop + (impl->BorderStyle != TuiBorderNone ? 1 : 0);

    for (i = 0; i < impl->ChildCount; i++) {
        /* Note: This is a simplified rendering - actual widgets would need
         * their own Render methods called through their vtables */
        /* For now, this serves as the container infrastructure */

        /* Each child widget would be rendered at:
         * contentX + Children[i].X, contentY + Children[i].Y */
    }

    return S_OK;
}

static HRESULT ANXAPI GroupBox_HandleInput(
    ITuiGroupBox *This,
    CONST TUI_INPUT_EVENT *Event,
    BOOLEAN *Handled
)
{
    TuiGroupBoxImpl *impl = (TuiGroupBoxImpl *)This;
    UINT32 i;

    /* Route input to children */
    for (i = 0; i < impl->ChildCount; i++) {
        /* Note: Actual implementation would call each child's HandleInput
         * through their vtables and check if they handled the event */
        /* This is infrastructure for event routing */
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiGroupBox_Vtbl GroupBoxVtbl = {
    GroupBox_QueryInterface,
    GroupBox_AddRef,
    GroupBox_Release,
    GroupBox_SetTitle,
    GroupBox_GetTitle,
    GroupBox_SetBorderStyle,
    GroupBox_AddChild,
    GroupBox_RemoveChild,
    GroupBox_ClearChildren,
    GroupBox_SetPadding,
    GroupBox_Render,
    GroupBox_HandleInput
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateGroupBox(
    IN  CONST CHAR8 *Title,
    OUT ITuiGroupBox **GroupBox
)
{
    TuiGroupBoxImpl *impl;

    if (GroupBox == NULL) return E_POINTER;

    impl = (TuiGroupBoxImpl *)calloc(1, sizeof(TuiGroupBoxImpl));
    if (impl == NULL) {
        *GroupBox = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &GroupBoxVtbl;
    InitWidgetState(&impl->State);

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

    *GroupBox = &impl->Interface;
    return S_OK;
}
