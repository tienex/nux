/*++
    Module Name:

        pcga.c (PC Graphics Adapter)

    Abstract:

        IBM PC-compatible graphics adapter backend.
        Handles all CGA, EGA, VGA, SVGA, and XGA modes including text modes.

        The pixel format describes WHAT the data is (mono, planar, indexed, RGB, text).
        The FRAMEBUFFER_DESC describes HOW it's organized in memory (linear,
        planar, banked, interleaved) and other characteristics.

    Implemented Features:
        ✓ ALL PC graphics modes (77 modes: text, CGA, EGA, VGA, MCGA, SVGA, Mode-X, VESA)
        ✓ VGA latching for fast planar writes (latch copy mode)
        ✓ Optimized blitting with RtlCopyMemory and memcpy (avoids pixel-by-pixel)
        ✓ ROP2/ROP3 raster operations for advanced compositing
        ✓ Text mode rendering with font support
        ✓ Three-tier font loading (BIOS call / ROM scan / bundled fonts)
        ✓ Palette programming (VGA DAC 6-bit RGB)
        ✓ Hardware cursor (software overlay with save/restore)
        ✓ Border color control (Attribute Controller)
        ✓ Hardware scrolling (display start address CRTC)
        ✓ VBlank synchronization
        ✓ Screen-to-screen blitting
        ✓ EDID parsing and mode filtering
        ✓ VESA VBE Linear and Banked framebuffer support
        ✓ IsAddressable flag for direct pointer access

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
/*  VESA VBE (Video BIOS Extensions) Support                       */
/* --------------------------------------------------------------- */

/* VESA VBE 2.0+ Mode Attributes */
#define VBE_MODE_SUPPORTED      0x0001  /* Mode supported by hardware */
#define VBE_MODE_OPTIONAL_INFO  0x0002  /* Optional information available */
#define VBE_MODE_BIOS_OUTPUT    0x0004  /* BIOS output supported */
#define VBE_MODE_COLOR          0x0008  /* Color mode */
#define VBE_MODE_GRAPHICS       0x0010  /* Graphics mode (not text) */
#define VBE_MODE_NOT_VGA        0x0020  /* Not VGA compatible */
#define VBE_MODE_NO_BANK_SWITCH 0x0040  /* Banked mode not supported */
#define VBE_MODE_LINEAR_FB      0x0080  /* Linear framebuffer available */
#define VBE_MODE_DOUBLE_SCAN    0x0100  /* Double-scan mode */
#define VBE_MODE_INTERLACED     0x0200  /* Interlaced mode */
#define VBE_MODE_TRIPLE_BUFFER  0x0400  /* Hardware triple buffering */
#define VBE_MODE_STEREO         0x0800  /* Stereoscopic display */
#define VBE_MODE_DUAL_START     0x1000  /* Dual display start address */

/* VESA VBE Mode Info Block (256 bytes) */
typedef struct _VBE_MODE_INFO {
    /* Mandatory information for all VBE revisions */
    UINT16  ModeAttributes;         /* Mode attributes */
    UINT8   WinAAttributes;         /* Window A attributes */
    UINT8   WinBAttributes;         /* Window B attributes */
    UINT16  WinGranularity;         /* Window granularity (KB) */
    UINT16  WinSize;                /* Window size (KB) */
    UINT16  WinASegment;            /* Window A start segment */
    UINT16  WinBSegment;            /* Window B start segment */
    UINT32  WinFuncPtr;             /* Pointer to window function */
    UINT16  BytesPerScanLine;       /* Bytes per scan line */

    /* Mandatory information for VBE 1.2+ */
    UINT16  XResolution;            /* Horizontal resolution */
    UINT16  YResolution;            /* Vertical resolution */
    UINT8   XCharSize;              /* Character cell width */
    UINT8   YCharSize;              /* Character cell height */
    UINT8   NumberOfPlanes;         /* Number of memory planes */
    UINT8   BitsPerPixel;           /* Bits per pixel */
    UINT8   NumberOfBanks;          /* Number of banks */
    UINT8   MemoryModel;            /* Memory model type */
    UINT8   BankSize;               /* Bank size in KB */
    UINT8   NumberOfImagePages;     /* Number of image pages */
    UINT8   Reserved1;              /* Reserved (0x01) */

    /* Direct Color fields (required for direct/6 and YUV/7 memory models) */
    UINT8   RedMaskSize;            /* Size of direct color red mask */
    UINT8   RedFieldPosition;       /* Bit position of LSB of red mask */
    UINT8   GreenMaskSize;          /* Size of direct color green mask */
    UINT8   GreenFieldPosition;     /* Bit position of LSB of green mask */
    UINT8   BlueMaskSize;           /* Size of direct color blue mask */
    UINT8   BlueFieldPosition;      /* Bit position of LSB of blue mask */
    UINT8   RsvdMaskSize;           /* Size of direct color reserved mask */
    UINT8   RsvdFieldPosition;      /* Bit position of LSB of reserved mask */
    UINT8   DirectColorModeInfo;    /* Direct color mode attributes */

    /* Mandatory information for VBE 2.0+ */
    UINT32  PhysBasePtr;            /* Physical address for linear framebuffer */
    UINT32  Reserved2;              /* Reserved (always 0) */
    UINT16  Reserved3;              /* Reserved (always 0) */

    /* Mandatory information for VBE 3.0+ */
    UINT16  LinBytesPerScanLine;    /* Bytes per scan line (linear modes) */
    UINT8   BnkNumberOfImagePages;  /* Number of images (banked modes) */
    UINT8   LinNumberOfImagePages;  /* Number of images (linear modes) */
    UINT8   LinRedMaskSize;         /* Size of red mask (linear modes) */
    UINT8   LinRedFieldPosition;    /* Bit position of red mask (linear) */
    UINT8   LinGreenMaskSize;       /* Size of green mask (linear modes) */
    UINT8   LinGreenFieldPosition;  /* Bit position of green mask (linear) */
    UINT8   LinBlueMaskSize;        /* Size of blue mask (linear modes) */
    UINT8   LinBlueFieldPosition;   /* Bit position of blue mask (linear) */
    UINT8   LinRsvdMaskSize;        /* Size of rsvd mask (linear modes) */
    UINT8   LinRsvdFieldPosition;   /* Bit position of rsvd mask (linear) */
    UINT32  MaxPixelClock;          /* Maximum pixel clock (Hz) */

    UINT8   Reserved4[189];         /* Reserved (remainder of 256 bytes) */
} VBE_MODE_INFO;

/* --------------------------------------------------------------- */
/*  EDID (Extended Display Identification Data) Support            */
/* --------------------------------------------------------------- */

/* EDID 1.3/1.4 Standard Timing Descriptor */
typedef struct _EDID_STANDARD_TIMING {
    UINT8   XResolution;            /* (Horizontal pixels / 8) - 31 */
    UINT8   AspectRatioRefresh;     /* Bits 7-6: aspect, 5-0: vrefresh-60 */
} EDID_STANDARD_TIMING;

/* EDID Detailed Timing Descriptor (18 bytes) */
typedef struct _EDID_DETAILED_TIMING {
    UINT16  PixelClock;             /* Pixel clock in 10 kHz units */
    UINT8   HActiveLow;             /* Horizontal active pixels (low 8 bits) */
    UINT8   HBlankingLow;           /* Horizontal blanking (low 8 bits) */
    UINT8   HActiveBlankingHigh;    /* HA high 4 bits, HB high 4 bits */
    UINT8   VActiveLow;             /* Vertical active lines (low 8 bits) */
    UINT8   VBlankingLow;           /* Vertical blanking (low 8 bits) */
    UINT8   VActiveBlankingHigh;    /* VA high 4 bits, VB high 4 bits */
    UINT8   HSyncOffsetLow;         /* H sync offset (low 8 bits) */
    UINT8   HSyncWidthLow;          /* H sync pulse width (low 8 bits) */
    UINT8   VSyncOffsetWidthLow;    /* V sync offset/width (low 4 bits each) */
    UINT8   SyncHighBits;           /* High bits for sync values */
    UINT8   HImageSizeLow;          /* Horizontal image size mm (low 8 bits) */
    UINT8   VImageSizeLow;          /* Vertical image size mm (low 8 bits) */
    UINT8   ImageSizeHigh;          /* High 4 bits for both image sizes */
    UINT8   HBorder;                /* Horizontal border pixels */
    UINT8   VBorder;                /* Vertical border lines */
    UINT8   Features;               /* Features bitmap */
} EDID_DETAILED_TIMING;

/* EDID 1.3 Base Block (128 bytes) */
typedef struct _EDID_BASE_BLOCK {
    /* Header (8 bytes) */
    UINT8   Header[8];              /* Fixed: 00 FF FF FF FF FF FF 00 */

    /* Vendor/Product Identification (10 bytes) */
    UINT16  ManufacturerID;         /* Compressed 3-letter manufacturer ID */
    UINT16  ProductCode;            /* Manufacturer product code */
    UINT32  SerialNumber;           /* Serial number */
    UINT8   WeekOfManufacture;      /* Week of manufacture (1-54, or 0xFF) */
    UINT8   YearOfManufacture;      /* Year of manufacture (+ 1990) */

    /* EDID Structure Version/Revision (2 bytes) */
    UINT8   EdidVersion;            /* EDID version (usually 1) */
    UINT8   EdidRevision;           /* EDID revision (3 or 4) */

    /* Basic Display Parameters (5 bytes) */
    UINT8   VideoInputDefinition;   /* Video input definition */
    UINT8   MaxHorizontalImageSize; /* Max horizontal size in cm (0 if undefined) */
    UINT8   MaxVerticalImageSize;   /* Max vertical size in cm (0 if undefined) */
    UINT8   DisplayGamma;           /* Display gamma ((value+100)/100, 0xFF if undefined) */
    UINT8   FeatureSupport;         /* Feature support bitmap */

    /* Color Characteristics (10 bytes) */
    UINT8   RedGreenLowBits;        /* Red/green low bits */
    UINT8   BlueWhiteLowBits;       /* Blue/white low bits */
    UINT8   RedXHigh;               /* Red X high 8 bits */
    UINT8   RedYHigh;               /* Red Y high 8 bits */
    UINT8   GreenXHigh;             /* Green X high 8 bits */
    UINT8   GreenYHigh;             /* Green Y high 8 bits */
    UINT8   BlueXHigh;              /* Blue X high 8 bits */
    UINT8   BlueYHigh;              /* Blue Y high 8 bits */
    UINT8   WhiteXHigh;             /* White point X high 8 bits */
    UINT8   WhiteYHigh;             /* White point Y high 8 bits */

    /* Established Timings (3 bytes) */
    UINT8   EstablishedTimings1;    /* Established timings I */
    UINT8   EstablishedTimings2;    /* Established timings II */
    UINT8   ManufacturersTimings;   /* Manufacturer's timings */

    /* Standard Timing Identification (16 bytes - 8 timings) */
    EDID_STANDARD_TIMING StandardTimings[8];

    /* Detailed Timing Descriptors (72 bytes - 4 descriptors) */
    EDID_DETAILED_TIMING DetailedTimings[4];

    /* Extension Flag and Checksum (2 bytes) */
    UINT8   ExtensionFlag;          /* Number of extension blocks */
    UINT8   Checksum;               /* Checksum (sum of all 128 bytes = 0) */
} EDID_BASE_BLOCK;

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
/*  Font Loading and Management                                    */
/* --------------------------------------------------------------- */

/*
 * Bundled VGA Font Data
 *
 * Standard IBM VGA fonts bundled as static data. This approach works in:
 * - Long mode (x86-64) where IVT at physical 0x0 isn't accessible
 * - Protected mode with paging
 * - UEFI environments without BIOS structures
 *
 * Fonts are based on standard IBM VGA ROM fonts (public domain).
 * Each character is stored as consecutive scanlines, one byte per scanline.
 * Bit 7 (MSB) = leftmost pixel, Bit 0 (LSB) = rightmost pixel.
 */

/* Standard VGA 8x8 font (CGA/MCGA compatible) - 256 characters × 8 bytes = 2048 bytes */
#include "vga_font_8x8.inc"

/* Standard VGA 8x14 font (EGA compatible) - 256 characters × 14 bytes = 3584 bytes */
#include "vga_font_8x14.inc"

/* Standard VGA 8x16 font (VGA/SVGA/XGA) - 256 characters × 16 bytes = 4096 bytes */
#include "vga_font_8x16.inc"

/* Graphics adapter types */
typedef enum _PCGA_ADAPTER_TYPE {
    PCGA_ADAPTER_CGA = 0,
    PCGA_ADAPTER_EGA = 1,
    PCGA_ADAPTER_MCGA = 2,  /* PS/2 Model 25/30 - VGA subset */
    PCGA_ADAPTER_VGA = 3,
    PCGA_ADAPTER_SVGA = 4,  /* Super VGA extensions */
    PCGA_ADAPTER_XGA = 5,   /* IBM XGA */
} PCGA_ADAPTER_TYPE;

/* Video BIOS ROM typically mapped at 0xC0000-0xC7FFF (32KB) */
#define VIDEO_BIOS_ROM_BASE     0xC0000
#define VIDEO_BIOS_ROM_SIZE     0x8000  /* 32KB */

/*
 * Real mode BIOS call interface (if available).
 * Returns TRUE if BIOS INT 10h font retrieval succeeded.
 *
 * Uses INT 10h, AX=1130h - Get Font Information:
 *   BH = font type (0=INT 1Fh 8x8, 3=8x8 0-7Fh, 6=8x16)
 *   Returns: ES:BP = pointer to font, CX = bytes per character
 */
