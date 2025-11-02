/*++
    Module Name:

        uefi_uga.c

    Abstract:

        UEFI UGA (Universal Graphics Adapter) protocol backend.

        UGA was the predecessor to GOP (Graphics Output Protocol) used in
        EFI 1.x and early UEFI 2.x implementations. This backend provides
        compatibility with older UEFI systems and some Mac EFI implementations.

        The UGA protocol provides:
        - GetMode(): Query current mode and framebuffer address
        - SetMode(): Set video mode
        - Blt(): Block transfer operations (fill, video fill, buffer to video, video to buffer)

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/backend_ext.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/framebuffer/dither.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>

/* --------------------------------------------------------------- */
/*  UGA Protocol Definitions (no UEFI headers required)             */
/* --------------------------------------------------------------- */

/* UGA Protocol GUID: 982c298b-f4fa-41cb-b838-77aa688fb839 */
#define EFI_UGA_DRAW_PROTOCOL_GUID \
    { 0x982c298b, 0xf4fa, 0x41cb, \
      { 0xb8, 0x38, 0x77, 0xaa, 0x68, 0x8f, 0xb8, 0x39 } }

typedef VOID* EFI_HANDLE;
typedef UINTN EFI_STATUS;

#define EFI_SUCCESS             0
#define EFI_INVALID_PARAMETER   2
#define EFI_UNSUPPORTED         3
#define EFI_DEVICE_ERROR        7
#define EFI_NOT_READY           6

/* UGA Blt Operations */
typedef enum {
    EfiUgaVideoFill,        /* Write data from BltBuffer to every pixel of video display */
    EfiUgaVideoToBltBuffer, /* Read data from video display to BltBuffer */
    EfiUgaBltBufferToVideo, /* Write data from BltBuffer to video display */
    EfiUgaVideoToVideo,     /* Copy from video display to video display */
    EfiUgaBltMax
} EFI_UGA_BLT_OPERATION;

/* UGA Pixel Format */
typedef struct {
    UINT8 Blue;
    UINT8 Green;
    UINT8 Red;
    UINT8 Reserved;
} EFI_UGA_PIXEL;

/* Forward declaration of UGA protocol */
typedef struct _EFI_UGA_DRAW_PROTOCOL EFI_UGA_DRAW_PROTOCOL;

/* UGA Protocol Interface */
typedef EFI_STATUS (EFIAPI *EFI_UGA_DRAW_PROTOCOL_GET_MODE)(
    IN  EFI_UGA_DRAW_PROTOCOL *This,
    OUT UINT32                *HorizontalResolution,
    OUT UINT32                *VerticalResolution,
    OUT UINT32                *ColorDepth,
    OUT UINT32                *RefreshRate
    );

typedef EFI_STATUS (EFIAPI *EFI_UGA_DRAW_PROTOCOL_SET_MODE)(
    IN EFI_UGA_DRAW_PROTOCOL *This,
    IN UINT32                HorizontalResolution,
    IN UINT32                VerticalResolution,
    IN UINT32                ColorDepth,
    IN UINT32                RefreshRate
    );

typedef EFI_STATUS (EFIAPI *EFI_UGA_DRAW_PROTOCOL_BLT)(
    IN EFI_UGA_DRAW_PROTOCOL *This,
    IN EFI_UGA_PIXEL         *BltBuffer OPTIONAL,
    IN EFI_UGA_BLT_OPERATION BltOperation,
    IN UINTN                 SourceX,
    IN UINTN                 SourceY,
    IN UINTN                 DestinationX,
    IN UINTN                 DestinationY,
    IN UINTN                 Width,
    IN UINTN                 Height,
    IN UINTN                 Delta OPTIONAL
    );

struct _EFI_UGA_DRAW_PROTOCOL {
    EFI_UGA_DRAW_PROTOCOL_GET_MODE GetMode;
    EFI_UGA_DRAW_PROTOCOL_SET_MODE SetMode;
    EFI_UGA_DRAW_PROTOCOL_BLT      Blt;
};

