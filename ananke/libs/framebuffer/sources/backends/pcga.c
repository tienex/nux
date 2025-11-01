/*++
    Module Name:

        pcga.c (PC Graphics Adapter)

    Abstract:

        IBM PC-compatible graphics adapter backend.
        Handles all CGA, EGA, VGA, SVGA, and XGA modes including text modes.

        The pixel format describes WHAT the data is (mono, planar, indexed, RGB, text).
        The FRAMEBUFFER_DESC describes HOW it's organized in memory (linear,
        planar, banked, interleaved) and other characteristics.

    TODO: Complete implementation with:
        1. ALL PC graphics modes (text, CGA, EGA, VGA, SVGA, XGA, Mode-X)
        2. VGA latching for fast planar writes
        3. Really fast blitting with direct memory access
        4. Text mode special case rendering
        5. Font loading (VGA ROM and custom fonts)
        6. Palette changes (VGA DAC programming)
        7. Hardware cursor support
        8. Border color control
        9. IsAddressable flag usage for direct pointer access

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
/*  VGA I/O Ports and Registers                                     */
/* --------------------------------------------------------------- */

/* VGA I/O Port Addresses */
#define VGA_SEQ_INDEX       0x3C4     /* Sequencer Index */
#define VGA_SEQ_DATA        0x3C5     /* Sequencer Data */
#define VGA_GC_INDEX        0x3CE     /* Graphics Controller Index */
#define VGA_GC_DATA         0x3CF     /* Graphics Controller Data */
#define VGA_CRTC_INDEX      0x3D4     /* CRTC Index (color modes) */
#define VGA_CRTC_DATA       0x3D5     /* CRTC Data (color modes) */
#define VGA_ATTR_INDEX      0x3C0     /* Attribute Controller Index */
#define VGA_ATTR_DATA_W     0x3C0     /* Attribute Controller Data Write */
#define VGA_ATTR_DATA_R     0x3C1     /* Attribute Controller Data Read */
#define VGA_MISC_WRITE      0x3C2     /* Miscellaneous Output Write */
#define VGA_MISC_READ       0x3CC     /* Miscellaneous Output Read */
#define VGA_DAC_READ_INDEX  0x3C7     /* DAC Read Index */
#define VGA_DAC_WRITE_INDEX 0x3C8     /* DAC Write Index */
#define VGA_DAC_DATA        0x3C9     /* DAC Data */
#define VGA_INPUT_STATUS_0  0x3C2     /* Input Status Register 0 */
#define VGA_INPUT_STATUS_1  0x3DA     /* Input Status Register 1 */

/* Sequencer Register Indices */
#define VGA_SEQ_RESET           0x00  /* Reset */
#define VGA_SEQ_CLOCKING_MODE   0x01  /* Clocking Mode */
#define VGA_SEQ_MAP_MASK        0x02  /* Map Mask (Plane Write Enable) */
#define VGA_SEQ_CHAR_MAP        0x03  /* Character Map Select */
#define VGA_SEQ_MEMORY_MODE     0x04  /* Memory Mode */

/* Graphics Controller Register Indices */
#define VGA_GC_SET_RESET        0x00  /* Set/Reset */
#define VGA_GC_ENABLE_SET_RESET 0x01  /* Enable Set/Reset */
#define VGA_GC_COLOR_COMPARE    0x02  /* Color Compare */
#define VGA_GC_DATA_ROTATE      0x03  /* Data Rotate */
#define VGA_GC_READ_MAP_SELECT  0x04  /* Read Map Select */
#define VGA_GC_GRAPHICS_MODE    0x05  /* Graphics Mode */
#define VGA_GC_MISCELLANEOUS    0x06  /* Miscellaneous */
#define VGA_GC_COLOR_DONT_CARE  0x07  /* Color Don't Care */
#define VGA_GC_BIT_MASK         0x08  /* Bit Mask */