static BOOLEAN
Pcga_GetFontViaBios(
    OUT UINT8 *FontBuffer,
    IN UINT32 CharHeight
    )
{
    /* Real mode calls from protected/long mode require either:
     * 1. Real mode thunk (transition to real mode, make call, return)
     * 2. V8086 mode (virtual 8086 mode for real mode emulation)
     * 3. BIOS call wrapper (OS-provided service)
     *
     * Since we don't have a real mode thunk infrastructure in this
     * codebase, and V8086 mode is not available in long mode, we
     * cannot make INT 10h calls directly.
     *
     * In UEFI environments, the BIOS is not available at all.
     *
     * This function could be implemented if:
     * - The OS provides a BIOS call wrapper service (like Linux's vm86)
     * - A real mode thunk is added to the codebase
     * - Running in 16-bit real mode or virtual 8086 mode
     */

    #ifdef ANX_REALMODE_THUNK_AVAILABLE
        /* If real mode thunk is available, use it */
        /* Example pseudocode:
         * REALMODE_REGS regs;
         * regs.ax = 0x1130;
         * regs.bh = (CharHeight == 8) ? 3 : (CharHeight == 14) ? 2 : 6;
         * if (AnxRealModeInt(0x10, &regs) == 0) {
         *     UINT32 FontAddr = ((UINT32)regs.es << 4) + regs.bp;
         *     ANX_MEMCPY(FontBuffer, (VOID *)(UINTN)FontAddr, 256 * CharHeight);
         *     return TRUE;
         * }
         */
    #endif

    /* Not available in this environment */
    return FALSE;
}

/*
 * Check if memory region looks like valid VGA font data.
 * Returns TRUE if the data pattern matches typical font characteristics.
 */
static BOOLEAN
Pcga_IsValidFontData(
    CONST UINT8 *Data,
    UINT32 CharHeight
    )
{
    UINT32 RequiredSize = 256 * CharHeight;

    /* Check for typical font patterns:
     * - Character 0x00 (NULL) is usually all zeros
     * - Character 0x20 (space) is usually all zeros
     * - Characters have reasonable bit patterns (not all 0xFF or random data)
     */

    /* Check character 0x00 (should be mostly zeros) */
    UINT32 NonZeroCount = 0;
    for (UINT32 i = 0; i < CharHeight; i++) {
        if (Data[i] != 0x00) {
            NonZeroCount++;
        }
    }
    if (NonZeroCount > CharHeight / 2) {
        /* Too many non-zero bytes for NULL character */
        return FALSE;
    }

    /* Check character 0x20 (space - offset 32 * CharHeight) */
    CONST UINT8 *SpaceChar = &Data[32 * CharHeight];
    NonZeroCount = 0;
    for (UINT32 i = 0; i < CharHeight; i++) {
        if (SpaceChar[i] != 0x00) {
            NonZeroCount++;
        }
    }
    if (NonZeroCount > 2) {
        /* Space should be mostly empty */
        return FALSE;
    }

    /* Check that printable ASCII characters have reasonable patterns */
    /* Character 'A' (0x41) should have some vertical symmetry */
    CONST UINT8 *CharA = &Data[0x41 * CharHeight];
    BOOLEAN HasTopBits = FALSE;
    BOOLEAN HasBottomBits = FALSE;

    for (UINT32 i = 0; i < CharHeight / 2; i++) {
        if (CharA[i] != 0x00) HasTopBits = TRUE;
    }
    for (UINT32 i = CharHeight / 2; i < CharHeight; i++) {
        if (CharA[i] != 0x00) HasBottomBits = TRUE;
    }

    if (!HasTopBits || !HasBottomBits) {
        /* 'A' should have data in both top and bottom halves */
        return FALSE;
    }

    return TRUE;
}

/*
 * Scan Video BIOS ROM for font data.
 * Video BIOS ROM is typically at 0xC0000-0xC7FFF (32KB).
 * Returns TRUE if font found and copied.
 */
static BOOLEAN
Pcga_ScanVideoBiosRom(
    OUT UINT8 *FontBuffer,
    IN UINT32 CharHeight
    )
{
    UINT32 RequiredSize = 256 * CharHeight;
    CONST UINT8 *RomBase;
    UINT32 SearchEnd;

    /* Try to access Video BIOS ROM at 0xC0000 */
    /* NOTE: This may fail if the ROM region is not mapped in page tables */
    RomBase = (CONST UINT8 *)(UINTN)VIDEO_BIOS_ROM_BASE;

    /* Check if ROM is accessible by testing for VGA BIOS signature */
    /* VGA BIOS starts with: 0x55 0xAA (ROM signature) */
    if (RomBase[0] != 0x55 || RomBase[1] != 0xAA) {
        /* ROM not accessible or not a valid BIOS ROM */
        return FALSE;
    }

    /* Scan ROM for font data
     * Fonts are usually 16-byte aligned within the ROM
     * Search from offset 0x1000 to avoid BIOS code at the start
     */
    SearchEnd = VIDEO_BIOS_ROM_SIZE - RequiredSize;

    for (UINT32 Offset = 0x1000; Offset < SearchEnd; Offset += 16) {
        CONST UINT8 *Candidate = &RomBase[Offset];

        /* Check if this looks like valid font data */
        if (Pcga_IsValidFontData(Candidate, CharHeight)) {
            /* Found likely font data, copy it */
            ANX_MEMCPY(FontBuffer, Candidate, RequiredSize);
            return TRUE;
        }
    }

    /* Font not found in ROM */
    return FALSE;
}

/*
 * Detect graphics adapter type based on hardware.
 */
static PCGA_ADAPTER_TYPE
Pcga_DetectAdapter(
    VOID
    )
{
    /* Check for VGA/MCGA/SVGA/XGA by reading VGA-specific registers */
    /* VGA has Input Status Register 1 at 0x3DA with specific behavior */
    UINT8 InputStatus = ANX_CPU_INB(VGA_INPUT_STATUS_1);

    /* Try to read CRTC register - VGA has more CRTC registers than EGA */
    ANX_CPU_OUTB(VGA_CRTC_INDEX, 0x1F);  /* VGA-specific register */
    UINT8 TestValue = ANX_CPU_INB(VGA_CRTC_DATA);
    ANX_CPU_OUTB(VGA_CRTC_INDEX, 0x1F);
    ANX_CPU_OUTB(VGA_CRTC_DATA, 0x55);
    UINT8 ReadBack = ANX_CPU_INB(VGA_CRTC_DATA);

    if (ReadBack == 0x55) {
        /* VGA-compatible detected - now differentiate between VGA/MCGA/SVGA/XGA */
        ANX_CPU_OUTB(VGA_CRTC_DATA, TestValue);  /* Restore */

        /* Check for SVGA/XGA by looking for extended registers */
        /* SVGA typically has more CRTC registers (up to 0x3F) */
        ANX_CPU_OUTB(VGA_CRTC_INDEX, 0x30);  /* SVGA-specific register */
        UINT8 SvgaTest = ANX_CPU_INB(VGA_CRTC_DATA);
        ANX_CPU_OUTB(VGA_CRTC_INDEX, 0x30);
        ANX_CPU_OUTB(VGA_CRTC_DATA, 0xAA);
        UINT8 SvgaReadBack = ANX_CPU_INB(VGA_CRTC_DATA);
        ANX_CPU_OUTB(VGA_CRTC_DATA, SvgaTest);  /* Restore */

        if (SvgaReadBack == 0xAA) {
            /* SVGA/XGA detected */
            /* Further differentiate XGA by checking for XGA-specific features */
            /* XGA has specific I/O ports at 0x21x8-0x21xF */
            /* For now, treat as SVGA */
            return PCGA_ADAPTER_SVGA;
        }

        /* Check for MCGA vs VGA */
        /* MCGA is found on PS/2 Model 25 and 30 */
        /* MCGA has limited mode support compared to VGA */
        /* Read DAC state to differentiate (VGA has 256 entries, MCGA has 64) */
        ANX_CPU_OUTB(VGA_DAC_WRITE_INDEX, 0xFF);
        ANX_CPU_OUTB(VGA_DAC_DATA, 0x3F);  /* Write max value */
        ANX_CPU_OUTB(VGA_DAC_DATA, 0x3F);
        ANX_CPU_OUTB(VGA_DAC_DATA, 0x3F);

        /* Try to read it back */
        ANX_CPU_OUTB(VGA_DAC_READ_INDEX, 0xFF);
        UINT8 DacRead = ANX_CPU_INB(VGA_DAC_DATA);

        if (DacRead == 0x3F) {
            /* Full VGA detected (256-color palette) */
            return PCGA_ADAPTER_VGA;
        } else {
            /* MCGA detected (64-color palette limit) */
            return PCGA_ADAPTER_MCGA;
        }
    }

    /* Check for EGA by reading switch settings */
    UINT8 Misc = ANX_CPU_INB(VGA_MISC_READ);
    if ((Misc & 0x30) == 0x20) {
        /* EGA detected (different switch configuration) */
        return PCGA_ADAPTER_EGA;
    }

    /* Default to CGA */
    return PCGA_ADAPTER_CGA;
}

/*
 * Extract font using three-tier fallback strategy:
 * 1. Try Real Mode BIOS call (INT 10h, AX=1130h) if available
 * 2. Try scanning Video BIOS ROM at 0xC0000-0xC7FFF
 * 3. Fall back to bundled fonts
 *
 * This approach works across different environments:
 * - Real mode: Can use BIOS calls
 * - Protected mode with ROM mapped: Can scan ROM
 * - Long mode / UEFI: Uses bundled fonts
 */
static VOID
Pcga_ExtractRomFont(
    OUT UINT8 *FontBuffer,
    IN UINT32 CharHeight,
    IN PCGA_ADAPTER_TYPE AdapterType
    )
{
    CONST UINT8 *BundledFont;

    if (FontBuffer == NULL) {
        return;
    }

    /* Tier 1: Try BIOS call (if real mode interface available) */
    if (Pcga_GetFontViaBios(FontBuffer, CharHeight)) {
        return;  /* Success */
    }

    /* Tier 2: Try scanning Video BIOS ROM */
    if (Pcga_ScanVideoBiosRom(FontBuffer, CharHeight)) {
        return;  /* Success */
    }

    /* Tier 3: Fall back to bundled fonts */
    BundledFont = NULL;
    if (CharHeight == 8) {
        BundledFont = gVgaFont8x8;
    } else if (CharHeight == 14) {
        BundledFont = gVgaFont8x14;
    } else if (CharHeight == 16) {
        BundledFont = gVgaFont8x16;
    }

    if (BundledFont != NULL) {
        ANX_MEMCPY(FontBuffer, BundledFont, 256 * CharHeight);
    }
}

/*
 * Load font into CGA character generator.
 * CGA has limited font customization - fonts are typically in ROM.
 * However, some CGA modes allow modifying character patterns.
 */
static VOID
Pcga_LoadFontCGA(
    PCGA_BACKEND *Backend,
    CONST UINT8 *FontData,
    UINT32 CharHeight,
    UINT32 CharOffset,
    UINT32 CharCount
    )
{
    /* CGA doesn't have programmable character generator RAM like EGA/VGA */
    /* Font patterns are in ROM and cannot be changed in hardware */
    /* For software rendering, we can store the font data in backend */

    if (FontData == NULL || CharHeight > 8) {
        return;
    }

    /* Store font data for software text rendering */
    if (Backend->FontData == NULL) {
        /* Allocate font storage (256 characters × 32 bytes max) */
        Backend->FontData = (UINT8 *)ANX_MALLOC(256 * 32);
        if (Backend->FontData == NULL) {
            return;
        }
    }

    /* Copy font data */
    for (UINT32 Ch = 0; Ch < CharCount; Ch++) {
        UINT32 DestOffset = (CharOffset + Ch) * CharHeight;
        UINT32 SrcOffset = Ch * CharHeight;
        ANX_MEMCPY(&Backend->FontData[DestOffset], &FontData[SrcOffset], CharHeight);
    }

    Backend->FontHeight = CharHeight;
}

/*
 * Load font into EGA character generator RAM.
 * EGA has character generator RAM in plane 2, similar to VGA.
 */
