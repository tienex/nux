/*++
    Module Name:

        image.h

    Abstract:

        Image interface for blitting with automatic format conversion.
        Supports blitting from/to screens, surfaces, and memory buffers.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>
#include <ananke/framebuffer.h>
#include <ananke/framebuffer/screen.h>

/* --------------------------------------------------------------- */
/*  Image Source Types                                              */
/* --------------------------------------------------------------- */

typedef enum _FB_IMAGE_SOURCE {
    FbImageSourceMemory     = 0,  /* From raw memory buffer */
    FbImageSourceScreen     = 1,  /* From screen */
    FbImageSourceSurface    = 2,  /* From surface */
} FB_IMAGE_SOURCE;

/* --------------------------------------------------------------- */
/*  IFramebufferImage - Image blitting interface                    */
/* --------------------------------------------------------------- */

#define ANX_IID_IFramebufferImage "FB000013-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferImage,
    0xFB000013, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferImage, IUnknown,
    IID_IFramebufferImage, ANX_IID_IFramebufferImage)

    /* Get image dimensions and format */
    ANX_IFACE_METHOD(HRESULT, GetInfo, (
        OUT UINT32 *Width,
        OUT UINT32 *Height,
        OUT FB_PIXEL_FORMAT *PixelFormat))

    /* Set source to memory buffer */
    ANX_IFACE_METHOD(HRESULT, SetMemorySource, (
        IN CONST VOID *Data,
        IN UINT32 Width,
        IN UINT32 Height,
        IN UINT32 Pitch,
        IN FB_PIXEL_FORMAT PixelFormat))

    /* Set source to screen region */
    ANX_IFACE_METHOD(HRESULT, SetScreenSource, (
        IN IFramebufferScreen *Screen,
        IN CONST FB_RECT *Rect))

    /* Set source to surface region */
    ANX_IFACE_METHOD(HRESULT, SetSurfaceSource, (
        IN IFramebufferSurface *Surface,
        IN CONST FB_RECT *Rect))

    /* Blit to screen with optional format conversion */
    ANX_IFACE_METHOD(HRESULT, BlitToScreen, (
        IN IFramebufferScreen *Screen,
        IN INT32 DestX,
        IN INT32 DestY,
        IN CONST FB_RECT *SourceRect,
        IN FB_ROP Rop))

    /* Blit to surface with optional format conversion */
    ANX_IFACE_METHOD(HRESULT, BlitToSurface, (
        IN IFramebufferSurface *Surface,
        IN INT32 DestX,
        IN INT32 DestY,
        IN CONST FB_RECT *SourceRect,
        IN FB_ROP Rop))

    /* Blit to memory buffer with format conversion */
    ANX_IFACE_METHOD(HRESULT, BlitToMemory, (
        OUT VOID *DestBuffer,
        IN UINT32 DestPitch,
        IN FB_PIXEL_FORMAT DestFormat,
        IN CONST FB_RECT *SourceRect))

    /* Transform to a different pixel format (in-place or to new image) */
    ANX_IFACE_METHOD(HRESULT, Transform, (
        IN FB_PIXEL_FORMAT TargetFormat,
        OUT IFramebufferImage **TransformedImage))

    /* Set dithering method for format conversions */
    ANX_IFACE_METHOD(HRESULT, SetDitherMethod, (
        IN FB_DITHER_METHOD Method))

    /* Set transparent color key for blitting */
    ANX_IFACE_METHOD(HRESULT, SetColorKey, (
        IN BOOLEAN Enable,
        IN FB_COLOR ColorKey))

ANX_END_INTERFACE(IFramebufferImage)

/* --------------------------------------------------------------- */
/*  Helper Macros for C (COBJMACROS style)                          */
/* --------------------------------------------------------------- */

#ifndef __cplusplus

#define IFramebufferImage_GetInfo(This, W, H, Fmt) \
    ((This)->lpVtbl->GetInfo(This, W, H, Fmt))
#define IFramebufferImage_SetMemorySource(This, Data, W, H, Pitch, Fmt) \
    ((This)->lpVtbl->SetMemorySource(This, Data, W, H, Pitch, Fmt))
#define IFramebufferImage_SetScreenSource(This, Screen, Rect) \
    ((This)->lpVtbl->SetScreenSource(This, Screen, Rect))
#define IFramebufferImage_SetSurfaceSource(This, Surface, Rect) \
    ((This)->lpVtbl->SetSurfaceSource(This, Surface, Rect))
#define IFramebufferImage_BlitToScreen(This, Screen, X, Y, SrcRect, Rop) \
    ((This)->lpVtbl->BlitToScreen(This, Screen, X, Y, SrcRect, Rop))
#define IFramebufferImage_BlitToSurface(This, Surface, X, Y, SrcRect, Rop) \
    ((This)->lpVtbl->BlitToSurface(This, Surface, X, Y, SrcRect, Rop))
#define IFramebufferImage_BlitToMemory(This, Dest, Pitch, Fmt, SrcRect) \
    ((This)->lpVtbl->BlitToMemory(This, Dest, Pitch, Fmt, SrcRect))
#define IFramebufferImage_Transform(This, Fmt, Out) \
    ((This)->lpVtbl->Transform(This, Fmt, Out))
#define IFramebufferImage_SetDitherMethod(This, Method) \
    ((This)->lpVtbl->SetDitherMethod(This, Method))
#define IFramebufferImage_SetColorKey(This, Enable, Key) \
    ((This)->lpVtbl->SetColorKey(This, Enable, Key))

#endif /* !__cplusplus */

/* --------------------------------------------------------------- */
/*  Factory Function                                                */
/* --------------------------------------------------------------- */

/*
 * Create an image object from a memory buffer.
 */
IFramebufferImage *
FbCreateImageFromMemory(
    IN CONST VOID *Data,
    IN UINT32 Width,
    IN UINT32 Height,
    IN UINT32 Pitch,
    IN FB_PIXEL_FORMAT PixelFormat
    );

/*
 * Create an image object from a screen region.
 */
IFramebufferImage *
FbCreateImageFromScreen(
    IN IFramebufferScreen *Screen,
    IN CONST FB_RECT *Rect
    );

/*
 * Create an image object from a surface region.
 */
IFramebufferImage *
FbCreateImageFromSurface(
    IN IFramebufferSurface *Surface,
    IN CONST FB_RECT *Rect
    );