/* VGA Write Modes (bits 0-1 of Graphics Mode register) */
#define VGA_WRITE_MODE_0        0x00  /* Normal write */
#define VGA_WRITE_MODE_1        0x01  /* Latch copy (fastest) */
#define VGA_WRITE_MODE_2        0x02  /* Fill with color */
#define VGA_WRITE_MODE_3        0x03  /* Set/reset with bitmask */

/* VGA Read Mode (bit 3 of Graphics Mode register) */
#define VGA_READ_MODE_0         0x00  /* Read from selected plane */
#define VGA_READ_MODE_1         0x08  /* Color compare */

/* Attribute Controller Register Indices */
#define VGA_ATTR_PALETTE_0      0x00  /* Palette registers 0-15 */
#define VGA_ATTR_MODE_CONTROL   0x10  /* Mode Control */
#define VGA_ATTR_OVERSCAN       0x11  /* Overscan Color (Border) */
#define VGA_ATTR_COLOR_PLANE_EN 0x12  /* Color Plane Enable */
#define VGA_ATTR_HORIZ_PEL_PAN  0x13  /* Horizontal Pel Panning */
#define VGA_ATTR_COLOR_SELECT   0x14  /* Color Select */

/* --------------------------------------------------------------- */
/*  PC Graphics Backend Structure                                   */
/* --------------------------------------------------------------- */

typedef struct _PCGA_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;

    /* Framebuffer access */
    UINT8                       *FramebufferBase;
    BOOLEAN                     IsAddressable;      /* Can use direct pointer */

    /* VGA Register State (for planar modes with latching) */
    UINT8                       CurrentWriteMode;   /* Current write mode (0-3) */
    UINT8                       CurrentReadMode;    /* Current read mode (0-1) */
    UINT8                       CurrentMapMask;     /* Plane write mask */
    UINT8                       CurrentReadMap;     /* Read plane select */
    UINT8                       CurrentSetReset;    /* Set/Reset value */
    UINT8                       CurrentEnableSetReset; /* Enable Set/Reset */
    UINT8                       CurrentBitMask;     /* Bit Mask register */
    UINT8                       CurrentDataRotate;  /* Data Rotate/Function Select */

    /* Bank switching (for VESA banked modes) */
    UINT32                      CurrentBank;
    VOID (*BankSwitchFunc)(UINT32 BankNumber);

    /* Runtime optimizations */
    VOID (*RtlCopyMemoryFunc)(VOID *Dest, CONST VOID *Src, SIZE_T Size);

    /* Palette (for indexed modes) */
    FB_PALETTE_ENTRY            Palette[256];
    UINT8                       BorderColor;        /* VGA overscan/border color */

    /* Font data (for text modes) */
    UINT8                       *FontData;          /* Custom font or NULL for ROM */
    UINT32                      FontHeight;         /* Character height in pixels */
    UINT32                      FontBank;           /* VGA font bank (0 or 1) */

    /* Cursor (for hardware cursor support) */
    BOOLEAN                     CursorVisible;
    INT32                       CursorX;
    INT32                       CursorY;
    UINT32                      CursorStart;        /* Cursor start scanline */
    UINT32                      CursorEnd;          /* Cursor end scanline */
} PCGA_BACKEND;

/* --------------------------------------------------------------- */
/*  VGA Register Manipulation (with state caching)                 */
/* --------------------------------------------------------------- */

static INLINE VOID
Pcga_WriteSeq(
    UINT8 Index,
    UINT8 Value
    )
{
    ANX_CPU_OUTB(VGA_SEQ_INDEX, Index);
    ANX_CPU_OUTB(VGA_SEQ_DATA, Value);
}

static INLINE UINT8
Pcga_ReadSeq(
    UINT8 Index
    )
{
    ANX_CPU_OUTB(VGA_SEQ_INDEX, Index);
    return ANX_CPU_INB(VGA_SEQ_DATA);
}

static INLINE VOID
Pcga_WriteGc(
    UINT8 Index,
    UINT8 Value
    )
{
    ANX_CPU_OUTB(VGA_GC_INDEX, Index);
    ANX_CPU_OUTB(VGA_GC_DATA, Value);
}

