/*++
    Module Name:

        engine.c

    Abstract:

        Framebuffer engine implementation.

        Provides software emulation for features not implemented by the
        hardware backend. The engine sits between user-facing interfaces
        and backends, automatically providing software fallbacks.

        Features:
        - Pixel format conversion with dithering
        - ROP (Raster Operation) support
        - Software cursor rendering
        - Format-agnostic blitting

--*/

#include <ananke/framebuffer/engine.h>
#include <ananke/framebuffer/pixelformat.h>
#include <ananke/framebuffer/dither.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Engine Context Structure                                       */
/* --------------------------------------------------------------- */

struct _FB_ENGINE_CONTEXT {
    IFramebufferBackend  *Backend;
    FRAMEBUFFER_DESC     Descriptor;
    BOOLEAN              Initialized;
};

/* --------------------------------------------------------------- */
/*  Engine Creation and Destruction                                */
/* --------------------------------------------------------------- */

/*
 * Create an engine context wrapping a backend.
 */
FB_ENGINE_CONTEXT *
FbCreateEngine(
    IN IFramebufferBackend *Backend
    )
{
    FB_ENGINE_CONTEXT *Engine;

    if (Backend == NULL) {
        return NULL;
    }

    /* Allocate engine context */
    Engine = (FB_ENGINE_CONTEXT *)ANX_MALLOC(sizeof(FB_ENGINE_CONTEXT));
    if (Engine == NULL) {
        return NULL;
    }

    /* Initialize context */
    ANX_MEMSET(Engine, 0, sizeof(FB_ENGINE_CONTEXT));
    Engine->Backend = Backend;
    Engine->Initialized = FALSE;

    /* Query backend descriptor */
    if (FAILED(IFramebufferBackend_GetDescriptor(Backend, &Engine->Descriptor))) {
        ANX_FREE(Engine);
        return NULL;
    }

    Engine->Initialized = TRUE;

    /* Add reference to backend */
    IUnknown_AddRef((IUnknown *)Backend);

    return Engine;
}

/*
 * Destroy an engine context.
 */
VOID
FbDestroyEngine(
    IN FB_ENGINE_CONTEXT *Engine
    )
{
    if (Engine == NULL) {
        return;
    }

    /* Release backend reference */
    if (Engine->Backend != NULL) {
        IUnknown_Release((IUnknown *)Engine->Backend);
    }

    /* Free context */
    ANX_FREE(Engine);
}

/* --------------------------------------------------------------- */
/*  Pixel Format Utilities                                         */
/* --------------------------------------------------------------- */

/*
 * Get bytes per pixel for a pixel format.
 */
UINT32
FbGetBytesPerPixel(
    IN FB_PIXEL_FORMAT Format
    )
{
    switch (Format) {
        case FbPixelFormat1Bpp:
            return 0;  /* Packed format, not byte-aligned */

        case FbPixelFormatIndexed4:
        case FbPixelFormatVga16Planar:
            return 0;  /* Packed/planar format */

        case FbPixelFormatIndexed256:
        case FbPixelFormatGrayscale8:
            return 1;

        case FbPixelFormatRgb555:
        case FbPixelFormatRgb565:
            return 2;

        case FbPixelFormatRgb888:
            return 3;

        case FbPixelFormatRgba8888:
        case FbPixelFormatBgra8888:
            return 4;

        case FbPixelFormatText:
            return 2;  /* Character + attribute */

        default:
            return 0;
    }
}

/*
 * Get bits per pixel for a pixel format.
 */
UINT32
FbGetBitsPerPixel(
    IN FB_PIXEL_FORMAT Format
    )
{
    switch (Format) {
        case FbPixelFormat1Bpp:
            return 1;

        case FbPixelFormatIndexed4:
            return 4;

        case FbPixelFormatVga16Planar:
            return 4;  /* 4 planes × 1 bit */

        case FbPixelFormatIndexed256:
        case FbPixelFormatGrayscale8:
            return 8;

        case FbPixelFormatRgb555:
            return 15;

        case FbPixelFormatRgb565:
            return 16;

        case FbPixelFormatRgb888:
            return 24;

        case FbPixelFormatRgba8888:
        case FbPixelFormatBgra8888:
            return 32;

        case FbPixelFormatText:
            return 16;  /* Character + attribute */

        default:
            return 0;
    }
}

