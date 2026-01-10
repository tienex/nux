/*
 * widget_flexcontainer.c - Flexible Container Widget
 *
 * Flexbox-like layout container with support for:
 * - Row and column direction
 * - Flex-grow and flex-shrink
 * - Alignment (start, center, end, stretch)
 * - Justify content (start, center, end, space-between, space-around, space-evenly)
 * - Wrapping
 * - Gap between items
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_FLEX_CHILDREN 64

/* Flex direction */
typedef enum {
    FlexDirectionRow,           /* Horizontal left-to-right */
    FlexDirectionRowReverse,    /* Horizontal right-to-left */
    FlexDirectionColumn,        /* Vertical top-to-bottom */
    FlexDirectionColumnReverse  /* Vertical bottom-to-top */
} FlexDirection;

/* Justify content (main axis) */
typedef enum {
    JustifyStart,         /* Pack items to start */
    JustifyEnd,           /* Pack items to end */
    JustifyCenter,        /* Pack items to center */
    JustifySpaceBetween,  /* Distribute evenly, first/last at edges */
    JustifySpaceAround,   /* Distribute evenly with equal space around */
    JustifySpaceEvenly    /* Distribute evenly with equal gaps */
} JustifyContent;

/* Align items (cross axis) */
typedef enum {
    AlignStart,    /* Align to start of cross axis */
    AlignEnd,      /* Align to end of cross axis */
    AlignCenter,   /* Center on cross axis */
    AlignStretch   /* Stretch to fill cross axis */
} AlignItems;

/* Flex wrap */
typedef enum {
    FlexNoWrap,     /* Single line */
    FlexWrap,       /* Multi-line, wrap to next line */
    FlexWrapReverse /* Multi-line, wrap in reverse */
} FlexWrap;

/* Child item properties */
typedef struct {
    VOID *Widget;          /* Child widget (any ITui* interface) */
    UINT32 FlexGrow;       /* Grow factor (0 = don't grow) */
    UINT32 FlexShrink;     /* Shrink factor (0 = don't shrink) */
    INT32 FlexBasis;       /* Initial size (-1 = auto) */
    AlignItems AlignSelf;  /* Override container alignment */
    INT32 Order;           /* Display order (default 0) */
} FlexChild;

typedef struct {
    ITuiFlexContainer Interface;
    WIDGET_STATE State;

    /* Children */
    FlexChild Children[MAX_FLEX_CHILDREN];
    UINT32 ChildCount;

    /* Layout properties */
    FlexDirection Direction;
    JustifyContent Justify;
    AlignItems Align;
    FlexWrap Wrap;
    UINT32 Gap;              /* Gap between items (in cells) */
    UINT32 RowGap;           /* Gap between rows */
    UINT32 ColumnGap;        /* Gap between columns */

    /* Padding */
    UINT32 PaddingTop;
    UINT32 PaddingRight;
    UINT32 PaddingBottom;
    UINT32 PaddingLeft;

} TuiFlexContainerImpl;

/* Helper: Is direction horizontal? */
static inline BOOLEAN IsHorizontal(FlexDirection dir) {
    return dir == FlexDirectionRow || dir == FlexDirectionRowReverse;
}

/* Helper: Get main axis size */
static inline UINT32 GetMainSize(FlexDirection dir, CONST TUI_RECT *rect) {
    return IsHorizontal(dir) ? rect->Width : rect->Height;
}

/* Helper: Get cross axis size */
static inline UINT32 GetCrossSize(FlexDirection dir, CONST TUI_RECT *rect) {
    return IsHorizontal(dir) ? rect->Height : rect->Width;
}

/* IUnknown methods */
static HRESULT ANXAPI FlexContainer_QueryInterface(
    ITuiFlexContainer *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiFlexContainer)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI FlexContainer_AddRef(ITuiFlexContainer *This)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI FlexContainer_Release(ITuiFlexContainer *This)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        /* Release all child widgets */
        for (UINT32 i = 0; i < impl->ChildCount; i++) {
            if (impl->Children[i].Widget) {
                /* Assume all widgets have IUnknown at offset 0 */
                IUnknown *unk = (IUnknown *)impl->Children[i].Widget;
                unk->Vtbl->Release(unk);
            }
        }
        free(impl);
    }

    return count;
}