static INLINE UINT8
Pcga_ReadGc(
    UINT8 Index
    )
{
    ANX_CPU_OUTB(VGA_GC_INDEX, Index);
    return ANX_CPU_INB(VGA_GC_DATA);
}

/* Set VGA write mode (0-3) with state caching */
static INLINE VOID
Pcga_SetWriteMode(
    PCGA_BACKEND *Backend,
    UINT8 WriteMode
    )
{
    if (Backend->CurrentWriteMode != WriteMode) {
        UINT8 Mode = Pcga_ReadGc(VGA_GC_GRAPHICS_MODE);
        Mode = (Mode & ~0x03) | (WriteMode & 0x03);
        Pcga_WriteGc(VGA_GC_GRAPHICS_MODE, Mode);
        Backend->CurrentWriteMode = WriteMode;
    }
}

/* Set plane write mask (Map Mask) */
static INLINE VOID
Pcga_SetMapMask(
    PCGA_BACKEND *Backend,
    UINT8 Mask
    )
{
    if (Backend->CurrentMapMask != Mask) {
        Pcga_WriteSeq(VGA_SEQ_MAP_MASK, Mask);
        Backend->CurrentMapMask = Mask;
    }
}

/* Set read plane select */
static INLINE VOID
Pcga_SetReadMap(
    PCGA_BACKEND *Backend,
    UINT8 Plane
    )
{
    if (Backend->CurrentReadMap != Plane) {
        Pcga_WriteGc(VGA_GC_READ_MAP_SELECT, Plane & 0x03);
        Backend->CurrentReadMap = Plane;
    }
}

/* Set Set/Reset registers for fast planar fill */
static INLINE VOID
Pcga_SetSetReset(
    PCGA_BACKEND *Backend,
    UINT8 Color,
    UINT8 EnableMask
    )
{
    if (Backend->CurrentSetReset != Color) {
        Pcga_WriteGc(VGA_GC_SET_RESET, Color);
        Backend->CurrentSetReset = Color;
    }
    if (Backend->CurrentEnableSetReset != EnableMask) {
        Pcga_WriteGc(VGA_GC_ENABLE_SET_RESET, EnableMask);
        Backend->CurrentEnableSetReset = EnableMask;
    }
}

/* Set Bit Mask register */
static INLINE VOID
Pcga_SetBitMask(
    PCGA_BACKEND *Backend,
    UINT8 Mask
    )
{
    if (Backend->CurrentBitMask != Mask) {
        Pcga_WriteGc(VGA_GC_BIT_MASK, Mask);
        Backend->CurrentBitMask = Mask;
    }
}

/* Set Data Rotate and hardware ROP function */
static INLINE VOID
Pcga_SetDataRotate(
    PCGA_BACKEND *Backend,
    UINT8 RotateCount,
    UINT8 Function  /* 0=unmodified, 1=AND, 2=OR, 3=XOR */
    )
{
    UINT8 Value = (Function << 3) | (RotateCount & 0x07);
    if (Backend->CurrentDataRotate != Value) {
        Pcga_WriteGc(VGA_GC_DATA_ROTATE, Value);
        Backend->CurrentDataRotate = Value;
    }
}

/* --------------------------------------------------------------- */
/*  VGA Latching Operations (Fast Planar Graphics)                 */
/* --------------------------------------------------------------- */

/*
 * Fast planar fill using Write Mode 0 with Set/Reset enabled.
 * This fills all planes simultaneously with the specified color.
 */
static VOID
Pcga_FillPlanar(
    PCGA_BACKEND *Backend,
    UINT32 Offset,
    UINT32 Count,
    UINT8 Color
    )
{
    /* Set write mode 0 with set/reset enabled for all planes */
    Pcga_SetWriteMode(Backend, VGA_WRITE_MODE_0);
    Pcga_SetMapMask(Backend, 0x0F);         /* Write all planes */
    Pcga_SetSetReset(Backend, Color, 0x0F); /* Set/Reset for all planes */
    Pcga_SetBitMask(Backend, 0xFF);         /* All bits */
    Pcga_SetDataRotate(Backend, 0, 0);      /* No rotation, unmodified */

    /* Write to framebuffer - VGA hardware uses Set/Reset value */
    volatile UINT8 *Dest = Backend->FramebufferBase + Offset;
    for (UINT32 i = 0; i < Count; i++) {
        /* Read to load latches, then write (hardware uses Set/Reset value) */
        volatile UINT8 Dummy = Dest[i];
        (VOID)Dummy;
        Dest[i] = 0; /* Value doesn't matter, Set/Reset is used */
    }
}

