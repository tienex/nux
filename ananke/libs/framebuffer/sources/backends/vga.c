/*++
    Module Name:

        vga.c

    Abstract:

        VGA Mode 13h backend implementation.
        320x200x256 linear framebuffer mode.

        This is the famous "unchained" VGA mode used in many DOS games.
        Memory is linear at 0xA0000 with 320x200 resolution and 256 colors.

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/backend_ext.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  VGA Hardware Definitions                                        */
/* --------------------------------------------------------------- */

#define VGA_BASE_ADDR       0xA0000
#define VGA_MODE13_SIZE     (320 * 200)

/* VGA I/O Ports */
#define VGA_DAC_WRITE       0x3C8     /* DAC Write Index */
#define VGA_DAC_DATA        0x3C9     /* DAC Data */
#define VGA_INPUT_STATUS    0x3DA     /* Input Status Register */

/* --------------------------------------------------------------- */
/*  VGA Backend Structure                                           */
/* --------------------------------------------------------------- */

typedef struct _VGA_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    UINT8                       *FramebufferBase;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;

    /* VGA palette (256 entries, RGB 6-bit) */
    FB_PALETTE_ENTRY            Palette[256];
} VGA_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE VgaFb_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE VgaFb_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE VgaFb_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE VgaFb_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE VgaFb_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE VgaFb_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE VgaFb_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE VgaFb_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE VgaFb_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE VgaFb_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE VgaFb_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE VgaFb_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gVgaFbVtbl = {
    .QueryInterface     = VgaFb_QueryInterface,
    .AddRef             = VgaFb_AddRef,
    .Release            = VgaFb_Release,
    .Initialize         = VgaFb_Initialize,
    .Clear              = VgaFb_Clear,
    .SetPixel           = VgaFb_SetPixel,
    .GetPixel           = VgaFb_GetPixel,
    .FillRect           = VgaFb_FillRect,
    .BlitMonoBitmap     = VgaFb_BlitMonoBitmap,
    .BlitBitmap         = VgaFb_BlitBitmap,
    .GetDescriptor      = VgaFb_GetDescriptor,
    .SetDitherMethod    = VgaFb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static INLINE VOID
VgaFb_WritePixel(
    VGA_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT8 ColorIndex
    )
{
    UINT32 Offset = Y * 320 + X;
    Backend->FramebufferBase[Offset] = ColorIndex;
}

static INLINE UINT8
VgaFb_ReadPixel(
    VGA_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    UINT32 Offset = Y * 320 + X;
    return Backend->FramebufferBase[Offset];
}

static UINT8
VgaFb_MapColorToIndex(
    VGA_BACKEND *Backend,
    FB_COLOR Color
    )
{
    return FbFindClosestPaletteEntry(Color, Backend->Palette, 256);
}

static VOID
VgaFb_SetPaletteEntry(
    VGA_BACKEND *Backend,
    UINT8 Index,
    FB_PALETTE_ENTRY Entry
    )
{
    Backend->Palette[Index] = Entry;

    /* Note: In a real implementation, would do I/O port writes to set DAC:
     * outb(VGA_DAC_WRITE, Index);
     * outb(VGA_DAC_DATA, Entry.Red >> 2);    // 6-bit DAC
     * outb(VGA_DAC_DATA, Entry.Green >> 2);
     * outb(VGA_DAC_DATA, Entry.Blue >> 2);
     */
}

static VOID
VgaFb_LoadDefaultPalette(
    VGA_BACKEND *Backend
    )
{
    /* Load a default 256-color palette (6-8-5 level RGB cube + grayscale) */

    /* 216 color RGB cube (6x6x6) */
    UINT32 Index = 0;
    for (UINT32 r = 0; r < 6; r++) {
        for (UINT32 g = 0; g < 6; g++) {
            for (UINT32 b = 0; b < 6; b++) {
                FB_PALETTE_ENTRY Entry;
                Entry.Red = (r * 255) / 5;
                Entry.Green = (g * 255) / 5;
                Entry.Blue = (b * 255) / 5;
                Entry.Reserved = 0;
                VgaFb_SetPaletteEntry(Backend, Index++, Entry);
            }
        }
    }

    /* 40 grayscale entries */
    for (UINT32 i = 0; i < 40; i++) {
        FB_PALETTE_ENTRY Entry;
        UINT8 Gray = (i * 255) / 39;
        Entry.Red = Gray;
        Entry.Green = Gray;
        Entry.Blue = Gray;
        Entry.Reserved = 0;
        VgaFb_SetPaletteEntry(Backend, Index++, Entry);
    }
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
VgaFb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    VGA_BACKEND *Backend = (VGA_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        VgaFb_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
VgaFb_AddRef(
    IFramebufferBackend *This
    )
{
    VGA_BACKEND *Backend = (VGA_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
VgaFb_Release(
    IFramebufferBackend *This
    )
{
    VGA_BACKEND *Backend = (VGA_BACKEND *)This;
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
VgaFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    VGA_BACKEND *Backend = (VGA_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    Backend->Descriptor = *Descriptor;
    Backend->FramebufferBase = (UINT8 *)(UINTN)Descriptor->PhysicalBase;
    Backend->Initialized = TRUE;

    /* Load default palette */
    VgaFb_LoadDefaultPalette(Backend);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VgaFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    VGA_BACKEND *Backend = (VGA_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    ColorIndex = VgaFb_MapColorToIndex(Backend, Color);

    /* Fast memset */
    for (UINT32 i = 0; i < VGA_MODE13_SIZE; i++) {
        Backend->FramebufferBase[i] = ColorIndex;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VgaFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    VGA_BACKEND *Backend = (VGA_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    if (X < 0 || X >= 320 || Y < 0 || Y >= 200) {
        return E_INVALIDARG;
    }

    ColorIndex = VgaFb_MapColorToIndex(Backend, Color);
    VgaFb_WritePixel(Backend, X, Y, ColorIndex);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VgaFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    VGA_BACKEND *Backend = (VGA_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= 320 || Y < 0 || Y >= 200) {
        return E_INVALIDARG;
    }

    ColorIndex = VgaFb_ReadPixel(Backend, X, Y);

    Color->Red = Backend->Palette[ColorIndex].Red;
    Color->Green = Backend->Palette[ColorIndex].Green;
    Color->Blue = Backend->Palette[ColorIndex].Blue;
    Color->Alpha = 255;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VgaFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    VGA_BACKEND *Backend = (VGA_BACKEND *)This;
    UINT8 ColorIndex;
    INT32 x, y;

    if (!Backend->Initialized || Rect == NULL) {
        return E_POINTER;
    }

    ColorIndex = VgaFb_MapColorToIndex(Backend, Color);

    for (y = Rect->Top; y < Rect->Bottom && y < 200; y++) {
        for (x = Rect->Left; x < Rect->Right && x < 320; x++) {
            if (x >= 0 && y >= 0) {
                VgaFb_WritePixel(Backend, x, y, ColorIndex);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VgaFb_BlitMonoBitmap(
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
    VGA_BACKEND *Backend = (VGA_BACKEND *)This;
    UINT8 FgIndex, BgIndex;
    UINT32 Row, Col;
    UINT32 ByteIndex, BitIndex;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    FgIndex = VgaFb_MapColorToIndex(Backend, Foreground);
    BgIndex = VgaFb_MapColorToIndex(Backend, Background);

    for (Row = 0; Row < Height; Row++) {
        for (Col = 0; Col < Width; Col++) {
            ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
            BitIndex = 7 - (Col % 8);

            UINT8 ColorIndex = (Bitmap[ByteIndex] & (1 << BitIndex)) ? FgIndex : BgIndex;

            INT32 DestX = X + Col;
            INT32 DestY = Y + Row;

            if (DestX >= 0 && DestX < 320 && DestY >= 0 && DestY < 200) {
                VgaFb_WritePixel(Backend, DestX, DestY, ColorIndex);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VgaFb_BlitBitmap(
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
VgaFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    VGA_BACKEND *Backend = (VGA_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
VgaFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    VGA_BACKEND *Backend = (VGA_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static VGA_BACKEND gVgaBackendInstance = {
    .Base.lpVtbl        = &gVgaFbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherNone,
};

IFramebufferBackend *
FbCreateVgaBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gVgaBackendInstance;
}