/* Layout calculation */
static VOID CalculateLayout(
    TuiFlexContainerImpl *impl,
    TUI_RECT *outPositions,
    UINT32 *outCount
)
{
    if (impl->ChildCount == 0) {
        *outCount = 0;
        return;
    }

    /* Calculate available space */
    UINT32 availableWidth = impl->State.Bounds.Width - impl->PaddingLeft - impl->PaddingRight;
    UINT32 availableHeight = impl->State.Bounds.Height - impl->PaddingTop - impl->PaddingBottom;

    BOOLEAN isHorizontal = IsHorizontal(impl->Direction);
    UINT32 mainSize = isHorizontal ? availableWidth : availableHeight;
    UINT32 crossSize = isHorizontal ? availableHeight : availableWidth;

    /* Sort children by order */
    FlexChild sortedChildren[MAX_FLEX_CHILDREN];
    memcpy(sortedChildren, impl->Children, impl->ChildCount * sizeof(FlexChild));

    /* Simple bubble sort by order */
    for (UINT32 i = 0; i < impl->ChildCount - 1; i++) {
        for (UINT32 j = 0; j < impl->ChildCount - i - 1; j++) {
            if (sortedChildren[j].Order > sortedChildren[j + 1].Order) {
                FlexChild temp = sortedChildren[j];
                sortedChildren[j] = sortedChildren[j + 1];
                sortedChildren[j + 1] = temp;
            }
        }
    }

    /* Calculate base sizes and total flex factors */
    UINT32 usedSpace = 0;
    UINT32 totalFlexGrow = 0;
    UINT32 totalFlexShrink = 0;
    UINT32 gap = impl->Gap;

    UINT32 sizes[MAX_FLEX_CHILDREN] = {0};

    for (UINT32 i = 0; i < impl->ChildCount; i++) {
        FlexChild *child = &sortedChildren[i];

        /* Start with flex-basis or auto size */
        if (child->FlexBasis >= 0) {
            sizes[i] = child->FlexBasis;
        } else {
            /* Auto: use minimum size (10 for now) */
            sizes[i] = 10;
        }

        usedSpace += sizes[i];
        totalFlexGrow += child->FlexGrow;
        totalFlexShrink += child->FlexShrink;
    }

    /* Add gaps */
    if (impl->ChildCount > 1) {
        usedSpace += gap * (impl->ChildCount - 1);
    }

    /* Distribute remaining space */
    if (usedSpace < mainSize && totalFlexGrow > 0) {
        /* Grow: distribute extra space */
        UINT32 extraSpace = mainSize - usedSpace;
        for (UINT32 i = 0; i < impl->ChildCount; i++) {
            if (sortedChildren[i].FlexGrow > 0) {
                sizes[i] += (extraSpace * sortedChildren[i].FlexGrow) / totalFlexGrow;
            }
        }
    } else if (usedSpace > mainSize && totalFlexShrink > 0) {
        /* Shrink: remove excess space */
        UINT32 deficit = usedSpace - mainSize;
        for (UINT32 i = 0; i < impl->ChildCount; i++) {
            if (sortedChildren[i].FlexShrink > 0) {
                UINT32 shrinkAmount = (deficit * sortedChildren[i].FlexShrink) / totalFlexShrink;
                if (sizes[i] > shrinkAmount) {
                    sizes[i] -= shrinkAmount;
                } else {
                    sizes[i] = 0;
                }
            }
        }
    }

    /* Calculate positions based on justify-content */
    UINT32 position = 0;
    UINT32 spacing = 0;

    /* Recalculate total used space after flex */
    usedSpace = 0;
    for (UINT32 i = 0; i < impl->ChildCount; i++) {
        usedSpace += sizes[i];
    }
    if (impl->ChildCount > 1) {
        usedSpace += gap * (impl->ChildCount - 1);
    }

    switch (impl->Justify) {
        case JustifyStart:
            position = 0;
            spacing = gap;
            break;

        case JustifyEnd:
            position = (mainSize > usedSpace) ? (mainSize - usedSpace) : 0;
            spacing = gap;
            break;

        case JustifyCenter:
            position = (mainSize > usedSpace) ? ((mainSize - usedSpace) / 2) : 0;
            spacing = gap;
            break;

        case JustifySpaceBetween:
            position = 0;
            if (impl->ChildCount > 1) {
                UINT32 totalItemSize = 0;
                for (UINT32 i = 0; i < impl->ChildCount; i++) {
                    totalItemSize += sizes[i];
                }
                spacing = (mainSize > totalItemSize) ?
                         ((mainSize - totalItemSize) / (impl->ChildCount - 1)) : gap;
            } else {
                spacing = 0;
            }
            break;

        case JustifySpaceAround:
            if (impl->ChildCount > 0) {
                UINT32 totalItemSize = 0;
                for (UINT32 i = 0; i < impl->ChildCount; i++) {
                    totalItemSize += sizes[i];
                }
                spacing = (mainSize > totalItemSize) ?
                         ((mainSize - totalItemSize) / impl->ChildCount) : gap;
                position = spacing / 2;
            }
            break;

        case JustifySpaceEvenly:
            if (impl->ChildCount > 0) {
                UINT32 totalItemSize = 0;
                for (UINT32 i = 0; i < impl->ChildCount; i++) {
                    totalItemSize += sizes[i];
                }
                spacing = (mainSize > totalItemSize) ?
                         ((mainSize - totalItemSize) / (impl->ChildCount + 1)) : gap;
                position = spacing;
            }
            break;
    }

    /* Apply positions */
    for (UINT32 i = 0; i < impl->ChildCount; i++) {
        TUI_RECT *rect = &outPositions[i];

        if (isHorizontal) {
            rect->X = impl->State.Bounds.X + impl->PaddingLeft + position;
            rect->Width = sizes[i];
            rect->Height = crossSize;

            /* Cross-axis alignment */
            AlignItems align = (sortedChildren[i].AlignSelf != AlignStart) ?
                              sortedChildren[i].AlignSelf : impl->Align;

            switch (align) {
                case AlignStart:
                    rect->Y = impl->State.Bounds.Y + impl->PaddingTop;
                    break;
                case AlignEnd:
                    rect->Y = impl->State.Bounds.Y + impl->PaddingTop + crossSize - rect->Height;
                    break;
                case AlignCenter:
                    rect->Y = impl->State.Bounds.Y + impl->PaddingTop + (crossSize - rect->Height) / 2;
                    break;
                case AlignStretch:
                    rect->Y = impl->State.Bounds.Y + impl->PaddingTop;
                    rect->Height = crossSize;
                    break;
            }
        } else {
            rect->Y = impl->State.Bounds.Y + impl->PaddingTop + position;
            rect->Height = sizes[i];
            rect->Width = crossSize;

            /* Cross-axis alignment */
            AlignItems align = (sortedChildren[i].AlignSelf != AlignStart) ?
                              sortedChildren[i].AlignSelf : impl->Align;

            switch (align) {
                case AlignStart:
                    rect->X = impl->State.Bounds.X + impl->PaddingLeft;
                    break;
                case AlignEnd:
                    rect->X = impl->State.Bounds.X + impl->PaddingLeft + crossSize - rect->Width;
                    break;
                case AlignCenter:
                    rect->X = impl->State.Bounds.X + impl->PaddingLeft + (crossSize - rect->Width) / 2;
                    break;
                case AlignStretch:
                    rect->X = impl->State.Bounds.X + impl->PaddingLeft;
                    rect->Width = crossSize;
                    break;
            }
        }

        position += sizes[i] + spacing;
    }

    *outCount = impl->ChildCount;
}

