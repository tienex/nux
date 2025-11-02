/*++
    Module Name:

        cursor.c

    Abstract:

        IFramebufferCursor implementation.

        Cursor is a pure data object representing cursor appearance.
        Internally uses IFramebufferImage for pixel storage.
        Display control (position, visibility) is handled by IFramebufferScreen.

--*/

#include <ananke/framebuffer/cursor.h>
#include <ananke/framebuffer/image.h>
#include <ananke/framebuffer/com_helpers.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Cursor Implementation Structure                                */
/* --------------------------------------------------------------- */

typedef struct _FB_CURSOR_IMPL {
    IFramebufferCursor      Base;
    REFOBJ                  RefCount;

    /* Cursor metadata */
    FB_CURSOR_DESC          Descriptor;

    /* Image storage */
    IFramebufferImage       *Image;         /* For static cursors */
    IFramebufferImage       **Frames;       /* For animated cursors */
    UINT32                  *FrameTimes;    /* Display time per frame (ms) */
} FB_CURSOR_IMPL;

/* --------------------------------------------------------------- */
/*  Forward Declarations                                            */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE FbCursor_QueryInterface(
    IFramebufferCursor *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE FbCursor_AddRef(IFramebufferCursor *This);
static UINT32 STDMETHODCALLTYPE FbCursor_Release(IFramebufferCursor *This);
static HRESULT STDMETHODCALLTYPE FbCursor_GetDescriptor(
    IFramebufferCursor *This, FB_CURSOR_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE FbCursor_GetType(
    IFramebufferCursor *This, FB_CURSOR_TYPE *Type);
static HRESULT STDMETHODCALLTYPE FbCursor_GetHotSpot(
    IFramebufferCursor *This, INT32 *X, INT32 *Y);
static HRESULT STDMETHODCALLTYPE FbCursor_GetImage(
    IFramebufferCursor *This, IFramebufferImage **Image);
static HRESULT STDMETHODCALLTYPE FbCursor_GetFrameCount(
    IFramebufferCursor *This, UINT32 *Count);
static HRESULT STDMETHODCALLTYPE FbCursor_GetFrame(
    IFramebufferCursor *This, UINT32 FrameIndex,
    IFramebufferImage **Image, UINT32 *DisplayTimeMs);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferCursorVtbl gCursorVtbl = {
    .QueryInterface     = FbCursor_QueryInterface,
    .AddRef             = FbCursor_AddRef,
    .Release            = FbCursor_Release,
    .GetDescriptor      = FbCursor_GetDescriptor,
    .GetType            = FbCursor_GetType,
    .GetHotSpot         = FbCursor_GetHotSpot,
    .GetImage           = FbCursor_GetImage,
    .GetFrameCount      = FbCursor_GetFrameCount,
    .GetFrame           = FbCursor_GetFrame,
};

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

FB_IMPLEMENT_IUNKNOWN(FbCursor, FB_CURSOR_IMPL, IFramebufferCursor, IID_IFramebufferCursor)

/* --------------------------------------------------------------- */
/*  IFramebufferCursor Implementation                               */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbCursor_GetDescriptor(
    IFramebufferCursor *This,
    FB_CURSOR_DESC *Descriptor
    )
{
    FB_CURSOR_IMPL *Cursor = (FB_CURSOR_IMPL *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Cursor->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbCursor_GetType(
    IFramebufferCursor *This,
    FB_CURSOR_TYPE *Type
    )
{
    FB_CURSOR_IMPL *Cursor = (FB_CURSOR_IMPL *)This;

    if (Type == NULL) {
        return E_POINTER;
    }

    *Type = Cursor->Descriptor.Type;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbCursor_GetHotSpot(
    IFramebufferCursor *This,
    INT32 *X,
    INT32 *Y
    )
{
    FB_CURSOR_IMPL *Cursor = (FB_CURSOR_IMPL *)This;

    if (X == NULL || Y == NULL) {
        return E_POINTER;
    }

    *X = Cursor->Descriptor.HotSpotX;
    *Y = Cursor->Descriptor.HotSpotY;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbCursor_GetImage(
    IFramebufferCursor *This,
    IFramebufferImage **Image
    )
{
    FB_CURSOR_IMPL *Cursor = (FB_CURSOR_IMPL *)This;

    if (Image == NULL) {
        return E_POINTER;
    }

    /* For static cursors, return the single image */
    if (Cursor->Descriptor.Type != FbCursorAnimated) {
        if (Cursor->Image == NULL) {
            return E_FAIL;
        }

        IUnknown_AddRef((IUnknown *)Cursor->Image);
        *Image = Cursor->Image;
        return S_OK;
    }

    /* For animated cursors, return current frame */
    if (Cursor->Frames == NULL || Cursor->Descriptor.FrameCount == 0) {
        return E_FAIL;
    }

    UINT32 CurrentFrame = Cursor->Descriptor.CurrentFrame;
    if (CurrentFrame >= Cursor->Descriptor.FrameCount) {
        CurrentFrame = 0;
    }

    IUnknown_AddRef((IUnknown *)Cursor->Frames[CurrentFrame]);
    *Image = Cursor->Frames[CurrentFrame];
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbCursor_GetFrameCount(
    IFramebufferCursor *This,
    UINT32 *Count
    )
{
    FB_CURSOR_IMPL *Cursor = (FB_CURSOR_IMPL *)This;

    if (Count == NULL) {
        return E_POINTER;
    }

    *Count = Cursor->Descriptor.FrameCount;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbCursor_GetFrame(
    IFramebufferCursor *This,
    UINT32 FrameIndex,
    IFramebufferImage **Image,
    UINT32 *DisplayTimeMs
    )
{
    FB_CURSOR_IMPL *Cursor = (FB_CURSOR_IMPL *)This;

    if (Image == NULL) {
        return E_POINTER;
    }

    /* Validate frame index */
    if (FrameIndex >= Cursor->Descriptor.FrameCount) {
        return E_INVALIDARG;
    }

    /* For static cursors */
    if (Cursor->Descriptor.Type != FbCursorAnimated) {
        if (FrameIndex != 0) {
            return E_INVALIDARG;
        }

        IUnknown_AddRef((IUnknown *)Cursor->Image);
        *Image = Cursor->Image;

        if (DisplayTimeMs != NULL) {
            *DisplayTimeMs = 0;  /* Static cursor has no display time */
        }
        return S_OK;
    }

    /* For animated cursors */
    if (Cursor->Frames == NULL || Cursor->Frames[FrameIndex] == NULL) {
        return E_FAIL;
    }

    IUnknown_AddRef((IUnknown *)Cursor->Frames[FrameIndex]);
    *Image = Cursor->Frames[FrameIndex];

    if (DisplayTimeMs != NULL && Cursor->FrameTimes != NULL) {
        *DisplayTimeMs = Cursor->FrameTimes[FrameIndex];
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Factory Functions                                              */
/* --------------------------------------------------------------- */

IFramebufferCursor *
FbCreateCursorFromImage(
    IN IFramebufferImage *Image,
    IN INT32 HotSpotX,
    IN INT32 HotSpotY
    )
{
    FB_CURSOR_IMPL *Cursor;
    FB_IMAGE_INFO ImageInfo;
    HRESULT Hr;

    if (Image == NULL) {
        return NULL;
    }

    /* Get image info */
    Hr = IFramebufferImage_GetInfo(Image, &ImageInfo);
    if (FAILED(Hr)) {
        return NULL;
    }

    /* Allocate cursor */
    Cursor = (FB_CURSOR_IMPL *)ANX_MALLOC(sizeof(FB_CURSOR_IMPL));
    if (Cursor == NULL) {
        return NULL;
    }

    ANX_MEMSET(Cursor, 0, sizeof(FB_CURSOR_IMPL));
    Cursor->Base.lpVtbl = &gCursorVtbl;
    Cursor->RefCount.RefCount = 1;

    /* Set up descriptor - determine type from image format */
    Cursor->Descriptor.Type = FbCursorColor;  /* Assume color cursor */
    Cursor->Descriptor.Width = ImageInfo.Width;
    Cursor->Descriptor.Height = ImageInfo.Height;
    Cursor->Descriptor.HotSpotX = HotSpotX;
    Cursor->Descriptor.HotSpotY = HotSpotY;
    Cursor->Descriptor.FrameCount = 1;
    Cursor->Descriptor.CurrentFrame = 0;

    /* Store image (add reference) */
    IUnknown_AddRef((IUnknown *)Image);
    Cursor->Image = Image;

    return &Cursor->Base;
}

IFramebufferCursor *
FbCreateMonoCursor(
    IN CONST FB_MONO_CURSOR_DESC *Descriptor
    )
{
    FB_CURSOR_IMPL *Cursor;
    IFramebufferImage *Image;
    UINT32 MaskSize;

    if (Descriptor == NULL || Descriptor->AndMask == NULL || Descriptor->XorMask == NULL) {
        return NULL;
    }

    /* Validate dimensions */
    if (Descriptor->Width == 0 || Descriptor->Height == 0 ||
        Descriptor->Width > FB_CURSOR_MAX_WIDTH ||
        Descriptor->Height > FB_CURSOR_MAX_HEIGHT) {
        return NULL;
    }

    /* Create image from monochrome data
     * Note: We need to convert mono masks to an image format.
     * For now, convert to 1-bit indexed or RGBA with alpha channel.
     * This is implementation-specific - mono masks need conversion.
     */

    /* Calculate mask size in bytes */
    MaskSize = ((Descriptor->Width + 7) / 8) * Descriptor->Height;

    /* Create monochrome image
     * TODO: Implement FbCreateImageFromMonoMask helper or
     * convert masks to RGBA format here
     */

    /* For now, create placeholder RGBA image */
    UINT32 ImageSize = Descriptor->Width * Descriptor->Height * 4;
    UINT8 *RgbaData = (UINT8 *)ANX_MALLOC(ImageSize);
    if (RgbaData == NULL) {
        return NULL;
    }

    /* Convert AND/XOR masks to RGBA
     * AND mask: 1 = use pixel, 0 = transparent
     * XOR mask: 1 = invert, 0 = black
     * Typical conversion:
     * - AND=0: Transparent (alpha=0)
     * - AND=1, XOR=0: Black pixel
     * - AND=1, XOR=1: White pixel (inverted)
     */
    for (UINT32 Y = 0; Y < Descriptor->Height; Y++) {
        for (UINT32 X = 0; X < Descriptor->Width; X++) {
            UINT32 ByteOffset = Y * ((Descriptor->Width + 7) / 8) + (X / 8);
            UINT32 BitMask = 1 << (7 - (X % 8));

            UINT8 AndBit = (Descriptor->AndMask[ByteOffset] & BitMask) ? 1 : 0;
            UINT8 XorBit = (Descriptor->XorMask[ByteOffset] & BitMask) ? 1 : 0;

            UINT32 PixelOffset = (Y * Descriptor->Width + X) * 4;

            if (AndBit == 0) {
                /* Transparent */
                RgbaData[PixelOffset + 0] = 0;    /* R */
                RgbaData[PixelOffset + 1] = 0;    /* G */
                RgbaData[PixelOffset + 2] = 0;    /* B */
                RgbaData[PixelOffset + 3] = 0;    /* A = transparent */
            } else {
                /* Opaque pixel */
                UINT8 Color = XorBit ? 255 : 0;   /* White or black */
                RgbaData[PixelOffset + 0] = Color;
                RgbaData[PixelOffset + 1] = Color;
                RgbaData[PixelOffset + 2] = Color;
                RgbaData[PixelOffset + 3] = 255;  /* A = opaque */
            }
        }
    }

    /* Create image from RGBA data */
    Image = FbCreateImageFromMemory(
        RgbaData,
        Descriptor->Width,
        Descriptor->Height,
        Descriptor->Width * 4,
        FbPixelFormatRGBA32
    );

    ANX_FREE(RgbaData);

    if (Image == NULL) {
        return NULL;
    }

    /* Create cursor from image */
    Cursor = (FB_CURSOR_IMPL *)FbCreateCursorFromImage(
        Image,
        Descriptor->HotSpotX,
        Descriptor->HotSpotY
    );

    /* Release our image reference (cursor now owns it) */
    IUnknown_Release((IUnknown *)Image);

    if (Cursor != NULL) {
        /* Update type to monochrome */
        Cursor->Descriptor.Type = FbCursorMono;
    }

    return (IFramebufferCursor *)Cursor;
}

IFramebufferCursor *
FbCreateColorCursor(
    IN CONST FB_COLOR_CURSOR_DESC *Descriptor
    )
{
    IFramebufferImage *Image;

    if (Descriptor == NULL || Descriptor->Data == NULL) {
        return NULL;
    }

    /* Validate dimensions */
    if (Descriptor->Width == 0 || Descriptor->Height == 0 ||
        Descriptor->Width > FB_CURSOR_MAX_WIDTH ||
        Descriptor->Height > FB_CURSOR_MAX_HEIGHT) {
        return NULL;
    }

    /* Create image from RGBA data */
    Image = FbCreateImageFromMemory(
        Descriptor->Data,
        Descriptor->Width,
        Descriptor->Height,
        Descriptor->Width * 4,  /* Pitch: RGBA = 4 bytes per pixel */
        FbPixelFormatRGBA32
    );

    if (Image == NULL) {
        return NULL;
    }

    /* Create cursor from image */
    IFramebufferCursor *Cursor = FbCreateCursorFromImage(
        Image,
        Descriptor->HotSpotX,
        Descriptor->HotSpotY
    );

    /* Release our image reference (cursor now owns it) */
    IUnknown_Release((IUnknown *)Image);

    return Cursor;
}

IFramebufferCursor *
FbCreateAnimatedCursor(
    IN CONST FB_ANIMATED_CURSOR_DESC *Descriptor
    )
{
    FB_CURSOR_IMPL *Cursor;
    UINT32 I;

    if (Descriptor == NULL || Descriptor->Frames == NULL || Descriptor->FrameCount == 0) {
        return NULL;
    }

    /* Validate dimensions and frame count */
    if (Descriptor->Width == 0 || Descriptor->Height == 0 ||
        Descriptor->Width > FB_CURSOR_MAX_WIDTH ||
        Descriptor->Height > FB_CURSOR_MAX_HEIGHT ||
        Descriptor->FrameCount > FB_CURSOR_MAX_FRAMES) {
        return NULL;
    }

    /* Allocate cursor */
    Cursor = (FB_CURSOR_IMPL *)ANX_MALLOC(sizeof(FB_CURSOR_IMPL));
    if (Cursor == NULL) {
        return NULL;
    }

    ANX_MEMSET(Cursor, 0, sizeof(FB_CURSOR_IMPL));
    Cursor->Base.lpVtbl = &gCursorVtbl;
    Cursor->RefCount.RefCount = 1;

    /* Set up descriptor */
    Cursor->Descriptor.Type = FbCursorAnimated;
    Cursor->Descriptor.Width = Descriptor->Width;
    Cursor->Descriptor.Height = Descriptor->Height;
    Cursor->Descriptor.HotSpotX = Descriptor->HotSpotX;
    Cursor->Descriptor.HotSpotY = Descriptor->HotSpotY;
    Cursor->Descriptor.FrameCount = Descriptor->FrameCount;
    Cursor->Descriptor.CurrentFrame = 0;

    /* Allocate frame arrays */
    Cursor->Frames = (IFramebufferImage **)ANX_MALLOC(
        sizeof(IFramebufferImage *) * Descriptor->FrameCount);
    Cursor->FrameTimes = (UINT32 *)ANX_MALLOC(
        sizeof(UINT32) * Descriptor->FrameCount);

    if (Cursor->Frames == NULL || Cursor->FrameTimes == NULL) {
        if (Cursor->Frames != NULL) ANX_FREE(Cursor->Frames);
        if (Cursor->FrameTimes != NULL) ANX_FREE(Cursor->FrameTimes);
        ANX_FREE(Cursor);
        return NULL;
    }

    ANX_MEMSET(Cursor->Frames, 0, sizeof(IFramebufferImage *) * Descriptor->FrameCount);

    /* Convert each frame to an image */
    for (I = 0; I < Descriptor->FrameCount; I++) {
        if (Descriptor->FrameType == FbCursorMono) {
            /* Monochrome frame - convert to image
             * Frame data contains AND mask followed by XOR mask
             */
            UINT32 MaskSize = ((Descriptor->Width + 7) / 8) * Descriptor->Height;
            CONST UINT8 *AndMask = Descriptor->Frames[I].Data;
            CONST UINT8 *XorMask = Descriptor->Frames[I].Data + MaskSize;

            FB_MONO_CURSOR_DESC MonoDesc = {
                .AndMask = AndMask,
                .XorMask = XorMask,
                .Width = Descriptor->Width,
                .Height = Descriptor->Height,
                .HotSpotX = Descriptor->HotSpotX,
                .HotSpotY = Descriptor->HotSpotY,
            };

            /* Create temporary mono cursor and extract its image */
            IFramebufferCursor *TempCursor = FbCreateMonoCursor(&MonoDesc);
            if (TempCursor == NULL) {
                goto cleanup_frames;
            }

            IFramebufferCursor_GetImage(TempCursor, &Cursor->Frames[I]);
            IUnknown_Release((IUnknown *)TempCursor);

        } else {
            /* Color frame - RGBA data */
            Cursor->Frames[I] = FbCreateImageFromMemory(
                Descriptor->Frames[I].Data,
                Descriptor->Width,
                Descriptor->Height,
                Descriptor->Width * 4,
                FbPixelFormatRGBA32
            );

            if (Cursor->Frames[I] == NULL) {
                goto cleanup_frames;
            }
        }

        /* Store display time */
        Cursor->FrameTimes[I] = Descriptor->Frames[I].DisplayTime;
    }

    return &Cursor->Base;

cleanup_frames:
    /* Cleanup on failure */
    for (I = 0; I < Descriptor->FrameCount; I++) {
        if (Cursor->Frames[I] != NULL) {
            IUnknown_Release((IUnknown *)Cursor->Frames[I]);
        }
    }
    ANX_FREE(Cursor->Frames);
    ANX_FREE(Cursor->FrameTimes);
    ANX_FREE(Cursor);
    return NULL;
}

/* --------------------------------------------------------------- */
/*  Standard Cursor Shapes (Data)                                  */
/* --------------------------------------------------------------- */

/* Standard arrow cursor (16x16 monochrome) */
CONST UINT8 gStandardArrowCursorAnd[16 * 16 / 8] = {
    0x3F, 0xFF,  /* 00111111 11111111 */
    0x1F, 0xFF,  /* 00011111 11111111 */
    0x0F, 0xFF,  /* 00001111 11111111 */
    0x07, 0xFF,  /* 00000111 11111111 */
    0x03, 0xFF,  /* 00000011 11111111 */
    0x01, 0xFF,  /* 00000001 11111111 */
    0x00, 0xFF,  /* 00000000 11111111 */
    0x00, 0x7F,  /* 00000000 01111111 */
    0x00, 0x3F,  /* 00000000 00111111 */
    0x00, 0x1F,  /* 00000000 00011111 */
    0x00, 0xFF,  /* 00000000 11111111 */
    0x01, 0xFF,  /* 00000001 11111111 */
    0x31, 0xFF,  /* 00110001 11111111 */
    0xF8, 0xFF,  /* 11111000 11111111 */
    0xF8, 0xFF,  /* 11111000 11111111 */
    0xFC, 0xFF,  /* 11111100 11111111 */
};

CONST UINT8 gStandardArrowCursorXor[16 * 16 / 8] = {
    0x00, 0x00,
    0x40, 0x00,
    0x60, 0x00,
    0x70, 0x00,
    0x78, 0x00,
    0x7C, 0x00,
    0x7E, 0x00,
    0x7F, 0x00,
    0x7F, 0x80,
    0x7C, 0x00,
    0x6C, 0x00,
    0x46, 0x00,
    0x06, 0x00,
    0x03, 0x00,
    0x03, 0x00,
    0x00, 0x00,
};

/* Note: Other standard cursors (I-beam, wait, crosshair) would be defined similarly */
CONST UINT8 gStandardIBeamCursorAnd[16 * 16 / 8] = { /* TODO */ };
CONST UINT8 gStandardIBeamCursorXor[16 * 16 / 8] = { /* TODO */ };
CONST UINT8 gStandardWaitCursorAnd[16 * 16 / 8] = { /* TODO */ };
CONST UINT8 gStandardWaitCursorXor[16 * 16 / 8] = { /* TODO */ };
CONST UINT8 gStandardCrosshairCursorAnd[16 * 16 / 8] = { /* TODO */ };
CONST UINT8 gStandardCrosshairCursorXor[16 * 16 / 8] = { /* TODO */ };
