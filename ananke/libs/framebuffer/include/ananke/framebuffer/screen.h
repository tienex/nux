/*++
    Module Name:

        screen.h

    Abstract:

        Screen and Surface interfaces for framebuffer access.
        IFramebufferScreen represents a hardware framebuffer.
        IFramebufferSurface represents a software surface.

        Both IFramebufferScreen and IFramebufferSurface inherit from
        IFramebufferBitmap to provide common blitting operations.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>
#include <ananke/framebuffer.h>
#include <ananke/framebuffer/manager.h>
#include <ananke/framebuffer/bitmap.h>

/* Forward declarations */
typedef struct _IFramebufferImage IFramebufferImage;
typedef struct _IFramebufferCursor IFramebufferCursor;

/* --------------------------------------------------------------- */
/*  ROP (Raster Operation) Codes                                    */
/* --------------------------------------------------------------- */

/*
 * ROP2 - Binary Raster Operations (16 codes)
 * Operate on Source (S) and Destination (D)
 */
typedef enum _FB_ROP2 {
    FbRop2Black         = 0x00,  /* 0 */
    FbRop2NotMergePen   = 0x01,  /* ~(D | S) */
    FbRop2MaskNotPen    = 0x02,  /* D & ~S */
    FbRop2NotCopyPen    = 0x03,  /* ~S */
    FbRop2MaskPenNot    = 0x04,  /* S & ~D */
    FbRop2Not           = 0x05,  /* ~D */
    FbRop2XorPen        = 0x06,  /* D ^ S */
    FbRop2NotMaskPen    = 0x07,  /* ~(D & S) */
    FbRop2MaskPen       = 0x08,  /* D & S */
    FbRop2NotXorPen     = 0x09,  /* ~(D ^ S) */
    FbRop2Nop           = 0x0A,  /* D (no operation) */
    FbRop2MergeNotPen   = 0x0B,  /* D | ~S */
    FbRop2CopyPen       = 0x0C,  /* S (copy) */
    FbRop2MergePenNot   = 0x0D,  /* S | ~D */
    FbRop2MergePen      = 0x0E,  /* D | S */
    FbRop2White         = 0x0F,  /* 1 */
} FB_ROP2;

/*
 * ROP3 - Ternary Raster Operations (256 codes)
 * Operate on Source (S), Destination (D), and Pattern/Brush (P)
 *
 * ROP3 codes are 8-bit values where each bit represents the output
 * for a specific combination of S, D, P inputs (truth table).
 *
 * Common ROP3 codes (compatible with Windows GDI):
 */
typedef enum _FB_ROP3 {
    FbRop3Blackness     = 0x00,  /* 0 */
    FbRop3DPSoon        = 0x01,  /* ~(D | (P | S)) */
    FbRop3DPSona        = 0x02,  /* D & ~(P | S) */
    FbRop3PSon          = 0x03,  /* ~(P | S) */
    FbRop3SDPona        = 0x04,  /* S & ~(D | P) */
    FbRop3DPon          = 0x05,  /* ~(D | P) */
    FbRop3PDSxnon       = 0x06,  /* ~(P | ~(D ^ S)) */
    FbRop3PDSaon        = 0x07,  /* ~(P | (D & S)) */
    FbRop3SDPnaa        = 0x08,  /* S & (D & ~P) */
    FbRop3PDSxon        = 0x09,  /* ~(P | (D ^ S)) */

    /* Common operations */
    FbRop3NoOp          = 0xAA,  /* D (no operation) */
    FbRop3MergeCopy     = 0xC0,  /* P & S */
    FbRop3SrcCopy       = 0xCC,  /* S (source copy) */
    FbRop3SrcPaint      = 0xEE,  /* D | S */
    FbRop3SrcAnd        = 0x88,  /* D & S */
    FbRop3SrcInvert     = 0x66,  /* D ^ S */
    FbRop3SrcErase      = 0x44,  /* S & ~D */
    FbRop3NotSrcCopy    = 0x33,  /* ~S */
    FbRop3NotSrcErase   = 0x11,  /* ~(S | D) */
    FbRop3DstInvert     = 0x55,  /* ~D */
    FbRop3PatCopy       = 0xF0,  /* P (pattern copy) */
    FbRop3PatPaint      = 0xFB,  /* D | ~S | P */
    FbRop3PatInvert     = 0x5A,  /* D ^ P */
    FbRop3Whiteness     = 0xFF,  /* 1 */
} FB_ROP3;

/* Generic ROP type for simple operations */
typedef UINT8 FB_ROP;

/* --------------------------------------------------------------- */
/*  IFramebufferScreen - Hardware framebuffer                       */
/* --------------------------------------------------------------- */

