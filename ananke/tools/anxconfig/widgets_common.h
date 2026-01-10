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

/* Box drawing characters - CP437/850 compatible fallback */
typedef struct {
    /* Single line box */
    CHAR8 SingleHorizontal;      /* ─ or 0xC4 */
    CHAR8 SingleVertical;        /* │ or 0xB3 */
    CHAR8 SingleTopLeft;         /* ┌ or 0xDA */
    CHAR8 SingleTopRight;        /* ┐ or 0xBF */
    CHAR8 SingleBottomLeft;      /* └ or 0xC0 */
    CHAR8 SingleBottomRight;     /* ┘ or 0xD9 */
    CHAR8 SingleCross;           /* ┼ or 0xC5 */
    CHAR8 SingleTeeLeft;         /* ├ or 0xC3 */
    CHAR8 SingleTeeRight;        /* ┤ or 0xB4 */
    CHAR8 SingleTeeTop;          /* ┬ or 0xC2 */
    CHAR8 SingleTeeBottom;       /* ┴ or 0xC1 */

    /* Double line box */
    CHAR8 DoubleHorizontal;      /* ═ or 0xCD */
    CHAR8 DoubleVertical;        /* ║ or 0xBA */
    CHAR8 DoubleTopLeft;         /* ╔ or 0xC9 */
    CHAR8 DoubleTopRight;        /* ╗ or 0xBB */
    CHAR8 DoubleBottomLeft;      /* ╚ or 0xC8 */
    CHAR8 DoubleBottomRight;     /* ╝ or 0xBC */
    CHAR8 DoubleCross;           /* ╬ or 0xCE */

    /* Shading */
    CHAR8 ShadeLight;            /* ░ or 0xB0 */
    CHAR8 ShadeMedium;           /* ▒ or 0xB1 */
    CHAR8 ShadeDark;             /* ▓ or 0xB2 */
    CHAR8 Block;                 /* █ or 0xDB */

    /* Arrows */
    CHAR8 ArrowUp;               /* ▲ or 0x1E */
    CHAR8 ArrowDown;             /* ▼ or 0x1F */
    CHAR8 ArrowLeft;             /* ◄ or 0x11 */
    CHAR8 ArrowRight;            /* ► or 0x10 */

    /* Additional glyphs from CP437/850 */
    CHAR8 CheckMark;             /* √ or 0xFB */
    CHAR8 Bullet;                /* • or 0x07 or 0xF9 */
    CHAR8 BulletHollow;          /* ○ or 0x09 */
    CHAR8 Diamond;               /* ◆ or 0x04 */
    CHAR8 Club;                  /* ♣ or 0x05 */
    CHAR8 Heart;                 /* ♥ or 0x03 */
    CHAR8 Spade;                 /* ♠ or 0x06 */
    CHAR8 Male;                  /* ♂ or 0x0B */
    CHAR8 Female;                /* ♀ or 0x0C */
    CHAR8 Note;                  /* ♪ or 0x0D */
    CHAR8 NoteDouble;            /* ♫ or 0x0E */
    CHAR8 Sun;                   /* ☼ or 0x0F */
    CHAR8 PointerRight;          /* ► or 0x10 (triangle) */
    CHAR8 PointerLeft;           /* ◄ or 0x11 (triangle) */
    CHAR8 UpDownArrow;           /* ↕ or 0x12 */
    CHAR8 DoubleExclamation;     /* ‼ or 0x13 */
    CHAR8 Paragraph;             /* ¶ or 0x14 */
    CHAR8 Section;               /* § or 0x15 */
    CHAR8 LeftRightArrow;        /* ↔ or 0x1D */
    CHAR8 PointerUp;             /* ▲ or 0x1E (solid) */
    CHAR8 PointerDown;           /* ▼ or 0x1F (solid) */
    CHAR8 House;                 /* ⌂ or 0x7F */
    CHAR8 VerticalBar;           /* │ or 0xB3 */
    CHAR8 PlusMinus;             /* ± or 0xF1 */
    CHAR8 GreaterEqual;          /* ≥ or 0xF2 */
    CHAR8 LessEqual;             /* ≤ or 0xF3 */
    CHAR8 Degree;                /* ° or 0xF8 */
    CHAR8 MiddleDot;             /* · or 0xFA */
    CHAR8 Squared;               /* ² or 0xFD */
    CHAR8 CenteredDot;           /* · or 0xFA */

    BOOLEAN IsUnicode;
} BoxChars;