/*
 * Fast planar copy using Write Mode 1 (latch copy).
 * This is the fastest way to copy planar memory - one read, one write per byte.
 */
static VOID
Pcga_CopyPlanar(
    PCGA_BACKEND *Backend,
    UINT32 DestOffset,
    UINT32 SrcOffset,
    UINT32 Count
    )
{
    /* Set write mode 1 (latch copy) */
    Pcga_SetWriteMode(Backend, VGA_WRITE_MODE_1);
    Pcga_SetMapMask(Backend, 0x0F);     /* Write all planes */
    Pcga_SetBitMask(Backend, 0xFF);     /* All bits */

    volatile UINT8 *Src = Backend->FramebufferBase + SrcOffset;
    volatile UINT8 *Dest = Backend->FramebufferBase + DestOffset;

    /* Read loads all 4 planes into latches, write copies latches */
    for (UINT32 i = 0; i < Count; i++) {
        volatile UINT8 LatchData = Src[i];  /* Load latches from all 4 planes */
        (VOID)LatchData;
        Dest[i] = 0;  /* Value doesn't matter, latches are copied */
    }
}

/*
 * Fast planar line fill with hardware ROP.
 * Uses Write Mode 0 with optional AND/OR/XOR operation.
 */
static VOID
Pcga_FillPlanarWithRop(
    PCGA_BACKEND *Backend,
    UINT32 Offset,
    UINT32 Count,
    UINT8 Color,
    UINT8 RopFunction  /* 0=replace, 1=AND, 2=OR, 3=XOR */
    )
{
    Pcga_SetWriteMode(Backend, VGA_WRITE_MODE_0);
    Pcga_SetMapMask(Backend, 0x0F);
    Pcga_SetSetReset(Backend, Color, 0x0F);
    Pcga_SetBitMask(Backend, 0xFF);
    Pcga_SetDataRotate(Backend, 0, RopFunction);  /* Hardware ROP */

    volatile UINT8 *Dest = Backend->FramebufferBase + Offset;
    for (UINT32 i = 0; i < Count; i++) {
        volatile UINT8 Dummy = Dest[i];  /* Load latches */
        (VOID)Dummy;
        Dest[i] = 0;  /* Hardware applies ROP with Set/Reset value */
    }
}

/* --------------------------------------------------------------- */
/*  Palette DAC Programming                                        */
/* --------------------------------------------------------------- */

static VOID
Pcga_SetPaletteEntry(
    PCGA_BACKEND *Backend,
    UINT8 Index,
    CONST FB_PALETTE_ENTRY *Entry
    )
{
    ANX_CPU_OUTB(VGA_DAC_WRITE_INDEX, Index);
    ANX_CPU_OUTB(VGA_DAC_DATA, Entry->Red >> 2);    /* VGA DAC is 6-bit */
    ANX_CPU_OUTB(VGA_DAC_DATA, Entry->Green >> 2);
    ANX_CPU_OUTB(VGA_DAC_DATA, Entry->Blue >> 2);
    Backend->Palette[Index] = *Entry;
}

static VOID
Pcga_SetPalette(
    PCGA_BACKEND *Backend,
    UINT32 StartIndex,
    UINT32 Count,
    CONST FB_PALETTE_ENTRY *Entries
    )
{
    ANX_CPU_OUTB(VGA_DAC_WRITE_INDEX, (UINT8)StartIndex);
    for (UINT32 i = 0; i < Count; i++) {
        ANX_CPU_OUTB(VGA_DAC_DATA, Entries[i].Red >> 2);
        ANX_CPU_OUTB(VGA_DAC_DATA, Entries[i].Green >> 2);
        ANX_CPU_OUTB(VGA_DAC_DATA, Entries[i].Blue >> 2);
        Backend->Palette[StartIndex + i] = Entries[i];
    }
}

