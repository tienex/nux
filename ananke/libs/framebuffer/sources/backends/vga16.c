/*++
    Module Name:

        vga16.c

    Abstract:

        VGA 16-color planar mode backend.
        Supports standard VGA 640x480 16-color mode (mode 12h) and
        compatible EGA/VGA graphics modes.

        Memory layout:
          - Base address: 0xA0000
          - 4 bit planes (64KB each)
          - Access via VGA sequencer and graphics controller

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  VGA Hardware Constants                                          */
/* --------------------------------------------------------------- */

#define VGA_BASE_ADDR           0xA0000
#define VGA_MEM_SIZE            0x10000  /* 64KB */

/* VGA I/O ports */
#define VGA_SEQ_INDEX           0x3C4
#define VGA_SEQ_DATA            0x3C5
#define VGA_GC_INDEX            0x3CE
#define VGA_GC_DATA             0x3CF

/* Sequencer registers */
#define VGA_SEQ_MAP_MASK        0x02

/* Graphics controller registers */
#define VGA_GC_READ_MAP         0x04
#define VGA_GC_MODE             0x05
#define VGA_GC_MISC             0x06

/* --------------------------------------------------------------- */
/*  VGA16 Backend Structure                                         */
/* --------------------------------------------------------------- */

typedef struct _VGA16_FB_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    UINT8                       *VideoMemory;
    BOOLEAN                     Initialized;
} VGA16_FB_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE Vga16Fb_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE Vga16Fb_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE Vga16Fb_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE Vga16Fb_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE Vga16Fb_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE Vga16Fb_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE Vga16Fb_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE Vga16Fb_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE Vga16Fb_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE Vga16Fb_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE Vga16Fb_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE Vga16Fb_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gVga16FbVtbl = {
    .QueryInterface     = Vga16Fb_QueryInterface,
    .AddRef             = Vga16Fb_AddRef,
    .Release            = Vga16Fb_Release,
    .Initialize         = Vga16Fb_Initialize,
    .Clear              = Vga16Fb_Clear,
    .SetPixel           = Vga16Fb_SetPixel,
    .GetPixel           = Vga16Fb_GetPixel,
    .FillRect           = Vga16Fb_FillRect,
    .BlitMonoBitmap     = Vga16Fb_BlitMonoBitmap,
    .BlitBitmap         = Vga16Fb_BlitBitmap,
    .GetDescriptor      = Vga16Fb_GetDescriptor,
    .SetDitherMethod    = Vga16Fb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  VGA I/O Helper Functions                                        */
/* --------------------------------------------------------------- */

#if defined(__i386__) || defined(__x86_64__)

static INLINE VOID
outb(UINT16 port, UINT8 val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static INLINE UINT8
inb(UINT16 port)
{
    UINT8 ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static INLINE VOID
Vga16_WriteSeq(UINT8 reg, UINT8 val)
{
    outb(VGA_SEQ_INDEX, reg);
    outb(VGA_SEQ_DATA, val);
}

static INLINE VOID
Vga16_WriteGC(UINT8 reg, UINT8 val)
{
    outb(VGA_GC_INDEX, reg);
    outb(VGA_GC_DATA, val);
}

static INLINE UINT8
Vga16_ReadGC(UINT8 reg)
{
    outb(VGA_GC_INDEX, reg);
    return inb(VGA_GC_DATA);
}

#else

/* Stub implementations for non-x86 architectures */
static INLINE VOID Vga16_WriteSeq(UINT8 reg, UINT8 val) { (void)reg; (void)val; }
static INLINE VOID Vga16_WriteGC(UINT8 reg, UINT8 val) { (void)reg; (void)val; }
static INLINE UINT8 Vga16_ReadGC(UINT8 reg) { (void)reg; return 0; }

#endif

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static INLINE VOID
Vga16Fb_WritePixel(
    VGA16_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT8 ColorIndex
    )
{
    UINT32 Offset;
    UINT8 Mask;
    UINT8 Plane;

    Offset = Y * (Backend->Descriptor.Width / 8) + (X / 8);
    Mask = 0x80 >> (X & 7);

    /* Write to all 4 planes based on color index */
    for (Plane = 0; Plane < 4; Plane++) {
        UINT8 BitValue = (ColorIndex >> Plane) & 1;

        /* Select plane to write */
        Vga16_WriteSeq(VGA_SEQ_MAP_MASK, 1 << Plane);

        /* Read-modify-write */
        if (BitValue) {
            Backend->VideoMemory[Offset] |= Mask;
        } else {
            Backend->VideoMemory[Offset] &= ~Mask;
        }
    }

    /* Restore all planes */
    Vga16_WriteSeq(VGA_SEQ_MAP_MASK, 0x0F);
}

static INLINE UINT8
Vga16Fb_ReadPixel(
    VGA16_FB_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    UINT32 Offset;
    UINT8 Mask;
    UINT8 Plane;
    UINT8 ColorIndex = 0;

    Offset = Y * (Backend->Descriptor.Width / 8) + (X / 8);
    Mask = 0x80 >> (X & 7);

    /* Read from all 4 planes */
    for (Plane = 0; Plane < 4; Plane++) {
        /* Select plane to read */
        Vga16_WriteGC(VGA_GC_READ_MAP, Plane);

        if (Backend->VideoMemory[Offset] & Mask) {
            ColorIndex |= (1 << Plane);
        }
    }

    return ColorIndex;
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
Vga16Fb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    VGA16_FB_BACKEND *Backend = (VGA16_FB_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        Vga16Fb_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
Vga16Fb_AddRef(
    IFramebufferBackend *This
    )
{
    VGA16_FB_BACKEND *Backend = (VGA16_FB_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
Vga16Fb_Release(
    IFramebufferBackend *This
    )
{
    VGA16_FB_BACKEND *Backend = (VGA16_FB_BACKEND *)This;
    return ANX_REF_DEC(&Backend->RefCount);
}

/* --------------------------------------------------------------- */
/*  IFramebufferBackend Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
Vga16Fb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    VGA16_FB_BACKEND *Backend = (VGA16_FB_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    Backend->Descriptor = *Descriptor;
    Backend->VideoMemory = (UINT8 *)(UINTN)VGA_BASE_ADDR;
    Backend->Initialized = TRUE;

    /* Set up VGA for planar mode */
    Vga16_WriteSeq(VGA_SEQ_MAP_MASK, 0x0F);  /* Enable all planes */
    Vga16_WriteGC(VGA_GC_MODE, 0x00);        /* Write mode 0 */

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Vga16Fb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    VGA16_FB_BACKEND *Backend = (VGA16_FB_BACKEND *)This;
    UINT8 ColorIndex;
    UINT8 Plane;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    ColorIndex = FbGetVga16Color(Color);

    /* Clear each plane individually */
    for (Plane = 0; Plane < 4; Plane++) {
        UINT8 FillByte = ((ColorIndex >> Plane) & 1) ? 0xFF : 0x00;
        UINT32 i;

        Vga16_WriteSeq(VGA_SEQ_MAP_MASK, 1 << Plane);

        for (i = 0; i < VGA_MEM_SIZE; i++) {
            Backend->VideoMemory[i] = FillByte;
        }
    }

    Vga16_WriteSeq(VGA_SEQ_MAP_MASK, 0x0F);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Vga16Fb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    VGA16_FB_BACKEND *Backend = (VGA16_FB_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    ColorIndex = FbGetVga16Color(Color);
    Vga16Fb_WritePixel(Backend, X, Y, ColorIndex);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Vga16Fb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    VGA16_FB_BACKEND *Backend = (VGA16_FB_BACKEND *)This;
    UINT8 ColorIndex;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    ColorIndex = Vga16Fb_ReadPixel(Backend, X, Y);
    *Color = FbUnpackPixel(ColorIndex, FbPixelFormatVga16Planar, 0, 0, 0);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Vga16Fb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    VGA16_FB_BACKEND *Backend = (VGA16_FB_BACKEND *)This;
    INT32 x, y;
    UINT8 ColorIndex;

    if (!Backend->Initialized || Rect == NULL) {
        return E_POINTER;
    }

    ColorIndex = FbGetVga16Color(Color);

    for (y = Rect->Top; y < Rect->Bottom && y < (INT32)Backend->Descriptor.Height; y++) {
        for (x = Rect->Left; x < Rect->Right && x < (INT32)Backend->Descriptor.Width; x++) {
            if (x >= 0 && y >= 0) {
                Vga16Fb_WritePixel(Backend, x, y, ColorIndex);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Vga16Fb_BlitMonoBitmap(
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
    VGA16_FB_BACKEND *Backend = (VGA16_FB_BACKEND *)This;
    UINT32 Row, Col;
    UINT32 ByteIndex, BitIndex;
    UINT8 FgIndex, BgIndex;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    FgIndex = FbGetVga16Color(Foreground);
    BgIndex = FbGetVga16Color(Background);

    for (Row = 0; Row < Height; Row++) {
        for (Col = 0; Col < Width; Col++) {
            INT32 Px = X + Col;
            INT32 Py = Y + Row;

            if (Px >= 0 && Px < (INT32)Backend->Descriptor.Width &&
                Py >= 0 && Py < (INT32)Backend->Descriptor.Height) {

                ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
                BitIndex = 7 - (Col % 8);

                if (Bitmap[ByteIndex] & (1 << BitIndex)) {
                    Vga16Fb_WritePixel(Backend, Px, Py, FgIndex);
                } else {
                    Vga16Fb_WritePixel(Backend, Px, Py, BgIndex);
                }
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Vga16Fb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    /* Not implemented yet */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
Vga16Fb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    VGA16_FB_BACKEND *Backend = (VGA16_FB_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
Vga16Fb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    /* VGA16 doesn't use dithering */
    (void)This;
    (void)Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static VGA16_FB_BACKEND gVga16BackendInstance = {
    .Base.lpVtbl        = &gVga16FbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
};

IFramebufferBackend *
FbCreateVga16Backend(
    VOID
    )
{
    return (IFramebufferBackend *)&gVga16BackendInstance;
}
