/*++
    Module Name:

        ega.c

    Abstract:

        IBM EGA (Enhanced Graphics Adapter) backend implementation.
        Supports 640x350x16 planar mode with 4 bit planes.

        EGA memory is located at 0xA0000 with planar memory organization.
        Each plane contains 1 bit per pixel, combined to form 16 colors.

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/backend_ext.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  EGA Hardware Definitions                                        */
/* --------------------------------------------------------------- */

#define EGA_BASE_ADDR       0xA0000
#define EGA_MEMORY_SIZE     0x10000   /* 64KB visible */
#define EGA_TOTAL_PLANES    4

/* EGA I/O Ports */
#define EGA_SEQ_INDEX       0x3C4     /* Sequencer Index */
#define EGA_SEQ_DATA        0x3C5     /* Sequencer Data */
#define EGA_GC_INDEX        0x3CE     /* Graphics Controller Index */
#define EGA_GC_DATA         0x3CF     /* Graphics Controller Data */

/* Sequencer Registers */
#define EGA_SEQ_MAP_MASK    0x02      /* Map Mask Register */

/* Graphics Controller Registers */
#define EGA_GC_READ_MAP     0x04      /* Read Map Select */
#define EGA_GC_MODE         0x05      /* Mode Register */
#define EGA_GC_BIT_MASK     0x08      /* Bit Mask */

/* --------------------------------------------------------------- */
/*  EGA Backend Structure                                           */
/* --------------------------------------------------------------- */