static VOID
Pcga_GetPaletteEntry(
    PCGA_BACKEND *Backend,
    UINT8 Index,
    FB_PALETTE_ENTRY *Entry
    )
{
    ANX_CPU_OUTB(VGA_DAC_READ_INDEX, Index);
    Entry->Red = ANX_CPU_INB(VGA_DAC_DATA) << 2;    /* Convert 6-bit to 8-bit */
    Entry->Green = ANX_CPU_INB(VGA_DAC_DATA) << 2;
    Entry->Blue = ANX_CPU_INB(VGA_DAC_DATA) << 2;
    Entry->Reserved = 0;
}

/* --------------------------------------------------------------- */
/*  Border Color Control (Attribute Controller Overscan)          */
/* --------------------------------------------------------------- */

static VOID
Pcga_SetBorderColor(
    PCGA_BACKEND *Backend,
    UINT8 Color
    )
{
    /* Read Input Status to reset attribute controller flip-flop */
    (VOID)ANX_CPU_INB(VGA_INPUT_STATUS_1);

    /* Write index */
    ANX_CPU_OUTB(VGA_ATTR_INDEX, VGA_ATTR_OVERSCAN);
    /* Write data */
    ANX_CPU_OUTB(VGA_ATTR_DATA_W, Color);

    /* Re-enable video */
    ANX_CPU_OUTB(VGA_ATTR_INDEX, 0x20);

    Backend->BorderColor = Color;
}

/* --------------------------------------------------------------- */
/*  Text Mode Caret Control (CRTC Cursor)                         */
/* --------------------------------------------------------------- */

static INLINE VOID
Pcga_WriteCrtc(
    UINT8 Index,
    UINT8 Value
    )
{
    ANX_CPU_OUTB(VGA_CRTC_INDEX, Index);
    ANX_CPU_OUTB(VGA_CRTC_DATA, Value);
}

static INLINE UINT8
Pcga_ReadCrtc(
    UINT8 Index
    )
{
    ANX_CPU_OUTB(VGA_CRTC_INDEX, Index);
    return ANX_CPU_INB(VGA_CRTC_DATA);
}

static VOID
Pcga_SetCaretPosition(
    PCGA_BACKEND *Backend,
    UINT32 X,
    UINT32 Y
    )
{
    /* Calculate linear position (row * columns + column) */
    UINT32 Position = Y * Backend->Descriptor.Width + X;

    /* CRTC registers 0x0E (Cursor Location High) and 0x0F (Cursor Location Low) */
    Pcga_WriteCrtc(0x0E, (UINT8)(Position >> 8));
    Pcga_WriteCrtc(0x0F, (UINT8)(Position & 0xFF));

    Backend->CursorX = X;
    Backend->CursorY = Y;
}

static VOID
Pcga_SetCaretShape(
    PCGA_BACKEND *Backend,
    UINT32 StartLine,
    UINT32 EndLine,
    BOOLEAN Visible
    )
{
    /* CRTC register 0x0A (Cursor Start), bit 5 = disable cursor */
    UINT8 Start = (UINT8)(StartLine & 0x1F);
    if (!Visible) {
        Start |= 0x20;  /* Disable cursor */
    }
    Pcga_WriteCrtc(0x0A, Start);

    /* CRTC register 0x0B (Cursor End) */
    Pcga_WriteCrtc(0x0B, (UINT8)(EndLine & 0x1F));

    Backend->CursorVisible = Visible;
    Backend->CursorStart = StartLine;
    Backend->CursorEnd = EndLine;
}

/* --------------------------------------------------------------- */
/*  Text Mode Rendering                                            */
/* --------------------------------------------------------------- */

