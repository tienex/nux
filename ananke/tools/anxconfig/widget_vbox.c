/*
 * widget_vbox.c - Vertical Box Container Widget
 *
 * Simple vertical stacking container that arranges children top-to-bottom.
 * Lighter weight alternative to flex container for common vertical layouts.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_VBOX_CHILDREN 64

/* Child properties */
typedef struct {
    VOID *Widget;           /* Child widget */
    UINT32 Height;          /* Fixed height (0 = auto-size) */
    BOOLEAN Expand;         /* Should expand to fill space */
    BOOLEAN Fill;           /* Should fill allocated space */
    UINT32 Padding;         /* Padding around this child */
} VBoxChild;

typedef struct {
    ITuiVBox Interface;
    WIDGET_STATE State;

    /* Children */
    VBoxChild Children[MAX_VBOX_CHILDREN];
    UINT32 ChildCount;

    /* Spacing between children */
    UINT32 Spacing;

    /* Homogeneous sizing */
    BOOLEAN Homogeneous;  /* All children get equal space */

} TuiVBoxImpl;

/* IUnknown methods */
static HRESULT ANXAPI VBox_QueryInterface(
    ITuiVBox *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiVBox)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI VBox_AddRef(ITuiVBox *This)
{
    TuiVBoxImpl *impl = (TuiVBoxImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI VBox_Release(ITuiVBox *This)
{
    TuiVBoxImpl *impl = (TuiVBoxImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        /* Release all child widgets */
        for (UINT32 i = 0; i < impl->ChildCount; i++) {
            if (impl->Children[i].Widget) {
                IUnknown *unk = (IUnknown *)impl->Children[i].Widget;
                unk->Vtbl->Release(unk);
            }
        }
        free(impl);
    }

    return count;
}

/* Render the VBox */
static HRESULT ANXAPI VBox_Render(
    ITuiVBox *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiVBoxImpl *impl = (TuiVBoxImpl *)This;

    if (!impl->State.Visible) return S_OK;
    if (impl->ChildCount == 0) return S_OK;

    UINT32 availableHeight = impl->State.Bounds.Height;
    UINT32 containerWidth = impl->State.Bounds.Width;

    /* Calculate total spacing */
    UINT32 totalSpacing = (impl->ChildCount > 1) ? (impl->Spacing * (impl->ChildCount - 1)) : 0;

    /* Calculate heights */
    UINT32 heights[MAX_VBOX_CHILDREN] = {0};
    UINT32 usedHeight = totalSpacing;
    UINT32 expandCount = 0;

    if (impl->Homogeneous) {
        /* Equal height for all children */
        UINT32 childHeight = (availableHeight - totalSpacing) / impl->ChildCount;
        for (UINT32 i = 0; i < impl->ChildCount; i++) {
            heights[i] = childHeight;
        }
        usedHeight = availableHeight;
    } else {
        /* Calculate based on fixed heights and expand flags */
        for (UINT32 i = 0; i < impl->ChildCount; i++) {
            if (impl->Children[i].Height > 0) {
                heights[i] = impl->Children[i].Height;
                usedHeight += heights[i];
            } else if (!impl->Children[i].Expand) {
                /* Auto size - use minimum (3 lines) */
                heights[i] = 3;
                usedHeight += heights[i];
            } else {
                expandCount++;
            }
        }

        /* Distribute remaining space to expanding children */
        if (expandCount > 0 && usedHeight < availableHeight) {
            UINT32 expandHeight = (availableHeight - usedHeight) / expandCount;
            for (UINT32 i = 0; i < impl->ChildCount; i++) {
                if (impl->Children[i].Height == 0 && impl->Children[i].Expand) {
                    heights[i] = expandHeight;
                }
            }
        }
    }

    /* Position and render children */
    UINT32 currentY = impl->State.Bounds.Y;

    for (UINT32 i = 0; i < impl->ChildCount; i++) {
        if (impl->Children[i].Widget) {
            VBoxChild *child = &impl->Children[i];

            /* Calculate child bounds */
            TUI_RECT childBounds;
            childBounds.X = impl->State.Bounds.X + child->Padding;
            childBounds.Y = currentY + child->Padding;

            if (child->Fill) {
                childBounds.Width = containerWidth - (2 * child->Padding);
                childBounds.Height = heights[i] - (2 * child->Padding);
            } else {
                /* Center the widget */
                childBounds.Width = containerWidth / 2;
                childBounds.Height = heights[i] - (2 * child->Padding);
                childBounds.X += (containerWidth - childBounds.Width) / 2;
            }

            /* Set bounds and render */
            ITuiButton *widget = (ITuiButton *)child->Widget;
            if (widget->Vtbl->SetBounds) {
                widget->Vtbl->SetBounds(widget, &childBounds);
            }
            if (widget->Vtbl->Render) {
                widget->Vtbl->Render(widget, Screen, childBounds.X, childBounds.Y, FALSE);
            }
        }

        currentY += heights[i] + impl->Spacing;
    }

    return S_OK;
}

/* Pack a child widget */
static HRESULT ANXAPI VBox_PackStart(
    ITuiVBox *This,
    VOID *Widget,
    BOOLEAN Expand,
    BOOLEAN Fill,
    UINT32 Padding
)
{
    TuiVBoxImpl *impl = (TuiVBoxImpl *)This;

    if (impl->ChildCount >= MAX_VBOX_CHILDREN) {
        return E_OUTOFMEMORY;
    }

    if (!Widget) return E_INVALIDARG;

    VBoxChild *child = &impl->Children[impl->ChildCount++];
    child->Widget = Widget;
    child->Height = 0;  /* Auto */
    child->Expand = Expand;
    child->Fill = Fill;
    child->Padding = Padding;

    /* AddRef the widget */
    IUnknown *unk = (IUnknown *)Widget;
    unk->Vtbl->AddRef(unk);

    return S_OK;
}

/* Set spacing */
static HRESULT ANXAPI VBox_SetSpacing(
    ITuiVBox *This,
    UINT32 Spacing
)
{
    TuiVBoxImpl *impl = (TuiVBoxImpl *)This;
    impl->Spacing = Spacing;
    return S_OK;
}

/* Set homogeneous */
static HRESULT ANXAPI VBox_SetHomogeneous(
    ITuiVBox *This,
    BOOLEAN Homogeneous
)
{
    TuiVBoxImpl *impl = (TuiVBoxImpl *)This;
    impl->Homogeneous = Homogeneous;
    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI VBox_SetBounds(ITuiVBox *This, CONST TUI_RECT *Bounds)
{
    TuiVBoxImpl *impl = (TuiVBoxImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI VBox_GetBounds(ITuiVBox *This, TUI_RECT *Bounds)
{
    TuiVBoxImpl *impl = (TuiVBoxImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI VBox_SetVisible(ITuiVBox *This, BOOLEAN Visible)
{
    TuiVBoxImpl *impl = (TuiVBoxImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI VBox_IsVisible(ITuiVBox *This)
{
    TuiVBoxImpl *impl = (TuiVBoxImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI VBox_SetEnabled(ITuiVBox *This, BOOLEAN Enabled)
{
    TuiVBoxImpl *impl = (TuiVBoxImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI VBox_IsEnabled(ITuiVBox *This)
{
    TuiVBoxImpl *impl = (TuiVBoxImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiVBoxVtbl VBoxVtbl = {
    VBox_QueryInterface,
    VBox_AddRef,
    VBox_Release,
    VBox_Render,
    VBox_SetBounds,
    VBox_GetBounds,
    VBox_SetVisible,
    VBox_IsVisible,
    VBox_SetEnabled,
    VBox_IsEnabled,
    VBox_PackStart,
    VBox_SetSpacing,
    VBox_SetHomogeneous
};

/* Factory function */
HRESULT AnxTuiCreateVBox(
    BOOLEAN Homogeneous,
    UINT32 Spacing,
    ITuiVBox **OutVBox
)
{
    TuiVBoxImpl *impl;

    if (!OutVBox) return E_INVALIDARG;

    impl = (TuiVBoxImpl *)malloc(sizeof(TuiVBoxImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiVBoxImpl));
    impl->Interface.Vtbl = &VBoxVtbl;
    InitWidgetState(&impl->State);

    impl->Homogeneous = Homogeneous;
    impl->Spacing = Spacing;

    impl->State.Bounds.Width = 80;
    impl->State.Bounds.Height = 24;

    *OutVBox = &impl->Interface;
    return S_OK;
}
