/*++
    Module Name:

        cursor.h

    Abstract:

        Cursor interface for mouse cursor support (mono and color).
        Supports hardware cursor when available, with software fallback.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>
#include <ananke/framebuffer.h>

/* --------------------------------------------------------------- */
/*  Cursor Definitions                                              */
/* --------------------------------------------------------------- */

#define FB_CURSOR_MAX_WIDTH     64
#define FB_CURSOR_MAX_HEIGHT    64

typedef enum _FB_CURSOR_TYPE {
    FbCursorMono        = 0,  /* 1-bit monochrome cursor with mask */
    FbCursorColor       = 1,  /* Full color cursor with alpha */
} FB_CURSOR_TYPE;

typedef struct _FB_CURSOR_DESC {
    FB_CURSOR_TYPE  Type;
    UINT32          Width;          /* Cursor width in pixels */
    UINT32          Height;         /* Cursor height in pixels */
    INT32           HotSpotX;       /* Hot spot X coordinate */
    INT32           HotSpotY;       /* Hot spot Y coordinate */
} FB_CURSOR_DESC;

/* --------------------------------------------------------------- */
/*  IFramebufferCursor - Cursor interface                           */
/* --------------------------------------------------------------- */

#define ANX_IID_IFramebufferCursor "FB000014-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferCursor,
    0xFB000014, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferCursor, IUnknown,
    IID_IFramebufferCursor, ANX_IID_IFramebufferCursor)

    /* Show or hide cursor */
    ANX_IFACE_METHOD(HRESULT, SetVisible, (
        IN BOOLEAN Visible))

    /* Get cursor visibility */
    ANX_IFACE_METHOD(HRESULT, IsVisible, (
        OUT BOOLEAN *Visible))

    /* Set cursor position */
    ANX_IFACE_METHOD(HRESULT, SetPosition, (
        IN INT32 X,
        IN INT32 Y))

    /* Get cursor position */
    ANX_IFACE_METHOD(HRESULT, GetPosition, (
        OUT INT32 *X,
        OUT INT32 *Y))

    /* Set monochrome cursor image
     * AND mask: 1 = use pixel, 0 = transparent
     * XOR mask: 1 = invert, 0 = black
     */
    ANX_IFACE_METHOD(HRESULT, SetMonoCursor, (
        IN CONST UINT8 *AndMask,
        IN CONST UINT8 *XorMask,
        IN UINT32 Width,
        IN UINT32 Height,
        IN INT32 HotSpotX,
        IN INT32 HotSpotY))

    /* Set color cursor image
     * Data is in RGBA format (4 bytes per pixel)
     */
    ANX_IFACE_METHOD(HRESULT, SetColorCursor, (
        IN CONST UINT8 *Data,
        IN UINT32 Width,
        IN UINT32 Height,
        IN INT32 HotSpotX,
        IN INT32 HotSpotY))

    /* Get cursor descriptor */
    ANX_IFACE_METHOD(HRESULT, GetDescriptor, (
        OUT FB_CURSOR_DESC *Descriptor))

    /* Check if hardware cursor is supported */
    ANX_IFACE_METHOD(HRESULT, IsHardwareCursor, (
        OUT BOOLEAN *IsHardware))

ANX_END_INTERFACE(IFramebufferCursor)

/* --------------------------------------------------------------- */
/*  Helper Macros for C (COBJMACROS style)                          */
/* --------------------------------------------------------------- */

#ifndef __cplusplus

#define IFramebufferCursor_SetVisible(This, Visible) \
    ((This)->lpVtbl->SetVisible(This, Visible))
#define IFramebufferCursor_IsVisible(This, Visible) \
    ((This)->lpVtbl->IsVisible(This, Visible))
#define IFramebufferCursor_SetPosition(This, X, Y) \
    ((This)->lpVtbl->SetPosition(This, X, Y))
#define IFramebufferCursor_GetPosition(This, X, Y) \
    ((This)->lpVtbl->GetPosition(This, X, Y))
#define IFramebufferCursor_SetMonoCursor(This, And, Xor, W, H, HX, HY) \
    ((This)->lpVtbl->SetMonoCursor(This, And, Xor, W, H, HX, HY))
#define IFramebufferCursor_SetColorCursor(This, Data, W, H, HX, HY) \
    ((This)->lpVtbl->SetColorCursor(This, Data, W, H, HX, HY))
#define IFramebufferCursor_GetDescriptor(This, Desc) \
    ((This)->lpVtbl->GetDescriptor(This, Desc))
#define IFramebufferCursor_IsHardwareCursor(This, IsHw) \
    ((This)->lpVtbl->IsHardwareCursor(This, IsHw))

#endif /* !__cplusplus */

/* --------------------------------------------------------------- */
/*  Standard Cursor Shapes                                          */
/* --------------------------------------------------------------- */

/* Standard arrow cursor (monochrome) */
extern CONST UINT8 gStandardArrowCursorAnd[16 * 16 / 8];
extern CONST UINT8 gStandardArrowCursorXor[16 * 16 / 8];

/* Standard I-beam cursor (monochrome) */
extern CONST UINT8 gStandardIBeamCursorAnd[16 * 16 / 8];
extern CONST UINT8 gStandardIBeamCursorXor[16 * 16 / 8];

/* Standard wait/hourglass cursor (monochrome) */
extern CONST UINT8 gStandardWaitCursorAnd[16 * 16 / 8];
extern CONST UINT8 gStandardWaitCursorXor[16 * 16 / 8];

/* Standard crosshair cursor (monochrome) */
extern CONST UINT8 gStandardCrosshairCursorAnd[16 * 16 / 8];
extern CONST UINT8 gStandardCrosshairCursorXor[16 * 16 / 8];

/* --------------------------------------------------------------- */
/*  Cursor Constructor                                              */
/* --------------------------------------------------------------- */

/*
 * Create a cursor for a backend.
 * The cursor is initially hidden with a default arrow shape.
 */
IFramebufferCursor *
FbCreateCursor(
    IN IFramebufferBackend *Backend
    );
