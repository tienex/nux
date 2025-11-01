/*++
    Module Name:

        pc_graphics.c

    Abstract:

        Unified IBM PC-compatible graphics adapter backend.
        Handles CGA, EGA, VGA, SVGA, and XGA modes.

        The pixel format describes WHAT the data is (mono, planar, indexed, RGB).
        The FRAMEBUFFER_DESC describes HOW it's organized in memory (linear,
        planar, banked, interleaved).

        Supported modes:
        - CGA: 320x200x4, 640x200x2 (bank-interleaved)
        - EGA: 640x350x16 (planar)
        - VGA: 640x480x16 (planar), 320x200x256 (linear), text modes
        - SVGA: Extended resolutions with linear or banked framebuffers
        - XGA: High-resolution modes

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/backend_ext.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/framebuffer/dither.h>
#include <ananke/framebuffer/screen.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>
#include <ananke/intrinsics.h>

/* --------------------------------------------------------------- */
/*  VGA I/O Ports                                                   */
/* --------------------------------------------------------------- */

#define VGA_SEQ_INDEX       0x3C4     /* Sequencer Index */
#define VGA_SEQ_DATA        0x3C5     /* Sequencer Data */
#define VGA_GC_INDEX        0x3CE     /* Graphics Controller Index */
#define VGA_GC_DATA         0x3CF     /* Graphics Controller Data */
#define VGA_DAC_WRITE       0x3C8     /* DAC Write Index */
#define VGA_DAC_DATA        0x3C9     /* DAC Data */
#define VGA_INPUT_STATUS    0x3DA     /* Input Status Register */

/* Sequencer Registers */
#define VGA_SEQ_MAP_MASK    0x02      /* Map Mask Register */

/* Graphics Controller Registers */
#define VGA_GC_READ_MAP     0x04      /* Read Map Select */
#define VGA_GC_MODE         0x05      /* Mode Register */
#define VGA_GC_BIT_MASK     0x08      /* Bit Mask */

/* --------------------------------------------------------------- */
/*  PC Graphics Backend Structure                                   */
/* --------------------------------------------------------------- */

