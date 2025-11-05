/*++
    Module Name:

        engine.h

    Abstract:

        Framebuffer engine that provides software emulation for features
        not implemented by the hardware backend.

        The engine sits between the user-facing interfaces (IFramebufferScreen,
        IFramebufferSurface, IFramebufferImage) and the backend implementation.
        It automatically falls back to software implementation when the backend
        doesn't support a feature.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/hresult.h>
#include <ananke/framebuffer.h>
#include <ananke/framebuffer/backend_ext.h>
#include <ananke/framebuffer/screen.h>

/* --------------------------------------------------------------- */
/*  Engine Context                                                  */
/* --------------------------------------------------------------- */

typedef struct _FB_ENGINE_CONTEXT FB_ENGINE_CONTEXT;

/* --------------------------------------------------------------- */
/*  Engine Creation                                                 */
/* --------------------------------------------------------------- */

/*
 * Create an engine context wrapping a backend.
 * The engine will query the backend's capabilities and provide
 * software emulation for unsupported features.
 */
FB_ENGINE_CONTEXT *
FbCreateEngine(
    IN IFramebufferBackend *Backend
    );

/*
 * Destroy an engine context.
 */
VOID
FbDestroyEngine(
    IN FB_ENGINE_CONTEXT *Engine
    );

/* --------------------------------------------------------------- */
/*  Engine Operations - Software Fallbacks                          */
/* --------------------------------------------------------------- */

/*
 * Fill rectangle with ROP operation.
 * Tries hardware acceleration first, falls back to software.
 */
HRESULT
FbEngineFillRect(
    IN FB_ENGINE_CONTEXT *Engine,
    IN CONST FB_RECT *Rect,
    IN FB_COLOR Color,
    IN FB_ROP Rop
    );

/*
 * Blit with pixel format conversion and ROP.
 * Automatically converts between pixel formats and applies dithering.
 */
HRESULT
FbEngineBlit(
    IN FB_ENGINE_CONTEXT *Engine,
    IN INT32 DestX,
    IN INT32 DestY,
    IN CONST VOID *SourceData,
    IN UINT32 SourceWidth,
    IN UINT32 SourceHeight,
    IN UINT32 SourcePitch,
    IN FB_PIXEL_FORMAT SourceFormat,
    IN FB_PIXEL_FORMAT DestFormat,
    IN CONST FB_RECT *SourceRect,
    IN FB_ROP Rop
    );

/*
 * Convert pixel format from source to destination.
 * Applies dithering when converting to lower bit depth.
 */
HRESULT
FbEngineConvertFormat(
    IN CONST VOID *SourceData,
    IN FB_PIXEL_FORMAT SourceFormat,
    IN UINT32 SourceWidth,
    IN UINT32 SourceHeight,
    IN UINT32 SourcePitch,
    OUT VOID *DestData,
    IN FB_PIXEL_FORMAT DestFormat,
    IN UINT32 DestPitch,
    IN FB_DITHER_METHOD DitherMethod
    );

/*
 * Apply ROP operation between source and destination pixels.
 */
UINT32
FbEngineApplyRop(
    IN UINT32 Source,
    IN UINT32 Dest,
    IN FB_ROP Rop
    );

/*
 * Software cursor rendering.
 * Used when hardware cursor is not available.
 */
HRESULT
FbEngineDrawCursor(
    IN FB_ENGINE_CONTEXT *Engine,
    IN INT32 X,
    IN INT32 Y,
    IN CONST UINT8 *CursorData,
    IN UINT32 Width,
    IN UINT32 Height,
    IN INT32 HotSpotX,
    IN INT32 HotSpotY,
    IN FB_CURSOR_TYPE Type
    );

/* --------------------------------------------------------------- */
/*  Pixel Format Utilities                                          */
/* --------------------------------------------------------------- */

/*
 * Get bytes per pixel for a pixel format.
 */
UINT32
FbGetBytesPerPixel(
    IN FB_PIXEL_FORMAT Format
    );

/*
 * Get bits per pixel for a pixel format.
 */
UINT32
FbGetBitsPerPixel(
    IN FB_PIXEL_FORMAT Format
    );

/*
 * Check if a pixel format is planar.
 */
BOOLEAN
FbIsFormatPlanar(
    IN FB_PIXEL_FORMAT Format
    );

/*
 * Check if a pixel format is indexed.
 */
BOOLEAN
FbIsFormatIndexed(
    IN FB_PIXEL_FORMAT Format
    );

/*
 * Get number of planes for planar formats.
 */
UINT32
FbGetNumPlanes(
    IN FB_PIXEL_FORMAT Format
    );
