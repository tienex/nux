/*
 * widget_splitview.c - Split View Widget
 *
 * Resizable two-pane container with draggable divider.
 * Supports horizontal and vertical orientation.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Split orientation */
typedef enum {
    SplitHorizontal,  /* Left/Right panes */
    SplitVertical     /* Top/Bottom panes */
} SplitOrientation;

typedef struct {
    ITuiSplitView Interface;
    WIDGET_STATE State;

    /* Child panes */
    VOID *Pane1;  /* Top or Left pane */
    VOID *Pane2;  /* Bottom or Right pane */

    /* Split configuration */
    SplitOrientation Orientation;
    UINT32 SplitPosition;     /* Position of divider */
    UINT32 MinPane1Size;      /* Minimum size for pane 1 */
    UINT32 MinPane2Size;      /* Minimum size for pane 2 */

    /* Divider */
    BOOLEAN ShowDivider;
    CHAR16 DividerChar;
    TUI_COLOR DividerFG;
    TUI_COLOR DividerBG;

    /* Resizing state */
    BOOLEAN Resizable;
    BOOLEAN IsDragging;
    INT32 DragStartPos;

} TuiSplitViewImpl;

/* IUnknown methods */
static HRESULT ANXAPI SplitView_QueryInterface(
    ITuiSplitView *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiSplitView)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI SplitView_AddRef(ITuiSplitView *This)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI SplitView_Release(ITuiSplitView *This)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        /* Release panes */
        if (impl->Pane1) {
            IUnknown *unk = (IUnknown *)impl->Pane1;
            unk->Vtbl->Release(unk);
        }
        if (impl->Pane2) {
            IUnknown *unk = (IUnknown *)impl->Pane2;
            unk->Vtbl->Release(unk);
        }
        free(impl);
    }

    return count;
}

/* Render the split view */
static HRESULT ANXAPI SplitView_Render(
    ITuiSplitView *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;

    if (!impl->State.Visible) return S_OK;

    UINT32 width = impl->State.Bounds.Width;
    UINT32 height = impl->State.Bounds.Height;

    if (impl->Orientation == SplitHorizontal) {
        /* Horizontal split (left/right) */
        UINT32 dividerWidth = impl->ShowDivider ? 1 : 0;

        /* Ensure split position is within bounds */
        if (impl->SplitPosition < impl->MinPane1Size) {
            impl->SplitPosition = impl->MinPane1Size;
        }
        if (impl->SplitPosition > width - impl->MinPane2Size - dividerWidth) {
            impl->SplitPosition = width - impl->MinPane2Size - dividerWidth;
        }

        /* Calculate pane sizes */
        UINT32 pane1Width = impl->SplitPosition;
        UINT32 pane2Width = width - impl->SplitPosition - dividerWidth;

        /* Render pane 1 (left) */
        if (impl->Pane1) {
            TUI_RECT pane1Bounds = {
                impl->State.Bounds.X,
                impl->State.Bounds.Y,
                pane1Width,
                height
            };

            ITuiButton *widget = (ITuiButton *)impl->Pane1;
            if (widget->Vtbl->SetBounds) {
                widget->Vtbl->SetBounds(widget, &pane1Bounds);
            }
            if (widget->Vtbl->Render) {
                widget->Vtbl->Render(widget, Screen, pane1Bounds.X, pane1Bounds.Y, FALSE);
            }
        }

        /* Render divider */
        if (impl->ShowDivider) {
            INT32 divX = impl->State.Bounds.X + impl->SplitPosition;
            for (UINT32 i = 0; i < height; i++) {
                Screen->Vtbl->WriteChar(Screen, divX, impl->State.Bounds.Y + i,
                                       impl->DividerChar, impl->DividerFG, impl->DividerBG);
            }
        }

        /* Render pane 2 (right) */
        if (impl->Pane2) {
            TUI_RECT pane2Bounds = {
                impl->State.Bounds.X + impl->SplitPosition + dividerWidth,
                impl->State.Bounds.Y,
                pane2Width,
                height
            };

            ITuiButton *widget = (ITuiButton *)impl->Pane2;
            if (widget->Vtbl->SetBounds) {
                widget->Vtbl->SetBounds(widget, &pane2Bounds);
            }
            if (widget->Vtbl->Render) {
                widget->Vtbl->Render(widget, Screen, pane2Bounds.X, pane2Bounds.Y, FALSE);
            }
        }

    } else {
        /* Vertical split (top/bottom) */
        UINT32 dividerHeight = impl->ShowDivider ? 1 : 0;

        /* Ensure split position is within bounds */
        if (impl->SplitPosition < impl->MinPane1Size) {
            impl->SplitPosition = impl->MinPane1Size;
        }
        if (impl->SplitPosition > height - impl->MinPane2Size - dividerHeight) {
            impl->SplitPosition = height - impl->MinPane2Size - dividerHeight;
        }

        /* Calculate pane sizes */
        UINT32 pane1Height = impl->SplitPosition;
        UINT32 pane2Height = height - impl->SplitPosition - dividerHeight;

        /* Render pane 1 (top) */
        if (impl->Pane1) {
            TUI_RECT pane1Bounds = {
                impl->State.Bounds.X,
                impl->State.Bounds.Y,
                width,
                pane1Height
            };

            ITuiButton *widget = (ITuiButton *)impl->Pane1;
            if (widget->Vtbl->SetBounds) {
                widget->Vtbl->SetBounds(widget, &pane1Bounds);
            }
            if (widget->Vtbl->Render) {
                widget->Vtbl->Render(widget, Screen, pane1Bounds.X, pane1Bounds.Y, FALSE);
            }
        }

        /* Render divider */
        if (impl->ShowDivider) {
            INT32 divY = impl->State.Bounds.Y + impl->SplitPosition;
            for (UINT32 i = 0; i < width; i++) {
                Screen->Vtbl->WriteChar(Screen, impl->State.Bounds.X + i, divY,
                                       impl->DividerChar, impl->DividerFG, impl->DividerBG);
            }
        }

        /* Render pane 2 (bottom) */
        if (impl->Pane2) {
            TUI_RECT pane2Bounds = {
                impl->State.Bounds.X,
                impl->State.Bounds.Y + impl->SplitPosition + dividerHeight,
                width,
                pane2Height
            };

            ITuiButton *widget = (ITuiButton *)impl->Pane2;
            if (widget->Vtbl->SetBounds) {
                widget->Vtbl->SetBounds(widget, &pane2Bounds);
            }
            if (widget->Vtbl->Render) {
                widget->Vtbl->Render(widget, Screen, pane2Bounds.X, pane2Bounds.Y, FALSE);
            }
        }
    }

    return S_OK;
}