typedef struct _PC_GRAPHICS_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;

    /* Framebuffer access */
    UINT8                       *FramebufferBase;

    /* Hardware state */
    UINT8                       CurrentMapMask;     /* For planar modes */
    UINT8                       CurrentReadMap;     /* For planar modes */
    UINT32                      CurrentBank;        /* For banked modes */

    /* Bank switching function (for VESA banked modes) */
    VOID (*BankSwitchFunc)(UINT32 BankNumber);

    /* Runtime RtlCopyMemory support (if available) */
    VOID (*RtlCopyMemoryFunc)(VOID *Dest, CONST VOID *Src, SIZE_T Size);

    /* Palette (for indexed modes) */
    FB_PALETTE_ENTRY            Palette[256];
} PC_GRAPHICS_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE PcGraphics_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE PcGraphics_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE PcGraphics_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE PcGraphics_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE PcGraphics_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE PcGraphics_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE PcGraphics_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE PcGraphics_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE PcGraphics_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE PcGraphics_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE PcGraphics_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE PcGraphics_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gPcGraphicsVtbl = {
    .QueryInterface     = PcGraphics_QueryInterface,
    .AddRef             = PcGraphics_AddRef,
    .Release            = PcGraphics_Release,
    .Initialize         = PcGraphics_Initialize,
    .Clear              = PcGraphics_Clear,
    .SetPixel           = PcGraphics_SetPixel,
    .GetPixel           = PcGraphics_GetPixel,
    .FillRect           = PcGraphics_FillRect,
    .BlitMonoBitmap     = PcGraphics_BlitMonoBitmap,
    .BlitBitmap         = PcGraphics_BlitBitmap,
    .GetDescriptor      = PcGraphics_GetDescriptor,
    .SetDitherMethod    = PcGraphics_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  Mode Definitions                                                */
/* --------------------------------------------------------------- */

typedef struct _PC_GRAPHICS_MODE {
    UINT32              ModeNumber;
    UINT32              Width;
    UINT32              Height;
    FB_PIXEL_FORMAT     PixelFormat;
    FB_MEMORY_ORGANIZATION MemoryOrganization;
    UINT32              NumPlanes;
    UINT64              PhysicalBase;
    UINT32              Pitch;
} PC_GRAPHICS_MODE;

static CONST PC_GRAPHICS_MODE gPcGraphicsModes[] = {
    /* CGA modes */
    { 0x04, 320, 200, FbPixelFormatIndexed4, FbMemoryInterleaved, 0, 0xB8000, 80 },
    { 0x06, 640, 200, FbPixelFormat1Bpp, FbMemoryInterleaved, 0, 0xB8000, 80 },

    /* EGA modes */
    { 0x0D, 320, 200, FbPixelFormatIndexed16, FbMemoryPlanar, 4, 0xA0000, 40 },
    { 0x0E, 640, 200, FbPixelFormatIndexed16, FbMemoryPlanar, 4, 0xA0000, 80 },
    { 0x10, 640, 350, FbPixelFormatIndexed16, FbMemoryPlanar, 4, 0xA0000, 80 },

    /* VGA modes */
    { 0x12, 640, 480, FbPixelFormatIndexed16, FbMemoryPlanar, 4, 0xA0000, 80 },
    { 0x13, 320, 200, FbPixelFormatIndexed256, FbMemoryLinear, 0, 0xA0000, 320 },

    /* Mode-X (tweaked mode 13h) */
    { 0x100, 320, 240, FbPixelFormatIndexed256, FbMemoryLinear, 0, 0xA0000, 320 },
};

#define PC_GRAPHICS_MODE_COUNT (sizeof(gPcGraphicsModes) / sizeof(gPcGraphicsModes[0]))

/* --------------------------------------------------------------- */
/*  Standard VGA Palette                                            */
/* --------------------------------------------------------------- */

static CONST FB_PALETTE_ENTRY gVgaPalette[16] = {
    { 0x00, 0x00, 0x00, 0 },  /* 0: Black */
    { 0x00, 0x00, 0xAA, 0 },  /* 1: Blue */
    { 0x00, 0xAA, 0x00, 0 },  /* 2: Green */
    { 0x00, 0xAA, 0xAA, 0 },  /* 3: Cyan */
    { 0xAA, 0x00, 0x00, 0 },  /* 4: Red */
    { 0xAA, 0x00, 0xAA, 0 },  /* 5: Magenta */
    { 0xAA, 0x55, 0x00, 0 },  /* 6: Brown */
    { 0xAA, 0xAA, 0xAA, 0 },  /* 7: Light Gray */
    { 0x55, 0x55, 0x55, 0 },  /* 8: Dark Gray */
    { 0x55, 0x55, 0xFF, 0 },  /* 9: Light Blue */
    { 0x55, 0xFF, 0x55, 0 },  /* 10: Light Green */
    { 0x55, 0xFF, 0xFF, 0 },  /* 11: Light Cyan */
    { 0xFF, 0x55, 0x55, 0 },  /* 12: Light Red */
    { 0xFF, 0x55, 0xFF, 0 },  /* 13: Light Magenta */
    { 0xFF, 0xFF, 0x55, 0 },  /* 14: Yellow */
    { 0xFF, 0xFF, 0xFF, 0 },  /* 15: White */
};

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static INLINE VOID
PcGraphics_SetMapMask(
    PC_GRAPHICS_BACKEND *Backend,
    UINT8 Mask
    )
{
    if (Backend->Descriptor.RequiresIoAccess && Backend->CurrentMapMask != Mask) {
        ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_MAP_MASK);
        ANX_CPU_OUTB(VGA_SEQ_DATA, Mask);
        Backend->CurrentMapMask = Mask;
    }
}

static INLINE VOID
PcGraphics_SetReadMap(
    PC_GRAPHICS_BACKEND *Backend,
    UINT8 Plane
    )
{
    if (Backend->Descriptor.RequiresIoAccess && Backend->CurrentReadMap != Plane) {
        ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_READ_MAP);
        ANX_CPU_OUTB(VGA_GC_DATA, Plane);
        Backend->CurrentReadMap = Plane;
    }
}