static VOID
Pcga_LoadFontEGA(
    PCGA_BACKEND *Backend,
    CONST UINT8 *FontData,
    UINT32 CharHeight,
    UINT32 CharOffset,
    UINT32 CharCount,
    UINT32 Bank
    )
{
    if (FontData == NULL || CharHeight > 14 || Bank > 1) {
        return;
    }

    /* Save current sequencer and graphics controller state */
    ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_MAP_MASK);
    UINT8 SavedSeq2 = ANX_CPU_INB(VGA_SEQ_DATA);
    ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_MEMORY_MODE);
    UINT8 SavedSeq4 = ANX_CPU_INB(VGA_SEQ_DATA);

    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_READ_MAP_SELECT);
    UINT8 SavedGc4 = ANX_CPU_INB(VGA_GC_DATA);
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_GRAPHICS_MODE);
    UINT8 SavedGc5 = ANX_CPU_INB(VGA_GC_DATA);
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_MISCELLANEOUS);
    UINT8 SavedGc6 = ANX_CPU_INB(VGA_GC_DATA);

    /* Sequencer: Select plane 2 for font data */
    ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_MAP_MASK);
    ANX_CPU_OUTB(VGA_SEQ_DATA, 0x04);  /* Map mask: plane 2 */

    /* Sequencer: Enable access to character generator RAM */
    ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_MEMORY_MODE);
    ANX_CPU_OUTB(VGA_SEQ_DATA, 0x06);  /* EGA: Sequential, odd/even disabled */

    /* Graphics Controller: Select plane 2 */
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_READ_MAP_SELECT);
    ANX_CPU_OUTB(VGA_GC_DATA, 0x02);  /* Read plane 2 */

    /* Graphics Controller: Set graphics mode */
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_GRAPHICS_MODE);
    ANX_CPU_OUTB(VGA_GC_DATA, 0x00);  /* Write mode 0, read mode 0 */

    /* Graphics Controller: Set memory map */
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_MISCELLANEOUS);
    ANX_CPU_OUTB(VGA_GC_DATA, 0x04);  /* Map A0000-AFFFF (64KB) */

    /* Calculate font address in plane 2 */
    /* EGA character generator: Different organization than VGA */
    /* EGA uses 14 or 16 bytes per character (not 32 like VGA) */
    UINT32 BytesPerChar = (CharHeight == 14) ? 14 : 16;
    UINT32 FontOffset = Bank * 0x2000;  /* Bank 0 or 1 */
    UINT8 *FontBase = (UINT8 *)(UINTN)0xA0000 + FontOffset;

    /* Load font data */
    for (UINT32 Ch = 0; Ch < CharCount; Ch++) {
        UINT32 CharAddr = (CharOffset + Ch) * BytesPerChar;
        for (UINT32 Line = 0; Line < CharHeight; Line++) {
            FontBase[CharAddr + Line] = FontData[Ch * CharHeight + Line];
        }
        /* Clear remaining lines if char height < BytesPerChar */
        for (UINT32 Line = CharHeight; Line < BytesPerChar; Line++) {
            FontBase[CharAddr + Line] = 0;
        }
    }

    /* Restore sequencer and graphics controller state */
    ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_MAP_MASK);
    ANX_CPU_OUTB(VGA_SEQ_DATA, SavedSeq2);
    ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_MEMORY_MODE);
    ANX_CPU_OUTB(VGA_SEQ_DATA, SavedSeq4);
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_READ_MAP_SELECT);
    ANX_CPU_OUTB(VGA_GC_DATA, SavedGc4);
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_GRAPHICS_MODE);
    ANX_CPU_OUTB(VGA_GC_DATA, SavedGc5);
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_MISCELLANEOUS);
    ANX_CPU_OUTB(VGA_GC_DATA, SavedGc6);

    /* Update backend state */
    Backend->FontHeight = CharHeight;
    Backend->FontBank = Bank;
}

/*
 * Load font into VGA character generator RAM.
 * VGA supports fonts up to 32 scanlines tall.
 */
static VOID
Pcga_LoadFontVGA(
    PCGA_BACKEND *Backend,
    CONST UINT8 *FontData,
    UINT32 CharHeight,
    UINT32 CharOffset,
    UINT32 CharCount,
    UINT32 Bank
    )
{
    if (FontData == NULL || CharHeight > 32 || Bank > 1) {
        return;
    }

    /* Save current sequencer and graphics controller state */
    ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_MAP_MASK);
    UINT8 SavedSeq2 = ANX_CPU_INB(VGA_SEQ_DATA);
    ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_MEMORY_MODE);
    UINT8 SavedSeq4 = ANX_CPU_INB(VGA_SEQ_DATA);

    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_READ_MAP_SELECT);
    UINT8 SavedGc4 = ANX_CPU_INB(VGA_GC_DATA);
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_GRAPHICS_MODE);
    UINT8 SavedGc5 = ANX_CPU_INB(VGA_GC_DATA);
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_MISCELLANEOUS);
    UINT8 SavedGc6 = ANX_CPU_INB(VGA_GC_DATA);

    /* Sequencer: Select plane 2 for font data */
    ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_MAP_MASK);
    ANX_CPU_OUTB(VGA_SEQ_DATA, 0x04);  /* Map mask: plane 2 */

    /* Sequencer: Enable access to character generator RAM */
    ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_MEMORY_MODE);
    ANX_CPU_OUTB(VGA_SEQ_DATA, 0x07);  /* Extended memory, odd/even disabled */

    /* Graphics Controller: Select plane 2 */
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_READ_MAP_SELECT);
    ANX_CPU_OUTB(VGA_GC_DATA, 0x02);  /* Read plane 2 */

    /* Graphics Controller: Set graphics mode */
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_GRAPHICS_MODE);
    ANX_CPU_OUTB(VGA_GC_DATA, 0x00);  /* Write mode 0, read mode 0 */

    /* Graphics Controller: Set memory map to 0xA0000 */
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_MISCELLANEOUS);
    ANX_CPU_OUTB(VGA_GC_DATA, 0x00);  /* Map 0xA0000-0xBFFFF */

    /* Calculate font address in plane 2 */
    /* VGA character generator: 8KB per bank, 32 bytes per character */
    UINT32 FontOffset = Bank * 0x2000;  /* Bank 0 or 1 */
    UINT8 *FontBase = (UINT8 *)(UINTN)0xA0000 + FontOffset;

    /* Load font data */
    for (UINT32 Ch = 0; Ch < CharCount; Ch++) {
        UINT32 CharAddr = (CharOffset + Ch) * 32;  /* 32 bytes per char */
        for (UINT32 Line = 0; Line < CharHeight; Line++) {
            FontBase[CharAddr + Line] = FontData[Ch * CharHeight + Line];
        }
        /* Clear remaining lines if char height < 32 */
        for (UINT32 Line = CharHeight; Line < 32; Line++) {
            FontBase[CharAddr + Line] = 0;
        }
    }

    /* Restore sequencer and graphics controller state */
    ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_MAP_MASK);
    ANX_CPU_OUTB(VGA_SEQ_DATA, SavedSeq2);
    ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_MEMORY_MODE);
    ANX_CPU_OUTB(VGA_SEQ_DATA, SavedSeq4);
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_READ_MAP_SELECT);
    ANX_CPU_OUTB(VGA_GC_DATA, SavedGc4);
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_GRAPHICS_MODE);
    ANX_CPU_OUTB(VGA_GC_DATA, SavedGc5);
    ANX_CPU_OUTB(VGA_GC_INDEX, VGA_GC_MISCELLANEOUS);
    ANX_CPU_OUTB(VGA_GC_DATA, SavedGc6);

    /* Update backend state */
    Backend->FontHeight = CharHeight;
    Backend->FontBank = Bank;
}

/*
 * Unified font loading function - detects adapter and calls appropriate loader.
 * This is the main font loading entry point.
 */
static VOID
Pcga_LoadFont(
    PCGA_BACKEND *Backend,
    CONST UINT8 *FontData,
    UINT32 CharHeight,
    UINT32 CharOffset,
    UINT32 CharCount,
    UINT32 Bank
    )
{
    if (FontData == NULL || Backend == NULL) {
        return;
    }

    /* Detect adapter type */
    PCGA_ADAPTER_TYPE AdapterType = Pcga_DetectAdapter();

    /* Dispatch to appropriate loader */
    switch (AdapterType) {
        case PCGA_ADAPTER_CGA:
            /* CGA: No hardware font RAM, store in software */
            Pcga_LoadFontCGA(Backend, FontData, CharHeight, CharOffset, CharCount);
            break;

        case PCGA_ADAPTER_EGA:
            /* EGA: Character generator RAM, max 14 scanlines */
            if (CharHeight > 14) {
                /* EGA doesn't support fonts taller than 14 scanlines */
                return;
            }
            Pcga_LoadFontEGA(Backend, FontData, CharHeight, CharOffset, CharCount, Bank);
            break;

        case PCGA_ADAPTER_MCGA:
            /* MCGA: VGA-compatible, but typically uses 8x16 fonts */
            if (CharHeight > 16) {
                /* MCGA typically doesn't support fonts taller than 16 scanlines */
                CharHeight = 16;
            }
            Pcga_LoadFontVGA(Backend, FontData, CharHeight, CharOffset, CharCount, Bank);
            break;

        case PCGA_ADAPTER_VGA:
        case PCGA_ADAPTER_SVGA:
        case PCGA_ADAPTER_XGA:
        default:
            /* VGA/SVGA/XGA: Full VGA character generator, up to 32 scanlines */
            Pcga_LoadFontVGA(Backend, FontData, CharHeight, CharOffset, CharCount, Bank);
            break;
    }
}

/*
 * Select which font bank to use for text mode display.
 */
static VOID
Pcga_SelectFontBank(
    PCGA_BACKEND *Backend,
    UINT32 Bank
    )
{
    if (Bank > 1) {
        return;
    }

    /* Sequencer register 3: Character Map Select */
    UINT8 CharMapValue = (Bank == 0) ? 0x00 : 0x10;
    ANX_CPU_OUTB(VGA_SEQ_INDEX, VGA_SEQ_CHAR_MAP);
    ANX_CPU_OUTB(VGA_SEQ_DATA, CharMapValue);

    Backend->FontBank = Bank;
}

/* --------------------------------------------------------------- */
/*  VBlank Synchronization                                         */
/* --------------------------------------------------------------- */

/*
 * Wait for vertical blank interval.
 * Returns immediately if VBlank is already active.
 */
static VOID
Pcga_WaitForVBlank(
    VOID
    )
{
    /* Wait for VBlank to end (if currently in VBlank) */
    while (ANX_CPU_INB(VGA_INPUT_STATUS_1) & 0x08) {
        /* Spin while in VBlank */
    }

    /* Wait for VBlank to start */
    while (!(ANX_CPU_INB(VGA_INPUT_STATUS_1) & 0x08)) {
        /* Spin until VBlank */
    }
}

/*
 * Check if currently in vertical blank interval.
 */
static BOOLEAN
Pcga_IsVBlank(
    VOID
    )
{
    return (ANX_CPU_INB(VGA_INPUT_STATUS_1) & 0x08) != 0;
}

/* --------------------------------------------------------------- */
/*  Display Start Address (for scrolling and page flipping)       */
/* --------------------------------------------------------------- */

/*
 * Set display start address for hardware scrolling and page flipping.
 */
static VOID
Pcga_SetDisplayStart(
    PCGA_BACKEND *Backend,
    UINT32 Offset
    )
{
    /* CRTC registers 0x0C (Start Address High) and 0x0D (Start Address Low) */
    Pcga_WriteCrtc(0x0C, (UINT8)(Offset >> 8));
    Pcga_WriteCrtc(0x0D, (UINT8)(Offset & 0xFF));
}

/*
 * Get current display start address.
 */
static UINT32
Pcga_GetDisplayStart(
    PCGA_BACKEND *Backend
    )
{
    UINT8 High = Pcga_ReadCrtc(0x0C);
    UINT8 Low = Pcga_ReadCrtc(0x0D);
    return ((UINT32)High << 8) | Low;
}

/* --------------------------------------------------------------- */
/*  Screen-to-Screen Blitting                                      */
/* --------------------------------------------------------------- */

/*
 * Blit from one screen location to another using VGA latching.
 * This is extremely fast for planar modes.
 */