#define ANX_IID_IFramebufferScreen "FB000011-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferScreen,
    0xFB000011, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferScreen, IUnknown,
    IID_IFramebufferScreen, ANX_IID_IFramebufferScreen)

    /* Get screen descriptor */
    ANX_IFACE_METHOD(HRESULT, GetDescriptor, (
        OUT FRAMEBUFFER_DESC *Descriptor))

    /* Get mode information */
    ANX_IFACE_METHOD(HRESULT, GetMode, (
        OUT FB_MODE_DESC *Mode))

    /* Clear screen with color */
    ANX_IFACE_METHOD(HRESULT, Clear, (
        IN FB_COLOR Color))

    /* Set a single pixel */
    ANX_IFACE_METHOD(HRESULT, SetPixel, (
        IN INT32 X,
        IN INT32 Y,
        IN FB_COLOR Color))

    /* Get a single pixel */
    ANX_IFACE_METHOD(HRESULT, GetPixel, (
        IN INT32 X,
        IN INT32 Y,
        OUT FB_COLOR *Color))

    /* Fill a rectangle */
    ANX_IFACE_METHOD(HRESULT, FillRect, (
        IN CONST FB_RECT *Rect,
        IN FB_COLOR Color,
        IN FB_ROP Rop))

    /* Blit from an image */
    ANX_IFACE_METHOD(HRESULT, BlitImage, (
        IN INT32 DestX,
        IN INT32 DestY,
        IN IFramebufferImage *Image,
        IN CONST FB_RECT *SourceRect,
        IN FB_ROP Rop))

    /* Wait for vertical blank */
    ANX_IFACE_METHOD(HRESULT, WaitForVBlank, (
        VOID))

    /* Page flipping support */
    ANX_IFACE_METHOD(HRESULT, GetActivePage, (
        OUT UINT32 *Page))

    ANX_IFACE_METHOD(HRESULT, SetActivePage, (
        IN UINT32 Page))

    ANX_IFACE_METHOD(HRESULT, GetVisiblePage, (
        OUT UINT32 *Page))

    ANX_IFACE_METHOD(HRESULT, SetVisiblePage, (
        IN UINT32 Page))

    /* Flip pages (swap active and visible) */
    ANX_IFACE_METHOD(HRESULT, FlipPages, (
        IN BOOLEAN WaitForVBlank))

    /* Get cursor interface */
    ANX_IFACE_METHOD(HRESULT, GetCursor, (
        OUT IFramebufferCursor **Cursor))

    /* Get palette interface (if indexed color mode) */
    ANX_IFACE_METHOD(HRESULT, GetPalette, (
        OUT IFramebufferPalette **Palette))

    /* Get text interface */
    ANX_IFACE_METHOD(HRESULT, GetText, (
        OUT IFramebufferText **Text))

    /* Lock screen for direct memory access */
    ANX_IFACE_METHOD(HRESULT, Lock, (
        OUT VOID **FramebufferAddress,
        OUT UINT32 *Pitch))

    /* Unlock screen */
    ANX_IFACE_METHOD(HRESULT, Unlock, (
        VOID))

ANX_END_INTERFACE(IFramebufferScreen)

/* --------------------------------------------------------------- */
/*  IFramebufferSurface - Software surface                          */
/* --------------------------------------------------------------- */

#define ANX_IID_IFramebufferSurface "FB000012-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferSurface,
    0xFB000012, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferSurface, IUnknown,
    IID_IFramebufferSurface, ANX_IID_IFramebufferSurface)

    /* Get surface descriptor */
    ANX_IFACE_METHOD(HRESULT, GetDescriptor, (
        OUT FRAMEBUFFER_DESC *Descriptor))

    /* Clear surface with color */
    ANX_IFACE_METHOD(HRESULT, Clear, (
        IN FB_COLOR Color))

    /* Set a single pixel */
    ANX_IFACE_METHOD(HRESULT, SetPixel, (
        IN INT32 X,
        IN INT32 Y,
        IN FB_COLOR Color))

    /* Get a single pixel */
    ANX_IFACE_METHOD(HRESULT, GetPixel, (
        IN INT32 X,
        IN INT32 Y,
        OUT FB_COLOR *Color))

    /* Fill a rectangle */
    ANX_IFACE_METHOD(HRESULT, FillRect, (
        IN CONST FB_RECT *Rect,
        IN FB_COLOR Color,
        IN FB_ROP Rop))

    /* Blit from an image */
    ANX_IFACE_METHOD(HRESULT, BlitImage, (
        IN INT32 DestX,
        IN INT32 DestY,
        IN IFramebufferImage *Image,
        IN CONST FB_RECT *SourceRect,
        IN FB_ROP Rop))

    /* Blit to a screen */
    ANX_IFACE_METHOD(HRESULT, BlitToScreen, (
        IN IFramebufferScreen *Screen,
        IN INT32 DestX,
        IN INT32 DestY,
        IN CONST FB_RECT *SourceRect,
        IN FB_ROP Rop))

    /* Get palette interface (if indexed color mode) */
    ANX_IFACE_METHOD(HRESULT, GetPalette, (
        OUT IFramebufferPalette **Palette))

    /* Get text interface */
    ANX_IFACE_METHOD(HRESULT, GetText, (
        OUT IFramebufferText **Text))

    /* Lock surface for direct memory access */
    ANX_IFACE_METHOD(HRESULT, Lock, (
        OUT VOID **MemoryAddress,
        OUT UINT32 *Pitch))

    /* Unlock surface */
    ANX_IFACE_METHOD(HRESULT, Unlock, (
        VOID))

