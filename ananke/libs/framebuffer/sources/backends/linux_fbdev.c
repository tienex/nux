/*++
    Module Name:

        linux_fbdev.c

    Abstract:

        Linux/BSD framebuffer device (fbdev) backend.
        Interfaces with /dev/fb0 and similar framebuffer devices.

        This backend uses runtime function pointers to avoid direct
        library dependencies. The actual I/O functions are provided
        by the platform layer.

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/backend_ext.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Linux fbdev Structures (simplified, no actual includes)         */
/* --------------------------------------------------------------- */

/* Simplified fb_fix_screeninfo structure */
typedef struct _FBDEV_FIX_SCREENINFO {
    CHAR8   Id[16];             /* Identification string */
    UINT64  SmemStart;          /* Start of frame buffer mem (physical address) */
    UINT32  SmemLen;            /* Length of frame buffer mem */
    UINT32  Type;               /* see FB_TYPE_* */
    UINT32  Visual;             /* see FB_VISUAL_* */
    UINT16  Xpanstep;           /* zero if no hardware panning */
    UINT16  Ypanstep;           /* zero if no hardware panning */
    UINT32  LineLength;         /* length of a line in bytes */
} FBDEV_FIX_SCREENINFO;

/* Simplified fb_var_screeninfo structure */
typedef struct _FBDEV_VAR_SCREENINFO {
    UINT32  Xres;               /* visible resolution */
    UINT32  Yres;
    UINT32  XresVirtual;        /* virtual resolution */
    UINT32  YresVirtual;
    UINT32  BitsPerPixel;       /* guess what */

    /* RGB bit field positions */
    struct {
        UINT32 Offset;          /* beginning of bitfield */
        UINT32 Length;          /* length of bitfield */
    } Red, Green, Blue, Transp;

    UINT32  Grayscale;          /* != 0 Graylevels instead of colors */
    UINT32  Activate;           /* see FB_ACTIVATE_* */
    UINT32  Height;             /* height of picture in mm */
    UINT32  Width;              /* width of picture in mm */
} FBDEV_VAR_SCREENINFO;

/* ioctl commands */
#define FBIOGET_FSCREENINFO 0x4602
#define FBIOGET_VSCREENINFO 0x4600
#define FBIOPUT_VSCREENINFO 0x4601
#define FBIOPAN_DISPLAY     0x4606
#define FBIO_WAITFORVSYNC   0x4620

/* --------------------------------------------------------------- */
/*  Function Pointer Interface (provided by platform)               */
/* --------------------------------------------------------------- */

typedef struct _FBDEV_INTERFACE {
    /* File operations */
    INT32 (*Open)(CONST CHAR8 *Path, INT32 Flags);
    INT32 (*Close)(INT32 Fd);
    INT32 (*Ioctl)(INT32 Fd, UINT64 Request, VOID *Arg);
    VOID* (*Mmap)(VOID *Addr, UINTN Length, INT32 Prot, INT32 Flags, INT32 Fd, UINT64 Offset);
    INT32 (*Munmap)(VOID *Addr, UINTN Length);

    /* Optional: VSync waiting */
    INT32 (*WaitVSync)(INT32 Fd);
} FBDEV_INTERFACE;

/* --------------------------------------------------------------- */
/*  Linux fbdev Backend Structure                                   */
/* --------------------------------------------------------------- */

