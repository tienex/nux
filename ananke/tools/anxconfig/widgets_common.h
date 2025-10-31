/*
 * Common Widget Definitions and Helpers
 *
 * Shared structures and utilities for TUI widget implementations.
 */

#ifndef __WIDGETS_COMMON_H__
#define __WIDGETS_COMMON_H__

#include <ananke/tui.h>
#include <ananke/base.h>

/* Common widget state */
typedef struct {
    UINTN RefCount;
    BOOLEAN Visible;
    BOOLEAN Enabled;
    BOOLEAN Focused;
    TUI_RECT Bounds;
    TUI_COLOR ForegroundColor;
    TUI_COLOR BackgroundColor;
} WIDGET_STATE;

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
