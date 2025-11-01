/*++
    Module Name:

        bitmap.h

    Abstract:

        IFramebufferBitmap - Base interface for Screen, Surface, and Image.
        Provides common bitmap operations for blitting and pixel access.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/framebuffer.h>
#include <ananke/framebuffer/screen.h>

/* --------------------------------------------------------------- */
/*  IFramebufferBitmap Interface                                   */
/* --------------------------------------------------------------- */

DEFINE_GUID(IID_IFramebufferBitmap,
    0x7B3A1C40, 0x8D6E, 0x4F2A, 0x9B, 0x7C, 0x3E, 0x5F, 0x8A, 0x2D, 0x1B, 0x4E);

#undef INTERFACE
#define INTERFACE IFramebufferBitmap

DECLARE_INTERFACE_(IFramebufferBitmap, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, VOID **ppvObject) PURE;
    STDMETHOD_(UINT32, AddRef)(THIS) PURE;
    STDMETHOD_(UINT32, Release)(THIS) PURE;

    /* IFramebufferBitmap methods */

    /**
     * Get the dimensions of the bitmap.
     *
     * @param Width - Receives the width in pixels
     * @param Height - Receives the height in pixels
     * @return S_OK on success
     */
    STDMETHOD(GetDimensions)(THIS_
        OUT UINT32 *Width,
        OUT UINT32 *Height) PURE;

    /**
     * Get the pixel format of the bitmap.
     *
     * @param Format - Receives the pixel format
     * @return S_OK on success
     */
    STDMETHOD(GetPixelFormat)(THIS_
        OUT FB_PIXEL_FORMAT *Format) PURE;

    /**
     * Get the descriptor describing the bitmap's memory organization.
     *
     * @param Descriptor - Receives the framebuffer descriptor
     * @return S_OK on success
     */
    STDMETHOD(GetDescriptor)(THIS_
        OUT FRAMEBUFFER_DESC *Descriptor) PURE;

    /**
     * Blit from another bitmap to this bitmap with optional ROP2 operation.
     *
     * @param X - Destination X coordinate
     * @param Y - Destination Y coordinate
     * @param Width - Width of the region to blit
     * @param Height - Height of the region to blit
     * @param Source - Source bitmap
     * @param SourceX - Source X coordinate
     * @param SourceY - Source Y coordinate
     * @param Rop - ROP2 operation (use FbRop2CopyPen for simple copy)
     * @return S_OK on success, E_NOTIMPL if ROP not supported
     */
    STDMETHOD(Blit)(THIS_
        IN INT32 X,
        IN INT32 Y,
        IN UINT32 Width,
        IN UINT32 Height,
        IN IFramebufferBitmap *Source,
        IN INT32 SourceX,
        IN INT32 SourceY,
        IN FB_ROP2 Rop) PURE;

    /**
     * Blit with ROP3 operation (ternary raster operation with pattern).
     *
     * @param X - Destination X coordinate
     * @param Y - Destination Y coordinate
     * @param Width - Width of the region to blit
     * @param Height - Height of the region to blit
     * @param Source - Source bitmap
     * @param SourceX - Source X coordinate
     * @param SourceY - Source Y coordinate
     * @param Pattern - Pattern bitmap (can be NULL)
     * @param PatternX - Pattern X offset
     * @param PatternY - Pattern Y offset
     * @param Rop - ROP3 operation
     * @return S_OK on success, E_NOTIMPL if ROP3 not supported
     */
    STDMETHOD(BlitRop3)(THIS_
        IN INT32 X,
        IN INT32 Y,
        IN UINT32 Width,
        IN UINT32 Height,
        IN IFramebufferBitmap *Source,
        IN INT32 SourceX,
        IN INT32 SourceY,
        IN IFramebufferBitmap *Pattern,
        IN INT32 PatternX,
        IN INT32 PatternY,
        IN FB_ROP3 Rop) PURE;

    /**
     * Fill a rectangular region with a solid color.
     *
     * @param Rect - Rectangle to fill
     * @param Color - Fill color
     * @return S_OK on success
     */
    STDMETHOD(FillRect)(THIS_
        IN CONST FB_RECT *Rect,
        IN FB_COLOR Color) PURE;

    /**
     * Set a single pixel.
     *
     * @param X - X coordinate
     * @param Y - Y coordinate
     * @param Color - Pixel color
     * @return S_OK on success
     */
    STDMETHOD(SetPixel)(THIS_
        IN INT32 X,
        IN INT32 Y,
        IN FB_COLOR Color) PURE;

    /**
     * Get a single pixel.
     *
     * @param X - X coordinate
     * @param Y - Y coordinate
     * @param Color - Receives the pixel color
     * @return S_OK on success
     */
    STDMETHOD(GetPixel)(THIS_
        IN INT32 X,
        IN INT32 Y,
        OUT FB_COLOR *Color) PURE;

    /**
     * Lock the bitmap for direct memory access.
     * Returns a pointer to the bitmap data and the pitch (bytes per scanline).
     *
     * @param Data - Receives pointer to bitmap data
     * @param Pitch - Receives pitch in bytes
     * @return S_OK on success, E_FAIL if cannot be locked
     */
    STDMETHOD(Lock)(THIS_
        OUT VOID **Data,
        OUT UINT32 *Pitch) PURE;

    /**
     * Unlock the bitmap after direct memory access.
     *
     * @return S_OK on success
     */
    STDMETHOD(Unlock)(THIS) PURE;
};