static VOID
Pcga_WriteTextChar(
    PCGA_BACKEND *Backend,
    UINT32 X,
    UINT32 Y,
    UINT8 Character,
    UINT8 Attribute
    )
{
    /* Text mode: character/attribute pairs */
    UINT32 Offset = (Y * Backend->Descriptor.Width + X) * 2;
    Backend->FramebufferBase[Offset] = Character;
    Backend->FramebufferBase[Offset + 1] = Attribute;
}

static VOID
Pcga_WriteTextString(
    PCGA_BACKEND *Backend,
    UINT32 X,
    UINT32 Y,
    CONST CHAR8 *String,
    UINT8 Attribute
    )
{
    UINT32 Offset = (Y * Backend->Descriptor.Width + X) * 2;
    UINT32 i = 0;

    while (String[i] != '\0' && (X + i) < Backend->Descriptor.Width) {
        Backend->FramebufferBase[Offset + i * 2] = (UINT8)String[i];
        Backend->FramebufferBase[Offset + i * 2 + 1] = Attribute;
        i++;
    }
}

static VOID
Pcga_ScrollText(
    PCGA_BACKEND *Backend,
    INT32 Lines,
    UINT8 FillAttribute
    )
{
    UINT32 Width = Backend->Descriptor.Width;
    UINT32 Height = Backend->Descriptor.Height;
    UINT32 LineBytes = Width * 2;  /* char + attribute */

    if (Lines > 0) {
        /* Scroll up */
        UINT32 SrcOffset = Lines * LineBytes;
        UINT32 Count = (Height - Lines) * LineBytes;

        /* Use fast copy if addressable */
        if (Backend->IsAddressable) {
            if (Backend->RtlCopyMemoryFunc) {
                Backend->RtlCopyMemoryFunc(Backend->FramebufferBase,
                                          Backend->FramebufferBase + SrcOffset,
                                          Count);
            } else {
                ANX_MEMCPY(Backend->FramebufferBase,
                          Backend->FramebufferBase + SrcOffset,
                          Count);
            }
        }

        /* Fill bottom lines */
        UINT32 FillOffset = (Height - Lines) * LineBytes;
        for (UINT32 i = 0; i < Lines * Width; i++) {
            Backend->FramebufferBase[FillOffset + i * 2] = ' ';
            Backend->FramebufferBase[FillOffset + i * 2 + 1] = FillAttribute;
        }
    }
}

/* --------------------------------------------------------------- */
/*  Hardware Mouse Cursor (Software Overlay)                      */
/* --------------------------------------------------------------- */

typedef struct _PCGA_CURSOR_STATE {
    UINT8   SavedData[64 * 64 * 4];  /* Saved background */
    UINT32  SavedX;
    UINT32  SavedY;
    UINT32  Width;
    UINT32  Height;
    BOOLEAN Valid;
} PCGA_CURSOR_STATE;

static PCGA_CURSOR_STATE gCursorState = {0};

static VOID
Pcga_ShowCursor(
    PCGA_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    CONST UINT8 *CursorData,  /* RGBA or AND/XOR masks */
    UINT32 Width,
    UINT32 Height,
    BOOLEAN IsColor
    )
{
    /* Save background and draw cursor */
    /* This is a software cursor - we save/restore the background */
    /* Implementation depends on pixel format */

    gCursorState.SavedX = X;
    gCursorState.SavedY = Y;
    gCursorState.Width = Width;
    gCursorState.Height = Height;
    gCursorState.Valid = TRUE;

    /* TODO: Actually implement cursor rendering based on format */
}