/* --------------------------------------------------------------- */
/*  UGA Backend Structure                                           */
/* --------------------------------------------------------------- */

typedef struct _UGA_BACKEND {
    IFramebufferBackend         Base;
    REFOBJ                      RefCount;
    FRAMEBUFFER_DESC            Descriptor;
    FB_DITHER_METHOD            DitherMethod;
    BOOLEAN                     Initialized;

    /* UGA protocol */
    EFI_UGA_DRAW_PROTOCOL       *UgaProtocol;

    /* Current mode */
    UINT32                      HorizontalResolution;
    UINT32                      VerticalResolution;
    UINT32                      ColorDepth;
    UINT32                      RefreshRate;

    /* Framebuffer info (UGA doesn't provide direct access) */
    UINT8                       *FramebufferBase;
    BOOLEAN                     HasDirectAccess;

    /* Software buffer for systems without direct framebuffer access */
    EFI_UGA_PIXEL               *SoftwareBuffer;
} UGA_BACKEND;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE UgaFb_QueryInterface(
    IFramebufferBackend *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE UgaFb_AddRef(IFramebufferBackend *This);
static UINT32 STDMETHODCALLTYPE UgaFb_Release(IFramebufferBackend *This);
static HRESULT STDMETHODCALLTYPE UgaFb_Initialize(
    IFramebufferBackend *This, CONST FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE UgaFb_Clear(
    IFramebufferBackend *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE UgaFb_SetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE UgaFb_GetPixel(
    IFramebufferBackend *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE UgaFb_FillRect(
    IFramebufferBackend *This, CONST FB_RECT *Rect, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE UgaFb_BlitMonoBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_COLOR Foreground, FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE UgaFb_BlitBitmap(
    IFramebufferBackend *This, INT32 X, INT32 Y, UINT32 Width, UINT32 Height,
    CONST UINT8 *Bitmap, FB_PIXEL_FORMAT SourceFormat);
static HRESULT STDMETHODCALLTYPE UgaFb_GetDescriptor(
    IFramebufferBackend *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE UgaFb_SetDitherMethod(
    IFramebufferBackend *This, FB_DITHER_METHOD Method);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferBackendVtbl gUgaFbVtbl = {
    .QueryInterface     = UgaFb_QueryInterface,
    .AddRef             = UgaFb_AddRef,
    .Release            = UgaFb_Release,
    .Initialize         = UgaFb_Initialize,
    .Clear              = UgaFb_Clear,
    .SetPixel           = UgaFb_SetPixel,
    .GetPixel           = UgaFb_GetPixel,
    .FillRect           = UgaFb_FillRect,
    .BlitMonoBitmap     = UgaFb_BlitMonoBitmap,
    .BlitBitmap         = UgaFb_BlitBitmap,
    .GetDescriptor      = UgaFb_GetDescriptor,
    .SetDitherMethod    = UgaFb_SetDitherMethod,
};

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static INLINE EFI_UGA_PIXEL
UgaFb_ColorToUgaPixel(
    FB_COLOR Color
    )
{
    EFI_UGA_PIXEL Pixel;
    Pixel.Blue = Color.Blue;
    Pixel.Green = Color.Green;
    Pixel.Red = Color.Red;
    Pixel.Reserved = 0;
    return Pixel;
}

static INLINE FB_COLOR
UgaFb_UgaPixelToColor(
    EFI_UGA_PIXEL Pixel
    )
{
    FB_COLOR Color;
    Color.Blue = Pixel.Blue;
    Color.Green = Pixel.Green;
    Color.Red = Pixel.Red;
    Color.Alpha = 255;
    return Color;
}

static HRESULT
UgaFb_FlushSoftwareBuffer(
    UGA_BACKEND *Backend
    )
{
    EFI_STATUS Status;

    if (!Backend->SoftwareBuffer || !Backend->UgaProtocol) {
        return E_FAIL;
    }

    /* Blit entire buffer to screen */
    Status = Backend->UgaProtocol->Blt(
        Backend->UgaProtocol,
        Backend->SoftwareBuffer,
        EfiUgaBltBufferToVideo,
        0, 0,                           /* Source X, Y */
        0, 0,                           /* Destination X, Y */
        Backend->HorizontalResolution,
        Backend->VerticalResolution,
        0                               /* Delta (0 = tightly packed) */
    );

    return (Status == EFI_SUCCESS) ? S_OK : E_FAIL;
}

static INLINE VOID
UgaFb_WritePixelToBuffer(
    UGA_BACKEND *Backend,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    if (Backend->SoftwareBuffer) {
        UINT32 Offset = Y * Backend->HorizontalResolution + X;
        Backend->SoftwareBuffer[Offset] = UgaFb_ColorToUgaPixel(Color);
    } else if (Backend->HasDirectAccess && Backend->FramebufferBase) {
        /* Direct framebuffer access (if available) */
        UINT32 Offset = Y * Backend->Descriptor.Pitch + X * 4; /* Assuming 32bpp */
        UINT32 *Pixel = (UINT32 *)(Backend->FramebufferBase + Offset);
        *Pixel = (Color.Red << 16) | (Color.Green << 8) | Color.Blue;
    }
}

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
UgaFb_QueryInterface(
    IFramebufferBackend *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    UGA_BACKEND *Backend = (UGA_BACKEND *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferBackend)) {
        *ppvObject = &Backend->Base;
        UgaFb_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
UgaFb_AddRef(
    IFramebufferBackend *This
    )
{
    UGA_BACKEND *Backend = (UGA_BACKEND *)This;
    return ANX_REF_INC(&Backend->RefCount);
}

static UINT32 STDMETHODCALLTYPE
UgaFb_Release(
    IFramebufferBackend *This
    )
{
    UGA_BACKEND *Backend = (UGA_BACKEND *)This;
    UINT32 RefCount = ANX_REF_DEC(&Backend->RefCount);

    if (RefCount == 0) {
        /* Cleanup - would free software buffer if allocated */
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  IFramebufferBackend Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
UgaFb_Initialize(
    IFramebufferBackend *This,
    CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    UGA_BACKEND *Backend = (UGA_BACKEND *)This;
    EFI_STATUS Status;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    if (Backend->UgaProtocol == NULL) {
        return E_FAIL;
    }

    /* Get current mode */
    Status = Backend->UgaProtocol->GetMode(
        Backend->UgaProtocol,
        &Backend->HorizontalResolution,
        &Backend->VerticalResolution,
        &Backend->ColorDepth,
        &Backend->RefreshRate
    );

    if (Status != EFI_SUCCESS) {
        return E_FAIL;
    }

    /* Fill in descriptor */
    Backend->Descriptor.Width = Backend->HorizontalResolution;
    Backend->Descriptor.Height = Backend->VerticalResolution;
    Backend->Descriptor.PixelFormat = FbPixelFormatRgba8888; /* UGA is always 32-bit BGRA */
    Backend->Descriptor.PhysicalBase = Descriptor->PhysicalBase;
    Backend->Descriptor.Size = Backend->HorizontalResolution * Backend->VerticalResolution * 4;
    Backend->Descriptor.Pitch = Backend->HorizontalResolution * 4;

    /* RGB masks for 32-bit BGRA */
    Backend->Descriptor.RedMask = 0x00FF0000;
    Backend->Descriptor.GreenMask = 0x0000FF00;
    Backend->Descriptor.BlueMask = 0x000000FF;

    /* Check if we have direct framebuffer access */
    if (Descriptor->PhysicalBase != 0) {
        Backend->FramebufferBase = (UINT8 *)(UINTN)Descriptor->PhysicalBase;
        Backend->HasDirectAccess = TRUE;
    } else {
        /* Allocate software buffer for systems without direct access */
        Backend->HasDirectAccess = FALSE;

        /* Static allocation for software buffer */
        static EFI_UGA_PIXEL sSoftwareBuffer[1920 * 1200]; /* Max common resolution */
        Backend->SoftwareBuffer = sSoftwareBuffer;
    }

    Backend->Initialized = TRUE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
UgaFb_Clear(
    IFramebufferBackend *This,
    FB_COLOR Color
    )
{
    UGA_BACKEND *Backend = (UGA_BACKEND *)This;
    EFI_STATUS Status;
    EFI_UGA_PIXEL UgaPixel;

    if (!Backend->Initialized || !Backend->UgaProtocol) {
        return E_FAIL;
    }

    UgaPixel = UgaFb_ColorToUgaPixel(Color);

    /* Use UGA Blt to fill the entire screen */
    Status = Backend->UgaProtocol->Blt(
        Backend->UgaProtocol,
        &UgaPixel,
        EfiUgaVideoFill,
        0, 0,                           /* Source X, Y (ignored for VideoFill) */
        0, 0,                           /* Destination X, Y */
        Backend->HorizontalResolution,
        Backend->VerticalResolution,
        0                               /* Delta */
    );

    if (Backend->SoftwareBuffer) {
        /* Update software buffer */
        UINT32 PixelCount = Backend->HorizontalResolution * Backend->VerticalResolution;
        for (UINT32 i = 0; i < PixelCount; i++) {
            Backend->SoftwareBuffer[i] = UgaPixel;
        }
    }

    return (Status == EFI_SUCCESS) ? S_OK : E_FAIL;
}

static HRESULT STDMETHODCALLTYPE
UgaFb_SetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    UGA_BACKEND *Backend = (UGA_BACKEND *)This;
    EFI_UGA_PIXEL UgaPixel;
    EFI_STATUS Status;

    if (!Backend->Initialized) {
        return E_FAIL;
    }

    if (X < 0 || X >= (INT32)Backend->HorizontalResolution ||
        Y < 0 || Y >= (INT32)Backend->VerticalResolution) {
        return E_INVALIDARG;
    }

    UgaPixel = UgaFb_ColorToUgaPixel(Color);

    if (Backend->HasDirectAccess) {
        /* Direct framebuffer write */
        UgaFb_WritePixelToBuffer(Backend, X, Y, Color);
    } else if (Backend->SoftwareBuffer) {
        /* Update software buffer */
        UINT32 Offset = Y * Backend->HorizontalResolution + X;
        Backend->SoftwareBuffer[Offset] = UgaPixel;

        /* Flush this single pixel to screen */
        Status = Backend->UgaProtocol->Blt(
            Backend->UgaProtocol,
            &Backend->SoftwareBuffer[Offset],
            EfiUgaBltBufferToVideo,
            0, 0,                       /* Source X, Y */
            X, Y,                       /* Destination X, Y */
            1, 1,                       /* Width, Height */
            0                           /* Delta */
        );

        return (Status == EFI_SUCCESS) ? S_OK : E_FAIL;
    } else {
        /* Use UGA Blt to set single pixel */
        Status = Backend->UgaProtocol->Blt(
            Backend->UgaProtocol,
            &UgaPixel,
            EfiUgaBltBufferToVideo,
            0, 0,                       /* Source X, Y */
            X, Y,                       /* Destination X, Y */
            1, 1,                       /* Width, Height */
            0                           /* Delta */
        );

        return (Status == EFI_SUCCESS) ? S_OK : E_FAIL;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
UgaFb_GetPixel(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    UGA_BACKEND *Backend = (UGA_BACKEND *)This;
    EFI_UGA_PIXEL UgaPixel;
    EFI_STATUS Status;

    if (!Backend->Initialized || Color == NULL) {
        return E_POINTER;
    }

    if (X < 0 || X >= (INT32)Backend->HorizontalResolution ||
        Y < 0 || Y >= (INT32)Backend->VerticalResolution) {
        return E_INVALIDARG;
    }

    if (Backend->SoftwareBuffer) {
        /* Read from software buffer */
        UINT32 Offset = Y * Backend->HorizontalResolution + X;
        *Color = UgaFb_UgaPixelToColor(Backend->SoftwareBuffer[Offset]);
        return S_OK;
    } else if (Backend->UgaProtocol) {
        /* Use UGA Blt to read pixel */
        Status = Backend->UgaProtocol->Blt(
            Backend->UgaProtocol,
            &UgaPixel,
            EfiUgaVideoToBltBuffer,
            X, Y,                       /* Source X, Y */
            0, 0,                       /* Destination X, Y */
            1, 1,                       /* Width, Height */
            0                           /* Delta */
        );

        if (Status == EFI_SUCCESS) {
            *Color = UgaFb_UgaPixelToColor(UgaPixel);
            return S_OK;
        }
    }

    return E_FAIL;
}

static HRESULT STDMETHODCALLTYPE
UgaFb_FillRect(
    IFramebufferBackend *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color
    )
{
    UGA_BACKEND *Backend = (UGA_BACKEND *)This;
    EFI_UGA_PIXEL UgaPixel;
    EFI_STATUS Status;
    INT32 Width, Height;

    if (!Backend->Initialized || Rect == NULL || !Backend->UgaProtocol) {
        return E_POINTER;
    }

    Width = Rect->Right - Rect->Left;
    Height = Rect->Bottom - Rect->Top;

    if (Width <= 0 || Height <= 0) {
        return E_INVALIDARG;
    }

    UgaPixel = UgaFb_ColorToUgaPixel(Color);

    /* Use UGA VideoFill operation for hardware acceleration */
    Status = Backend->UgaProtocol->Blt(
        Backend->UgaProtocol,
        &UgaPixel,
        EfiUgaVideoFill,
        0, 0,                           /* Source X, Y (ignored) */
        Rect->Left, Rect->Top,          /* Destination X, Y */
        Width, Height,
        0                               /* Delta */
    );

    if (Backend->SoftwareBuffer) {
        /* Update software buffer */
        for (INT32 y = Rect->Top; y < Rect->Bottom; y++) {
            for (INT32 x = Rect->Left; x < Rect->Right; x++) {
                if (x >= 0 && x < (INT32)Backend->HorizontalResolution &&
                    y >= 0 && y < (INT32)Backend->VerticalResolution) {
                    UINT32 Offset = y * Backend->HorizontalResolution + x;
                    Backend->SoftwareBuffer[Offset] = UgaPixel;
                }
            }
        }
    }

    return (Status == EFI_SUCCESS) ? S_OK : E_FAIL;
}

static HRESULT STDMETHODCALLTYPE
UgaFb_BlitMonoBitmap(
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
    UGA_BACKEND *Backend = (UGA_BACKEND *)This;
    EFI_UGA_PIXEL FgPixel, BgPixel;
    EFI_UGA_PIXEL *BltBuffer;
    UINT32 Row, Col;
    UINT32 ByteIndex, BitIndex;
    EFI_STATUS Status;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    FgPixel = UgaFb_ColorToUgaPixel(Foreground);
    BgPixel = UgaFb_ColorToUgaPixel(Background);

    /* Allocate temporary blt buffer */
    static EFI_UGA_PIXEL sBltBuffer[128 * 128]; /* Static for common glyph sizes */
    BltBuffer = sBltBuffer;

    /* Convert bitmap to UGA pixel buffer */
    for (Row = 0; Row < Height; Row++) {
        for (Col = 0; Col < Width; Col++) {
            ByteIndex = Row * ((Width + 7) / 8) + (Col / 8);
            BitIndex = 7 - (Col % 8);

            BltBuffer[Row * Width + Col] =
                (Bitmap[ByteIndex] & (1 << BitIndex)) ? FgPixel : BgPixel;
        }
    }

    /* Blt buffer to screen */
    if (Backend->UgaProtocol) {
        Status = Backend->UgaProtocol->Blt(
            Backend->UgaProtocol,
            BltBuffer,
            EfiUgaBltBufferToVideo,
            0, 0,                       /* Source X, Y */
            X, Y,                       /* Destination X, Y */
            Width, Height,
            Width * sizeof(EFI_UGA_PIXEL) /* Delta */
        );

        if (Status != EFI_SUCCESS) {
            return E_FAIL;
        }
    }

    /* Update software buffer if present */
    if (Backend->SoftwareBuffer) {
        for (Row = 0; Row < Height; Row++) {
            for (Col = 0; Col < Width; Col++) {
                INT32 DestX = X + Col;
                INT32 DestY = Y + Row;
                if (DestX >= 0 && DestX < (INT32)Backend->HorizontalResolution &&
                    DestY >= 0 && DestY < (INT32)Backend->VerticalResolution) {
                    UINT32 Offset = DestY * Backend->HorizontalResolution + DestX;
                    Backend->SoftwareBuffer[Offset] = BltBuffer[Row * Width + Col];
                }
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
UgaFb_BlitBitmap(
    IFramebufferBackend *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    CONST UINT8 *Bitmap,
    FB_PIXEL_FORMAT SourceFormat
    )
{
    UGA_BACKEND *Backend = (UGA_BACKEND *)This;

    if (!Backend->Initialized || Bitmap == NULL) {
        return E_POINTER;
    }

    /* UGA backend may not have direct framebuffer access */
    /* For now, implement only direct framebuffer path */
    /* TODO: Use UGA Blt() protocol for systems without direct access */

    if (Backend->HasDirectAccess && Backend->FramebufferBase != NULL) {
        /* Fast path: matching pixel format with direct framebuffer access */
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
                /* Direct row-by-row copy */
                for (UINT32 Row = 0; Row < Height; Row++) {
                    INT32 DestY = Y + Row;
                    if (DestY < 0 || DestY >= (INT32)Backend->Descriptor.Height) {
                        continue;
                    }

                    UINT32 DestOffset = DestY * Backend->Descriptor.Pitch + X * BytesPerPixel;
                    UINT32 SrcOffset = Row * Width * BytesPerPixel;
                    UINT32 CopyWidth = Width * BytesPerPixel;

                    /* Bounds check */
                    if (X >= 0 && (X + Width) <= Backend->Descriptor.Width) {
                        /* Simple memcpy for unclipped case */
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
    }

    /* Format conversion handled by engine */
    /* Or use UGA Blt() protocol for acceleration */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
UgaFb_GetDescriptor(
    IFramebufferBackend *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    UGA_BACKEND *Backend = (UGA_BACKEND *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Backend->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
UgaFb_SetDitherMethod(
    IFramebufferBackend *This,
    FB_DITHER_METHOD Method
    )
{
    UGA_BACKEND *Backend = (UGA_BACKEND *)This;
    Backend->DitherMethod = Method;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static UGA_BACKEND gUgaBackendInstance = {
    .Base.lpVtbl        = &gUgaFbVtbl,
    .RefCount.RefCount  = 1,
    .Initialized        = FALSE,
    .DitherMethod       = FbDitherNone,
    .UgaProtocol        = NULL,
    .HasDirectAccess    = FALSE,
    .SoftwareBuffer     = NULL,
};

IFramebufferBackend *
FbCreateUefiUgaBackend(
    VOID
    )
{
    return (IFramebufferBackend *)&gUgaBackendInstance;
}

/*
 * Set the UGA protocol instance.
 * Must be called before Initialize().
 */
VOID
FbUefiUgaSetProtocol(
    IN IFramebufferBackend *Backend,
    IN VOID *UgaProtocol
    )
{
    UGA_BACKEND *UgaBackend = (UGA_BACKEND *)Backend;
    UgaBackend->UgaProtocol = (EFI_UGA_DRAW_PROTOCOL *)UgaProtocol;
}