static VOID
PcGraphics_WritePixelLinear(
    PC_GRAPHICS_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT8 ColorIndex
    )
{
    UINT32 Offset = Y * Backend->Descriptor.Pitch + X;
    Backend->FramebufferBase[Offset] = ColorIndex;
}

static VOID
PcGraphics_WritePixelInterleaved(
    PC_GRAPHICS_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT8 ColorIndex
    )
{
    /* Bank-interleaved (CGA-style): even/odd scanlines in different banks */
    UINT32 RowOffset = (Y & 1) ? Backend->Descriptor.BankOffset : 0;
    UINT32 ByteOffset = (Y / Backend->Descriptor.BankInterleave) *
                        Backend->Descriptor.Pitch + (X / 4);

    UINT8 *Addr = Backend->FramebufferBase + RowOffset + ByteOffset;
    UINT32 BitOffset = (3 - (X % 4)) * 2;  /* 2 bits per pixel */

    UINT8 Mask = ~(0x03 << BitOffset);
    *Addr = (*Addr & Mask) | ((ColorIndex & 0x03) << BitOffset);
}

static VOID
PcGraphics_WritePixelPlanar(
    PC_GRAPHICS_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT8 ColorIndex
    )
{
    /* Planar mode (EGA/VGA): each plane holds one bit per pixel */
    UINT32 Offset = Y * (Backend->Descriptor.Pitch / Backend->Descriptor.NumPlanes) + (X / 8);
    UINT8 BitMask = 0x80 >> (X % 8);
    UINT8 *Addr = Backend->FramebufferBase + Offset;

    /* Write to all planes */
    for (UINT32 Plane = 0; Plane < Backend->Descriptor.NumPlanes; Plane++) {
        UINT8 PlaneMask = (1 << Plane);
        PcGraphics_SetMapMask(Backend, PlaneMask);

        /* Read latch */
        volatile UINT8 Dummy = *Addr;
        (VOID)Dummy;

        /* Write bit */
        if (ColorIndex & PlaneMask) {
            *Addr = *Addr | BitMask;
        } else {
            *Addr = *Addr & ~BitMask;
        }
    }
}

static VOID
PcGraphics_WritePixel(
    PC_GRAPHICS_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT8 ColorIndex
    )
{
    switch (Backend->Descriptor.MemoryOrganization) {
        case FbMemoryLinear:
            PcGraphics_WritePixelLinear(Backend, X, Y, ColorIndex);
            break;

        case FbMemoryInterleaved:
            PcGraphics_WritePixelInterleaved(Backend, X, Y, ColorIndex);
            break;

        case FbMemoryPlanar:
            PcGraphics_WritePixelPlanar(Backend, X, Y, ColorIndex);
            break;

        case FbMemoryBanked:
            /* Handle bank switching for VESA modes */
            /* Would calculate bank and switch if needed */
            PcGraphics_WritePixelLinear(Backend, X, Y, ColorIndex);
            break;
    }
}

static UINT8
PcGraphics_MapColorToIndex(
    PC_GRAPHICS_BACKEND *Backend,
    FB_COLOR Color
    )
{
    switch (Backend->Descriptor.PixelFormat) {
        case FbPixelFormat1Bpp:
        case FbPixelFormat2Bpp:
        case FbPixelFormat4Bpp: {
            /* Grayscale */
            UINT8 Gray = FbRgbToGray(Color);
            UINT8 MaxValue = (1 << (Backend->Descriptor.PixelFormat == FbPixelFormat1Bpp ? 1 :
                                    Backend->Descriptor.PixelFormat == FbPixelFormat2Bpp ? 2 : 4)) - 1;
            return (Gray * MaxValue) / 255;
        }

        case FbPixelFormatIndexed4:
        case FbPixelFormatIndexed16:
        case FbPixelFormatIndexed256: {
            UINT32 PaletteSize = Backend->Descriptor.PixelFormat == FbPixelFormatIndexed4 ? 4 :
                                Backend->Descriptor.PixelFormat == FbPixelFormatIndexed16 ? 16 : 256;
            return FbFindClosestPaletteEntry(Color, Backend->Palette, PaletteSize);
        }

        case FbPixelFormatPlanar2:
        case FbPixelFormatPlanar4:
        case FbPixelFormatPlanar6:
        case FbPixelFormatPlanar8: {
            UINT32 NumColors = 1 << Backend->Descriptor.NumPlanes;
            return FbFindClosestPaletteEntry(Color, gVgaPalette,
                                           NumColors < 16 ? NumColors : 16);
        }

        default:
            return 0;
    }
}

