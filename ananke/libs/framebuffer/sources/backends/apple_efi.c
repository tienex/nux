/*++
    Module Name:

        apple_efi.c

    Abstract:

        Apple EFI framebuffer backend.
        Handles Apple-specific EFI framebuffer quirks and features.

        Apple Macs use standard UEFI GOP but with some quirks:
        - Often use non-standard resolutions (5K displays, etc.)
        - May have different pixel formats (BGR instead of RGB)
        - Some models have dual framebuffers (internal + external)

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/framebuffer/dither.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Apple EFI Backend Structure                                     */
/* --------------------------------------------------------------- */

typedef struct _APPLE_EFI_FB_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    UINT8                       *FramebufferBase;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;
    BOOLEAN                     IsBgrMode;      /* BGR vs RGB */
    BOOLEAN                     IsRetinaDisplay; /* HiDPI/Retina */
} APPLE_EFI_FB_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE AppleEfiFb_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE AppleEfiFb_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE AppleEfiFb_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE AppleEfiFb_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE AppleEfiFb_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE AppleEfiFb_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE AppleEfiFb_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE AppleEfiFb_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE AppleEfiFb_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE AppleEfiFb_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE AppleEfiFb_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE AppleEfiFb_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gAppleEfiFbVtbl = {
    .QueryInterface     = AppleEfiFb_QueryInterface,
    .AddRef             = AppleEfiFb_AddRef,
    .Release            = AppleEfiFb_Release,
    .Initialize         = AppleEfiFb_Initialize,
    .Clear              = AppleEfiFb_Clear,
    .SetPixel           = AppleEfiFb_SetPixel,
    .GetPixel           = AppleEfiFb_GetPixel,
    .FillRect           = AppleEfiFb_FillRect,
    .BlitMonoBitmap     = AppleEfiFb_BlitMonoBitmap,
    .BlitBitmap         = AppleEfiFb_BlitBitmap,
    .GetDescriptor      = AppleEfiFb_GetDescriptor,
    .SetDitherMethod    = AppleEfiFb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static INLINE UINT32
AppleEfiFb_PackPixelBgr(
    APPLE_EFI_FB_BACKEND *Backend,
    FB_COLOR Color
    )
{
    UINT32 Pixel;

    if (Backend->IsBgrMode) {
        /* Swap R and B for BGR mode */
        Pixel = ((UINT32)Color.Blue  << 16) |
                ((UINT32)Color.Green <<  8) |
                ((UINT32)Color.Red   <<  0);
    } else {
        /* Standard RGB */
        Pixel = FbPackPixel(Color, Backend->Descriptor.PixelFormat,
                           Backend->Descriptor.RedMask,
                           Backend->Descriptor.GreenMask,
                           Backend->Descriptor.BlueMask);
    }

    return Pixel;
}

static INLINE VOID
AppleEfiFb_WritePixel(
    APPLE_EFI_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT32 PixelValue
    )
{
    UINT8 *Addr;
    UINT32 Offset;

    Offset = Y * Backend->Descriptor.Pitch + X * 4;
    Addr = Backend->FramebufferBase + Offset;
    *(UINT32 *)Addr = PixelValue;
}

static INLINE UINT32
AppleEfiFb_ReadPixel(
    APPLE_EFI_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    UINT8 *Addr;
    UINT32 Offset;

    Offset = Y * Backend->Descriptor.Pitch + X * 4;
    Addr = Backend->FramebufferBase + Offset;
    return *(UINT32 *)Addr;
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
AppleEfiFb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    APPLE_EFI_FB_BACKEND *Backend = (APPLE_EFI_FB_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        AppleEfiFb_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
AppleEfiFb_AddRef(
    IFramebufferBackend *This
    )
{
    APPLE_EFI_FB_BACKEND *Backend = (APPLE_EFI_FB_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
AppleEfiFb_Release(
    IFramebufferBackend *This
    )
{
    APPLE_EFI_FB_BACKEND *Backend = (APPLE_EFI_FB_BACKEND *)This;
    return ANX_REF_DEC(&Backend->RefCount);
}

/* --------------------------------------------------------------- */
/*  IFramebufferBackend Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
AppleEfiFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    APPLE_EFI_FB_BACKEND *Backend = (APPLE_EFI_FB_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    Backend->Descriptor = *Descriptor;
    Backend->FramebufferBase = (UINT8 *)(UINTN)Descriptor->PhysicalBase;

    /* Detect BGR mode by checking mask ordering */
    if (Descriptor->BlueMask > Descriptor->RedMask) {
        Backend->IsBgrMode = TRUE;
    }

    /* Detect Retina display (typically > 2560 width) */
    if (Descriptor->Width > 2560) {
        Backend->IsRetinaDisplay = TRUE;
    }

    Backend->Initialized = TRUE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AppleEfiFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    APPLE_EFI_FB_BACKEND *Backend = (APPLE_EFI_FB_BACKEND *)This;
    UINT32 PixelValue;
    UINT32 x, y;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    PixelValue = AppleEfiFb_PackPixelBgr(Backend, Color);

    for (y = 0; y < Backend->Descriptor.Height; y++) {
        for (x = 0; x < Backend->Descriptor.Width; x++) {
            AppleEfiFb_WritePixel(Backend, x, y, PixelValue);
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AppleEfiFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    APPLE_EFI_FB_BACKEND *Backend = (APPLE_EFI_FB_BACKEND *)This;
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

    PixelValue = AppleEfiFb_PackPixelBgr(Backend, DitheredColor);
    AppleEfiFb_WritePixel(Backend, X, Y, PixelValue);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AppleEfiFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    APPLE_EFI_FB_BACKEND *Backend = (APPLE_EFI_FB_BACKEND *)This;
    UINT32 PixelValue;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    PixelValue = AppleEfiFb_ReadPixel(Backend, X, Y);

    if (Backend->IsBgrMode) {
        /* Swap R and B when reading */
        Color->Blue  = (PixelValue >> 16) & 0xFF;
        Color->Green = (PixelValue >>  8) & 0xFF;
        Color->Red   = (PixelValue >>  0) & 0xFF;
        Color->Alpha = 255;
    } else {
        *Color = FbUnpackPixel(PixelValue, Backend->Descriptor.PixelFormat,
                              Backend->Descriptor.RedMask,
                              Backend->Descriptor.GreenMask,
                              Backend->Descriptor.BlueMask);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AppleEfiFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    APPLE_EFI_FB_BACKEND *Backend = (APPLE_EFI_FB_BACKEND *)This;
    UINT32 PixelValue;
    INT32 x, y;

    if (!Backend->Initialized || Rect == NULL) {
        return E_POINTER;
    }

    PixelValue = AppleEfiFb_PackPixelBgr(Backend, Color);

    for (y = Rect->Top; y < Rect->Bottom && y < (INT32)Backend->Descriptor.Height; y++) {
        for (x = Rect->Left; x < Rect->Right && x < (INT32)Backend->Descriptor.Width; x++) {
            if (x >= 0 && y >= 0) {
                AppleEfiFb_WritePixel(Backend, x, y, PixelValue);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AppleEfiFb_BlitMonoBitmap(
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
    APPLE_EFI_FB_BACKEND *Backend = (APPLE_EFI_FB_BACKEND *)This;
    UINT32 FgPixel, BgPixel;
    UINT32 Row, Col;
    UINT32 ByteIndex, BitIndex;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    FgPixel = AppleEfiFb_PackPixelBgr(Backend, Foreground);
    BgPixel = AppleEfiFb_PackPixelBgr(Backend, Background);

    for (Row = 0; Row < Height; Row++) {
        for (Col = 0; Col < Width; Col++) {
            INT32 Px = X + Col;
            INT32 Py = Y + Row;

            if (Px >= 0 && Px < (INT32)Backend->Descriptor.Width &&
                Py >= 0 && Py < (INT32)Backend->Descriptor.Height) {

                ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
                BitIndex = 7 - (Col % 8);

                if (Bitmap[ByteIndex] & (1 << BitIndex)) {
                    AppleEfiFb_WritePixel(Backend, Px, Py, FgPixel);
                } else {
                    AppleEfiFb_WritePixel(Backend, Px, Py, BgPixel);
                }
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AppleEfiFb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    APPLE_EFI_FB_BACKEND *Backend = (APPLE_EFI_FB_BACKEND *)This;

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

    /* Format conversion handled by engine */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
AppleEfiFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    APPLE_EFI_FB_BACKEND *Backend = (APPLE_EFI_FB_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
AppleEfiFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    APPLE_EFI_FB_BACKEND *Backend = (APPLE_EFI_FB_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static APPLE_EFI_FB_BACKEND gAppleEfiBackendInstance = {
    .Base.lpVtbl        = &gAppleEfiFbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherNone,
    .IsBgrMode          = FALSE,
    .IsRetinaDisplay    = FALSE,
};

IFramebufferBackend *
FbCreateAppleEfiBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gAppleEfiBackendInstance;
}
