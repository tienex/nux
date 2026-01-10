/*
 * composer.c - Compositor for Drawing Surfaces
 *
 * Manages drawing surfaces and composites them to the screen.
 * Widgets draw to surfaces instead of directly to the screen.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_SURFACES 64

typedef struct {
    ITuiWidget *Widget;
    ITuiSurface *Surface;
    INT32 ZOrder;
    BOOLEAN Dirty;
    TUI_RECT Bounds;
} RegisteredSurface;

typedef struct {
    ITuiComposer Interface;
    UINTN RefCount;

    /* Surface management */
    RegisteredSurface Surfaces[MAX_SURFACES];
    UINT32 SurfaceCount;

    /* Dirty region tracking */
    TUI_RECT DirtyRegions[MAX_SURFACES];
    UINT32 DirtyRegionCount;

} TuiComposerImpl;

/* IUnknown methods */
static HRESULT ANXAPI Composer_QueryInterface(
    ITuiComposer *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiComposer)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI Composer_AddRef(ITuiComposer *This)
{
    TuiComposerImpl *impl = (TuiComposerImpl *)This;
    return ++impl->RefCount;
}

static UINTN ANXAPI Composer_Release(ITuiComposer *This)
{
    TuiComposerImpl *impl = (TuiComposerImpl *)This;
    UINTN count = --impl->RefCount;

    if (count == 0) {
        /* Release all surfaces */
        for (UINT32 i = 0; i < impl->SurfaceCount; i++) {
            if (impl->Surfaces[i].Surface) {
                impl->Surfaces[i].Surface->Vtbl->Release(impl->Surfaces[i].Surface);
            }
            if (impl->Surfaces[i].Widget) {
                ITuiResponder *responder = (ITuiResponder *)impl->Surfaces[i].Widget;
                responder->Vtbl->Release(responder);
            }
        }

        free(impl);
    }

    return count;
}

/* Create surface */
static HRESULT ANXAPI Composer_CreateSurface(
    ITuiComposer *This,
    UINT32 Width,
    UINT32 Height,
    ITuiSurface **OutSurface
)
{
    TuiComposerImpl *impl = (TuiComposerImpl *)This;

    if (!OutSurface) return E_INVALIDARG;

    /* Create a new surface using the existing factory */
    return AnxTuiCreateSurface(Width, Height, OutSurface);
}

/* Helper: Compare surfaces by Z-order for sorting */
static int CompareSurfacesByZOrder(const void *a, const void *b)
{
    const RegisteredSurface *surfA = (const RegisteredSurface *)a;
    const RegisteredSurface *surfB = (const RegisteredSurface *)b;
    return surfA->ZOrder - surfB->ZOrder;
}

/* Composite surfaces to screen */
static HRESULT ANXAPI Composer_Composite(
    ITuiComposer *This,
    ITuiScreen *Screen
)
{
    TuiComposerImpl *impl = (TuiComposerImpl *)This;

    if (!Screen) return E_INVALIDARG;

    /* Sort surfaces by Z-order (back to front) */
    qsort(impl->Surfaces, impl->SurfaceCount, sizeof(RegisteredSurface), CompareSurfacesByZOrder);

    /* Composite each dirty surface */
    for (UINT32 i = 0; i < impl->SurfaceCount; i++) {
        RegisteredSurface *regSurf = &impl->Surfaces[i];

        if (!regSurf->Dirty || !regSurf->Surface) {
            continue;
        }

        /* Get widget's draw listener */
        ITuiResponder *responder = (ITuiResponder *)regSurf->Widget;
        ITuiDrawListener *drawListener = NULL;
        HRESULT hr = responder->Vtbl->QueryInterface(
            responder,
            &IID_ITuiDrawListener,
            (VOID **)&drawListener
        );

        if (SUCCEEDED(hr) && drawListener) {
            /* Request widget to draw itself to its surface */
            TUI_RECT dirtyRect = regSurf->Bounds;
            drawListener->Vtbl->OnDraw(drawListener, regSurf->Surface, &dirtyRect);
            drawListener->Vtbl->Release(drawListener);
        }

        /* Composite surface to screen */
        /* This would copy the surface pixels to the screen at the widget's bounds */
        /* For now, we'll just mark it as clean */
        regSurf->Dirty = FALSE;
    }

    /* Clear dirty regions */
    impl->DirtyRegionCount = 0;

    return S_OK;
}

