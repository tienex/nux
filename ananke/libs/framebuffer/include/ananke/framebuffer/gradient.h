/*++
    Module Name:

        gradient.h

    Abstract:

        Gradient objects for smooth color transitions.
        Similar to Core Graphics CGGradient.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>
#include <ananke/framebuffer.h>

/* --------------------------------------------------------------- */
/*  Gradient Types                                                 */
/* --------------------------------------------------------------- */

/* Gradient drawing options */
typedef enum _FB_GRADIENT_DRAWING_OPTIONS {
    FbGradientDrawsBeforeStartLocation  = (1 << 0),  /* Extend gradient before start */
    FbGradientDrawsAfterEndLocation     = (1 << 1),  /* Extend gradient after end */
} FB_GRADIENT_DRAWING_OPTIONS;

/* --------------------------------------------------------------- */
/*  IFramebufferGradient - Gradient Object (like CGGradient)       */
/* --------------------------------------------------------------- */

/*
 * Gradient object that defines smooth color transitions.
 * Can be used for linear or radial gradients.
 *
 * Similar to CGGradient in Core Graphics.
 */

#define ANX_IID_IFramebufferGradient "FB000023-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferGradient,
    0xFB000023, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferGradient, IUnknown,
    IID_IFramebufferGradient, ANX_IID_IFramebufferGradient)

    /* Get gradient information */
    ANX_IFACE_METHOD(HRESULT, GetStopCount, (
        OUT UINT32 *Count))

    /* Get color at specific location (0.0 to 1.0) */
    ANX_IFACE_METHOD(HRESULT, GetColorAtLocation, (
        IN FLOAT Location,
        OUT FB_COLOR *Color))

    /* Get all gradient stops */
    ANX_IFACE_METHOD(HRESULT, GetStops, (
        OUT FB_GRADIENT_STOP *Stops,
        IN UINT32 MaxStops,
        OUT UINT32 *StopCount))

ANX_END_INTERFACE(IFramebufferGradient)

/* --------------------------------------------------------------- */
/*  IFramebufferShading - Shading Object (like CGShading)          */
/* --------------------------------------------------------------- */

/*
 * Shading object for procedural color generation.
 * More flexible than gradients - can generate colors based on position.
 *
 * Similar to CGShading in Core Graphics.
 */

#define ANX_IID_IFramebufferShading "FB000024-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferShading,
    0xFB000024, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

/* Shading callback function - computes color at given position */
typedef VOID (CALLBACK *FB_SHADING_CALLBACK)(
    IN VOID *UserData,
    IN CONST FLOAT *Position,    /* [x, y] for axial, [x, y, r] for radial */
    OUT FB_COLOR *Color
    );

ANX_BEGIN_INTERFACE(IFramebufferShading, IUnknown,
    IID_IFramebufferShading, ANX_IID_IFramebufferShading)

    /* Evaluate shading at specific position */
    ANX_IFACE_METHOD(HRESULT, Evaluate, (
        IN CONST FLOAT *Position,
        OUT FB_COLOR *Color))

ANX_END_INTERFACE(IFramebufferShading)

/* --------------------------------------------------------------- */
/*  Factory Functions                                              */
/* --------------------------------------------------------------- */

/*
 * Create gradient with color stops.
 *
 * Similar to CGGradientCreateWithColorComponents or
 * CGGradientCreateWithColors.
 *
 * Parameters:
 *   Colors - Array of colors for gradient stops
 *   Locations - Array of locations (0.0 to 1.0), or NULL for evenly spaced
 *   Count - Number of color stops
 */
IFramebufferGradient *
FbCreateGradient(
    IN CONST FB_COLOR *Colors,
    IN CONST FLOAT *Locations,
    IN UINT32 Count
    );

/*
 * Create axial (linear) shading.
 *
 * Similar to CGShadingCreateAxial.
 */
IFramebufferShading *
FbCreateAxialShading(
    IN FB_SHADING_CALLBACK Callback,
    IN VOID *UserData,
    IN FLOAT StartX,
    IN FLOAT StartY,
    IN FLOAT EndX,
    IN FLOAT EndY,
    IN BOOLEAN ExtendStart,
    IN BOOLEAN ExtendEnd
    );

/*
 * Create radial shading.
 *
 * Similar to CGShadingCreateRadial.
 */
IFramebufferShading *
FbCreateRadialShading(
    IN FB_SHADING_CALLBACK Callback,
    IN VOID *UserData,
    IN FLOAT StartX,
    IN FLOAT StartY,
    IN FLOAT StartRadius,
    IN FLOAT EndX,
    IN FLOAT EndY,
    IN FLOAT EndRadius,
    IN BOOLEAN ExtendStart,
    IN BOOLEAN ExtendEnd
    );

/*
 * Create function-based shading.
 *
 * Similar to CGShadingCreateWithFunction.
 */
IFramebufferShading *
FbCreateFunctionShading(
    IN FB_SHADING_CALLBACK Callback,
    IN VOID *UserData
    );
