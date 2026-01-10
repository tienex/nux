/*
 * widget_hbox.c - Horizontal Box Container Widget
 *
 * Simple horizontal stacking container that arranges children left-to-right.
 * Lighter weight alternative to flex container for common horizontal layouts.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_HBOX_CHILDREN 64

/* Child properties */
typedef struct {
    VOID *Widget;           /* Child widget */
    UINT32 Width;           /* Fixed width (0 = auto-size) */
    BOOLEAN Expand;         /* Should expand to fill space */
    BOOLEAN Fill;           /* Should fill allocated space */
    UINT32 Padding;         /* Padding around this child */
} HBoxChild;

typedef struct {
    ITuiHBox Interface;
    WIDGET_STATE State;

    /* Children */
    HBoxChild Children[MAX_HBOX_CHILDREN];
    UINT32 ChildCount;

    /* Spacing between children */
    UINT32 Spacing;

    /* Homogeneous sizing */
    BOOLEAN Homogeneous;  /* All children get equal space */

} TuiHBoxImpl;

/* IUnknown methods */
static HRESULT ANXAPI HBox_QueryInterface(
    ITuiHBox *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiHBox)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI HBox_AddRef(ITuiHBox *This)
{
    TuiHBoxImpl *impl = (TuiHBoxImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI HBox_Release(ITuiHBox *This)
{
    TuiHBoxImpl *impl = (TuiHBoxImpl *)This;
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

/* Render the HBox */
static HRESULT ANXAPI HBox_Render(
    ITuiHBox *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiHBoxImpl *impl = (TuiHBoxImpl *)This;

    if (!impl->State.Visible) return S_OK;
    if (impl->ChildCount == 0) return S_OK;

    UINT32 availableWidth = impl->State.Bounds.Width;
    UINT32 containerHeight = impl->State.Bounds.Height;

    /* Calculate total spacing */
    UINT32 totalSpacing = (impl->ChildCount > 1) ? (impl->Spacing * (impl->ChildCount - 1)) : 0;

    /* Calculate widths */
    UINT32 widths[MAX_HBOX_CHILDREN] = {0};
    UINT32 usedWidth = totalSpacing;
    UINT32 expandCount = 0;

    if (impl->Homogeneous) {
        /* Equal width for all children */
        UINT32 childWidth = (availableWidth - totalSpacing) / impl->ChildCount;
        for (UINT32 i = 0; i < impl->ChildCount; i++) {
            widths[i] = childWidth;
        }
        usedWidth = availableWidth;
    } else {
        /* Calculate based on fixed widths and expand flags */
        for (UINT32 i = 0; i < impl->ChildCount; i++) {
            if (impl->Children[i].Width > 0) {
                widths[i] = impl->Children[i].Width;
                usedWidth += widths[i];
            } else if (!impl->Children[i].Expand) {
                /* Auto size - use minimum (10 columns) */
                widths[i] = 10;
                usedWidth += widths[i];
            } else {
                expandCount++;
            }
        }

        /* Distribute remaining space to expanding children */
        if (expandCount > 0 && usedWidth < availableWidth) {
            UINT32 expandWidth = (availableWidth - usedWidth) / expandCount;
            for (UINT32 i = 0; i < impl->ChildCount; i++) {
                if (impl->Children[i].Width == 0 && impl->Children[i].Expand) {
                    widths[i] = expandWidth;
                }
            }
        }
    }

    /* Position and render children */
    UINT32 currentX = impl->State.Bounds.X;

    for (UINT32 i = 0; i < impl->ChildCount; i++) {
        if (impl->Children[i].Widget) {
            HBoxChild *child = &impl->Children[i];

            /* Calculate child bounds */
            TUI_RECT childBounds;
            childBounds.X = currentX + child->Padding;
            childBounds.Y = impl->State.Bounds.Y + child->Padding;

            if (child->Fill) {
                childBounds.Width = widths[i] - (2 * child->Padding);
                childBounds.Height = containerHeight - (2 * child->Padding);
            } else {
                /* Center the widget */
                childBounds.Width = widths[i] - (2 * child->Padding);
                childBounds.Height = containerHeight / 2;
                childBounds.Y += (containerHeight - childBounds.Height) / 2;
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

        currentX += widths[i] + impl->Spacing;
    }

    return S_OK;
}

/* Pack a child widget */
static HRESULT ANXAPI HBox_PackStart(
    ITuiHBox *This,
    VOID *Widget,
    BOOLEAN Expand,
    BOOLEAN Fill,
    UINT32 Padding
)
{
    TuiHBoxImpl *impl = (TuiHBoxImpl *)This;

    if (impl->ChildCount >= MAX_HBOX_CHILDREN) {
        return E_OUTOFMEMORY;
    }

    if (!Widget) return E_INVALIDARG;

    HBoxChild *child = &impl->Children[impl->ChildCount++];
    child->Widget = Widget;
    child->Width = 0;  /* Auto */
    child->Expand = Expand;
    child->Fill = Fill;
    child->Padding = Padding;

    /* AddRef the widget */
    IUnknown *unk = (IUnknown *)Widget;
    unk->Vtbl->AddRef(unk);

    return S_OK;
}

/* Set spacing */
static HRESULT ANXAPI HBox_SetSpacing(
    ITuiHBox *This,
    UINT32 Spacing
)
{
    TuiHBoxImpl *impl = (TuiHBoxImpl *)This;
    impl->Spacing = Spacing;
    return S_OK;
}

/* Set homogeneous */
static HRESULT ANXAPI HBox_SetHomogeneous(
    ITuiHBox *This,
    BOOLEAN Homogeneous
)
{
    TuiHBoxImpl *impl = (TuiHBoxImpl *)This;
    impl->Homogeneous = Homogeneous;
    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI HBox_SetBounds(ITuiHBox *This, CONST TUI_RECT *Bounds)
{
    TuiHBoxImpl *impl = (TuiHBoxImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI HBox_GetBounds(ITuiHBox *This, TUI_RECT *Bounds)
{
    TuiHBoxImpl *impl = (TuiHBoxImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI HBox_SetVisible(ITuiHBox *This, BOOLEAN Visible)
{
    TuiHBoxImpl *impl = (TuiHBoxImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI HBox_IsVisible(ITuiHBox *This)
{
    TuiHBoxImpl *impl = (TuiHBoxImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI HBox_SetEnabled(ITuiHBox *This, BOOLEAN Enabled)
{
    TuiHBoxImpl *impl = (TuiHBoxImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI HBox_IsEnabled(ITuiHBox *This)
{
    TuiHBoxImpl *impl = (TuiHBoxImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiHBoxVtbl HBoxVtbl = {
    HBox_QueryInterface,
    HBox_AddRef,
    HBox_Release,
    HBox_Render,
    HBox_SetBounds,
    HBox_GetBounds,
    HBox_SetVisible,
    HBox_IsVisible,
    HBox_SetEnabled,
    HBox_IsEnabled,
    HBox_PackStart,
    HBox_SetSpacing,
    HBox_SetHomogeneous
};

/* Factory function */
HRESULT AnxTuiCreateHBox(
    BOOLEAN Homogeneous,
    UINT32 Spacing,
    ITuiHBox **OutHBox
)
{
    TuiHBoxImpl *impl;

    if (!OutHBox) return E_INVALIDARG;

    impl = (TuiHBoxImpl *)malloc(sizeof(TuiHBoxImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiHBoxImpl));
    impl->Interface.Vtbl = &HBoxVtbl;
    InitWidgetState(&impl->State);

    impl->Homogeneous = Homogeneous;
    impl->Spacing = Spacing;

    impl->State.Bounds.Width = 80;
    impl->State.Bounds.Height = 24;

    *OutHBox = &impl->Interface;
    return S_OK;
}
