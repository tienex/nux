/*++
    Module Name:

        image.c

    Abstract:

        IFramebufferImage implementation.

        Provides a flexible image abstraction that can wrap memory buffers,
        screen regions, or surface regions as a source. Supports blitting
        to any destination with automatic format conversion, dithering,
        color keying, and ROP operations.

--*/

#include <ananke/framebuffer/image.h>
#include <ananke/framebuffer/engine.h>
#include <ananke/framebuffer/backends.h>
#include <ananke/framebuffer/com_helpers.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Image Implementation Structure                                 */
/* --------------------------------------------------------------- */

typedef struct _FB_IMAGE_IMPL {
    IFramebufferImage       Base;
    REFOBJ                  RefCount;

    /* Source information */
    FB_IMAGE_SOURCE         SourceType;
    UINT32                  Width;
    UINT32                  Height;
    FB_PIXEL_FORMAT         PixelFormat;

    /* Memory source */
    CONST VOID              *MemoryData;
    UINT32                  MemoryPitch;
    BOOLEAN                 OwnsMemory;

    /* Screen/Surface source */
    IFramebufferScreen      *SourceScreen;
    IFramebufferSurface     *SourceSurface;
    FB_RECT                 SourceRect;

    /* Rendering options */
    FB_DITHER_METHOD        DitherMethod;
    BOOLEAN                 ColorKeyEnabled;
    FB_COLOR                ColorKey;
} FB_IMAGE_IMPL;