/*
 * Check if a pixel format is planar.
 */
BOOLEAN
FbIsFormatPlanar(
    IN FB_PIXEL_FORMAT Format
    )
{
    return (Format == FbPixelFormatVga16Planar);
}

/*
 * Check if a pixel format is indexed.
 */
BOOLEAN
FbIsFormatIndexed(
    IN FB_PIXEL_FORMAT Format
    )
{
    return (Format == FbPixelFormatIndexed4 ||
            Format == FbPixelFormatIndexed256 ||
            Format == FbPixelFormatVga16Planar);
}

/*
 * Get number of planes for planar formats.
 */
UINT32
FbGetNumPlanes(
    IN FB_PIXEL_FORMAT Format
    )
{
    if (Format == FbPixelFormatVga16Planar) {
        return 4;
    }
    return 1;
}

/* --------------------------------------------------------------- */
/*  ROP Operations                                                  */
/* --------------------------------------------------------------- */

/*
 * Apply ROP operation between source and destination pixels.
 */
UINT32
FbEngineApplyRop(
    IN UINT32 Source,
    IN UINT32 Dest,
    IN FB_ROP Rop
    )
{
    switch (Rop) {
        case FbRopCopy:
            return Source;

        case FbRopXor:
            return Source ^ Dest;

        case FbRopOr:
            return Source | Dest;

        case FbRopAnd:
            return Source & Dest;

        case FbRopNot:
            return ~Source;

        case FbRopNop:
            return Dest;

        case FbRopInvert:
            return ~Dest;

        case FbRopNand:
            return ~(Source & Dest);

        case FbRopNor:
            return ~(Source | Dest);

        case FbRopEquiv:
            return ~(Source ^ Dest);

        case FbRopSrcAnd:
            return Source & Dest;

        case FbRopSrcInvert:
            return Source ^ (~Dest);

        case FbRopDstInvert:
            return (~Source) ^ Dest;

        case FbRopMerge:
            return Source | Dest;

        default:
            return Source;  /* Default to copy */
    }
}

/* --------------------------------------------------------------- */
/*  Format Conversion                                               */
/* --------------------------------------------------------------- */

/*
 * Convert pixel format from source to destination.
 * Applies dithering when converting to lower bit depth.
 */
