/*++
    Module Name:

        cga.c

    Abstract:

        IBM CGA (Color Graphics Adapter) backend implementation.
        Supports:
        - Mode 4: 320x200, 4 colors (2bpp indexed)
        - Mode 5: 320x200, 4 colors (grayscale)
        - Mode 6: 640x200, 2 colors (1bpp monochrome)

        CGA memory is located at 0xB8000 for text modes and
        0xB8000 (alt 0xBC000) for graphics modes with bank interleaving.

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/backend_ext.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/framebuffer/dither.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  CGA Hardware Definitions                                        */
/* --------------------------------------------------------------- */

#define CGA_BASE_ADDR       0xB8000
#define CGA_ALT_ADDR        0xBC000
#define CGA_MEMORY_SIZE     0x4000    /* 16KB */

/* CGA I/O Ports */
#define CGA_MODE_CTRL       0x3D8     /* Mode Control Register */
#define CGA_COLOR_SELECT    0x3D9     /* Color Select Register */
#define CGA_STATUS          0x3DA     /* Status Register */

/* CGA Modes */
#define CGA_MODE_320x200x4  0x04      /* 320x200, 4 colors */
#define CGA_MODE_640x200x2  0x06      /* 640x200, monochrome */

/* --------------------------------------------------------------- */
/*  CGA Backend Structure                                           */
/* --------------------------------------------------------------- */

