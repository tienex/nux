/*
 * Common Widget Definitions and Helpers
 *
 * Shared structures and utilities for TUI widget implementations.
 */

#ifndef __WIDGETS_COMMON_H__
#define __WIDGETS_COMMON_H__

#include <ananke/tui.h>
#include <ananke/base.h>
#include <stdlib.h>
#include <stddef.h>

/* Forward declaration */
struct _WIDGET_BASE;

/* Common widget state */
typedef struct {
    UINTN RefCount;
    BOOLEAN Visible;
    BOOLEAN Enabled;
    BOOLEAN Focused;
    TUI_RECT Bounds;
    TUI_COLOR ForegroundColor;
    TUI_COLOR BackgroundColor;

    /* Ownership tree */
    struct _WIDGET_BASE *Parent;
    struct _WIDGET_BASE **Children;
    UINT32 ChildCount;
    UINT32 ChildCapacity;

    /* Clipping and rendering */
    BOOLEAN ClipChildren;        /* TRUE = clip children to parent bounds (default) */
    BOOLEAN AllowOverflow;       /* TRUE = this widget can overflow parent (default FALSE) */
    INT32 ZOrder;                /* Higher values render on top */
    TUI_RECT ClipRect;          /* Current clipping rectangle (computed) */

    /* Destruction callback */
    VOID (*OnDestroy)(VOID *Widget);
} WIDGET_STATE;

/* Base widget structure for ownership tracking */
typedef struct _WIDGET_BASE {
    VOID *Vtbl;                  /* Pointer to actual vtable */
    WIDGET_STATE State;
} WIDGET_BASE;

/* Initialize common widget state */
static inline VOID InitWidgetState(WIDGET_STATE *State)
{
    State->RefCount = 1;
    State->Visible = TRUE;
    State->Enabled = TRUE;
    State->Focused = FALSE;
    State->Bounds.X = 0;
    State->Bounds.Y = 0;
    State->Bounds.Width = 0;
    State->Bounds.Height = 0;
    State->ForegroundColor = TuiColorWhite;
    State->BackgroundColor = TuiColorBlack;

    /* Ownership tree */
    State->Parent = NULL;
    State->Children = NULL;
    State->ChildCount = 0;
    State->ChildCapacity = 0;

    /* Clipping (default: clip children, don't overflow parent) */
    State->ClipChildren = TRUE;
    State->AllowOverflow = FALSE;
    State->ZOrder = 0;
    State->ClipRect.X = 0;
    State->ClipRect.Y = 0;
    State->ClipRect.Width = 0;
    State->ClipRect.Height = 0;

    State->OnDestroy = NULL;
}

/* Add child to widget */
static inline HRESULT AddChildWidget(WIDGET_STATE *Parent, WIDGET_BASE *Child)
{
    if (!Parent || !Child) return E_INVALIDARG;

    /* Ensure capacity */
    if (Parent->ChildCount >= Parent->ChildCapacity) {
        UINT32 newCapacity = Parent->ChildCapacity == 0 ? 4 : Parent->ChildCapacity * 2;
        WIDGET_BASE **newChildren = (WIDGET_BASE **)realloc(Parent->Children,
            newCapacity * sizeof(WIDGET_BASE *));
        if (!newChildren) return E_OUTOFMEMORY;

        Parent->Children = newChildren;
        Parent->ChildCapacity = newCapacity;
    }

    /* Add child */
    Parent->Children[Parent->ChildCount++] = Child;
    Child->State.Parent = (WIDGET_BASE *)((char *)Parent - offsetof(WIDGET_BASE, State));

    return S_OK;
}

/* Remove child from widget */
static inline HRESULT RemoveChildWidget(WIDGET_STATE *Parent, WIDGET_BASE *Child)
{
    if (!Parent || !Child) return E_INVALIDARG;

    for (UINT32 i = 0; i < Parent->ChildCount; i++) {
        if (Parent->Children[i] == Child) {
            /* Shift remaining children */
            for (UINT32 j = i; j < Parent->ChildCount - 1; j++) {
                Parent->Children[j] = Parent->Children[j + 1];
            }
            Parent->ChildCount--;
            Child->State.Parent = NULL;
            return S_OK;
        }
    }

    return E_NOTFOUND;
}

/* Compute clip rectangle for widget based on parent chain */
static inline VOID ComputeClipRect(WIDGET_STATE *State)
{
    if (!State) return;

    /* Start with widget's own bounds */
    State->ClipRect = State->Bounds;

    /* If we don't allow overflow and have a parent, intersect with parent's clip rect */
    if (!State->AllowOverflow && State->Parent) {
        WIDGET_STATE *parentState = &State->Parent->State;

        /* Compute intersection */
        INT32 left = State->ClipRect.X > parentState->ClipRect.X ?
            State->ClipRect.X : parentState->ClipRect.X;
        INT32 top = State->ClipRect.Y > parentState->ClipRect.Y ?
            State->ClipRect.Y : parentState->ClipRect.Y;
        INT32 right = (State->ClipRect.X + State->ClipRect.Width) <
            (parentState->ClipRect.X + parentState->ClipRect.Width) ?
            (State->ClipRect.X + State->ClipRect.Width) :
            (parentState->ClipRect.X + parentState->ClipRect.Width);
        INT32 bottom = (State->ClipRect.Y + State->ClipRect.Height) <
            (parentState->ClipRect.Y + parentState->ClipRect.Height) ?
            (State->ClipRect.Y + State->ClipRect.Height) :
            (parentState->ClipRect.Y + parentState->ClipRect.Height);

        State->ClipRect.X = left;
        State->ClipRect.Y = top;
        State->ClipRect.Width = (right > left) ? (right - left) : 0;
        State->ClipRect.Height = (bottom > top) ? (bottom - top) : 0;
    }
}

