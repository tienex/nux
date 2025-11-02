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

    /* Hardware-accelerated clipping (VESA 2.0+ supports this) */

    /* Set hardware clipping rectangle
     * All subsequent draw operations will be clipped to this region.
     * Returns S_OK if hardware supports clipping, E_NOTIMPL otherwise.
     */
    ANX_IFACE_METHOD(HRESULT, SetClipRect, (
        IN CONST FB_RECT *Rect))

    /* Get current hardware clipping rectangle
     * Returns S_FALSE if no clipping is active.
     */
    ANX_IFACE_METHOD(HRESULT, GetClipRect, (
        OUT FB_RECT *Rect,
        OUT BOOLEAN *IsActive))

    /* Reset hardware clipping (disable clipping) */
    ANX_IFACE_METHOD(HRESULT, ResetClip, (
        VOID))

ANX_END_INTERFACE(IFramebufferBackendExt)

/* --------------------------------------------------------------- */
/*  Hardware Capability Flags                                       */
/* --------------------------------------------------------------- */

/* Returned by GetCapabilities() to indicate what hardware features
 * are supported by the backend/driver.
 */

#define FB_CAP_PAGE_FLIPPING        0x00000001  /* Hardware page flipping */
#define FB_CAP_VSYNC                0x00000002  /* VBlank synchronization */
#define FB_CAP_HARDWARE_CURSOR      0x00000004  /* Hardware mouse cursor */
#define FB_CAP_HARDWARE_FILL        0x00000008  /* Hardware-accelerated fills */
#define FB_CAP_HARDWARE_BLIT        0x00000010  /* Hardware-accelerated blits */
#define FB_CAP_HARDWARE_LINE        0x00000020  /* Hardware-accelerated lines */
#define FB_CAP_ROP_SUPPORT          0x00000040  /* ROP (raster operations) */
#define FB_CAP_HARDWARE_CLIPPING    0x00000080  /* Hardware clipping regions */
#define FB_CAP_ALPHA_BLENDING       0x00000100  /* Hardware alpha blending */
#define FB_CAP_STRETCH_BLIT         0x00000200  /* Stretch blitting */
#define FB_CAP_ROTATION             0x00000400  /* Screen rotation */
#define FB_CAP_3D_ACCELERATION      0x00000800  /* 3D acceleration */

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
#define IFramebufferBackendExt_SetClipRect(This, Rect) \
    ((This)->lpVtbl->SetClipRect(This, Rect))
#define IFramebufferBackendExt_GetClipRect(This, Rect, Active) \
    ((This)->lpVtbl->GetClipRect(This, Rect, Active))
#define IFramebufferBackendExt_ResetClip(This) \
    ((This)->lpVtbl->ResetClip(This))

#endif /* !__cplusplus */

/* --------------------------------------------------------------- */
/*  PC Graphics Backend API (for pcga.c)                           */
/*  Direct functions for mode enumeration, EDID, and detection     */
/* --------------------------------------------------------------- */

/* Forward declaration for EDID structure */
typedef struct _EDID_BASE_BLOCK EDID_BASE_BLOCK;

/*
 * Create the PC Graphics backend instance.
 */
IFramebufferBackend*
FbCreatePcGraphicsBackend(
    VOID
    );

/*
 * Set bank switching function for VESA banked modes.
 */
VOID
FbPcGraphicsSetBankFunction(
    IN IFramebufferBackend *Backend,
    IN VOID (*BankSwitchFunc)(UINT32 BankNumber)
    );

/*
 * Set RtlCopyMemory function for optimized memory operations.
 */
VOID
FbPcGraphicsSetRtlCopyMemory(
    IN IFramebufferBackend *Backend,
    IN VOID (*RtlCopyMemoryFunc)(VOID *Dest, CONST VOID *Src, SIZE_T Size)
    );

/*
 * Get the number of available PC graphics modes.
 */
UINT32
FbPcGraphicsGetModeCount(
    VOID
    );

/*
 * Query information about a specific mode by index.
 */
HRESULT
FbPcGraphicsQueryMode(
    IN UINT32 ModeIndex,
    OUT FRAMEBUFFER_DESC *ModeDesc
    );

/*
 * Set a specific video mode by mode number.
 */
