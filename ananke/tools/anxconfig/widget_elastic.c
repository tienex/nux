/*
 * Elastic Container Widget Implementation
 *
 * Provides flexible layout with widgets that can expand/contract
 * to fill available space. Supports horizontal and vertical stacking.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

#define MAX_ELASTIC_CHILDREN 32

typedef enum {
    ElasticHorizontal,
    ElasticVertical
} ElasticDirection;

typedef enum {
    ElasticSizeFixed,      /* Fixed size in pixels/chars */
    ElasticSizeExpand,     /* Expand to fill available space */
    ElasticSizePercent     /* Percentage of available space */
} ElasticSizeMode;

typedef struct {
    VOID *Widget;
    ElasticSizeMode SizeMode;
    UINT32 Size;           /* Fixed size or percentage (0-100) */
    UINT32 MinSize;        /* Minimum size */
    UINT32 MaxSize;        /* Maximum size (0 = unlimited) */
    UINT32 ComputedSize;   /* Actual computed size after layout */
    INT32 ComputedX;       /* Computed X position */
    INT32 ComputedY;       /* Computed Y position */
} ElasticChild;

typedef struct {
    ITuiElasticContainer Interface;
    WIDGET_STATE State;
    ElasticDirection Direction;
    ElasticChild Children[MAX_ELASTIC_CHILDREN];
    UINT32 ChildCount;
    UINT32 Spacing;        /* Spacing between children */
} TuiElasticContainerImpl;