static VOID
Pcga_HideCursor(
    PCGA_BACKEND *Backend
    )
{
    if (gCursorState.Valid) {
        /* Restore saved background */
        /* TODO: Implement background restoration */
        gCursorState.Valid = FALSE;
    }
}

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
    PCGA_BACKEND *Backend,
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
    PCGA_BACKEND *Backend,
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
    PCGA_BACKEND *Backend,
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
    PCGA_BACKEND *Backend,
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
    PCGA_BACKEND *Backend,
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
    PCGA_BACKEND *Backend,
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
    PCGA_BACKEND *Backend,
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
    PCGA_BACKEND *Backend,
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
    PCGA_BACKEND *Backend,
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
    PCGA_BACKEND *Backend,
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
    PCGA_BACKEND *Backend = (PCGA_BACKEND *)This;

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
    PCGA_BACKEND *Backend = (PCGA_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
PcGraphics_Release(
    IFramebufferBackend *This
    )
{
    PCGA_BACKEND *Backend = (PCGA_BACKEND *)This;
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
    PCGA_BACKEND *Backend = (PCGA_BACKEND *)This;

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
    PCGA_BACKEND *Backend = (PCGA_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    /* Handle text mode specially */
    if (Backend->Descriptor.PixelFormat == FbPixelFormatText) {
        UINT8 Attr = 0x07;  /* Light gray on black */
        for (UINT32 y = 0; y < Backend->Descriptor.Height; y++) {
            for (UINT32 x = 0; x < Backend->Descriptor.Width; x++) {
                Pcga_WriteTextChar(Backend, x, y, ' ', Attr);
            }
        }
        return S_OK;
    }

    ColorIndex = PcGraphics_MapColorToIndex(Backend, Color);

    /* Use VGA latching for planar modes */
    if (Backend->Descriptor.PixelFormat == FbPixelFormatPlanar) {
        UINT32 TotalBytes = Backend->Descriptor.Height *
                           (Backend->Descriptor.Pitch / Backend->Descriptor.NumPlanes);
        Pcga_FillPlanar(Backend, 0, TotalBytes, ColorIndex);
        return S_OK;
    }

    /* Use memset for linear indexed modes */
    if (Backend->Descriptor.MemoryOrganization == FbMemoryLinear &&
        Backend->Descriptor.PixelFormat == FbPixelFormatIndexed) {
        for (UINT32 y = 0; y < Backend->Descriptor.Height; y++) {
            UINT32 Offset = y * Backend->Descriptor.Pitch;
            ANX_MEMSET(&Backend->FramebufferBase[Offset], ColorIndex, Backend->Descriptor.Width);
        }
        return S_OK;
    }

    /* Fallback for other modes */
    for (UINT32 y = 0; y < Backend->Descriptor.Height; y++) {
        for (UINT32 x = 0; x < Backend->Descriptor.Width; x++) {
            PcGraphics_WritePixel(Backend, x, y, ColorIndex);
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
    PCGA_BACKEND *Backend = (PCGA_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    /* Text mode */
    if (Backend->Descriptor.PixelFormat == FbPixelFormatText) {
        Pcga_WriteTextChar(Backend, X, Y, ' ', 0x0F);  /* White on black */
        return S_OK;
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
    PCGA_BACKEND *Backend = (PCGA_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized || Rect == NULL) {
        return E_POINTER;
    }

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

    /* Text mode */
    if (Backend->Descriptor.PixelFormat == FbPixelFormatText) {
        UINT8 Attr = 0x07;  /* Light gray on black */
        for (UINT32 y = Top; y < (UINT32)Bottom; y++) {
            for (UINT32 x = Left; x < (UINT32)Right; x++) {
                Pcga_WriteTextChar(Backend, x, y, ' ', Attr);
            }
        }
        return S_OK;
    }

    ColorIndex = PcGraphics_MapColorToIndex(Backend, Color);

    /* Use VGA latching for planar modes */
    if (Backend->Descriptor.PixelFormat == FbPixelFormatPlanar) {
        for (UINT32 y = Top; y < (UINT32)Bottom; y++) {
            UINT32 Offset = y * (Backend->Descriptor.Pitch / Backend->Descriptor.NumPlanes) + (Left / 8);
            UINT32 Count = (Width + 7) / 8;
            Pcga_FillPlanar(Backend, Offset, Count, ColorIndex);
        }
        return S_OK;
    }

    /* Use memset for linear indexed modes */
    if (Backend->Descriptor.MemoryOrganization == FbMemoryLinear &&
        Backend->Descriptor.PixelFormat == FbPixelFormatIndexed) {
        for (UINT32 y = Top; y < (UINT32)Bottom; y++) {
            UINT32 Offset = y * Backend->Descriptor.Pitch + Left;
            ANX_MEMSET(&Backend->FramebufferBase[Offset], ColorIndex, Width);
        }
        return S_OK;
    }

    /* Fallback */
    for (INT32 y = Top; y < Bottom; y++) {
        for (INT32 x = Left; x < Right; x++) {
            PcGraphics_WritePixel(Backend, x, y, ColorIndex);
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
    PCGA_BACKEND *Backend = (PCGA_BACKEND *)This;
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
    PCGA_BACKEND *Backend = (PCGA_BACKEND *)This;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    /* Text mode - not supported for blitting */
    if (Backend->Descriptor.PixelFormat == FbPixelFormatText) {
        return E_NOTIMPL;
    }

    /* Fast path: matching format */
    if (SourceFormat == Backend->Descriptor.PixelFormat) {
        /* Planar to planar using VGA latching */
        if (Backend->Descriptor.PixelFormat == FbPixelFormatPlanar) {
            for (UINT32 Row = 0; Row < Height; Row++) {
                UINT32 DestOffset = (Y + Row) * (Backend->Descriptor.Pitch / Backend->Descriptor.NumPlanes) + (X / 8);
                UINT32 SrcOffset = Row * ((Width + 7) / 8);
                UINT32 Count = (Width + 7) / 8;

                /* Use latch copy for matching planar format */
                if (Backend->IsAddressable) {
                    Pcga_CopyPlanar(Backend, DestOffset, SrcOffset, Count);
                }
            }
            return S_OK;
        }

        /* Linear indexed to linear indexed */
        if (Backend->Descriptor.MemoryOrganization == FbMemoryLinear &&
            Backend->Descriptor.PixelFormat == FbPixelFormatIndexed) {
            for (UINT32 Row = 0; Row < Height; Row++) {
                UINT32 DestOffset = (Y + Row) * Backend->Descriptor.Pitch + X;
                UINT32 SrcOffset = Row * Width;
                if (Backend->RtlCopyMemoryFunc) {
                    Backend->RtlCopyMemoryFunc(&Backend->FramebufferBase[DestOffset],
                                              &Bitmap[SrcOffset], Width);
                } else {
                    ANX_MEMCPY(&Backend->FramebufferBase[DestOffset], &Bitmap[SrcOffset], Width);
                }
            }
            return S_OK;
        }

        /* RGB formats */
        if (Backend->Descriptor.PixelFormat == FbPixelFormatRgb) {
            UINT32 BytesPerPixel = (Backend->Descriptor.BitsPerPixel + 7) / 8;
            return PcGraphics_BlitLinearSameFormat(Backend, X, Y, Width, Height,
                                                  Bitmap, Width * BytesPerPixel);
        }
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
    PCGA_BACKEND *Backend = (PCGA_BACKEND *)This;

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
    PCGA_BACKEND *Backend = (PCGA_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static PCGA_BACKEND gPcgaBackendInstance = {
    .Base.lpVtbl        = &gPcGraphicsVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherNone,
    .BankSwitchFunc     = NULL,
    .RtlCopyMemoryFunc  = NULL,
    .IsAddressable      = FALSE,
    .BorderColor        = 0,
    .FontData           = NULL,
    .CursorVisible      = FALSE,
};

IFramebufferBackend *
FbCreatePcGraphicsBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gPcgaBackendInstance;
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
    PCGA_BACKEND *PcgaBackend = (PCGA_BACKEND *)Backend;
    PcgaBackend->BankSwitchFunc = BankSwitchFunc;
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
    PCGA_BACKEND *PcgaBackend = (PCGA_BACKEND *)Backend;
    PcgaBackend->RtlCopyMemoryFunc = RtlCopyMemoryFunc;
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
    PCGA_BACKEND *PcBackend = (PCGA_BACKEND *)Backend;

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
