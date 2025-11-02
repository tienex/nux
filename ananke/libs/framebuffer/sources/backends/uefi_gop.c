/*++
    Module Name:

        uefi_gop.c

    Abstract:

        UEFI Graphics Output Protocol (GOP) framebuffer backend.
        Provides framebuffer access through UEFI GOP interface.

        This backend wraps the UEFI GOP protocol and provides
        a simpler COM interface for framebuffer operations.

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/framebuffer/dither.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  UEFI GOP Backend Structure                                      */
/* --------------------------------------------------------------- */

typedef struct _UEFI_GOP_FB_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    UINT8                       *FramebufferBase;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;

    /* UEFI GOP specific */
    VOID                        *GopProtocol;  /* EFI_GRAPHICS_OUTPUT_PROTOCOL* */
} UEFI_GOP_FB_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE UefiGopFb_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE UefiGopFb_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE UefiGopFb_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE UefiGopFb_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE UefiGopFb_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE UefiGopFb_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE UefiGopFb_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE UefiGopFb_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE UefiGopFb_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE UefiGopFb_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE UefiGopFb_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE UefiGopFb_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gUefiGopFbVtbl = {
    .QueryInterface     = UefiGopFb_QueryInterface,
    .AddRef             = UefiGopFb_AddRef,
    .Release            = UefiGopFb_Release,
    .Initialize         = UefiGopFb_Initialize,
    .Clear              = UefiGopFb_Clear,
    .SetPixel           = UefiGopFb_SetPixel,
    .GetPixel           = UefiGopFb_GetPixel,
    .FillRect           = UefiGopFb_FillRect,
    .BlitMonoBitmap     = UefiGopFb_BlitMonoBitmap,
    .BlitBitmap         = UefiGopFb_BlitBitmap,
    .GetDescriptor      = UefiGopFb_GetDescriptor,
    .SetDitherMethod    = UefiGopFb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static INLINE VOID
UefiGopFb_WritePixel(
    UEFI_GOP_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT32 PixelValue
    )
{
    UINT8 *Addr;
    UINT32 Offset;

    /* Most UEFI GOP uses 32-bit pixels */
    Offset = Y * Backend->Descriptor.Pitch + X * 4;
    Addr = Backend->FramebufferBase + Offset;

    switch (Backend->Descriptor.PixelFormat) {
        case FbPixelFormatRgb888:
            *(UINT32 *)Addr = PixelValue;
            break;

        case FbPixelFormatRgb565:
        case FbPixelFormatRgb555:
            *(UINT16 *)Addr = (UINT16)PixelValue;
            break;

        case FbPixelFormatIndexed256:
            *(UINT8 *)Addr = (UINT8)PixelValue;
            break;

        default:
            *(UINT32 *)Addr = PixelValue;
            break;
    }
}

static INLINE UINT32
UefiGopFb_ReadPixel(
    UEFI_GOP_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    UINT8 *Addr;
    UINT32 Offset;

    Offset = Y * Backend->Descriptor.Pitch + X * 4;
    Addr = Backend->FramebufferBase + Offset;

    switch (Backend->Descriptor.PixelFormat) {
        case FbPixelFormatRgb888:
            return *(UINT32 *)Addr;

        case FbPixelFormatRgb565:
        case FbPixelFormatRgb555:
            return *(UINT16 *)Addr;

        case FbPixelFormatIndexed256:
            return *(UINT8 *)Addr;

        default:
            return *(UINT32 *)Addr;
    }
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
UefiGopFb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    UEFI_GOP_FB_BACKEND *Backend = (UEFI_GOP_FB_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        UefiGopFb_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
UefiGopFb_AddRef(
    IFramebufferBackend *This
    )
{
    UEFI_GOP_FB_BACKEND *Backend = (UEFI_GOP_FB_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
UefiGopFb_Release(
    IFramebufferBackend *This
    )
{
    UEFI_GOP_FB_BACKEND *Backend = (UEFI_GOP_FB_BACKEND *)This;
    return ANX_REF_DEC(&Backend->RefCount);
}

/* --------------------------------------------------------------- */
/*  IFramebufferBackend Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
UefiGopFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    UEFI_GOP_FB_BACKEND *Backend = (UEFI_GOP_FB_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    /* Copy descriptor from UEFI GOP mode info */
    Backend->Descriptor = *Descriptor;
    Backend->FramebufferBase = (UINT8 *)(UINTN)Descriptor->PhysicalBase;
    Backend->Initialized = TRUE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
UefiGopFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    UEFI_GOP_FB_BACKEND *Backend = (UEFI_GOP_FB_BACKEND *)This;
    UINT32 PixelValue;
    UINT32 x, y;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    PixelValue = FbPackPixel(Color, Backend->Descriptor.PixelFormat,
                             Backend->Descriptor.RedMask,
                             Backend->Descriptor.GreenMask,
                             Backend->Descriptor.BlueMask);

    /* Use Blt to clear if GOP protocol available */
    /* Otherwise fall back to pixel-by-pixel clear */
    for (y = 0; y < Backend->Descriptor.Height; y++) {
        for (x = 0; x < Backend->Descriptor.Width; x++) {
            UefiGopFb_WritePixel(Backend, x, y, PixelValue);
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
UefiGopFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    UEFI_GOP_FB_BACKEND *Backend = (UEFI_GOP_FB_BACKEND *)This;
    UINT32 PixelValue;
    FB_COLOR DitheredColor;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    DitheredColor = Color;
    if (Backend->DitherMethod == FbDitherClassicMac) {
        FbDitherRgb(X, Y, &DitheredColor, Backend->Descriptor.PixelFormat);
    }

    PixelValue = FbPackPixel(DitheredColor, Backend->Descriptor.PixelFormat,
                             Backend->Descriptor.RedMask,
                             Backend->Descriptor.GreenMask,
                             Backend->Descriptor.BlueMask);

    UefiGopFb_WritePixel(Backend, X, Y, PixelValue);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
UefiGopFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    UEFI_GOP_FB_BACKEND *Backend = (UEFI_GOP_FB_BACKEND *)This;
    UINT32 PixelValue;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    PixelValue = UefiGopFb_ReadPixel(Backend, X, Y);
    *Color = FbUnpackPixel(PixelValue, Backend->Descriptor.PixelFormat,
                           Backend->Descriptor.RedMask,
                           Backend->Descriptor.GreenMask,
                           Backend->Descriptor.BlueMask);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
UefiGopFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    UEFI_GOP_FB_BACKEND *Backend = (UEFI_GOP_FB_BACKEND *)This;
    UINT32 PixelValue;
    INT32 x, y;

    if (!Backend->Initialized || Rect == NULL) {
        return E_POINTER;
    }

    PixelValue = FbPackPixel(Color, Backend->Descriptor.PixelFormat,
                             Backend->Descriptor.RedMask,
                             Backend->Descriptor.GreenMask,
                             Backend->Descriptor.BlueMask);

    /* Could use GOP Blt for acceleration here */
    for (y = Rect->Top; y < Rect->Bottom && y < (INT32)Backend->Descriptor.Height; y++) {
        for (x = Rect->Left; x < Rect->Right && x < (INT32)Backend->Descriptor.Width; x++) {
            if (x >= 0 && y >= 0) {
                UefiGopFb_WritePixel(Backend, x, y, PixelValue);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
UefiGopFb_BlitMonoBitmap(
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
    UEFI_GOP_FB_BACKEND *Backend = (UEFI_GOP_FB_BACKEND *)This;
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
                    UefiGopFb_WritePixel(Backend, Px, Py, FgPixel);
                } else {
                    UefiGopFb_WritePixel(Backend, Px, Py, BgPixel);
                }
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
UefiGopFb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    UEFI_GOP_FB_BACKEND *Backend = (UEFI_GOP_FB_BACKEND *)This;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    /* Fast path: matching pixel format */
    if (SourceFormat == Backend->Descriptor.PixelFormat) {
        UINT32 BytesPerPixel = 0;

        /* Determine bytes per pixel */
        if (Backend->Descriptor.PixelFormat == FbPixelFormatIndexed256) {
            BytesPerPixel = 1;
        } else if (Backend->Descriptor.PixelFormat == FbPixelFormatRgb555 ||
                   Backend->Descriptor.PixelFormat == FbPixelFormatRgb565) {
            BytesPerPixel = 2;
        } else if (Backend->Descriptor.PixelFormat == FbPixelFormatRgb888 ||
                   Backend->Descriptor.PixelFormat == FbPixelFormatBgr888) {
            BytesPerPixel = 3;
        } else if (Backend->Descriptor.PixelFormat == FbPixelFormatRgba8888 ||
                   Backend->Descriptor.PixelFormat == FbPixelFormatBgra8888) {
            BytesPerPixel = 4;
        }

        if (BytesPerPixel > 0) {
            /* Direct row-by-row copy */
            /* TODO: Could use GOP Blt() for hardware acceleration */
            for (UINT32 Row = 0; Row < Height; Row++) {
                INT32 DestY = Y + Row;
                if (DestY < 0 || DestY >= (INT32)Backend->Descriptor.Height) {
                    continue;
                }

                UINT32 DestOffset = DestY * Backend->Descriptor.Pitch + X * BytesPerPixel;
                UINT32 SrcOffset = Row * Width * BytesPerPixel;
                UINT32 CopyWidth = Width * BytesPerPixel;

                /* Bounds check */
                if (X >= 0 && (X + Width) <= Backend->Descriptor.Width) {
                    /* Simple memcpy for unclipped case */
                    UINT8 *DestAddr = Backend->FramebufferBase + DestOffset;
                    CONST UINT8 *SrcAddr = &Bitmap[SrcOffset];
                    for (UINT32 i = 0; i < CopyWidth; i++) {
                        DestAddr[i] = SrcAddr[i];
                    }
                } else {
                    /* Clipped - copy pixel by pixel */
                    for (UINT32 Col = 0; Col < Width; Col++) {
                        INT32 DestX = X + Col;
                        if (DestX >= 0 && DestX < (INT32)Backend->Descriptor.Width) {
                            UINT8 *DestAddr = Backend->FramebufferBase + DestOffset + Col * BytesPerPixel;
                            CONST UINT8 *SrcAddr = &Bitmap[SrcOffset + Col * BytesPerPixel];
                            for (UINT32 b = 0; b < BytesPerPixel; b++) {
                                DestAddr[b] = SrcAddr[b];
                            }
                        }
                    }
                }
            }
            return S_OK;
        }
    }

    /* Other formats - let engine handle conversion */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
UefiGopFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    UEFI_GOP_FB_BACKEND *Backend = (UEFI_GOP_FB_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
UefiGopFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    UEFI_GOP_FB_BACKEND *Backend = (UEFI_GOP_FB_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static UEFI_GOP_FB_BACKEND gUefiGopBackendInstance = {
    .Base.lpVtbl        = &gUefiGopFbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherNone,
    .GopProtocol        = NULL,
};

IFramebufferBackend *
FbCreateUefiGopBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gUefiGopBackendInstance;
}

/*
 * Set the GOP protocol instance for accelerated operations.
 * This is optional but recommended for better performance.
 */
VOID
FbUefiGopSetProtocol(
    IFramebufferBackend *Backend,
    VOID *GopProtocol
    )
{
    UEFI_GOP_FB_BACKEND *UefiBackend = (UEFI_GOP_FB_BACKEND *)Backend;
    UefiBackend->GopProtocol = GopProtocol;
}
