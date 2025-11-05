/*++
    Module Name:

        layer.c

    Abstract:

        Implementation of IFramebufferLayer for off-screen rendering.
        Similar to Core Graphics CGLayer.

    Environment:

        C implementation.
--*/

#include <ananke/framebuffer/layer.h>
#include <ananke/framebuffer/graphics2d.h>
#include <ananke/framebuffer/surface.h>
#include <ananke/memory.h>

/* --------------------------------------------------------------- */
/*  IFramebufferLayer Implementation                               */
/* --------------------------------------------------------------- */

typedef struct _FB_LAYER_IMPL {
    IFramebufferLayer       Base;
    REFOBJ                  RefCount;

    /* Layer dimensions */
    UINT32                  Width;
    UINT32                  Height;

    /* Backing surface */
    IFramebufferSurface     *Surface;
    BOOLEAN                 OwnsSurface;

    /* Graphics context for drawing into layer */
    IFramebuffer2DContext   *Context;
} FB_LAYER_IMPL;

/* --------------------------------------------------------------- */
/*  IUnknown Methods                                               */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbLayer_QueryInterface(
    IFramebufferLayer *This,
    REFIID Riid,
    VOID **Object
    )
{
    if (AnxIsEqualGuid(Riid, &IID_IUnknown) ||
        AnxIsEqualGuid(Riid, &IID_IFramebufferLayer)) {
        IUnknown_AddRef((IUnknown *)This);
        *Object = This;
        return S_OK;
    }

    *Object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE
FbLayer_AddRef(
    IFramebufferLayer *This
    )
{
    FB_LAYER_IMPL *Layer = (FB_LAYER_IMPL *)This;
    return AnxInterlockedIncrement(&Layer->RefCount);
}

static ULONG STDMETHODCALLTYPE
FbLayer_Release(
    IFramebufferLayer *This
    )
{
    FB_LAYER_IMPL *Layer = (FB_LAYER_IMPL *)This;
    ULONG RefCount = AnxInterlockedDecrement(&Layer->RefCount);

    if (RefCount == 0) {
        if (Layer->Context != NULL) {
            IUnknown_Release((IUnknown *)Layer->Context);
        }
        if (Layer->Surface != NULL && Layer->OwnsSurface) {
            IUnknown_Release((IUnknown *)Layer->Surface);
        }
        AnxFree(Layer);
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  IFramebufferLayer Methods                                      */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbLayer_GetSize(
    IFramebufferLayer *This,
    UINT32 *Width,
    UINT32 *Height
    )
{
    FB_LAYER_IMPL *Layer = (FB_LAYER_IMPL *)This;

    if (Width != NULL) {
        *Width = Layer->Width;
    }
    if (Height != NULL) {
        *Height = Layer->Height;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbLayer_GetContext(
    IFramebufferLayer *This,
    IFramebuffer2DContext **Context
    )
{
    FB_LAYER_IMPL *Layer = (FB_LAYER_IMPL *)This;

    if (Context == NULL) {
        return E_POINTER;
    }

    if (Layer->Context == NULL) {
        return E_FAIL;
    }

    IUnknown_AddRef((IUnknown *)Layer->Context);
    *Context = Layer->Context;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbLayer_Clear(
    IFramebufferLayer *This,
    FB_COLOR Color
    )
{
    FB_LAYER_IMPL *Layer = (FB_LAYER_IMPL *)This;

    if (Layer->Context == NULL) {
        return E_FAIL;
    }

    return IFramebuffer2DContext_Clear(Layer->Context, Color);
}

static HRESULT STDMETHODCALLTYPE
FbLayer_GetSurface(
    IFramebufferLayer *This,
    IFramebufferSurface **Surface
    )
{
    FB_LAYER_IMPL *Layer = (FB_LAYER_IMPL *)This;

    if (Surface == NULL) {
        return E_POINTER;
    }

    if (Layer->Surface == NULL) {
        return E_FAIL;
    }

    IUnknown_AddRef((IUnknown *)Layer->Surface);
    *Surface = Layer->Surface;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  VTable                                                         */
/* --------------------------------------------------------------- */

static IFramebufferLayerVtbl FbLayer_Vtbl = {
    /* IUnknown */
    FbLayer_QueryInterface,
    FbLayer_AddRef,
    FbLayer_Release,

    /* IFramebufferLayer */
    FbLayer_GetSize,
    FbLayer_GetContext,
    FbLayer_Clear,
    FbLayer_GetSurface,
};

/* --------------------------------------------------------------- */
/*  Factory Functions                                              */
/* --------------------------------------------------------------- */

IFramebufferLayer *
FbCreateLayer(
    IN IFramebuffer2DContext *Context,
    IN UINT32 Width,
    IN UINT32 Height
    )
{
    if (Context == NULL || Width == 0 || Height == 0) {
        return NULL;
    }

    /* Get the context's surface to determine pixel format */
    IFramebufferSurface *ContextSurface = NULL;
    HRESULT Hr = IFramebuffer2DContext_GetSurface(Context, &ContextSurface);
    if (FAILED(Hr)) {
        return NULL;
    }

    FB_SURFACE_DESC SurfaceDesc;
    Hr = IFramebufferSurface_GetDescriptor(ContextSurface, &SurfaceDesc);
    IUnknown_Release((IUnknown *)ContextSurface);

    if (FAILED(Hr)) {
        return NULL;
    }

    /* Create layer surface with same format as context */
    IFramebufferSurface *LayerSurface = FbCreateSurface(
        Width, Height, SurfaceDesc.PixelFormat);

    if (LayerSurface == NULL) {
        return NULL;
    }

    /* Create graphics context for the layer */
    IFramebuffer2DContext *LayerContext = FbCreate2DContext(LayerSurface);
    if (LayerContext == NULL) {
        IUnknown_Release((IUnknown *)LayerSurface);
        return NULL;
    }

    /* Create layer object */
    FB_LAYER_IMPL *Layer = (FB_LAYER_IMPL *)AnxAllocate(sizeof(FB_LAYER_IMPL));
    if (Layer == NULL) {
        IUnknown_Release((IUnknown *)LayerContext);
        IUnknown_Release((IUnknown *)LayerSurface);
        return NULL;
    }

    AnxZeroMemory(Layer, sizeof(FB_LAYER_IMPL));

    Layer->Base.lpVtbl = &FbLayer_Vtbl;
    Layer->RefCount = 1;
    Layer->Width = Width;
    Layer->Height = Height;
    Layer->Surface = LayerSurface;
    Layer->OwnsSurface = TRUE;
    Layer->Context = LayerContext;

    return (IFramebufferLayer *)Layer;
}

IFramebufferLayer *
FbCreateLayerFromSurface(
    IN IFramebufferSurface *Surface
    )
{
    if (Surface == NULL) {
        return NULL;
    }

    /* Get surface dimensions */
    FB_SURFACE_DESC Desc;
    HRESULT Hr = IFramebufferSurface_GetDescriptor(Surface, &Desc);
    if (FAILED(Hr)) {
        return NULL;
    }

    /* Create graphics context for the surface */
    IFramebuffer2DContext *LayerContext = FbCreate2DContext(Surface);
    if (LayerContext == NULL) {
        return NULL;
    }

    /* Create layer object */
    FB_LAYER_IMPL *Layer = (FB_LAYER_IMPL *)AnxAllocate(sizeof(FB_LAYER_IMPL));
    if (Layer == NULL) {
        IUnknown_Release((IUnknown *)LayerContext);
        return NULL;
    }

    AnxZeroMemory(Layer, sizeof(FB_LAYER_IMPL));

    Layer->Base.lpVtbl = &FbLayer_Vtbl;
    Layer->RefCount = 1;
    Layer->Width = Desc.Width;
    Layer->Height = Desc.Height;

    /* Don't own the surface - caller owns it */
    IUnknown_AddRef((IUnknown *)Surface);
    Layer->Surface = Surface;
    Layer->OwnsSurface = FALSE;
    Layer->Context = LayerContext;

    return (IFramebufferLayer *)Layer;
}