/* --------------------------------------------------------------- */
/*  Forward Declarations                                            */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE FbImage_QueryInterface(
    IFramebufferImage *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE FbImage_AddRef(IFramebufferImage *This);
static UINT32 STDMETHODCALLTYPE FbImage_Release(IFramebufferImage *This);
static HRESULT STDMETHODCALLTYPE FbImage_GetInfo(
    IFramebufferImage *This, UINT32 *Width, UINT32 *Height, FB_PIXEL_FORMAT *PixelFormat);
static HRESULT STDMETHODCALLTYPE FbImage_SetMemorySource(
    IFramebufferImage *This, CONST VOID *Data, UINT32 Width, UINT32 Height,
    UINT32 Pitch, FB_PIXEL_FORMAT PixelFormat);
static HRESULT STDMETHODCALLTYPE FbImage_SetScreenSource(
    IFramebufferImage *This, IFramebufferScreen *Screen, CONST FB_RECT *Rect);
static HRESULT STDMETHODCALLTYPE FbImage_SetSurfaceSource(
    IFramebufferImage *This, IFramebufferSurface *Surface, CONST FB_RECT *Rect);
static HRESULT STDMETHODCALLTYPE FbImage_BlitToScreen(
    IFramebufferImage *This, IFramebufferScreen *Screen, INT32 DestX, INT32 DestY,
    CONST FB_RECT *SourceRect, FB_ROP Rop);
static HRESULT STDMETHODCALLTYPE FbImage_BlitToSurface(
    IFramebufferImage *This, IFramebufferSurface *Surface, INT32 DestX, INT32 DestY,
    CONST FB_RECT *SourceRect, FB_ROP Rop);
static HRESULT STDMETHODCALLTYPE FbImage_BlitToMemory(
    IFramebufferImage *This, VOID *DestBuffer, UINT32 DestPitch,
    FB_PIXEL_FORMAT DestFormat, CONST FB_RECT *SourceRect);
static HRESULT STDMETHODCALLTYPE FbImage_Transform(
    IFramebufferImage *This, FB_PIXEL_FORMAT TargetFormat, IFramebufferImage **TransformedImage);
static HRESULT STDMETHODCALLTYPE FbImage_SetDitherMethod(
    IFramebufferImage *This, FB_DITHER_METHOD Method);
static HRESULT STDMETHODCALLTYPE FbImage_SetColorKey(
    IFramebufferImage *This, BOOLEAN Enable, FB_COLOR ColorKey);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferImageVtbl gImageVtbl = {
    .QueryInterface     = FbImage_QueryInterface,
    .AddRef             = FbImage_AddRef,
    .Release            = FbImage_Release,
    .GetInfo            = FbImage_GetInfo,
    .SetMemorySource    = FbImage_SetMemorySource,
    .SetScreenSource    = FbImage_SetScreenSource,
    .SetSurfaceSource   = FbImage_SetSurfaceSource,
    .BlitToScreen       = FbImage_BlitToScreen,
    .BlitToSurface      = FbImage_BlitToSurface,
    .BlitToMemory       = FbImage_BlitToMemory,
    .Transform          = FbImage_Transform,
    .SetDitherMethod    = FbImage_SetDitherMethod,
    .SetColorKey        = FbImage_SetColorKey,
};

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

FB_IMPLEMENT_IUNKNOWN(FbImage, FB_IMAGE_IMPL, IFramebufferImage, IID_IFramebufferImage)

/* --------------------------------------------------------------- */
/*  IFramebufferImage Implementation                                */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbImage_GetInfo(
    IFramebufferImage *This,
    UINT32 *Width,
    UINT32 *Height,
    FB_PIXEL_FORMAT *PixelFormat
    )
{
    FB_IMAGE_IMPL *Image = (FB_IMAGE_IMPL *)This;

    if (Width != NULL) {
        *Width = Image->Width;
    }
    if (Height != NULL) {
        *Height = Image->Height;
    }
    if (PixelFormat != NULL) {
        *PixelFormat = Image->PixelFormat;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbImage_SetMemorySource(
    IFramebufferImage *This,
    CONST VOID *Data,
    UINT32 Width,
    UINT32 Height,
    UINT32 Pitch,
    FB_PIXEL_FORMAT PixelFormat
    )
{
    FB_IMAGE_IMPL *Image = (FB_IMAGE_IMPL *)This;

    if (Data == NULL) {
        return E_POINTER;
    }

    if (Width == 0 || Height == 0) {
        return E_INVALIDARG;
    }

    /* Release previous sources */
    if (Image->SourceScreen != NULL) {
        IUnknown_Release((IUnknown *)Image->SourceScreen);
        Image->SourceScreen = NULL;
    }
    if (Image->SourceSurface != NULL) {
        IUnknown_Release((IUnknown *)Image->SourceSurface);
        Image->SourceSurface = NULL;
    }

    /* Set memory source */
    Image->SourceType = FbImageSourceMemory;
    Image->MemoryData = Data;
    Image->MemoryPitch = Pitch;
    Image->Width = Width;
    Image->Height = Height;
    Image->PixelFormat = PixelFormat;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbImage_SetScreenSource(
    IFramebufferImage *This,
    IFramebufferScreen *Screen,
    CONST FB_RECT *Rect
    )
{
    FB_IMAGE_IMPL *Image = (FB_IMAGE_IMPL *)This;
    FRAMEBUFFER_DESC Descriptor;
    HRESULT Hr;

    if (Screen == NULL) {
        return E_POINTER;
    }

    /* Get screen descriptor */
    Hr = IFramebufferScreen_GetDescriptor(Screen, &Descriptor);
    if (FAILED(Hr)) {
        return Hr;
    }

    /* Release previous sources */
    if (Image->SourceScreen != NULL) {
        IUnknown_Release((IUnknown *)Image->SourceScreen);
    }
    if (Image->SourceSurface != NULL) {
        IUnknown_Release((IUnknown *)Image->SourceSurface);
        Image->SourceSurface = NULL;
    }

    /* Set screen source */
    Image->SourceType = FbImageSourceScreen;
    Image->SourceScreen = Screen;
    IUnknown_AddRef((IUnknown *)Screen);

    if (Rect != NULL) {
        Image->SourceRect = *Rect;
        Image->Width = Rect->Width;
        Image->Height = Rect->Height;
    } else {
        Image->SourceRect.X = 0;
        Image->SourceRect.Y = 0;
        Image->SourceRect.Width = Descriptor.Width;
        Image->SourceRect.Height = Descriptor.Height;
        Image->Width = Descriptor.Width;
        Image->Height = Descriptor.Height;
    }

    Image->PixelFormat = Descriptor.PixelFormat;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbImage_SetSurfaceSource(
    IFramebufferImage *This,
    IFramebufferSurface *Surface,
    CONST FB_RECT *Rect
    )
{
    FB_IMAGE_IMPL *Image = (FB_IMAGE_IMPL *)This;
    FRAMEBUFFER_DESC Descriptor;
    HRESULT Hr;

    if (Surface == NULL) {
        return E_POINTER;
    }

    /* Get surface descriptor */
    Hr = IFramebufferSurface_GetDescriptor(Surface, &Descriptor);
    if (FAILED(Hr)) {
        return Hr;
    }

    /* Release previous sources */
    if (Image->SourceScreen != NULL) {
        IUnknown_Release((IUnknown *)Image->SourceScreen);
        Image->SourceScreen = NULL;
    }
    if (Image->SourceSurface != NULL) {
        IUnknown_Release((IUnknown *)Image->SourceSurface);
    }

    /* Set surface source */
    Image->SourceType = FbImageSourceSurface;
    Image->SourceSurface = Surface;
    IUnknown_AddRef((IUnknown *)Surface);

    if (Rect != NULL) {
        Image->SourceRect = *Rect;
        Image->Width = Rect->Width;
        Image->Height = Rect->Height;
    } else {
        Image->SourceRect.X = 0;
        Image->SourceRect.Y = 0;
        Image->SourceRect.Width = Descriptor.Width;
        Image->SourceRect.Height = Descriptor.Height;
        Image->Width = Descriptor.Width;
        Image->Height = Descriptor.Height;
    }

    Image->PixelFormat = Descriptor.PixelFormat;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbImage_BlitToScreen(
    IFramebufferImage *This,
    IFramebufferScreen *Screen,
    INT32 DestX,
    INT32 DestY,
    CONST FB_RECT *SourceRect,
    FB_ROP Rop
    )
{
    FB_IMAGE_IMPL *Image = (FB_IMAGE_IMPL *)This;
    UINT32 SrcX, SrcY, Width, Height;
    HRESULT Hr;

    if (Screen == NULL) {
        return E_POINTER;
    }

    /* Determine source rectangle */
    if (SourceRect != NULL) {
        SrcX = SourceRect->X;
        SrcY = SourceRect->Y;
        Width = SourceRect->Width;
        Height = SourceRect->Height;
    } else {
        SrcX = 0;
        SrcY = 0;
        Width = Image->Width;
        Height = Image->Height;
    }

    /* Blit based on source type */
    switch (Image->SourceType) {
        case FbImageSourceMemory:
            /* Blit from memory buffer pixel-by-pixel */
            for (UINT32 Y = 0; Y < Height; Y++) {
                for (UINT32 X = 0; X < Width; X++) {
                    UINT32 Offset = (SrcY + Y) * Image->MemoryPitch +
                                   (SrcX + X) * FbGetBytesPerPixel(Image->PixelFormat);
                    FB_COLOR Color = FbUnpackPixel((CONST UINT8 *)Image->MemoryData + Offset,
                                                   Image->PixelFormat);

                    /* Apply color key if enabled */
                    if (Image->ColorKeyEnabled) {
                        if (Color.Red == Image->ColorKey.Red &&
                            Color.Green == Image->ColorKey.Green &&
                            Color.Blue == Image->ColorKey.Blue) {
                            continue;  /* Skip transparent pixel */
                        }
                    }

                    IFramebufferScreen_SetPixel(Screen, DestX + X, DestY + Y, Color);
                }
            }
            Hr = S_OK;
            break;

        case FbImageSourceScreen:
            /* Screen-to-screen blit */
            for (UINT32 Y = 0; Y < Height; Y++) {
                for (UINT32 X = 0; X < Width; X++) {
                    FB_COLOR Color;
                    Hr = IFramebufferScreen_GetPixel(Image->SourceScreen,
                                                     Image->SourceRect.X + SrcX + X,
                                                     Image->SourceRect.Y + SrcY + Y,
                                                     &Color);
                    if (SUCCEEDED(Hr)) {
                        if (Image->ColorKeyEnabled) {
                            if (Color.Red == Image->ColorKey.Red &&
                                Color.Green == Image->ColorKey.Green &&
                                Color.Blue == Image->ColorKey.Blue) {
                                continue;
                            }
                        }
                        IFramebufferScreen_SetPixel(Screen, DestX + X, DestY + Y, Color);
                    }
                }
            }
            Hr = S_OK;
            break;

        case FbImageSourceSurface:
            /* Surface-to-screen blit */
            for (UINT32 Y = 0; Y < Height; Y++) {
                for (UINT32 X = 0; X < Width; X++) {
                    FB_COLOR Color;
                    Hr = IFramebufferSurface_GetPixel(Image->SourceSurface,
                                                      Image->SourceRect.X + SrcX + X,
                                                      Image->SourceRect.Y + SrcY + Y,
                                                      &Color);
                    if (SUCCEEDED(Hr)) {
                        if (Image->ColorKeyEnabled) {
                            if (Color.Red == Image->ColorKey.Red &&
                                Color.Green == Image->ColorKey.Green &&
                                Color.Blue == Image->ColorKey.Blue) {
                                continue;
                            }
                        }
                        IFramebufferScreen_SetPixel(Screen, DestX + X, DestY + Y, Color);
                    }
                }
            }
            Hr = S_OK;
            break;

        default:
            Hr = E_FAIL;
            break;
    }

    return Hr;
}

static HRESULT STDMETHODCALLTYPE
FbImage_BlitToSurface(
    IFramebufferImage *This,
    IFramebufferSurface *Surface,
    INT32 DestX,
    INT32 DestY,
    CONST FB_RECT *SourceRect,
    FB_ROP Rop
    )
{
    FB_IMAGE_IMPL *Image = (FB_IMAGE_IMPL *)This;
    UINT32 SrcX, SrcY, Width, Height;
    HRESULT Hr;

    if (Surface == NULL) {
        return E_POINTER;
    }

    /* Determine source rectangle */
    if (SourceRect != NULL) {
        SrcX = SourceRect->X;
        SrcY = SourceRect->Y;
        Width = SourceRect->Width;
        Height = SourceRect->Height;
    } else {
        SrcX = 0;
        SrcY = 0;
        Width = Image->Width;
        Height = Image->Height;
    }

    /* Blit based on source type (similar to BlitToScreen) */
    switch (Image->SourceType) {
        case FbImageSourceMemory:
            for (UINT32 Y = 0; Y < Height; Y++) {
                for (UINT32 X = 0; X < Width; X++) {
                    UINT32 Offset = (SrcY + Y) * Image->MemoryPitch +
                                   (SrcX + X) * FbGetBytesPerPixel(Image->PixelFormat);
                    FB_COLOR Color = FbUnpackPixel((CONST UINT8 *)Image->MemoryData + Offset,
                                                   Image->PixelFormat);

                    if (Image->ColorKeyEnabled) {
                        if (Color.Red == Image->ColorKey.Red &&
                            Color.Green == Image->ColorKey.Green &&
                            Color.Blue == Image->ColorKey.Blue) {
                            continue;
                        }
                    }

                    IFramebufferSurface_SetPixel(Surface, DestX + X, DestY + Y, Color);
                }
            }
            Hr = S_OK;
            break;

        case FbImageSourceScreen:
            for (UINT32 Y = 0; Y < Height; Y++) {
                for (UINT32 X = 0; X < Width; X++) {
                    FB_COLOR Color;
                    Hr = IFramebufferScreen_GetPixel(Image->SourceScreen,
                                                     Image->SourceRect.X + SrcX + X,
                                                     Image->SourceRect.Y + SrcY + Y,
                                                     &Color);
                    if (SUCCEEDED(Hr)) {
                        if (Image->ColorKeyEnabled) {
                            if (Color.Red == Image->ColorKey.Red &&
                                Color.Green == Image->ColorKey.Green &&
                                Color.Blue == Image->ColorKey.Blue) {
                                continue;
                            }
                        }
                        IFramebufferSurface_SetPixel(Surface, DestX + X, DestY + Y, Color);
                    }
                }
            }
            Hr = S_OK;
            break;

        case FbImageSourceSurface:
            for (UINT32 Y = 0; Y < Height; Y++) {
                for (UINT32 X = 0; X < Width; X++) {
                    FB_COLOR Color;
                    Hr = IFramebufferSurface_GetPixel(Image->SourceSurface,
                                                      Image->SourceRect.X + SrcX + X,
                                                      Image->SourceRect.Y + SrcY + Y,
                                                      &Color);
                    if (SUCCEEDED(Hr)) {
                        if (Image->ColorKeyEnabled) {
                            if (Color.Red == Image->ColorKey.Red &&
                                Color.Green == Image->ColorKey.Green &&
                                Color.Blue == Image->ColorKey.Blue) {
                                continue;
                            }
                        }
                        IFramebufferSurface_SetPixel(Surface, DestX + X, DestY + Y, Color);
                    }
                }
            }
            Hr = S_OK;
            break;

        default:
            Hr = E_FAIL;
            break;
    }

    return Hr;
}

static HRESULT STDMETHODCALLTYPE
FbImage_BlitToMemory(
    IFramebufferImage *This,
    VOID *DestBuffer,
    UINT32 DestPitch,
    FB_PIXEL_FORMAT DestFormat,
    CONST FB_RECT *SourceRect
    )
{
    FB_IMAGE_IMPL *Image = (FB_IMAGE_IMPL *)This;
    UINT32 SrcX, SrcY, Width, Height;

    if (DestBuffer == NULL) {
        return E_POINTER;
    }

    /* Determine source rectangle */
    if (SourceRect != NULL) {
        SrcX = SourceRect->X;
        SrcY = SourceRect->Y;
        Width = SourceRect->Width;
        Height = SourceRect->Height;
    } else {
        SrcX = 0;
        SrcY = 0;
        Width = Image->Width;
        Height = Image->Height;
    }

    /* Only support memory-to-memory for now */
    if (Image->SourceType == FbImageSourceMemory) {
        /* Use engine for format conversion */
        return FbEngineConvertFormat(
            Image->MemoryData,
            Image->PixelFormat,
            Width,
            Height,
            Image->MemoryPitch,
            DestBuffer,
            DestFormat,
            DestPitch,
            Image->DitherMethod);
    }

    /* For screen/surface sources, copy pixel-by-pixel */
    for (UINT32 Y = 0; Y < Height; Y++) {
        for (UINT32 X = 0; X < Width; X++) {
            FB_COLOR Color = {0, 0, 0, 0};

            if (Image->SourceType == FbImageSourceScreen) {
                IFramebufferScreen_GetPixel(Image->SourceScreen,
                                            Image->SourceRect.X + SrcX + X,
                                            Image->SourceRect.Y + SrcY + Y,
                                            &Color);
            } else if (Image->SourceType == FbImageSourceSurface) {
                IFramebufferSurface_GetPixel(Image->SourceSurface,
                                             Image->SourceRect.X + SrcX + X,
                                             Image->SourceRect.Y + SrcY + Y,
                                             &Color);
            }

            UINT32 DestOffset = Y * DestPitch + X * FbGetBytesPerPixel(DestFormat);
            FbPackPixel(&Color, (UINT8 *)DestBuffer + DestOffset, DestFormat);
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbImage_Transform(
    IFramebufferImage *This,
    FB_PIXEL_FORMAT TargetFormat,
    IFramebufferImage **TransformedImage
    )
{
    FB_IMAGE_IMPL *Image = (FB_IMAGE_IMPL *)This;
    IFramebufferImage *NewImage;
    VOID *NewMemory;
    UINT32 NewPitch;
    HRESULT Hr;

    if (TransformedImage == NULL) {
        return E_POINTER;
    }

    /* Calculate new pitch */
    NewPitch = Image->Width * FbGetBytesPerPixel(TargetFormat);
    if (NewPitch % 4 != 0) {
        NewPitch += 4 - (NewPitch % 4);
    }

    /* Allocate memory for transformed image */
    NewMemory = ANX_MALLOC(NewPitch * Image->Height);
    if (NewMemory == NULL) {
        return E_OUTOFMEMORY;
    }

    /* Convert to new format */
    Hr = FbImage_BlitToMemory(This, NewMemory, NewPitch, TargetFormat, NULL);
    if (FAILED(Hr)) {
        ANX_FREE(NewMemory);
        return Hr;
    }

    /* Create new image */
    NewImage = FbCreateImageFromMemory(NewMemory, Image->Width, Image->Height,
                                       NewPitch, TargetFormat);
    if (NewImage == NULL) {
        ANX_FREE(NewMemory);
        return E_OUTOFMEMORY;
    }

    /* Mark that the new image owns the memory */
    ((FB_IMAGE_IMPL *)NewImage)->OwnsMemory = TRUE;

    *TransformedImage = NewImage;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbImage_SetDitherMethod(
    IFramebufferImage *This,
    FB_DITHER_METHOD Method
    )
{
    FB_IMAGE_IMPL *Image = (FB_IMAGE_IMPL *)This;
    Image->DitherMethod = Method;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbImage_SetColorKey(
    IFramebufferImage *This,
    BOOLEAN Enable,
    FB_COLOR ColorKey
    )
{
    FB_IMAGE_IMPL *Image = (FB_IMAGE_IMPL *)This;
    Image->ColorKeyEnabled = Enable;
    Image->ColorKey = ColorKey;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructors                                             */
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
    )
{
    FB_IMAGE_IMPL *Image;
    HRESULT Hr;

    if (Data == NULL || Width == 0 || Height == 0) {
        return NULL;
    }

    /* Allocate image object */
    Image = (FB_IMAGE_IMPL *)ANX_MALLOC(sizeof(FB_IMAGE_IMPL));
    if (Image == NULL) {
        return NULL;
    }

    /* Initialize */
    ANX_MEMSET(Image, 0, sizeof(FB_IMAGE_IMPL));
    Image->Base.lpVtbl = &gImageVtbl;
    Image->RefCount.RefCount = 1;
    Image->DitherMethod = FbDitherNone;
    Image->ColorKeyEnabled = FALSE;

    /* Set memory source */
    Hr = FbImage_SetMemorySource(&Image->Base, Data, Width, Height, Pitch, PixelFormat);
    if (FAILED(Hr)) {
        ANX_FREE(Image);
        return NULL;
    }

    return &Image->Base;
}

/*
 * Create an image object from a screen region.
 */
IFramebufferImage *
FbCreateImageFromScreen(
    IN IFramebufferScreen *Screen,
    IN CONST FB_RECT *Rect
    )
{
    FB_IMAGE_IMPL *Image;
    HRESULT Hr;

    if (Screen == NULL) {
        return NULL;
    }

    /* Allocate image object */
    Image = (FB_IMAGE_IMPL *)ANX_MALLOC(sizeof(FB_IMAGE_IMPL));
    if (Image == NULL) {
        return NULL;
    }

    /* Initialize */
    ANX_MEMSET(Image, 0, sizeof(FB_IMAGE_IMPL));
    Image->Base.lpVtbl = &gImageVtbl;
    Image->RefCount.RefCount = 1;
    Image->DitherMethod = FbDitherNone;
    Image->ColorKeyEnabled = FALSE;

    /* Set screen source */
    Hr = FbImage_SetScreenSource(&Image->Base, Screen, Rect);
    if (FAILED(Hr)) {
        ANX_FREE(Image);
        return NULL;
    }

    return &Image->Base;
}

/*
 * Create an image object from a surface region.
 */
IFramebufferImage *
FbCreateImageFromSurface(
    IN IFramebufferSurface *Surface,
    IN CONST FB_RECT *Rect
    )
{
    FB_IMAGE_IMPL *Image;
    HRESULT Hr;

    if (Surface == NULL) {
        return NULL;
    }

    /* Allocate image object */
    Image = (FB_IMAGE_IMPL *)ANX_MALLOC(sizeof(FB_IMAGE_IMPL));
    if (Image == NULL) {
        return NULL;
    }

    /* Initialize */
    ANX_MEMSET(Image, 0, sizeof(FB_IMAGE_IMPL));
    Image->Base.lpVtbl = &gImageVtbl;
    Image->RefCount.RefCount = 1;
    Image->DitherMethod = FbDitherNone;
    Image->ColorKeyEnabled = FALSE;

    /* Set surface source */
    Hr = FbImage_SetSurfaceSource(&Image->Base, Surface, Rect);
    if (FAILED(Hr)) {
        ANX_FREE(Image);
        return NULL;
    }

    return &Image->Base;
}