HRESULT
FbEngineConvertFormat(
    IN CONST VOID *SourceData,
    IN FB_PIXEL_FORMAT SourceFormat,
    IN UINT32 SourceWidth,
    IN UINT32 SourceHeight,
    IN UINT32 SourcePitch,
    OUT VOID *DestData,
    IN FB_PIXEL_FORMAT DestFormat,
    IN UINT32 DestPitch,
    IN FB_DITHER_METHOD DitherMethod
    )
{
    UINT32 Y, X;
    CONST UINT8 *SrcRow;
    UINT8 *DstRow;

    if (SourceData == NULL || DestData == NULL) {
        return E_POINTER;
    }

    if (SourceWidth == 0 || SourceHeight == 0) {
        return E_INVALIDARG;
    }

    /* Fast path: same format */
    if (SourceFormat == DestFormat) {
        for (Y = 0; Y < SourceHeight; Y++) {
            SrcRow = (CONST UINT8 *)SourceData + (Y * SourcePitch);
            DstRow = (UINT8 *)DestData + (Y * DestPitch);

            UINT32 RowBytes = SourceWidth * FbGetBytesPerPixel(SourceFormat);
            if (RowBytes == 0) {
                /* Packed formats - copy full pitch */
                RowBytes = SourcePitch < DestPitch ? SourcePitch : DestPitch;
            }

            ANX_MEMCPY(DstRow, SrcRow, RowBytes);
        }
        return S_OK;
    }

    /* Convert pixel by pixel */
    for (Y = 0; Y < SourceHeight; Y++) {
        for (X = 0; X < SourceWidth; X++) {
            FB_COLOR Color;

            /* Unpack source pixel */
            UINT32 SrcOffset = Y * SourcePitch + X * FbGetBytesPerPixel(SourceFormat);
            Color = FbUnpackPixel(SourceData + SrcOffset, SourceFormat);

            /* Apply dithering if needed */
            if (DitherMethod != FbDitherNone) {
                FbDitherRgb(X, Y, &Color, DestFormat);
            }

            /* Pack destination pixel */
            UINT32 DstOffset = Y * DestPitch + X * FbGetBytesPerPixel(DestFormat);
            FbPackPixel(&Color, DestData + DstOffset, DestFormat);
        }
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Engine Operations                                               */
/* --------------------------------------------------------------- */

/*
 * Fill rectangle with ROP operation.
 */
HRESULT
FbEngineFillRect(
    IN FB_ENGINE_CONTEXT *Engine,
    IN CONST FB_RECT *Rect,
    IN FB_COLOR Color,
    IN FB_ROP Rop
    )
{
    if (Engine == NULL || !Engine->Initialized) {
        return E_POINTER;
    }

    if (Rect == NULL) {
        return E_INVALIDARG;
    }

    /* Try hardware acceleration first */
    if (Rop == FbRopCopy) {
        HRESULT Hr = IFramebufferBackend_FillRect(Engine->Backend, Rect, Color);
        if (SUCCEEDED(Hr)) {
            return Hr;
        }
    }

    /* Software fallback: pixel-by-pixel with ROP */
    for (INT32 Y = Rect->Y; Y < Rect->Y + (INT32)Rect->Height; Y++) {
        for (INT32 X = Rect->X; X < Rect->X + (INT32)Rect->Width; X++) {
            if (Rop == FbRopCopy) {
                IFramebufferBackend_SetPixel(Engine->Backend, X, Y, Color);
            } else {
                /* Read-modify-write for ROP */
                FB_COLOR DestColor;
                if (SUCCEEDED(IFramebufferBackend_GetPixel(Engine->Backend, X, Y, &DestColor))) {
                    UINT32 SrcPixel = (Color.Red << 16) | (Color.Green << 8) | Color.Blue;
                    UINT32 DstPixel = (DestColor.Red << 16) | (DestColor.Green << 8) | DestColor.Blue;
                    UINT32 Result = FbEngineApplyRop(SrcPixel, DstPixel, Rop);

                    FB_COLOR ResultColor;
                    ResultColor.Red = (Result >> 16) & 0xFF;
                    ResultColor.Green = (Result >> 8) & 0xFF;
                    ResultColor.Blue = Result & 0xFF;
                    ResultColor.Alpha = 255;

                    IFramebufferBackend_SetPixel(Engine->Backend, X, Y, ResultColor);
                }
            }
        }
    }

    return S_OK;
}

/*
 * Blit with pixel format conversion and ROP.
 */
HRESULT
FbEngineBlit(
    IN FB_ENGINE_CONTEXT *Engine,
    IN INT32 DestX,
    IN INT32 DestY,
    IN CONST VOID *SourceData,
    IN UINT32 SourceWidth,
    IN UINT32 SourceHeight,
    IN UINT32 SourcePitch,
    IN FB_PIXEL_FORMAT SourceFormat,
    IN FB_PIXEL_FORMAT DestFormat,
    IN CONST FB_RECT *SourceRect,
    IN FB_ROP Rop
    )
{
    UINT32 Width, Height;
    UINT32 SrcX, SrcY;

    if (Engine == NULL || !Engine->Initialized) {
        return E_POINTER;
    }

    if (SourceData == NULL) {
        return E_POINTER;
    }

    /* Determine blit region */
    if (SourceRect != NULL) {
        SrcX = SourceRect->X;
        SrcY = SourceRect->Y;
        Width = SourceRect->Width;
        Height = SourceRect->Height;
    } else {
        SrcX = 0;
        SrcY = 0;
        Width = SourceWidth;
        Height = SourceHeight;
    }

    /* Try hardware acceleration for simple cases */
    if (SourceFormat == DestFormat && Rop == FbRopCopy) {
        /* Direct blit without conversion */
        HRESULT Hr = IFramebufferBackend_BlitBitmap(
            Engine->Backend,
            DestX, DestY,
            Width, Height,
            SourceData,
            SourceFormat);

        if (SUCCEEDED(Hr)) {
            return Hr;
        }
    }

    /* Software fallback: pixel-by-pixel conversion and ROP */
    for (UINT32 Y = 0; Y < Height; Y++) {
        for (UINT32 X = 0; X < Width; X++) {
            /* Get source pixel */
            UINT32 SrcOffset = (SrcY + Y) * SourcePitch +
                              (SrcX + X) * FbGetBytesPerPixel(SourceFormat);
            FB_COLOR SrcColor = FbUnpackPixel((CONST UINT8 *)SourceData + SrcOffset, SourceFormat);

            /* Apply ROP if needed */
            if (Rop != FbRopCopy) {
                FB_COLOR DestColor;
                if (SUCCEEDED(IFramebufferBackend_GetPixel(
                    Engine->Backend, DestX + X, DestY + Y, &DestColor))) {

                    UINT32 SrcPixel = (SrcColor.Red << 16) | (SrcColor.Green << 8) | SrcColor.Blue;
                    UINT32 DstPixel = (DestColor.Red << 16) | (DestColor.Green << 8) | DestColor.Blue;
                    UINT32 Result = FbEngineApplyRop(SrcPixel, DstPixel, Rop);

                    SrcColor.Red = (Result >> 16) & 0xFF;
                    SrcColor.Green = (Result >> 8) & 0xFF;
                    SrcColor.Blue = Result & 0xFF;
                }
            }

            /* Write destination pixel */
            IFramebufferBackend_SetPixel(Engine->Backend, DestX + X, DestY + Y, SrcColor);
        }
    }

    return S_OK;
}

/*
 * Software cursor rendering.
 */
HRESULT
FbEngineDrawCursor(
    IN FB_ENGINE_CONTEXT *Engine,
    IN INT32 X,
    IN INT32 Y,
    IN CONST UINT8 *CursorData,
    IN UINT32 Width,
    IN UINT32 Height,
    IN INT32 HotSpotX,
    IN INT32 HotSpotY,
    IN FB_CURSOR_TYPE Type
    )
{
    UINT32 CX, CY;
    INT32 ScreenX, ScreenY;

    if (Engine == NULL || !Engine->Initialized) {
        return E_POINTER;
    }

    if (CursorData == NULL) {
        return E_POINTER;
    }

    /* Adjust for hotspot */
    X -= HotSpotX;
    Y -= HotSpotY;

    /* Render cursor */
    for (CY = 0; CY < Height; CY++) {
        for (CX = 0; CX < Width; CX++) {
            ScreenX = X + CX;
            ScreenY = Y + CY;

            /* Check bounds */
            if (ScreenX < 0 || ScreenY < 0 ||
                ScreenX >= (INT32)Engine->Descriptor.Width ||
                ScreenY >= (INT32)Engine->Descriptor.Height) {
                continue;
            }

            /* Get cursor pixel (assuming 1BPP for now) */
            UINT32 ByteOffset = CY * ((Width + 7) / 8) + (CX / 8);
            UINT32 BitOffset = 7 - (CX % 8);
            BOOLEAN PixelSet = (CursorData[ByteOffset] & (1 << BitOffset)) != 0;

            if (PixelSet) {
                FB_COLOR Color;

                switch (Type) {
                    case FbCursorArrow:
                    case FbCursorHand:
                    case FbCursorCrosshair:
                        /* Black cursor */
                        Color.Red = Color.Green = Color.Blue = 0;
                        Color.Alpha = 255;
                        break;

                    case FbCursorInvert:
                        /* XOR cursor */
                        if (SUCCEEDED(IFramebufferBackend_GetPixel(
                            Engine->Backend, ScreenX, ScreenY, &Color))) {
                            Color.Red = 255 - Color.Red;
                            Color.Green = 255 - Color.Green;
                            Color.Blue = 255 - Color.Blue;
                        }
                        break;

                    default:
                        Color.Red = Color.Green = Color.Blue = 0;
                        Color.Alpha = 255;
                        break;
                }

                IFramebufferBackend_SetPixel(Engine->Backend, ScreenX, ScreenY, Color);
            }
        }
    }

    return S_OK;
}
