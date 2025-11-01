/*++
    Module Name:

        screen.h

    Abstract:

        Screen and Surface interfaces for framebuffer access.
        IFramebufferScreen represents a hardware framebuffer.
        IFramebufferSurface represents a software surface.

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

/* Forward declarations */
typedef struct _IFramebufferImage IFramebufferImage;
typedef struct _IFramebufferCursor IFramebufferCursor;

/* --------------------------------------------------------------- */
/*  ROP (Raster Operation) Codes                                    */
/* --------------------------------------------------------------- */

typedef enum _FB_ROP {
    FbRopCopy           = 0,    /* Destination = Source */
    FbRopXor            = 1,    /* Destination ^= Source */
    FbRopOr             = 2,    /* Destination |= Source */
    FbRopAnd            = 3,    /* Destination &= Source */
    FbRopNot            = 4,    /* Destination = ~Source */
    FbRopBlend          = 5,    /* Alpha blend (if supported) */
    FbRopAdd            = 6,    /* Additive blend */
    FbRopSubtract       = 7,    /* Subtractive blend */
} FB_ROP;

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