typedef struct _CGA_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    UINT8                       *FramebufferBase;
    FB_DITHER_METHOD            DitherMethod;
    UINT32                      CurrentMode;
    BOOLEAN                     Initialized;

    /* CGA-specific state */
    UINT8                       ColorSelect;     /* Color palette selection */
    BOOLEAN                     CompositeModeEnabled;
} CGA_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE CgaFb_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE CgaFb_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE CgaFb_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE CgaFb_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE CgaFb_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE CgaFb_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE CgaFb_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE CgaFb_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE CgaFb_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE CgaFb_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE CgaFb_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE CgaFb_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gCgaFbVtbl = {
    .QueryInterface     = CgaFb_QueryInterface,
    .AddRef             = CgaFb_AddRef,
    .Release            = CgaFb_Release,
    .Initialize         = CgaFb_Initialize,
    .Clear              = CgaFb_Clear,
    .SetPixel           = CgaFb_SetPixel,
    .GetPixel           = CgaFb_GetPixel,
    .FillRect           = CgaFb_FillRect,
    .BlitMonoBitmap     = CgaFb_BlitMonoBitmap,
    .BlitBitmap         = CgaFb_BlitBitmap,
    .GetDescriptor      = CgaFb_GetDescriptor,
    .SetDitherMethod    = CgaFb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  CGA Default Palettes                                            */
/* --------------------------------------------------------------- */

/* CGA Palette 0 (Green/Red/Yellow) */
static CONST FB_COLOR gCgaPalette0[4] = {
    { 0x00, 0x00, 0x00, 0xFF },  /* Black */
    { 0x00, 0xAA, 0x00, 0xFF },  /* Green */
    { 0xAA, 0x00, 0x00, 0xFF },  /* Red */
    { 0xAA, 0xAA, 0x00, 0xFF },  /* Brown/Yellow */
};

/* CGA Palette 1 (Cyan/Magenta/White) */
static CONST FB_COLOR gCgaPalette1[4] = {
    { 0x00, 0x00, 0x00, 0xFF },  /* Black */
    { 0x00, 0xAA, 0xAA, 0xFF },  /* Cyan */
    { 0xAA, 0x00, 0xAA, 0xFF },  /* Magenta */
    { 0xAA, 0xAA, 0xAA, 0xFF },  /* White */
};

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static INLINE VOID
CgaFb_WritePixel4Color(
    CGA_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT8 ColorIndex
    )
{
    /* CGA 320x200x4 uses bank-interleaved memory:
     * Even scanlines: offset 0
     * Odd scanlines:  offset 0x2000
     */
    UINT32 RowOffset = (Y & 1) ? 0x2000 : 0;
    UINT32 ByteOffset = (Y / 2) * 80 + (X / 4);
    UINT32 BitOffset = (3 - (X % 4)) * 2;

    UINT8 *Addr = Backend->FramebufferBase + RowOffset + ByteOffset;
    UINT8 Mask = ~(0x03 << BitOffset);
    UINT8 Value = (*Addr & Mask) | ((ColorIndex & 0x03) << BitOffset);

    *Addr = Value;
}

static INLINE VOID
CgaFb_WritePixel2Color(
    CGA_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT8 ColorIndex
    )
{
    /* CGA 640x200x2 uses bank-interleaved memory:
     * Even scanlines: offset 0
     * Odd scanlines:  offset 0x2000
     */
    UINT32 RowOffset = (Y & 1) ? 0x2000 : 0;
    UINT32 ByteOffset = (Y / 2) * 80 + (X / 8);
    UINT32 BitOffset = 7 - (X % 8);

    UINT8 *Addr = Backend->FramebufferBase + RowOffset + ByteOffset;

    if (ColorIndex) {
        *Addr |= (1 << BitOffset);
    } else {
        *Addr &= ~(1 << BitOffset);
    }
}

static UINT8
CgaFb_MapColorToIndex(
    CGA_BACKEND *Backend,
    FB_COLOR Color
    )
{
    CONST FB_COLOR *Palette;
    UINT32 PaletteSize;

    if (Backend->CurrentMode == CGA_MODE_320x200x4) {
        /* Use the selected palette */
        Palette = (Backend->ColorSelect & 0x10) ? gCgaPalette1 : gCgaPalette0;
        PaletteSize = 4;
    } else {
        /* 640x200x2: monochrome */
        UINT8 Gray = FbRgbToGray(Color);
        return (Gray > 128) ? 1 : 0;
    }

    return FbFindClosestPaletteEntry(Color, (CONST FB_PALETTE_ENTRY *)Palette, PaletteSize);
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
CgaFb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    CGA_BACKEND *Backend = (CGA_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        CgaFb_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
CgaFb_AddRef(
    IFramebufferBackend *This
    )
{
    CGA_BACKEND *Backend = (CGA_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
CgaFb_Release(
    IFramebufferBackend *This
    )
{
    CGA_BACKEND *Backend = (CGA_BACKEND *)This;
    UINT32 RefCount = ANX_REF_DEC(&Backend->RefCount);

    if (RefCount == 0) {
        /* Cleanup would go here */
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  IFramebufferBackend Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
CgaFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    CGA_BACKEND *Backend = (CGA_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    /* Copy descriptor */
    Backend->Descriptor = *Descriptor;
    Backend->FramebufferBase = (UINT8 *)(UINTN)Descriptor->PhysicalBase;
    Backend->Initialized = TRUE;

    /* Determine mode from dimensions */
    if (Descriptor->Width == 320 && Descriptor->Height == 200) {
        Backend->CurrentMode = CGA_MODE_320x200x4;
    } else if (Descriptor->Width == 640 && Descriptor->Height == 200) {
        Backend->CurrentMode = CGA_MODE_640x200x2;
    } else {
        return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
CgaFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    CGA_BACKEND *Backend = (CGA_BACKEND *)This;
    UINT8 ColorIndex;
    UINT32 x, y;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    ColorIndex = CgaFb_MapColorToIndex(Backend, Color);

    for (y = 0; y < Backend->Descriptor.Height; y++) {
        for (x = 0; x < Backend->Descriptor.Width; x++) {
            if (Backend->CurrentMode == CGA_MODE_320x200x4) {
                CgaFb_WritePixel4Color(Backend, x, y, ColorIndex);
            } else {
                CgaFb_WritePixel2Color(Backend, x, y, ColorIndex);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
CgaFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    CGA_BACKEND *Backend = (CGA_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    ColorIndex = CgaFb_MapColorToIndex(Backend, Color);

    if (Backend->CurrentMode == CGA_MODE_320x200x4) {
        CgaFb_WritePixel4Color(Backend, X, Y, ColorIndex);
    } else {
        CgaFb_WritePixel2Color(Backend, X, Y, ColorIndex);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
CgaFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    /* Not implemented for CGA - reading pixels is uncommon */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
CgaFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    CGA_BACKEND *Backend = (CGA_BACKEND *)This;
    UINT8 ColorIndex;
    INT32 x, y;

    if (!Backend->Initialized || Rect == NULL) {
        return E_POINTER;
    }

    ColorIndex = CgaFb_MapColorToIndex(Backend, Color);

    for (y = Rect->Top; y < Rect->Bottom && y < (INT32)Backend->Descriptor.Height; y++) {
        for (x = Rect->Left; x < Rect->Right && x < (INT32)Backend->Descriptor.Width; x++) {
            if (x >= 0 && y >= 0) {
                if (Backend->CurrentMode == CGA_MODE_320x200x4) {
                    CgaFb_WritePixel4Color(Backend, x, y, ColorIndex);
                } else {
                    CgaFb_WritePixel2Color(Backend, x, y, ColorIndex);
                }
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
CgaFb_BlitMonoBitmap(
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
    CGA_BACKEND *Backend = (CGA_BACKEND *)This;
    UINT8 FgIndex, BgIndex;
    UINT32 Row, Col;
    UINT32 ByteIndex, BitIndex;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    FgIndex = CgaFb_MapColorToIndex(Backend, Foreground);
    BgIndex = CgaFb_MapColorToIndex(Backend, Background);

    for (Row = 0; Row < Height; Row++) {
        for (Col = 0; Col < Width; Col++) {
            ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
            BitIndex = 7 - (Col % 8);

            UINT8 ColorIndex = (Bitmap[ByteIndex] & (1 << BitIndex)) ? FgIndex : BgIndex;

            if (Backend->CurrentMode == CGA_MODE_320x200x4) {
                CgaFb_WritePixel4Color(Backend, X + Col, Y + Row, ColorIndex);
            } else {
                CgaFb_WritePixel2Color(Backend, X + Col, Y + Row, ColorIndex);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
CgaFb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    /* Not implemented - format conversion handled by engine */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
CgaFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    CGA_BACKEND *Backend = (CGA_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
CgaFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    CGA_BACKEND *Backend = (CGA_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static CGA_BACKEND gCgaBackendInstance = {
    .Base.lpVtbl        = &gCgaFbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherNone,
    .ColorSelect        = 0,
    .CompositeModeEnabled = FALSE,
};

IFramebufferBackend *
FbCreateCgaBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gCgaBackendInstance;
}