ANX_END_INTERFACE(IFramebufferSurface)

/* --------------------------------------------------------------- */
/*  Helper Macros for C (COBJMACROS style)                          */
/* --------------------------------------------------------------- */

#ifndef __cplusplus

#define IFramebufferScreen_GetDescriptor(This, Desc) \
    ((This)->lpVtbl->GetDescriptor(This, Desc))
#define IFramebufferScreen_GetMode(This, Mode) \
    ((This)->lpVtbl->GetMode(This, Mode))
#define IFramebufferScreen_Clear(This, Color) \
    ((This)->lpVtbl->Clear(This, Color))
#define IFramebufferScreen_SetPixel(This, X, Y, Color) \
    ((This)->lpVtbl->SetPixel(This, X, Y, Color))
#define IFramebufferScreen_GetPixel(This, X, Y, Color) \
    ((This)->lpVtbl->GetPixel(This, X, Y, Color))
#define IFramebufferScreen_FillRect(This, Rect, Color, Rop) \
    ((This)->lpVtbl->FillRect(This, Rect, Color, Rop))
#define IFramebufferScreen_BlitImage(This, X, Y, Img, SrcRect, Rop) \
    ((This)->lpVtbl->BlitImage(This, X, Y, Img, SrcRect, Rop))
#define IFramebufferScreen_WaitForVBlank(This) \
    ((This)->lpVtbl->WaitForVBlank(This))
#define IFramebufferScreen_GetActivePage(This, Page) \
    ((This)->lpVtbl->GetActivePage(This, Page))
#define IFramebufferScreen_SetActivePage(This, Page) \
    ((This)->lpVtbl->SetActivePage(This, Page))
#define IFramebufferScreen_GetVisiblePage(This, Page) \
    ((This)->lpVtbl->GetVisiblePage(This, Page))
#define IFramebufferScreen_SetVisiblePage(This, Page) \
    ((This)->lpVtbl->SetVisiblePage(This, Page))
#define IFramebufferScreen_FlipPages(This, Wait) \
    ((This)->lpVtbl->FlipPages(This, Wait))
#define IFramebufferScreen_GetCursor(This, Cursor) \
    ((This)->lpVtbl->GetCursor(This, Cursor))
#define IFramebufferScreen_GetPalette(This, Pal) \
    ((This)->lpVtbl->GetPalette(This, Pal))
#define IFramebufferScreen_GetText(This, Text) \
    ((This)->lpVtbl->GetText(This, Text))
#define IFramebufferScreen_Lock(This, Addr, Pitch) \
    ((This)->lpVtbl->Lock(This, Addr, Pitch))
#define IFramebufferScreen_Unlock(This) \
    ((This)->lpVtbl->Unlock(This))

#define IFramebufferSurface_GetDescriptor(This, Desc) \
    ((This)->lpVtbl->GetDescriptor(This, Desc))
#define IFramebufferSurface_Clear(This, Color) \
    ((This)->lpVtbl->Clear(This, Color))
#define IFramebufferSurface_SetPixel(This, X, Y, Color) \
    ((This)->lpVtbl->SetPixel(This, X, Y, Color))
#define IFramebufferSurface_GetPixel(This, X, Y, Color) \
    ((This)->lpVtbl->GetPixel(This, X, Y, Color))
#define IFramebufferSurface_FillRect(This, Rect, Color, Rop) \
    ((This)->lpVtbl->FillRect(This, Rect, Color, Rop))
#define IFramebufferSurface_BlitImage(This, X, Y, Img, SrcRect, Rop) \
    ((This)->lpVtbl->BlitImage(This, X, Y, Img, SrcRect, Rop))
#define IFramebufferSurface_BlitToScreen(This, Screen, X, Y, SrcRect, Rop) \
    ((This)->lpVtbl->BlitToScreen(This, Screen, X, Y, SrcRect, Rop))
#define IFramebufferSurface_GetPalette(This, Pal) \
    ((This)->lpVtbl->GetPalette(This, Pal))
#define IFramebufferSurface_GetText(This, Text) \
    ((This)->lpVtbl->GetText(This, Text))
#define IFramebufferSurface_Lock(This, Addr, Pitch) \
    ((This)->lpVtbl->Lock(This, Addr, Pitch))
#define IFramebufferSurface_Unlock(This) \
    ((This)->lpVtbl->Unlock(This))

#endif /* !__cplusplus */
