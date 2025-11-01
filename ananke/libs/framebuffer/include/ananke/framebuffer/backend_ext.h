/*++
    Module Name:

        backend_ext.h

    Abstract:

        Extended backend interface for hardware-specific implementations.
        This interface is NOT exposed to users - only used internally by
        the framebuffer engine to delegate operations to hardware.

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
#include <ananke/framebuffer/screen.h>

/* --------------------------------------------------------------- */
/*  IFramebufferBackendExt - Extended backend interface             */
/* --------------------------------------------------------------- */

#define ANX_IID_IFramebufferBackendExt "FB000020-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferBackendExt,
    0xFB000020, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferBackendExt, IFramebufferBackend,
    IID_IFramebufferBackendExt, ANX_IID_IFramebufferBackendExt)

    /* Mode enumeration and management */
    ANX_IFACE_METHOD(HRESULT, EnumerateModes, (
        OUT FB_MODE_DESC *Modes,
        IN UINT32 MaxModes,
        OUT UINT32 *NumModes))

    ANX_IFACE_METHOD(HRESULT, GetCurrentMode, (
        OUT FB_MODE_DESC *Mode))

    ANX_IFACE_METHOD(HRESULT, SetMode, (
        IN UINT32 ModeNumber))

    /* Hardware capabilities */
    ANX_IFACE_METHOD(HRESULT, GetCapabilities, (
        OUT UINT32 *Capabilities))

    /* VBlank support */
    ANX_IFACE_METHOD(HRESULT, WaitForVBlank, (
        VOID))

    /* Page flipping */
    ANX_IFACE_METHOD(HRESULT, GetActivePage, (
        OUT UINT32 *Page))

    ANX_IFACE_METHOD(HRESULT, SetActivePage, (
        IN UINT32 Page))

    ANX_IFACE_METHOD(HRESULT, GetVisiblePage, (
        OUT UINT32 *Page))

    ANX_IFACE_METHOD(HRESULT, SetVisiblePage, (
        IN UINT32 Page))

    ANX_IFACE_METHOD(HRESULT, FlipPages, (
        IN BOOLEAN WaitForVBlank))

    /* Hardware-accelerated fill with ROP */
    ANX_IFACE_METHOD(HRESULT, FillRectRop, (
        IN CONST FB_RECT *Rect,
        IN FB_COLOR Color,
        IN FB_ROP Rop))

    /* Hardware-accelerated blit with ROP */
    ANX_IFACE_METHOD(HRESULT, BlitRop, (
        IN INT32 DestX,
        IN INT32 DestY,
        IN CONST UINT8 *SourceData,
        IN UINT32 SourceWidth,
        IN UINT32 SourceHeight,
        IN UINT32 SourcePitch,
        IN FB_PIXEL_FORMAT SourceFormat,
        IN CONST FB_RECT *SourceRect,
        IN FB_ROP Rop))

    /* Hardware-accelerated line drawing */
    ANX_IFACE_METHOD(HRESULT, DrawLine, (
        IN INT32 X1,
        IN INT32 Y1,
        IN INT32 X2,
        IN INT32 Y2,
        IN FB_COLOR Color))

    /* Hardware cursor support */
    ANX_IFACE_METHOD(HRESULT, SetHardwareCursor, (
        IN BOOLEAN Visible,
        IN INT32 X,
        IN INT32 Y,
        IN CONST UINT8 *CursorData,
        IN UINT32 Width,
        IN UINT32 Height,
        IN INT32 HotSpotX,
        IN INT32 HotSpotY,
        IN FB_CURSOR_TYPE Type))

ANX_END_INTERFACE(IFramebufferBackendExt)

/* --------------------------------------------------------------- */
/*  Helper Macros for C (COBJMACROS style)                          */
/* --------------------------------------------------------------- */

#ifndef __cplusplus

#define IFramebufferBackendExt_EnumerateModes(This, Modes, Max, Num) \
    ((This)->lpVtbl->EnumerateModes(This, Modes, Max, Num))
#define IFramebufferBackendExt_GetCurrentMode(This, Mode) \
    ((This)->lpVtbl->GetCurrentMode(This, Mode))
#define IFramebufferBackendExt_SetMode(This, Mode) \
    ((This)->lpVtbl->SetMode(This, Mode))
#define IFramebufferBackendExt_GetCapabilities(This, Caps) \
    ((This)->lpVtbl->GetCapabilities(This, Caps))
#define IFramebufferBackendExt_WaitForVBlank(This) \
    ((This)->lpVtbl->WaitForVBlank(This))
#define IFramebufferBackendExt_GetActivePage(This, Page) \
    ((This)->lpVtbl->GetActivePage(This, Page))
#define IFramebufferBackendExt_SetActivePage(This, Page) \
    ((This)->lpVtbl->SetActivePage(This, Page))
#define IFramebufferBackendExt_GetVisiblePage(This, Page) \
    ((This)->lpVtbl->GetVisiblePage(This, Page))
#define IFramebufferBackendExt_SetVisiblePage(This, Page) \
    ((This)->lpVtbl->SetVisiblePage(This, Page))
#define IFramebufferBackendExt_FlipPages(This, Wait) \
    ((This)->lpVtbl->FlipPages(This, Wait))
#define IFramebufferBackendExt_FillRectRop(This, Rect, Color, Rop) \
    ((This)->lpVtbl->FillRectRop(This, Rect, Color, Rop))
#define IFramebufferBackendExt_BlitRop(This, X, Y, Src, W, H, P, Fmt, R, Rop) \
    ((This)->lpVtbl->BlitRop(This, X, Y, Src, W, H, P, Fmt, R, Rop))
#define IFramebufferBackendExt_DrawLine(This, X1, Y1, X2, Y2, Color) \
    ((This)->lpVtbl->DrawLine(This, X1, Y1, X2, Y2, Color))
#define IFramebufferBackendExt_SetHardwareCursor(This, Vis, X, Y, Data, W, H, HX, HY, Type) \
    ((This)->lpVtbl->SetHardwareCursor(This, Vis, X, Y, Data, W, H, HX, HY, Type))

#endif /* !__cplusplus */