/* Mark region as dirty */
static HRESULT ANXAPI Composer_MarkDirty(
    ITuiComposer *This,
    CONST TUI_RECT *Rect
)
{
    TuiComposerImpl *impl = (TuiComposerImpl *)This;

    if (!Rect) return E_INVALIDARG;

    /* Add to dirty regions */
    if (impl->DirtyRegionCount < MAX_SURFACES) {
        impl->DirtyRegions[impl->DirtyRegionCount++] = *Rect;
    }

    /* Mark all overlapping surfaces as dirty */
    for (UINT32 i = 0; i < impl->SurfaceCount; i++) {
        RegisteredSurface *surf = &impl->Surfaces[i];

        /* Check if surface bounds intersect with dirty rect */
        if (surf->Bounds.X < Rect->X + (INT32)Rect->Width &&
            surf->Bounds.X + (INT32)surf->Bounds.Width > Rect->X &&
            surf->Bounds.Y < Rect->Y + (INT32)Rect->Height &&
            surf->Bounds.Y + (INT32)surf->Bounds.Height > Rect->Y)
        {
            surf->Dirty = TRUE;
        }
    }

    return S_OK;
}

/* Register surface */
static HRESULT ANXAPI Composer_RegisterSurface(
    ITuiComposer *This,
    ITuiWidget *Widget,
    ITuiSurface *Surface,
    INT32 ZOrder
)
{
    TuiComposerImpl *impl = (TuiComposerImpl *)This;

    if (!Widget || !Surface) return E_INVALIDARG;

    if (impl->SurfaceCount >= MAX_SURFACES) {
        return E_OUTOFMEMORY;
    }

    RegisteredSurface *regSurf = &impl->Surfaces[impl->SurfaceCount++];
    regSurf->Widget = Widget;
    regSurf->Surface = Surface;
    regSurf->ZOrder = ZOrder;
    regSurf->Dirty = TRUE;

    /* Get widget bounds */
    Widget->Vtbl->GetBounds(Widget, &regSurf->Bounds);

    /* Add references */
    Surface->Vtbl->AddRef(Surface);
    ITuiResponder *responder = (ITuiResponder *)Widget;
    responder->Vtbl->AddRef(responder);

    return S_OK;
}

/* Unregister surface */
static HRESULT ANXAPI Composer_UnregisterSurface(
    ITuiComposer *This,
    ITuiWidget *Widget
)
{
    TuiComposerImpl *impl = (TuiComposerImpl *)This;

    if (!Widget) return E_INVALIDARG;

    /* Find and remove the surface */
    for (UINT32 i = 0; i < impl->SurfaceCount; i++) {
        if (impl->Surfaces[i].Widget == Widget) {
            /* Release references */
            if (impl->Surfaces[i].Surface) {
                impl->Surfaces[i].Surface->Vtbl->Release(impl->Surfaces[i].Surface);
            }
            ITuiResponder *responder = (ITuiResponder *)Widget;
            responder->Vtbl->Release(responder);

            /* Mark region as dirty */
            TUI_RECT rect = impl->Surfaces[i].Bounds;
            Composer_MarkDirty(This, &rect);

            /* Shift remaining surfaces */
            for (UINT32 j = i; j < impl->SurfaceCount - 1; j++) {
                impl->Surfaces[j] = impl->Surfaces[j + 1];
            }
            impl->SurfaceCount--;

            return S_OK;
        }
    }

    return E_INVALIDARG;
}

/* VTable */
static ITuiComposer_Vtbl ComposerVtbl = {
    Composer_QueryInterface,
    Composer_AddRef,
    Composer_Release,
    Composer_CreateSurface,
    Composer_Composite,
    Composer_MarkDirty,
    Composer_RegisterSurface,
    Composer_UnregisterSurface
};

/* Factory function */
HRESULT AnxTuiCreateComposer(ITuiComposer **OutComposer)
{
    TuiComposerImpl *impl;

    if (!OutComposer) return E_INVALIDARG;

    impl = (TuiComposerImpl *)malloc(sizeof(TuiComposerImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiComposerImpl));
    impl->Interface.Vtbl = &ComposerVtbl;
    impl->RefCount = 1;

    *OutComposer = &impl->Interface;
    return S_OK;
}