typedef struct _FBDEV_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;

    /* fbdev state */
    INT32                       FbFd;           /* File descriptor for /dev/fbX */
    VOID                        *MappedMemory;  /* mmap'd framebuffer */
    UINT8                       *FramebufferBase;

    FBDEV_FIX_SCREENINFO        FixInfo;
    FBDEV_VAR_SCREENINFO        VarInfo;

    /* Function interface */
    FBDEV_INTERFACE             *Interface;

    /* Current page for page flipping */
    UINT32                      ActivePage;
    UINT32                      VisiblePage;
} FBDEV_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE FbdevFb_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE FbdevFb_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE FbdevFb_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE FbdevFb_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE FbdevFb_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE FbdevFb_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE FbdevFb_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE FbdevFb_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE FbdevFb_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE FbdevFb_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE FbdevFb_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE FbdevFb_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gFbdevFbVtbl = {
    .QueryInterface     = FbdevFb_QueryInterface,
    .AddRef             = FbdevFb_AddRef,
    .Release            = FbdevFb_Release,
    .Initialize         = FbdevFb_Initialize,
    .Clear              = FbdevFb_Clear,
    .SetPixel           = FbdevFb_SetPixel,
    .GetPixel           = FbdevFb_GetPixel,
    .FillRect           = FbdevFb_FillRect,
    .BlitMonoBitmap     = FbdevFb_BlitMonoBitmap,
    .BlitBitmap         = FbdevFb_BlitBitmap,
    .GetDescriptor      = FbdevFb_GetDescriptor,
    .SetDitherMethod    = FbdevFb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static FB_PIXEL_FORMAT
FbdevFb_DeterminePixelFormat(
    FBDEV_VAR_SCREENINFO *VarInfo
    )
{
    UINT32 Bpp = VarInfo->BitsPerPixel;

    if (Bpp == 32 && VarInfo->Red.Length == 8 && VarInfo->Green.Length == 8 && VarInfo->Blue.Length == 8) {
        if (VarInfo->Red.Offset == 16) {
            return FbPixelFormatRgba8888;  /* RGB888 or RGBA8888 */
        } else {
            return FbPixelFormatBgra8888;  /* BGR888 or BGRA8888 */
        }
    } else if (Bpp == 24) {
        return FbPixelFormatRgb888;
    } else if (Bpp == 16) {
        if (VarInfo->Green.Length == 6) {
            return FbPixelFormatRgb565;
        } else {
            return FbPixelFormatRgb555;
        }
    } else if (Bpp == 8) {
        return FbPixelFormatIndexed256;
    } else if (Bpp == 4) {
        return FbPixelFormatIndexed16;
    } else if (Bpp == 1) {
        return FbPixelFormat1Bpp;
    }

    return FbPixelFormatInvalid;
}

static INLINE VOID
FbdevFb_WritePixel(
    FBDEV_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    UINT32 PixelValue
    )
{
    UINT32 BytesPerPixel = Backend->Descriptor.PixelFormat >= FbPixelFormatRgba8888 ? 4 :
                           Backend->Descriptor.PixelFormat >= FbPixelFormatRgb888 ? 3 :
                           Backend->Descriptor.PixelFormat >= FbPixelFormatRgb555 ? 2 : 1;

    UINT32 Offset = Y * Backend->FixInfo.LineLength + X * BytesPerPixel;
    UINT8 *Addr = Backend->FramebufferBase + Offset;

    switch (BytesPerPixel) {
        case 4:
            *(UINT32 *)Addr = PixelValue;
            break;
        case 3:
            Addr[0] = (PixelValue >> 0) & 0xFF;
            Addr[1] = (PixelValue >> 8) & 0xFF;
            Addr[2] = (PixelValue >> 16) & 0xFF;
            break;
        case 2:
            *(UINT16 *)Addr = (UINT16)PixelValue;
            break;
        case 1:
            *Addr = (UINT8)PixelValue;
            break;
    }
}

static INLINE UINT32
FbdevFb_ReadPixel(
    FBDEV_BACKEND *Backend,
    INT32 X,
    INT32 Y
    )
{
    UINT32 BytesPerPixel = Backend->Descriptor.PixelFormat >= FbPixelFormatRgba8888 ? 4 :
                           Backend->Descriptor.PixelFormat >= FbPixelFormatRgb888 ? 3 :
                           Backend->Descriptor.PixelFormat >= FbPixelFormatRgb555 ? 2 : 1;

    UINT32 Offset = Y * Backend->FixInfo.LineLength + X * BytesPerPixel;
    UINT8 *Addr = Backend->FramebufferBase + Offset;

    switch (BytesPerPixel) {
        case 4:
            return *(UINT32 *)Addr;
        case 3:
            return (Addr[0] << 0) | (Addr[1] << 8) | (Addr[2] << 16);
        case 2:
            return *(UINT16 *)Addr;
        case 1:
            return *Addr;
        default:
            return 0;
    }
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbdevFb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    FBDEV_BACKEND *Backend = (FBDEV_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        FbdevFb_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
FbdevFb_AddRef(
    IFramebufferBackend *This
    )
{
    FBDEV_BACKEND *Backend = (FBDEV_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
FbdevFb_Release(
    IFramebufferBackend *This
    )
{
    FBDEV_BACKEND *Backend = (FBDEV_BACKEND *)This;
    UINT32 RefCount = ANX_REF_DEC(&Backend->RefCount);

    if (RefCount == 0) {
        /* Cleanup - munmap and close */
        if (Backend->Interface && Backend->MappedMemory) {
            Backend->Interface->Munmap(Backend->MappedMemory, Backend->FixInfo.SmemLen);
        }
        if (Backend->Interface && Backend->FbFd >= 0) {
            Backend->Interface->Close(Backend->FbFd);
        }
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  IFramebufferBackend Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbdevFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    FBDEV_BACKEND *Backend = (FBDEV_BACKEND *)This;

    if (Descriptor == NULL || Backend->Interface == NULL) {
        return E_POINTER;
    }

    /* Open framebuffer device */
    Backend->FbFd = Backend->Interface->Open("/dev/fb0", 2); /* O_RDWR = 2 */
    if (Backend->FbFd < 0) {
        return E_FAIL;
    }

    /* Get fixed screen info */
    if (Backend->Interface->Ioctl(Backend->FbFd, FBIOGET_FSCREENINFO, &Backend->FixInfo) < 0) {
        Backend->Interface->Close(Backend->FbFd);
        return E_FAIL;
    }

    /* Get variable screen info */
    if (Backend->Interface->Ioctl(Backend->FbFd, FBIOGET_VSCREENINFO, &Backend->VarInfo) < 0) {
        Backend->Interface->Close(Backend->FbFd);
        return E_FAIL;
    }

    /* Map framebuffer memory */
    /* PROT_READ|PROT_WRITE = 3, MAP_SHARED = 1 */
    Backend->MappedMemory = Backend->Interface->Mmap(NULL, Backend->FixInfo.SmemLen,
                                                      3, 1, Backend->FbFd, 0);
    if (Backend->MappedMemory == (VOID *)-1) {
        Backend->Interface->Close(Backend->FbFd);
        return E_FAIL;
    }

    /* Fill in descriptor */
    Backend->Descriptor.Width = Backend->VarInfo.Xres;
    Backend->Descriptor.Height = Backend->VarInfo.Yres;
    Backend->Descriptor.Pitch = Backend->FixInfo.LineLength;
    Backend->Descriptor.PhysicalBase = Backend->FixInfo.SmemStart;
    Backend->Descriptor.Size = Backend->FixInfo.SmemLen;
    Backend->Descriptor.PixelFormat = FbdevFb_DeterminePixelFormat(&Backend->VarInfo);

    /* RGB masks */
    Backend->Descriptor.RedMask = ((1 << Backend->VarInfo.Red.Length) - 1) << Backend->VarInfo.Red.Offset;
    Backend->Descriptor.GreenMask = ((1 << Backend->VarInfo.Green.Length) - 1) << Backend->VarInfo.Green.Offset;
    Backend->Descriptor.BlueMask = ((1 << Backend->VarInfo.Blue.Length) - 1) << Backend->VarInfo.Blue.Offset;

    Backend->FramebufferBase = (UINT8 *)Backend->MappedMemory;
    Backend->Initialized = TRUE;
    Backend->ActivePage = 0;
    Backend->VisiblePage = 0;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbdevFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    FBDEV_BACKEND *Backend = (FBDEV_BACKEND *)This;
    UINT32 PixelValue;
    UINT32 x, y;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    PixelValue = FbPackPixel(Color, Backend->Descriptor.PixelFormat,
                             Backend->Descriptor.RedMask,
                             Backend->Descriptor.GreenMask,
                             Backend->Descriptor.BlueMask);

    for (y = 0; y < Backend->Descriptor.Height; y++) {
        for (x = 0; x < Backend->Descriptor.Width; x++) {
            FbdevFb_WritePixel(Backend, x, y, PixelValue);
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbdevFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    FBDEV_BACKEND *Backend = (FBDEV_BACKEND *)This;
    UINT32 PixelValue;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    PixelValue = FbPackPixel(Color, Backend->Descriptor.PixelFormat,
                             Backend->Descriptor.RedMask,
                             Backend->Descriptor.GreenMask,
                             Backend->Descriptor.BlueMask);

    FbdevFb_WritePixel(Backend, X, Y, PixelValue);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbdevFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    FBDEV_BACKEND *Backend = (FBDEV_BACKEND *)This;
    UINT32 PixelValue;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= (INT32)Backend->Descriptor.Width ||
        Y < 0 || Y >= (INT32)Backend->Descriptor.Height) {
        return E_INVALIDARG;
    }

    /* Read raw pixel value from framebuffer */
    PixelValue = FbdevFb_ReadPixel(Backend, X, Y);

    /* Unpack to FB_COLOR */
    *Color = FbUnpackPixel(PixelValue, Backend->Descriptor.PixelFormat,
                           Backend->Descriptor.RedMask,
                           Backend->Descriptor.GreenMask,
                           Backend->Descriptor.BlueMask);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbdevFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    FBDEV_BACKEND *Backend = (FBDEV_BACKEND *)This;
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
                FbdevFb_WritePixel(Backend, x, y, PixelValue);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbdevFb_BlitMonoBitmap(
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
    FBDEV_BACKEND *Backend = (FBDEV_BACKEND *)This;
    UINT32 FgPixel, BgPixel;
    UINT32 Row, Col;
    UINT32 ByteIndex, BitIndex;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    FgPixel = FbPackPixel(Foreground, Backend->Descriptor.PixelFormat,
                          Backend->Descriptor.RedMask, Backend->Descriptor.GreenMask,
                          Backend->Descriptor.BlueMask);

    BgPixel = FbPackPixel(Background, Backend->Descriptor.PixelFormat,
                          Backend->Descriptor.RedMask, Backend->Descriptor.GreenMask,
                          Backend->Descriptor.BlueMask);

    for (Row = 0; Row < Height; Row++) {
        for (Col = 0; Col < Width; Col++) {
            ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
            BitIndex = 7 - (Col % 8);

            UINT32 PixelValue = (Bitmap[ByteIndex] & (1 << BitIndex)) ? FgPixel : BgPixel;
            FbdevFb_WritePixel(Backend, X + Col, Y + Row, PixelValue);
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbdevFb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    FBDEV_BACKEND *Backend = (FBDEV_BACKEND *)This;

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
            /* Direct row-by-row copy using framebuffer pitch */
            for (UINT32 Row = 0; Row < Height; Row++) {
                INT32 DestY = Y + Row;
                if (DestY < 0 || DestY >= (INT32)Backend->Descriptor.Height) {
                    continue;
                }

                UINT32 DestOffset = DestY * Backend->FixInfo.LineLength + X * BytesPerPixel;
                UINT32 SrcOffset = Row * Width * BytesPerPixel;
                UINT32 CopyWidth = Width * BytesPerPixel;

                /* Bounds check for destination */
                if (X >= 0 && (X + Width) <= Backend->Descriptor.Width) {
                    /* Simple memory copy for unclipped case */
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

    /* Other formats - let engine handle conversion */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
FbdevFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    FBDEV_BACKEND *Backend = (FBDEV_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbdevFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    FBDEV_BACKEND *Backend = (FBDEV_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static FBDEV_BACKEND gFbdevBackendInstance = {
    .Base.lpVtbl        = &gFbdevFbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherNone,
    .FbFd               = -1,
    .MappedMemory       = NULL,
    .Interface          = NULL,
};

IFramebufferBackend *
FbCreateLinuxFbdevBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gFbdevBackendInstance;
}

/*
 * Set the fbdev interface functions.
 * Must be called before Initialize().
 */
VOID
FbLinuxFbdevSetInterface(
    IN IFramebufferBackend *Backend,
    IN FBDEV_INTERFACE *Interface
    )
{
    FBDEV_BACKEND *FbdevBackend = (FBDEV_BACKEND *)Backend;
    FbdevBackend->Interface = Interface;
}