/* Handle mouse input for resizing */
static HRESULT ANXAPI SplitView_HandleMouse(
    ITuiSplitView *This,
    CONST TUI_MOUSE_EVENT *Event
)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;

    if (!impl->Resizable) return S_OK;

    /* Check if mouse is on divider */
    BOOLEAN onDivider = FALSE;

    if (impl->Orientation == SplitHorizontal) {
        INT32 divX = impl->State.Bounds.X + impl->SplitPosition;
        if (Event->X == divX &&
            Event->Y >= impl->State.Bounds.Y &&
            Event->Y < impl->State.Bounds.Y + (INT32)impl->State.Bounds.Height) {
            onDivider = TRUE;
        }
    } else {
        INT32 divY = impl->State.Bounds.Y + impl->SplitPosition;
        if (Event->Y == divY &&
            Event->X >= impl->State.Bounds.X &&
            Event->X < impl->State.Bounds.X + (INT32)impl->State.Bounds.Width) {
            onDivider = TRUE;
        }
    }

    /* Handle drag */
    if (Event->Type == TuiMousePress && onDivider) {
        impl->IsDragging = TRUE;
        impl->DragStartPos = (impl->Orientation == SplitHorizontal) ? Event->X : Event->Y;
    } else if (Event->Type == TuiMouseRelease) {
        impl->IsDragging = FALSE;
    } else if (Event->Type == TuiMouseDrag && impl->IsDragging) {
        INT32 currentPos = (impl->Orientation == SplitHorizontal) ? Event->X : Event->Y;
        INT32 delta = currentPos - impl->DragStartPos;

        /* Update split position */
        INT32 newPos = (INT32)impl->SplitPosition + delta;
        if (newPos < (INT32)impl->MinPane1Size) {
            newPos = impl->MinPane1Size;
        }

        UINT32 maxSize = (impl->Orientation == SplitHorizontal) ?
                        impl->State.Bounds.Width : impl->State.Bounds.Height;
        if (newPos > (INT32)(maxSize - impl->MinPane2Size - 1)) {
            newPos = maxSize - impl->MinPane2Size - 1;
        }

        impl->SplitPosition = newPos;
        impl->DragStartPos = currentPos;
    }

    return S_OK;
}