/* Global box character set - will be initialized based on terminal capabilities */
static BoxChars gBoxChars = {
    /* CP437/850 fallback (default) */
    0xC4, 0xB3, 0xDA, 0xBF, 0xC0, 0xD9, 0xC5, 0xC3, 0xB4, 0xC2, 0xC1,  /* Single */
    0xCD, 0xBA, 0xC9, 0xBB, 0xC8, 0xBC, 0xCE,                          /* Double */
    0xB0, 0xB1, 0xB2, 0xDB,                                            /* Shading */
    0x1E, 0x1F, 0x11, 0x10,                                            /* Arrows */
    /* Additional glyphs */
    0xFB, 0x07, 0x09, 0x04, 0x05, 0x03, 0x06,                          /* CheckMark, Bullets, Suits */
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F,                                      /* Male, Female, Notes, Sun */
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x1D, 0x1E, 0x1F,             /* Pointers, Arrows, Symbols */
    0x7F, 0xB3, 0xF1, 0xF2, 0xF3, 0xF8, 0xFA, 0xFD, 0xFA,             /* House, Bar, Math, Dots */
    FALSE
};

/* Initialize box characters for Unicode mode */
static inline VOID InitBoxCharsUnicode(VOID)
{
    gBoxChars.SingleHorizontal = 0x2500;     /* ─ */
    gBoxChars.SingleVertical = 0x2502;       /* │ */
    gBoxChars.SingleTopLeft = 0x250C;        /* ┌ */
    gBoxChars.SingleTopRight = 0x2510;       /* ┐ */
    gBoxChars.SingleBottomLeft = 0x2514;     /* └ */
    gBoxChars.SingleBottomRight = 0x2518;    /* ┘ */
    gBoxChars.SingleCross = 0x253C;          /* ┼ */
    gBoxChars.SingleTeeLeft = 0x251C;        /* ├ */
    gBoxChars.SingleTeeRight = 0x2524;       /* ┤ */
    gBoxChars.SingleTeeTop = 0x252C;         /* ┬ */
    gBoxChars.SingleTeeBottom = 0x2534;      /* ┴ */

    gBoxChars.DoubleHorizontal = 0x2550;     /* ═ */
    gBoxChars.DoubleVertical = 0x2551;       /* ║ */
    gBoxChars.DoubleTopLeft = 0x2554;        /* ╔ */
    gBoxChars.DoubleTopRight = 0x2557;       /* ╗ */
    gBoxChars.DoubleBottomLeft = 0x255A;     /* ╚ */
    gBoxChars.DoubleBottomRight = 0x255D;    /* ╝ */
    gBoxChars.DoubleCross = 0x256C;          /* ╬ */

    gBoxChars.ShadeLight = 0x2591;           /* ░ */
    gBoxChars.ShadeMedium = 0x2592;          /* ▒ */
    gBoxChars.ShadeDark = 0x2593;            /* ▓ */
    gBoxChars.Block = 0x2588;                /* █ */

    gBoxChars.ArrowUp = 0x25B2;              /* ▲ */
    gBoxChars.ArrowDown = 0x25BC;            /* ▼ */
    gBoxChars.ArrowLeft = 0x25C4;            /* ◄ */
    gBoxChars.ArrowRight = 0x25BA;           /* ► */

    /* Additional Unicode glyphs */
    gBoxChars.CheckMark = 0x221A;            /* √ */
    gBoxChars.Bullet = 0x2022;               /* • */
    gBoxChars.BulletHollow = 0x25CB;         /* ○ */
    gBoxChars.Diamond = 0x25C6;              /* ◆ */
    gBoxChars.Club = 0x2663;                 /* ♣ */
    gBoxChars.Heart = 0x2665;                /* ♥ */
    gBoxChars.Spade = 0x2660;                /* ♠ */
    gBoxChars.Male = 0x2642;                 /* ♂ */
    gBoxChars.Female = 0x2640;               /* ♀ */
    gBoxChars.Note = 0x266A;                 /* ♪ */
    gBoxChars.NoteDouble = 0x266B;           /* ♫ */
    gBoxChars.Sun = 0x263C;                  /* ☼ */
    gBoxChars.PointerRight = 0x25BA;         /* ► */
    gBoxChars.PointerLeft = 0x25C4;          /* ◄ */
    gBoxChars.UpDownArrow = 0x2195;          /* ↕ */
    gBoxChars.DoubleExclamation = 0x203C;    /* ‼ */
    gBoxChars.Paragraph = 0x00B6;            /* ¶ */
    gBoxChars.Section = 0x00A7;              /* § */
    gBoxChars.LeftRightArrow = 0x2194;       /* ↔ */
    gBoxChars.PointerUp = 0x25B2;            /* ▲ */
    gBoxChars.PointerDown = 0x25BC;          /* ▼ */
    gBoxChars.House = 0x2302;                /* ⌂ */
    gBoxChars.VerticalBar = 0x2502;          /* │ */
    gBoxChars.PlusMinus = 0x00B1;            /* ± */
    gBoxChars.GreaterEqual = 0x2265;         /* ≥ */
    gBoxChars.LessEqual = 0x2264;            /* ≤ */
    gBoxChars.Degree = 0x00B0;               /* ° */
    gBoxChars.MiddleDot = 0x00B7;            /* · */
    gBoxChars.Squared = 0x00B2;              /* ² */
    gBoxChars.CenteredDot = 0x00B7;          /* · */

    gBoxChars.IsUnicode = TRUE;
}