/* Render the flex container */
static HRESULT ANXAPI FlexContainer_Render(
    ITuiFlexContainer *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;

    if (!impl->State.Visible) return S_OK;

    /* Calculate layout */
    TUI_RECT positions[MAX_FLEX_CHILDREN];
    UINT32 count;
    CalculateLayout(impl, positions, &count);

    /* Render children */
    for (UINT32 i = 0; i < count; i++) {
        if (impl->Children[i].Widget) {
            /* All widgets should have Render method at same vtable offset */
            /* This is a simplification - in real code we'd query interface */
            ITuiButton *widget = (ITuiButton *)impl->Children[i].Widget;

            /* Set widget bounds */
            if (widget->Vtbl->SetBounds) {
                widget->Vtbl->SetBounds(widget, &positions[i]);
            }

            /* Render widget */
            if (widget->Vtbl->Render) {
                widget->Vtbl->Render(widget, Screen, positions[i].X, positions[i].Y, FALSE);
            }
        }
    }

    return S_OK;
}

/* Add a child widget */
static HRESULT ANXAPI FlexContainer_AddChild(
    ITuiFlexContainer *This,
    VOID *Widget,
    UINT32 FlexGrow,
    UINT32 FlexShrink,
    INT32 FlexBasis
)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;

    if (impl->ChildCount >= MAX_FLEX_CHILDREN) {
        return E_OUTOFMEMORY;
    }

    if (!Widget) return E_INVALIDARG;

    FlexChild *child = &impl->Children[impl->ChildCount++];
    child->Widget = Widget;
    child->FlexGrow = FlexGrow;
    child->FlexShrink = FlexShrink;
    child->FlexBasis = FlexBasis;
    child->AlignSelf = AlignStart;  /* Use container default */
    child->Order = 0;

    /* AddRef the widget */
    IUnknown *unk = (IUnknown *)Widget;
    unk->Vtbl->AddRef(unk);

    return S_OK;
}

