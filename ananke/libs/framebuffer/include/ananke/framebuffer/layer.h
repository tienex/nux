/*++
    Module Name:

        layer.h

    Abstract:

        Layer objects for off-screen rendering and compositing.
        Similar to Core Graphics CGLayer.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>
#include <ananke/framebuffer.h>

/* Forward declarations */
typedef struct IFramebuffer2DContext IFramebuffer2DContext;

/* --------------------------------------------------------------- */
/*  IFramebufferLayer - Layer Object (like CGLayer)                */
/* --------------------------------------------------------------- */

/*
 * Layer object for off-screen rendering.
 * Optimized for repeated drawing of the same content.
 *
 * Similar to CGLayer in Core Graphics. Layers are useful for:
 * - Caching rendered content for reuse
 * - Creating complex compositions
 * - Building up images incrementally
 * - Pattern tiles
 *
 * Unlike surfaces, layers are device-dependent and optimized
 * for the specific graphics context they're created with.
 */

#define ANX_IID_IFramebufferLayer "FB000025-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferLayer,
    0xFB000025, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferLayer, IUnknown,
    IID_IFramebufferLayer, ANX_IID_IFramebufferLayer)

    /* Get layer dimensions */
    ANX_IFACE_METHOD(HRESULT, GetSize, (
        OUT UINT32 *Width,
        OUT UINT32 *Height))

    /* Get graphics context for drawing into the layer
     * Similar to CGLayerGetContext()
     */
    ANX_IFACE_METHOD(HRESULT, GetContext, (
        OUT IFramebuffer2DContext **Context))

    /* Clear layer contents */
    ANX_IFACE_METHOD(HRESULT, Clear, (
        IN FB_COLOR Color))

    /* Get underlying surface (if layer is backed by a surface) */
    ANX_IFACE_METHOD(HRESULT, GetSurface, (
        OUT IFramebufferSurface **Surface))

ANX_END_INTERFACE(IFramebufferLayer)

/* --------------------------------------------------------------- */
/*  Factory Functions                                              */
/* --------------------------------------------------------------- */

/*
 * Create layer in context with specified size.
 *
 * Similar to CGLayerCreateWithContext().
 *
 * The layer is optimized for drawing into the specified context.
 * Drawing the same content multiple times is more efficient when
 * rendered into a layer first.
 *
 * Parameters:
 *   Context - Graphics context to optimize layer for
 *   Width - Layer width in pixels
 *   Height - Layer height in pixels
 *
 * Returns:
 *   New layer object, or NULL on failure
 */
IFramebufferLayer *
FbCreateLayer(
    IN IFramebuffer2DContext *Context,
    IN UINT32 Width,
    IN UINT32 Height
    );

/*
 * Create layer from existing surface.
 *
 * Wraps a surface as a layer for compositing.
 */
IFramebufferLayer *
FbCreateLayerFromSurface(
    IN IFramebufferSurface *Surface
    );