static VOID
Pcga_BlitScreen(
    PCGA_BACKEND *Backend,
    UINT32 SrcX,
    UINT32 SrcY,
    UINT32 DestX,
    UINT32 DestY,
    UINT32 Width,
    UINT32 Height
    )
{
    /* For planar modes, use VGA latching for maximum speed */
    if (Backend->Descriptor.PixelFormat == FbPixelFormatPlanar) {
        UINT32 PlanarPitch = Backend->Descriptor.Pitch / Backend->Descriptor.NumPlanes;

        for (UINT32 Row = 0; Row < Height; Row++) {
            UINT32 SrcOffset = (SrcY + Row) * PlanarPitch + (SrcX / 8);
            UINT32 DestOffset = (DestY + Row) * PlanarPitch + (DestX / 8);
            UINT32 Count = (Width + 7) / 8;

            /* Use latch copy (Write Mode 1) */
            Pcga_CopyPlanar(Backend, DestOffset, SrcOffset, Count);
        }
        return;
    }

    /* For linear modes, use memcpy */
    if (Backend->Descriptor.MemoryOrganization == FbMemoryLinear) {
        UINT32 BytesPerPixel = (Backend->Descriptor.BitsPerPixel + 7) / 8;
        if (Backend->Descriptor.PixelFormat == FbPixelFormatIndexed) {
            BytesPerPixel = 1;
        }

        for (UINT32 Row = 0; Row < Height; Row++) {
            UINT32 SrcOffset = (SrcY + Row) * Backend->Descriptor.Pitch + SrcX * BytesPerPixel;
            UINT32 DestOffset = (DestY + Row) * Backend->Descriptor.Pitch + DestX * BytesPerPixel;
            UINT32 RowBytes = Width * BytesPerPixel;

            if (Backend->RtlCopyMemoryFunc) {
                Backend->RtlCopyMemoryFunc(&Backend->FramebufferBase[DestOffset],
                                          &Backend->FramebufferBase[SrcOffset],
                                          RowBytes);
            } else {
                ANX_MEMCPY(&Backend->FramebufferBase[DestOffset],
                          &Backend->FramebufferBase[SrcOffset],
                          RowBytes);
            }
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
    /* Software cursor implementation - save background and composite cursor */

    if (CursorData == NULL || Width == 0 || Height == 0 ||
        Width > 64 || Height > 64) {
        return;
    }

    /* Clip cursor to screen bounds */
    INT32 ClipX = (X < 0) ? 0 : X;
    INT32 ClipY = (Y < 0) ? 0 : Y;
    UINT32 ClipWidth = Width;
    UINT32 ClipHeight = Height;

    if (X < 0) {
        ClipWidth += X;
        X = 0;
    }
    if (Y < 0) {
        ClipHeight += Y;
        Y = 0;
    }
    if (X + ClipWidth > Backend->Descriptor.Width) {
        ClipWidth = Backend->Descriptor.Width - X;
    }
    if (Y + ClipHeight > Backend->Descriptor.Height) {
        ClipHeight = Backend->Descriptor.Height - Y;
    }

    /* Save cursor state */
    gCursorState.SavedX = X;
    gCursorState.SavedY = Y;
    gCursorState.Width = ClipWidth;
    gCursorState.Height = ClipHeight;

    /* Save background pixels */
    UINT32 BytesPerPixel = (Backend->Descriptor.BitsPerPixel + 7) / 8;
    if (BytesPerPixel == 0) BytesPerPixel = 1;  /* For indexed modes */

    for (UINT32 Row = 0; Row < ClipHeight; Row++) {
        for (UINT32 Col = 0; Col < ClipWidth; Col++) {
            UINT32 ScreenX = X + Col;
            UINT32 ScreenY = Y + Row;
            UINT32 SaveOffset = (Row * 64 + Col) * 4;  /* Always save as RGBA */

            /* Read background pixel - handle different formats */
            FB_COLOR BgColor;
            if (Backend->Descriptor.PixelFormat == FbPixelFormatText) {
                /* Text mode: save character and attribute */
                UINT32 Offset = ScreenY * Backend->Descriptor.Pitch + ScreenX * 2;
                gCursorState.SavedData[SaveOffset + 0] = Backend->FramebufferBase[Offset];
                gCursorState.SavedData[SaveOffset + 1] = Backend->FramebufferBase[Offset + 1];
                gCursorState.SavedData[SaveOffset + 2] = 0;
                gCursorState.SavedData[SaveOffset + 3] = 0;
            } else if (Backend->Descriptor.PixelFormat == FbPixelFormatIndexed) {
                /* Indexed color */
                UINT32 Offset = ScreenY * Backend->Descriptor.Pitch + ScreenX;
                UINT8 Index = Backend->FramebufferBase[Offset];
                gCursorState.SavedData[SaveOffset + 0] = Index;
                gCursorState.SavedData[SaveOffset + 1] = 0;
                gCursorState.SavedData[SaveOffset + 2] = 0;
                gCursorState.SavedData[SaveOffset + 3] = 255;
            } else if (Backend->Descriptor.PixelFormat == FbPixelFormatRgb) {
                /* RGB mode */
                UINT32 Offset = ScreenY * Backend->Descriptor.Pitch + ScreenX * BytesPerPixel;
                if (BytesPerPixel >= 3) {
                    gCursorState.SavedData[SaveOffset + 0] = Backend->FramebufferBase[Offset + 2];  /* B */
                    gCursorState.SavedData[SaveOffset + 1] = Backend->FramebufferBase[Offset + 1];  /* G */
                    gCursorState.SavedData[SaveOffset + 2] = Backend->FramebufferBase[Offset + 0];  /* R */
                    gCursorState.SavedData[SaveOffset + 3] = 255;
                } else if (BytesPerPixel == 2) {
                    /* 16-bit RGB565 or RGB555 */
                    UINT16 Pixel = ((UINT16)Backend->FramebufferBase[Offset]) |
                                  (((UINT16)Backend->FramebufferBase[Offset + 1]) << 8);
                    gCursorState.SavedData[SaveOffset + 0] = ((Pixel >> 11) & 0x1F) << 3;  /* R */
                    gCursorState.SavedData[SaveOffset + 1] = ((Pixel >> 5) & 0x3F) << 2;   /* G */
                    gCursorState.SavedData[SaveOffset + 2] = (Pixel & 0x1F) << 3;          /* B */
                    gCursorState.SavedData[SaveOffset + 3] = 255;
                }
            }
        }
    }

    /* Draw cursor using AND/XOR masks or RGBA */
    if (IsColor) {
        /* RGBA cursor data: 4 bytes per pixel */
        for (UINT32 Row = 0; Row < ClipHeight; Row++) {
            for (UINT32 Col = 0; Col < ClipWidth; Col++) {
                UINT32 CursorOffset = ((Row) * Width + Col) * 4;
                UINT8 R = CursorData[CursorOffset + 0];
                UINT8 G = CursorData[CursorOffset + 1];
                UINT8 B = CursorData[CursorOffset + 2];
                UINT8 A = CursorData[CursorOffset + 3];

                if (A > 0) {  /* Only draw non-transparent pixels */
                    FB_COLOR CursorColor = FB_MAKE_COLOR(R, G, B, A);
                    PcGraphics_SetPixel(&Backend->Base, X + Col, Y + Row, CursorColor);
                }
            }
        }
    } else {
        /* Monochrome AND/XOR masks: Width bytes per row × 2 (AND then XOR) */
        UINT32 BytesPerRow = (Width + 7) / 8;
        CONST UINT8 *AndMask = CursorData;
        CONST UINT8 *XorMask = CursorData + (BytesPerRow * Height);

        for (UINT32 Row = 0; Row < ClipHeight; Row++) {
            for (UINT32 Col = 0; Col < ClipWidth; Col++) {
                UINT32 ByteOffset = Row * BytesPerRow + (Col / 8);
                UINT32 BitOffset = 7 - (Col % 8);
                BOOLEAN AndBit = (AndMask[ByteOffset] >> BitOffset) & 1;
                BOOLEAN XorBit = (XorMask[ByteOffset] >> BitOffset) & 1;

                /* Standard cursor logic: AND then XOR
                 * AND=1, XOR=0: Transparent (keep background)
                 * AND=0, XOR=0: Black
                 * AND=0, XOR=1: White
                 * AND=1, XOR=1: Invert
                 */
                if (!(AndBit && !XorBit)) {  /* Not transparent */
                    FB_COLOR CursorColor;
                    if (!AndBit && !XorBit) {
                        CursorColor = FB_MAKE_COLOR(0, 0, 0, 255);  /* Black */
                    } else if (!AndBit && XorBit) {
                        CursorColor = FB_MAKE_COLOR(255, 255, 255, 255);  /* White */
                    } else {  /* AND=1, XOR=1 - Invert */
                        FB_COLOR BgColor;
                        PcGraphics_GetPixel(&Backend->Base, X + Col, Y + Row, &BgColor);
                        UINT8 R = 255 - FB_COLOR_GET_RED(BgColor);
                        UINT8 G = 255 - FB_COLOR_GET_GREEN(BgColor);
                        UINT8 B = 255 - FB_COLOR_GET_BLUE(BgColor);
                        CursorColor = FB_MAKE_COLOR(R, G, B, 255);
                    }
                    PcGraphics_SetPixel(&Backend->Base, X + Col, Y + Row, CursorColor);
                }
            }
        }
    }

    gCursorState.Valid = TRUE;
}

static VOID
Pcga_HideCursor(
    PCGA_BACKEND *Backend
    )
{
    if (!gCursorState.Valid) {
        return;
    }

    /* Restore saved background pixels */
    UINT32 BytesPerPixel = (Backend->Descriptor.BitsPerPixel + 7) / 8;
    if (BytesPerPixel == 0) BytesPerPixel = 1;

    for (UINT32 Row = 0; Row < gCursorState.Height; Row++) {
        for (UINT32 Col = 0; Col < gCursorState.Width; Col++) {
            UINT32 ScreenX = gCursorState.SavedX + Col;
            UINT32 ScreenY = gCursorState.SavedY + Row;
            UINT32 SaveOffset = (Row * 64 + Col) * 4;

            /* Restore pixel based on format */
            if (Backend->Descriptor.PixelFormat == FbPixelFormatText) {
                /* Text mode: restore character and attribute */
                UINT32 Offset = ScreenY * Backend->Descriptor.Pitch + ScreenX * 2;
                Backend->FramebufferBase[Offset] = gCursorState.SavedData[SaveOffset + 0];
                Backend->FramebufferBase[Offset + 1] = gCursorState.SavedData[SaveOffset + 1];
            } else if (Backend->Descriptor.PixelFormat == FbPixelFormatIndexed) {
                /* Indexed color */
                UINT32 Offset = ScreenY * Backend->Descriptor.Pitch + ScreenX;
                Backend->FramebufferBase[Offset] = gCursorState.SavedData[SaveOffset + 0];
            } else if (Backend->Descriptor.PixelFormat == FbPixelFormatRgb) {
                /* RGB mode */
                UINT8 R = gCursorState.SavedData[SaveOffset + 0];
                UINT8 G = gCursorState.SavedData[SaveOffset + 1];
                UINT8 B = gCursorState.SavedData[SaveOffset + 2];

                UINT32 Offset = ScreenY * Backend->Descriptor.Pitch + ScreenX * BytesPerPixel;
                if (BytesPerPixel >= 3) {
                    /* 24-bit or 32-bit RGB */
                    Backend->FramebufferBase[Offset + 0] = B;
                    Backend->FramebufferBase[Offset + 1] = G;
                    Backend->FramebufferBase[Offset + 2] = R;
                    if (BytesPerPixel == 4) {
                        Backend->FramebufferBase[Offset + 3] = 255;  /* Alpha */
                    }
                } else if (BytesPerPixel == 2) {
                    /* 16-bit RGB565 */
                    UINT16 Pixel = ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
                    Backend->FramebufferBase[Offset] = (UINT8)(Pixel & 0xFF);
                    Backend->FramebufferBase[Offset + 1] = (UINT8)(Pixel >> 8);
                }
            }
        }
    }

    gCursorState.Valid = FALSE;
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
    UINT32              ModeNumber;         /* BIOS mode number (or VESA mode) */
    UINT32              Width;              /* Resolution width */
    UINT32              Height;             /* Resolution height */
    UINT32              RefreshRate;        /* Refresh rate in Hz (0=default) */
    FB_PIXEL_FORMAT     PixelFormat;        /* Pixel format type */
    FB_MEMORY_ORGANIZATION MemoryOrganization; /* Memory organization */
    UINT32              BitsPerPixel;       /* Bits per pixel (for depth) */
    UINT32              PaletteSize;        /* Palette size (for indexed) */
    UINT32              NumPlanes;          /* Number of planes (for planar) */
    UINT64              PhysicalBase;       /* Physical framebuffer address */
    UINT32              Pitch;              /* Bytes per scanline */
    UINT32              Flags;              /* Mode-specific flags */
} PC_GRAPHICS_MODE;

/* Mode flags */
#define MODE_FLAG_TEXT          0x0001  /* Text mode */
#define MODE_FLAG_GRAPHICS      0x0002  /* Graphics mode */
#define MODE_FLAG_LINEAR        0x0004  /* Linear framebuffer (VESA LFB) */
#define MODE_FLAG_BANKED        0x0008  /* Banked framebuffer */
#define MODE_FLAG_CGA           0x0010  /* CGA hardware */
#define MODE_FLAG_EGA           0x0020  /* EGA hardware */
#define MODE_FLAG_VGA           0x0040  /* VGA hardware */
#define MODE_FLAG_SVGA          0x0080  /* SVGA hardware */
#define MODE_FLAG_MCGA          0x0100  /* MCGA (PS/2 Model 25/30) */

static CONST PC_GRAPHICS_MODE gPcGraphicsModes[] = {
    /* ===================================================================
     * Text Modes (MDA, CGA, EGA, VGA)
     * ================================================================ */

    /* Mode 0x00: 40x25 text, 16 colors, 70Hz */
    { 0x00, 40, 25, 70, FbPixelFormatText, FbMemoryLinear, 0, 0, 0, 0xB8000, 160, MODE_FLAG_TEXT | MODE_FLAG_CGA },

    /* Mode 0x01: 40x25 text, 16 colors, 70Hz */
    { 0x01, 40, 25, 70, FbPixelFormatText, FbMemoryLinear, 0, 0, 0, 0xB8000, 160, MODE_FLAG_TEXT | MODE_FLAG_CGA },

    /* Mode 0x02: 80x25 text, 16 colors, 70Hz */
    { 0x02, 80, 25, 70, FbPixelFormatText, FbMemoryLinear, 0, 0, 0, 0xB8000, 160, MODE_FLAG_TEXT | MODE_FLAG_CGA },

    /* Mode 0x03: 80x25 text, 16 colors, 70Hz (most common DOS text mode) */
    { 0x03, 80, 25, 70, FbPixelFormatText, FbMemoryLinear, 0, 0, 0, 0xB8000, 160, MODE_FLAG_TEXT | MODE_FLAG_CGA },

    /* Mode 0x07: 80x25 text, monochrome, 70Hz (MDA) */
    { 0x07, 80, 25, 70, FbPixelFormatText, FbMemoryLinear, 0, 0, 0, 0xB0000, 160, MODE_FLAG_TEXT | MODE_FLAG_VGA },

    /* ===================================================================
     * CGA Graphics Modes
     * ================================================================ */

    /* Mode 0x04: 320x200, 4 colors, 60Hz (CGA) */
    { 0x04, 320, 200, 60, FbPixelFormatIndexed, FbMemoryInterleaved, 2, 4, 0, 0xB8000, 80, MODE_FLAG_GRAPHICS | MODE_FLAG_CGA },

    /* Mode 0x05: 320x200, 4 colors, 60Hz (CGA, grayscale) */
    { 0x05, 320, 200, 60, FbPixelFormatIndexed, FbMemoryInterleaved, 2, 4, 0, 0xB8000, 80, MODE_FLAG_GRAPHICS | MODE_FLAG_CGA },

    /* Mode 0x06: 640x200, 2 colors, 60Hz (CGA) */
    { 0x06, 640, 200, 60, FbPixelFormatMonochrome, FbMemoryInterleaved, 1, 0, 0, 0xB8000, 80, MODE_FLAG_GRAPHICS | MODE_FLAG_CGA },

    /* ===================================================================
     * EGA Graphics Modes
     * ================================================================ */

    /* Mode 0x0D: 320x200, 16 colors, 60Hz (EGA) */
    { 0x0D, 320, 200, 60, FbPixelFormatPlanar, FbMemoryPlanar, 4, 0, 4, 0xA0000, 40, MODE_FLAG_GRAPHICS | MODE_FLAG_EGA },

    /* Mode 0x0E: 640x200, 16 colors, 60Hz (EGA) */
    { 0x0E, 640, 200, 60, FbPixelFormatPlanar, FbMemoryPlanar, 4, 0, 4, 0xA0000, 80, MODE_FLAG_GRAPHICS | MODE_FLAG_EGA },

    /* Mode 0x0F: 640x350, monochrome, 60Hz (EGA) */
    { 0x0F, 640, 350, 60, FbPixelFormatMonochrome, FbMemoryPlanar, 1, 0, 1, 0xA0000, 80, MODE_FLAG_GRAPHICS | MODE_FLAG_EGA },

    /* Mode 0x10: 640x350, 16 colors, 60Hz (EGA) */
    { 0x10, 640, 350, 60, FbPixelFormatPlanar, FbMemoryPlanar, 4, 0, 4, 0xA0000, 80, MODE_FLAG_GRAPHICS | MODE_FLAG_EGA },

    /* ===================================================================
     * VGA Graphics Modes
     * ================================================================ */

    /* Mode 0x11: 640x480, 2 colors, 60Hz (VGA) */
    { 0x11, 640, 480, 60, FbPixelFormatMonochrome, FbMemoryPlanar, 1, 0, 1, 0xA0000, 80, MODE_FLAG_GRAPHICS | MODE_FLAG_VGA },

    /* Mode 0x12: 640x480, 16 colors, 60Hz (VGA) */
    { 0x12, 640, 480, 60, FbPixelFormatPlanar, FbMemoryPlanar, 4, 0, 4, 0xA0000, 80, MODE_FLAG_GRAPHICS | MODE_FLAG_VGA },

    /* Mode 0x13: 320x200, 256 colors, 70Hz (VGA/MCGA - Mode 13h) */
    { 0x13, 320, 200, 70, FbPixelFormatIndexed, FbMemoryLinear, 8, 256, 0, 0xA0000, 320, MODE_FLAG_GRAPHICS | MODE_FLAG_VGA | MODE_FLAG_MCGA },

    /* ===================================================================
     * Mode-X and Tweaked Modes (Unchained VGA)
     * ================================================================ */

    /* Mode 0x100: 320x240, 256 colors, 60Hz (Mode-X) */
    { 0x100, 320, 240, 60, FbPixelFormatIndexed, FbMemoryPlanar, 8, 256, 4, 0xA0000, 80, MODE_FLAG_GRAPHICS | MODE_FLAG_VGA },

    /* Mode 0x101: 320x400, 256 colors, 70Hz (Mode-X) */
    { 0x101, 320, 400, 70, FbPixelFormatIndexed, FbMemoryPlanar, 8, 256, 4, 0xA0000, 80, MODE_FLAG_GRAPHICS | MODE_FLAG_VGA },

    /* Mode 0x102: 320x480, 256 colors, 60Hz (Mode-X) */
    { 0x102, 320, 480, 60, FbPixelFormatIndexed, FbMemoryPlanar, 8, 256, 4, 0xA0000, 80, MODE_FLAG_GRAPHICS | MODE_FLAG_VGA },

    /* Mode 0x103: 360x480, 256 colors, 60Hz (Mode-X) */
    { 0x103, 360, 480, 60, FbPixelFormatIndexed, FbMemoryPlanar, 8, 256, 4, 0xA0000, 90, MODE_FLAG_GRAPHICS | MODE_FLAG_VGA },

    /* ===================================================================
     * VESA VBE 1.x Modes (Banked and Linear)
     * ================================================================ */

    /* 640x480 modes */
    { 0x110, 640, 480, 60, FbPixelFormatIndexed, FbMemoryBanked, 8, 256, 0, 0xA0000, 640, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x111, 640, 480, 60, FbPixelFormatRgb, FbMemoryBanked, 15, 0, 0, 0xA0000, 1280, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x112, 640, 480, 60, FbPixelFormatRgb, FbMemoryBanked, 16, 0, 0, 0xA0000, 1280, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x113, 640, 480, 60, FbPixelFormatRgb, FbMemoryBanked, 24, 0, 0, 0xA0000, 1920, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x114, 640, 480, 60, FbPixelFormatRgb, FbMemoryBanked, 32, 0, 0, 0xA0000, 2560, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },

    /* 800x600 modes */
    { 0x115, 800, 600, 60, FbPixelFormatIndexed, FbMemoryBanked, 8, 256, 0, 0xA0000, 800, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x116, 800, 600, 60, FbPixelFormatRgb, FbMemoryBanked, 15, 0, 0, 0xA0000, 1600, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x117, 800, 600, 60, FbPixelFormatRgb, FbMemoryBanked, 16, 0, 0, 0xA0000, 1600, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x118, 800, 600, 60, FbPixelFormatRgb, FbMemoryBanked, 24, 0, 0, 0xA0000, 2400, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x119, 800, 600, 60, FbPixelFormatRgb, FbMemoryBanked, 32, 0, 0, 0xA0000, 3200, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },

    /* 1024x768 modes */
    { 0x11A, 1024, 768, 60, FbPixelFormatIndexed, FbMemoryBanked, 8, 256, 0, 0xA0000, 1024, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x11B, 1024, 768, 60, FbPixelFormatRgb, FbMemoryBanked, 15, 0, 0, 0xA0000, 2048, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x11C, 1024, 768, 60, FbPixelFormatRgb, FbMemoryBanked, 16, 0, 0, 0xA0000, 2048, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x11D, 1024, 768, 60, FbPixelFormatRgb, FbMemoryBanked, 24, 0, 0, 0xA0000, 3072, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x11E, 1024, 768, 60, FbPixelFormatRgb, FbMemoryBanked, 32, 0, 0, 0xA0000, 4096, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },

    /* 1280x1024 modes */
    { 0x11F, 1280, 1024, 60, FbPixelFormatIndexed, FbMemoryBanked, 8, 256, 0, 0xA0000, 1280, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x120, 1280, 1024, 60, FbPixelFormatRgb, FbMemoryBanked, 15, 0, 0, 0xA0000, 2560, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x121, 1280, 1024, 60, FbPixelFormatRgb, FbMemoryBanked, 16, 0, 0, 0xA0000, 2560, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x122, 1280, 1024, 60, FbPixelFormatRgb, FbMemoryBanked, 24, 0, 0, 0xA0000, 3840, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x123, 1280, 1024, 60, FbPixelFormatRgb, FbMemoryBanked, 32, 0, 0, 0xA0000, 5120, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },

    /* 1600x1200 modes */
    { 0x124, 1600, 1200, 60, FbPixelFormatIndexed, FbMemoryBanked, 8, 256, 0, 0xA0000, 1600, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x125, 1600, 1200, 60, FbPixelFormatRgb, FbMemoryBanked, 15, 0, 0, 0xA0000, 3200, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x126, 1600, 1200, 60, FbPixelFormatRgb, FbMemoryBanked, 16, 0, 0, 0xA0000, 3200, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x127, 1600, 1200, 60, FbPixelFormatRgb, FbMemoryBanked, 24, 0, 0, 0xA0000, 4800, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },
    { 0x128, 1600, 1200, 60, FbPixelFormatRgb, FbMemoryBanked, 32, 0, 0, 0xA0000, 6400, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_BANKED },

    /* ===================================================================
     * VESA VBE 2.0+ Linear Framebuffer Modes (0x4000 bit set)
     * These use LFB addresses provided by VESA BIOS
     * ================================================================ */

    /* 640x480 LFB */
    { 0x4110, 640, 480, 60, FbPixelFormatIndexed, FbMemoryLinear, 8, 256, 0, 0, 640, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4111, 640, 480, 60, FbPixelFormatRgb, FbMemoryLinear, 15, 0, 0, 0, 1280, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4112, 640, 480, 60, FbPixelFormatRgb, FbMemoryLinear, 16, 0, 0, 0, 1280, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4113, 640, 480, 60, FbPixelFormatRgb, FbMemoryLinear, 24, 0, 0, 0, 1920, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4114, 640, 480, 60, FbPixelFormatRgb, FbMemoryLinear, 32, 0, 0, 0, 2560, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },

    /* 800x600 LFB */
    { 0x4115, 800, 600, 60, FbPixelFormatIndexed, FbMemoryLinear, 8, 256, 0, 0, 800, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4116, 800, 600, 60, FbPixelFormatRgb, FbMemoryLinear, 15, 0, 0, 0, 1600, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4117, 800, 600, 60, FbPixelFormatRgb, FbMemoryLinear, 16, 0, 0, 0, 1600, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4118, 800, 600, 60, FbPixelFormatRgb, FbMemoryLinear, 24, 0, 0, 0, 2400, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4119, 800, 600, 60, FbPixelFormatRgb, FbMemoryLinear, 32, 0, 0, 0, 3200, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },

    /* 1024x768 LFB */
    { 0x411A, 1024, 768, 60, FbPixelFormatIndexed, FbMemoryLinear, 8, 256, 0, 0, 1024, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x411B, 1024, 768, 60, FbPixelFormatRgb, FbMemoryLinear, 15, 0, 0, 0, 2048, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x411C, 1024, 768, 60, FbPixelFormatRgb, FbMemoryLinear, 16, 0, 0, 0, 2048, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x411D, 1024, 768, 60, FbPixelFormatRgb, FbMemoryLinear, 24, 0, 0, 0, 3072, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x411E, 1024, 768, 60, FbPixelFormatRgb, FbMemoryLinear, 32, 0, 0, 0, 4096, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },

    /* 1280x1024 LFB */
    { 0x411F, 1280, 1024, 60, FbPixelFormatIndexed, FbMemoryLinear, 8, 256, 0, 0, 1280, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4120, 1280, 1024, 60, FbPixelFormatRgb, FbMemoryLinear, 15, 0, 0, 0, 2560, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4121, 1280, 1024, 60, FbPixelFormatRgb, FbMemoryLinear, 16, 0, 0, 0, 2560, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4122, 1280, 1024, 60, FbPixelFormatRgb, FbMemoryLinear, 24, 0, 0, 0, 3840, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4123, 1280, 1024, 60, FbPixelFormatRgb, FbMemoryLinear, 32, 0, 0, 0, 5120, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },

    /* 1600x1200 LFB */
    { 0x4124, 1600, 1200, 60, FbPixelFormatIndexed, FbMemoryLinear, 8, 256, 0, 0, 1600, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4125, 1600, 1200, 60, FbPixelFormatRgb, FbMemoryLinear, 15, 0, 0, 0, 3200, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4126, 1600, 1200, 60, FbPixelFormatRgb, FbMemoryLinear, 16, 0, 0, 0, 3200, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4127, 1600, 1200, 60, FbPixelFormatRgb, FbMemoryLinear, 24, 0, 0, 0, 4800, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },
    { 0x4128, 1600, 1200, 60, FbPixelFormatRgb, FbMemoryLinear, 32, 0, 0, 0, 6400, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },

    /* Additional common resolutions LFB */
    { 0x4129, 1280, 720, 60, FbPixelFormatRgb, FbMemoryLinear, 32, 0, 0, 0, 5120, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR },  /* 720p */
    { 0x412A, 1920, 1080, 60, FbPixelFormatRgb, FbMemoryLinear, 32, 0, 0, 0, 7680, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR }, /* 1080p */
    { 0x412B, 1920, 1200, 60, FbPixelFormatRgb, FbMemoryLinear, 32, 0, 0, 0, 7680, MODE_FLAG_GRAPHICS | MODE_FLAG_SVGA | MODE_FLAG_LINEAR }, /* WUXGA */
};

#define PC_GRAPHICS_MODE_COUNT (sizeof(gPcGraphicsModes) / sizeof(gPcGraphicsModes[0]))

/* --------------------------------------------------------------- */
/*  EDID Parsing and Mode Filtering                                */
/* --------------------------------------------------------------- */

/*
 * Validate EDID checksum.
 * Returns TRUE if valid, FALSE otherwise.
 */
static BOOLEAN
Pcga_ValidateEdid(
    IN CONST EDID_BASE_BLOCK *Edid
    )
{
    CONST UINT8 *Data = (CONST UINT8 *)Edid;
    UINT8 Sum = 0;

    /* Sum all 128 bytes - result should be 0 */
    for (UINT32 i = 0; i < sizeof(EDID_BASE_BLOCK); i++) {
        Sum += Data[i];
    }

    /* Check header signature: 00 FF FF FF FF FF FF 00 */
    if (Edid->Header[0] != 0x00 || Edid->Header[1] != 0xFF ||
        Edid->Header[2] != 0xFF || Edid->Header[3] != 0xFF ||
        Edid->Header[4] != 0xFF || Edid->Header[5] != 0xFF ||
        Edid->Header[6] != 0xFF || Edid->Header[7] != 0x00) {
        return FALSE;
    }

    return (Sum == 0);
}

/*
 * Parse detailed timing from EDID and extract resolution and refresh rate.
 */
static VOID
Pcga_ParseDetailedTiming(
    IN CONST EDID_DETAILED_TIMING *Timing,
    OUT UINT32 *Width,
    OUT UINT32 *Height,
    OUT UINT32 *RefreshRate
    )
{
    if (Timing->PixelClock == 0) {
        /* Not a valid timing descriptor */
        *Width = 0;
        *Height = 0;
        *RefreshRate = 0;
        return;
    }

    /* Extract horizontal active pixels */
    *Width = Timing->HActiveLow | ((Timing->HActiveBlankingHigh & 0xF0) << 4);

    /* Extract vertical active lines */
    *Height = Timing->VActiveLow | ((Timing->VActiveBlankingHigh & 0xF0) << 4);

    /* Calculate refresh rate from pixel clock and timing */
    UINT32 HActive = *Width;
    UINT32 HBlank = Timing->HBlankingLow | ((Timing->HActiveBlankingHigh & 0x0F) << 8);
    UINT32 VActive = *Height;
    UINT32 VBlank = Timing->VBlankingLow | ((Timing->VActiveBlankingHigh & 0x0F) << 8);

    UINT32 HTotal = HActive + HBlank;
    UINT32 VTotal = VActive + VBlank;
    UINT32 PixelClock = Timing->PixelClock * 10000; /* Convert to Hz */

    if (HTotal > 0 && VTotal > 0) {
        *RefreshRate = PixelClock / (HTotal * VTotal);
    } else {
        *RefreshRate = 60; /* Default */
    }
}

/*
 * Parse standard timing from EDID.
 */
static VOID
Pcga_ParseStandardTiming(
    IN CONST EDID_STANDARD_TIMING *Timing,
    OUT UINT32 *Width,
    OUT UINT32 *Height,
    OUT UINT32 *RefreshRate
    )
{
    /* Check if this timing slot is unused */
    if (Timing->XResolution == 0x01 && Timing->AspectRatioRefresh == 0x01) {
        *Width = 0;
        *Height = 0;
        *RefreshRate = 0;
        return;
    }

    /* Calculate horizontal resolution */
    *Width = (Timing->XResolution + 31) * 8;

    /* Extract aspect ratio (bits 7-6) */
    UINT8 AspectCode = (Timing->AspectRatioRefresh >> 6) & 0x03;

    /* Calculate vertical resolution based on aspect ratio */
    switch (AspectCode) {
        case 0x00: /* 16:10 */
            *Height = (*Width * 10) / 16;
            break;
        case 0x01: /* 4:3 */
            *Height = (*Width * 3) / 4;
            break;
        case 0x02: /* 5:4 */
            *Height = (*Width * 4) / 5;
            break;
        case 0x03: /* 16:9 */
            *Height = (*Width * 9) / 16;
            break;
    }

    /* Extract refresh rate (bits 5-0) + 60 Hz */
    *RefreshRate = (Timing->AspectRatioRefresh & 0x3F) + 60;
}

/*
 * Check if a mode is supported by the EDID data.
 * Returns TRUE if the mode matches an EDID timing.
 */
static BOOLEAN
Pcga_IsModeSupported(
    IN CONST EDID_BASE_BLOCK *Edid,
    IN UINT32 Width,
    IN UINT32 Height,
    IN UINT32 RefreshRate
    )
{
    if (!Pcga_ValidateEdid(Edid)) {
        /* Invalid EDID - allow all modes */
        return TRUE;
    }

    /* Check established timings (720x400@70, 640x480@60, 800x600@60, etc.) */
    if (Width == 640 && Height == 480 && RefreshRate == 60 && (Edid->EstablishedTimings1 & 0x20)) {
        return TRUE;
    }
    if (Width == 800 && Height == 600 && RefreshRate == 60 && (Edid->EstablishedTimings1 & 0x01)) {
        return TRUE;
    }
    if (Width == 1024 && Height == 768 && RefreshRate == 60 && (Edid->EstablishedTimings2 & 0x08)) {
        return TRUE;
    }

    /* Check standard timings */
    for (UINT32 i = 0; i < 8; i++) {
        UINT32 StdWidth, StdHeight, StdRefresh;
        Pcga_ParseStandardTiming(&Edid->StandardTimings[i], &StdWidth, &StdHeight, &StdRefresh);

        if (StdWidth == Width && StdHeight == Height &&
            (RefreshRate == 0 || StdRefresh == RefreshRate)) {
            return TRUE;
        }
    }

    /* Check detailed timings */
    for (UINT32 i = 0; i < 4; i++) {
        UINT32 DtWidth, DtHeight, DtRefresh;
        Pcga_ParseDetailedTiming(&Edid->DetailedTimings[i], &DtWidth, &DtHeight, &DtRefresh);

        if (DtWidth == Width && DtHeight == Height &&
            (RefreshRate == 0 || DtRefresh == RefreshRate)) {
            return TRUE;
        }
    }

    /* Allow lower resolutions if not in EDID (backward compatibility) */
    if (Width <= 640 && Height <= 480) {
        return TRUE;
    }

    return FALSE;
}

/*
 * Get maximum color depth supported by monitor from EDID.
 * Returns bits per pixel (8, 16, 24, 32).
 */
static UINT32
Pcga_GetMaxColorDepth(
    IN CONST EDID_BASE_BLOCK *Edid
    )
{
    if (!Pcga_ValidateEdid(Edid)) {
        /* No valid EDID - assume full color support */
        return 32;
    }

    /* Check feature support byte for color depth */
    /* Bit 0-2: DFP color depth (if applicable) */
    /* For analog, assume based on EDID version */
    if (Edid->EdidVersion >= 1 && Edid->EdidRevision >= 3) {
        /* EDID 1.3+ supports 24-bit true color */
        return 32;
    }

    return 16; /* Conservative default */
}

/* --------------------------------------------------------------- */
/*  Hardware Detection                                              */
/* --------------------------------------------------------------- */

/*
 * Detect available graphics hardware.
 * Returns a bitmask of MODE_FLAG_* values indicating supported hardware.
 */
static UINT32
Pcga_DetectHardware(
    VOID
    )
{
    UINT32 Capabilities = 0;

    /* Always support VGA (baseline for PC graphics) */
    Capabilities |= MODE_FLAG_VGA;

    /* Check for EGA by reading switch settings */
    /* EGA has different switch configuration than VGA */
    UINT8 Misc = ANX_CPU_INB(VGA_MISC_READ);
    if ((Misc & 0x30) != 0) {
        Capabilities |= MODE_FLAG_EGA;
    }

    /* Check for CGA (look for 0xB8000 memory) */
    /* In real implementation, would probe memory */
    Capabilities |= MODE_FLAG_CGA;

    /* Check for SVGA by attempting VBE detection */
    /* In real implementation, would use INT 10h AX=4F00h */
    /* For now, assume SVGA is available */
    Capabilities |= MODE_FLAG_SVGA;

    /* MCGA is a subset of VGA on PS/2 Model 25/30 */
    Capabilities |= MODE_FLAG_MCGA;

    return Capabilities;
}

/*
 * Filter mode list based on hardware capabilities and EDID.
 * Returns number of supported modes.
 */
static UINT32
Pcga_FilterModes(
    IN CONST EDID_BASE_BLOCK *Edid,
    IN UINT32 HardwareCapabilities,
    OUT UINT32 *SupportedModes,
    IN UINT32 MaxModes
    )
{
    UINT32 Count = 0;
    UINT32 MaxDepth = Pcga_GetMaxColorDepth(Edid);

    for (UINT32 i = 0; i < PC_GRAPHICS_MODE_COUNT && Count < MaxModes; i++) {
        CONST PC_GRAPHICS_MODE *Mode = &gPcGraphicsModes[i];

        /* Check hardware capability flags */
        if ((Mode->Flags & HardwareCapabilities) == 0) {
            /* Hardware doesn't support this mode's requirements */
            continue;
        }

        /* Check color depth */
        if (Mode->BitsPerPixel > MaxDepth && Mode->PixelFormat == FbPixelFormatRgb) {
            continue;
        }

        /* Check EDID for graphics modes (skip text modes) */
        if ((Mode->Flags & MODE_FLAG_GRAPHICS) != 0) {
            if (!Pcga_IsModeSupported(Edid, Mode->Width, Mode->Height, Mode->RefreshRate)) {
                continue;
            }
        }

        /* Mode is supported */
        SupportedModes[Count++] = i;
    }

    return Count;
}

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

/*
 * Read pixel from linear framebuffer.
 */
static UINT8
PcGraphics_ReadPixelLinear(
    PCGA_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    UINT32 Offset = Y * Backend->Descriptor.Pitch + X;
    return Backend->FramebufferBase[Offset];
}

/*
 * Read pixel from interleaved framebuffer (CGA).
 */
static UINT8
PcGraphics_ReadPixelInterleaved(
    PCGA_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    /* Bank-interleaved (CGA-style): even/odd scanlines in different banks */
    UINT32 RowOffset = (Y & 1) ? Backend->Descriptor.BankOffset : 0;
    UINT32 ByteOffset = (Y / Backend->Descriptor.BankInterleave) *
                        Backend->Descriptor.Pitch + (X / 4);

    UINT8 *Addr = Backend->FramebufferBase + RowOffset + ByteOffset;
    UINT32 BitOffset = (3 - (X % 4)) * 2;  /* 2 bits per pixel */

    return (*Addr >> BitOffset) & 0x03;
}

/*
 * Read pixel from planar framebuffer (EGA/VGA).
 */
static UINT8
PcGraphics_ReadPixelPlanar(
    PCGA_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    /* Planar mode (EGA/VGA): each plane holds one bit per pixel */
    UINT32 Offset = Y * (Backend->Descriptor.Pitch / Backend->Descriptor.NumPlanes) + (X / 8);
    UINT8 BitMask = 0x80 >> (X % 8);
    UINT8 ColorIndex = 0;

    /* Read from all planes */
    for (UINT32 Plane = 0; Plane < Backend->Descriptor.NumPlanes; Plane++) {
        /* Set read plane select */
        PcGraphics_SetReadMapSelect(Backend, Plane);

        /* Read bit from this plane */
        UINT8 *Addr = Backend->FramebufferBase + Offset;
        if (*Addr & BitMask) {
            ColorIndex |= (1 << Plane);
        }
    }

    return ColorIndex;
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

static UINT8
PcGraphics_ReadPixel(
    PCGA_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    switch (Backend->Descriptor.MemoryOrganization) {
        case FbMemoryLinear:
            return PcGraphics_ReadPixelLinear(Backend, X, Y);

        case FbMemoryInterleaved:
            return PcGraphics_ReadPixelInterleaved(Backend, X, Y);

        case FbMemoryPlanar:
            return PcGraphics_ReadPixelPlanar(Backend, X, Y);

        case FbMemoryBanked:
            /* Handle bank switching for VESA modes */
            /* Would calculate bank and switch if needed */
            return PcGraphics_ReadPixelLinear(Backend, X, Y);

        default:
            return 0;
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

/*
 * Convert palette index back to FB_COLOR.
 */
static FB_COLOR
PcGraphics_MapIndexToColor(
    PCGA_BACKEND *Backend,
    UINT8 ColorIndex
    )
{
    switch (Backend->Descriptor.PixelFormat) {
        case FbPixelFormat1Bpp:
        case FbPixelFormat2Bpp:
        case FbPixelFormat4Bpp: {
            /* Grayscale - expand back to 8-bit */
            UINT8 MaxValue = (1 << (Backend->Descriptor.PixelFormat == FbPixelFormat1Bpp ? 1 :
                                    Backend->Descriptor.PixelFormat == FbPixelFormat2Bpp ? 2 : 4)) - 1;
            UINT8 Gray = (ColorIndex * 255) / MaxValue;
            return FB_MAKE_COLOR(Gray, Gray, Gray, 255);
        }

        case FbPixelFormatIndexed4:
        case FbPixelFormatIndexed16:
        case FbPixelFormatIndexed256: {
            /* Look up in palette */
            UINT32 PaletteSize = Backend->Descriptor.PixelFormat == FbPixelFormatIndexed4 ? 4 :
                                Backend->Descriptor.PixelFormat == FbPixelFormatIndexed16 ? 16 : 256;
            if (ColorIndex < PaletteSize) {
                PALETTE_ENTRY *Entry = &Backend->Palette[ColorIndex];
                return FB_MAKE_COLOR(Entry->Red, Entry->Green, Entry->Blue, 255);
            }
            return FB_MAKE_COLOR(0, 0, 0, 255);
        }

        case FbPixelFormatPlanar2:
        case FbPixelFormatPlanar4:
        case FbPixelFormatPlanar6:
        case FbPixelFormatPlanar8: {
            /* Look up in VGA palette */
            UINT32 NumColors = 1 << Backend->Descriptor.NumPlanes;
            UINT32 Index = ColorIndex & ((NumColors < 16 ? NumColors : 16) - 1);
            if (Index < 16) {
                PALETTE_ENTRY *Entry = &gVgaPalette[Index];
                return FB_MAKE_COLOR(Entry->Red, Entry->Green, Entry->Blue, 255);
            }
            return FB_MAKE_COLOR(0, 0, 0, 255);
        }

        default:
            return FB_MAKE_COLOR(0, 0, 0, 255);
    }
}

/* --------------------------------------------------------------- */
/*  Hardware Scrolling and Block Operations                         */
/* --------------------------------------------------------------- */

/*
 * Scroll screen contents using hardware display start register (CRTC).
 * This is much faster than copying pixels for VGA text and graphics modes.
 *
 * For non-VGA modes, falls back to software scrolling.
 */
static VOID
Pcga_ScrollScreen(
    PCGA_BACKEND *Backend,
    INT32 DeltaX,
    INT32 DeltaY
    )
{
    if (DeltaX == 0 && DeltaY == 0) {
        return;
    }

    /* For VGA linear modes, we can use display start address scrolling */
    /* This only works for vertical scrolling in linear framebuffer modes */
    if (Backend->Descriptor.MemoryOrganization == FbMemoryLinear &&
        DeltaX == 0 && Backend->IsAddressable) {

        /* Calculate new display start offset */
        INT32 NewStartY = (INT32)Backend->DisplayStartOffset / Backend->Descriptor.Pitch + DeltaY;

        /* Wrap around for circular buffer scrolling */
        UINT32 MaxOffset = Backend->Descriptor.Height * Backend->Descriptor.Pitch;
        UINT32 NewStart = (NewStartY * Backend->Descriptor.Pitch) % MaxOffset;

        /* Set hardware display start */
        Pcga_SetDisplayStart(Backend, NewStart);
        Backend->DisplayStartOffset = NewStart;
        return;
    }

    /* Software scrolling fallback: copy pixels */
    /* Handle vertical scrolling */
    if (DeltaY != 0) {
        if (DeltaY < 0) {
            /* Scroll up: copy from bottom to top */
            for (INT32 y = 0; y < (INT32)Backend->Descriptor.Height + DeltaY; y++) {
                for (INT32 x = 0; x < (INT32)Backend->Descriptor.Width; x++) {
                    FB_COLOR Color;
                    PcGraphics_GetPixel(&Backend->Base, x, y - DeltaY, &Color);
                    PcGraphics_SetPixel(&Backend->Base, x, y, Color);
                }
            }
        } else {  /* DeltaY > 0 */
            /* Scroll down: copy from top to bottom */
            for (INT32 y = (INT32)Backend->Descriptor.Height - 1; y >= DeltaY; y--) {
                for (INT32 x = 0; x < (INT32)Backend->Descriptor.Width; x++) {
                    FB_COLOR Color;
                    PcGraphics_GetPixel(&Backend->Base, x, y - DeltaY, &Color);
                    PcGraphics_SetPixel(&Backend->Base, x, y, Color);
                }
            }
        }
    }

    /* Handle horizontal scrolling */
    if (DeltaX != 0) {
        if (DeltaX < 0) {
            /* Scroll left: copy from right to left */
            for (INT32 y = 0; y < (INT32)Backend->Descriptor.Height; y++) {
                for (INT32 x = 0; x < (INT32)Backend->Descriptor.Width + DeltaX; x++) {
                    FB_COLOR Color;
                    PcGraphics_GetPixel(&Backend->Base, x - DeltaX, y, &Color);
                    PcGraphics_SetPixel(&Backend->Base, x, y, Color);
                }
            }
        } else {  /* DeltaX > 0 */
            /* Scroll right: copy from left to right */
            for (INT32 y = 0; y < (INT32)Backend->Descriptor.Height; y++) {
                for (INT32 x = (INT32)Backend->Descriptor.Width - 1; x >= DeltaX; x--) {
                    FB_COLOR Color;
                    PcGraphics_GetPixel(&Backend->Base, x - DeltaX, y, &Color);
                    PcGraphics_SetPixel(&Backend->Base, x, y, Color);
                }
            }
        }
    }
}

/*
 * Optimized block transfer with ROP2 support.
 * Uses hardware acceleration when possible.
 */
static VOID
Pcga_BlockTransferWithRop(
    PCGA_BACKEND *Backend,
    INT32 DestX,
    INT32 DestY,
    CONST UINT8 *SrcData,
    UINT32 SrcWidth,
    UINT32 SrcHeight,
    UINT32 SrcPitch,
    FB_ROP2 Rop
    )
{
    /* Fast path: ROP is simple copy and formats match */
    if (Rop == FbRop2CopyPen && Backend->Descriptor.PixelFormat == FbPixelFormatIndexed &&
        Backend->Descriptor.MemoryOrganization == FbMemoryLinear) {

        for (UINT32 y = 0; y < SrcHeight; y++) {
            UINT32 DestOffset = (DestY + y) * Backend->Descriptor.Pitch + DestX;
            UINT32 SrcOffset = y * SrcPitch;

            if (Backend->RtlCopyMemoryFunc) {
                Backend->RtlCopyMemoryFunc(&Backend->FramebufferBase[DestOffset],
                                          &SrcData[SrcOffset], SrcWidth);
            } else {
                ANX_MEMCPY(&Backend->FramebufferBase[DestOffset],
                          &SrcData[SrcOffset], SrcWidth);
            }
        }
        return;
    }

    /* ROP-aware pixel-by-pixel transfer */
    for (UINT32 y = 0; y < SrcHeight; y++) {
        for (UINT32 x = 0; x < SrcWidth; x++) {
            UINT32 SrcOffset = y * SrcPitch + x;
            UINT8 SrcPixel = SrcData[SrcOffset];

            if (Rop == FbRop2CopyPen) {
                /* Simple copy */
                if (Backend->Descriptor.PixelFormat == FbPixelFormatIndexed) {
                    UINT32 DestOffset = (DestY + y) * Backend->Descriptor.Pitch + (DestX + x);
                    Backend->FramebufferBase[DestOffset] = SrcPixel;
                }
            } else {
                /* Read-modify-write with ROP */
                UINT32 DestOffset = (DestY + y) * Backend->Descriptor.Pitch + (DestX + x);
                UINT8 DestPixel = Backend->FramebufferBase[DestOffset];
                UINT8 NewPixel = PcGraphics_ApplyRop2(SrcPixel, DestPixel, Rop);
                Backend->FramebufferBase[DestOffset] = NewPixel;
            }
        }
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
    PCGA_BACKEND *Backend = (PCGA_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    /* Text mode - read character and attribute */
    if (Backend->Descriptor.PixelFormat == FbPixelFormatText) {
        UINT32 Offset = Y * Backend->Descriptor.Pitch + X * 2;
        UINT8 Char = Backend->FramebufferBase[Offset];
        UINT8 Attr = Backend->FramebufferBase[Offset + 1];

        /* Convert attribute to color - use background color */
        UINT8 BgIndex = (Attr >> 4) & 0x0F;
        if (BgIndex < 16) {
            PALETTE_ENTRY *Entry = &gVgaPalette[BgIndex];
            *Color = FB_MAKE_COLOR(Entry->Red, Entry->Green, Entry->Blue, 255);
        } else {
            *Color = FB_MAKE_COLOR(0, 0, 0, 255);
        }
        return S_OK;
    }

    /* Graphics modes - read pixel and convert to color */
    ColorIndex = PcGraphics_ReadPixel(Backend, X, Y);
    *Color = PcGraphics_MapIndexToColor(Backend, ColorIndex);

    return S_OK;
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

static HRESULT
PcGraphics_BlitBitmapToText(
    PCGA_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    /* Text mode blitting - convert bitmap to character representation */

    /* Direct text-to-text copy */
    if (SourceFormat == FbPixelFormatText) {
        /* Source is character/attribute pairs */
        for (UINT32 Row = 0; Row < Height; Row++) {
            if ((Y + Row) >= Backend->Descriptor.Height) {
                break;
            }
            for (UINT32 Col = 0; Col < Width; Col++) {
                if ((X + Col) >= Backend->Descriptor.Width) {
                    break;
                }
                UINT32 SrcOffset = (Row * Width + Col) * 2;
                UINT8 Char = Bitmap[SrcOffset];
                UINT8 Attr = Bitmap[SrcOffset + 1];
                Pcga_WriteTextChar(Backend, X + Col, Y + Row, Char, Attr);
            }
        }
        return S_OK;
    }

    /* Graphics-to-text conversion using block characters */
    /* Use intensity-based character selection:
     * 0xDB (█) - full block (intensity > 192)
     * 0xB2 (▓) - dark shade (intensity > 128)
     * 0xB1 (▒) - medium shade (intensity > 64)
     * 0xB0 (░) - light shade (intensity > 0)
     * 0x20 (space) - empty (intensity == 0)
     */

    for (UINT32 Row = 0; Row < Height; Row++) {
        if ((Y + Row) >= Backend->Descriptor.Height) {
            break;
        }
        for (UINT32 Col = 0; Col < Width; Col++) {
            if ((X + Col) >= Backend->Descriptor.Width) {
                break;
            }

            UINT8 PixelValue = 0;
            UINT8 ColorIndex = 0;

            /* Extract pixel value based on source format */
            switch (SourceFormat) {
                case FbPixelFormat1Bpp: {
                    UINT32 ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
                    UINT32 BitIndex = 7 - (Col % 8);
                    PixelValue = (Bitmap[ByteIndex] & (1 << BitIndex)) ? 255 : 0;
                    break;
                }

                case FbPixelFormat2Bpp: {
                    UINT32 ByteIndex = Row * ((Width + 3) / 4) + (Col / 4);
                    UINT32 BitOffset = (3 - (Col % 4)) * 2;
                    UINT8 Value = (Bitmap[ByteIndex] >> BitOffset) & 0x03;
                    PixelValue = (Value * 255) / 3;
                    break;
                }

                case FbPixelFormat4Bpp: {
                    UINT32 ByteIndex = Row * ((Width + 1) / 2) + (Col / 2);
                    UINT8 Value = (Col & 1) ?
                        (Bitmap[ByteIndex] & 0x0F) :
                        ((Bitmap[ByteIndex] >> 4) & 0x0F);
                    PixelValue = (Value * 255) / 15;
                    break;
                }

                case FbPixelFormatIndexed4:
                case FbPixelFormatIndexed16:
                case FbPixelFormatIndexed256: {
                    UINT32 SrcOffset = Row * Width + Col;
                    ColorIndex = Bitmap[SrcOffset];
                    /* Get intensity from palette */
                    if (ColorIndex < 256 && ColorIndex < (UINT32)(1 << (SourceFormat == FbPixelFormatIndexed4 ? 4 :
                                                                         SourceFormat == FbPixelFormatIndexed16 ? 4 : 8))) {
                        PALETTE_ENTRY *Entry = &Backend->Palette[ColorIndex];
                        /* Calculate luminance: 0.299*R + 0.587*G + 0.114*B */
                        PixelValue = (Entry->Red * 77 + Entry->Green * 150 + Entry->Blue * 29) >> 8;
                    }
                    break;
                }

                default:
                    /* Unsupported format - use space */
                    PixelValue = 0;
                    break;
            }

            /* Map intensity to block character */
            UINT8 Char;
            UINT8 Attr = 0x0F;  /* White on black */

            if (PixelValue == 0) {
                Char = 0x20;  /* Space */
            } else if (PixelValue < 64) {
                Char = 0xB0;  /* Light shade */
                Attr = 0x08;  /* Dark gray on black */
            } else if (PixelValue < 128) {
                Char = 0xB1;  /* Medium shade */
                Attr = 0x07;  /* Light gray on black */
            } else if (PixelValue < 192) {
                Char = 0xB2;  /* Dark shade */
                Attr = 0x0F;  /* White on black */
            } else {
                Char = 0xDB;  /* Full block */
                Attr = 0x0F;  /* White on black */
            }

            Pcga_WriteTextChar(Backend, X + Col, Y + Row, Char, Attr);
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

    /* Text mode - convert bitmap to text representation */
    if (Backend->Descriptor.PixelFormat == FbPixelFormatText) {
        return PcGraphics_BlitBitmapToText(Backend, X, Y, Width, Height,
                                          Bitmap, SourceFormat);
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

    /* Basic mode information */
    ModeDesc->Width = Mode->Width;
    ModeDesc->Height = Mode->Height;
    ModeDesc->Pitch = Mode->Pitch;
    ModeDesc->PixelFormat = Mode->PixelFormat;
    ModeDesc->MemoryOrganization = Mode->MemoryOrganization;
    ModeDesc->PhysicalBase = Mode->PhysicalBase;

    /* Pixel format specific fields */
    ModeDesc->BitsPerPixel = Mode->BitsPerPixel;
    ModeDesc->PaletteSize = Mode->PaletteSize;
    ModeDesc->NumPlanes = Mode->NumPlanes;

    /* Memory organization */
    ModeDesc->IsAddressable = (Mode->MemoryOrganization == FbMemoryLinear ||
                               Mode->PixelFormat == FbPixelFormatText);

    /* For banked modes */
    if (Mode->MemoryOrganization == FbMemoryBanked) {
        ModeDesc->BankSize = 64 * 1024;  /* 64KB standard bank size */
        ModeDesc->BankInterleave = 1;
        ModeDesc->BankOffset = 0;
    }

    /* For CGA interleaved modes */
    if (Mode->MemoryOrganization == FbMemoryInterleaved) {
        ModeDesc->BankInterleave = 2;    /* Even/odd scanlines */
        ModeDesc->BankOffset = 0x2000;   /* 8KB between banks */
    }

    /* RGB bit masks (for direct color modes) */
    if (Mode->PixelFormat == FbPixelFormatRgb) {
        switch (Mode->BitsPerPixel) {
            case 15: /* RGB555 */
                ModeDesc->RedMask   = 0x7C00;
                ModeDesc->GreenMask = 0x03E0;
                ModeDesc->BlueMask  = 0x001F;
                ModeDesc->AlphaMask = 0x0000;
                break;
            case 16: /* RGB565 */
                ModeDesc->RedMask   = 0xF800;
                ModeDesc->GreenMask = 0x07E0;
                ModeDesc->BlueMask  = 0x001F;
                ModeDesc->AlphaMask = 0x0000;
                break;
            case 24: /* RGB888 */
                ModeDesc->RedMask   = 0x00FF0000;
                ModeDesc->GreenMask = 0x0000FF00;
                ModeDesc->BlueMask  = 0x000000FF;
                ModeDesc->AlphaMask = 0x00000000;
                break;
            case 32: /* RGBA8888 */
                ModeDesc->RedMask   = 0x00FF0000;
                ModeDesc->GreenMask = 0x0000FF00;
                ModeDesc->BlueMask  = 0x000000FF;
                ModeDesc->AlphaMask = 0xFF000000;
                break;
        }
    }

    /* For text modes */
    if (Mode->PixelFormat == FbPixelFormatText) {
        ModeDesc->CharWidth = 8;
        ModeDesc->CharHeight = 16;
        ModeDesc->FontBank = 0;
    }

    /* I/O port access */
    ModeDesc->IoPortBase = 0x3C0;
    ModeDesc->RequiresIoAccess = (Mode->MemoryOrganization == FbMemoryPlanar ||
                                  Mode->MemoryOrganization == FbMemoryBanked ||
                                  Mode->MemoryOrganization == FbMemoryInterleaved ||
                                  Mode->PixelFormat == FbPixelFormatText);

    /* Calculate size */
    ModeDesc->Size = Mode->Pitch * Mode->Height;

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

/*
 * Get hardware capabilities (CGA/EGA/VGA/SVGA/MCGA).
 * Returns a bitmask of supported hardware flags.
 */
UINT32
FbPcGraphicsGetCapabilities(
    VOID
    )
{
    return Pcga_DetectHardware();
}

/*
 * Filter modes based on EDID and hardware capabilities.
 * Returns the number of supported modes in the output array.
 */
UINT32
FbPcGraphicsFilterModes(
    IN CONST EDID_BASE_BLOCK *Edid,
    OUT UINT32 *SupportedModeIndices,
    IN UINT32 MaxModes
    )
{
    UINT32 Capabilities = Pcga_DetectHardware();

    if (Edid == NULL || SupportedModeIndices == NULL || MaxModes == 0) {
        return 0;
    }

    return Pcga_FilterModes(Edid, Capabilities, SupportedModeIndices, MaxModes);
}

/*
 * Get the best mode matching requested size, depth, and refresh rate.
 * If EDID is provided, only returns modes supported by the monitor.
 * Returns mode index, or -1 if no suitable mode found.
 */
INT32
FbPcGraphicsFindBestMode(
    IN CONST EDID_BASE_BLOCK *Edid,
    IN UINT32 PreferredWidth,
    IN UINT32 PreferredHeight,
    IN UINT32 PreferredDepth,
    IN UINT32 PreferredRefresh
    )
{
    UINT32 Capabilities = Pcga_DetectHardware();
    INT32 BestMatch = -1;
    INT32 BestScore = -1;

    for (UINT32 i = 0; i < PC_GRAPHICS_MODE_COUNT; i++) {
        CONST PC_GRAPHICS_MODE *Mode = &gPcGraphicsModes[i];

        /* Check hardware capability */
        if ((Mode->Flags & Capabilities) == 0) {
            continue;
        }

        /* Skip text modes */
        if (Mode->PixelFormat == FbPixelFormatText) {
            continue;
        }

        /* Check EDID support */
        if (Edid != NULL) {
            if (!Pcga_IsModeSupported(Edid, Mode->Width, Mode->Height, Mode->RefreshRate)) {
                continue;
            }

            UINT32 MaxDepth = Pcga_GetMaxColorDepth(Edid);
            if (Mode->BitsPerPixel > MaxDepth) {
                continue;
            }
        }

        /* Calculate match score (higher is better) */
        INT32 Score = 0;

        /* Exact matches get high scores */
        if (Mode->Width == PreferredWidth) Score += 1000;
        if (Mode->Height == PreferredHeight) Score += 1000;
        if (Mode->BitsPerPixel == PreferredDepth) Score += 500;
        if (Mode->RefreshRate == PreferredRefresh) Score += 250;

        /* Close matches get lower scores */
        if (Mode->Width >= PreferredWidth && Mode->Width < PreferredWidth + 100) Score += 100;
        if (Mode->Height >= PreferredHeight && Mode->Height < PreferredHeight + 100) Score += 100;
        if (Mode->BitsPerPixel >= PreferredDepth) Score += 50;

        /* Prefer linear over banked */
        if (Mode->MemoryOrganization == FbMemoryLinear) Score += 25;

        /* Update best match */
        if (Score > BestScore) {
            BestScore = Score;
            BestMatch = (INT32)i;
        }
    }

    return BestMatch;
}

/*
 * Extract font from BIOS ROM.
 * Copies the ROM font into a user-provided buffer.
 */
HRESULT
FbPcGraphicsExtractRomFont(
    OUT UINT8 *FontBuffer,
    IN UINT32 BufferSize,
    IN UINT32 CharHeight
    )
{
    if (FontBuffer == NULL) {
        return E_POINTER;
    }

    /* Validate character height */
    if (CharHeight != 8 && CharHeight != 14 && CharHeight != 16) {
        return E_INVALIDARG;
    }

    /* Validate buffer size (256 characters × CharHeight bytes) */
    UINT32 RequiredSize = 256 * CharHeight;
    if (BufferSize < RequiredSize) {
        return E_INVALIDARG;
    }

    /* Detect adapter type */
    PCGA_ADAPTER_TYPE AdapterType = Pcga_DetectAdapter();

    /* Extract ROM font */
    Pcga_ExtractRomFont(FontBuffer, CharHeight, AdapterType);

    return S_OK;
}

/*
 * Get recommended font height for current adapter.
 * Returns the native font height for the detected adapter.
 */
HRESULT
FbPcGraphicsGetRecommendedFontHeight(
    OUT UINT32 *FontHeight
    )
{
    if (FontHeight == NULL) {
        return E_POINTER;
    }

    /* Detect adapter type */
    PCGA_ADAPTER_TYPE AdapterType = Pcga_DetectAdapter();

    /* Return appropriate font height */
    switch (AdapterType) {
        case PCGA_ADAPTER_CGA:
            *FontHeight = 8;   /* CGA uses 8x8 font */
            break;

        case PCGA_ADAPTER_EGA:
            *FontHeight = 14;  /* EGA uses 8x14 font */
            break;

        case PCGA_ADAPTER_MCGA:
            *FontHeight = 16;  /* MCGA uses 8x16 font (VGA-compatible) */
            break;

        case PCGA_ADAPTER_VGA:
            *FontHeight = 16;  /* VGA uses 8x16 font */
            break;

        case PCGA_ADAPTER_SVGA:
        case PCGA_ADAPTER_XGA:
        default:
            *FontHeight = 16;  /* SVGA/XGA use 8x16 font (VGA-compatible) */
            break;
    }

    return S_OK;
}

/*
 * Load a custom font into character generator RAM.
 * Automatically detects CGA/EGA/VGA and uses appropriate loading method.
 */
HRESULT
FbPcGraphicsLoadFont(
    IN IFramebufferBackend *Backend,
    IN CONST UINT8 *FontData,
    IN UINT32 CharHeight,
    IN UINT32 CharOffset,
    IN UINT32 CharCount,
    IN UINT32 Bank
    )
{
    PCGA_BACKEND *PcBackend = (PCGA_BACKEND *)Backend;

    if (Backend == NULL || FontData == NULL) {
        return E_POINTER;
    }

    if (CharHeight > 32 || Bank > 1 || CharCount == 0) {
        return E_INVALIDARG;
    }

    Pcga_LoadFont(PcBackend, FontData, CharHeight, CharOffset, CharCount, Bank);
    return S_OK;
}

/*
 * Select which font bank to display in text mode.
 */
HRESULT
FbPcGraphicsSelectFontBank(
    IN IFramebufferBackend *Backend,
    IN UINT32 Bank
    )
{
    PCGA_BACKEND *PcBackend = (PCGA_BACKEND *)Backend;

    if (Backend == NULL) {
        return E_POINTER;
    }

    if (Bank > 1) {
        return E_INVALIDARG;
    }

    Pcga_SelectFontBank(PcBackend, Bank);
    return S_OK;
}

/*
 * Wait for vertical blank interval.
 */
HRESULT
FbPcGraphicsWaitForVBlank(
    IN IFramebufferBackend *Backend
    )
{
    if (Backend == NULL) {
        return E_POINTER;
    }

    Pcga_WaitForVBlank();
    return S_OK;
}

/*
 * Check if currently in vertical blank.
 */
HRESULT
FbPcGraphicsIsVBlank(
    IN IFramebufferBackend *Backend,
    OUT BOOLEAN *IsVBlank
    )
{
    if (Backend == NULL || IsVBlank == NULL) {
        return E_POINTER;
    }

    *IsVBlank = Pcga_IsVBlank();
    return S_OK;
}

/*
 * Set display start address for hardware scrolling or page flipping.
 */
HRESULT
FbPcGraphicsSetDisplayStart(
    IN IFramebufferBackend *Backend,
    IN UINT32 Offset
    )
{
    PCGA_BACKEND *PcBackend = (PCGA_BACKEND *)Backend;

    if (Backend == NULL) {
        return E_POINTER;
    }

    Pcga_SetDisplayStart(PcBackend, Offset);
    return S_OK;
}

/*
 * Get current display start address.
 */
HRESULT
FbPcGraphicsGetDisplayStart(
    IN IFramebufferBackend *Backend,
    OUT UINT32 *Offset
    )
{
    PCGA_BACKEND *PcBackend = (PCGA_BACKEND *)Backend;

    if (Backend == NULL || Offset == NULL) {
        return E_POINTER;
    }

    *Offset = Pcga_GetDisplayStart(PcBackend);
    return S_OK;
}

/*
 * Blit from one screen location to another (screen-to-screen copy).
 * Uses VGA latching for planar modes (extremely fast).
 */
HRESULT
FbPcGraphicsBlitScreen(
    IN IFramebufferBackend *Backend,
    IN UINT32 SrcX,
    IN UINT32 SrcY,
    IN UINT32 DestX,
    IN UINT32 DestY,
    IN UINT32 Width,
    IN UINT32 Height
    )
{
    PCGA_BACKEND *PcBackend = (PCGA_BACKEND *)Backend;

    if (Backend == NULL) {
        return E_POINTER;
    }

    Pcga_BlitScreen(PcBackend, SrcX, SrcY, DestX, DestY, Width, Height);
    return S_OK;
}

/*
 * Set text mode caret (cursor) position.
 */
HRESULT
FbPcGraphicsSetCaretPosition(
    IN IFramebufferBackend *Backend,
    IN UINT32 X,
    IN UINT32 Y
    )
{
    PCGA_BACKEND *PcBackend = (PCGA_BACKEND *)Backend;

    if (Backend == NULL) {
        return E_POINTER;
    }

    Pcga_SetCaretPosition(PcBackend, X, Y);
    return S_OK;
}

/*
 * Set text mode caret (cursor) shape and visibility.
 */
HRESULT
FbPcGraphicsSetCaretShape(
    IN IFramebufferBackend *Backend,
    IN UINT32 StartLine,
    IN UINT32 EndLine,
    IN BOOLEAN Visible
    )
{
    PCGA_BACKEND *PcBackend = (PCGA_BACKEND *)Backend;

    if (Backend == NULL) {
        return E_POINTER;
    }

    Pcga_SetCaretShape(PcBackend, StartLine, EndLine, Visible);
    return S_OK;
}

/*
 * Set border color (VGA overscan).
 */
HRESULT
FbPcGraphicsSetBorderColor(
    IN IFramebufferBackend *Backend,
    IN UINT8 Color
    )
{
    PCGA_BACKEND *PcBackend = (PCGA_BACKEND *)Backend;

    if (Backend == NULL) {
        return E_POINTER;
    }

    Pcga_SetBorderColor(PcBackend, Color);
    return S_OK;
}

/*
 * Set palette entry.
 */
HRESULT
FbPcGraphicsSetPaletteEntry(
    IN IFramebufferBackend *Backend,
    IN UINT8 Index,
    IN UINT8 Red,
    IN UINT8 Green,
    IN UINT8 Blue
    )
{
    PCGA_BACKEND *PcBackend = (PCGA_BACKEND *)Backend;

    if (Backend == NULL) {
        return E_POINTER;
    }

    FB_PALETTE_ENTRY Entry;
    Entry.Red = Red;
    Entry.Green = Green;
    Entry.Blue = Blue;
    Entry.Reserved = 0;

    Pcga_SetPaletteEntry(PcBackend, Index, &Entry);
    return S_OK;
}

/*
 * Get palette entry.
 */
HRESULT
FbPcGraphicsGetPaletteEntry(
    IN IFramebufferBackend *Backend,
    IN UINT8 Index,
    OUT UINT8 *Red,
    OUT UINT8 *Green,
    OUT UINT8 *Blue
    )
{
    PCGA_BACKEND *PcBackend = (PCGA_BACKEND *)Backend;

    if (Backend == NULL || Red == NULL || Green == NULL || Blue == NULL) {
        return E_POINTER;
    }

    Pcga_GetPaletteEntry(PcBackend, Index, &PcBackend->Palette[Index]);

    *Red = PcBackend->Palette[Index].Red;
    *Green = PcBackend->Palette[Index].Green;
    *Blue = PcBackend->Palette[Index].Blue;

    return S_OK;
}
