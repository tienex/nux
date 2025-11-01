/*++
    Module Name:

        generic.c

    Abstract:

        Generic framebuffer backend implementation.
        Supports all RGB formats and indexed color modes.

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/framebuffer/dither.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Generic Backend Structure                                       */
/* --------------------------------------------------------------- */

typedef struct _GENERIC_FB_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    UINT8                       *FramebufferBase;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;
} GENERIC_FB_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE GenericFb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE GenericFb_AddRef(
    IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE GenericFb_Release(
    IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE GenericFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE GenericFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE GenericFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE GenericFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE GenericFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE GenericFb_BlitMonoBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_COLOR Foreground,
    FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE GenericFb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE GenericFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE GenericFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gGenericFbVtbl = {
    .QueryInterface     = GenericFb_QueryInterface,
    .AddRef             = GenericFb_AddRef,
    .Release            = GenericFb_Release,
    .Initialize         = GenericFb_Initialize,
    .Clear              = GenericFb_Clear,
    .SetPixel           = GenericFb_SetPixel,
    .GetPixel           = GenericFb_GetPixel,
    .FillRect           = GenericFb_FillRect,
    .BlitMonoBitmap     = GenericFb_BlitMonoBitmap,
    .BlitBitmap         = GenericFb_BlitBitmap,
    .GetDescriptor      = GenericFb_GetDescriptor,
    .SetDitherMethod    = GenericFb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static INLINE VOID
GenericFb_WritePixel(
    GENERIC_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT32 PixelValue
    )
{
    UINT8 *Addr;
    UINT32 Offset;

    Offset = Y * Backend->Descriptor.Pitch + X * sizeof(UINT32);
    Addr = Backend->FramebufferBase + Offset;

    /* Write pixel based on format */
    switch (Backend->Descriptor.PixelFormat) {
        case FbPixelFormatRgb888:
        case FbPixelFormatRgb555:
        case FbPixelFormatRgb565:
            *(UINT32 *)Addr = PixelValue;
            break;

        case FbPixelFormatIndexed256:
            *(UINT8 *)Addr = (UINT8)PixelValue;
            break;

        default:
            break;
    }
}

static INLINE UINT32
GenericFb_ReadPixel(
    GENERIC_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    UINT8 *Addr;
    UINT32 Offset;

    Offset = Y * Backend->Descriptor.Pitch + X * sizeof(UINT32);
    Addr = Backend->FramebufferBase + Offset;

    switch (Backend->Descriptor.PixelFormat) {
        case FbPixelFormatRgb888:
        case FbPixelFormatRgb555:
        case FbPixelFormatRgb565:
            return *(UINT32 *)Addr;

        case FbPixelFormatIndexed256:
            return *(UINT8 *)Addr;

        default:
            return 0;
    }
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
GenericFb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    GENERIC_FB_BACKEND *Backend = (GENERIC_FB_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        GenericFb_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
GenericFb_AddRef(
    IFramebufferBackend *This
    )
{
    GENERIC_FB_BACKEND *Backend = (GENERIC_FB_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
GenericFb_Release(
    IFramebufferBackend *This
    )
{
    GENERIC_FB_BACKEND *Backend = (GENERIC_FB_BACKEND *)This;
    UINT32 RefCount = ANX_REF_DEC(&Backend->RefCount);

    if (RefCount == 0) {
        /* Free backend - for now just leak it since we don't have malloc */
        /* In a real implementation, this would call a memory allocator */
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  IFramebufferBackend Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
GenericFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    GENERIC_FB_BACKEND *Backend = (GENERIC_FB_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    /* Copy descriptor */
    Backend->Descriptor = *Descriptor;
    Backend->FramebufferBase = (UINT8 *)(UINTN)Descriptor->PhysicalBase;
    Backend->Initialized = TRUE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GenericFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    GENERIC_FB_BACKEND *Backend = (GENERIC_FB_BACKEND *)This;
    UINT32 PixelValue;
    UINT32 x, y;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    PixelValue = FbPackPixel(Color, Backend->Descriptor.PixelFormat,
                             Backend->Descriptor.RedMask,
                             Backend->Descriptor.GreenMask,
                             Backend->Descriptor.BlueMask);

    for (y = 0; y < Backend->Descriptor.Height; y++) {
        for (x = 0; x < Backend->Descriptor.Width; x++) {
            GenericFb_WritePixel(Backend, x, y, PixelValue);
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GenericFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    GENERIC_FB_BACKEND *Backend = (GENERIC_FB_BACKEND *)This;
    UINT32 PixelValue;
    FB_COLOR DitheredColor;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    /* Apply dithering if enabled */
    DitheredColor = Color;
    if (Backend->DitherMethod == FbDitherClassicMac) {
        FbDitherRgb(X, Y, &DitheredColor, Backend->Descriptor.PixelFormat);
    }

    PixelValue = FbPackPixel(DitheredColor, Backend->Descriptor.PixelFormat,
                             Backend->Descriptor.RedMask,
                             Backend->Descriptor.GreenMask,
                             Backend->Descriptor.BlueMask);

    GenericFb_WritePixel(Backend, X, Y, PixelValue);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GenericFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    GENERIC_FB_BACKEND *Backend = (GENERIC_FB_BACKEND *)This;
    UINT32 PixelValue;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    PixelValue = GenericFb_ReadPixel(Backend, X, Y);
    *Color = FbUnpackPixel(PixelValue, Backend->Descriptor.PixelFormat,
                           Backend->Descriptor.RedMask,
                           Backend->Descriptor.GreenMask,
                           Backend->Descriptor.BlueMask);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GenericFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    GENERIC_FB_BACKEND *Backend = (GENERIC_FB_BACKEND *)This;
    UINT32 PixelValue;
    INT32 x, y;

    if (!Backend->Initialized || Rect == NULL) {
        return E_POINTER;
    }

    PixelValue = FbPackPixel(Color, Backend->Descriptor.PixelFormat,
                             Backend->Descriptor.RedMask,
                             Backend->Descriptor.GreenMask,
                             Backend->Descriptor.BlueMask);

    for (y = Rect->Top; y < Rect->Bottom && y < (INT32)Backend->Descriptor.Height; y++) {
        for (x = Rect->Left; x < Rect->Right && x < (INT32)Backend->Descriptor.Width; x++) {
            if (x >= 0 && y >= 0) {
                GenericFb_WritePixel(Backend, x, y, PixelValue);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GenericFb_BlitMonoBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_COLOR Foreground,
    FB_COLOR Background
    )
{
    GENERIC_FB_BACKEND *Backend = (GENERIC_FB_BACKEND *)This;
    UINT32 FgPixel, BgPixel;
    UINT32 Row, Col;
    UINT32 ByteIndex, BitIndex;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    FgPixel = FbPackPixel(Foreground, Backend->Descriptor.PixelFormat,
                          Backend->Descriptor.RedMask,
                          Backend->Descriptor.GreenMask,
                          Backend->Descriptor.BlueMask);

    BgPixel = FbPackPixel(Background, Backend->Descriptor.PixelFormat,
                          Backend->Descriptor.RedMask,
                          Backend->Descriptor.GreenMask,
                          Backend->Descriptor.BlueMask);

    for (Row = 0; Row < Height; Row++) {
        for (Col = 0; Col < Width; Col++) {
            ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
            BitIndex = 7 - (Col % 8);

            if (Bitmap[ByteIndex] & (1 << BitIndex)) {
                GenericFb_WritePixel(Backend, X + Col, Y + Row, FgPixel);
            } else {
                GenericFb_WritePixel(Backend, X + Col, Y + Row, BgPixel);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GenericFb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    /* Not implemented yet - would convert source format to target format */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
GenericFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    GENERIC_FB_BACKEND *Backend = (GENERIC_FB_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
GenericFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    GENERIC_FB_BACKEND *Backend = (GENERIC_FB_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static GENERIC_FB_BACKEND gGenericBackendInstance = {
    .Base.lpVtbl        = &gGenericFbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherNone,
};

IFramebufferBackend *
FbCreateGenericBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gGenericBackendInstance;
}