/* Set pane 1 */
static HRESULT ANXAPI SplitView_SetPane1(
    ITuiSplitView *This,
    VOID *Widget
)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;

    /* Release old pane */
    if (impl->Pane1) {
        IUnknown *unk = (IUnknown *)impl->Pane1;
        unk->Vtbl->Release(unk);
    }

    /* Set new pane */
    impl->Pane1 = Widget;
    if (Widget) {
        IUnknown *unk = (IUnknown *)Widget;
        unk->Vtbl->AddRef(unk);
    }

    return S_OK;
}

/* Set pane 2 */
static HRESULT ANXAPI SplitView_SetPane2(
    ITuiSplitView *This,
    VOID *Widget
)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;

    /* Release old pane */
    if (impl->Pane2) {
        IUnknown *unk = (IUnknown *)impl->Pane2;
        unk->Vtbl->Release(unk);
    }

    /* Set new pane */
    impl->Pane2 = Widget;
    if (Widget) {
        IUnknown *unk = (IUnknown *)Widget;
        unk->Vtbl->AddRef(unk);
    }

    return S_OK;
}

/* Set split position */
static HRESULT ANXAPI SplitView_SetSplitPosition(
    ITuiSplitView *This,
    UINT32 Position
)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;
    impl->SplitPosition = Position;
    return S_OK;
}

/* Set orientation */
static HRESULT ANXAPI SplitView_SetOrientation(
    ITuiSplitView *This,
    UINT32 Orientation
)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;
    impl->Orientation = (SplitOrientation)Orientation;
    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI SplitView_SetBounds(ITuiSplitView *This, CONST TUI_RECT *Bounds)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI SplitView_GetBounds(ITuiSplitView *This, TUI_RECT *Bounds)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI SplitView_SetVisible(ITuiSplitView *This, BOOLEAN Visible)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI SplitView_IsVisible(ITuiSplitView *This)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI SplitView_SetEnabled(ITuiSplitView *This, BOOLEAN Enabled)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI SplitView_IsEnabled(ITuiSplitView *This)
{
    TuiSplitViewImpl *impl = (TuiSplitViewImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiSplitViewVtbl SplitViewVtbl = {
    SplitView_QueryInterface,
    SplitView_AddRef,
    SplitView_Release,
    SplitView_Render,
    SplitView_HandleMouse,
    SplitView_SetBounds,
    SplitView_GetBounds,
    SplitView_SetVisible,
    SplitView_IsVisible,
    SplitView_SetEnabled,
    SplitView_IsEnabled,
    SplitView_SetPane1,
    SplitView_SetPane2,
    SplitView_SetSplitPosition,
    SplitView_SetOrientation
};

/* Factory function */
HRESULT AnxTuiCreateSplitView(
    UINT32 Orientation,  /* 0=Horizontal, 1=Vertical */
    UINT32 InitialPosition,
    ITuiSplitView **OutSplitView
)
{
    TuiSplitViewImpl *impl;

    if (!OutSplitView) return E_INVALIDARG;

    impl = (TuiSplitViewImpl *)malloc(sizeof(TuiSplitViewImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiSplitViewImpl));
    impl->Interface.Vtbl = &SplitViewVtbl;
    InitWidgetState(&impl->State);

    impl->Orientation = (SplitOrientation)Orientation;
    impl->SplitPosition = InitialPosition;
    impl->MinPane1Size = 1;
    impl->MinPane2Size = 1;

    /* Divider configuration */
    impl->ShowDivider = TRUE;
    impl->DividerChar = (Orientation == 0) ? gBoxChars.SingleVertical : gBoxChars.SingleHorizontal;
    impl->DividerFG = TuiColorBrightBlack;
    impl->DividerBG = TuiColorBlack;

    impl->Resizable = TRUE;
    impl->IsDragging = FALSE;

    impl->State.Bounds.Width = 80;
    impl->State.Bounds.Height = 24;

    *OutSplitView = &impl->Interface;
    return S_OK;
}
