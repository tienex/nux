/*++
    Module Name:

        vesa_banked.c

    Abstract:

        VESA BIOS Extensions (VBE) banked/segmented framebuffer backend.
        Supports VESA 1.x/2.0 banked modes with 64KB window switching.

        This backend handles the complexities of bank switching for
        accessing framebuffer memory through a small window (typically 64KB).

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/framebuffer/dither.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  VESA Banked Constants                                           */
/* --------------------------------------------------------------- */

#define VESA_BANK_SIZE          0x10000  /* 64KB */
#define VESA_BANK_MASK          0xFFFF
#define VESA_BANK_SHIFT         16

/* --------------------------------------------------------------- */
/*  VESA Banked Backend Structure                                   */
/* --------------------------------------------------------------- */

typedef struct _VESA_BANKED_FB_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    UINT8                       *WindowBase;
    UINT32                      CurrentBank;
    UINT32                      WindowGranularity;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;

    /* Bank switching function pointer (set via INT 10h) */
    VOID (*SetBank)(UINT32 Bank);
} VESA_BANKED_FB_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE VesaBankedFb_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE VesaBankedFb_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE VesaBankedFb_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE VesaBankedFb_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE VesaBankedFb_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE VesaBankedFb_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE VesaBankedFb_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE VesaBankedFb_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE VesaBankedFb_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE VesaBankedFb_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE VesaBankedFb_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE VesaBankedFb_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gVesaBankedFbVtbl = {
    .QueryInterface     = VesaBankedFb_QueryInterface,
    .AddRef             = VesaBankedFb_AddRef,
    .Release            = VesaBankedFb_Release,
    .Initialize         = VesaBankedFb_Initialize,
    .Clear              = VesaBankedFb_Clear,
    .SetPixel           = VesaBankedFb_SetPixel,
    .GetPixel           = VesaBankedFb_GetPixel,
    .FillRect           = VesaBankedFb_FillRect,
    .BlitMonoBitmap     = VesaBankedFb_BlitMonoBitmap,
    .BlitBitmap         = VesaBankedFb_BlitBitmap,
    .GetDescriptor      = VesaBankedFb_GetDescriptor,
    .SetDitherMethod    = VesaBankedFb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  Bank Switching Helpers                                          */
/* --------------------------------------------------------------- */

static VOID
VesaBankedFb_SwitchToBank(
    VESA_BANKED_FB_BACKEND *Backend,
    UINT32 Bank
    )
{
    if (Backend->CurrentBank != Bank) {
        if (Backend->SetBank != NULL) {
            Backend->SetBank(Bank);
        }
        Backend->CurrentBank = Bank;
    }
}

static INLINE VOID
VesaBankedFb_MapAddress(
    VESA_BANKED_FB_BACKEND *Backend,
    UINT32 LinearOffset,
    UINT32 *Bank,
    UINT32 *WindowOffset
    )
{
    *Bank = LinearOffset / VESA_BANK_SIZE;
    *WindowOffset = LinearOffset & VESA_BANK_MASK;
}

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static VOID
VesaBankedFb_WritePixel(
    VESA_BANKED_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT32 PixelValue
    )
{
    UINT32 LinearOffset;
    UINT32 Bank, WindowOffset;
    UINT32 BytesPerPixel;

    BytesPerPixel = 4;
    if (Backend->Descriptor.PixelFormat == FbPixelFormatRgb555 ||
        Backend->Descriptor.PixelFormat == FbPixelFormatRgb565) {
        BytesPerPixel = 2;
    } else if (Backend->Descriptor.PixelFormat == FbPixelFormatIndexed256) {
        BytesPerPixel = 1;
    }

    LinearOffset = Y * Backend->Descriptor.Pitch + X * BytesPerPixel;
    VesaBankedFb_MapAddress(Backend, LinearOffset, &Bank, &WindowOffset);
    VesaBankedFb_SwitchToBank(Backend, Bank);

    switch (BytesPerPixel) {
        case 1:
            Backend->WindowBase[WindowOffset] = (UINT8)PixelValue;
            break;
        case 2:
            *(UINT16 *)(Backend->WindowBase + WindowOffset) = (UINT16)PixelValue;
            break;
        case 4:
            *(UINT32 *)(Backend->WindowBase + WindowOffset) = PixelValue;
            break;
    }
}

static UINT32
VesaBankedFb_ReadPixel(
    VESA_BANKED_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    UINT32 LinearOffset;
    UINT32 Bank, WindowOffset;
    UINT32 BytesPerPixel;

    BytesPerPixel = 4;
    if (Backend->Descriptor.PixelFormat == FbPixelFormatRgb555 ||
        Backend->Descriptor.PixelFormat == FbPixelFormatRgb565) {
        BytesPerPixel = 2;
    } else if (Backend->Descriptor.PixelFormat == FbPixelFormatIndexed256) {
        BytesPerPixel = 1;
    }

    LinearOffset = Y * Backend->Descriptor.Pitch + X * BytesPerPixel;
    VesaBankedFb_MapAddress(Backend, LinearOffset, &Bank, &WindowOffset);
    VesaBankedFb_SwitchToBank(Backend, Bank);

    switch (BytesPerPixel) {
        case 1:
            return Backend->WindowBase[WindowOffset];
        case 2:
            return *(UINT16 *)(Backend->WindowBase + WindowOffset);
        case 4:
            return *(UINT32 *)(Backend->WindowBase + WindowOffset);
        default:
            return 0;
    }
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
VesaBankedFb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    VESA_BANKED_FB_BACKEND *Backend = (VESA_BANKED_FB_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        VesaBankedFb_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
VesaBankedFb_AddRef(
    IFramebufferBackend *This
    )
{
    VESA_BANKED_FB_BACKEND *Backend = (VESA_BANKED_FB_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
VesaBankedFb_Release(
    IFramebufferBackend *This
    )
{
    VESA_BANKED_FB_BACKEND *Backend = (VESA_BANKED_FB_BACKEND *)This;
    return ANX_REF_DEC(&Backend->RefCount);
}

/* --------------------------------------------------------------- */
/*  IFramebufferBackend Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
VesaBankedFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    VESA_BANKED_FB_BACKEND *Backend = (VESA_BANKED_FB_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    Backend->Descriptor = *Descriptor;
    Backend->WindowBase = (UINT8 *)(UINTN)0xA0000;  /* Standard VGA window */
    Backend->CurrentBank = 0xFFFFFFFF;  /* Force initial bank switch */
    Backend->WindowGranularity = 64;  /* 64KB granularity */
    Backend->Initialized = TRUE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VesaBankedFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    VESA_BANKED_FB_BACKEND *Backend = (VESA_BANKED_FB_BACKEND *)This;
    UINT32 PixelValue;
    UINT32 x, y;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    PixelValue = FbPackPixel(Color, Backend->Descriptor.PixelFormat,
                             Backend->Descriptor.RedMask,
                             Backend->Descriptor.GreenMask,
                             Backend->Descriptor.BlueMask);

    /* Clear using banked access */
    for (y = 0; y < Backend->Descriptor.Height; y++) {
        for (x = 0; x < Backend->Descriptor.Width; x++) {
            VesaBankedFb_WritePixel(Backend, x, y, PixelValue);
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VesaBankedFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    VESA_BANKED_FB_BACKEND *Backend = (VESA_BANKED_FB_BACKEND *)This;
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

    VesaBankedFb_WritePixel(Backend, X, Y, PixelValue);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VesaBankedFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    VESA_BANKED_FB_BACKEND *Backend = (VESA_BANKED_FB_BACKEND *)This;
    UINT32 PixelValue;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    PixelValue = VesaBankedFb_ReadPixel(Backend, X, Y);
    *Color = FbUnpackPixel(PixelValue, Backend->Descriptor.PixelFormat,
                           Backend->Descriptor.RedMask,
                           Backend->Descriptor.GreenMask,
                           Backend->Descriptor.BlueMask);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VesaBankedFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    VESA_BANKED_FB_BACKEND *Backend = (VESA_BANKED_FB_BACKEND *)This;
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
                VesaBankedFb_WritePixel(Backend, x, y, PixelValue);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VesaBankedFb_BlitMonoBitmap(
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
    VESA_BANKED_FB_BACKEND *Backend = (VESA_BANKED_FB_BACKEND *)This;
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
                    VesaBankedFb_WritePixel(Backend, Px, Py, FgPixel);
                } else {
                    VesaBankedFb_WritePixel(Backend, Px, Py, BgPixel);
                }
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VesaBankedFb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
VesaBankedFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    VESA_BANKED_FB_BACKEND *Backend = (VESA_BANKED_FB_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VesaBankedFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    VESA_BANKED_FB_BACKEND *Backend = (VESA_BANKED_FB_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static VESA_BANKED_FB_BACKEND gVesaBankedBackendInstance = {
    .Base.lpVtbl        = &gVesaBankedFbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherNone,
    .SetBank            = NULL,
};

IFramebufferBackend *
FbCreateVesaBankedBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gVesaBankedBackendInstance;
}

/*
 * Set the bank switching function.
 * This should be called after backend creation with a function
 * that performs VESA INT 10h AX=4F05h bank switching.
 */
VOID
FbVesaBankedSetBankFunction(
    IFramebufferBackend *Backend,
    VOID (*SetBankFunc)(UINT32)
    )
{
    VESA_BANKED_FB_BACKEND *VesaBackend = (VESA_BANKED_FB_BACKEND *)Backend;
    VesaBackend->SetBank = SetBankFunc;
}
