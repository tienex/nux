/*++
    Module Name:

        vesa_linear.c

    Abstract:

        VESA BIOS Extensions (VBE) linear framebuffer backend.
        Supports VESA 2.0+ linear framebuffer modes (LFB).

        This backend is essentially the same as the generic backend
        but with VESA-specific initialization and mode setting support.

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/framebuffer/dither.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  VESA Linear Backend Structure                                   */
/* --------------------------------------------------------------- */

typedef struct _VESA_LINEAR_FB_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    UINT8                       *FramebufferBase;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;
    UINT16                      VesaModeNumber;
} VESA_LINEAR_FB_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE VesaLinearFb_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE VesaLinearFb_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE VesaLinearFb_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE VesaLinearFb_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE VesaLinearFb_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE VesaLinearFb_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE VesaLinearFb_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE VesaLinearFb_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE VesaLinearFb_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE VesaLinearFb_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE VesaLinearFb_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE VesaLinearFb_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gVesaLinearFbVtbl = {
    .QueryInterface     = VesaLinearFb_QueryInterface,
    .AddRef             = VesaLinearFb_AddRef,
    .Release            = VesaLinearFb_Release,
    .Initialize         = VesaLinearFb_Initialize,
    .Clear              = VesaLinearFb_Clear,
    .SetPixel           = VesaLinearFb_SetPixel,
    .GetPixel           = VesaLinearFb_GetPixel,
    .FillRect           = VesaLinearFb_FillRect,
    .BlitMonoBitmap     = VesaLinearFb_BlitMonoBitmap,
    .BlitBitmap         = VesaLinearFb_BlitBitmap,
    .GetDescriptor      = VesaLinearFb_GetDescriptor,
    .SetDitherMethod    = VesaLinearFb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static INLINE VOID
VesaLinearFb_WritePixel(
    VESA_LINEAR_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT32 PixelValue
    )
{
    UINT8 *Addr;
    UINT32 Offset;
    UINT32 BytesPerPixel;

    BytesPerPixel = 4;  /* Most VESA modes use 32-bit */
    if (Backend->Descriptor.PixelFormat == FbPixelFormatRgb555 ||
        Backend->Descriptor.PixelFormat == FbPixelFormatRgb565) {
        BytesPerPixel = 2;
    } else if (Backend->Descriptor.PixelFormat == FbPixelFormatIndexed256) {
        BytesPerPixel = 1;
    }

    Offset = Y * Backend->Descriptor.Pitch + X * BytesPerPixel;
    Addr = Backend->FramebufferBase + Offset;

    switch (BytesPerPixel) {
        case 1:
            *(UINT8 *)Addr = (UINT8)PixelValue;
            break;
        case 2:
            *(UINT16 *)Addr = (UINT16)PixelValue;
            break;
        case 4:
            *(UINT32 *)Addr = PixelValue;
            break;
    }
}

static INLINE UINT32
VesaLinearFb_ReadPixel(
    VESA_LINEAR_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    UINT8 *Addr;
    UINT32 Offset;
    UINT32 BytesPerPixel;

    BytesPerPixel = 4;
    if (Backend->Descriptor.PixelFormat == FbPixelFormatRgb555 ||
        Backend->Descriptor.PixelFormat == FbPixelFormatRgb565) {
        BytesPerPixel = 2;
    } else if (Backend->Descriptor.PixelFormat == FbPixelFormatIndexed256) {
        BytesPerPixel = 1;
    }

    Offset = Y * Backend->Descriptor.Pitch + X * BytesPerPixel;
    Addr = Backend->FramebufferBase + Offset;

    switch (BytesPerPixel) {
        case 1:
            return *(UINT8 *)Addr;
        case 2:
            return *(UINT16 *)Addr;
        case 4:
            return *(UINT32 *)Addr;
        default:
            return 0;
    }
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
VesaLinearFb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    VESA_LINEAR_FB_BACKEND *Backend = (VESA_LINEAR_FB_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        VesaLinearFb_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
VesaLinearFb_AddRef(
    IFramebufferBackend *This
    )
{
    VESA_LINEAR_FB_BACKEND *Backend = (VESA_LINEAR_FB_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
VesaLinearFb_Release(
    IFramebufferBackend *This
    )
{
    VESA_LINEAR_FB_BACKEND *Backend = (VESA_LINEAR_FB_BACKEND *)This;
    return ANX_REF_DEC(&Backend->RefCount);
}

/* --------------------------------------------------------------- */
/*  IFramebufferBackend Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
VesaLinearFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    VESA_LINEAR_FB_BACKEND *Backend = (VESA_LINEAR_FB_BACKEND *)This;

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
VesaLinearFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    VESA_LINEAR_FB_BACKEND *Backend = (VESA_LINEAR_FB_BACKEND *)This;
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
            VesaLinearFb_WritePixel(Backend, x, y, PixelValue);
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VesaLinearFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    VESA_LINEAR_FB_BACKEND *Backend = (VESA_LINEAR_FB_BACKEND *)This;
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

    VesaLinearFb_WritePixel(Backend, X, Y, PixelValue);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VesaLinearFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    VESA_LINEAR_FB_BACKEND *Backend = (VESA_LINEAR_FB_BACKEND *)This;
    UINT32 PixelValue;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    PixelValue = VesaLinearFb_ReadPixel(Backend, X, Y);
    *Color = FbUnpackPixel(PixelValue, Backend->Descriptor.PixelFormat,
                           Backend->Descriptor.RedMask,
                           Backend->Descriptor.GreenMask,
                           Backend->Descriptor.BlueMask);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VesaLinearFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    VESA_LINEAR_FB_BACKEND *Backend = (VESA_LINEAR_FB_BACKEND *)This;
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
                VesaLinearFb_WritePixel(Backend, x, y, PixelValue);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VesaLinearFb_BlitMonoBitmap(
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
    VESA_LINEAR_FB_BACKEND *Backend = (VESA_LINEAR_FB_BACKEND *)This;
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
            INT32 Px = X + Col;
            INT32 Py = Y + Row;

            if (Px >= 0 && Px < (INT32)Backend->Descriptor.Width &&
                Py >= 0 && Py < (INT32)Backend->Descriptor.Height) {

                ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
                BitIndex = 7 - (Col % 8);

                if (Bitmap[ByteIndex] & (1 << BitIndex)) {
                    VesaLinearFb_WritePixel(Backend, Px, Py, FgPixel);
                } else {
                    VesaLinearFb_WritePixel(Backend, Px, Py, BgPixel);
                }
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VesaLinearFb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    /* Not implemented yet */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
VesaLinearFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    VESA_LINEAR_FB_BACKEND *Backend = (VESA_LINEAR_FB_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VesaLinearFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    VESA_LINEAR_FB_BACKEND *Backend = (VESA_LINEAR_FB_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static VESA_LINEAR_FB_BACKEND gVesaLinearBackendInstance = {
    .Base.lpVtbl        = &gVesaLinearFbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherNone,
    .VesaModeNumber     = 0,
};

IFramebufferBackend *
FbCreateVesaLinearBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gVesaLinearBackendInstance;
}