/* Detect and initialize appropriate character set */
static inline VOID InitBoxChars(TUI_UNICODE_LEVEL UnicodeLevel)
{
    if (UnicodeLevel >= TuiUnicodeBasic) {
        InitBoxCharsUnicode();
    } else {
        /* Already initialized with CP437/850 values */
        gBoxChars.IsUnicode = FALSE;
    }
}

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

    /* Top border */
    Screen->Vtbl->WriteChar(Screen, X, Y, gBoxChars.SingleTopLeft, Fg, Bg);
    for (i = 1; i < Width - 1; i++) {
        Screen->Vtbl->WriteChar(Screen, X + i, Y, gBoxChars.SingleHorizontal, Fg, Bg);
    }
    Screen->Vtbl->WriteChar(Screen, X + Width - 1, Y, gBoxChars.SingleTopRight, Fg, Bg);

    /* Sides */
    for (i = 1; i < Height - 1; i++) {
        Screen->Vtbl->WriteChar(Screen, X, Y + i, gBoxChars.SingleVertical, Fg, Bg);
        Screen->Vtbl->WriteChar(Screen, X + Width - 1, Y + i, gBoxChars.SingleVertical, Fg, Bg);
    }

    /* Bottom border */
    Screen->Vtbl->WriteChar(Screen, X, Y + Height - 1, gBoxChars.SingleBottomLeft, Fg, Bg);
    for (i = 1; i < Width - 1; i++) {
        Screen->Vtbl->WriteChar(Screen, X + i, Y + Height - 1, gBoxChars.SingleHorizontal, Fg, Bg);
    }
    Screen->Vtbl->WriteChar(Screen, X + Width - 1, Y + Height - 1, gBoxChars.SingleBottomRight, Fg, Bg);
}

/* Draw a double-line box */
static inline VOID DrawBoxDouble(
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

    /* Top border */
    Screen->Vtbl->WriteChar(Screen, X, Y, gBoxChars.DoubleTopLeft, Fg, Bg);
    for (i = 1; i < Width - 1; i++) {
        Screen->Vtbl->WriteChar(Screen, X + i, Y, gBoxChars.DoubleHorizontal, Fg, Bg);
    }
    Screen->Vtbl->WriteChar(Screen, X + Width - 1, Y, gBoxChars.DoubleTopRight, Fg, Bg);

    /* Sides */
    for (i = 1; i < Height - 1; i++) {
        Screen->Vtbl->WriteChar(Screen, X, Y + i, gBoxChars.DoubleVertical, Fg, Bg);
        Screen->Vtbl->WriteChar(Screen, X + Width - 1, Y + i, gBoxChars.DoubleVertical, Fg, Bg);
    }

    /* Bottom border */
    Screen->Vtbl->WriteChar(Screen, X, Y + Height - 1, gBoxChars.DoubleBottomLeft, Fg, Bg);
    for (i = 1; i < Width - 1; i++) {
        Screen->Vtbl->WriteChar(Screen, X + i, Y + Height - 1, gBoxChars.DoubleHorizontal, Fg, Bg);
    }
    Screen->Vtbl->WriteChar(Screen, X + Width - 1, Y + Height - 1, gBoxChars.DoubleBottomRight, Fg, Bg);
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