/* IUnknown methods */
static HRESULT ANXAPI ElasticContainer_QueryInterface(
    ITuiElasticContainer *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI ElasticContainer_AddRef(ITuiElasticContainer *This)
{
    TuiElasticContainerImpl *impl = (TuiElasticContainerImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ElasticContainer_Release(ITuiElasticContainer *This)
{
    TuiElasticContainerImpl *impl = (TuiElasticContainerImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiElasticContainer methods */
static HRESULT ANXAPI ElasticContainer_SetDirection(
    ITuiElasticContainer *This,
    BOOLEAN Horizontal
)
{
    TuiElasticContainerImpl *impl = (TuiElasticContainerImpl *)This;
    impl->Direction = Horizontal ? ElasticHorizontal : ElasticVertical;
    return S_OK;
}

static HRESULT ANXAPI ElasticContainer_SetSpacing(
    ITuiElasticContainer *This,
    UINT32 Spacing
)
{
    TuiElasticContainerImpl *impl = (TuiElasticContainerImpl *)This;
    impl->Spacing = Spacing;
    return S_OK;
}

static HRESULT ANXAPI ElasticContainer_AddChild(
    ITuiElasticContainer *This,
    VOID *Widget,
    BOOLEAN Expand,
    UINT32 MinSize,
    UINT32 MaxSize
)
{
    TuiElasticContainerImpl *impl = (TuiElasticContainerImpl *)This;

    if (Widget == NULL) return E_POINTER;
    if (impl->ChildCount >= MAX_ELASTIC_CHILDREN) return E_OUTOFMEMORY;

    impl->Children[impl->ChildCount].Widget = Widget;
    impl->Children[impl->ChildCount].SizeMode = Expand ? ElasticSizeExpand : ElasticSizeFixed;
    impl->Children[impl->ChildCount].Size = MinSize;
    impl->Children[impl->ChildCount].MinSize = MinSize;
    impl->Children[impl->ChildCount].MaxSize = MaxSize;
    impl->Children[impl->ChildCount].ComputedSize = MinSize;
    impl->ChildCount++;

    return S_OK;
}

static HRESULT ANXAPI ElasticContainer_AddChildPercent(
    ITuiElasticContainer *This,
    VOID *Widget,
    UINT32 Percent
)
{
    TuiElasticContainerImpl *impl = (TuiElasticContainerImpl *)This;

    if (Widget == NULL) return E_POINTER;
    if (impl->ChildCount >= MAX_ELASTIC_CHILDREN) return E_OUTOFMEMORY;
    if (Percent > 100) return E_INVALIDARG;

    impl->Children[impl->ChildCount].Widget = Widget;
    impl->Children[impl->ChildCount].SizeMode = ElasticSizePercent;
    impl->Children[impl->ChildCount].Size = Percent;
    impl->Children[impl->ChildCount].MinSize = 0;
    impl->Children[impl->ChildCount].MaxSize = 0;
    impl->ChildCount++;

    return S_OK;
}

static HRESULT ANXAPI ElasticContainer_RemoveChild(
    ITuiElasticContainer *This,
    VOID *Widget
)
{
    TuiElasticContainerImpl *impl = (TuiElasticContainerImpl *)This;
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

/* Helper: Compute layout for children */
static VOID ComputeLayout(
    TuiElasticContainerImpl *impl,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height
)
{
    UINT32 i;
    UINT32 availableSize;
    UINT32 usedSize = 0;
    UINT32 expandCount = 0;
    UINT32 totalPercent = 0;

    /* Determine available size */
    availableSize = (impl->Direction == ElasticHorizontal) ? Width : Height;

    /* Subtract spacing */
    if (impl->ChildCount > 1) {
        availableSize -= impl->Spacing * (impl->ChildCount - 1);
    }

    /* First pass: compute fixed and percent sizes */
    for (i = 0; i < impl->ChildCount; i++) {
        if (impl->Children[i].SizeMode == ElasticSizeFixed) {
            impl->Children[i].ComputedSize = impl->Children[i].Size;
            usedSize += impl->Children[i].ComputedSize;
        } else if (impl->Children[i].SizeMode == ElasticSizePercent) {
            UINT32 percentSize = (availableSize * impl->Children[i].Size) / 100;
            impl->Children[i].ComputedSize = percentSize;
            usedSize += percentSize;
            totalPercent += impl->Children[i].Size;
        } else if (impl->Children[i].SizeMode == ElasticSizeExpand) {
            expandCount++;
        }
    }

    /* Second pass: distribute remaining space to expanding widgets */
    if (expandCount > 0 && availableSize > usedSize) {
        UINT32 remainingSize = availableSize - usedSize;
        UINT32 expandSize = remainingSize / expandCount;

        for (i = 0; i < impl->ChildCount; i++) {
            if (impl->Children[i].SizeMode == ElasticSizeExpand) {
                impl->Children[i].ComputedSize = expandSize;

                /* Respect min/max constraints */
                if (impl->Children[i].MinSize > 0 &&
                    impl->Children[i].ComputedSize < impl->Children[i].MinSize) {
                    impl->Children[i].ComputedSize = impl->Children[i].MinSize;
                }
                if (impl->Children[i].MaxSize > 0 &&
                    impl->Children[i].ComputedSize > impl->Children[i].MaxSize) {
                    impl->Children[i].ComputedSize = impl->Children[i].MaxSize;
                }
            }
        }
    }

    /* Third pass: compute positions */
    UINT32 currentPos = 0;
    for (i = 0; i < impl->ChildCount; i++) {
        if (impl->Direction == ElasticHorizontal) {
            impl->Children[i].ComputedX = X + currentPos;
            impl->Children[i].ComputedY = Y;
        } else {
            impl->Children[i].ComputedX = X;
            impl->Children[i].ComputedY = Y + currentPos;
        }

        currentPos += impl->Children[i].ComputedSize + impl->Spacing;
    }
}

static HRESULT ANXAPI ElasticContainer_Render(
    ITuiElasticContainer *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height
)
{
    TuiElasticContainerImpl *impl = (TuiElasticContainerImpl *)This;
    UINT32 i;

    if (!impl->State.Visible) return S_OK;

    /* Compute layout */
    ComputeLayout(impl, X, Y, Width, Height);

    /* Render children at computed positions */
    for (i = 0; i < impl->ChildCount; i++) {
        /* Note: Actual rendering would call each child's Render method
         * through their vtables with computed position and size:
         *   childWidget->Vtbl->Render(
         *       childWidget,
         *       Screen,
         *       Children[i].ComputedX,
         *       Children[i].ComputedY,
         *       (Direction == Horizontal) ? Children[i].ComputedSize : Width,
         *       (Direction == Vertical) ? Children[i].ComputedSize : Height
         *   );
         */
    }

    return S_OK;
}

static HRESULT ANXAPI ElasticContainer_HandleInput(
    ITuiElasticContainer *This,
    CONST TUI_INPUT_EVENT *Event,
    BOOLEAN *Handled
)
{
    TuiElasticContainerImpl *impl = (TuiElasticContainerImpl *)This;
    UINT32 i;

    /* Route input to children */
    for (i = 0; i < impl->ChildCount; i++) {
        /* Note: Actual implementation would route events to children */
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiElasticContainer_Vtbl ElasticContainerVtbl = {
    ElasticContainer_QueryInterface,
    ElasticContainer_AddRef,
    ElasticContainer_Release,
    ElasticContainer_SetDirection,
    ElasticContainer_SetSpacing,
    ElasticContainer_AddChild,
    ElasticContainer_AddChildPercent,
    ElasticContainer_RemoveChild,
    ElasticContainer_Render,
    ElasticContainer_HandleInput
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateElasticContainer(
    IN  BOOLEAN Horizontal,
    OUT ITuiElasticContainer **Container
)
{
    TuiElasticContainerImpl *impl;

    if (Container == NULL) return E_POINTER;

    impl = (TuiElasticContainerImpl *)calloc(1, sizeof(TuiElasticContainerImpl));
    if (impl == NULL) {
        *Container = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &ElasticContainerVtbl;
    InitWidgetState(&impl->State);

    impl->Direction = Horizontal ? ElasticHorizontal : ElasticVertical;
    impl->ChildCount = 0;
    impl->Spacing = 1;

    *Container = &impl->Interface;
    return S_OK;
}