#undef INTERFACE

typedef struct IFramebufferBitmapVtbl {
    /* IUnknown */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IFramebufferBitmap *This, REFIID riid, VOID **ppvObject);
    UINT32 (STDMETHODCALLTYPE *AddRef)(IFramebufferBitmap *This);
    UINT32 (STDMETHODCALLTYPE *Release)(IFramebufferBitmap *This);

    /* IFramebufferBitmap */
    HRESULT (STDMETHODCALLTYPE *GetDimensions)(IFramebufferBitmap *This, UINT32 *Width, UINT32 *Height);
    HRESULT (STDMETHODCALLTYPE *GetPixelFormat)(IFramebufferBitmap *This, FB_PIXEL_FORMAT *Format);
    HRESULT (STDMETHODCALLTYPE *GetDescriptor)(IFramebufferBitmap *This, FRAMEBUFFER_DESC *Descriptor);
    HRESULT (STDMETHODCALLTYPE *Blit)(IFramebufferBitmap *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
                                      IFramebufferBitmap *Source, INT32 SourceX, INT32 SourceY, FB_ROP2 Rop);
    HRESULT (STDMETHODCALLTYPE *BlitRop3)(IFramebufferBitmap *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
                                          IFramebufferBitmap *Source, INT32 SourceX, INT32 SourceY,
                                          IFramebufferBitmap *Pattern, INT32 PatternX, INT32 PatternY, FB_ROP3 Rop);
    HRESULT (STDMETHODCALLTYPE *FillRect)(IFramebufferBitmap *This, CONST FB_RECT *Rect, FB_COLOR Color);
    HRESULT (STDMETHODCALLTYPE *SetPixel)(IFramebufferBitmap *This, INT32 X, INT32 Y, FB_COLOR Color);
    HRESULT (STDMETHODCALLTYPE *GetPixel)(IFramebufferBitmap *This, INT32 X, INT32 Y, FB_COLOR *Color);
    HRESULT (STDMETHODCALLTYPE *Lock)(IFramebufferBitmap *This, VOID **Data, UINT32 *Pitch);
    HRESULT (STDMETHODCALLTYPE *Unlock)(IFramebufferBitmap *This);
} IFramebufferBitmapVtbl;

struct IFramebufferBitmap {
    CONST IFramebufferBitmapVtbl *lpVtbl;
};