/* --------------------------------------------------------------- */
/*  ROP2/ROP3 Operations                                            */
/* --------------------------------------------------------------- */

static INLINE UINT8
PcGraphics_ApplyRop2(
    UINT8 Source,
    UINT8 Dest,
    FB_ROP2 Rop
    )
{
    switch (Rop) {
        case FbRop2Black:          return 0x00;
        case FbRop2NotMergePen:    return ~(Source | Dest);
        case FbRop2MaskNotPen:     return Dest & ~Source;
        case FbRop2NotCopyPen:     return ~Source;
        case FbRop2MaskPenNot:     return Source & ~Dest;
        case FbRop2Not:            return ~Dest;
        case FbRop2XorPen:         return Source ^ Dest;
        case FbRop2NotMaskPen:     return ~(Source & Dest);
        case FbRop2MaskPen:        return Source & Dest;
        case FbRop2NotXorPen:      return ~(Source ^ Dest);
        case FbRop2Nop:            return Dest;
        case FbRop2MergeNotPen:    return ~Source | Dest;
        case FbRop2CopyPen:        return Source;
        case FbRop2MergePenNot:    return Source | ~Dest;
        case FbRop2MergePen:       return Source | Dest;
        case FbRop2White:          return 0xFF;
        default:                   return Source;
    }
}

static INLINE UINT8
PcGraphics_ApplyRop3(
    UINT8 Source,
    UINT8 Dest,
    UINT8 Pattern,
    FB_ROP3 Rop
    )
{
    /* ROP3 truth table: for each bit position, apply the 8-bit ROP code
     * based on S (bit 0), D (bit 1), P (bit 2) */
    UINT8 Result = 0;
    for (UINT32 Bit = 0; Bit < 8; Bit++) {
        UINT8 S = (Source >> Bit) & 1;
        UINT8 D = (Dest >> Bit) & 1;
        UINT8 P = (Pattern >> Bit) & 1;
        UINT8 Index = (P << 2) | (D << 1) | S;
        UINT8 OutBit = (Rop >> Index) & 1;
        Result |= (OutBit << Bit);
    }
    return Result;
}

/* --------------------------------------------------------------- */
/*  Optimized Memory Operations                                     */
/* --------------------------------------------------------------- */

static INLINE VOID
PcGraphics_FastCopy(
    PC_GRAPHICS_BACKEND *Backend,
    VOID *Dest,
    CONST VOID *Src,
    SIZE_T Size
    )
{
    if (Backend->RtlCopyMemoryFunc != NULL) {
        Backend->RtlCopyMemoryFunc(Dest, Src, Size);
    } else {
        ANX_MEMCPY(Dest, Src, Size);
    }
}

static VOID
PcGraphics_FillLinear(
    PC_GRAPHICS_BACKEND *Backend,
    UINT32 X,
    UINT32 Y,
    UINT32 Width,
    UINT32 Height,
    UINT8 ColorIndex
    )
{
    /* Fast fill for linear indexed modes using memset */
    for (UINT32 Row = 0; Row < Height; Row++) {
        UINT32 Offset = (Y + Row) * Backend->Descriptor.Pitch + X;
        ANX_MEMSET(&Backend->FramebufferBase[Offset], ColorIndex, Width);
    }
}