typedef struct _EGA_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    UINT8                       *FramebufferBase;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;

    /* EGA-specific state */
    UINT8                       CurrentMapMask;
    UINT8                       CurrentReadMap;
} EGA_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE EgaFb_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE EgaFb_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE EgaFb_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE EgaFb_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE EgaFb_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE EgaFb_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE EgaFb_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE EgaFb_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE EgaFb_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE EgaFb_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE EgaFb_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE EgaFb_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gEgaFbVtbl = {
    .QueryInterface     = EgaFb_QueryInterface,
    .AddRef             = EgaFb_AddRef,
    .Release            = EgaFb_Release,
    .Initialize         = EgaFb_Initialize,
    .Clear              = EgaFb_Clear,
    .SetPixel           = EgaFb_SetPixel,
    .GetPixel           = EgaFb_GetPixel,
    .FillRect           = EgaFb_FillRect,
    .BlitMonoBitmap     = EgaFb_BlitMonoBitmap,
    .BlitBitmap         = EgaFb_BlitBitmap,
    .GetDescriptor      = EgaFb_GetDescriptor,
    .SetDitherMethod    = EgaFb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  EGA Standard Palette (64-color EGA palette)                     */
/* --------------------------------------------------------------- */

static CONST FB_COLOR gEgaPalette[16] = {
    { 0x00, 0x00, 0x00, 0xFF },  /* 0: Black */
    { 0x00, 0x00, 0xAA, 0xFF },  /* 1: Blue */
    { 0x00, 0xAA, 0x00, 0xFF },  /* 2: Green */
    { 0x00, 0xAA, 0xAA, 0xFF },  /* 3: Cyan */
    { 0xAA, 0x00, 0x00, 0xFF },  /* 4: Red */
    { 0xAA, 0x00, 0xAA, 0xFF },  /* 5: Magenta */
    { 0xAA, 0x55, 0x00, 0xFF },  /* 6: Brown */
    { 0xAA, 0xAA, 0xAA, 0xFF },  /* 7: Light Gray */
    { 0x55, 0x55, 0x55, 0xFF },  /* 8: Dark Gray */
    { 0x55, 0x55, 0xFF, 0xFF },  /* 9: Light Blue */
    { 0x55, 0xFF, 0x55, 0xFF },  /* 10: Light Green */
    { 0x55, 0xFF, 0xFF, 0xFF },  /* 11: Light Cyan */
    { 0xFF, 0x55, 0x55, 0xFF },  /* 12: Light Red */
    { 0xFF, 0x55, 0xFF, 0xFF },  /* 13: Light Magenta */
    { 0xFF, 0xFF, 0x55, 0xFF },  /* 14: Yellow */
    { 0xFF, 0xFF, 0xFF, 0xFF },  /* 15: White */
};

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static INLINE VOID
EgaFb_SetMapMask(
    EGA_BACKEND *Backend,
    UINT8 Mask
    )
{
    if (Backend->CurrentMapMask != Mask) {
        /* Note: In a real implementation, would do I/O port writes:
         * outb(EGA_SEQ_INDEX, EGA_SEQ_MAP_MASK);
         * outb(EGA_SEQ_DATA, Mask);
         */
        Backend->CurrentMapMask = Mask;
    }
}

static INLINE VOID
EgaFb_SetReadMap(
    EGA_BACKEND *Backend,
    UINT8 Plane
    )
{
    if (Backend->CurrentReadMap != Plane) {
        /* Note: In a real implementation, would do I/O port writes:
         * outb(EGA_GC_INDEX, EGA_GC_READ_MAP);
         * outb(EGA_GC_DATA, Plane);
         */
        Backend->CurrentReadMap = Plane;
    }
}

static VOID
EgaFb_WritePixelPlanar(
    EGA_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT8 ColorIndex
    )
{
    UINT32 Offset = Y * (Backend->Descriptor.Width / 8) + (X / 8);
    UINT8 BitMask = 0x80 >> (X % 8);
    UINT8 *Addr = Backend->FramebufferBase + Offset;

    /* Write to all planes simultaneously using map mask */
    EgaFb_SetMapMask(Backend, 0x0F);  /* Enable all planes */

    /* Read the latch */
    volatile UINT8 Dummy = *Addr;
    (VOID)Dummy;

    /* Write with appropriate plane mask */
    for (UINT32 Plane = 0; Plane < 4; Plane++) {
        UINT8 PlaneMask = (1 << Plane);
        EgaFb_SetMapMask(Backend, PlaneMask);

        if (ColorIndex & PlaneMask) {
            *Addr = *Addr | BitMask;
        } else {
            *Addr = *Addr & ~BitMask;
        }
    }
}

static UINT8
EgaFb_MapColorToIndex(
    FB_COLOR Color
    )
{
    return FbFindClosestPaletteEntry(Color, (CONST FB_PALETTE_ENTRY *)gEgaPalette, 16);
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
EgaFb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    EGA_BACKEND *Backend = (EGA_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        EgaFb_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
EgaFb_AddRef(
    IFramebufferBackend *This
    )
{
    EGA_BACKEND *Backend = (EGA_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
EgaFb_Release(
    IFramebufferBackend *This
    )
{
    EGA_BACKEND *Backend = (EGA_BACKEND *)This;
    UINT32 RefCount = ANX_REF_DEC(&Backend->RefCount);

    if (RefCount == 0) {
        /* Cleanup */
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  IFramebufferBackend Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
EgaFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    EGA_BACKEND *Backend = (EGA_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    Backend->Descriptor = *Descriptor;
    Backend->FramebufferBase = (UINT8 *)(UINTN)Descriptor->PhysicalBase;
    Backend->Initialized = TRUE;
    Backend->CurrentMapMask = 0x0F;
    Backend->CurrentReadMap = 0;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
EgaFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    EGA_BACKEND *Backend = (EGA_BACKEND *)This;
    UINT8 ColorIndex;
    UINT32 Size;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    ColorIndex = EgaFb_MapColorToIndex(Color);

    /* Clear each plane */
    Size = Backend->Descriptor.Width * Backend->Descriptor.Height / 8;

    for (UINT32 Plane = 0; Plane < 4; Plane++) {
        UINT8 PlaneMask = (1 << Plane);
        UINT8 FillValue = (ColorIndex & PlaneMask) ? 0xFF : 0x00;

        EgaFb_SetMapMask(Backend, PlaneMask);

        for (UINT32 i = 0; i < Size; i++) {
            Backend->FramebufferBase[i] = FillValue;
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
EgaFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    EGA_BACKEND *Backend = (EGA_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    ColorIndex = EgaFb_MapColorToIndex(Color);
    EgaFb_WritePixelPlanar(Backend, X, Y, ColorIndex);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
EgaFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    EGA_BACKEND *Backend = (EGA_BACKEND *)This;
    UINT32 Offset;
    UINT8 BitMask;
    UINT8 ColorIndex = 0;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    Offset = Y * (Backend->Descriptor.Width / 8) + (X / 8);
    BitMask = 0x80 >> (X % 8);

    /* Read from each plane */
    for (UINT32 Plane = 0; Plane < 4; Plane++) {
        EgaFb_SetReadMap(Backend, Plane);
        UINT8 Value = Backend->FramebufferBase[Offset];

        if (Value & BitMask) {
            ColorIndex |= (1 << Plane);
        }
    }

    *Color = gEgaPalette[ColorIndex & 0x0F];
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
EgaFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    EGA_BACKEND *Backend = (EGA_BACKEND *)This;
    UINT8 ColorIndex;
    INT32 x, y;

    if (!Backend->Initialized || Rect == NULL) {
        return E_POINTER;
    }

    ColorIndex = EgaFb_MapColorToIndex(Color);

    for (y = Rect->Top; y < Rect->Bottom && y < (INT32)Backend->Descriptor.Height; y++) {
        for (x = Rect->Left; x < Rect->Right && x < (INT32)Backend->Descriptor.Width; x++) {
            if (x >= 0 && y >= 0) {
                EgaFb_WritePixelPlanar(Backend, x, y, ColorIndex);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
EgaFb_BlitMonoBitmap(
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
    EGA_BACKEND *Backend = (EGA_BACKEND *)This;
    UINT8 FgIndex, BgIndex;
    UINT32 Row, Col;
    UINT32 ByteIndex, BitIndex;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    FgIndex = EgaFb_MapColorToIndex(Foreground);
    BgIndex = EgaFb_MapColorToIndex(Background);

    for (Row = 0; Row < Height; Row++) {
        for (Col = 0; Col < Width; Col++) {
            ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
            BitIndex = 7 - (Col % 8);

            UINT8 ColorIndex = (Bitmap[ByteIndex] & (1 << BitIndex)) ? FgIndex : BgIndex;
            EgaFb_WritePixelPlanar(Backend, X + Col, Y + Row, ColorIndex);
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
EgaFb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    /* Format conversion handled by engine */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
EgaFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    EGA_BACKEND *Backend = (EGA_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
EgaFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    EGA_BACKEND *Backend = (EGA_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static EGA_BACKEND gEgaBackendInstance = {
    .Base.lpVtbl        = &gEgaFbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherNone,
    .CurrentMapMask     = 0x0F,
    .CurrentReadMap     = 0,
};

IFramebufferBackend *
FbCreateEgaBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gEgaBackendInstance;
}