HRESULT
FbPcGraphicsSetMode(
    IN IFramebufferBackend *Backend,
    IN UINT32 ModeNumber
    );

/*
 * Get hardware capabilities (CGA/EGA/VGA/SVGA/MCGA).
 * Returns a bitmask of supported hardware flags.
 */
UINT32
FbPcGraphicsGetCapabilities(
    VOID
    );

/*
 * Filter modes based on EDID and hardware capabilities.
 * Returns the number of supported modes in the output array.
 */
UINT32
FbPcGraphicsFilterModes(
    IN CONST EDID_BASE_BLOCK *Edid,
    OUT UINT32 *SupportedModeIndices,
    IN UINT32 MaxModes
    );

/*
 * Find the best mode matching requested size, depth, and refresh rate.
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
    );

/*
 * Extract font from BIOS ROM (CGA/EGA/MCGA/VGA/SVGA/XGA).
 * Copies the ROM font into a user-provided buffer.
 * Supported heights: 8 (CGA), 14 (EGA), 16 (MCGA/VGA/SVGA/XGA).
 */
HRESULT
FbPcGraphicsExtractRomFont(
    OUT UINT8 *FontBuffer,
    IN UINT32 BufferSize,
    IN UINT32 CharHeight
    );

/*
 * Get recommended font height for current graphics adapter.
 * Returns 8 for CGA, 14 for EGA, 16 for MCGA/VGA/SVGA/XGA.
 */
HRESULT
FbPcGraphicsGetRecommendedFontHeight(
    OUT UINT32 *FontHeight
    );

/*
 * Load a custom font into character generator RAM.
 * Automatically detects CGA/EGA/MCGA/VGA/SVGA/XGA and uses appropriate method.
 * CGA: stores in software buffer (no hardware font RAM)
 * EGA: loads into character generator RAM (max 14 scanlines)
 * MCGA: loads into character generator RAM (max 16 scanlines, VGA-compatible)
 * VGA/SVGA/XGA: loads into character generator RAM (max 32 scanlines)
 */
HRESULT
FbPcGraphicsLoadFont(
    IN IFramebufferBackend *Backend,
    IN CONST UINT8 *FontData,
    IN UINT32 CharHeight,
    IN UINT32 CharOffset,
    IN UINT32 CharCount,
    IN UINT32 Bank
    );

/*
 * Select which font bank to display in text mode.
 */
HRESULT
FbPcGraphicsSelectFontBank(
    IN IFramebufferBackend *Backend,
    IN UINT32 Bank
    );

/*
 * Wait for vertical blank interval.
 */
HRESULT
FbPcGraphicsWaitForVBlank(
    IN IFramebufferBackend *Backend
    );

/*
 * Check if currently in vertical blank.
 */
HRESULT
FbPcGraphicsIsVBlank(
    IN IFramebufferBackend *Backend,
    OUT BOOLEAN *IsVBlank
    );

/*
 * Set display start address for hardware scrolling or page flipping.
 */
HRESULT
FbPcGraphicsSetDisplayStart(
    IN IFramebufferBackend *Backend,
    IN UINT32 Offset
    );

/*
 * Get current display start address.
 */
HRESULT
FbPcGraphicsGetDisplayStart(
    IN IFramebufferBackend *Backend,
    OUT UINT32 *Offset
    );

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
    );

/*
 * Set text mode caret (cursor) position.
 */
HRESULT
FbPcGraphicsSetCaretPosition(
    IN IFramebufferBackend *Backend,
    IN UINT32 X,
    IN UINT32 Y
    );

/*
 * Set text mode caret (cursor) shape and visibility.
 */
HRESULT
FbPcGraphicsSetCaretShape(
    IN IFramebufferBackend *Backend,
    IN UINT32 StartLine,
    IN UINT32 EndLine,
    IN BOOLEAN Visible
    );

/*
 * Set border color (VGA overscan).
 */
HRESULT
FbPcGraphicsSetBorderColor(
    IN IFramebufferBackend *Backend,
    IN UINT8 Color
    );

/*
 * Set palette entry (VGA DAC).
 */
HRESULT
FbPcGraphicsSetPaletteEntry(
    IN IFramebufferBackend *Backend,
    IN UINT8 Index,
    IN UINT8 Red,
    IN UINT8 Green,
    IN UINT8 Blue
    );

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
    );