static HRESULT
PcGraphics_BlitLinearSameFormat(
    PC_GRAPHICS_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    UINT32 SourcePitch
    )
{
    /* Fast blit for matching linear formats using memcpy */
    for (UINT32 Row = 0; Row < Height; Row++) {
        if ((Y + (INT32)Row) < 0 || (Y + (INT32)Row) >= (INT32)Backend->Descriptor.Height) {
            continue;
        }
        if (X < 0 || X >= (INT32)Backend->Descriptor.Width) {
            continue;
        }

        UINT32 DestOffset = (Y + Row) * Backend->Descriptor.Pitch + X;
        UINT32 SrcOffset = Row * SourcePitch;
        UINT32 CopyWidth = Width;

        /* Clip to screen bounds */
        if (X + (INT32)CopyWidth > (INT32)Backend->Descriptor.Width) {
            CopyWidth = Backend->Descriptor.Width - X;
        }

        PcGraphics_FastCopy(Backend,
                           &Backend->FramebufferBase[DestOffset],
                           &Bitmap[SrcOffset],
                           CopyWidth);
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
PcGraphics_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    PC_GRAPHICS_BACKEND *Backend = (PC_GRAPHICS_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        PcGraphics_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
PcGraphics_AddRef(
    IFramebufferBackend *This
    )
{
    PC_GRAPHICS_BACKEND *Backend = (PC_GRAPHICS_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
PcGraphics_Release(
    IFramebufferBackend *This
    )
{
    PC_GRAPHICS_BACKEND *Backend = (PC_GRAPHICS_BACKEND *)This;
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
PcGraphics_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    PC_GRAPHICS_BACKEND *Backend = (PC_GRAPHICS_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    Backend->Descriptor = *Descriptor;
    Backend->FramebufferBase = (UINT8 *)(UINTN)Descriptor->PhysicalBase;
    Backend->Initialized = TRUE;
    Backend->CurrentMapMask = 0x0F;
    Backend->CurrentReadMap = 0;
    Backend->CurrentBank = 0;

    /* Initialize palette */
    for (UINT32 i = 0; i < 16; i++) {
        Backend->Palette[i] = gVgaPalette[i];
    }

    /* Load default 256-color palette for higher color modes */
    for (UINT32 i = 16; i < 256; i++) {
        /* Simple RGB cube */
        UINT8 r = ((i >> 5) & 0x07) * 36;
        UINT8 g = ((i >> 2) & 0x07) * 36;
        UINT8 b = (i & 0x03) * 85;
        Backend->Palette[i].Red = r;
        Backend->Palette[i].Green = g;
        Backend->Palette[i].Blue = b;
        Backend->Palette[i].Reserved = 0;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PcGraphics_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    PC_GRAPHICS_BACKEND *Backend = (PC_GRAPHICS_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    ColorIndex = PcGraphics_MapColorToIndex(Backend, Color);

    /* Use optimized fill for linear indexed modes */
    if (Backend->Descriptor.MemoryOrganization == FbMemoryLinear &&
        (Backend->Descriptor.PixelFormat == FbPixelFormatIndexed4 ||
         Backend->Descriptor.PixelFormat == FbPixelFormatIndexed16 ||
         Backend->Descriptor.PixelFormat == FbPixelFormatIndexed256)) {
        PcGraphics_FillLinear(Backend, 0, 0,
                             Backend->Descriptor.Width,
                             Backend->Descriptor.Height,
                             ColorIndex);
    } else {
        /* Fallback to pixel-by-pixel for planar/banked modes */
        for (UINT32 y = 0; y < Backend->Descriptor.Height; y++) {
            for (UINT32 x = 0; x < Backend->Descriptor.Width; x++) {
                PcGraphics_WritePixel(Backend, x, y, ColorIndex);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PcGraphics_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    PC_GRAPHICS_BACKEND *Backend = (PC_GRAPHICS_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    ColorIndex = PcGraphics_MapColorToIndex(Backend, Color);
    PcGraphics_WritePixel(Backend, X, Y, ColorIndex);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PcGraphics_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    /* Reading pixels requires handling each memory organization differently */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
PcGraphics_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    PC_GRAPHICS_BACKEND *Backend = (PC_GRAPHICS_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized || Rect == NULL) {
        return E_POINTER;
    }

    ColorIndex = PcGraphics_MapColorToIndex(Backend, Color);

    /* Clip to screen bounds */
    INT32 Left = Rect->Left < 0 ? 0 : Rect->Left;
    INT32 Top = Rect->Top < 0 ? 0 : Rect->Top;
    INT32 Right = Rect->Right > (INT32)Backend->Descriptor.Width ?
                  (INT32)Backend->Descriptor.Width : Rect->Right;
    INT32 Bottom = Rect->Bottom > (INT32)Backend->Descriptor.Height ?
                   (INT32)Backend->Descriptor.Height : Rect->Bottom;

    if (Left >= Right || Top >= Bottom) {
        return S_OK;  /* Nothing to draw */
    }

    UINT32 Width = Right - Left;
    UINT32 Height = Bottom - Top;

    /* Use optimized fill for linear indexed modes */
    if (Backend->Descriptor.MemoryOrganization == FbMemoryLinear &&
        (Backend->Descriptor.PixelFormat == FbPixelFormatIndexed4 ||
         Backend->Descriptor.PixelFormat == FbPixelFormatIndexed16 ||
         Backend->Descriptor.PixelFormat == FbPixelFormatIndexed256)) {
        PcGraphics_FillLinear(Backend, Left, Top, Width, Height, ColorIndex);
    } else {
        /* Fallback to pixel-by-pixel for planar/banked modes */
        for (INT32 y = Top; y < Bottom; y++) {
            for (INT32 x = Left; x < Right; x++) {
                PcGraphics_WritePixel(Backend, x, y, ColorIndex);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PcGraphics_BlitMonoBitmap(
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
    PC_GRAPHICS_BACKEND *Backend = (PC_GRAPHICS_BACKEND *)This;
    UINT8 FgIndex, BgIndex;
    UINT32 Row, Col;
    UINT32 ByteIndex, BitIndex;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    FgIndex = PcGraphics_MapColorToIndex(Backend, Foreground);
    BgIndex = PcGraphics_MapColorToIndex(Backend, Background);

    for (Row = 0; Row < Height; Row++) {
        for (Col = 0; Col < Width; Col++) {
            ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
            BitIndex = 7 - (Col % 8);

            UINT8 ColorIndex = (Bitmap[ByteIndex] & (1 << BitIndex)) ? FgIndex : BgIndex;
            PcGraphics_WritePixel(Backend, X + Col, Y + Row, ColorIndex);
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PcGraphics_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    PC_GRAPHICS_BACKEND *Backend = (PC_GRAPHICS_BACKEND *)This;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    /* Fast path: matching format and linear memory organization */
    if (SourceFormat == Backend->Descriptor.PixelFormat &&
        Backend->Descriptor.MemoryOrganization == FbMemoryLinear &&
        (SourceFormat == FbPixelFormatIndexed4 ||
         SourceFormat == FbPixelFormatIndexed16 ||
         SourceFormat == FbPixelFormatIndexed256 ||
         SourceFormat == FbPixelFormatRgb8 ||
         SourceFormat == FbPixelFormatRgb16 ||
         SourceFormat == FbPixelFormatRgb24 ||
         SourceFormat == FbPixelFormatRgb32)) {

        UINT32 BytesPerPixel = 1;
        if (SourceFormat == FbPixelFormatRgb16) BytesPerPixel = 2;
        else if (SourceFormat == FbPixelFormatRgb24) BytesPerPixel = 3;
        else if (SourceFormat == FbPixelFormatRgb32) BytesPerPixel = 4;

        return PcGraphics_BlitLinearSameFormat(Backend, X, Y, Width, Height,
                                              Bitmap, Width * BytesPerPixel);
    }

    /* Format conversion handled by engine */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
PcGraphics_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    PC_GRAPHICS_BACKEND *Backend = (PC_GRAPHICS_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
PcGraphics_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    PC_GRAPHICS_BACKEND *Backend = (PC_GRAPHICS_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static PC_GRAPHICS_BACKEND gPcGraphicsBackendInstance = {
    .Base.lpVtbl        = &gPcGraphicsVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherNone,
    .BankSwitchFunc     = NULL,
    .RtlCopyMemoryFunc  = NULL,
};

IFramebufferBackend *
FbCreatePcGraphicsBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gPcGraphicsBackendInstance;
}

/*
 * Set bank switching function for VESA banked modes.
 */
VOID
FbPcGraphicsSetBankFunction(
    IN IFramebufferBackend *Backend,
    IN VOID (*BankSwitchFunc)(UINT32)
    )
{
    PC_GRAPHICS_BACKEND *PcBackend = (PC_GRAPHICS_BACKEND *)Backend;
    PcBackend->BankSwitchFunc = BankSwitchFunc;
}

/*
 * Set RtlCopyMemory function for optimized memory operations.
 */
VOID
FbPcGraphicsSetRtlCopyMemory(
    IN IFramebufferBackend *Backend,
    IN VOID (*RtlCopyMemoryFunc)(VOID *, CONST VOID *, SIZE_T)
    )
{
    PC_GRAPHICS_BACKEND *PcBackend = (PC_GRAPHICS_BACKEND *)Backend;
    PcBackend->RtlCopyMemoryFunc = RtlCopyMemoryFunc;
}

/*
 * Query the number of available video modes.
 */
UINT32
FbPcGraphicsGetModeCount(
    VOID
    )
{
    return PC_GRAPHICS_MODE_COUNT;
}

/*
 * Query information about a specific video mode.
 */
HRESULT
FbPcGraphicsQueryMode(
    IN UINT32 ModeIndex,
    OUT FRAMEBUFFER_DESC *ModeDesc
    )
{
    if (ModeIndex >= PC_GRAPHICS_MODE_COUNT || ModeDesc == NULL) {
        return E_INVALIDARG;
    }

    CONST PC_GRAPHICS_MODE *Mode = &gPcGraphicsModes[ModeIndex];

    ANX_MEMSET(ModeDesc, 0, sizeof(FRAMEBUFFER_DESC));
    ModeDesc->Width = Mode->Width;
    ModeDesc->Height = Mode->Height;
    ModeDesc->Pitch = Mode->Pitch;
    ModeDesc->PixelFormat = Mode->PixelFormat;
    ModeDesc->MemoryOrganization = Mode->MemoryOrganization;
    ModeDesc->PhysicalBase = Mode->PhysicalBase;
    ModeDesc->NumPlanes = Mode->NumPlanes;
    ModeDesc->IoPortBase = 0x3C0;
    ModeDesc->RequiresIoAccess = (Mode->MemoryOrganization == FbMemoryPlanar ||
                                  Mode->MemoryOrganization == FbMemoryInterleaved);

    /* Set appropriate bit masks and parameters based on format */
    switch (Mode->PixelFormat) {
        case FbPixelFormatIndexed4:
        case FbPixelFormatIndexed16:
        case FbPixelFormatIndexed256:
            ModeDesc->BankInterleave = (Mode->MemoryOrganization == FbMemoryInterleaved) ? 2 : 1;
            ModeDesc->BankOffset = (Mode->MemoryOrganization == FbMemoryInterleaved) ? 0x2000 : 0;
            break;

        default:
            break;
    }

    return S_OK;
}

/*
 * Set a specific video mode.
 */
HRESULT
FbPcGraphicsSetMode(
    IN IFramebufferBackend *Backend,
    IN UINT32 ModeNumber
    )
{
    PC_GRAPHICS_BACKEND *PcBackend = (PC_GRAPHICS_BACKEND *)Backend;

    /* Find the mode */
    for (UINT32 i = 0; i < PC_GRAPHICS_MODE_COUNT; i++) {
        if (gPcGraphicsModes[i].ModeNumber == ModeNumber) {
            FRAMEBUFFER_DESC Desc;
            FbPcGraphicsQueryMode(i, &Desc);
            return PcGraphics_Initialize(Backend, &Desc);
        }
    }

    return E_INVALIDARG;
}