/* Remove a child widget */
static HRESULT ANXAPI FlexContainer_RemoveChild(
    ITuiFlexContainer *This,
    VOID *Widget
)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;

    for (UINT32 i = 0; i < impl->ChildCount; i++) {
        if (impl->Children[i].Widget == Widget) {
            /* Release the widget */
            IUnknown *unk = (IUnknown *)Widget;
            unk->Vtbl->Release(unk);

            /* Shift remaining children */
            for (UINT32 j = i; j < impl->ChildCount - 1; j++) {
                impl->Children[j] = impl->Children[j + 1];
            }
            impl->ChildCount--;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

/* Set flex direction */
static HRESULT ANXAPI FlexContainer_SetDirection(
    ITuiFlexContainer *This,
    UINT32 Direction
)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;
    impl->Direction = (FlexDirection)Direction;
    return S_OK;
}

/* Set justify content */
static HRESULT ANXAPI FlexContainer_SetJustifyContent(
    ITuiFlexContainer *This,
    UINT32 Justify
)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;
    impl->Justify = (JustifyContent)Justify;
    return S_OK;
}

/* Set align items */
static HRESULT ANXAPI FlexContainer_SetAlignItems(
    ITuiFlexContainer *This,
    UINT32 Align
)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;
    impl->Align = (AlignItems)Align;
    return S_OK;
}

/* Set gap */
static HRESULT ANXAPI FlexContainer_SetGap(
    ITuiFlexContainer *This,
    UINT32 Gap
)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;
    impl->Gap = Gap;
    return S_OK;
}

/* Set padding */
static HRESULT ANXAPI FlexContainer_SetPadding(
    ITuiFlexContainer *This,
    UINT32 Top,
    UINT32 Right,
    UINT32 Bottom,
    UINT32 Left
)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;
    impl->PaddingTop = Top;
    impl->PaddingRight = Right;
    impl->PaddingBottom = Bottom;
    impl->PaddingLeft = Left;
    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI FlexContainer_SetBounds(ITuiFlexContainer *This, CONST TUI_RECT *Bounds)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI FlexContainer_GetBounds(ITuiFlexContainer *This, TUI_RECT *Bounds)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI FlexContainer_SetVisible(ITuiFlexContainer *This, BOOLEAN Visible)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI FlexContainer_IsVisible(ITuiFlexContainer *This)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI FlexContainer_SetEnabled(ITuiFlexContainer *This, BOOLEAN Enabled)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI FlexContainer_IsEnabled(ITuiFlexContainer *This)
{
    TuiFlexContainerImpl *impl = (TuiFlexContainerImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiFlexContainerVtbl FlexContainerVtbl = {
    FlexContainer_QueryInterface,
    FlexContainer_AddRef,
    FlexContainer_Release,
    FlexContainer_Render,
    FlexContainer_SetBounds,
    FlexContainer_GetBounds,
    FlexContainer_SetVisible,
    FlexContainer_IsVisible,
    FlexContainer_SetEnabled,
    FlexContainer_IsEnabled,
    FlexContainer_AddChild,
    FlexContainer_RemoveChild,
    FlexContainer_SetDirection,
    FlexContainer_SetJustifyContent,
    FlexContainer_SetAlignItems,
    FlexContainer_SetGap,
    FlexContainer_SetPadding
};

/* Factory function */
HRESULT AnxTuiCreateFlexContainer(ITuiFlexContainer **OutContainer)
{
    TuiFlexContainerImpl *impl;

    if (!OutContainer) return E_INVALIDARG;

    impl = (TuiFlexContainerImpl *)malloc(sizeof(TuiFlexContainerImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiFlexContainerImpl));
    impl->Interface.Vtbl = &FlexContainerVtbl;
    InitWidgetState(&impl->State);

    /* Default flex properties */
    impl->Direction = FlexDirectionRow;
    impl->Justify = JustifyStart;
    impl->Align = AlignStretch;
    impl->Wrap = FlexNoWrap;
    impl->Gap = 1;

    impl->State.Bounds.Width = 80;
    impl->State.Bounds.Height = 24;

    *OutContainer = &impl->Interface;
    return S_OK;
}