/* Destroy widget and all children recursively */
static inline VOID DestroyWidgetTree(WIDGET_BASE *Widget)
{
    if (!Widget) return;

    WIDGET_STATE *state = &Widget->State;

    /* Destroy all children first */
    for (UINT32 i = 0; i < state->ChildCount; i++) {
        DestroyWidgetTree(state->Children[i]);
    }

    /* Free children array */
    if (state->Children) {
        free(state->Children);
        state->Children = NULL;
        state->ChildCount = 0;
        state->ChildCapacity = 0;
    }

    /* Call custom destroy handler if present */
    if (state->OnDestroy) {
        state->OnDestroy(Widget);
    }

    /* Note: Don't free Widget itself here - that's handled by Release() */
}

/* Check if rendering should be clipped */
static inline BOOLEAN ShouldClipRendering(
    CONST WIDGET_STATE *State,
    INT32 X,
    INT32 Y
)
{
    if (!State) return FALSE;

    /* Check if point is within clip rectangle */
    return (X >= State->ClipRect.X &&
            X < State->ClipRect.X + State->ClipRect.Width &&
            Y >= State->ClipRect.Y &&
            Y < State->ClipRect.Y + State->ClipRect.Height);
}

/* Compare function for Z-order sorting */
static inline int CompareZOrder(const void *a, const void *b)
{
    WIDGET_BASE *wa = *(WIDGET_BASE **)a;
    WIDGET_BASE *wb = *(WIDGET_BASE **)b;
    return wa->State.ZOrder - wb->State.ZOrder;
}

/* Sort children by Z-order (low to high, so higher values render last/on top) */
static inline VOID SortChildrenByZOrder(WIDGET_STATE *State)
{
    if (!State || State->ChildCount <= 1) return;

    qsort(State->Children, State->ChildCount, sizeof(WIDGET_BASE *), CompareZOrder);
}

/* Update clip rectangles for entire widget tree */
static inline VOID UpdateClipRectsRecursive(WIDGET_BASE *Widget)
{
    if (!Widget) return;

    /* Compute this widget's clip rect */
    ComputeClipRect(&Widget->State);

    /* Recursively update children */
    for (UINT32 i = 0; i < Widget->State.ChildCount; i++) {
        UpdateClipRectsRecursive(Widget->State.Children[i]);
    }
}

/* Check if point is inside widget bounds */
static inline BOOLEAN IsPointInWidget(CONST WIDGET_STATE *State, INT32 X, INT32 Y)
{
    return (X >= State->Bounds.X &&
            X < State->Bounds.X + State->Bounds.Width &&
            Y >= State->Bounds.Y &&
            Y < State->Bounds.Y + State->Bounds.Height);
}

/* Draw a single-line box */
static inline VOID DrawBoxSingle(
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    TUI_COLOR Fg,
    TUI_COLOR Bg
)
{
    UINT32 i;
    CHAR8 line[256];

    if (Width > sizeof(line) - 1) Width = sizeof(line) - 1;

    /* Top border */
    line[0] = '+';
    for (i = 1; i < Width - 1; i++) line[i] = '-';
    line[Width - 1] = '+';
    line[Width] = '\0';
    Screen->Vtbl->WriteText(Screen, X, Y, line, Fg, Bg);

    /* Sides */
    for (i = 1; i < Height - 1; i++) {
        Screen->Vtbl->WriteText(Screen, X, Y + i, "|", Fg, Bg);
        Screen->Vtbl->WriteText(Screen, X + Width - 1, Y + i, "|", Fg, Bg);
    }

    /* Bottom border */
    Screen->Vtbl->WriteText(Screen, X, Y + Height - 1, line, Fg, Bg);
}

/* Clear rectangle */
static inline VOID ClearRect(
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    TUI_COLOR Bg
)
{
    UINT32 i, j;
    CHAR8 spaces[256];

    if (Width > sizeof(spaces) - 1) Width = sizeof(spaces) - 1;
    for (i = 0; i < Width; i++) spaces[i] = ' ';
    spaces[Width] = '\0';

    for (j = 0; j < Height; j++) {
        Screen->Vtbl->WriteText(Screen, X, Y + j, spaces, TuiColorWhite, Bg);
    }
}

#endif /* __WIDGETS_COMMON_H__ */
