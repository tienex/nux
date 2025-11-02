/*++
    Module Name:

        hercules.c

    Abstract:

        Hercules Graphics Card backend (720x348, 1BPP monochrome).
        Supports the Hercules Graphics Card (HGC) and compatible cards.

        Memory layout:
          - Base address: 0xB0000
          - Four interleaved banks of 8KB each
          - Each bank contains alternating scan lines

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/dither.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Hercules Hardware Constants                                    */
/* --------------------------------------------------------------- */

#define HERCULES_BASE_ADDR      0xB0000
#define HERCULES_WIDTH          720
#define HERCULES_HEIGHT         348
#define HERCULES_BYTES_PER_LINE 90  /* 720 / 8 */
#define HERCULES_BANK_SIZE      0x2000  /* 8KB per bank */

/* --------------------------------------------------------------- */
/*  Hercules Backend Structure                                      */
/* --------------------------------------------------------------- */

typedef struct _HERCULES_FB_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    UINT8                       *VideoMemory;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;
} HERCULES_FB_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE HercFb_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE HercFb_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE HercFb_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE HercFb_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE HercFb_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE HercFb_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE HercFb_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE HercFb_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE HercFb_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE HercFb_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE HercFb_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE HercFb_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gHercFbVtbl = {
    .QueryInterface     = HercFb_QueryInterface,
    .AddRef             = HercFb_AddRef,
    .Release            = HercFb_Release,
    .Initialize         = HercFb_Initialize,
    .Clear              = HercFb_Clear,
    .SetPixel           = HercFb_SetPixel,
    .GetPixel           = HercFb_GetPixel,
    .FillRect           = HercFb_FillRect,
    .BlitMonoBitmap     = HercFb_BlitMonoBitmap,
    .BlitBitmap         = HercFb_BlitBitmap,
    .GetDescriptor      = HercFb_GetDescriptor,
    .SetDitherMethod    = HercFb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static INLINE UINT8 *
HercFb_GetPixelAddr(
    HERCULES_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    UINT32 Bank;
    UINT32 Offset;

    /* Hercules uses 4 interleaved banks */
    Bank = (Y & 3);  /* Y % 4 */
    Offset = (Y / 4) * HERCULES_BYTES_PER_LINE + (X / 8);

    return Backend->VideoMemory + (Bank * HERCULES_BANK_SIZE) + Offset;
}

static INLINE VOID
HercFb_WritePixel(
    HERCULES_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    BOOLEAN PixelOn
    )
{
    UINT8 *Addr;
    UINT8 Mask;

    Addr = HercFb_GetPixelAddr(Backend, X, Y);
    Mask = 0x80 >> (X & 7);

    if (PixelOn) {
        *Addr |= Mask;
    } else {
        *Addr &= ~Mask;
    }
}

static INLINE BOOLEAN
HercFb_ReadPixel(
    HERCULES_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    UINT8 *Addr;
    UINT8 Mask;

    Addr = HercFb_GetPixelAddr(Backend, X, Y);
    Mask = 0x80 >> (X & 7);

    return (*Addr & Mask) != 0;
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
HercFb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    HERCULES_FB_BACKEND *Backend = (HERCULES_FB_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        HercFb_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
HercFb_AddRef(
    IFramebufferBackend *This
    )
{
    HERCULES_FB_BACKEND *Backend = (HERCULES_FB_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
HercFb_Release(
    IFramebufferBackend *This
    )
{
    HERCULES_FB_BACKEND *Backend = (HERCULES_FB_BACKEND *)This;
    return ANX_REF_DEC(&Backend->RefCount);
}

/* --------------------------------------------------------------- */
/*  IFramebufferBackend Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
HercFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    HERCULES_FB_BACKEND *Backend = (HERCULES_FB_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    /* Set up descriptor for Hercules */
    Backend->Descriptor.PixelFormat = FbPixelFormat1Bpp;
    Backend->Descriptor.Width = HERCULES_WIDTH;
    Backend->Descriptor.Height = HERCULES_HEIGHT;
    Backend->Descriptor.Pitch = HERCULES_BYTES_PER_LINE;
    Backend->Descriptor.PhysicalBase = HERCULES_BASE_ADDR;
    Backend->Descriptor.Size = HERCULES_BANK_SIZE * 4;

    Backend->VideoMemory = (UINT8 *)(UINTN)HERCULES_BASE_ADDR;
    Backend->Initialized = TRUE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HercFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    HERCULES_FB_BACKEND *Backend = (HERCULES_FB_BACKEND *)This;
    UINT32 i;
    UINT8 FillByte;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    /* Convert color to monochrome */
    FillByte = ((Color.Red + Color.Green + Color.Blue) >= 384) ? 0xFF : 0x00;

    /* Clear all 4 banks */
    for (i = 0; i < HERCULES_BANK_SIZE * 4; i++) {
        Backend->VideoMemory[i] = FillByte;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HercFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    HERCULES_FB_BACKEND *Backend = (HERCULES_FB_BACKEND *)This;
    BOOLEAN PixelOn;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    if (X < 0 || X >= HERCULES_WIDTH || Y < 0 || Y >= HERCULES_HEIGHT) {
        return E_INVALIDARG;
    }

    /* Apply dithering if enabled */
    if (Backend->DitherMethod == FbDitherClassicMac) {
        UINT8 Gray = (UINT8)((Color.Red * 77 + Color.Green * 150 + Color.Blue * 29) >> 8);
        PixelOn = FbDitherClassicMac(X, Y, Gray);
    } else {
        /* Simple threshold */
        PixelOn = ((Color.Red + Color.Green + Color.Blue) >= 384);
    }

    HercFb_WritePixel(Backend, X, Y, PixelOn);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HercFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    HERCULES_FB_BACKEND *Backend = (HERCULES_FB_BACKEND *)This;
    BOOLEAN PixelOn;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= HERCULES_WIDTH || Y < 0 || Y >= HERCULES_HEIGHT) {
        return E_INVALIDARG;
    }

    PixelOn = HercFb_ReadPixel(Backend, X, Y);

    Color->Red = Color->Green = Color->Blue = PixelOn ? 255 : 0;
    Color->Alpha = 255;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HercFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    HERCULES_FB_BACKEND *Backend = (HERCULES_FB_BACKEND *)This;
    INT32 x, y;
    BOOLEAN PixelOn;

    if (!Backend->Initialized || Rect == NULL) {
        return E_POINTER;
    }

    PixelOn = ((Color.Red + Color.Green + Color.Blue) >= 384);

    for (y = Rect->Top; y < Rect->Bottom && y < HERCULES_HEIGHT; y++) {
        for (x = Rect->Left; x < Rect->Right && x < HERCULES_WIDTH; x++) {
            if (x >= 0 && y >= 0) {
                HercFb_WritePixel(Backend, x, y, PixelOn);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HercFb_BlitMonoBitmap(
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
    HERCULES_FB_BACKEND *Backend = (HERCULES_FB_BACKEND *)This;
    UINT32 Row, Col;
    UINT32 ByteIndex, BitIndex;
    BOOLEAN FgOn, BgOn;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    FgOn = ((Foreground.Red + Foreground.Green + Foreground.Blue) >= 384);
    BgOn = ((Background.Red + Background.Green + Background.Blue) >= 384);

    for (Row = 0; Row < Height; Row++) {
        for (Col = 0; Col < Width; Col++) {
            INT32 Px = X + Col;
            INT32 Py = Y + Row;

            if (Px >= 0 && Px < HERCULES_WIDTH && Py >= 0 && Py < HERCULES_HEIGHT) {
                ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
                BitIndex = 7 - (Col % 8);

                if (Bitmap[ByteIndex] & (1 << BitIndex)) {
                    HercFb_WritePixel(Backend, Px, Py, FgOn);
                } else {
                    HercFb_WritePixel(Backend, Px, Py, BgOn);
                }
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HercFb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    HERCULES_FB_BACKEND *Backend = (HERCULES_FB_BACKEND *)This;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    /* Fast path: 1bpp monochrome format (native) */
    if (SourceFormat == FbPixelFormat1Bpp) {
        UINT32 BytesPerRow = (Width + 7) / 8;

        for (UINT32 Row = 0; Row < Height; Row++) {
            INT32 DestY = Y + Row;
            if (DestY < 0 || DestY >= HERCULES_HEIGHT) {
                continue;
            }

            /* For efficiency, use direct memory access for aligned cases */
            if ((X % 8) == 0 && (Width % 8) == 0) {
                /* Byte-aligned - direct copy */
                UINT8 *DestAddr = HercFb_GetPixelAddr(Backend, X, DestY);
                CONST UINT8 *SrcAddr = &Bitmap[Row * BytesPerRow];
                for (UINT32 ByteIdx = 0; ByteIdx < BytesPerRow; ByteIdx++) {
                    if ((X / 8 + ByteIdx) < HERCULES_BYTES_PER_LINE) {
                        DestAddr[ByteIdx] = SrcAddr[ByteIdx];
                    }
                }
            } else {
                /* Bit-aligned - pixel-by-pixel */
                for (UINT32 Col = 0; Col < Width; Col++) {
                    INT32 DestX = X + Col;
                    if (DestX < 0 || DestX >= HERCULES_WIDTH) {
                        continue;
                    }

                    UINT32 ByteIdx = Row * BytesPerRow + (Col / 8);
                    UINT32 BitIdx = 7 - (Col % 8);
                    BOOLEAN PixelOn = (Bitmap[ByteIdx] >> BitIdx) & 1;

                    HercFb_WritePixel(Backend, DestX, DestY, PixelOn);
                }
            }
        }
        return S_OK;
    }

    /* Grayscale formats - convert to 1bpp using threshold/dithering */
    if (SourceFormat == FbPixelFormat2Bpp ||
        SourceFormat == FbPixelFormat4Bpp ||
        SourceFormat == FbPixelFormat8Bpp) {

        for (UINT32 Row = 0; Row < Height; Row++) {
            INT32 DestY = Y + Row;
            if (DestY < 0 || DestY >= HERCULES_HEIGHT) {
                continue;
            }

            for (UINT32 Col = 0; Col < Width; Col++) {
                INT32 DestX = X + Col;
                if (DestX < 0 || DestX >= HERCULES_WIDTH) {
                    continue;
                }

                UINT8 GrayValue = 0;

                /* Extract grayscale value based on format */
                if (SourceFormat == FbPixelFormat2Bpp) {
                    UINT32 ByteIdx = Row * ((Width + 3) / 4) + (Col / 4);
                    UINT32 BitOffset = (3 - (Col % 4)) * 2;
                    UINT8 Value = (Bitmap[ByteIdx] >> BitOffset) & 0x03;
                    GrayValue = (Value * 255) / 3;
                } else if (SourceFormat == FbPixelFormat4Bpp) {
                    UINT32 ByteIdx = Row * ((Width + 1) / 2) + (Col / 2);
                    UINT8 Value = (Col & 1) ?
                        (Bitmap[ByteIdx] & 0x0F) :
                        ((Bitmap[ByteIdx] >> 4) & 0x0F);
                    GrayValue = (Value * 255) / 15;
                } else {  /* FbPixelFormat8Bpp */
                    UINT32 ByteIdx = Row * Width + Col;
                    GrayValue = Bitmap[ByteIdx];
                }

                /* Apply dithering or simple threshold */
                BOOLEAN PixelOn;
                if (Backend->DitherMethod == FbDitherClassicMac) {
                    PixelOn = FbDitherClassicMac(DestX, DestY, GrayValue);
                } else {
                    PixelOn = (GrayValue >= 128);
                }

                HercFb_WritePixel(Backend, DestX, DestY, PixelOn);
            }
        }
        return S_OK;
    }

    /* Other formats - not supported for monochrome display */
    /* Engine should convert to grayscale first */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
HercFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    HERCULES_FB_BACKEND *Backend = (HERCULES_FB_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
HercFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    HERCULES_FB_BACKEND *Backend = (HERCULES_FB_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static HERCULES_FB_BACKEND gHerculesBackendInstance = {
    .Base.lpVtbl        = &gHercFbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherClassicMac,
};

IFramebufferBackend *
FbCreateHerculesBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gHerculesBackendInstance;
}
